# TTN -> sensor.community
rxf 2020-02-03

### LoRa payload

**Since firmware X.X:** Geiger + environmental data are sent in **one combined uplink** (Port 1, 18 bytes).

Port | Byte(s) | Hex | Description | Example
-----|---------|-----|-------------|---------
1    | 0/1/2/3 | 00000107 | Number of counts | 263
1    | 4/5/6   | 0249F0   | Measurement time [ms] for these counts (`sample_time_ms`) | 150000
1    | 7/8     | 10C0     | Software version (`software_version`) | 1.12.0 (see below)
1    | 9       | 16       | Geiger tube ID (`tube`) | Si**22**G
1    | 10/11   | 0107     | BMP/BME temperature x10 (`temperature`) | 0x0107 = 263 -> 26.3 C
1    | 12      | 9A       | BME humidity x2 (`humidity`) | 0x9A = 154 -> 77.0% (0 for BMP280)
1    | 13/14   | 2794     | BMP/BME pressure x10 (`pressure`) | 0x2794 = 10132 -> 1013.2 hPa
1    | 15      | 01       | THP sensor type | 0=no sensor, 1=BMP280, 2=BME280, 3=BME680
1    | 16/17   | 0096     | Gas resistance in kOhm (BME680 only) | 0x0096 = 150 kOhm (0 for BMP280/BME280)

**Note:** If no THP sensor (BMP280/BME280/BME680) is present, bytes 10-17 are all `0x00` and byte 15 is `0x00`.

Software version encoding: top 4 bits are the major version (here 1, max 15), next 8 bits are minor (here 0x0C -> 12), lowest 4 bits are patch level (here 0).

Implemented tube identifiers:

Name | Number [hex]
-----|-------------
SBM-19 | 0x13
SBM-20 | 0x14
Si22G  | 0x16

**Supported THP sensors:**
- **BMP280** (type 1): temperature + pressure only (humidity = 0, gas = 0)
- **BME280** (type 2): temperature + humidity + pressure (gas = 0)
- **BME680** (type 3): temperature + humidity + pressure + gas resistance

### Payload decoder

The JavaScript decoder below can be added in the TTN Console under **Payload formatters -> Uplink** to convert raw bytes into readable values. It also computes CPM, CPS, and dose rate in uSv/h.

**Setup steps:**
1. Open the TTN Console and select your application.
2. In the left menu choose **Payload formatters** -> **Uplink**.
3. Set **Formatter type** to **Javascript**.
4. Paste the code below:

```javascript
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
  // Temperature: signed int16_t (supports negative values)
  let TEMP_RAW = bytes[10] * 0x100 + bytes[11];
  if (TEMP_RAW & 0x8000) TEMP_RAW = TEMP_RAW - 0x10000; // Convert to signed
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
  const USVH = CPS / 12.2792;  // Conversion factor for Si22G tube

  const result = {
    // Geiger data
    counts: COUNTS,
    cpm: CPM,
    cps: Math.round(CPS * 100) / 100,
    uSvph: Number(USVH.toFixed(3)),
    sample_time_ms: DT_MS,
    tube_number: TUBE_NBR,
    sw_version: `V${(SW_VERSION >> 12) & 0x0F}.${(SW_VERSION >> 4) & 0xFF}.${SW_VERSION & 0x0F}`,

    // Sensor type
    sensor_type: SENSOR_TYPE
  };

  // Add THP data if sensor available (sensor_type > 0)
  if (SENSOR_TYPE_RAW > 0) {
    if (TEMP_C !== null) result.temperature_C = TEMP_C;
    if (HUMIDITY_PERCENT !== null) result.humidity_percent = HUMIDITY_PERCENT;
    if (PRESSURE_HPA !== null) result.pressure_hPa = PRESSURE_HPA;

    // Add gas resistance for BME680 only
    if (SENSOR_TYPE_RAW === 3 && GAS_KOHM !== null) {
      result.gas_resistance_kOhm = GAS_KOHM;
    }
  }

  return {
    data: result,
    warnings: [],
    errors: []
  };
}
```

**Decoded fields:**

**Geiger data:**
- **counts**: Total GM tube counts during the measurement interval
- **cpm**: Counts per minute (extrapolated from counts and `sample_time_ms`)
- **cps**: Counts per second (calculated)
- **uSvph**: Dose rate in uSv/h using the Si22G conversion factor
- **sample_time_ms**: Measurement interval in milliseconds
- **tube_number**: GM tube ID (0x13=SBM-19, 0x14=SBM-20, 0x16=Si22G)
- **sw_version**: MultiGeiger firmware version
- **sensor_type**: THP sensor type (`none`, `BMP280`, `BME280`, `BME680`)

**THP data (only if a sensor is present):**
- **temperature_C**: Temperature in C (BMP280/BME280/BME680)
- **humidity_percent**: Relative humidity in % (BME280/BME680 only)
- **pressure_hPa**: Air pressure in hPa (BMP280/BME280/BME680)
- **gas_resistance_kOhm**: Gas resistance in kOhm (BME680 only)

**Note:** The conversion factor 12.2792 CPS/uSv/h is specific to the Si22G tube.
For other tube types adjust the factor according to calibration data.

### HTTP integration

HTTP integration to sensor.community is provided via the webhook service **ttn2luft.citysensor.de**.
The LoRa payload is transformed into the sensor.community format on the server side.
