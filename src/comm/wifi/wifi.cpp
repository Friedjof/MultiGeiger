// measurements data transmission related code
// - via WiFi to internet servers
// - via LoRa to TTN (to internet servers)

#include "wifi.hpp"

#include <string.h>
#include <esp_system.h>

#include "app/controller.hpp"
#include "web_files.h"

extern MultiGeigerController controller;

#ifndef MQTT_BASE_TOPIC
#define MQTT_BASE_TOPIC ""
#endif

// Global instances
WiFiService wifiService;
WebServer server(80);

// Simple session handling for config/OTA endpoints
static const unsigned long SESSION_TTL_MS = 30UL * 60UL * 1000UL;  // 30 minutes
static const unsigned long SESSION_TTL_SECONDS = SESSION_TTL_MS / 1000UL;
static const int MAX_SESSIONS = 3;
static const char* SESSION_COOKIE_NAME = "session";

struct SessionToken {
  String token;
  unsigned long expiresAt;
};

static SessionToken sessions[MAX_SESSIONS];

// CA Roots for LetsEncrypt Certificates (cross-signed):
// - 1. ISRG Root X1 - valid until 2035-06-04
// - 2. DST Root CA X3 - valid until 2021-09-30
// CA Roots for Google stuff:
// - 3. GlobalSign Root R1 - valid until 2028-01-28
// CA Root for Amazon stuff:
// - 4. Amazon Root CA 1 - valid until 2038-01-17
static const char ca_certs[] = R"=====(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT
DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow
PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD
Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O
rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq
OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b
xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw
7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD
aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV
HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG
SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69
ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr
AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz
R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5
JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo
Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG
A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv
b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw
MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i
YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT
aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ
jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp
xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp
1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG
snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ
U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8
9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E
BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B
AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz
yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE
38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP
AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad
DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME
HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)=====";
static unsigned int lora_software_version;
static bool isLoraBoard;

// Flags for scheduled restart (prevents ESP crash on config save)
static bool restartScheduled = false;
static unsigned long restartTime = 0;

// Config page heartbeat tracking
static bool configPageActive = false;
static unsigned long lastConfigPingTime = 0;
static const unsigned long CONFIG_PING_TIMEOUT_MS = 5000;  // 5 seconds

// Timestamps for last successful transmissions (milliseconds since boot)
static unsigned long lastLoRaSendTime = 0;

void setup_transmission(const char *version, char *ssid, bool loraHardware) {
  isLoraBoard = loraHardware;

  if (isLoraBoard) {
    int major, minor, patch;
    sscanf(version, "V%d.%d.%d", &major, &minor, &patch);
    lora_software_version = (major << 12) + (minor << 4) + patch;
    setup_lorawan();
  }

  Config& cfg = configService.getConfig();
  set_status(STATUS_TTN, cfg.sendToLora ? ST_TTN_INIT : ST_TTN_OFF);
}

void WifiManager::pollWeb() {
  // Process WiFi state machine and DNS requests
  wifiService.loop();

  // Handle web server requests
  server.handleClient();
}

void poll_transmission() {
  // Start ArduinoOTA once WiFi is connected (only once)
  static bool arduinoOtaStarted = false;
  if (!arduinoOtaStarted && WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.begin();
    log(INFO, "ArduinoOTA: Ready for IDE uploads (hostname: %s)", WiFi.getHostname());
    arduinoOtaStarted = true;
  }

  // Handle ArduinoOTA requests (only if started)
  if (arduinoOtaStarted) {
    ArduinoOTA.handle();
  }

  // Check for scheduled restart
  if (restartScheduled && millis() >= restartTime) {
    log(INFO, "Executing scheduled restart...");
    delay(100);  // Small delay to ensure log is flushed
    ESP.restart();
  }

  // Check if config page is active but no ping received for CONFIG_PING_TIMEOUT_MS
  // If timeout, user likely left the page without saving -> re-enable ticks
  if (configPageActive && (millis() - lastConfigPingTime) > CONFIG_PING_TIMEOUT_MS) {
    log(INFO, "Config page heartbeat timeout - re-enabling ticks");
    configPageActive = false;
    tick_enable(true);
  }

  if (isLoraBoard) {
    // The LMIC needs to be polled a lot; and this is very low cost if the LMIC isn't
    // active. So we just act as a bridge. We need this routine so we can see
    // `isLoraBoard`. Most C compilers will notice the tail call and optimize this
    // to a jump.
    poll_lorawan();
  }
}

// LoRa payload:
// To minimise airtime and follow the 'TTN Fair Access Policy', we send all data in one message.
// We do NOT use Cayenne LPP.
// The payload stays compact for downstream decoders (e.g., TTN integrations).
// For byte definitions see docs/source/ttn_payload.rst.
//
// Combined payload structure (18 bytes total):
// Bytes 0-3:   GM counts (uint32_t)
// Bytes 4-6:   Measurement interval in ms (uint24_t, max ~4 hours)
// Bytes 7-8:   Software version (uint16_t)
// Byte  9:     Tube type number (uint8_t)
// Bytes 10-11: Temperature * 10 (int16_t, signed, e.g., 25.3°C = 253, -5.2°C = -52)
// Byte  12:    Humidity * 2 (uint8_t, e.g., 50.5% = 101)
// Bytes 13-14: Pressure * 10 (uint16_t, e.g., 1013.25 hPa → 10132)
// Byte  15:    THP sensor type (uint8_t: 0=none, 1=BMP280, 2=BME280, 3=BME680)
// Bytes 16-17: Gas resistance in kOhm (uint16_t, BME680 only, 0 for other sensors)
int send_ttn_combined(int tube_nbr, unsigned int dt, unsigned int gm_counts,
                      bool have_thp, float temperature, float humidity, float pressure,
                      float gas_resistance, int sensor_type) {
  unsigned char ttnData[18];

  // Geiger data (10 bytes)
  ttnData[0] = (gm_counts >> 24) & 0xFF;
  ttnData[1] = (gm_counts >> 16) & 0xFF;
  ttnData[2] = (gm_counts >> 8) & 0xFF;
  ttnData[3] = gm_counts & 0xFF;
  ttnData[4] = (dt >> 16) & 0xFF;
  ttnData[5] = (dt >> 8) & 0xFF;
  ttnData[6] = dt & 0xFF;
  ttnData[7] = (lora_software_version >> 8) & 0xFF;
  ttnData[8] = lora_software_version & 0xFF;
  ttnData[9] = tube_nbr;

  // THP+Gas data (8 bytes) - send zeros if no sensor available
  if (have_thp) {
    ttnData[10] = ((int)(temperature * 10)) >> 8;
    ttnData[11] = ((int)(temperature * 10)) & 0xFF;
    ttnData[12] = (int)(humidity * 2);
    ttnData[13] = ((int)(pressure * 10)) >> 8;
    ttnData[14] = ((int)(pressure * 10)) & 0xFF;

    // Sensor type mapping: 0=BMP280, 2280=BME280, 680=BME680 → 1, 2, 3
    uint8_t sensor_type_byte = 0;
    if (sensor_type == 280) sensor_type_byte = 1;       // BMP280
    else if (sensor_type == 2280) sensor_type_byte = 2; // BME280
    else if (sensor_type == 680) sensor_type_byte = 3;  // BME680
    ttnData[15] = sensor_type_byte;

    // Gas resistance (BME680 only)
    ttnData[16] = ((int)gas_resistance) >> 8;
    ttnData[17] = ((int)gas_resistance) & 0xFF;
  } else {
    // No THP sensor: send zeros
    memset(&ttnData[10], 0, 8);
  }

  return lorawan_send(1, ttnData, 18, false, NULL, NULL, NULL);
}

