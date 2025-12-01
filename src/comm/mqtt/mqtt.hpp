// MQTT publishing (proof-of-concept).
// Config is compile-time only for now (see config.default.hpp / config.hpp).

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#if MQTT_USE_TLS
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif
#include <PubSubClient.h>

#include "core/core.hpp"
#include "config/config.hpp"

class MqttPublisher {
public:
  void begin(const char *deviceName);
  void loop();
  void publishMeasurement(const String &tubeType, int tubeNbr, unsigned int dt_ms, unsigned int hv_pulses,
                          unsigned int gm_counts, unsigned int cpm, bool have_thp, float temperature,
                          float humidity, float pressure, int wifi_status);
  void publishLive(float countRate, float doseRate, int counts, int dt_ms, int hv_pulses_delta,
                   int accumulated_counts, int accumulated_time_ms, float accumulated_rate, float accumulated_dose,
                   float temperature, float humidity, float pressure);

private:
  void ensureConnected();
  bool publish(const String &topicSuffix, const String &payload);
  bool publishValue(const String &topicSuffix, const String &value);
  void publishTimestamp(const String &topicSuffix);

#if MQTT_USE_TLS
  WiFiClientSecure netClient;
#else
  WiFiClient netClient;
#endif
  PubSubClient client{netClient};
  String baseTopic;
  unsigned long lastReconnectAttempt = 0;
  bool initialized = false;
  unsigned long lastPublishMs = 0;
};
