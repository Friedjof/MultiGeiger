# Web Frontend Workflow

## 🔄 Entwicklungs-Workflow

### 1. Lokale Entwicklung

```bash
# Terminal 1: Starte lokalen Server
cd web
python3 -m http.server 8000

# Browser öffnen:
http://localhost:8000/dashboard.html
```

Das Dashboard nutzt automatisch Mock-Daten (`mock-api.json`) wenn lokal entwickelt wird.

### 2. Dateien bearbeiten

- `dashboard.html` - HTML-Struktur
- `style.css` - Styling (eigenes CSS!)
- `app.js` - JavaScript-Logik
- `mock-api.json` - Test-Daten

Browser lädt automatisch neu bei Änderungen (mit Live Server Extension).

### 3. Build & Deploy

```bash
# Methode 1: Nur Web-Assets neu bauen
make web

# Methode 2: Alles neu bauen (inkl. Web)
make clean && make

# Methode 3: Bauen + Flashen + Monitor
make run
```

## 📦 Was passiert beim Build?

```
make web
  ↓
tools/embed_web.py
  ↓
1. Liest web/dashboard.html, web/style.css, web/app.js
2. Komprimiert mit gzip (~70% kleiner!)
3. Konvertiert in C-Arrays
  ↓
src/comm/wifi/web_assets.h (generiert)
  ↓
make build (PlatformIO)
  ↓
firmware.bin
```

## 🎯 Make-Befehle

| Befehl | Beschreibung |
|--------|--------------|
| `make web` | Nur Web-Assets neu bauen |
| `make build` | Web + Firmware bauen |
| `make flash` | Firmware flashen |
| `make monitor` | Serial Monitor |
| `make run` | Flash + Monitor |
| `make clean` | Alles löschen |

## ⚡ Schneller Workflow

```bash
# 1. HTML/CSS/JS bearbeiten in web/
# 2. Im Browser testen (localhost:8000)
# 3. Wenn fertig:
make run
```

Der `make build` Befehl führt **automatisch** `make web` aus!

## 🔍 Debug-Tipps

### Browser
- F12 → Console für JavaScript-Fehler
- Network-Tab für API-Calls

### ESP32
```bash
make monitor
# Zeigt:
# - "Building web frontend..." beim Build
# - HTTP-Requests im Serial Monitor
# - API-Responses
```

### Build-Probleme
```bash
# Alte Artifacts löschen
make clean

# Web-Assets manuell neu generieren
python3 tools/embed_web.py

# Prüfen ob generiert wurde
ls -lh src/comm/wifi/web_assets.h
```

## 📊 Größen-Optimierung

Aktuelle Kompression (gzip):
- `dashboard.html`: 3746 → 859 bytes (77% kleiner)
- `style.css`: 4974 → 1536 bytes (69% kleiner)
- `app.js`: 5486 → 1597 bytes (71% kleiner)

**Gesamt**: ~14 KB → ~4 KB (Flash-Speicher)

## 🚀 Live-Reload (optional)

Mit VS Code "Live Server" Extension:
1. Extension installieren
2. Rechtsklick auf `dashboard.html`
3. "Open with Live Server"

Änderungen werden sofort im Browser sichtbar!
