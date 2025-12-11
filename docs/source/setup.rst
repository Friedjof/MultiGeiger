Setup
-----

Recommended toolchain: PlatformIO CLI (via the included ``Makefile``).

1. Clone the repository and install PlatformIO (`pio`) and ``make`` in your ``PATH``.
2. Prepare your local configuration:

   .. code-block:: bash

      make setup  # copies src/config/config.default.hpp -> src/config/config.hpp

   Adjust ``src/config/config.hpp`` to your hardware. Important switches:

   - ``TUBE_TYPE``: SBM20, SBM19, Si22G.
   - Network targets: ``SEND2LORA``, ``SEND2BLE``.
   - UI/alarms: ``SHOW_DISPLAY``, ``SPEAKER_TICK``, ``LED_TICK``, ``LOCAL_ALARM_SOUND``, ``LOCAL_ALARM_THRESHOLD``, ``LOCAL_ALARM_FACTOR``.
   
   .. note::

      Direct HTTP uploads to sensor.community / madavi.de have been removed. Use BLE, MQTT, or TTN/LoRaWAN forwarding instead.

3. Build/flash/monitor with the provided targets (default PlatformIO environment ``geiger`` uses the Heltec Wireless Stick board definition):

   - ``make build`` – compile
   - ``make flash`` – upload firmware
   - ``make monitor`` – serial console (115200 Baud)
   - ``make run`` – flash + monitor

   You can override the PlatformIO environment with ``ENV=<name>``.

Hardware variants
#################

The firmware auto-detects LoRa hardware via ``HWTESTPIN`` and adjusts the display layout and DIP switch pinout accordingly. The two supported Heltec modules are:

-  **Heltec WiFi Kit 32**: WiFi-only, larger display.
-  **Heltec Wireless Stick**: WiFi + LoRa, small display.

Runtime DIP switches (read once at boot) combine with your compile-time flags:

- SW0 ``speaker_on``: enable tick/alarm if ``SPEAKER_TICK`` allows it.
- SW1 ``display_on``: enable OLED if ``SHOW_DISPLAY`` is true.
- SW2 ``led_on``: enable white LED tick if ``LED_TICK`` is true.
- SW3 ``ble_on``: enable BLE advertising if ``SEND2BLE`` is true.

Procedure after startup
#######################

The device establishes its own WiFi access point (AP) on every boot. The SSID of the AP is **ESP32-xxxxxx**, where the xxx are the chip ID (or MAC address) of the WiFi chip (example: **ESP32-51564452**). **Please write down this number, it will be needed later.**

**New WiFi behavior (firmware 1.18+):**

- The AP opens **automatically on every boot** for 30 seconds, regardless of whether WiFi credentials are configured.
- If a client connects to the AP, the AP **remains open indefinitely** until the client disconnects.
- When the client disconnects and WiFi STA credentials are configured, the device **automatically switches to STA mode**.
- If no client connects within 30 seconds and WiFi STA is configured, the device switches to STA mode automatically.
- This gives you a 30-second window to reconfigure WiFi on every boot, even if WiFi credentials are already set.

Configuring the device via WiFi
###############################

After the WiFi AP of the device appears on your cell phone or computer, connect to it. The connection asks for a password, it is **ESP32Geiger**.

**Captive Portal (firmware 1.18+):**

The device now features a **captive portal** that automatically redirects you to the configuration page when you connect to the AP. On most devices (smartphones, tablets, laptops), a browser window will automatically open showing the configuration page. If this doesn't happen automatically, you can manually navigate to **192.168.4.1** in your browser.

The configuration page will open automatically, showing all available settings.


The settings page has the following lines: 

-  Geiger accesspoint SSID
   This is the SSID of the built-in AP and can be changed. If the sensor was already registered with this number at sensor.community, a new registration is mandatory.
-  Geiger accesspoint password
   This is the password for the built-in AP. It **MUST** be changed the first time. If desired, the default password **ESP32Geiger** can be used again. The field must not be left blank. Save the password to your favourite password manager.
-  Admin user
   Fixed username: **admin**. The admin password is the same as the AP password above. There is no separate HTTP Basic Auth field in the portal; change the AP password to change the web login password.
-  WiFi client SSID
   Here you have to enter the SSID of the WLAN you want to connect for network/internet access. 
-  WiFi client password And here the corresponding password.

For more security, it is recommended to use a separate WiFi network (e.g. guest network) to ensure an isolated communication from the normal network.

If everything is entered, press **Apply** and the data are stored in the internal EEPROM. Leave this page via **Cancel**, because only in this way the program closes the Config-Mode and connects to the local WiFi network.
If there is no **Cancel** Button, go back to the WiFi settings
of the device and type in the normal home network parameters again.

**CAUTION**. **When updating to version 1.13, the WiFi settings must be
re-entered**. In future versions this step shall become obsolete.

Furthermore, the following options can be defined on the settings page:

-  Speaker tick, LED tick and display on/off. **Note (firmware 1.18+):** These settings are now **applied immediately** after saving, without requiring a reboot. The settings respect DIP switch states (SW0=speaker, SW1=display, SW2=LED).
-  MQTT broker configuration (host, port, TLS, credentials, topics)
-  If LoRa hardware is available: the LoRa parameters (DEVEUI, APPEUI
   and APPKEY) can be entered here.


The firmware on the MultiGeiger can be updated with the link **Firmware update** at the End of the settings page. Download the .bin file, select it via **Browse…** and click **Update**. It will take roughly 30sec for uploading and flashing the firmware. If you see **Update Success! Rebooting…**, the MultiGeiger will reboot and the new firmware will be active.

If **Update error: …** appears, the update did not work. The previous firmware is still active.

The settings page can be called up from your own WiFi at any time. To do this, just enter in the address bar of the browser: http://esp32-xxxxxxx (xxxxx is the chip ID – see above). 
If it does not work with this hostname, use the IP address of the Geiger counter instead. The Ip address can be found in the devices list in your router.
If successful, the login page appears. 
Enter **admin** as username and the chosen password (see above). Now you will see the settings page as described.

Server for measured data
########################

Direct HTTP uploads to external services (sensor.community, madavi.de) have been removed. Use BLE, MQTT, or LoRa/TTN to export your measurements to your own backend.

Login to sensor.community
#########################

Legacy-only: registering devices at sensor.community is no longer required because the firmware no longer uploads data there.
