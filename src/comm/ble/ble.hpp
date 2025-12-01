// Code related to the transmission of the measured data via Bluetooth Low Energy,
// uses GATT Heart Rate Measurement Service for notifications with CPM update.

#ifndef _BLE_H_
#define _BLE_H_

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

#endif // _BLE_H_
