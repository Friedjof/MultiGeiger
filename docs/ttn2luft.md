# TTN --> Luftdaten
rxf 2020-02-03

### Payload der LoRa-Sendung


Port | ByteNr | Wert [hex]| Beschreibung | Beispiele
-----|--------|------|------------- | ---------
1    | 0/1/2/3 | 00000107 | Anzahl der Impulse (counts) | => 263
1    | 4/5/6   | 0249F0 | Messzeit [ms] für diese Impulse (sample\_time\_ms) | => 150000
1	   |	  7/8  | 10C0   | Software-Version (software_version)| 1.12.0 (siehe unten)
1	   |	   9   |  16    | Bezeichnung des Zählrohres (tube) |Si**22**G
||||
2	   |	  0/1   | 0107   | BME280 Temperatur in 0.1° (temperature)| 0x107 => 26.3°
2	   |	   2    | 9A     | BME280 Feuchte in 0.5% (humidity)|0x9A => 77.0%
2	   |	  3/4   | 26E0   | BME280 Luftdruck in 0.1 hPa (pressure) | 0x26E0 => 995.2 hPa

Erläuterung zur Software-Version: Die obersten 4 Bit sind die Major-Version (hier 1, max. 15), die folgenden 8 Bit die Minor-Version (hier 0x0C => 12) und die untersten 4 Bit der Patchlevel (hier 0).

Bezeichnung der z. Zt. implementierten Zählrohre:  

Name | Nummer [hex]
-----|-------
SBM-19 | 0x13
SBM-20 | 0x14
Si22G	 | 0x16


Die Daten des BME280 werden nur gesendet, wenn auch ein BME280 vorhanden ist.

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

  if (fPort !== 1 || bytes.length !== 10) {
    return {
      data: {},
      warnings: [`fPort: ${fPort}, bytes: ${bytes.length}`],
      errors: []
    };
  }

  const COUNTS = bytes[0] * 0x1000000 + bytes[1] * 0x10000 + bytes[2] * 0x100 + bytes[3];
  const DT_MS = bytes[4] * 0x10000 + bytes[5] * 0x100 + bytes[6];
  const SW_VERSION = bytes[7] * 0x100 + bytes[8];
  const TUBE_NBR = bytes[9];

  const CPS = DT_MS > 0 ? COUNTS / (DT_MS / 1000) : 0;
  const CPM = DT_MS > 0 ? Math.round(COUNTS * 60000 / DT_MS * 10) / 10 : 0;
  const USVH = CPS / 12.2792;  // Conversion factor for Si22G tube

  return {
    data: {
      counts: COUNTS,
      cpm: CPM,
      cps: Math.round(CPS * 100) / 100,
      usvh: Number(USVH.toFixed(3)),
      sample_time_ms: DT_MS,
      tube_number: TUBE_NBR,
      sw_version: `V${(SW_VERSION >> 12) & 0x0F}.${(SW_VERSION >> 4) & 0xFF}.${SW_VERSION & 0x0F}`
    },
    warnings: [],
    errors: []
  };
}
```

**Dekodierte Datenfelder:**

- **counts**: Gesamtzahl der GM-Tube-Impulse während des Messintervalls
- **cpm**: Counts per Minute (hochgerechnet aus counts und sample_time_ms)
- **cps**: Counts per Second (berechnet)
- **usvh**: Dosisleistung in µSv/h (Mikrosievert pro Stunde) mit Si22G-Umrechnungsfaktor
- **sample_time_ms**: Messintervall in Millisekunden
- **tube_number**: GM-Tube-Typ-Kennung (0x13=SBM-19, 0x14=SBM-20, 0x16=Si22G)
- **sw_version**: MultiGeiger Firmware-Version

**Hinweis:** Der Umrechnungsfaktor 12.2792 CPS/µSv/h ist spezifisch für das Si22G-Zählrohr.
Für andere Zählrohrtypen muss dieser Faktor entsprechend der Kalibrierungsdaten angepasst werden.

### HTTP-Integration

Die HTTP-Integration zu sensor.community erfolgt über den Webhook-Dienst **ttn2luft.citysensor.de**.
Die Umwandlung der LoRa-Payload in das sensor.community-Format wird serverseitig durchgeführt.
