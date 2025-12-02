/**
 * @file ble.hpp
 * @brief Bluetooth Low Energy (BLE) interface for CPM broadcasting
 *
 * Transmits radiation measurements via BLE using the GATT Heart Rate
 * Measurement Service. The heart rate value represents Counts Per Minute (CPM).
 * This allows standard heart rate monitor apps to display radiation levels.
 */

#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "config/config.hpp"
#include "drivers/display/display.hpp"

class BleService {
public:
  void begin(char *deviceName, bool enabled);
  void update(unsigned int cpm);
  bool connected() const;
  void disable();

private:
  class ServerCallbacks;
  class CharacteristicCallbacks;

  NimBLEServer *bleServer = nullptr;
  bool ble_enabled = false;
  bool device_connected = false;
  unsigned int status_HRCP = 0;
  unsigned int cpm_update_counter = 0;
  uint8_t txBuffer_HRM[5];
  uint8_t txBuffer_HRPOS[1] = {TUBE_TYPE};
};
