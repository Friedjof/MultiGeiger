# TTN --> Luftdaten
rxf 2020-02-03

### Payload der LoRa-Sendung

**Seit Version X.X:** Geiger- und Umweltdaten werden in **einer kombinierten Nachricht** (Port 1, 18 Bytes) gesendet.

Port | ByteNr | Wert [hex]| Beschreibung | Beispiele
-----|--------|------|------------- | ---------
1    | 0/1/2/3 | 00000107 | Anzahl der Impulse (counts) | => 263
1    | 4/5/6   | 0249F0 | Messzeit [ms] für diese Impulse (sample\_time\_ms) | => 150000
1    | 7/8  | 10C0   | Software-Version (software_version)| 1.12.0 (siehe unten)
1    | 9   |  16    | Bezeichnung des Zählrohres (tube) |Si**22**G
1    | 10/11   | 0107   | BMP/BME Temperatur × 10 (temperature)| 0x0107 = 263 → 26.3°C
1    | 12    | 9A     | BME Feuchte × 2 (humidity)|0x9A = 154 → 77.0% (0 bei BMP280)
1    | 13/14   | 2794   | BMP/BME Luftdruck × 10 (pressure) | 0x2794 = 10132 → 1013.2 hPa
1    | 15      | 01     | THP Sensor-Typ | 0=kein Sensor, 1=BMP280, 2=BME280, 3=BME680
1    | 16/17   | 0096   | Gas-Widerstand in kΩ (BME680 only) | 0x0096 = 150 kΩ (0 bei BMP280/BME280)

**Hinweis:** Wenn kein THP-Sensor (BMP280/BME280/BME680) vorhanden ist, sind die Bytes 10-17 alle `0x00` und Byte 15 ist `0x00`.

Erläuterung zur Software-Version: Die obersten 4 Bit sind die Major-Version (hier 1, max. 15), die folgenden 8 Bit die Minor-Version (hier 0x0C => 12) und die untersten 4 Bit der Patchlevel (hier 0).

Bezeichnung der z. Zt. implementierten Zählrohre:

Name | Nummer [hex]
-----|-------
SBM-19 | 0x13
SBM-20 | 0x14
Si22G  | 0x16

**Unterstützte THP-Sensoren:**
- **BMP280** (Typ 1): Nur Temperatur + Luftdruck (Humidity = 0, Gas = 0)
- **BME280** (Typ 2): Temperatur + Luftfeuchtigkeit + Luftdruck (Gas = 0)
- **BME680** (Typ 3): Temperatur + Luftfeuchtigkeit + Luftdruck + Gas-Widerstand

### Payload-Decoder

Der folgende JavaScript-Decoder kann in der TTN Console unter **Payload formatters → Uplink** eingetragen werden,
um die Rohdaten in lesbare Werte umzuwandeln. Der Decoder berechnet CPM (Counts per Minute), CPS (Counts per Second)
und die Dosisleistung in µSv/h.

**Installation:**
1. TTN Console öffnen → Application auswählen
2. Im Menü links **Payload formatters** → **Uplink** wählen
3. **Formatter type**: **Javascript** auswählen
4. Folgenden Code einfügen:

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

**Dekodierte Datenfelder:**

**Geiger-Daten:**
- **counts**: Gesamtzahl der GM-Tube-Impulse während des Messintervalls
- **cpm**: Counts per Minute (hochgerechnet aus counts und sample_time_ms)
- **cps**: Counts per Second (berechnet)
- **uSvph**: Dosisleistung in µSv/h (Mikrosievert pro Stunde) mit Si22G-Umrechnungsfaktor
- **sample_time_ms**: Messintervall in Millisekunden
- **tube_number**: GM-Tube-Typ-Kennung (0x13=SBM-19, 0x14=SBM-20, 0x16=Si22G)
- **sw_version**: MultiGeiger Firmware-Version
- **sensor_type**: THP-Sensor Typ (`none`, `BMP280`, `BME280`, `BME680`)

**THP-Daten (nur wenn Sensor vorhanden):**
- **temperature_C**: Temperatur in °C (BMP280/BME280/BME680)
- **humidity_percent**: Relative Luftfeuchtigkeit in % (nur BME280/BME680)
- **pressure_hPa**: Luftdruck in hPa (BMP280/BME280/BME680)
- **gas_resistance_kOhm**: Gas-Widerstand in kΩ (nur BME680)

**Hinweis:** Der Umrechnungsfaktor 12.2792 CPS/µSv/h ist spezifisch für das Si22G-Zählrohr.
Für andere Zählrohrtypen muss dieser Faktor entsprechend der Kalibrierungsdaten angepasst werden.

### HTTP-Integration

Die HTTP-Integration zu sensor.community erfolgt über den Webhook-Dienst **ttn2luft.citysensor.de**.
Die Umwandlung der LoRa-Payload in das sensor.community-Format wird serverseitig durchgeführt.
