// measurements data transmission related code
// - via WiFi to internet servers
// - via LoRa to TTN (to internet servers)

#include "wifi.hpp"

#include <string.h>

#include "app/controller.hpp"
#include "web_assets.h"

extern MultiGeigerController controller;

#ifndef MQTT_BASE_TOPIC
#define MQTT_BASE_TOPIC ""
#endif

extern IotWebConf iotWebConf;

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
// Hosts for data delivery

// use http for now, could we use https?
#define MADAVI "http://api-rrd.madavi.de/data.php"

// use http for now, server operator tells there are performance issues with https.
#define SENSORCOMMUNITY "http://api.sensor.community/v1/push-sensor-data/"

// Send http(s) post requests to a custom server
// Note: Custom toilet URLs from https://ptsv2.com/ can be used for debugging
// and work with https and http.
#define CUSTOMSRV "https://ptsv2.com/t/xxxxx-yyyyyyyyyy/post"
// Get your own toilet URL and put it here before setting this to true.
#define SEND2CUSTOMSRV false

static String http_software_version;
static unsigned int lora_software_version;
static String chipID;
static bool isLoraBoard;

typedef struct https_client {
  WiFiClientSecure *wc;
  HTTPClient *hc;
} HttpsClient;

static HttpsClient c_madavi, c_sensorc, c_customsrv;

void setup_transmission(const char *version, char *ssid, bool loraHardware) {
  chipID = String(ssid);
  chipID.replace("ESP32", "esp32");
  isLoraBoard = loraHardware;

  http_software_version = String(version);

  if (isLoraBoard) {
    int major, minor, patch;
    sscanf(version, "V%d.%d.%d", &major, &minor, &patch);
    lora_software_version = (major << 12) + (minor << 4) + patch;
    setup_lorawan();
  }

  c_madavi.wc = new WiFiClientSecure;
  c_madavi.wc->setCACert(ca_certs);
  c_madavi.hc = new HTTPClient;

  c_sensorc.wc = new WiFiClientSecure;
  c_sensorc.wc->setCACert(ca_certs);
  c_sensorc.hc = new HTTPClient;

  c_customsrv.wc = new WiFiClientSecure;
  c_customsrv.wc->setCACert(ca_certs);
  c_customsrv.hc = new HTTPClient;

  set_status(STATUS_SCOMM, sendToCommunity ? ST_SCOMM_INIT : ST_SCOMM_OFF);
  set_status(STATUS_MADAVI, sendToMadavi ? ST_MADAVI_INIT : ST_MADAVI_OFF);
  set_status(STATUS_TTN, sendToLora ? ST_TTN_INIT : ST_TTN_OFF);
}

void poll_transmission() {
  if (isLoraBoard) {
    // The LMIC needs to be polled a lot; and this is very low cost if the LMIC isn't
    // active. So we just act as a bridge. We need this routine so we can see
    // `isLoraBoard`. Most C compilers will notice the tail call and optimize this
    // to a jump.
    poll_lorawan();
  }
}

void prepare_http(HttpsClient *client, const char *host) {
  if (host[4] == 's')  // https
    client->hc->begin(*client->wc, host);
  else  // http
    client->hc->begin(host);
  client->hc->addHeader("Content-Type", "application/json; charset=UTF-8");
  client->hc->addHeader("Connection", "keep-alive");
  client->hc->addHeader("X-Sensor", chipID);
}

int send_http(HttpsClient *client, String body) {
  if (DEBUG_SERVER_SEND)
    log(DEBUG, "http request body: %s", body.c_str());

  int httpResponseCode = client->hc->POST(body);
  if (httpResponseCode > 0) {
    String response = client->hc->getString();
    if (DEBUG_SERVER_SEND) {
      log(DEBUG, "http code: %d", httpResponseCode);
      log(DEBUG, "http response: %s", response.c_str());
    }
  } else {
    log(ERROR, "Error on sending POST: %d", httpResponseCode);
  }
  client->hc->end();
  return httpResponseCode;
}

int send_http_geiger(HttpsClient *client, const char *host, unsigned int timediff, unsigned int hv_pulses,
                     unsigned int gm_counts, unsigned int cpm, int xpin) {
  char body[1000];
  prepare_http(client, host);
  if (xpin != XPIN_NO_XPIN) {
    client->hc->addHeader("X-PIN", String(xpin));
  }
  const char *json_format = R"=====(
{
 "software_version": "%s",
 "sensordatavalues": [
  {"value_type": "counts_per_minute", "value": "%d"},
  {"value_type": "hv_pulses", "value": "%d"},
  {"value_type": "counts", "value": "%d"},
  {"value_type": "sample_time_ms", "value": "%d"}
 ]
}
)=====";
  snprintf(body, 1000, json_format,
           http_software_version.c_str(),
           cpm,
           hv_pulses,
           gm_counts,
           timediff);
  return send_http(client, body);
}

