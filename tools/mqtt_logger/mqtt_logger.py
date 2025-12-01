#!/usr/bin/env python3
"""
MultiGeiger MQTT Data Logger

Simple MQTT subscriber that logs MultiGeiger data to SQLite database.
Each metric gets its own table with (timestamp, value).

Setup:
    1. Copy .env.example to .env
    2. Edit .env with your MQTT broker settings
    3. Run: uv run mqtt_logger.py

For systemd:
    See README.md for service configuration
"""

import sqlite3
import json
import sys
import os
import signal
import logging
from pathlib import Path

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Error: paho-mqtt not installed. Install with: uv sync")
    sys.exit(1)

try:
    from dotenv import load_dotenv
except ImportError:
    print("Error: python-dotenv not installed. Install with: uv sync")
    sys.exit(1)


# Metric definitions: (metric_name, sql_type)
METRICS = [
    ('count_rate_cps', 'REAL'),
    ('dose_rate_uSvph', 'REAL'),
    ('counts', 'INTEGER'),
    ('cpm', 'INTEGER'),
    ('dt_ms', 'INTEGER'),
    ('hv_pulses', 'INTEGER'),
    ('accum_counts', 'INTEGER'),
    ('accum_time_ms', 'INTEGER'),
    ('accum_rate_cps', 'REAL'),
    ('accum_dose_uSvph', 'REAL'),
    ('tube_type', 'TEXT'),
    ('tube_id', 'INTEGER'),
    ('temperature', 'REAL'),
    ('humidity', 'REAL'),
    ('pressure', 'REAL'),
    ('device_timestamp', 'INTEGER'),
]


