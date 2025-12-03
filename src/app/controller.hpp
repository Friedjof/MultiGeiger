/**
 * @file controller.hpp
 * @brief Main controller for the MultiGeiger device
 *
 * This file contains the MultiGeigerController class which orchestrates
 * all hardware modules (sensors, display, communication) and manages the
 * main measurement loop.
 */

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

/**
 * @class MultiGeigerController
 * @brief Main controller coordinating all device subsystems
 *
 * Manages initialization, measurement cycles, sensor reading, data transmission,
 * and user interface (display, speaker) for the MultiGeiger radiation detector.
 */
class MultiGeigerController {
public:
  /** @brief Initialize all subsystems (sensors, display, communication) */
  void begin();

  /** @brief Execute one iteration of the main measurement loop */
  void loopOnce();

  /** @brief Update LED and speaker tick settings
   *  @param ledTick Enable/disable LED tick on radiation detection
   *  @param speakerTick Enable/disable speaker tick on radiation detection
   */
  void applyTickSettings(bool ledTick, bool speakerTick);

  /** @brief Update display on/off setting
   *  @param showDisplay Enable/disable OLED display
   */
  void applyDisplaySetting(bool showDisplay);

  /** @brief Get current radiation counts */
  unsigned long getCounts() const { return gm_counts; }

  /** @brief Get current temperature (°C) */
  float getTemperature() const { return temperature; }

  /** @brief Get current humidity (%) */
  float getHumidity() const { return humidity; }

  /** @brief Get current pressure (hPa) */
  float getPressure() const { return pressure; }

  /** @brief Check if environmental sensors are available */
  bool hasThp() const { return have_thp; }

  /** @brief Check for HV error */
  bool hasHvError() const { return hv_error; }

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
