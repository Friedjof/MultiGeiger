# MultiGeiger Tools

Collection of tools for working with MultiGeiger data.

## MQTT Logger

Python daemon to log MQTT data to SQLite database.

**Location:** `mqtt_logger/`

**Quick Start:**
```bash
cd mqtt_logger
cp .env.example .env
nano .env  # Configure your MQTT broker
uv sync
uv run mqtt_logger.py
```

See [mqtt_logger/README.md](mqtt_logger/README.md) for full documentation.

## Features

- Simple data model: One table per metric with (timestamp, value)
- Runs as daemon in background
- Systemd service support
- Query and export tools included
- Uses uv package manager
- Configuration via .env file
