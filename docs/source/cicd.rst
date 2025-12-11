.. include:: global.rst.inc

CI/CD and Firmware Delivery
===========================

Workflows
---------

- **Build (`.github/workflows/build.yml`)**: runs on every push/PR (master/main/develop) and on manual dispatch. Steps: install PlatformIO, create ``src/config/config.hpp`` from the default via ``tools/prepare_config.sh``, embed web assets, build ``geiger``, report size, upload artifacts.
- **Release (`.github/workflows/release.yml`)**: triggered by tags (or manual). Performs the same build and then packages binaries to ``binaries/geiger/`` and publishes them as release assets.

Getting firmware from Actions artifacts
---------------------------------------

1. In GitHub, open **Actions** → select a **Build** run → **Artifacts** → download ``firmware-geiger-<sha>.zip``.
2. The archive contains:

   - ``firmware.bin`` — flashable binary
   - ``firmware.elf`` — symbols for debugging

3. Flash the binary, for example with esptool (adjust port/baud):

   .. code-block:: bash

      esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
        write_flash 0x10000 firmware.bin

   Or via PlatformIO:

   .. code-block:: bash

      pio run -e geiger -t upload --upload-port /dev/ttyUSB0 --upload-speed 460800

Getting firmware from Releases
------------------------------

1. Open the **Releases** page and pick the desired tag.
2. Download the assets from ``binaries/geiger/`` (same ``firmware.bin/.elf`` as in the artifacts).
3. Flash using the same esptool or PlatformIO commands as above.

Notes
-----

- ``tools/prepare_config.sh`` auto-generates ``src/config/config.hpp`` from the default; adjust configuration locally if you build yourself.
- ``scripts/web_to_header.py`` runs on each build (via ``make web``) so firmware always contains the current Vite-built web UI.
