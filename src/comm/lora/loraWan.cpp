#include "loraWan.hpp"

// Compile this only if we have a LoRa capable hardware

// loraWan hardware related code
// code based on free-to-use / do-anything-what-you-want-with-it example code
// by Thomas Telkamp, Matthijs Kooijman and Terry Moore, MCCI.

// IOs for optional LoRa
//
// SX1276 (pin) => ESP32 (pin)
// ===========================
// SCK = GPIO5
// MISO = GPIO19
// MOSI = GPIO27
// CS = GPIO18
// RESET = GPIO14
// DIO0 (8) = GPIO26 (15)
// DIO1 (9) = GPIO33 (13)
// DIO2 (10) = GPIO32 (12)

// Send a valid LoRaWAN packet using frequency and encryption settings matching
// those of the The Things Network.
//
// This uses ABP (Activation By Personalization), where DevAddr and session keys
// (NwkSKey, AppSKey) are pre-configured and set directly without over-the-air join.
// ABP is required for single-channel gateways like Dragino LG01-N which cannot
// handle OTAA downlinks reliably.
//
// Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
// g1, 0.1% in g2), but not the TTN fair usage policy.
//
// To use this code, register your device with TTN in ABP mode and configure
// the DevAddr, NwkSKey, and AppSKey in the web interface.
//
// Do not forget to define the radio type correctly in
// arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.

// ABP Mode - OTAA functions still required by LMIC library but not used
// These are stubs that should never be called since we use ABP (no join)
void os_getArtEui(u1_t *buf) {
  // ABP mode - this function should never be called
  memset(buf, 0, 8);
}

void os_getDevEui(u1_t *buf) {
  // ABP mode - this function should never be called
  memset(buf, 0, 8);
}

void os_getDevKey(u1_t *buf) {
  // ABP mode - this function should never be called
  memset(buf, 0, 16);
}

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = LORA_CS,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = LORA_RST,
  .dio = {LORA_IRQ, LORA_IO1, LORA_IO2 },
};

static volatile transmissionStatus_t txStatus;
static uint8_t *__rxPort;
static uint8_t *__rxBuffer;
static uint8_t *__rxSz;

void onEvent(ev_t ev) {
  switch (ev) {
  case EV_SCAN_TIMEOUT:
    txStatus = TX_STATUS_ENDING_ERROR;
    log(INFO, "LoRa: Scan timeout - no gateway found");
    log(DEBUG, "EV_SCAN_TIMEOUT");
    break;
  case EV_BEACON_FOUND:
    txStatus = TX_STATUS_UNKNOWN;
    log(DEBUG, "EV_BEACON_FOUND");
    break;
  case EV_BEACON_MISSED:
    txStatus = TX_STATUS_UNKNOWN;
    log(DEBUG, "EV_BEACON_MISSED");
    break;
  case EV_BEACON_TRACKED:
    txStatus = TX_STATUS_UNKNOWN;
    log(DEBUG, "EV_BEACON_TRACKED");
    break;
  case EV_JOINING:
    txStatus = TX_STATUS_JOINING;
    log(INFO, "LoRa: Joining TTN network (OTAA)...");
    log(DEBUG, "EV_JOINING");
    break;
  case EV_JOINED:
    txStatus = TX_STATUS_JOINED;
    log(INFO, "LoRa: Successfully joined TTN!");
    log(DEBUG, "EV_JOINED");
    {
      u4_t netid = 0;
      devaddr_t devaddr = 0;
      u1_t nwkKey[16];
      u1_t artKey[16];
      LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
      String ak, nk;
      for (int i = 0; i < sizeof(artKey); ++i) {
        ak += String(artKey[i], 16);
      }
      for (int i = 0; i < sizeof(nwkKey); ++i) {
        nk += String(nwkKey[i], 16);
      }
      log(DEBUG, "netid: %d devaddr: %x artKey: %s nwkKey: %s", netid, devaddr, ak.c_str(), nk.c_str());
    }
    // Disable link check validation (automatically enabled
    // during join, but because slow data rates change max TX
    // size, we don't use it in this example.
    LMIC_setLinkCheckMode(0);
    break;
  // This event is defined but not used in the code.
  // No point in wasting codespace on it.
  // case EV_RFU1:
  //   log(DEBUG, "EV_RFU1");
  //   break;
  case EV_JOIN_FAILED:
    txStatus = TX_STATUS_ENDING_ERROR;
    log(INFO, "LoRa: Join FAILED - check credentials (DEVEUI, APPEUI, APPKEY)");
    log(DEBUG, "EV_JOIN_FAILED");
    break;
  case EV_REJOIN_FAILED:
    txStatus = TX_STATUS_ENDING_ERROR;
    log(INFO, "LoRa: Rejoin FAILED");
    log(DEBUG, "EV_REJOIN_FAILED");
    break;
  case EV_TXCOMPLETE:
    log(INFO, "LoRa: Transmission complete");
    log(DEBUG, "EV_TXCOMPLETE (includes waiting for RX windows)");
    txStatus =   TX_STATUS_UPLINK_SUCCESS;
    if (LMIC.txrxFlags & TXRX_ACK) {
      txStatus = TX_STATUS_UPLINK_ACKED;
      log(DEBUG, "Received ack");
    }
    if (LMIC.dataLen) {
      log(DEBUG, "Received %d bytes of payload", LMIC.dataLen);
      if (__rxPort != NULL) *__rxPort = LMIC.frame[LMIC.dataBeg - 1];
      if (__rxSz != NULL) *__rxSz = LMIC.dataLen;
      if (__rxBuffer != NULL) memcpy(__rxBuffer, &LMIC.frame[LMIC.dataBeg], LMIC.dataLen);
      txStatus = TX_STATUS_UPLINK_ACKED_WITHDOWNLINK;
    }
    break;
  case EV_LOST_TSYNC:
    txStatus = TX_STATUS_ENDING_ERROR;
    log(DEBUG, "EV_LOST_TSYNC");
    break;
  case EV_RESET:
    txStatus = TX_STATUS_ENDING_ERROR;
    log(DEBUG, "EV_RESET");
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    txStatus = TX_STATUS_UNKNOWN;
    log(DEBUG, "EV_RXCOMPLETE");
    break;
  case EV_LINK_DEAD:
    txStatus = TX_STATUS_ENDING_ERROR;
    log(DEBUG, "EV_LINK_DEAD");
    break;
  case EV_LINK_ALIVE:
    txStatus = TX_STATUS_UNKNOWN;
    log(DEBUG, "EV_LINK_ALIVE");
    break;
  // This event is defined but not used in the code.
  // No point in wasting codespace on it.
  // case EV_SCAN_FOUND:
  //   log(DEBUG, "EV_SCAN_FOUND");
  //   break;
  case EV_TXSTART:
    txStatus = TX_STATUS_UNKNOWN;
    log(INFO, "LoRa: Starting transmission...");
    log(DEBUG, "EV_TXSTART");
    break;
  default:
    txStatus = TX_STATUS_UNKNOWN;
    log(INFO, "LoRa: Unknown event: %u", (unsigned int) ev);
    log(DEBUG, "Unknown event: %u", (unsigned int) ev);
    break;
  }
}


