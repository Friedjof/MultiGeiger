#!/usr/bin/env python3
"""
MultiGeiger MQTT Data Logger

Simple MQTT subscriber that logs MultiGeiger data to SQLite database.
Reads configuration from .env file or environment variables.

Setup:
    1. Copy .env.example to .env
    2. Edit .env with your MQTT broker settings
    3. Run: uv run mqtt_logger.py

For crontab:
    */5 * * * * cd /path/to/tools && /usr/bin/env uv run mqtt_logger.py --oneshot
"""

import sqlite3
import argparse
import json
import sys
import os
import signal
import logging
from datetime import datetime
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


class MultiGeigerLogger:
    def __init__(self, db_path=None, base_topic=None):
        # Load .env file
        load_dotenv()

        # Setup logging first
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

        # Main measurements table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS measurements (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                device_timestamp INTEGER,
                count_rate_cps REAL,
                dose_rate_uSvph REAL,
                counts INTEGER,
                cpm INTEGER,
                dt_ms INTEGER,
                hv_pulses INTEGER,
                accum_counts INTEGER,
                accum_time_ms INTEGER,
                accum_rate_cps REAL,
                accum_dose_uSvph REAL,
                tube_type TEXT,
                tube_id INTEGER
            )
        ''')

        # Environmental data table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS environmental (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                temperature REAL,
                humidity REAL,
                pressure REAL
            )
        ''')

        # Status table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS status (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                status_json TEXT,
                wifi_status INTEGER,
                mqtt_connected BOOLEAN
            )
        ''')

        # Create indexes for faster queries
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_measurements_timestamp ON measurements(timestamp)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_environmental_timestamp ON environmental(timestamp)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_status_timestamp ON status(timestamp)')

        self.conn.commit()
        self.logger.info(f"Database initialized: {self.db_path.absolute()}")

    def on_connect(self, client, userdata, flags, rc):
        """Callback when connected to MQTT broker."""
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

    def on_message(self, client, userdata, msg):
        """Callback when message is received."""
        topic = msg.topic
        payload = msg.payload.decode('utf-8')

        # Extract metric name from topic
        if topic.startswith(f"{self.base_topic}/live/"):
            metric = topic[len(f"{self.base_topic}/live/"):]
            self.handle_live_metric(metric, payload)
        elif topic == f"{self.base_topic}/status":
            self.handle_status(payload)

    def handle_live_metric(self, metric, value):
        """Store individual live metric."""
        cursor = self.conn.cursor()

        try:
            # Try to convert to appropriate type
            if metric in ['count_rate_cps', 'dose_rate_uSvph', 'accum_rate_cps', 'accum_dose_uSvph',
                         'temperature', 'humidity', 'pressure']:
                value = float(value)
            elif metric in ['counts', 'cpm', 'dt_ms', 'hv_pulses', 'accum_counts',
                           'accum_time_ms', 'tube_id', 'timestamp']:
                value = int(value)
            # tube_type stays as string

        except ValueError:
            pass  # Keep as string if conversion fails

        # Store environmental data separately
        if metric == 'temperature':
            cursor.execute('INSERT INTO environmental (temperature) VALUES (?)', (value,))
            self.conn.commit()
            self.logger.debug(f"Temperature: {value}°C")

        elif metric == 'humidity':
            cursor.execute('UPDATE environmental SET humidity = ? WHERE id = (SELECT MAX(id) FROM environmental)', (value,))
            self.conn.commit()
            self.logger.debug(f"Humidity: {value}%")

        elif metric == 'pressure':
            cursor.execute('UPDATE environmental SET pressure = ? WHERE id = (SELECT MAX(id) FROM environmental)', (value,))
            self.conn.commit()
            self.logger.debug(f"Pressure: {value} hPa")

        # Store main measurements
        elif metric in ['count_rate_cps', 'dose_rate_uSvph', 'counts', 'cpm', 'dt_ms',
                       'hv_pulses', 'accum_counts', 'accum_time_ms', 'accum_rate_cps',
                       'accum_dose_uSvph', 'tube_type', 'tube_id', 'timestamp']:

            # Get or create current measurement row
            cursor.execute('SELECT id FROM measurements ORDER BY id DESC LIMIT 1')
            row = cursor.fetchone()

            if row:
                # Update existing row
                cursor.execute(f'UPDATE measurements SET {metric} = ? WHERE id = ?', (value, row[0]))
            else:
                # Create new row
                cursor.execute(f'INSERT INTO measurements ({metric}) VALUES (?)', (value,))

            self.conn.commit()

            if metric == 'cpm':
                self.logger.info(f"CPM: {value}")
            elif metric == 'dose_rate_uSvph':
                self.logger.info(f"Dose Rate: {value} µSv/h")

    def handle_status(self, payload):
        """Store status JSON."""
        try:
            status_data = json.loads(payload)
            wifi_status = status_data.get('wifi_status')
            mqtt_connected = status_data.get('mqtt_connected')

            cursor = self.conn.cursor()
            cursor.execute('''
                INSERT INTO status (status_json, wifi_status, mqtt_connected)
                VALUES (?, ?, ?)
            ''', (payload, wifi_status, mqtt_connected))
            self.conn.commit()

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

    parser = argparse.ArgumentParser(
        description='MultiGeiger MQTT Data Logger',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('--broker', help='MQTT broker (default: from .env)')
    parser.add_argument('--port', type=int, help='MQTT port (default: from .env)')
    parser.add_argument('--base-topic', help='Base topic (default: from .env)')
    parser.add_argument('--username', help='MQTT username (default: from .env)')
    parser.add_argument('--password', help='MQTT password (default: from .env)')
    parser.add_argument('--db', help='Database file (default: from .env)')
    parser.add_argument('--client-id', help='MQTT client ID (default: from .env)')
    parser.add_argument('--oneshot', action='store_true',
                       help='Run once and exit (for crontab)')

    args = parser.parse_args()

    # Get config from args or environment
    broker = args.broker or os.getenv('MQTT_BROKER')
    port = args.port or int(os.getenv('MQTT_PORT', '1883'))
    base_topic = args.base_topic or os.getenv('MQTT_BASE_TOPIC')
    username = args.username or os.getenv('MQTT_USERNAME')
    password = args.password or os.getenv('MQTT_PASSWORD')
    db_path = args.db or os.getenv('DB_PATH', 'multigeiger_data.db')
    client_id = args.client_id or os.getenv('MQTT_CLIENT_ID', 'multigeiger-logger')

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

    # Setup MQTT client
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

        if args.oneshot:
            # For crontab: connect, wait for one message cycle, then exit
            logger.info("One-shot mode: waiting for message cycle...")
            client.loop_start()
            import time
            time.sleep(60)  # Wait 60 seconds for messages
            client.loop_stop()
            logger.info("One-shot complete")
        else:
            # Normal mode: run forever
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