// Legacy functions kept for backward compatibility (now just wrappers)
int send_ttn_geiger(int tube_nbr, unsigned int dt, unsigned int gm_counts) {
  return send_ttn_combined(tube_nbr, dt, gm_counts, false, 0.0, 0.0, 0.0, 0.0, 0);
}

int send_ttn_thp(float temperature, float humidity, float pressure) {
  // This function is deprecated and should not be called anymore
  // Kept for compatibility but does nothing (combined function handles THP)
  log(INFO, "send_ttn_thp() called but deprecated - use send_ttn_combined()");
  return TX_STATUS_UPLINK_SUCCESS;
}

void transmit_data(String tube_type, int tube_nbr, unsigned int dt, unsigned int hv_pulses, unsigned int gm_counts, unsigned int cpm,
                   int have_thp, float temperature, float humidity, float pressure, float gas_resistance, int sensor_type, int wifi_status) {
  (void)wifi_status;  // WiFi state no longer gates transmissions
  Config& cfg = configService.getConfig();

  if (isLoraBoard && cfg.sendToLora && (strcmp(cfg.devaddr, "") != 0)) {    // send only if we have ABP credentials
    bool ttn_ok;
    log(INFO, "Sending to TTN ...");
    log(INFO, "  - isLoraBoard: %d, sendToLora: %d, devaddr: %s", isLoraBoard, cfg.sendToLora, cfg.devaddr);
    set_status(STATUS_TTN, ST_TTN_SENDING);
    display_status();
    // Send combined message with GM + THP + Gas data in one transmission
    int rc1 = send_ttn_combined(tube_nbr, dt, gm_counts, have_thp, temperature, humidity, pressure, gas_resistance, sensor_type);
    log(INFO, "TTN send_ttn_combined result: %d (have_thp=%d, sensor_type=%d)", rc1, have_thp, sensor_type);
    ttn_ok = (rc1 == TX_STATUS_UPLINK_SUCCESS);
    log(INFO, "TTN transmission %s (rc=%d)", ttn_ok ? "SUCCESS" : "FAILED", rc1);
    set_status(STATUS_TTN, ttn_ok ? ST_TTN_IDLE : ST_TTN_ERROR);
    if (ttn_ok) {
      lastLoRaSendTime = millis();
    }
    display_status();
  } else {
    // Log why LoRa is not sending
    if (!isLoraBoard) {
      log(INFO, "NOT sending to TTN: LoRa hardware not detected");
    } else if (!cfg.sendToLora) {
      log(INFO, "NOT sending to TTN: 'Send to LoRa' disabled in config");
    } else if (strcmp(cfg.devaddr, "") == 0) {
      log(INFO, "NOT sending to TTN: DevAddr is empty (ABP not configured)");
    }
  }
}

// Web Configuration related code
// also: OTA updates

// QoS for MQTT (not configurable via UI currently)
int mqttQos = MQTT_QOS;

char ssid[33];  // Device name / AP SSID

char *buildSSID(void);

unsigned long getESPchipID() {
  uint64_t espid = ESP.getEfuseMac();
  uint8_t *pespid = (uint8_t *)&espid;
  uint32_t id = 0;
  uint8_t *pid = (uint8_t *)&id;
  pid[0] = (uint8_t)pespid[5];
  pid[1] = (uint8_t)pespid[4];
  pid[2] = (uint8_t)pespid[3];
  log(INFO, "ID: %08X", id);
  log(INFO, "MAC: %04X%08X", (uint16_t)(espid >> 32), (uint32_t)espid);
  return id;
}

char *buildSSID() {
  // build SSID from ESP chip id (last 6 hex digits of MAC)
  uint32_t id = getESPchipID();
  sprintf(ssid, "MultiGeiger-%06X", id & 0xFFFFFF);
  return ssid;
}

// Session helpers ------------------------------------------------------------

static String getJsonString(const String &body, const char *key) {
  String needle = "\"" + String(key) + "\":\"";
  int start = body.indexOf(needle);
  if (start < 0) return "";
  start += needle.length();
  int end = body.indexOf("\"", start);
  if (end < 0) return "";
  return body.substring(start, end);
}

static void pruneSessions(void) {
  unsigned long now = millis();
  for (int i = 0; i < MAX_SESSIONS; ++i) {
    if (sessions[i].token.length() == 0) continue;
    if ((long)(sessions[i].expiresAt - now) <= 0) {
      sessions[i].token = "";
      sessions[i].expiresAt = 0;
    }
  }
}

static String generateSessionToken(void) {
  char buf[9];  // 8 hex chars + null terminator
  String token;
  token.reserve(32);
  for (int i = 0; i < 4; ++i) {  // 4 * 8 hex chars = 32 chars
    uint32_t r = esp_random();
    snprintf(buf, sizeof(buf), "%08lx", static_cast<unsigned long>(r));
    token += buf;
  }
  if (token.length() > 32) {
    token.remove(32);
  }
  return token;
}

static bool storeSessionToken(const String &token) {
  unsigned long now = millis();
  int target = -1;
  unsigned long oldestRemaining = 0xFFFFFFFFUL;

  for (int i = 0; i < MAX_SESSIONS; ++i) {
    if (sessions[i].token.length() == 0) {
      target = i;
      break;
    }

    unsigned long remaining = sessions[i].expiresAt - now;
    if ((long)remaining <= 0) {
      target = i;
      break;
    }

    if (remaining < oldestRemaining) {
      oldestRemaining = remaining;
      target = i;
    }
  }

  if (target < 0) {
    return false;
  }

  sessions[target].token = token;
  sessions[target].expiresAt = now + SESSION_TTL_MS;
  return true;
}

static String extractCookieValue(const String &cookieHeader, const char *name) {
  if (cookieHeader.length() == 0) return "";

  String needle = String(name) + "=";
  int start = cookieHeader.indexOf(needle);
  if (start < 0) return "";
  start += needle.length();

  int end = cookieHeader.indexOf(';', start);
  if (end < 0) end = cookieHeader.length();

  String value = cookieHeader.substring(start, end);
  value.trim();
  return value;
}