class MultiGeigerLogger:
    def __init__(self, db_path=None, base_topic=None):
        # Load .env file
        load_dotenv()

        # Setup logging
        log_level = os.getenv('LOG_LEVEL', 'INFO')
        logging.basicConfig(
            level=getattr(logging, log_level),
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        self.logger = logging.getLogger(__name__)

        # Setup paths and database
        self.db_path = Path(db_path or os.getenv('DB_PATH', 'multigeiger_data.db'))
        self.base_topic = (base_topic or os.getenv('MQTT_BASE_TOPIC', 'ESP32-51564452')).rstrip('/')
        self.conn = None
        self.setup_database()

    def setup_database(self):
        """Create database and tables if they don't exist."""
        self.conn = sqlite3.connect(self.db_path, check_same_thread=False)
        cursor = self.conn.cursor()

        # Create one table per metric
        for metric_name, sql_type in METRICS:
            cursor.execute(f'''
                CREATE TABLE IF NOT EXISTS {metric_name} (
                    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                    value {sql_type}
                )
            ''')
            # Create index for faster queries
            cursor.execute(f'CREATE INDEX IF NOT EXISTS idx_{metric_name}_timestamp ON {metric_name}(timestamp)')

        # Status table for JSON data
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS status (
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                value TEXT
            )
        ''')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_status_timestamp ON status(timestamp)')

        self.conn.commit()
        self.logger.info(f"Database initialized: {self.db_path.absolute()}")
        self.logger.info(f"Created {len(METRICS)} metric tables + status table")

    def on_connect(self, client, userdata, flags, rc, properties=None):
        """Callback when connected to MQTT broker.

        Supports both paho-mqtt v1 (4 args) and v2 (5 args) APIs.
        """
        if rc == 0:
            self.logger.info("Connected to MQTT broker")
            # Subscribe to all live topics
            topic = f"{self.base_topic}/live/#"
            client.subscribe(topic)
            self.logger.info(f"Subscribed to: {topic}")

            # Also subscribe to status
            status_topic = f"{self.base_topic}/status"
            client.subscribe(status_topic)
            self.logger.info(f"Subscribed to: {status_topic}")
        else:
            self.logger.error(f"Connection failed with code {rc}")

    def on_message(self, client, userdata, msg, properties=None):
        """Callback when message is received.

        Supports both paho-mqtt v1 (3 args) and v2 (4 args) APIs.
        """
        topic = msg.topic
        payload = msg.payload.decode('utf-8')

        # Extract metric name from topic
        if topic.startswith(f"{self.base_topic}/live/"):
            metric = topic[len(f"{self.base_topic}/live/"):]
            self.handle_metric(metric, payload)
        elif topic == f"{self.base_topic}/status":
            self.handle_status(payload)

    def handle_metric(self, metric, value):
        """Store metric value in its dedicated table."""
        # Check if this metric has a table
        metric_names = [m[0] for m in METRICS]
        if metric not in metric_names:
            self.logger.debug(f"Unknown metric: {metric}")
            return

        cursor = self.conn.cursor()

        try:
            # Convert to appropriate type
            metric_type = dict(METRICS)[metric]
            if metric_type == 'REAL':
                value = float(value)
            elif metric_type == 'INTEGER':
                value = int(value)
            # TEXT stays as string

        except ValueError:
            self.logger.warning(f"Failed to convert {metric}={value} to {metric_type}")
            return

        # Insert into metric's table
        cursor.execute(f'INSERT INTO {metric} (value) VALUES (?)', (value,))
        self.conn.commit()

        # Log important metrics
        if metric == 'cpm':
            self.logger.info(f"☢️  CPM: {value}")
        elif metric == 'dose_rate_uSvph':
            self.logger.info(f"☢️  Dose Rate: {value} µSv/h")
        elif metric == 'temperature':
            self.logger.debug(f"Temperature: {value}°C")
        elif metric == 'humidity':
            self.logger.debug(f"Humidity: {value}%")
        elif metric == 'pressure':
            self.logger.debug(f"Pressure: {value} hPa")

    def handle_status(self, payload):
        """Store status JSON."""
        cursor = self.conn.cursor()
        cursor.execute('INSERT INTO status (value) VALUES (?)', (payload,))
        self.conn.commit()

        try:
            status_data = json.loads(payload)
            wifi_status = status_data.get('wifi_status')
            mqtt_connected = status_data.get('mqtt_connected')
            self.logger.info(f"Status update - WiFi: {wifi_status}, MQTT: {mqtt_connected}")
        except json.JSONDecodeError:
            self.logger.warning(f"Invalid JSON in status: {payload}")

    def close(self):
        """Close database connection."""
        if self.conn:
            self.conn.close()
            self.logger.info("Database connection closed")


def signal_handler(signum, frame):
    """Handle termination signals gracefully."""
    print("\nReceived signal, shutting down...")
    sys.exit(0)


def main():
    # Load .env file
    load_dotenv()

    # Get config from environment
    broker = os.getenv('MQTT_BROKER')
    port = int(os.getenv('MQTT_PORT', '1883'))
    base_topic = os.getenv('MQTT_BASE_TOPIC')
    username = os.getenv('MQTT_USERNAME')
    password = os.getenv('MQTT_PASSWORD')
    db_path = os.getenv('DB_PATH', 'multigeiger_data.db')
    client_id = os.getenv('MQTT_CLIENT_ID', 'multigeiger-logger')

    if not broker:
        print("Error: MQTT_BROKER not set. Copy .env.example to .env and configure.")
        sys.exit(1)

    if not base_topic:
        print("Error: MQTT_BASE_TOPIC not set. Copy .env.example to .env and configure.")
        sys.exit(1)

    # Setup logging
    log_level = os.getenv('LOG_LEVEL', 'INFO')
    logging.basicConfig(
        level=getattr(logging, log_level),
        format='%(asctime)s - %(levelname)s - %(message)s'
    )
    logger = logging.getLogger(__name__)

    logger.info("=" * 60)
    logger.info("MultiGeiger MQTT Data Logger")
    logger.info("=" * 60)
    logger.info(f"Broker: {broker}:{port}")
    logger.info(f"Base Topic: {base_topic}")
    logger.info(f"Database: {db_path}")
    logger.info("=" * 60)

    # Initialize logger
    data_logger = MultiGeigerLogger(db_path=db_path, base_topic=base_topic)

    # Setup MQTT client (use newer API version if available)
    try:
        # paho-mqtt >= 2.0.0
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=client_id)
    except AttributeError:
        # paho-mqtt < 2.0.0 (fallback)
        client = mqtt.Client(client_id=client_id)

    client.on_connect = data_logger.on_connect
    client.on_message = data_logger.on_message

    if username:
        client.username_pw_set(username, password)

    # Setup signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    try:
        logger.info(f"Connecting to {broker}:{port}...")
        client.connect(broker, port, 60)
        logger.info("Logger started. Press Ctrl+C to stop.")
        client.loop_forever()

    except KeyboardInterrupt:
        logger.info("Stopping logger...")
        client.disconnect()
        data_logger.close()
        logger.info("Stopped. Data saved.")

    except Exception as e:
        logger.error(f"Error: {e}")
        data_logger.close()
        sys.exit(1)


if __name__ == '__main__':
    main()
