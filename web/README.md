# MultiGeiger Web UI (Vite)

Ein einzelnes Vite-Frontend (plain JS) bündelt Dashboard, Einstellungen und OTA-Upload. Mock-API kann per Query-Parameter zugeschaltet werden, die Build-Pipeline erzeugt einen gzip-komprimierten C-Header.

## Struktur

```
web/
├── index.html          # Einstieg für Vite
├── src/
│   ├── main.js        # Bootstrapping
│   ├── app.js         # App-Logik (Dashboard, Config, OTA)
│   └── style.css      # UI-Styles
├── public/
│   └── mock/api.js    # Mock-API (localStorage-gestützter State)
├── vite.config.js     # Version-Define, Proxy, Build
└── package.json
```

## Entwicklung

```bash
cd web
npm install
npm run dev
```

- Mock aktivieren: `http://localhost:5173/?mock=1` (wird lokal standardmäßig genutzt, `mock=0` deaktiviert).
- Vite-Dev-Server proxyt `/api` auf `http://192.168.4.1` (siehe `vite.config.js`).

## Build & Header

```bash
# Build + Header via Make
make web

# Manuell
cd web && npm run build
python scripts/web_to_header.py web/dist -o lib/WebService/generated/web_files.h
```

Das Skript gzippt alle `dist`-Assets und erzeugt `web_files.h` mit:

```cpp
struct WebFile { const char* path; const uint8_t* data; size_t size; const char* mime_type; };
extern const WebFile webFiles[];
extern const size_t webFilesCount;
const WebFile* findWebFile(const String& path);
void sendWebFile(WebServer& server, const WebFile* file);
```

## Firmware-Serve

Im Webserver den angefragten Pfad (z. B. `server.uri()`) in `findWebFile` suchen und bei Treffer mit `sendWebFile` zurückgeben; für SPA-Routen auf `/index.html` zurückfallen. Der Make-Target `build-web` erzeugt den Header automatisch.
