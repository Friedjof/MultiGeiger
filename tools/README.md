# MultiGeiger Tools

Collection of tools for working with MultiGeiger data.

## TTN Data Fetcher

Tools to fetch and store MultiGeiger LoRaWAN data from The Things Network (TTN v3).

**Location:** `ttn_fetcher/`

**Components:**
- `fetch_ttn_data.py` - One-time data fetcher (CLI tool)
- `ttn_daemon.py` - Background daemon with SQLite storage
- `ttn-fetcher.service` - systemd service file

**Quick Start:**
```bash
cd ttn_fetcher

# Install dependencies
pip install -r requirements.txt

# Create config
cp ttn_config.example.json ttn_config.json
nano ttn_config.json  # Add your TTN credentials

# Fetch data once
python3 fetch_ttn_data.py --config ttn_config.json

# Run as daemon (polls every 5 minutes)
python3 ttn_daemon.py --config ttn_config.json --interval 300
```

**Features:**
- Fetch uplink messages from TTN v3 Storage Integration API
- Store data to SQLite database with automatic deduplication
- Parse decoded payload (GM counts, CPM, CPS, tube info, gateway metadata)
- Export to JSON or CSV
- Run as systemd service (daemon mode)
- Query historical data with SQL

See [ttn_fetcher/README.md](ttn_fetcher/README.md) for full documentation including:
- systemd service installation
- Database schema and SQL examples
- Grafana integration
- Troubleshooting guide

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
