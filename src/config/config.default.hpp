// Device configuration defaults.
// Copy this file to config.hpp (ignored by git) and adjust to your hardware.

#pragma once

// TUBE_TYPE values (predefined at sensor.community, DO NOT CHANGE):
#define TUBE_UNKNOWN 0 // this can be used for experimenting with other GM tubes and has a 0 CPM to uSv/h conversion factor.
#define SBM20 1
#define SBM19 2
#define Si22G 3

// your Geiger-Mueller counter tube:
#define TUBE_TYPE Si22G

// DEFAULT_LOG_LEVEL values (DO NOT CHANGE)
#include "core/core.hpp"

// your log level:
#define DEFAULT_LOG_LEVEL INFO

// SERIAL_DEBUG values (DO NOT CHANGE)
// (values declared in core.hpp)
// your serial logging style:
#define SERIAL_DEBUG Serial_Logging

// Server transmission debugging:
// if set to true, print debug info on serial (USB) interface while sending to servers (madavi or sensor.community)
#define DEBUG_SERVER_SEND true

// Speaker Ticks with every pulse?
#define SPEAKER_TICK true

// White LED on uC board flashing with every pulse?
#define LED_TICK true

// Enable display?
#define SHOW_DISPLAY true

// Play a start sound at boot/reboot time?
#define PLAY_SOUND true

// Send to servers:
// Send data to Madavi server?
// Madavi should be used to see values in real time.
#define SEND2MADAVI true

// Send data to sensor.community server?
// Should always be true so that the data is archived there. Standard server for devices without LoRa.
#define SEND2SENSORCOMMUNITY true

// Send data via MQTT?
// Proof-of-concept: configure broker/credentials below and payload will be published every measurement interval.
#define SEND2MQTT false

// MQTT broker configuration (manual for now, later via WebUI)
#define MQTT_BROKER "192.168.1.10"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
// TLS is currently optional and uses a shared CA cert string in code when enabled.
#define MQTT_USE_TLS false
// QoS 0 or 1 (PubSubClient limitation); retain flag for published messages.
#define MQTT_QOS 0
#define MQTT_RETAIN false

// Send data via LoRa to TTN?
// Only for devices with LoRa, automatically deactivated for devices without LoRa.
// If this is set to true, sending to Madavi and sensor.community should be deactivated!
// Note: The TTN configuration needs to be done in lorawan.cpp (starting at line 65).
#define SEND2LORA false

// Send data via BLE?
// Device provides "Heart Rate Service" (0x180D) and these characteristics.
// 0x2A37: Heart Rate Measurement
// --> sends current CPM as shown on display + rolling packet counter as energy expenditure (roll-over @0xFF).
// 0x2A38: Heart Rate Sensor Position --> sends TUBE_TYPE
// 0x2A39: Heart Rate Control Point --> allows to reset "energy expenditure", as required by service definition
#define SEND2BLE false

// Play an alarm sound when radiation level is too high?
// Activates when either accumulated dose rate reaches the set threshold (see below)
// or when the current dose rate is higher than the accumulated dose rate by the set factor (see below).
// ! Requires a valid tube type to be set in order to calculate dose rate.
#define LOCAL_ALARM_SOUND false

// Accumulated dose rate threshold to trigger the local alarm
// The accumulated dose rate is the overall average since the last start of MultiGeiger.
// Default value: 0.500 µSv/h
// ! Requires a valid tube type to be set in order to calculate dose rate.
#define LOCAL_ALARM_THRESHOLD 0.500  // µSv/h

// Factor of current dose rate vs. accumulated dose rate to trigger the local alarm
// Default value: 3
// Example: accumulated dose rate from days of operation is at 0.1 µSv/h.
// If current dose rate rises to (default) 3 times the accumulated dose rate, i.e. 0.3 µSv/h, trigger the local alarm.
// ! Requires a valid tube type to be set in order to calculate dose rate.
#define LOCAL_ALARM_FACTOR 3  // current / accumulated dose rate

// LoRa timeout
#define LORA_TIMEOUT_MS 30000L

// WiFi/HTTP endpoints and behavior
#define MADAVI_URL "http://api-rrd.madavi.de/data.php"
#define SENSORCOMMUNITY_URL "http://api.sensor.community/v1/push-sensor-data/"
#define CUSTOMSRV_URL "https://ptsv2.com/t/xxxxx-yyyyyyyyyy/post"
#define SEND2CUSTOMSRV false
#define CONFIG_VERSION "015"

// Web config checkboxes have 'selected' if checked, so we need 9 byte for this string.
#define CHECKBOX_LEN 9

// NTP / time settings
#define NTP_SRV_1 "pool.ntp.org"
#define NTP_SRV_2 "time.nist.gov"
#define TZ_OFFSET 0  // timezone offset from UTC [s]
#define DST_OFFSET 0  // DST offset from NONDST [s]

// Sensor pins / timing
#define PIN_TEST_OUTPUT -1
#define PIN_HV_FET_OUTPUT 23
#define PIN_HV_CAP_FULL_INPUT 22
#define PIN_GMC_COUNT_INPUT 2
#define GMC_DEAD_TIME 190
#define MAX_CHARGE_PULSES 3333
#define TUBE_PERIOD_DURATION_US 100

// IO pins
#define HWTESTPIN 26
#define PIN_SPEAKER_OUTPUT_P 12
#define PIN_SPEAKER_OUTPUT_N 0

// Speaker timing
#define SPEAKER_PERIOD_DURATION_US 1000

// Display pins
#define PIN_DISPLAY_ON 21
#define PIN_OLED_RST 16
#define PIN_OLED_SCL 15
#define PIN_OLED_SDA 4

// Measurement/display timing
#define MEASUREMENT_INTERVAL_SEC 150
#define AFTERSTART_MS 5000
#define DISPLAY_REFRESH_MS 10000
#define MINCOUNTS_DISPLAY 100
#define LOOP_DURATION_MS 1000
