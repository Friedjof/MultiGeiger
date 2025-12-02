/**
 * @file mqtt.hpp
 * @brief MQTT client for publishing measurement data
 *
 * Provides MQTT connectivity with TLS support for publishing
 * radiation measurements to an MQTT broker.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "core/core.hpp"
#include "config/config.hpp"

/**
 * @struct MqttConfig
 * @brief Configuration structure for MQTT client
 */
struct MqttConfig {
  bool enabled;        ///< Enable/disable MQTT publishing
  String host;         ///< MQTT broker hostname
  uint16_t port;       ///< MQTT broker port
  bool useTls;         ///< Use TLS/SSL encryption
  String username;     ///< MQTT authentication username
  String password;     ///< MQTT authentication password
  bool retain;         ///< Set retain flag on published messages
  int qos;             ///< Quality of Service level (0, 1, or 2)
  String baseTopic;    ///< Base topic prefix for all publications
};

class MqttPublisher {
public:
  void begin(const MqttConfig &cfg, const char *deviceName);
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

  void configureClient();

  MqttConfig config{};
  String deviceBaseTopic;
  WiFiClient plainClient;
  WiFiClientSecure tlsClient;
  Client *activeClient = nullptr;
  PubSubClient client;
  String baseTopic;
  unsigned long lastReconnectAttempt = 0;
  bool initialized = false;
  unsigned long lastPublishMs = 0;
};
