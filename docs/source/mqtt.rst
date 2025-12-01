MQTT Integration
================

The MultiGeiger firmware supports MQTT publishing (available since firmware 1.18+). This allows real-time streaming of radiation measurements and sensor data to any MQTT broker.

Configuration
-------------

MQTT can be configured via the web interface at ``/config``:

- **Send to MQTT**: Enable/disable MQTT publishing
- **MQTT host**: Broker hostname or IP address (e.g., ``mqtt.example.com`` or ``192.168.1.100``)
- **MQTT port**: Broker port (default: ``1883`` for plain, ``8883`` for TLS)
- **Use TLS**: Enable encrypted connection (PoC implementation, uses insecure mode without certificate validation)
- **Retain MQTT messages**: Set retain flag on published messages
- **MQTT username**: Authentication username (leave empty for no auth)
- **MQTT password**: Authentication password
- **Base topic**: Custom base topic prefix (optional, defaults to device name ``ESP32-XXXXXX``)

Topic Structure
---------------

All messages are published under the configured base topic. The default structure is:

.. code-block:: text

   <baseTopic>/live/<metric>
   <baseTopic>/status

Where ``<baseTopic>`` is either the configured custom base topic or the device name (e.g., ``ESP32-51564452``).

Published Topics
----------------

Live Measurement Data
~~~~~~~~~~~~~~~~~~~~~

These topics are published during each measurement cycle (every 150 seconds by default):

+-------------------------------+------------------+----------------------------------------------+
| Topic                         | Data Type        | Description                                  |
+===============================+==================+==============================================+
| ``live/count_rate_cps``       | float (3 dec)    | Current count rate in counts per second      |
+-------------------------------+------------------+----------------------------------------------+
| ``live/dose_rate_uSvph``      | float (3 dec)    | Current dose rate in µSv/h                   |
+-------------------------------+------------------+----------------------------------------------+
| ``live/counts``               | integer          | Number of GM tube counts in this interval    |
+-------------------------------+------------------+----------------------------------------------+
| ``live/cpm``                  | integer          | Counts per minute                            |
+-------------------------------+------------------+----------------------------------------------+
| ``live/dt_ms``                | integer          | Measurement interval in milliseconds         |
+-------------------------------+------------------+----------------------------------------------+
| ``live/hv_pulses``            | integer          | High voltage pulses (HV circuit health)      |
+-------------------------------+------------------+----------------------------------------------+
| ``live/accum_counts``         | integer          | Accumulated counts since boot                |
+-------------------------------+------------------+----------------------------------------------+
| ``live/accum_time_ms``        | integer          | Accumulated measurement time in milliseconds |
+-------------------------------+------------------+----------------------------------------------+
| ``live/accum_rate_cps``       | float (3 dec)    | Accumulated average rate in CPS              |
+-------------------------------+------------------+----------------------------------------------+
| ``live/accum_dose_uSvph``     | float (3 dec)    | Accumulated average dose rate in µSv/h       |
+-------------------------------+------------------+----------------------------------------------+
| ``live/tube_type``            | string           | Geiger-Müller tube type (e.g., "Si22G")      |
+-------------------------------+------------------+----------------------------------------------+
| ``live/tube_id``              | integer          | Tube type ID number                          |
+-------------------------------+------------------+----------------------------------------------+
| ``live/timestamp``            | integer          | UTC Unix timestamp                           |
+-------------------------------+------------------+----------------------------------------------+