static String sessionTokenFromRequest(void) {
  String cookieHeader = server.header("Cookie");
  String token = extractCookieValue(cookieHeader, SESSION_COOKIE_NAME);
  if (token.length() > 0) {
    return token;
  }

  const String authHeader = server.header("Authorization");
  const String bearerPrefix = "Bearer ";
  if (authHeader.startsWith(bearerPrefix)) {
    return authHeader.substring(bearerPrefix.length());
  }

  return "";
}

static bool validateSessionToken(const String &token) {
  if (token.length() == 0) {
    return false;
  }

  unsigned long now = millis();
  for (int i = 0; i < MAX_SESSIONS; ++i) {
    if (sessions[i].token.length() == 0) continue;

    if (sessions[i].token == token) {
      if ((long)(sessions[i].expiresAt - now) > 0) {
        sessions[i].expiresAt = now + SESSION_TTL_MS;  // Sliding expiration
        return true;
      }

      sessions[i].token = "";
      sessions[i].expiresAt = 0;
      return false;
    }
  }

  return false;
}

static bool hasValidSession(void) {
  // Skip auth when in AP mode - WiFi password is sufficient authentication
  WiFiState state = wifiService.getState();
  if (state == WIFI_STATE_AP_MODE || state == WIFI_STATE_BOOT) {
    return true;
  }

  pruneSessions();
  String token = sessionTokenFromRequest();
  return validateSessionToken(token);
}

static void clearSessions(void) {
  for (int i = 0; i < MAX_SESSIONS; ++i) {
    sessions[i].token = "";
    sessions[i].expiresAt = 0;
  }
}

/**
 * @brief Check session authentication for protected endpoints
 * @return true if authenticated, false otherwise (sends 401 response)
 */
bool checkHttpAuth(bool sendResponse = true) {
  if (hasValidSession()) {
    return true;
  }

  if (sendResponse) {
    server.send(401, "application/json", "{\"status\":\"unauthorized\",\"message\":\"Login required\"}");
  }
  return false;
}

// Rate limiting for config POSTs
static unsigned long lastConfigPostTime = 0;
static int configPostAttempts = 0;
#define CONFIG_POST_MIN_INTERVAL_MS 1000  // Min 1 second between POSTs
#define CONFIG_POST_MAX_ATTEMPTS 10       // Max 10 attempts per minute

/**
 * @brief Check rate limiting for config POST requests
 * @return true if allowed, false if rate limit exceeded
 */
bool checkRateLimit(void) {
  unsigned long now = millis();

  // Reset counter every minute
  if (now - lastConfigPostTime > 60000) {
    configPostAttempts = 0;
  }

  // Check if too many attempts
  if (configPostAttempts >= CONFIG_POST_MAX_ATTEMPTS) {
    log(WARNING, "Rate limit exceeded for config POST");
    server.send(429, "application/json", "{\"status\":\"error\",\"message\":\"Too many requests. Try again later.\"}");
    return false;
  }

  // Check minimum interval
  if (now - lastConfigPostTime < CONFIG_POST_MIN_INTERVAL_MS) {
    log(WARNING, "Config POST too fast");
    server.send(429, "application/json", "{\"status\":\"error\",\"message\":\"Please wait before sending another request.\"}");
    return false;
  }

  lastConfigPostTime = now;
  configPostAttempts++;
  return true;
}

/**
 * @brief Login endpoint: validates credentials and sets a session cookie.
 */
void handleAuthLogin(void) {
  pruneSessions();

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body missing\"}");
    return;
  }

  String body = server.arg("plain");
  String username = getJsonString(body, "username");
  String password = getJsonString(body, "password");

  // DEBUG: Log received credentials
  log(INFO, "[AUTH DEBUG] Received login attempt");
  log(INFO, "[AUTH DEBUG] Request body: %s", body.c_str());
  log(INFO, "[AUTH DEBUG] Parsed username: '%s' (len=%d)", username.c_str(), username.length());
  log(INFO, "[AUTH DEBUG] Parsed password: '%s' (len=%d)", password.c_str(), password.length());

  Config& cfg = configService.getConfig();

  // DEBUG: Log stored credentials
  log(INFO, "[AUTH DEBUG] Stored httpAuthUser: '%s' (len=%d, first_byte=%d)",
      cfg.httpAuthUser, strlen(cfg.httpAuthUser), (int)cfg.httpAuthUser[0]);
  log(INFO, "[AUTH DEBUG] Stored httpAuthPass: '%s' (len=%d, first_byte=%d)",
      cfg.httpAuthPass, strlen(cfg.httpAuthPass), (int)cfg.httpAuthPass[0]);

  // Safety net: if stored credentials are empty (old config), restore defaults
  if (cfg.httpAuthUser[0] == '\0' || cfg.httpAuthPass[0] == '\0') {
    log(WARNING, "[AUTH DEBUG] Credentials are empty, restoring defaults");
    strncpy(cfg.httpAuthUser, HTTP_AUTH_USER, HTTP_AUTH_LEN);
    cfg.httpAuthUser[HTTP_AUTH_LEN - 1] = '\0';
    strncpy(cfg.httpAuthPass, HTTP_AUTH_PASS, HTTP_AUTH_LEN);
    cfg.httpAuthPass[HTTP_AUTH_LEN - 1] = '\0';
    configService.save();
    log(WARNING, "HTTP auth creds were empty - restored defaults");
    log(INFO, "[AUTH DEBUG] After restore - httpAuthUser: '%s', httpAuthPass: '%s'",
        cfg.httpAuthUser, cfg.httpAuthPass);
  }

  // DEBUG: Log comparison
  String storedUser = String(cfg.httpAuthUser);
  String storedPass = String(cfg.httpAuthPass);
  log(INFO, "[AUTH DEBUG] Comparison:");
  log(INFO, "[AUTH DEBUG]   username '%s' vs stored '%s' -> %s",
      username.c_str(), storedUser.c_str(),
      (username == storedUser) ? "MATCH" : "MISMATCH");
  log(INFO, "[AUTH DEBUG]   password '%s' vs stored '%s' -> %s",
      password.c_str(), storedPass.c_str(),
      (password == storedPass) ? "MATCH" : "MISMATCH");

  if (username != String(cfg.httpAuthUser) || password != String(cfg.httpAuthPass)) {
    log(WARNING, "Login failed for user %s", username.c_str());
    log(WARNING, "[AUTH DEBUG] Login REJECTED");
    server.send(401, "application/json", "{\"status\":\"unauthorized\",\"message\":\"Invalid credentials\"}");
    return;
  }

  log(INFO, "[AUTH DEBUG] Login ACCEPTED");

  String token = generateSessionToken();
  if (!storeSessionToken(token)) {
    server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Could not create session\"}");
    return;
  }

  String cookie = String(SESSION_COOKIE_NAME) + "=" + token +
                  "; Path=/; Max-Age=" + String(SESSION_TTL_SECONDS) +
                  "; HttpOnly; SameSite=Lax";
  server.sendHeader("Set-Cookie", cookie);
  log(INFO, "Session created for user %s", username.c_str());
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