int send_http_thp(HttpsClient *client, const char *host, float temperature, float humidity, float pressure, int xpin) {
  char body[1000];
  prepare_http(client, host);
  if(xpin != XPIN_NO_XPIN) {
    client->hc->addHeader("X-PIN", String(xpin));
  }
  const char *json_format = R"=====(
{
 "software_version": "%s",
 "sensordatavalues": [
  {"value_type": "temperature", "value": "%.2f"},
  {"value_type": "humidity", "value": "%.2f"},
  {"value_type": "pressure", "value": "%.2f"}
 ]
}
)=====";
  snprintf(body, 1000, json_format,
           http_software_version.c_str(),
           temperature,
           humidity,
           pressure);
  return send_http(client, body);
}

// two extra functions for MADAVI, because MADAVI needs the sensorname in value_type to recognize the sensors
int send_http_geiger_2_madavi(HttpsClient *client, String tube_type, unsigned int timediff, unsigned int hv_pulses,
                               unsigned int gm_counts, unsigned int cpm) {
  char body[1000];
  prepare_http(client, MADAVI);
  tube_type = tube_type.substring(10);
  const char *json_format = R"=====(
{
 "software_version": "%s",
 "sensordatavalues": [
  {"value_type": "%s_counts_per_minute", "value": "%d"},
  {"value_type": "%s_hv_pulses", "value": "%d"},
  {"value_type": "%s_counts", "value": "%d"},
  {"value_type": "%s_sample_time_ms", "value": "%d"}
 ]
}
)=====";
  snprintf(body, 1000, json_format,
           http_software_version.c_str(),
           tube_type.c_str(), cpm,
           tube_type.c_str(), hv_pulses,
           tube_type.c_str(), gm_counts,
           tube_type.c_str(), timediff);
  return send_http(client, body);
}

int send_http_thp_2_madavi(HttpsClient *client, float temperature, float humidity, float pressure) {
  char body[1000];
  prepare_http(client, MADAVI);
  const char *json_format = R"=====(
{
 "software_version": "%s",
 "sensordatavalues": [
  {"value_type": "BME280_temperature", "value": "%.2f"},
  {"value_type": "BME280_humidity", "value": "%.2f"},
  {"value_type": "BME280_pressure", "value": "%.2f"}
 ]
}
)=====";
  snprintf(body, 1000, json_format,
           http_software_version.c_str(),
           temperature,
           humidity,
           pressure);
  return send_http(client, body);
}

// LoRa payload:
// To minimise airtime and follow the 'TTN Fair Access Policy', we only send necessary bytes.
// We do NOT use Cayenne LPP.
// The payload will be translated via http integration and a small program to be compatible with sensor.community.
// For byte definitions see ttn2luft.pdf in docs directory.
int send_ttn_geiger(int tube_nbr, unsigned int dt, unsigned int gm_counts) {
  unsigned char ttnData[10];
  // first the number of GM counts
  ttnData[0] = (gm_counts >> 24) & 0xFF;
  ttnData[1] = (gm_counts >> 16) & 0xFF;
  ttnData[2] = (gm_counts >> 8) & 0xFF;
  ttnData[3] = gm_counts & 0xFF;
  // now 3 bytes for the measurement interval [in ms] (max ca. 4 hours)
  ttnData[4] = (dt >> 16) & 0xFF;
  ttnData[5] = (dt >> 8) & 0xFF;
  ttnData[6] = dt & 0xFF;
  // next two bytes are software version
  ttnData[7] = (lora_software_version >> 8) & 0xFF;
  ttnData[8] = lora_software_version & 0xFF;
  // next byte is the tube number
  ttnData[9] = tube_nbr;
  return lorawan_send(1, ttnData, 10, false, NULL, NULL, NULL);
}

int send_ttn_thp(float temperature, float humidity, float pressure) {
  unsigned char ttnData[5];
  ttnData[0] = ((int)(temperature * 10)) >> 8;
  ttnData[1] = ((int)(temperature * 10)) & 0xFF;
  ttnData[2] = (int)(humidity * 2);
  ttnData[3] = ((int)(pressure / 10)) >> 8;
  ttnData[4] = ((int)(pressure / 10)) & 0xFF;
  return lorawan_send(2, ttnData, 5, false, NULL, NULL, NULL);
}

