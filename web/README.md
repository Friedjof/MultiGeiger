# MultiGeiger Web Dashboard

Modernes Live-Dashboard für den MultiGeiger Strahlungsdetektor.

## 🎨 Features

- **Live-Daten**: Automatische Aktualisierung alle 2 Sekunden
- **Modernes Design**: Eigenes CSS ohne Bootstrap
- **Responsive**: Funktioniert auf Desktop und Mobile
- **Dark Theme**: Augenfreundliches dunkles Design
- **Komprimiert**: Gzip-Kompression für minimalen Speicherverbrauch

## 📁 Struktur

```
web/
├── dashboard.html    # Haupt-HTML-Datei
├── style.css         # Eigenes CSS (kein Bootstrap!)
├── app.js           # JavaScript für Live-Updates
└── README.md        # Diese Datei
```

## 🛠️ Entwicklung

### Lokale Vorschau

1. **Mit Python HTTP Server**:
   ```bash
   cd web
   python3 -m http.server 8000
   ```
   Dann öffne: http://localhost:8000/dashboard.html

2. **Mit Live Server (VS Code Extension)**:
   - Installiere "Live Server" Extension
   - Rechtsklick auf `dashboard.html` → "Open with Live Server"

### Mock-Daten für Entwicklung

Erstelle eine Datei `mock-api.json` im `web/` Verzeichnis:

```json
{
  "counts": 1234,
  "cpm": 45.2,
  "dose_uSvh": 0.123,
  "uptime_s": 3600,
  "hv_error": false,
  "temperature": 22.5,
  "humidity": 65.0,
  "pressure": 1013.2,
  "has_thp": true
}
```

Dann ändere in `app.js` temporär:
```javascript
const API_ENDPOINT = '/mock-api.json';  // Statt '/api/status'
```

## 🚀 Deployment auf ESP32

### 1. Web-Assets einbetten

Führe das Python-Script aus, um die Dateien zu komprimieren und in C-Code zu konvertieren:

```bash
cd /path/to/MultiGeiger
python3 tools/embed_web.py
```

Das Script:
- Liest alle Dateien aus `web/`
- Komprimiert sie mit gzip
- Generiert `src/comm/wifi/web_assets.h` mit C-Arrays

### 2. In ESP32-Code einbinden

Die Datei `web_assets.h` wird automatisch generiert. Verwende sie so:

```cpp
#include "web_assets.h"

// In deiner handleRoot-Funktion:
server.sendHeader("Content-Encoding", "gzip");
server.send_P(200, "text/html", (const char*)dashboard_html_gz, dashboard_html_gz_len);
```

### 3. Build & Flash

```bash
make clean && make
make flash
```

## 📊 API-Endpunkt

Das Dashboard erwartet einen `/api/status` Endpunkt mit folgendem JSON:

```json
{
  "counts": 1234,           // Gesamtzahl der Counts
  "cpm": 45.2,             // Counts per Minute
  "dose_uSvh": 0.123,      // Dosisrate in µSv/h
  "uptime_s": 3600,        // Uptime in Sekunden
  "hv_error": false,       // Hochspannungsfehler
  "temperature": 22.5,     // Temperatur in °C (optional)
  "humidity": 65.0,        // Luftfeuchtigkeit in % (optional)
  "pressure": 1013.2,      // Luftdruck in hPa (optional)
  "has_thp": true          // Sind THP-Sensoren verfügbar?
}
```

## 🎨 Anpassungen

### Farben ändern

In `style.css` die CSS-Variablen anpassen:

```css
:root {
    --primary: #2563eb;      /* Primärfarbe */
    --success: #10b981;      /* Erfolgsfarbe */
    --danger: #ef4444;       /* Fehlerfarbe */
    /* ... */
}
```

### Update-Intervall ändern

In `app.js`:

```javascript
const UPDATE_INTERVAL = 2000; // Millisekunden
```

## 📝 Hinweise

- **Kein Bootstrap**: Alles mit eigenem CSS für minimale Größe
- **PROGMEM**: Assets werden im Flash gespeichert, nicht im RAM
- **Gzip**: ~70% Kompression der Dateigröße
- **Responsive**: Mobile-First Design

## 🐛 Debugging

1. **Browser-Konsole**: F12 → Console für JavaScript-Fehler
2. **Network-Tab**: Zeigt API-Anfragen und Antworten
3. **Serial Monitor**: ESP32-Logs mit `make monitor`

## 📄 Lizenz

Teil des MultiGeiger-Projekts - siehe Haupt-Lizenz.
