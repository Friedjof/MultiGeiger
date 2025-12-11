.. include:: global.rst.inc

LoRa/TTN Payload
================

Overview
--------

Since firmware X.X the MultiGeiger sends Geiger + environmental data in one combined uplink on **Port 1** with a fixed length of **18 bytes**.

Payload format
--------------

.. list-table::
   :header-rows: 1
   :widths: 12 14 14 50 20

   * - Port
     - Byte(s)
     - Hex
     - Description
     - Example
   * - 1
     - 0/1/2/3
     - 00000107
     - Number of counts
     - 263
   * - 1
     - 4/5/6
     - 0249F0
     - Measurement time in ms (``sample_time_ms``)
     - 150000
   * - 1
     - 7/8
     - 10C0
     - Software version (``software_version``)
     - 1.12.0
   * - 1
     - 9
     - 16
     - Geiger tube ID (``tube``)
     - Si22G
   * - 1
     - 10/11
     - 0107
     - BMP/BME temperature x10 (``temperature``)
     - 0x0107 = 263 -> 26.3 C
   * - 1
     - 12
     - 9A
     - BME humidity x2 (``humidity``)
     - 0x9A = 154 -> 77.0% (0 for BMP280)
   * - 1
     - 13/14
     - 2794
     - BMP/BME pressure x10 (``pressure``)
     - 0x2794 = 10132 -> 1013.2 hPa
   * - 1
     - 15
     - 01
     - THP sensor type
     - 0=no sensor, 1=BMP280, 2=BME280, 3=BME680
   * - 1
     - 16/17
     - 0096
     - Gas resistance in kOhm (BME680 only)
     - 0x0096 = 150 kOhm (0 for BMP280/BME280)

Notes
-----

- If no THP sensor (BMP280/BME280/BME680) is present, bytes 10-17 are all ``0x00`` and byte 15 is ``0x00``.
- Software version encoding: top 4 bits are the major version (max 15), next 8 bits are minor, lowest 4 bits are patch.
- Implemented tube identifiers: SBM-19 (0x13), SBM-20 (0x14), Si22G (0x16).
- Supported THP sensors:

  - BMP280 (type 1): temperature + pressure only (humidity = 0, gas = 0)
  - BME280 (type 2): temperature + humidity + pressure (gas = 0)
  - BME680 (type 3): temperature + humidity + pressure + gas resistance

TTN decoder
-----------

To decode the payload in the TTN Console, add this JavaScript under **Payload formatters -> Uplink**:

.. code-block:: javascript

   function decodeUplink(input) {
     const bytes = input.bytes;
     const fPort = parseInt(input.fPort) || 0;

     if (fPort !== 1 || bytes.length !== 18) {
       return {
         data: {},
         warnings: [`fPort: ${fPort}, bytes: ${bytes.length} (expected: fPort 1, 18 bytes)`],
         errors: []
       };
     }

     // Geiger data (bytes 0-9)
     const COUNTS = bytes[0] * 0x1000000 + bytes[1] * 0x10000 + bytes[2] * 0x100 + bytes[3];
     const DT_MS = bytes[4] * 0x10000 + bytes[5] * 0x100 + bytes[6];
     const SW_VERSION = bytes[7] * 0x100 + bytes[8];
     const TUBE_NBR = bytes[9];

     // THP data (bytes 10-14)
     let TEMP_RAW = bytes[10] * 0x100 + bytes[11];
     if (TEMP_RAW & 0x8000) TEMP_RAW = TEMP_RAW - 0x10000; // signed
     const TEMP_C = TEMP_RAW !== 0 ? (TEMP_RAW / 10.0) : null;

     const HUMIDITY_RAW = bytes[12];
     const HUMIDITY_PERCENT = HUMIDITY_RAW > 0 ? (HUMIDITY_RAW / 2.0) : null;

     const PRESSURE_RAW = bytes[13] * 0x100 + bytes[14];
     const PRESSURE_HPA = PRESSURE_RAW > 0 ? (PRESSURE_RAW / 10.0) : null;

     // Sensor type (byte 15)
     const SENSOR_TYPE_RAW = bytes[15];
     const SENSOR_TYPES = ['none', 'BMP280', 'BME280', 'BME680'];
     const SENSOR_TYPE = SENSOR_TYPES[SENSOR_TYPE_RAW] || 'unknown';

     // Gas resistance (bytes 16-17, BME680 only)
     const GAS_RAW = bytes[16] * 0x100 + bytes[17];
     const GAS_KOHM = GAS_RAW > 0 ? GAS_RAW : null;

     // Calculate radiation values
     const CPS = DT_MS > 0 ? COUNTS / (DT_MS / 1000) : 0;
     const CPM = DT_MS > 0 ? Math.round(COUNTS * 60000 / DT_MS * 10) / 10 : 0;
     const USVH = CPS / 12.2792;  // Si22G factor

     const result = {
       counts: COUNTS,
       cpm: CPM,
       cps: Math.round(CPS * 100) / 100,
       uSvph: Number(USVH.toFixed(3)),
       sample_time_ms: DT_MS,
       tube_number: TUBE_NBR,
       sw_version: `V${(SW_VERSION >> 12) & 0x0F}.${(SW_VERSION >> 4) & 0xFF}.${SW_VERSION & 0x0F}`,
       sensor_type: SENSOR_TYPE
     };

     // Add THP data if sensor available (sensor_type > 0)
     if (SENSOR_TYPE_RAW > 0) {
       if (TEMP_C !== null) result.temperature_C = TEMP_C;
       if (HUMIDITY_PERCENT !== null) result.humidity_percent = HUMIDITY_PERCENT;
       if (PRESSURE_HPA !== null) result.pressure_hPa = PRESSURE_HPA;
       if (SENSOR_TYPE_RAW === 3 && GAS_KOHM !== null) {
         result.gas_resistance_kOhm = GAS_KOHM;
       }
     }

     return { data: result, warnings: [], errors: [] };
   }

Decoded fields
--------------

- **counts**: GM tube counts in the interval
- **cpm**: counts per minute (from counts and ``sample_time_ms``)
- **cps**: counts per second
- **uSvph**: dose rate in uSv/h (Si22G conversion factor)
- **sample_time_ms**: measurement interval in ms
- **tube_number**: tube ID (0x13=SBM-19, 0x14=SBM-20, 0x16=Si22G)
- **sw_version**: firmware version string
- **sensor_type**: ``none``, ``BMP280``, ``BME280``, or ``BME680``
- **temperature_C**: temperature (C, if sensor)
- **humidity_percent**: humidity (%, if sensor)
- **pressure_hPa**: pressure (hPa, if sensor)
- **gas_resistance_kOhm**: gas resistance (kOhm, BME680 only)

HTTP integration
----------------

Legacy note: previously this payload was forwarded to sensor.community via the webhook service ``https://ttn2luft.citysensor.de``. That external service is no longer maintained.