Environmental Sensor Data (Optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If a BME280/BME680 sensor is connected, these additional topics are published:

+-------------------------------+------------------+----------------------------------------------+
| Topic                         | Data Type        | Description                                  |
+===============================+==================+==============================================+
| ``live/temperature``          | float (2 dec)    | Temperature in degrees Celsius (°C)          |
+-------------------------------+------------------+----------------------------------------------+
| ``live/humidity``             | float (2 dec)    | Relative humidity in percent (%)             |
+-------------------------------+------------------+----------------------------------------------+
| ``live/pressure``             | float (2 dec)    | Atmospheric pressure in hectopascals (hPa)   |
+-------------------------------+------------------+----------------------------------------------+

Status Information
~~~~~~~~~~~~~~~~~~

+-------------------------------+------------------+----------------------------------------------+
| Topic                         | Data Type        | Description                                  |
+===============================+==================+==============================================+
| ``status``                    | JSON object      | Complete status information (see below)      |
+-------------------------------+------------------+----------------------------------------------+

The ``status`` topic contains a JSON object with all current measurements and system status:

.. code-block:: json

   {
     "wifi_status": 1,
     "mqtt_connected": true,
     "last_publish_ms": 123456,
     "counts": 42,
     "cpm": 17,
     "hv_pulses": 150,
     "dt_ms": 150000,
     "have_thp": true,
     "timestamp": "2025-01-15T12:34:56Z"
   }

**JSON Fields:**

- ``wifi_status``: WiFi connection status code (0=off, 1=connected, 2=error, 3=connecting, 4=AP mode)
- ``mqtt_connected``: MQTT broker connection status (boolean)
- ``last_publish_ms``: Milliseconds since last successful publish
- ``counts``: GM tube counts in this measurement
- ``cpm``: Counts per minute
- ``hv_pulses``: High voltage pulses
- ``dt_ms``: Measurement interval in milliseconds
- ``have_thp``: Whether environmental sensor data is available (boolean)
- ``timestamp``: UTC timestamp string

Example Configuration
---------------------

Home Assistant
~~~~~~~~~~~~~~

Here's an example Home Assistant MQTT sensor configuration for the MultiGeiger:

.. code-block:: yaml

   mqtt:
     sensor:
       - name: "MultiGeiger Dose Rate"
         state_topic: "ESP32-51564452/live/dose_rate_uSvph"
         unit_of_measurement: "µSv/h"
         value_template: "{{ value | float }}"
         
       - name: "MultiGeiger CPM"
         state_topic: "ESP32-51564452/live/cpm"
         unit_of_measurement: "CPM"
         value_template: "{{ value | int }}"
         
       - name: "MultiGeiger Temperature"
         state_topic: "ESP32-51564452/live/temperature"
         unit_of_measurement: "°C"
         device_class: temperature
         value_template: "{{ value | float }}"

Node-RED
~~~~~~~~

Example Node-RED flow to subscribe and process MultiGeiger MQTT data:

.. code-block:: json

   [
     {
       "id": "mqtt-in",
       "type": "mqtt in",
       "topic": "ESP32-51564452/live/#",
       "broker": "mqtt-broker-id",
       "name": "MultiGeiger Data"
     }
   ]

Troubleshooting
---------------

**MQTT not publishing:**

1. Check that "Send to MQTT" is enabled in the web config
2. Verify MQTT broker hostname/IP and port are correct
3. Check authentication credentials if your broker requires auth
4. Monitor serial output for MQTT connection status messages
5. Verify WiFi is connected (MQTT requires active WiFi connection)

**TLS connection fails:**

- The current TLS implementation is a proof-of-concept using insecure mode (no certificate validation)
- Some brokers may reject insecure TLS connections
- For production use, consider using plain MQTT or setting up proper certificate validation

**Messages not retained:**

- Enable "Retain MQTT messages" in the web configuration
- Note that retained messages persist on the broker until explicitly cleared

QoS and Reliability
-------------------

- Current implementation uses configurable QoS (Quality of Service) level
- Default QoS can be set in configuration
- Messages are published every measurement cycle (default: 150 seconds)
- If MQTT broker is unreachable, messages are not queued (they are skipped)
- Connection is automatically retried every 5 seconds

Technical Notes
---------------

- **Client ID**: ``MultiGeiger-<baseTopic>`` (slashes removed)
- **Buffer Size**: 512 bytes
- **Reconnect Interval**: 5 seconds
- **TLS Mode**: Insecure (skips certificate validation) - PoC only
- **Message Format**: Simple value strings for individual metrics, JSON for status
- **Timestamp Format**: Unix epoch time (seconds since 1970-01-01 00:00:00 UTC)
