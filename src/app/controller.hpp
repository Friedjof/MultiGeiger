#pragma once

#include <Arduino.h>

#include "config/config.hpp"
#include "core/core.hpp"
#include "drivers/clock/clock.hpp"
#include "drivers/io/io.hpp"
#include "drivers/sensors/sensors.hpp"
#include "drivers/display/display.hpp"
#include "comm/ble/ble.hpp"
#include "comm/wifi/wifi.hpp"
#include "comm/lora/loraWan.hpp"
#include "comm/mqtt/mqtt.hpp"

class MultiGeigerController {
public:
  void begin();
  void loopOnce();

private:
  void setupNtp(int wifiStatus);
  int updateWifiStatus();
  int updateBleStatus();
  void readThp(unsigned long current_ms);
  void publish(unsigned long current_ms, unsigned long current_counts, unsigned long gm_count_timestamp, unsigned long current_hv_pulses,
               float temperature, float humidity, float pressure);
  void oneMinuteLog(unsigned long current_ms, unsigned long current_counts);
  void statisticsLog(unsigned long current_counts, unsigned int time_between);
  void transmit(unsigned long current_ms, unsigned long current_counts, unsigned long gm_count_timestamp, unsigned long current_hv_pulses,
                bool have_thp, float temperature, float humidity, float pressure, int wifi_status);

  IoModule io;
  Sensors sensors;
  DisplayModule display;
  BleService ble;
  WifiManager wifi;
  MqttPublisher mqtt;
  ClockModule clock;

  bool isLoraBoard = false;
  bool hv_error = false;
  Switches switches_state{};
  bool have_thp = false;
  float temperature = 0.0f;
  float humidity = 0.0f;
  float pressure = 0.0f;
  unsigned long hv_pulses = 0;
  unsigned long gm_counts = 0;
  unsigned long gm_count_timestamp = 0;
  unsigned int gm_count_time_between = 0;
};
