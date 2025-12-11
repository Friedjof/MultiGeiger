# Web Frontend Workflow (Vite)

## Entwicklung

```bash
cd web
npm install
npm run dev
# Browser: http://localhost:5173/?mock=1  (Mock an, mock=0 aus)
```

- Plain-JS-Vite, keine Frameworks. Entry: `index.html` → `src/main.js`.
- Dev-Server proxyt `/api` → `http://192.168.4.1` (anpassbar in `vite.config.js`).

## Build → Header

```bash
make web
# oder manuell:
cd web && npm run build
python scripts/web_to_header.py web/dist -o lib/WebService/generated/web_files.h
```

- `web/dist` wird gzip-komprimiert und in `web_files.h` geschrieben.
- Struktur im Header: `webFiles[]`, `webFilesCount`, `findWebFile(path)`, `sendWebFile(server, file)`.

## Firmware-Serve

1. In HTTP-Handler angefragten Pfad (`server.uri()`) via `findWebFile` nachschlagen.
2. Bei Treffer: `sendWebFile(server, file)` (setzt `Content-Encoding: gzip`).
3. Für SPA-Routen auf `/index.html` zurückfallen.

## Make-Ziele

| Befehl     | Beschreibung                                 |
|------------|----------------------------------------------|
| `make web` | npm build + Header-Generation                |
| `make build` | Firmware build (ruft `make web` vorher)    |
| `make flash` | Firmware flashen                           |
| `make run` | Flash + Monitor                              |
| `make clean` | Build-Artefakte + Header löschen           |

## Debug-Tipps

- Browser-Konsole + Network-Tab prüfen.
- Wenn Mock aktiv ist: State liegt in `localStorage` (`multigeiger:config`).
- Build-Fehler: `web/dist` existiert? Header vorhanden? `make clean && make web`.
