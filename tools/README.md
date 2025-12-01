# MultiGeiger MQTT Logger

Python-Tool zum Aufzeichnen von MultiGeiger MQTT-Daten in SQLite-Datenbank.

## Schnellstart

**1. Installation**

```bash
cd tools
cp .env.example .env
nano .env  # MQTT-Zugangsdaten eintragen
uv sync
```

**2. Logger starten**

```bash
uv run mqtt_logger.py
```

**3. Daten anschauen**

```bash
uv run query_data.py
```

## Konfiguration (.env)

Alle Einstellungen werden in `.env` gespeichert:

```bash
# MQTT Broker
MQTT_BROKER=192.168.1.100
MQTT_PORT=1883
MQTT_BASE_TOPIC=ESP32-51564452

# Optional: Authentifizierung
MQTT_USERNAME=user
MQTT_PASSWORD=pass

# Datenbank
DB_PATH=multigeiger_data.db

# Logging
LOG_LEVEL=INFO  # DEBUG, INFO, WARNING, ERROR
```

## Crontab-Integration

Für automatisches Logging alle 5 Minuten:

**1. Crontab öffnen:**

```bash
crontab -e
```

**2. Eintrag hinzufügen:**

```cron
# MultiGeiger MQTT Logger - alle 5 Minuten
*/5 * * * * cd /home/user/MultiGeiger/tools && /usr/bin/env uv run mqtt_logger.py --oneshot >> /tmp/mqtt_logger.log 2>&1
```

**3. Pfad anpassen:**
- `/home/user/MultiGeiger/tools` → Dein tools-Verzeichnis
- `/tmp/mqtt_logger.log` → Gewünschte Log-Datei

**Alternative: Systemd Service**

Für Dauerbetrieb als Service:

```bash
# /etc/systemd/system/multigeiger-logger.service
[Unit]
Description=MultiGeiger MQTT Logger
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/home/user/MultiGeiger/tools
ExecStart=/usr/bin/env uv run mqtt_logger.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Aktivieren:
```bash
sudo systemctl enable multigeiger-logger
sudo systemctl start multigeiger-logger
sudo systemctl status multigeiger-logger
```

## Daten-Abfrage

**Alle Statistiken:**
```bash
uv run query_data.py
```

**Letzte 20 Messungen:**
```bash
uv run query_data.py --latest 20
```

**Zusammenfassung:**
```bash
uv run query_data.py --summary
```

**CSV-Export:**
```bash
uv run query_data.py --export data.csv
```

**Direkt mit SQLite:**
```bash
sqlite3 multigeiger_data.db "SELECT * FROM measurements ORDER BY timestamp DESC LIMIT 10"
```

## Datenbank-Schema

**measurements** - Strahlungsmesswerte
- `cpm` - Counts per Minute
- `dose_rate_uSvph` - Dosisleistung (µSv/h)
- `counts` - Gezählte Impulse
- `hv_pulses` - HV-Impulse
- `accum_*` - Akkumulierte Werte

**environmental** - Umweltsensoren
- `temperature` (°C)
- `humidity` (%)
- `pressure` (hPa)

**status** - Gerätestatus (JSON)

## Troubleshooting

**uv nicht gefunden:**
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

**Verbindungsprobleme:**
```bash
# Debugging aktivieren
LOG_LEVEL=DEBUG uv run mqtt_logger.py
```

**Datenbank locked:**
```bash
# Prozesse prüfen
ps aux | grep mqtt_logger
kill <pid>
```

**Crontab funktioniert nicht:**
```bash
# Log-Datei prüfen
tail -f /tmp/mqtt_logger.log

# Manuell testen
cd /path/to/tools && uv run mqtt_logger.py --oneshot
```

## Tipps

- `.env` in `.gitignore` eintragen (bereits vorhanden)
- Für Langzeit-Logging: Systemd Service nutzen
- Log-Rotation einrichten bei viel Traffic
- Datenbank regelmäßig sichern