void setup_lorawan() {
  log(INFO, "LoRa: Initializing LMIC stack (ABP mode)...");
  txStatus = TX_STATUS_UNKNOWN;

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // Parse ABP credentials from hex strings
  uint32_t devAddr;
  uint8_t nwkSKey[16];
  uint8_t appSKey[16];

  // Convert DevAddr from hex string (e.g., "26011D01") to uint32_t
  devAddr = strtoul(devaddr, NULL, 16);

  // Convert NwkSKey and AppSKey from hex strings to byte arrays
  hex2data(nwkSKey, (const char *) nwkskey, 16);
  hex2data(appSKey, (const char *) appskey, 16);

  log(INFO, "LoRa: Setting ABP session (DevAddr: 0x%08X)", devAddr);

  // Set ABP session keys (netid=0 for TTN)
  LMIC_setSession(0x1, devAddr, nwkSKey, appSKey);

  // Configure for single-channel gateway (868.1 MHz, SF7)
  // Disable all channels except channel 0 (868.1 MHz)
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // Keep CH0
  for (uint8_t i = 1; i < 9; i++) {
    LMIC_disableChannel(i);  // Disable CH1-CH8
  }

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // Set data rate to SF7 (fastest for single-channel)
  LMIC_setDrTxpow(DR_SF7, 14);

  // Disable ADR (Adaptive Data Rate) for single-channel gateway
  LMIC_setAdrMode(0);

  log(INFO, "LoRa: ABP initialized (Single-Channel: 868.1 MHz, SF7)");
  txStatus = TX_STATUS_JOINED;  // ABP is always "joined"
}

void poll_lorawan() {
  os_runloop_once();
}

// Send LoRaWan frame with ack or not
// - txPort : port to transmit
// - txBuffer : message to transmit
// - txSz : size of the message to transmit
// - ack : true for message ack & downlink / false for pure uplink
//   When Ack is false, the downlink buffer can be set to NULL as rxSz and rPort
// - rxPort : where to write the port where downlink has been received
// - rxBuffer : where the downlinked data will be stored
// - rxSz : size of received data
transmissionStatus_t lorawan_send(uint8_t txPort, uint8_t *txBuffer, uint8_t txSz, bool ack, uint8_t *rxPort, uint8_t *rxBuffer, uint8_t *rxSz) {
  log(INFO, "LoRa: lorawan_send() called - port %d, %d bytes", txPort, txSz);

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & (OP_POLL | OP_TXDATA | OP_TXRXPEND)) {
    log(INFO, "LoRa: LMIC busy (opmode=0x%02x), not sending", LMIC.opmode);
    log(DEBUG, "OP_POLL | OP_TXDATA | OP_TXRXPEND, not sending");
    return TX_STATUS_ENDING_ERROR;
  } else {
    log(INFO, "LoRa: Queuing data for transmission...");
    txStatus = TX_STATUS_UNKNOWN;
    __rxPort = rxPort;
    __rxBuffer = rxBuffer;
    __rxSz = rxSz;
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(txPort, txBuffer, txSz, ((ack) ? 1 : 0));
    log(INFO, "LoRa: Waiting for transmission to complete (timeout: %d ms)...", LORA_TIMEOUT_MS);
    // wait for completion
    uint64_t start = millis();
    while (true) {
      switch (txStatus) {
      case TX_STATUS_UNKNOWN:
      case TX_STATUS_JOINING:
      case TX_STATUS_JOINED:
        os_runloop_once();
        break;
      case TX_STATUS_UPLINK_SUCCESS:
      case TX_STATUS_UPLINK_ACKED:
      case TX_STATUS_UPLINK_ACKED_WITHDOWNLINK:
      case TX_STATUS_UPLINK_ACKED_WITHDOWNLINK_PENDING:
        return txStatus;
      case TX_STATUS_ENDING_ERROR:
      case TX_STATUS_TIMEOUT:
        break;
      }
      if (millis() - start > LORA_TIMEOUT_MS) {
        log(INFO, "LoRa: TIMEOUT after %d ms - reinitializing LMIC", LORA_TIMEOUT_MS);
        setup_lorawan();
        return TX_STATUS_TIMEOUT;
      }
    }
  }
}
