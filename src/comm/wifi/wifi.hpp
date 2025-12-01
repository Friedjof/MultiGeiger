// WiFi configuration (captive portal/OTA) and HTTP/LoRa transmissions.

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include <IotWebConfESP32HTTPUpdateServer.h>

#include "core/core.hpp"
#include "drivers/display/display.hpp"
#include "drivers/io/io.hpp"
#include "comm/lora/loraWan.hpp"
#include "config/config.hpp"

extern bool speakerTick;
extern bool playSound;
extern bool ledTick;
extern bool showDisplay;
extern bool sendToCommunity;
extern bool sendToMadavi;
extern bool sendToLora;
extern bool sendToBle;
extern bool soundLocalAlarm;
extern bool sendToMqtt;
extern bool mqttUseTls;
extern bool mqttRetain;
extern uint16_t mqttPort;
extern int mqttQos;
extern char mqttHost[];
extern char mqttUsername[];
extern char mqttPassword[];
extern char mqttBaseTopic[];

extern char appeui[];
extern char deveui[];
extern char appkey[];

extern float localAlarmThreshold;
extern int localAlarmFactor;

extern char ssid[];
extern IotWebConf iotWebConf;

void setup_webconf(bool loraHardware);

// Sensor-PINS.
// They are called PIN, because in the first days of Feinstaub sensor they were
// really the CPU-Pins. Now they are 'virtual' pins to distinguish different sensors.
// Since we send to sensor.community, we have to use their numbers.
// PIN number 0 doesn't exist, so we use it to disable the X-PIN header.
#define XPIN_NO_XPIN 0
#define XPIN_RADIATION 19
#define XPIN_BME280 11

void setup_transmission(const char *version, char *ssid, bool lora);
void transmit_data(String tube_type, int tube_nbr, unsigned int dt, unsigned int hv_pulses, unsigned int gm_counts, unsigned int cpm,
                   int have_thp, float temperature, float humidity, float pressure, int wifi_status);

// The Arduino LMIC wants to be polled from loop(). This takes care of that on LoRa boards.
void poll_transmission(void);

// Thin OO wrapper for WiFi/web configuration and transmissions.
class WifiManager {
public:
  void beginWeb(bool loraHardware) { setup_webconf(loraHardware); }
  void beginTx(const char *version, char *chipSsid, bool loraHardware) { setup_transmission(version, chipSsid, loraHardware); }
  void pollTx() { poll_transmission(); }
  void pollWeb() { iotWebConf.doLoop(); }
  void send(const String &tube_type, int tube_nbr, unsigned int dt, unsigned int hv_pulses, unsigned int gm_counts, unsigned int cpm,
            int have_thp, float temperature, float humidity, float pressure, int wifi_status) {
    transmit_data(tube_type, tube_nbr, dt, hv_pulses, gm_counts, cpm, have_thp, temperature, humidity, pressure, wifi_status);
  }
};