void transmit_data(String tube_type, int tube_nbr, unsigned int dt, unsigned int hv_pulses, unsigned int gm_counts, unsigned int cpm,
                   int have_thp, float temperature, float humidity, float pressure, int wifi_status) {
  int rc1, rc2;

  #if SEND2CUSTOMSRV
  bool customsrv_ok;
  log(INFO, "Sending to CUSTOMSRV ...");
  rc1 = send_http_geiger(&c_customsrv, CUSTOMSRV, dt, hv_pulses, gm_counts, cpm, XPIN_NO_XPIN);
  rc2 = have_thp ? send_http_thp(&c_customsrv, CUSTOMSRV, temperature, humidity, pressure, XPIN_NO_XPIN) : 200;
  customsrv_ok = (rc1 == 200) && (rc2 == 200);
  log(INFO, "Sent to CUSTOMSRV, status: %s, http: %d %d", customsrv_ok ? "ok" : "error", rc1, rc2);
  #endif

  if(sendToMadavi && (wifi_status == ST_WIFI_CONNECTED)) {
    bool madavi_ok;
    log(INFO, "Sending to Madavi ...");
    set_status(STATUS_MADAVI, ST_MADAVI_SENDING);
    display_status();
    rc1 = send_http_geiger_2_madavi(&c_madavi, tube_type, dt, hv_pulses, gm_counts, cpm);
    rc2 = have_thp ? send_http_thp_2_madavi(&c_madavi, temperature, humidity, pressure) : 200;
    delay(300);
    madavi_ok = (rc1 == 200) && (rc2 == 200);
    log(INFO, "Sent to Madavi, status: %s, http: %d %d", madavi_ok ? "ok" : "error", rc1, rc2);
    set_status(STATUS_MADAVI, madavi_ok ? ST_MADAVI_IDLE : ST_MADAVI_ERROR);
    display_status();
  }

  if(sendToCommunity  && (wifi_status == ST_WIFI_CONNECTED)) {
    bool scomm_ok;
    log(INFO, "Sending to sensor.community ...");
    set_status(STATUS_SCOMM, ST_SCOMM_SENDING);
    display_status();
    rc1 = send_http_geiger(&c_sensorc, SENSORCOMMUNITY, dt, hv_pulses, gm_counts, cpm, XPIN_RADIATION);
    rc2 = have_thp ? send_http_thp(&c_sensorc, SENSORCOMMUNITY, temperature, humidity, pressure, XPIN_BME280) : 201;
    delay(300);
    scomm_ok = (rc1 == 201) && (rc2 == 201);
    log(INFO, "Sent to sensor.community, status: %s, http: %d %d", scomm_ok ? "ok" : "error", rc1, rc2);
    set_status(STATUS_SCOMM, scomm_ok ? ST_SCOMM_IDLE : ST_SCOMM_ERROR);
    display_status();
  }

  if(isLoraBoard && sendToLora && (strcmp(appeui, "") != 0)) {    // send only, if we have LoRa credentials
    bool ttn_ok;
    log(INFO, "Sending to TTN ...");
    set_status(STATUS_TTN, ST_TTN_SENDING);
    display_status();
    rc1 = send_ttn_geiger(tube_nbr, dt, gm_counts);
    rc2 = have_thp ? send_ttn_thp(temperature, humidity, pressure) : TX_STATUS_UPLINK_SUCCESS;
    ttn_ok = (rc1 == TX_STATUS_UPLINK_SUCCESS) && (rc2 == TX_STATUS_UPLINK_SUCCESS);
    set_status(STATUS_TTN, ttn_ok ? ST_TTN_IDLE : ST_TTN_ERROR);
    display_status();
  }
}

// Web Configuration related code
// also: OTA updates

// Checkboxes have 'selected' if checked, so we need 9 byte for this string.
#define CHECKBOX_LEN 9

bool speakerTick = SPEAKER_TICK;
bool playSound = PLAY_SOUND;
bool ledTick = LED_TICK;
bool showDisplay = SHOW_DISPLAY;
bool sendToCommunity = SEND2SENSORCOMMUNITY;
bool sendToMadavi = SEND2MADAVI;
bool sendToLora = SEND2LORA;
bool sendToBle = SEND2BLE;
bool soundLocalAlarm = LOCAL_ALARM_SOUND;
bool sendToMqtt = SEND2MQTT;

#define MQTT_HOST_LEN 64
#define MQTT_USER_LEN IOTWEBCONF_WORD_LEN
#define MQTT_PASS_LEN IOTWEBCONF_WORD_LEN
#define MQTT_BASE_TOPIC_LEN 64

char mqttHost[MQTT_HOST_LEN] = MQTT_BROKER;
uint16_t mqttPort = MQTT_PORT;
bool mqttUseTls = MQTT_USE_TLS;
bool mqttRetain = MQTT_RETAIN;
int mqttQos = MQTT_QOS;
char mqttUsername[MQTT_USER_LEN] = MQTT_USERNAME;
char mqttPassword[MQTT_PASS_LEN] = MQTT_PASSWORD;
char mqttBaseTopic[MQTT_BASE_TOPIC_LEN] = MQTT_BASE_TOPIC;

char speakerTick_c[CHECKBOX_LEN];
char playSound_c[CHECKBOX_LEN];
char ledTick_c[CHECKBOX_LEN];
char showDisplay_c[CHECKBOX_LEN];
char sendToCommunity_c[CHECKBOX_LEN];
char sendToMadavi_c[CHECKBOX_LEN];
char sendToLora_c[CHECKBOX_LEN];
char sendToBle_c[CHECKBOX_LEN];
char soundLocalAlarm_c[CHECKBOX_LEN];
char sendToMqtt_c[CHECKBOX_LEN];
char mqttUseTls_c[CHECKBOX_LEN];
char mqttRetain_c[CHECKBOX_LEN];

char appeui[17] = "";
char deveui[17] = "";
char appkey[IOTWEBCONF_WORD_LEN] = "";

