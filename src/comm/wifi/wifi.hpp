// WiFi configuration (captive portal/OTA) and HTTP/LoRa transmissions.

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>

#include "core/core.hpp"
#include "drivers/display/display.hpp"
#include "drivers/io/io.hpp"
#include "comm/lora/loraWan.hpp"
#include "config/config.hpp"
#include "config/ConfigService.hpp"
#include "comm/wifi/WiFiService.hpp"

extern char ssid[];
extern WiFiService wifiService;

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
                   int have_thp, float temperature, float humidity, float pressure, float gas_resistance, int sensor_type, int wifi_status);

// The Arduino LMIC wants to be polled from loop(). This takes care of that on LoRa boards.
void poll_transmission(void);

// Thin OO wrapper for WiFi/web configuration and transmissions.
class WifiManager {
public:
  void beginWeb(bool loraHardware) { setup_webconf(loraHardware); }
  void beginTx(const char *version, char *chipSsid, bool loraHardware) { setup_transmission(version, chipSsid, loraHardware); }
  void pollTx() { poll_transmission(); }
  void pollWeb();  // Declaration - implemented in wifi.cpp
  WiFiState getState() { return wifiService.getState(); }
  void send(const String &tube_type, int tube_nbr, unsigned int dt, unsigned int hv_pulses, unsigned int gm_counts, unsigned int cpm,
            int have_thp, float temperature, float humidity, float pressure, float gas_resistance, int sensor_type, int wifi_status) {
    transmit_data(tube_type, tube_nbr, dt, hv_pulses, gm_counts, cpm, have_thp, temperature, humidity, pressure, gas_resistance, sensor_type, wifi_status);
  }
};