/**
 * @brief Validate and sanitize string input
 * @param input String to validate
 * @param maxLen Maximum allowed length
 * @return true if valid, false otherwise
 */
bool validateString(const String &input, size_t maxLen) {
  if (input.length() == 0 || input.length() >= maxLen) {
    return false;
  }

  // Check for null bytes (potential exploit)
  for (size_t i = 0; i < input.length(); i++) {
    if (input[i] == '\0') {
      return false;
    }
  }

  return true;
}

/**
 * @brief Validate port number
 * @param port Port number to validate
 * @return true if valid (1-65535), false otherwise
 */
bool validatePort(uint16_t port) {
  return (port >= 1 && port <= 65535);
}

/**
 * @brief Validate float value in range
 * @param value Value to validate
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @return true if valid, false otherwise
 */
bool validateFloat(float value, float min, float max) {
  return (value >= min && value <= max && !isnan(value) && !isinf(value));
}

/**
 * @brief Validate integer value in range
 * @param value Value to validate
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @return true if valid, false otherwise
 */
bool validateInt(int value, int min, int max) {
  return (value >= min && value <= max);
}

/**
 * @brief Validate hex string (for LoRa keys)
 * @param input String to validate
 * @param expectedLen Expected length (0 = any length)
 * @return true if valid hex string, false otherwise
 */
bool validateHexString(const String &input, size_t expectedLen) {
  if (input.length() == 0 || input.length() >= LORA_KEY_LEN) {
    return false;
  }

  if (expectedLen > 0 && input.length() != expectedLen) {
    return false;
  }

  // Check if all characters are valid hex
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Check and sync time from X-Client-Time header if device time is invalid
 *
 * Call this at the start of every request handler.
 * Automatically sets device time from browser if current time < 2020.
 */
void checkAndSyncClientTime(void) {
  const time_t YEAR_2020 = 1577836800;  // 2020-01-01 00:00:00 UTC
  static bool time_synced = false;

  time_t now = time(nullptr);

  // If time is already valid, skip
  if (now >= YEAR_2020) {
    time_synced = true;
    return;
  }

  // Get X-Client-Time header
  String clientTimeHeader = server.header("X-Client-Time");
  if (clientTimeHeader.length() == 0) {
    return;  // No client time available
  }

  // Parse client time (milliseconds)
  unsigned long long client_time_ms = strtoull(clientTimeHeader.c_str(), nullptr, 10);
  if (client_time_ms < 1000000000000ULL) {  // Sanity check: must be > year 2001
    return;
  }

  time_t new_time = (time_t)(client_time_ms / 1000ULL);

  // Validate time is reasonable
  if (new_time < YEAR_2020) {
    return;
  }

  // Set system time
  struct timeval tv = {new_time, 0};
  settimeofday(&tv, nullptr);

  struct tm local_tm;
  localtime_r(&new_time, &local_tm);
  char time_str[20];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &local_tm);

  log(INFO, "Time auto-synced from browser - New time: %s", time_str);
  time_synced = true;
}

/**
 * @brief API endpoint for live status data (JSON)
 */
void handleApiStatus(void) {
  // Auto-sync time from browser if needed
  checkAndSyncClientTime();

  unsigned long counts = controller.getCounts();
  float temp = controller.getTemperature();
  float hum = controller.getHumidity();
  float press = controller.getPressure();
  float gas = controller.getGasResistance();
  bool thp = controller.hasThp();
  bool hvErr = controller.hasHvError();
  int sensorType = controller.getSensorType();

  const char* sensorName = nullptr;
  switch (sensorType) {
    case 680:
      sensorName = "BME680";
      break;
    case 2280:
      sensorName = "BME280";
      break;
    case 280:
      sensorName = "BMP280";
      break;
    default:
      break;
  }

  bool hasHumidity = (sensorType == 2280 || sensorType == 680);
  bool hasGas = (sensorType == 680);

  // Calculate CPM and dose rate (simplified calculation)
  unsigned long uptime_ms = millis();
  unsigned long uptime_s = uptime_ms / 1000;
  float cpm = (uptime_s > 0) ? (counts * 60.0 / uptime_s) : 0.0;
  float dose_rate = cpm * tubes[TUBE_TYPE].cps_to_uSvph / 60.0;  // µSv/h

  String json = "{";
  json += "\"counts\":" + String(counts) + ",";
  json += "\"cpm\":" + String(cpm, 1) + ",";
  json += "\"dose_uSvh\":" + String(dose_rate, 3) + ",";
  json += "\"uptime_s\":" + String(uptime_s) + ",";
  json += "\"hv_error\":" + String(hvErr ? "true" : "false");

  if (sensorName != nullptr) {
    json += ",\"sensor\":\"" + String(sensorName) + "\"";
  }

  if (thp) {
    json += ",\"temperature\":" + String(temp, 1);
    json += ",\"pressure\":" + String(press, 1);
    if (hasHumidity) {
      json += ",\"humidity\":" + String(hum, 1);
    }
    if (hasGas) {
      json += ",\"gas\":" + String(gas, 0);
    }
  }

  json += ",\"has_thp\":" + String(thp ? "true" : "false");

  // WiFi status
  WiFiState wifiState = wifiService.getState();
  bool isAP = (wifiState == WIFI_STATE_AP_MODE);
  bool isOnline = (wifiState == WIFI_STATE_ONLINE);

  json += ",\"wifi_mode\":\"" + String(isAP ? "AP" : "STA") + "\"";
  json += ",\"wifi_connected\":" + String((isOnline || isAP) ? "true" : "false");

  if (isAP) {
    // In AP mode, show device name as SSID
    json += ",\"wifi_ssid\":\"" + String(wifiService.getHostname()) + "\"";
    json += ",\"wifi_ip\":\"" + WiFi.softAPIP().toString() + "\"";
  } else if (isOnline) {
    // In STA mode, show connected SSID and signal strength
    json += ",\"wifi_ssid\":\"" + WiFi.SSID() + "\"";
    json += ",\"wifi_ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"wifi_rssi\":" + String(WiFi.RSSI());
  }

  // MQTT status
  unsigned long mqttLastPublish = controller.getMqtt().getLastPublishTime();
  json += ",\"mqtt_enabled\":" + String(configService.getConfig().sendToMqtt ? "true" : "false");
  json += ",\"mqtt_connected\":" + String((mqttLastPublish > 0) ? "true" : "false");

  if (mqttLastPublish > 0) {
    // Convert millis() to Unix epoch milliseconds
    time_t now = time(nullptr);
    unsigned long currentMillis = millis();

    // Handle millis() overflow (occurs after ~49 days)
    unsigned long millisAgo;
    if (currentMillis >= mqttLastPublish) {
      millisAgo = currentMillis - mqttLastPublish;
    } else {
      // Overflow occurred
      millisAgo = (0xFFFFFFFFUL - mqttLastPublish) + currentMillis + 1;
    }

    // Convert to Unix epoch in milliseconds
    unsigned long long epochMs = ((unsigned long long)now * 1000ULL) - millisAgo;
    json += ",\"mqtt_last_publish\":" + String(epochMs);
  }

  // LoRa status
  if (lastLoRaSendTime > 0) {
    // Convert millis() to Unix epoch milliseconds
    time_t now = time(nullptr);
    unsigned long currentMillis = millis();

    // Handle millis() overflow (occurs after ~49 days)
    unsigned long millisAgo;
    if (currentMillis >= lastLoRaSendTime) {
      millisAgo = currentMillis - lastLoRaSendTime;
    } else {
      // Overflow occurred
      millisAgo = (0xFFFFFFFFUL - lastLoRaSendTime) + currentMillis + 1;
    }

    // Convert to Unix epoch in milliseconds
    unsigned long long epochMs = ((unsigned long long)now * 1000ULL) - millisAgo;

    json += ",\"lora_enabled\":true";
    json += ",\"lora_last_send\":" + String(epochMs);
  } else {
    json += ",\"lora_enabled\":" + String(configService.getConfig().sendToLora ? "true" : "false");
  }

  // BLE status
  int bleConnections = controller.getBle().getConnectedCount();
  json += ",\"ble_enabled\":" + String(configService.getConfig().sendToBle ? "true" : "false");
  json += ",\"ble_connections\":" + String(bleConnections);

  json += "}";

  server.send(200, "application/json", json);
}