float localAlarmThreshold = LOCAL_ALARM_THRESHOLD;
int localAlarmFactor = (int)LOCAL_ALARM_FACTOR;

iotwebconf::ParameterGroup grpMisc = iotwebconf::ParameterGroup("misc", "Misc. Settings");
iotwebconf::CheckboxParameter startSoundParam = iotwebconf::CheckboxParameter("Start sound", "startSound", playSound_c, CHECKBOX_LEN, playSound);
iotwebconf::CheckboxParameter speakerTickParam = iotwebconf::CheckboxParameter("Speaker tick", "speakerTick", speakerTick_c, CHECKBOX_LEN, speakerTick);
iotwebconf::CheckboxParameter ledTickParam = iotwebconf::CheckboxParameter("LED tick", "ledTick", ledTick_c, CHECKBOX_LEN, ledTick);
iotwebconf::CheckboxParameter showDisplayParam = iotwebconf::CheckboxParameter("Show display", "showDisplay", showDisplay_c, CHECKBOX_LEN, showDisplay);

iotwebconf::ParameterGroup grpTransmission = iotwebconf::ParameterGroup("transmission", "Transmission Settings");
iotwebconf::CheckboxParameter sendToCommunityParam = iotwebconf::CheckboxParameter("Send to sensor.community", "send2Community", sendToCommunity_c, CHECKBOX_LEN, sendToCommunity);
iotwebconf::CheckboxParameter sendToMadaviParam = iotwebconf::CheckboxParameter("Send to madavi.de", "send2Madavi", sendToMadavi_c, CHECKBOX_LEN, sendToMadavi);
iotwebconf::CheckboxParameter sendToBleParam = iotwebconf::CheckboxParameter("Send to BLE (Reboot required!)", "send2ble", sendToBle_c, CHECKBOX_LEN, sendToBle);

iotwebconf::ParameterGroup grpLoRa = iotwebconf::ParameterGroup("lora", "LoRa Settings");
iotwebconf::CheckboxParameter sendToLoraParam = iotwebconf::CheckboxParameter("Send to LoRa (=>TTN)", "send2lora", sendToLora_c, CHECKBOX_LEN, sendToLora);
iotwebconf::TextParameter deveuiParam = iotwebconf::TextParameter("DEVEUI", "deveui", deveui, 17);
iotwebconf::TextParameter appeuiParam = iotwebconf::TextParameter("APPEUI", "appeui", appeui, 17);
iotwebconf::TextParameter appkeyParam = iotwebconf::TextParameter("APPKEY", "appkey", appkey, 33);

iotwebconf::ParameterGroup grpMqtt = iotwebconf::ParameterGroup("mqtt", "MQTT Settings");
iotwebconf::CheckboxParameter sendToMqttParam = iotwebconf::CheckboxParameter("Send to MQTT", "send2mqtt", sendToMqtt_c, CHECKBOX_LEN, sendToMqtt);
iotwebconf::TextParameter mqttHostParam = iotwebconf::TextParameter("MQTT host", "mqttHost", mqttHost, MQTT_HOST_LEN);
auto mqttPortParam =
  iotwebconf::Builder<iotwebconf::IntTParameter<uint16_t>>("mqttPort")
  .label("MQTT port")
  .defaultValue(mqttPort)
  .min(1).max(65535)
  .placeholder("1883")
  .build();
iotwebconf::CheckboxParameter mqttUseTlsParam = iotwebconf::CheckboxParameter("Use TLS (insecure PoC)", "mqttTls", mqttUseTls_c, CHECKBOX_LEN, mqttUseTls);
iotwebconf::CheckboxParameter mqttRetainParam = iotwebconf::CheckboxParameter("Retain MQTT messages", "mqttRetain", mqttRetain_c, CHECKBOX_LEN, mqttRetain);
iotwebconf::TextParameter mqttUserParam = iotwebconf::TextParameter("MQTT username", "mqttUser", mqttUsername, MQTT_USER_LEN);
iotwebconf::TextParameter mqttPassParam = iotwebconf::TextParameter("MQTT password", "mqttPass", mqttPassword, MQTT_PASS_LEN);
iotwebconf::TextParameter mqttBaseTopicParam = iotwebconf::TextParameter("Base topic (optional)", "mqttBase", mqttBaseTopic, MQTT_BASE_TOPIC_LEN);

iotwebconf::ParameterGroup grpAlarm = iotwebconf::ParameterGroup("alarm", "Local Alarm Setting");
iotwebconf::CheckboxParameter soundLocalAlarmParam = iotwebconf::CheckboxParameter("Enable local alarm sound", "soundLocalAlarm", soundLocalAlarm_c, CHECKBOX_LEN, soundLocalAlarm);
iotwebconf::FloatTParameter localAlarmThresholdParam =
  iotwebconf::Builder<iotwebconf::FloatTParameter>("localAlarmThreshold").
  label("Local alarm threshold (µSv/h)").
  defaultValue(localAlarmThreshold).
  step(0.1).placeholder("e.g. 0.5").build();
