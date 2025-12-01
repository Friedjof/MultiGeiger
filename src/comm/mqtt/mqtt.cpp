// MQTT publishing (proof-of-concept, manual config only).

#include "mqtt.hpp"

static const unsigned long RECONNECT_INTERVAL_MS = 5000;
static const size_t MQTT_BUFFER_SIZE = 512;

void MqttPublisher::begin(const char *deviceName) {
  if (!SEND2MQTT) {
    log(DEBUG, "MQTT: disabled (SEND2MQTT=false)");
    return;
  }

#if MQTT_USE_TLS
  netClient.setInsecure();  // PoC: skip certificate validation
  log(INFO, "MQTT: TLS enabled (insecure, no CA check)");
#endif
  client.setClient(netClient);
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setBufferSize(MQTT_BUFFER_SIZE);

  baseTopic = deviceName;
  if (!baseTopic.endsWith("/"))
    baseTopic += "/";

  initialized = true;
  log(INFO, "MQTT: init base topic %s broker=%s:%d", baseTopic.c_str(), MQTT_BROKER, MQTT_PORT);
}

void MqttPublisher::loop() {
  if (!SEND2MQTT || !initialized)
    return;

  if (!client.connected())
    ensureConnected();
  if (client.connected())
    client.loop();
}

void MqttPublisher::ensureConnected() {
  if (!SEND2MQTT || !initialized)
    return;

  if (WiFi.status() != WL_CONNECTED) {
    log(DEBUG, "MQTT: waiting for WiFi before connect");
    return;
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt < RECONNECT_INTERVAL_MS)
    return;
  lastReconnectAttempt = now;

  String clientId = "MultiGeiger-" + baseTopic;
  clientId.replace("/", "");  // keep it simple for broker
  bool useAuth = strlen(MQTT_USERNAME) > 0;
  bool connected;
  if (useAuth)
    connected = client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
  else
    connected = client.connect(clientId.c_str());

  if (connected) {
    log(INFO, "MQTT: connected to %s:%d as %s", MQTT_BROKER, MQTT_PORT, clientId.c_str());
  } else {
    log(WARNING, "MQTT: connect failed rc=%d", client.state());
  }
}

bool MqttPublisher::publish(const String &topicSuffix, const String &payload) {
  if (!SEND2MQTT || !initialized)
    return false;
  if (!client.connected()) {
    log(DEBUG, "MQTT: client not connected, retrying");
    ensureConnected();
    if (!client.connected()) {
      log(WARNING, "MQTT: publish skipped, still offline");
      return false;
    }
  }

  String topic = baseTopic + topicSuffix;
  bool ok = client.publish(topic.c_str(), payload.c_str(), MQTT_RETAIN);
  if (!ok) {
    log(WARNING, "MQTT: publish failed for %s state=%d", topic.c_str(), client.state());
  } else {
    log(DEBUG, "MQTT: publish %s -> %s", topic.c_str(), payload.c_str());
    lastPublishMs = millis();
  }
  return ok;
}

bool MqttPublisher::publishValue(const String &topicSuffix, const String &value) {
  return publish(topicSuffix, value);
}

void MqttPublisher::publishTimestamp(const String &topicSuffix) {
  publishValue(topicSuffix, String(utctime()));
}

void MqttPublisher::publishLive(float countRate, float doseRate, int counts, int dt_ms, int hv_pulses_delta,
                                int accumulated_counts, int accumulated_time_ms, float accumulated_rate, float accumulated_dose,
                                float temperature, float humidity, float pressure) {
  if (!SEND2MQTT || !initialized)
    return;

  if (!client.connected()) {
    log(INFO, "MQTT: skip live publish, not connected (will retry)");
  }

  // publish each value under live/<metric>
  publishValue("live/count_rate_cps", String(countRate, 3));
  publishValue("live/dose_rate_uSvph", String(doseRate, 3));
  publishValue("live/counts", String(counts));
  publishValue("live/dt_ms", String(dt_ms));
  publishValue("live/hv_pulses", String(hv_pulses_delta));
  publishValue("live/accum_counts", String(accumulated_counts));
  publishValue("live/accum_time_ms", String(accumulated_time_ms));
  publishValue("live/accum_rate_cps", String(accumulated_rate, 3));
  publishValue("live/accum_dose_uSvph", String(accumulated_dose, 3));
  publishValue("live/temperature", String(temperature, 2));
  publishValue("live/humidity", String(humidity, 2));
  publishValue("live/pressure", String(pressure, 2));
  publishTimestamp("live/timestamp");
}

void MqttPublisher::publishMeasurement(const String &tubeType, int tubeNbr, unsigned int dt_ms, unsigned int hv_pulses,
                                       unsigned int gm_counts, unsigned int cpm, bool have_thp, float temperature,
                                       float humidity, float pressure, int wifi_status) {
  if (!SEND2MQTT || !initialized)
    return;

  if (!client.connected()) {
    log(INFO, "MQTT: skip publish, not connected (will retry)");
  }

  log(INFO, "MQTT: publish measurement counts=%u cpm=%u hv=%u dt=%u thp=%s wifi_status=%d",
      gm_counts, cpm, hv_pulses, dt_ms, have_thp ? "yes" : "no", wifi_status);

  // simple value topics under live/*
  publishValue("live/counts", String(gm_counts));
  publishValue("live/cpm", String(cpm));
  publishValue("live/hv_pulses", String(hv_pulses));
  publishValue("live/dt_ms", String(dt_ms));
  publishValue("live/tube_type", tubeType);
  publishValue("live/tube_id", String(tubeNbr));
  publishTimestamp("live/timestamp");

  // thp (optional)
  if (have_thp) {
    publishValue("live/temperature", String(temperature, 2));
    publishValue("live/humidity", String(humidity, 2));
    publishValue("live/pressure", String(pressure, 2));
  } else {
    log(INFO, "MQTT: no THP available, skipping THP publish");
  }

  // status JSON
  char buf[MQTT_BUFFER_SIZE];
  snprintf(buf, sizeof(buf),
           "{\"wifi_status\":%d,\"mqtt_connected\":%s,\"last_publish_ms\":%lu,\"counts\":%u,\"cpm\":%u,\"hv_pulses\":%u,\"dt_ms\":%u,\"have_thp\":%s,\"timestamp\":\"%s\"}",
           wifi_status,
           client.connected() ? "true" : "false",
           lastPublishMs,
           gm_counts,
           cpm,
           hv_pulses,
           dt_ms,
           have_thp ? "true" : "false",
           utctime());
  publish("status", String(buf));
}