/**
 * @brief API endpoint for current device time (JSON)
 *
 * Provides the current epoch in milliseconds and timezone offset in minutes.
 * Offset uses localtime offset (e.g. Berlin CET/CEST).
 */
void handleApiTime(void) {
  // Auto-sync time from browser if needed
  checkAndSyncClientTime();

  time_t now = time(nullptr);
  struct tm local_tm;
  struct tm utc_tm;
  localtime_r(&now, &local_tm);
  gmtime_r(&now, &utc_tm);

  // Calculate offset between local time and UTC in seconds
  time_t local_epoch = mktime(&local_tm);
  time_t utc_epoch = mktime(&utc_tm);
  long offset_sec = static_cast<long>(difftime(local_epoch, utc_epoch));
  long offset_min = offset_sec / 60;

  unsigned long long epoch_ms = static_cast<unsigned long long>(now) * 1000ULL;

  // Log time sync request with local time
  char local_time_str[20];
  strftime(local_time_str, sizeof(local_time_str), "%Y-%m-%d %H:%M:%S", &local_tm);
  log(INFO, "Time sync request - Local: %s, Offset: %+d min, Epoch: %llu ms",
      local_time_str, offset_min, epoch_ms);

  String json = "{";
  json += "\"epoch_ms\":" + String(epoch_ms) + ",";
  json += "\"tz_offset_min\":" + String(offset_min);
  json += "}";

  server.send(200, "application/json", json);
}

static bool sendWebAsset(const String& path) {
  String normalized = path;
  if (normalized == "/" || normalized.length() == 0) {
    normalized = "/index.html";
  }

  const WebFile* file = findWebFile(normalized);
  if (!file && normalized != "/index.html") {
    file = findWebFile("/index.html");
  }
  if (!file) {
    return false;
  }

  sendWebFile(server, file);
  return true;
}

void handleRoot(void) {  // Handle web requests to "/" path.
  // Serve dashboard shell; config is reachable via /config (auth protected)
  if (!sendWebAsset("/index.html")) {
    server.send(500, "text/plain", "Web UI missing");
  }
}

void handleConfigPage(void) {
  // Serve the UI shell without forcing auth, so the login dialog can be shown.
  // If a valid session already exists, disable ticks immediately.
  if (checkHttpAuth(false)) {
    tick_enable(false);
    configPageActive = true;
    lastConfigPingTime = millis();
  }

  // Serve config page
  if (!sendWebAsset("/index.html")) {
    server.send(500, "text/plain", "Web UI missing");
  }
}