iotwebconf::IntTParameter<int16_t> localAlarmFactorParam =
  iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("localAlarmFactor").
  label("Factor of current dose rate vs. accumulated").
  defaultValue(localAlarmFactor).
  min(2).max(100).
  step(1).placeholder("2..100").build();

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;

char *buildSSID(void);

// SSID == thingName
const char *theName = buildSSID();
char ssid[IOTWEBCONF_WORD_LEN];  // LEN == 33 (2020-01-13)

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "ESP32Geiger";

IotWebConf iotWebConf(theName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

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
  // build SSID from ESP chip id
  uint32_t id = getESPchipID();
  sprintf(ssid, "ESP32-%d", id);
  return ssid;
}

/**
 * @brief API endpoint for live status data (JSON)
 */
void handleApiStatus(void) {
  unsigned long counts = controller.getCounts();
  float temp = controller.getTemperature();
  float hum = controller.getHumidity();
  float press = controller.getPressure();
  bool thp = controller.hasThp();
  bool hvErr = controller.hasHvError();

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
  json += "\"hv_error\":" + String(hvErr ? "true" : "false") + ",";

  if (thp) {
    json += "\"temperature\":" + String(temp, 1) + ",";
    json += "\"humidity\":" + String(hum, 1) + ",";
    json += "\"pressure\":" + String(press, 1) + ",";
  }

  json += "\"has_thp\":" + String(thp ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

void handleRoot(void) {  // Handle web requests to "/" path.
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal()) {
    // -- Captive portal requests were already served.
    return;
  }

  // looks like user wants to do some configuration or maybe flash firmware.
  // while accessing the flash, we need to turn ticking off to avoid exceptions.
  // user needs to save the config (or flash firmware + reboot) to turn it on again.
  // note: it didn't look like there is an easy way to put this call at the right place
  // (start of fw flash / start of config save) - this is why it is here.
  tick_enable(false);

  // Auto-redirect to config page in AP mode (Captive Portal behavior)
  if (iotWebConf.getState() == iotwebconf::ApMode) {
    log(INFO, "Captive portal: redirecting to /config");
    server.sendHeader("Location", "/config");
    server.send(302, "text/plain", "");
    return;
  }

  // Serve modern dashboard with live data
  serveCompressed(server, dashboard_html_gz, dashboard_html_gz_len, "text/html");
}

static char lastWiFiSSID[IOTWEBCONF_WORD_LEN] = "";
static WiFiEventId_t wifiEventId;
static WiFiEventId_t apConnectEventId;
static WiFiEventId_t apDisconnectEventId;

static bool hasConfiguredWifi() {
  const char *cfgSsid = iotWebConf.getWifiSsidParameter()->valueBuffer;
  return (cfgSsid != nullptr) && (strlen(cfgSsid) > 0);
}

static void onWifiEvent(arduino_event_id_t event, arduino_event_info_t info) {
  if (event != ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
    return;

  int reason = info.wifi_sta_disconnected.reason;
  log(INFO, "WiFi disconnect event (reason=%d)", reason);

  if (!hasConfiguredWifi()) {
    log(INFO, "WiFi reconnect skipped: no client SSID configured");
    return;
  }

  log(INFO, "WiFi reconnecting to configured network via IotWebConf");
  // The regular doLoop will push us back to Connecting state.
}

static void onApClientConnectEvent(arduino_event_id_t event, arduino_event_info_t info) {
  if (event != ARDUINO_EVENT_WIFI_AP_STACONNECTED)
    return;

  log(INFO, "AP client connected, keeping AP open indefinitely");
  // Disable AP timeout when client connects - keep AP open
  iotWebConf.setApTimeoutMs(0);  // 0 = no timeout, AP stays open
}

static void onApClientDisconnectEvent(arduino_event_id_t event, arduino_event_info_t info) {
  if (event != ARDUINO_EVENT_WIFI_AP_STADISCONNECTED)
    return;

  log(INFO, "AP client disconnected");

  // If WiFi STA is configured, switch to STA mode
  if (hasConfiguredWifi() && iotWebConf.getState() == iotwebconf::ApMode) {
    log(INFO, "WiFi STA configured, switching to STA mode");
    iotWebConf.forceApMode(false);  // will change state to Connecting if allowed
  }
}

void loadConfigVariables(void) {
  // check if WiFi SSID has changed. If so, restart cpu. Otherwise, the program will not use the new SSID
  if ((strcmp(lastWiFiSSID, "") != 0) && (strcmp(lastWiFiSSID, iotWebConf.getWifiSsidParameter()->valueBuffer) != 0)) {
    log(INFO, "Doing restart...");
    ESP.restart();
  }
  strcpy(lastWiFiSSID, iotWebConf.getWifiSsidParameter()->valueBuffer);

  speakerTick = speakerTickParam.isChecked();
  playSound = startSoundParam.isChecked();
  ledTick = ledTickParam.isChecked();
  showDisplay = showDisplayParam.isChecked();
  sendToCommunity = sendToCommunityParam.isChecked();
  sendToMadavi = sendToMadaviParam.isChecked();
  sendToLora = sendToLoraParam.isChecked();
  sendToBle = sendToBleParam.isChecked();
  soundLocalAlarm = soundLocalAlarmParam.isChecked();
  localAlarmThreshold = localAlarmThresholdParam.value();
  localAlarmFactor = localAlarmFactorParam.value();
  sendToMqtt = sendToMqttParam.isChecked();
  mqttUseTls = mqttUseTlsParam.isChecked();
  mqttRetain = mqttRetainParam.isChecked();
  mqttPort = mqttPortParam.value();
  strncpy(mqttHost, mqttHostParam.valueBuffer, MQTT_HOST_LEN);
  mqttHost[MQTT_HOST_LEN - 1] = '\0';
  strncpy(mqttUsername, mqttUserParam.valueBuffer, MQTT_USER_LEN);
  mqttUsername[MQTT_USER_LEN - 1] = '\0';
  strncpy(mqttPassword, mqttPassParam.valueBuffer, MQTT_PASS_LEN);
  mqttPassword[MQTT_PASS_LEN - 1] = '\0';
  strncpy(mqttBaseTopic, mqttBaseTopicParam.valueBuffer, MQTT_BASE_TOPIC_LEN);
  mqttBaseTopic[MQTT_BASE_TOPIC_LEN - 1] = '\0';
}

void configSaved(void) {
  log(INFO, "Config saved. ");
  loadConfigVariables();
  tick_enable(true);
  // Apply updated settings immediately (LED, speaker, display)
  controller.applyTickSettings(ledTick, speakerTick);
  controller.applyDisplaySetting(showDisplay);
}

void handleGetConfig(void) {
  String json = "{";

  // WiFi settings
  json += "\"thingName\":\"" + String(iotWebConf.getThingNameParameter()->valueBuffer) + "\",";
  json += "\"apPassword\":\"********\",";  // Don't expose actual password
  json += "\"wifiSsid\":\"" + String(iotWebConf.getWifiSsidParameter()->valueBuffer) + "\",";
  json += "\"wifiPassword\":\"\",";  // Don't expose actual password

  // Misc settings
  json += "\"startSound\":" + String(playSound ? "true" : "false") + ",";
  json += "\"speakerTick\":" + String(speakerTick ? "true" : "false") + ",";
  json += "\"ledTick\":" + String(ledTick ? "true" : "false") + ",";
  json += "\"showDisplay\":" + String(showDisplay ? "true" : "false") + ",";

  // Transmission settings
  json += "\"sendToCommunity\":" + String(sendToCommunity ? "true" : "false") + ",";
  json += "\"sendToMadavi\":" + String(sendToMadavi ? "true" : "false") + ",";
  json += "\"sendToBle\":" + String(sendToBle ? "true" : "false") + ",";

  // MQTT settings
  json += "\"sendToMqtt\":" + String(sendToMqtt ? "true" : "false") + ",";
  json += "\"mqttHost\":\"" + String(mqttHost) + "\",";
  json += "\"mqttPort\":" + String(mqttPort) + ",";
  json += "\"mqttUseTls\":" + String(mqttUseTls ? "true" : "false") + ",";
  json += "\"mqttRetain\":" + String(mqttRetain ? "true" : "false") + ",";
  json += "\"mqttUsername\":\"" + String(mqttUsername) + "\",";
  json += "\"mqttPassword\":\"\",";  // Don't expose actual password
  json += "\"mqttBaseTopic\":\"" + String(mqttBaseTopic) + "\",";

  // LoRa settings
  json += "\"hasLora\":" + String(isLoraBoard ? "true" : "false") + ",";
  if (isLoraBoard) {
    json += "\"sendToLora\":" + String(sendToLora ? "true" : "false") + ",";
    json += "\"deveui\":\"" + String(deveui) + "\",";
    json += "\"appeui\":\"" + String(appeui) + "\",";
    json += "\"appkey\":\"" + String(appkey) + "\",";
  }

  // Alarm settings
  json += "\"soundLocalAlarm\":" + String(soundLocalAlarm ? "true" : "false") + ",";
  json += "\"localAlarmThreshold\":" + String(localAlarmThreshold, 1) + ",";
  json += "\"localAlarmFactor\":" + String(localAlarmFactor);

  json += "}";

  server.send(200, "application/json", json);
}

void handlePostConfig(void) {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not received");
    return;
  }

  String body = server.arg("plain");
  log(INFO, "Received config update");

  // Parse JSON manually (simple approach - ESP32 can use ArduinoJson if needed)
  // For now, we'll use a simpler approach - just update the IotWebConf parameters
  // and call configSaved()

  // WiFi settings (only update if provided and not empty)
  int idx;
  idx = body.indexOf("\"thingName\":\"");
  if (idx >= 0) {
    int start = idx + 13;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      if (val.length() > 0) {
        strncpy(iotWebConf.getThingNameParameter()->valueBuffer, val.c_str(), IOTWEBCONF_WORD_LEN);
      }
    }
  }

  idx = body.indexOf("\"wifiSsid\":\"");
  if (idx >= 0) {
    int start = idx + 12;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      strncpy(iotWebConf.getWifiSsidParameter()->valueBuffer, val.c_str(), IOTWEBCONF_WORD_LEN);
    }
  }

  idx = body.indexOf("\"wifiPassword\":\"");
  if (idx >= 0) {
    int start = idx + 16;
    int end = body.indexOf("\"", start);
    if (end > start && end > start + 1) {  // Only update if password is provided
      String val = body.substring(start, end);
      if (val.length() > 0) {
        strncpy(iotWebConf.getWifiPasswordParameter()->valueBuffer, val.c_str(), IOTWEBCONF_PASSWORD_LEN);
      }
    }
  }

  // Boolean settings - simple parse
  playSound = body.indexOf("\"startSound\":true") >= 0;
  speakerTick = body.indexOf("\"speakerTick\":true") >= 0;
  ledTick = body.indexOf("\"ledTick\":true") >= 0;
  showDisplay = body.indexOf("\"showDisplay\":true") >= 0;
  sendToCommunity = body.indexOf("\"sendToCommunity\":true") >= 0;
  sendToMadavi = body.indexOf("\"sendToMadavi\":true") >= 0;
  sendToBle = body.indexOf("\"sendToBle\":true") >= 0;
  sendToMqtt = body.indexOf("\"sendToMqtt\":true") >= 0;
  mqttUseTls = body.indexOf("\"mqttUseTls\":true") >= 0;
  mqttRetain = body.indexOf("\"mqttRetain\":true") >= 0;
  soundLocalAlarm = body.indexOf("\"soundLocalAlarm\":true") >= 0;

  if (isLoraBoard) {
    sendToLora = body.indexOf("\"sendToLora\":true") >= 0;
  }

  // String settings
  idx = body.indexOf("\"mqttHost\":\"");
  if (idx >= 0) {
    int start = idx + 12;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      strncpy(mqttHost, val.c_str(), MQTT_HOST_LEN);
    }
  }

  idx = body.indexOf("\"mqttUsername\":\"");
  if (idx >= 0) {
    int start = idx + 16;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      strncpy(mqttUsername, val.c_str(), MQTT_USER_LEN);
    }
  }

  idx = body.indexOf("\"mqttPassword\":\"");
  if (idx >= 0) {
    int start = idx + 16;
    int end = body.indexOf("\"", start);
    if (end > start && end > start + 1) {
      String val = body.substring(start, end);
      if (val.length() > 0) {
        strncpy(mqttPassword, val.c_str(), MQTT_PASS_LEN);
      }
    }
  }

  idx = body.indexOf("\"mqttBaseTopic\":\"");
  if (idx >= 0) {
    int start = idx + 17;
    int end = body.indexOf("\"", start);
    if (end > start) {
      String val = body.substring(start, end);
      strncpy(mqttBaseTopic, val.c_str(), MQTT_BASE_TOPIC_LEN);
    }
  }

  // Numeric settings
  idx = body.indexOf("\"mqttPort\":");
  if (idx >= 0) {
    int start = idx + 11;
    int end = body.indexOf(",", start);
    if (end < 0) end = body.indexOf("}", start);
    if (end > start) {
      mqttPort = body.substring(start, end).toInt();
    }
  }

  idx = body.indexOf("\"localAlarmThreshold\":");
  if (idx >= 0) {
    int start = idx + 22;
    int end = body.indexOf(",", start);
    if (end < 0) end = body.indexOf("}", start);
    if (end > start) {
      localAlarmThreshold = body.substring(start, end).toFloat();
    }
  }

  idx = body.indexOf("\"localAlarmFactor\":");
  if (idx >= 0) {
    int start = idx + 19;
    int end = body.indexOf(",", start);
    if (end < 0) end = body.indexOf("}", start);
    if (end > start) {
      localAlarmFactor = body.substring(start, end).toInt();
    }
  }

  // Save configuration
  iotWebConf.saveConfig();
  configSaved();

  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Configuration saved\"}");

  // Restart device after short delay
  delay(500);
  ESP.restart();
}

