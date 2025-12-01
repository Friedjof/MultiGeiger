# MultiGeiger MQTT Logger

Python daemon to log MultiGeiger MQTT data to SQLite database.

## Data Model

Simple design: **One table per metric**, each with only `(timestamp, value)`.

Tables created:
- `count_rate_cps`, `dose_rate_uSvph`, `cpm`, `counts`, `dt_ms`
- `hv_pulses`, `accum_counts`, `accum_time_ms`, `accum_rate_cps`, `accum_dose_uSvph`
- `tube_type`, `tube_id`, `device_timestamp`
- `temperature`, `humidity`, `pressure`
- `status` (JSON data)

## Quick Start

**1. Installation**

```bash
cd tools/mqtt_logger
cp .env.example .env
nano .env  # Configure MQTT broker
uv sync
```

**2. Run logger**

```bash
uv run mqtt_logger.py
```

**3. Query data**

```bash
uv run query_data.py
```

## Configuration (.env)

```bash
# MQTT Broker
MQTT_BROKER=192.168.1.100
MQTT_PORT=1883
MQTT_BASE_TOPIC=ESP32-51564452

# Optional: Authentication
MQTT_USERNAME=user
MQTT_PASSWORD=pass

# Database
DB_PATH=multigeiger_data.db

# Logging
LOG_LEVEL=INFO  # DEBUG, INFO, WARNING, ERROR
```

## Systemd Service

Run logger as background daemon:

**1. Create service file:**

```bash
sudo nano /etc/systemd/system/multigeiger-logger.service
```

**2. Add configuration:**

```ini
[Unit]
Description=MultiGeiger MQTT Logger
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/home/your-user/MultiGeiger/tools/mqtt_logger
ExecStart=/usr/bin/env uv run mqtt_logger.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

**3. Enable and start:**

```bash
sudo systemctl enable multigeiger-logger
sudo systemctl start multigeiger-logger
sudo systemctl status multigeiger-logger
```

**View logs:**

```bash
sudo journalctl -u multigeiger-logger -f
```

## Visual Reports

**24-hour report:**
```bash
uv run report.py
```

**7-day report:**
```bash
uv run report.py --days 7
```

**Custom timeframe:**
```bash
uv run report.py --hours 48
```

**Save to file:**
```bash
uv run report.py --days 7 --save report.png
```

## Query Examples

**Show all tables:**
```bash
uv run query_data.py --tables
```

**Latest values from all metrics:**
```bash
uv run query_data.py --all
```

**Specific metric (last 20):**
```bash
uv run query_data.py --metric cpm --limit 20
```

**Export metric to CSV:**
```bash
uv run query_data.py --metric dose_rate_uSvph --export dose_rate.csv
```

**Direct SQLite access:**
```bash
sqlite3 multigeiger_data.db "SELECT * FROM cpm ORDER BY timestamp DESC LIMIT 10"
```

## Troubleshooting

**uv not found:**
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

**Connection issues:**
```bash
LOG_LEVEL=DEBUG uv run mqtt_logger.py
```

**Stop logger:**
```bash
# If running in terminal: Ctrl+C
# If systemd service:
sudo systemctl stop multigeiger-logger
```

**Database locked:**
```bash
# Find process
ps aux | grep mqtt_logger
kill <pid>
```