void handleConfigPing(void) {
  // Check authentication
  if (!checkHttpAuth()) {
    return;  // Auth failed, 401 response already sent
  }

  // Mark config page as active even when loaded via SPA
  configPageActive = true;
  tick_enable(false);
  // Update heartbeat timestamp
  lastConfigPingTime = millis();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

static char lastWiFiSSID[33] = "";

void loadConfigVariables(void) {
  Config& cfg = configService.getConfig();

  // check if WiFi SSID has changed. If so, restart cpu. Otherwise, the program will not use the new SSID
  if ((strcmp(lastWiFiSSID, "") != 0) && (strcmp(lastWiFiSSID, cfg.wifiSsid) != 0)) {
    log(INFO, "WiFi SSID changed - restarting...");
    ESP.restart();
  }
  strcpy(lastWiFiSSID, cfg.wifiSsid);
}

void configSaved(void) {
  log(INFO, "Config saved. ");
  Config& cfg = configService.getConfig();

  loadConfigVariables();
  configPageActive = false;  // Config saved, no longer on config page
  tick_enable(true);

  // Apply updated settings immediately (LED, speaker, display)
  controller.applyTickSettings(cfg.ledTick, cfg.speakerTick);
  controller.applyDisplaySetting(cfg.showDisplay);

  // Update WiFi connection if credentials changed
  if (cfg.wifiSsid[0] != '\0') {
    wifiService.connectToWiFi(cfg.wifiSsid, cfg.wifiPassword);
  }
}

void handleGetConfig(void) {
  // Check authentication
  if (!checkHttpAuth()) {
    return;  // Auth failed, 401 response already sent
  }

  Config& cfg = configService.getConfig();
  String json = "{";

  // WiFi settings
  json += "\"thingName\":\"" + String(cfg.deviceName) + "\",";
  json += "\"apPassword\":\"********\",";  // Don't expose actual password
  json += "\"wifiSsid\":\"" + String(cfg.wifiSsid) + "\",";
  json += "\"wifiPassword\":\"\",";  // Don't expose actual password
  json += "\"httpAuthUser\":\"" + String(cfg.httpAuthUser) + "\",";
  json += "\"httpAuthPassword\":\"\",";

  // Misc settings
  json += "\"speakerTick\":" + String(cfg.speakerTick ? "true" : "false") + ",";
  json += "\"ledTick\":" + String(cfg.ledTick ? "true" : "false") + ",";
  json += "\"showDisplay\":" + String(cfg.showDisplay ? "true" : "false") + ",";

  // Transmission settings
  json += "\"sendToBle\":" + String(cfg.sendToBle ? "true" : "false") + ",";

  // MQTT settings
  json += "\"sendToMqtt\":" + String(cfg.sendToMqtt ? "true" : "false") + ",";
  json += "\"mqttHost\":\"" + String(cfg.mqttHost) + "\",";
  json += "\"mqttPort\":" + String(cfg.mqttPort) + ",";
  json += "\"mqttUseTls\":" + String(cfg.mqttUseTls ? "true" : "false") + ",";
  json += "\"mqttRetain\":" + String(cfg.mqttRetain ? "true" : "false") + ",";
  json += "\"mqttUsername\":\"" + String(cfg.mqttUsername) + "\",";
  json += "\"mqttPassword\":\"\",";  // Don't expose actual password
  json += "\"mqttBaseTopic\":\"" + String(cfg.mqttBaseTopic) + "\",";

  // LoRa settings
  json += "\"hasLora\":" + String(isLoraBoard ? "true" : "false") + ",";
  if (isLoraBoard) {
    json += "\"sendToLora\":" + String(cfg.sendToLora ? "true" : "false") + ",";
    json += "\"devaddr\":\"" + String(cfg.devaddr) + "\",";
    json += "\"nwkskey\":\"" + String(cfg.nwkskey) + "\",";
    json += "\"appskey\":\"" + String(cfg.appskey) + "\",";
  }

  // Alarm settings
  json += "\"soundLocalAlarm\":" + String(cfg.soundLocalAlarm ? "true" : "false") + ",";
  json += "\"localAlarmThreshold\":" + String(cfg.localAlarmThreshold, 1) + ",";
  json += "\"localAlarmFactor\":" + String(cfg.localAlarmFactor);

  json += "}";

  server.send(200, "application/json", json);
}

void handlePostConfig(void) {
  // Check authentication
  if (!checkHttpAuth()) {
    return;  // Auth failed, 401 response already sent
  }

  // Check rate limiting
  if (!checkRateLimit()) {
    return;  // Rate limit exceeded, 429 response already sent
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body missing\"}");
    return;
  }

  String body = server.arg("plain");

  // Validate body size (protect against DoS)
  if (body.length() > 4096) {  // Max 4KB
    log(WARNING, "Config POST body too large: %d bytes", body.length());
    server.send(413, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
    return;
  }

  log(INFO, "Received config update (%d bytes)", body.length());

  Config& cfg = configService.getConfig();
  bool authChanged = false;

  // Parse JSON manually (simple approach - ESP32 can use ArduinoJson if needed)
  int idx;

  // WiFi settings (only update if provided and not empty)
  idx = body.indexOf("\"thingName\":\"");
  if (idx >= 0) {
    int start = idx + 13;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      if (val.length() > 0 && validateString(val, DEVICE_NAME_LEN)) {
        strncpy(cfg.deviceName, val.c_str(), DEVICE_NAME_LEN);
        cfg.deviceName[DEVICE_NAME_LEN - 1] = '\0';
      } else if (val.length() >= DEVICE_NAME_LEN) {
        log(WARNING, "thingName too long, ignoring");
      }
    }
  }

  idx = body.indexOf("\"wifiSsid\":\"");
  if (idx >= 0) {
    int start = idx + 12;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      if (validateString(val, WIFI_SSID_LEN)) {
        strncpy(cfg.wifiSsid, val.c_str(), WIFI_SSID_LEN);
        cfg.wifiSsid[WIFI_SSID_LEN - 1] = '\0';
      } else if (val.length() >= WIFI_SSID_LEN) {
        log(WARNING, "wifiSsid too long, ignoring");
      }
    }
  }

  idx = body.indexOf("\"wifiPassword\":\"");
  if (idx >= 0) {
    int start = idx + 16;
    int end = body.indexOf("\"", start);
    if (end > start && end > start + 1) {  // Only update if password is provided
      String val = body.substring(start, end);
      if (val.length() > 0 && validateString(val, WIFI_PASS_LEN)) {
        strncpy(cfg.wifiPassword, val.c_str(), WIFI_PASS_LEN);
        cfg.wifiPassword[WIFI_PASS_LEN - 1] = '\0';
      } else if (val.length() >= WIFI_PASS_LEN) {
        log(WARNING, "wifiPassword too long, ignoring");
      }
    }
  }

  // HTTP auth settings
  idx = body.indexOf("\"httpAuthUser\":\"");
  if (idx >= 0) {
    log(INFO, "[CONFIG DEBUG] Found httpAuthUser in request at idx=%d", idx);
    const char *key = "\"httpAuthUser\":\"";
    int start = idx + strlen(key);
    int end = body.indexOf("\"", start);
    log(INFO, "[CONFIG DEBUG] Parsing: start=%d, end=%d", start, end);

    // DEBUG: Show what we're extracting
    int needleLen = strlen(key);
    log(INFO, "[CONFIG DEBUG] Needle: '%s' (len=%d)", key, needleLen);
    log(INFO, "[CONFIG DEBUG] Using offset: %d (FIXED!)", needleLen);

    if (end > start) {
      String val = body.substring(start, end);
      log(INFO, "[CONFIG DEBUG] Extracted httpAuthUser value: '%s' (len=%d)", val.c_str(), val.length());
      log(INFO, "[CONFIG DEBUG] Current stored value: '%s'", cfg.httpAuthUser);

      if (val.length() > 0) {
        if (validateString(val, HTTP_AUTH_LEN)) {
          if (val != String(cfg.httpAuthUser)) {
            log(INFO, "[CONFIG DEBUG] httpAuthUser changed, updating from '%s' to '%s'",
                cfg.httpAuthUser, val.c_str());
            strncpy(cfg.httpAuthUser, val.c_str(), HTTP_AUTH_LEN);
            cfg.httpAuthUser[HTTP_AUTH_LEN - 1] = '\0';
            log(INFO, "[CONFIG DEBUG] After strncpy: '%s'", cfg.httpAuthUser);
            authChanged = true;
          } else {
            log(INFO, "[CONFIG DEBUG] httpAuthUser unchanged");
          }
        } else if (val.length() >= HTTP_AUTH_LEN) {
          log(WARNING, "httpAuthUser too long, ignoring");
        }
      }
    }
  }

  idx = body.indexOf("\"httpAuthPassword\":\"");
  if (idx >= 0) {
    const char *key = "\"httpAuthPassword\":\"";
    int start = idx + strlen(key);
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      if (val.length() == 0) {
        // Empty string => do not change password
      } else if (validateString(val, HTTP_AUTH_LEN)) {
        if (val != String(cfg.httpAuthPass)) {
          strncpy(cfg.httpAuthPass, val.c_str(), HTTP_AUTH_LEN);
          cfg.httpAuthPass[HTTP_AUTH_LEN - 1] = '\0';
          authChanged = true;
        }
      } else if (val.length() >= HTTP_AUTH_LEN) {
        log(WARNING, "httpAuthPassword too long, ignoring");
      }
    }
  }

  // Boolean settings - simple parse
  cfg.speakerTick = body.indexOf("\"speakerTick\":true") >= 0;
  cfg.ledTick = body.indexOf("\"ledTick\":true") >= 0;
  cfg.showDisplay = body.indexOf("\"showDisplay\":true") >= 0;
  cfg.sendToBle = body.indexOf("\"sendToBle\":true") >= 0;
  cfg.sendToMqtt = body.indexOf("\"sendToMqtt\":true") >= 0;
  cfg.mqttUseTls = body.indexOf("\"mqttUseTls\":true") >= 0;
  cfg.mqttRetain = body.indexOf("\"mqttRetain\":true") >= 0;
  cfg.soundLocalAlarm = body.indexOf("\"soundLocalAlarm\":true") >= 0;

  if (isLoraBoard) {
    cfg.sendToLora = body.indexOf("\"sendToLora\":true") >= 0;
  }

  // String settings
  idx = body.indexOf("\"mqttHost\":\"");
  if (idx >= 0) {
    int start = idx + 12;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      if (val.length() < MQTT_HOST_LEN) {
        strncpy(cfg.mqttHost, val.c_str(), MQTT_HOST_LEN);
        cfg.mqttHost[MQTT_HOST_LEN - 1] = '\0';
      } else {
        log(WARNING, "mqttHost too long, ignoring");
      }
    }
  }

  idx = body.indexOf("\"mqttUsername\":\"");
  if (idx >= 0) {
    int start = idx + 16;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      if (val.length() < MQTT_USER_LEN) {
        strncpy(cfg.mqttUsername, val.c_str(), MQTT_USER_LEN);
        cfg.mqttUsername[MQTT_USER_LEN - 1] = '\0';
      } else {
        log(WARNING, "mqttUsername too long, ignoring");
      }
    }
  }

  idx = body.indexOf("\"mqttPassword\":\"");
  if (idx >= 0) {
    int start = idx + 16;
    int end = body.indexOf("\"", start);
    if (end > start && end > start + 1) {
      String val = body.substring(start, end);
      if (val.length() > 0 && val.length() < MQTT_PASS_LEN) {
        strncpy(cfg.mqttPassword, val.c_str(), MQTT_PASS_LEN);
        cfg.mqttPassword[MQTT_PASS_LEN - 1] = '\0';
      } else if (val.length() >= MQTT_PASS_LEN) {
        log(WARNING, "mqttPassword too long, ignoring");
      }
    }
  }

  idx = body.indexOf("\"mqttBaseTopic\":\"");
  if (idx >= 0) {
    int start = idx + 17;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      if (val.length() < MQTT_TOPIC_LEN) {
        strncpy(cfg.mqttBaseTopic, val.c_str(), MQTT_TOPIC_LEN);
        cfg.mqttBaseTopic[MQTT_TOPIC_LEN - 1] = '\0';
      } else {
        log(WARNING, "mqttBaseTopic too long, ignoring");
      }
    }
  }

  // Numeric settings
  idx = body.indexOf("\"mqttPort\":");
  if (idx >= 0) {
    int start = idx + 11;
    int end = body.indexOf(",", start);
    if (end < 0) end = body.indexOf("}", start);
    if (end > start) {
      uint16_t port = body.substring(start, end).toInt();
      if (validatePort(port)) {
        cfg.mqttPort = port;
      } else {
        log(WARNING, "Invalid mqttPort: %d, ignoring", port);
      }
    }
  }

  idx = body.indexOf("\"localAlarmThreshold\":");
  if (idx >= 0) {
    int start = idx + 22;
    int end = body.indexOf(",", start);
    if (end < 0) end = body.indexOf("}", start);
    if (end > start) {
      float threshold = body.substring(start, end).toFloat();
      if (validateFloat(threshold, 0.0, 1000.0)) {  // 0-1000 µSv/h is reasonable
        cfg.localAlarmThreshold = threshold;
      } else {
        log(WARNING, "Invalid localAlarmThreshold: %.2f, ignoring", threshold);
      }
    }
  }

  idx = body.indexOf("\"localAlarmFactor\":");
  if (idx >= 0) {
    int start = idx + 19;
    int end = body.indexOf(",", start);
    if (end < 0) end = body.indexOf("}", start);
    if (end > start) {
      int factor = body.substring(start, end).toInt();
      if (validateInt(factor, 2, 100)) {  // 2-100 as per config
        cfg.localAlarmFactor = factor;
      } else {
        log(WARNING, "Invalid localAlarmFactor: %d, ignoring", factor);
      }
    }
  }

  // ABP LoRa credentials (only if LoRa hardware is present)
  if (isLoraBoard) {
    idx = body.indexOf("\"devaddr\":\"");
    if (idx >= 0) {
      int start = idx + 11;
      int end = body.indexOf("\"", start);
      if (end > start) {
        String val = body.substring(start, end);
        if (validateHexString(val, 8)) {  // DevAddr is 8 hex chars
          strncpy(cfg.devaddr, val.c_str(), LORA_KEY_LEN);
          cfg.devaddr[LORA_KEY_LEN - 1] = '\0';
        } else {
          log(WARNING, "Invalid devaddr (must be 8 hex chars), ignoring");
        }
      }
    }

    idx = body.indexOf("\"nwkskey\":\"");
    if (idx >= 0) {
      int start = idx + 11;
      int end = body.indexOf("\"", start);
      if (end > start) {
        String val = body.substring(start, end);
        if (validateHexString(val, 32)) {  // NwkSKey is 32 hex chars
          strncpy(cfg.nwkskey, val.c_str(), LORA_KEY_LEN);
          cfg.nwkskey[LORA_KEY_LEN - 1] = '\0';
        } else {
          log(WARNING, "Invalid nwkskey (must be 32 hex chars), ignoring");
        }
      }
    }

    idx = body.indexOf("\"appskey\":\"");
    if (idx >= 0) {
      int start = idx + 11;
      int end = body.indexOf("\"", start);
      if (end > start) {
        String val = body.substring(start, end);
        if (validateHexString(val, 32)) {  // AppSKey is 32 hex chars
          strncpy(cfg.appskey, val.c_str(), LORA_KEY_LEN);
          cfg.appskey[LORA_KEY_LEN - 1] = '\0';
        } else {
          log(WARNING, "Invalid appskey (must be 32 hex chars), ignoring");
        }
      }
    }
  }

  // Save configuration
  configService.save();
  configSaved();

  if (authChanged) {
    clearSessions();
    ArduinoOTA.setPassword(cfg.httpAuthPass);
    log(INFO, "HTTP auth credentials changed - sessions cleared");
  }

  // Send success response
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Configuration saved\"}");

  // Schedule restart after TCP connection is properly closed
  // This prevents crashes due to incomplete HTTP response transmission
  restartScheduled = true;
  restartTime = millis() + 2000;  // Restart in 2 seconds
  log(INFO, "Restart scheduled in 2 seconds...");
}

