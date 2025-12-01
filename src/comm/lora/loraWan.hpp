#ifndef _LORAWAN_H_
#define _LORAWAN_H_

#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include "drivers/hal/heltecv2.hpp"
#include "core/core.hpp"

extern char appeui[];
extern char deveui[];
extern char appkey[];

typedef enum {
  TX_STATUS_UNKNOWN = 0,
  TX_STATUS_JOINING,
  TX_STATUS_JOINED,
  TX_STATUS_ENDING_ERROR,
  TX_STATUS_TIMEOUT,
  TX_STATUS_UPLINK_SUCCESS,
  TX_STATUS_UPLINK_ACKED,
  TX_STATUS_UPLINK_ACKED_WITHDOWNLINK,
  TX_STATUS_UPLINK_ACKED_WITHDOWNLINK_PENDING
} transmissionStatus_t;

#define LORA_TIMEOUT_MS 30000L

void setup_lorawan();

// call os_runloop_once(); a separate function to keep the LMIC header files mostly hidden.
void poll_lorawan();
transmissionStatus_t lorawan_send(uint8_t txPort, uint8_t *txBuffer, uint8_t txSz, bool ack, uint8_t *rxPort, uint8_t *rxBuffer, uint8_t *rxSz);

// Thin OO wrapper for LoRaWAN handling.
class LoraWanModule {
public:
  void begin() { setup_lorawan(); }
  void poll() { poll_lorawan(); }
  transmissionStatus_t send(uint8_t txPort, uint8_t *txBuffer, uint8_t txSz, bool ack, uint8_t *rxPort, uint8_t *rxBuffer, uint8_t *rxSz) {
    return lorawan_send(txPort, txBuffer, txSz, ack, rxPort, rxBuffer, rxSz);
  }
};

#endif // _LORAWAN_H_
