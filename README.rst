MultiGeiger
===========

.. image:: https://github.com/Ecocurious2/MultiGeiger/actions/workflows/build.yml/badge.svg
   :target: https://github.com/Ecocurious2/MultiGeiger/actions/workflows/build.yml
   :alt: Build Status

.. image:: https://readthedocs.org/projects/multigeiger/badge/?version=latest
   :target: https://multigeiger.readthedocs.org/
   :alt: Documentation Status

The MultiGeiger is an ESP32-based radioactivity measurement device designed for citizen science projects. It features a modern web interface, multiple connectivity options (WiFi, LoRa, BLE), and environmental sensors.

✨ Features
-----------

* **Radiation Measurement**: Accurate detection using Geiger-Müller tubes
* **Modern Web Interface**: Real-time dashboard and configuration page
* **Multiple Connectivity**: WiFi, LoRa/TTN, BLE, MQTT
* **Environmental Sensors**: Optional temperature, humidity, and pressure monitoring (BME280/BME680)
* **Data Platforms**: Automatic upload to sensor.community and madavi.de
* **Mobile-Optimized**: Responsive design for all screen sizes
* **Low Power**: Optimized for battery operation

🚀 Quick Start
--------------

**Default WiFi Credentials:**

* SSID: ``MultiGeiger-XXXXXX`` (last 6 digits of MAC address)
* Password: ``ESP32Geiger``

1. Power on your MultiGeiger device
2. Connect to the WiFi access point
3. Open http://192.168.4.1 in your browser
4. Configure your settings via the web interface

📚 Documentation
----------------

**Online Documentation** (ReadTheDocs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

https://multigeiger.readthedocs.org/ (English + Deutsch)

Comprehensive documentation with versioning and multi-language support. Use the language switcher in the lower right corner.

**Local Documentation**
~~~~~~~~~~~~~~~~~~~~~~~

Build the documentation locally:

.. code-block:: bash

   make docs

Then open ``docs/build/html/index.html`` in your browser.

**Key Documentation Files:**

* `Assembly Guide <docs/assembly/Aufbauanleitung.pdf>`_ (PDF, German)
* `Setup Instructions <docs/source/setup.rst>`_
* `LoRa/TTN Configuration <docs/source/setup_lora.rst>`_
* `Deployment Guide <docs/source/deployment.rst>`_
* `FAQ <docs/source/faq.rst>`_

🌐 Web Interface
----------------

The MultiGeiger features a modern, mobile-optimized web interface:

* **Dashboard**: Real-time radiation monitoring with live updates
* **Config Page**: Easy configuration of WiFi, MQTT, LoRa, and sensors
* **Accordion UI**: Collapsible sections for better mobile experience
* **Responsive Design**: Works on phones, tablets, and desktops

Access at: http://multigeiger.local/ or http://192.168.4.1/ (AP mode)

🔧 Development
--------------

**Requirements:**

* PlatformIO (for firmware)
* Python 3.11+ (for build tools and documentation)
* uv (Python package manager)

**Building:**

.. code-block:: bash

   # Build web frontend + firmware
   make build

   # Build web assets only
   make web

   # Flash to device
   make flash

   # Monitor serial output
   make monitor

**Project Structure:**

.. code-block:: text

   MultiGeiger/
   ├── src/              # Firmware source code
   ├── web/              # Web interface (HTML/CSS/JS)
   ├── docs/             # Documentation
   ├── tools/            # Build tools (embed_web.py)
   ├── .github/          # CI/CD workflows
   └── platformio.ini    # PlatformIO configuration

📡 Connectivity & Data Upload
------------------------------

**Supported Platforms:**

* sensor.community (luftdaten.info)
* madavi.de
* Custom MQTT brokers
* LoRaWAN/The Things Network (TTN)

**Protocols:**

* WiFi (802.11 b/g/n)
* LoRa (optional, with compatible hardware)
* Bluetooth Low Energy (BLE)
* MQTT (with TLS support)

🗺️ Community & Resources
--------------------------

* **Live Map**: https://multigeiger.citysensor.de/
* **Ecocurious Project Page**: https://ecocurious.de/projekte/multigeiger-2/ (German)
* **Video Tutorials**: https://play.wa.binary-kitchen.de/_/global/raw.githubusercontent.com/ecocurious2/rc3_2020/main/main.json (German)

🤝 Contributing
---------------

Contributions are welcome! The project uses:

* **CI/CD**: Automated builds via GitHub Actions
* **Code Quality**: All PRs are automatically built and tested
* **Documentation**: Sphinx for docs, ReStructuredText format

See `.github/README.md <.github/README.md>`_ for CI/CD information.

📄 License
----------

See `LICENSE <LICENSE>`_ file for details.

👥 Authors
----------

See `AUTHORS <AUTHORS>`_ file for contributors.

🛠️ Hardware
------------

**Supported Boards:**

* Heltec WiFi Kit 32
* Heltec Wireless Stick
* Other ESP32-based boards (with modifications)

**Required Components:**

* Geiger-Müller tube (various types supported)
* High voltage generator
* Optional: BME280/BME680 environmental sensor
* Optional: LoRa module (for TTN connectivity)

See hardware documentation in `docs/hardware/ <docs/hardware/>`_ for schematics and PCB files.

---

*Made with ❤️ by the Ecocurious community*