void setup_webconf(bool loraHardware) {
  isLoraBoard = loraHardware;

  // Initialize ConfigService
  if (!configService.begin()) {
    log(ERROR, "Failed to initialize ConfigService");
  }

  Config& cfg = configService.getConfig();

  // TEMPORARY FIX: Reset corrupted auth credentials to defaults
  // TODO: Remove this code after credentials are fixed!
  log(WARNING, "=== TEMPORARY AUTH RESET ACTIVE ===");
  log(WARNING, "Current httpAuthUser: '%s' (len=%d)", cfg.httpAuthUser, strlen(cfg.httpAuthUser));
  log(WARNING, "Current httpAuthPass: '%s' (len=%d)", cfg.httpAuthPass, strlen(cfg.httpAuthPass));

  if (strcmp(cfg.httpAuthUser, HTTP_AUTH_USER) != 0 || strcmp(cfg.httpAuthPass, HTTP_AUTH_PASS) != 0) {
    log(WARNING, "Auth credentials are corrupted! Resetting to defaults...");
    strncpy(cfg.httpAuthUser, HTTP_AUTH_USER, HTTP_AUTH_LEN);
    cfg.httpAuthUser[HTTP_AUTH_LEN - 1] = '\0';
    strncpy(cfg.httpAuthPass, HTTP_AUTH_PASS, HTTP_AUTH_LEN);
    cfg.httpAuthPass[HTTP_AUTH_LEN - 1] = '\0';
    configService.save();
    log(WARNING, "Auth credentials RESET to: user='%s', pass='%s'", cfg.httpAuthUser, cfg.httpAuthPass);
    log(WARNING, "=== PLEASE REMOVE THIS TEMPORARY CODE AFTER VERIFYING LOGIN WORKS ===");
  } else {
    log(INFO, "Auth credentials are correct - no reset needed");
  }

  // if we don't have LoRa hardware, do not send to LoRa
  if (!isLoraBoard) {
    cfg.sendToLora = false;
  }

  // Build SSID from device name
  buildSSID();

  // Initialize WiFi Service
  wifiService.begin(cfg.deviceName, cfg.apPassword);
  wifiService.setApTimeout(30000);         // AP timeout: 30 seconds if no client connects
  wifiService.setConnectionTimeout(20000);  // STA connect timeout: 20 seconds

  // If WiFi credentials are configured, try to connect
  if (configService.hasWifiConfig()) {
    wifiService.connectToWiFi(cfg.wifiSsid, cfg.wifiPassword);
  }

  loadConfigVariables();

  // Setup ArduinoOTA callbacks (will begin() later when WiFi is ready)
  ArduinoOTA.setHostname(cfg.deviceName);
  ArduinoOTA.setPassword(cfg.httpAuthPass);  // Use same password as HTTP auth

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    log(INFO, "ArduinoOTA: Start updating %s", type.c_str());
    tick_enable(false);  // Disable ticks during OTA
  });

  ArduinoOTA.onEnd([]() {
    log(INFO, "ArduinoOTA: Update complete");
    tick_enable(true);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int lastPercent = 0;
    unsigned int percent = (progress / (total / 100));
    if (percent != lastPercent && percent % 10 == 0) {
      log(INFO, "ArduinoOTA: Progress: %u%%", percent);
      lastPercent = percent;
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    log(ERROR, "ArduinoOTA: Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) log(ERROR, "Auth Failed");
    else if (error == OTA_BEGIN_ERROR) log(ERROR, "Begin Failed");
    else if (error == OTA_CONNECT_ERROR) log(ERROR, "Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) log(ERROR, "Receive Failed");
    else if (error == OTA_END_ERROR) log(ERROR, "End Failed");
    tick_enable(true);  // Re-enable ticks after error
  });

  // Note: ArduinoOTA.begin() will be called in poll_transmission() once WiFi is connected

  auto redirectToCaptivePortal = []() {
    log(INFO, "Captive portal probe detected, redirecting to dashboard");
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  };

  // Enable CORS for API endpoints
  server.enableCORS(true);
  // Collect headers we need to inspect (cookies, bearer tokens, client time)
  const char *headerKeys[] = {"Cookie", "Authorization", "X-Client-Time"};
  server.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.on("/config", handleConfigPage);
  server.on("/config/", handleConfigPage);
  server.on("/api/status", handleApiStatus);
  server.on("/api/auth/login", HTTP_POST, handleAuthLogin);

  // Config API endpoints
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);
  server.on("/api/config/ping", HTTP_POST, handleConfigPing);

  // Time API endpoint
  server.on("/api/time", HTTP_GET, handleApiTime);

  // Captive portal probes (Android/Windows/Apple) - redirect to config page
  server.on("/generate_204", HTTP_ANY, redirectToCaptivePortal);      // Android
  server.on("/gen_204", HTTP_ANY, redirectToCaptivePortal);           // older Android variants
  server.on("/hotspot-detect.html", HTTP_ANY, redirectToCaptivePortal);  // Apple
  server.on("/ncsi.txt", HTTP_ANY, redirectToCaptivePortal);          // Windows

  // OTA Update endpoint - explicitly register handlers for HTTPUpdateServer
  server.on("/update", HTTP_GET, []() {
    if (!checkHttpAuth()) return;
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, []() {
    if (!checkHttpAuth()) {
      return;
    }
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    // Upload handler
    if (!checkHttpAuth()) {
      return;
    }
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      log(INFO, "Update: %s", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        log(INFO, "Update Success: %u bytes", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.onNotFound([]() {
    const String path = server.uri();

    // Common browser requests that we can silently ignore
    if (path == "/favicon.ico" || path == "/robots.txt" || path == "/sitemap.xml") {
      server.send(404, "text/plain", "");
      return;
    }

    // Try to serve web asset
    if (sendWebAsset(path)) {
      return;
    }

    // SPA fallback
    if (sendWebAsset("/index.html")) {
      return;
    }

    // Log unhandled requests for debugging
    log(WARNING, "Unhandled request: %s %s", server.method() == HTTP_GET ? "GET" : "POST", path.c_str());
    server.send(404, "text/plain", "Not found");
  });

  // Start web server
  server.begin();
  log(INFO, "Web server started");
}
