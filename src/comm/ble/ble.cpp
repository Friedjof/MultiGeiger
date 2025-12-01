// Code related to the transmission of the measured data via Bluetooth Low Energy,
// uses GATT Heart Rate Measurement Service for notifications with CPM update.
//
// Heart Rate Measurement = Radiation CPM, Energy Expense = Rolling Packet Counter
//
// Based on Neil Kolban's example file: https://github.com/nkolban/ESP32_BLE_Arduino
// Based on Andreas Spiess' example file: https://github.com/SensorsIot/Bluetooth-BLE-on-Arduino-IDE/blob/master/Polar_H7_Sensor/Polar_H7_Sensor.ino

#include "comm/ble/ble.hpp"

#define BLE_SERVICE_HEART_RATE    BLEUUID((uint16_t)0x180D)  // 16 bit UUID of Heart Rate Service
#define BLE_CHAR_HR_MEASUREMENT   BLEUUID((uint16_t)0x2A37)  // 16 bit UUID of Heart Rate Measurement Characteristic
#define BLE_CHAR_HR_POSITION      BLEUUID((uint16_t)0x2A38)  // 16 bit UUID of Heart Rate Sensor Position Characteristic
#define BLE_CHAR_HR_CONTROLPOINT  BLEUUID((uint16_t)0x2A39)  // 16 bit UUID of Heart Rate Control Point Characteristic
#define BLE_DESCR_UUID            BLEUUID((uint16_t)0x2901)  // 16 bit UUID of BLE Descriptor

class BleService::ServerCallbacks: public NimBLEServerCallbacks {
public:
  explicit ServerCallbacks(BleService *svc): svc_(svc) {}
  void onConnect(NimBLEServer *pServer) {
    log(INFO, "BLE device connected");
    svc_->device_connected = true;
  }
  void onDisconnect(NimBLEServer *pServer) {
    svc_->device_connected = false;
    log(INFO, "BLE device disconnected");
  }
private:
  BleService *svc_;
};

class BleService::CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
public:
  explicit CharacteristicCallbacks(BleService *svc): svc_(svc) {}
  void onWrite(NimBLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue().c_str();
    svc_->status_HRCP = (rxValue.length() > 0) ? (unsigned int)rxValue[0] : 0;
  }
private:
  BleService *svc_;
};

bool BleService::connected() const {
  return ble_enabled && device_connected;
}

void BleService::update(unsigned int cpm) {
  if (!ble_enabled)
    return;
  cpm_update_counter++;
  cpm_update_counter = cpm_update_counter & 0xFFFF;
  if (status_HRCP > 0) {
    log(DEBUG, "HR Control Point Value received: %d, resetting packet counter", status_HRCP);
    cpm_update_counter = 0;
    status_HRCP = 0;
  }
  const uint8_t flags_HRS = 0b00001001; // bit 0 --> 1 = HR (cpm) as UINT16, bit 3 --> add UINT16 Energy Expended (rolling update counter)
  txBuffer_HRM[0] = flags_HRS;
  txBuffer_HRM[1] = cpm & 0xFF;
  txBuffer_HRM[2] = (cpm >> 8) & 0xFF;
  txBuffer_HRM[3] = cpm_update_counter & 0xFF;
  txBuffer_HRM[4] = (cpm_update_counter >> 8) & 0xFF;
  if (bleServer && bleServer->getConnectedCount()) {
    NimBLEService *bleSvc = bleServer->getServiceByUUID(BLE_SERVICE_HEART_RATE);
    if (bleSvc) {
      NimBLECharacteristic *bleChr = bleSvc->getCharacteristic(BLE_CHAR_HR_MEASUREMENT);
      if (bleChr) {
        bleChr->setValue(txBuffer_HRM, 5);
        bleChr->notify();
      }
    }
  }
}

void BleService::begin(char *device_name, bool ble_on) {
  if (!ble_on) {
    set_status(STATUS_BLE, ST_BLE_OFF);
    ble_enabled = false;
    return;
  }
  ble_enabled = true;

  set_status(STATUS_BLE, ST_BLE_INIT);
  NimBLEDevice::init(device_name);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks(this));

  NimBLEService *bleService = bleServer->createService(BLE_SERVICE_HEART_RATE);

  NimBLECharacteristic *bleCharHRM = bleService->createCharacteristic(BLE_CHAR_HR_MEASUREMENT, NIMBLE_PROPERTY::NOTIFY);
  NimBLEDescriptor *bleDescriptorHRM = bleCharHRM->createDescriptor(BLE_DESCR_UUID, NIMBLE_PROPERTY::READ, 20);
  bleDescriptorHRM->setValue("Radiation rate CPM");

  NimBLECharacteristic *bleCharHRCP = bleService->createCharacteristic(BLE_CHAR_HR_CONTROLPOINT, NIMBLE_PROPERTY::WRITE);
  NimBLEDescriptor *bleDescriptorHRCP = bleCharHRCP->createDescriptor(BLE_DESCR_UUID, NIMBLE_PROPERTY::READ, 50);
  bleDescriptorHRCP->setValue("0x01 for Energy Exp. (packet counter) reset");
  NimBLECharacteristic *bleCharHRPOS = bleService->createCharacteristic(BLE_CHAR_HR_POSITION, NIMBLE_PROPERTY::READ);
  NimBLEDescriptor *bleDescriptorHRPOS = bleCharHRPOS->createDescriptor(BLE_DESCR_UUID, NIMBLE_PROPERTY::READ, 30);
  bleDescriptorHRPOS->setValue("Geiger Mueller Tube Type");
  bleCharHRCP->setCallbacks(new CharacteristicCallbacks(this));

  bleCharHRPOS->setValue(txBuffer_HRPOS, 1);

  bleServer->getAdvertising()->addServiceUUID(BLE_SERVICE_HEART_RATE);

  bleService->start();
  bleServer->getAdvertising()->start();

  set_status(STATUS_BLE, ST_BLE_CONNECTABLE);
  log(INFO, "BLE service advertising started, device name: %s, MAC: %s", device_name, BLEDevice::getAddress().toString().c_str());
}

void BleService::disable() {
  ble_enabled = false;
  set_status(STATUS_BLE, ST_BLE_OFF);
}