void setup_webconf(bool loraHardware) {
  isLoraBoard = loraHardware;
  iotWebConf.setConfigSavedCallback(&configSaved);
  // *INDENT-OFF*   <- for 'astyle' to not format the following 3 lines
  iotWebConf.setupUpdateServer(
    [](const char *updatePath) { httpUpdater.setup(&server, updatePath); },
    [](const char *userName, char *password) { httpUpdater.updateCredentials(userName, password); });
  // *INDENT-ON*
  // override the confusing default labels of IotWebConf:
  iotWebConf.getThingNameParameter()->label = "Geiger accesspoint SSID";
  iotWebConf.getApPasswordParameter()->label = "Geiger accesspoint password";
  iotWebConf.getWifiSsidParameter()->label = "WiFi client SSID";
  iotWebConf.getWifiPasswordParameter()->label = "WiFi client password";

  // add the setting parameter
  grpMisc.addItem(&startSoundParam);
  grpMisc.addItem(&speakerTickParam);
  grpMisc.addItem(&ledTickParam);
  grpMisc.addItem(&showDisplayParam);
  iotWebConf.addParameterGroup(&grpMisc);
  grpTransmission.addItem(&sendToCommunityParam);
  grpTransmission.addItem(&sendToMadaviParam);
  grpTransmission.addItem(&sendToBleParam);
  iotWebConf.addParameterGroup(&grpTransmission);
  grpMqtt.addItem(&sendToMqttParam);
  grpMqtt.addItem(&mqttHostParam);
  grpMqtt.addItem(&mqttPortParam);
  grpMqtt.addItem(&mqttUseTlsParam);
  grpMqtt.addItem(&mqttRetainParam);
  grpMqtt.addItem(&mqttUserParam);
  grpMqtt.addItem(&mqttPassParam);
  grpMqtt.addItem(&mqttBaseTopicParam);
  iotWebConf.addParameterGroup(&grpMqtt);
  if (isLoraBoard) {
    grpLoRa.addItem(&sendToLoraParam);
    grpLoRa.addItem(&deveuiParam);
    grpLoRa.addItem(&appeuiParam);
    grpLoRa.addItem(&appkeyParam);
    iotWebConf.addParameterGroup(&grpLoRa);
  }
  grpAlarm.addItem(&soundLocalAlarmParam);
  grpAlarm.addItem(&localAlarmThresholdParam);
  grpAlarm.addItem(&localAlarmFactorParam);
  iotWebConf.addParameterGroup(&grpAlarm);

  // if we don't have LoRa hardware, do not send to LoRa
  if (!isLoraBoard)
    sendToLora = false;

  iotWebConf.init();
  loadConfigVariables();

  // Ensure AP password is set; otherwise library forces permanent AP mode.
  if (iotWebConf.getApPasswordParameter()->valueBuffer[0] == '\0') {
    strncpy(iotWebConf.getApPasswordParameter()->valueBuffer, wifiInitialApPassword, IOTWEBCONF_PASSWORD_LEN);
    iotWebConf.getApPasswordParameter()->valueBuffer[IOTWEBCONF_PASSWORD_LEN - 1] = '\0';
    iotWebConf.saveConfig();
  }

  // Always start AP for 30 seconds on boot (gives user time to configure WiFi)
  iotWebConf.setApTimeoutMs(30000);              // AP timeout: 30 seconds if no client connects
  iotWebConf.setWifiConnectionTimeoutMs(20000);  // STA connect timeout: 20 seconds

  if (hasConfiguredWifi()) {
    iotWebConf.forceApMode(false);  // allow leaving AP mode to connect to STA
  }

  wifiEventId = WiFi.onEvent(onWifiEvent);
  apConnectEventId = WiFi.onEvent(onApClientConnectEvent, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  apDisconnectEventId = WiFi.onEvent(onApClientDisconnectEvent, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  (void)wifiEventId;  // suppress unused warning for now
  (void)apConnectEventId;
  (void)apDisconnectEventId;

  auto redirectToCaptivePortal = []() {
    log(INFO, "Captive portal probe detected, redirecting to /config");
    server.sendHeader("Location", "/config");
    server.send(302, "text/plain", "");
  };

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/api/status", handleApiStatus);

  // Serve dashboard assets
  server.on("/style.css", []() {
    serveCompressed(server, style_css_gz, style_css_gz_len, "text/css");
  });
  server.on("/app.js", []() {
    serveCompressed(server, app_js_gz, app_js_gz_len, "application/javascript");
  });

  // Serve config page and assets
  server.on("/config.html", []() {
    serveCompressed(server, config_html_gz, config_html_gz_len, "text/html");
  });
  server.on("/config-style.css", []() {
    serveCompressed(server, config_style_css_gz, config_style_css_gz_len, "text/css");
  });
  server.on("/config.js", []() {
    serveCompressed(server, config_js_gz, config_js_gz_len, "application/javascript");
  });

  // Config API endpoints
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);

  // Captive portal probes (Android/Windows/Apple) - redirect to config page
  server.on("/generate_204", HTTP_ANY, redirectToCaptivePortal);      // Android
  server.on("/gen_204", HTTP_ANY, redirectToCaptivePortal);           // older Android variants
  server.on("/hotspot-detect.html", HTTP_ANY, redirectToCaptivePortal);  // Apple
  server.on("/ncsi.txt", HTTP_ANY, redirectToCaptivePortal);          // Windows
  server.on("/config", []() {
    serveCompressed(server, config_html_gz, config_html_gz_len, "text/html");
  });
  server.on("/firmware", [] { iotWebConf.handleConfig(); });  // Keep IotWebConf for firmware update
  server.onNotFound([]() {
    // Quietly redirect captive-portal probes (e.g. connectivitycheck.gstatic.com) to root
    if (iotWebConf.handleCaptivePortal()) {
      return;
    }
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
}
