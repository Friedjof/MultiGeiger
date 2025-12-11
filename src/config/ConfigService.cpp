/**
 * @file ConfigService.cpp
 * @brief Configuration management implementation using ESP32 Preferences (NVS)
 */

#include "ConfigService.hpp"
#include "config.hpp"
#include "core/core.hpp"

// Global instance
ConfigService configService;

const char* ConfigService::NAMESPACE = "multigeiger";

Config::Config() {
    setDefaults();
}

void Config::setDefaults() {
    log(INFO, "[CONFIG DEBUG] Setting default values...");

    // Build default device name from chip ID
    uint32_t chipId = (uint32_t)ESP.getEfuseMac();
    snprintf(deviceName, DEVICE_NAME_LEN, "MultiGeiger-%06X", chipId & 0xFFFFFF);

    // WiFi settings - defaults
    strncpy(apPassword, "ESP32Geiger", AP_PASS_LEN);
    wifiSsid[0] = '\0';  // Empty = not configured
    wifiPassword[0] = '\0';

    // Misc settings
    speakerTick = SPEAKER_TICK;
    ledTick = LED_TICK;
    showDisplay = SHOW_DISPLAY;

    // Transmission settings
    sendToBle = SEND2BLE;

    // LoRa settings
    sendToLora = SEND2LORA;
    devaddr[0] = '\0';
    nwkskey[0] = '\0';
    appskey[0] = '\0';

    // MQTT settings
    sendToMqtt = SEND2MQTT;
    strncpy(mqttHost, MQTT_BROKER, MQTT_HOST_LEN);
    mqttPort = MQTT_PORT;
    mqttUseTls = MQTT_USE_TLS;
    mqttRetain = MQTT_RETAIN;
    strncpy(mqttUsername, MQTT_USERNAME, MQTT_USER_LEN);
    strncpy(mqttPassword, MQTT_PASSWORD, MQTT_PASS_LEN);
    strncpy(mqttBaseTopic, MQTT_BASE_TOPIC, MQTT_TOPIC_LEN);

    // Alarm settings
    soundLocalAlarm = LOCAL_ALARM_SOUND;
    localAlarmThreshold = LOCAL_ALARM_THRESHOLD;
    localAlarmFactor = LOCAL_ALARM_FACTOR;

    // HTTP Auth settings
    log(INFO, "[CONFIG DEBUG] Setting HTTP Auth defaults from defines:");
    log(INFO, "[CONFIG DEBUG]   HTTP_AUTH_USER = '%s'", HTTP_AUTH_USER);
    log(INFO, "[CONFIG DEBUG]   HTTP_AUTH_PASS = '%s'", HTTP_AUTH_PASS);

    strncpy(httpAuthUser, HTTP_AUTH_USER, HTTP_AUTH_LEN);
    strncpy(httpAuthPass, HTTP_AUTH_PASS, HTTP_AUTH_LEN);

    log(INFO, "[CONFIG DEBUG] After strncpy:");
    log(INFO, "[CONFIG DEBUG]   httpAuthUser = '%s' (len=%d)", httpAuthUser, strlen(httpAuthUser));
    log(INFO, "[CONFIG DEBUG]   httpAuthPass = '%s' (len=%d)", httpAuthPass, strlen(httpAuthPass));
}

ConfigService::ConfigService() {
}

ConfigService::~ConfigService() {
    prefs.end();
}

bool ConfigService::begin() {
    // Open NVS namespace in read-write mode
    if (!prefs.begin(NAMESPACE, false)) {
        log(ERROR, "Failed to open NVS namespace");
        return false;
    }

    log(INFO, "ConfigService initialized");
    log(INFO, "[CONFIG DEBUG] Checking NVS state...");

    // Check if this is first boot (no config saved)
    bool hasDeviceName = prefs.isKey("deviceName");
    bool hasHttpAuthUser = prefs.isKey("httpAuthUser");
    bool hasHttpAuthPass = prefs.isKey("httpAuthPass");

    log(INFO, "[CONFIG DEBUG] NVS keys present:");
    log(INFO, "[CONFIG DEBUG]   deviceName: %s", hasDeviceName ? "YES" : "NO");
    log(INFO, "[CONFIG DEBUG]   httpAuthUser: %s", hasHttpAuthUser ? "YES" : "NO");
    log(INFO, "[CONFIG DEBUG]   httpAuthPass: %s", hasHttpAuthPass ? "YES" : "NO");

    if (!hasDeviceName) {
        log(INFO, "First boot detected, initializing with defaults");
        log(INFO, "[CONFIG DEBUG] Calling reset() to set defaults");
        reset();
        return true;
    }

    // Load configuration
    log(INFO, "[CONFIG DEBUG] Not first boot, loading existing config");
    if (!load()) {
        log(WARNING, "Failed to load config, using defaults");
        reset();
        return false;
    }

    return true;
}

bool ConfigService::load() {
    log(INFO, "[CONFIG DEBUG] Loading configuration from NVS...");

    // DEBUG: Check if auth keys exist before loading
    bool userKeyExists = prefs.isKey("httpAuthUser");
    bool passKeyExists = prefs.isKey("httpAuthPass");
    log(INFO, "[CONFIG DEBUG] NVS key 'httpAuthUser' exists: %s", userKeyExists ? "YES" : "NO");
    log(INFO, "[CONFIG DEBUG] NVS key 'httpAuthPass' exists: %s", passKeyExists ? "YES" : "NO");

    // DEBUG: Show buffer state before getString
    log(INFO, "[CONFIG DEBUG] Before load - httpAuthUser buffer: '%s' (first_byte=%d)",
        config.httpAuthUser, (int)config.httpAuthUser[0]);
    log(INFO, "[CONFIG DEBUG] Before load - httpAuthPass buffer: '%s' (first_byte=%d)",
        config.httpAuthPass, (int)config.httpAuthPass[0]);

    // WiFi settings
    prefs.getString("deviceName", config.deviceName, DEVICE_NAME_LEN);
    prefs.getString("apPassword", config.apPassword, AP_PASS_LEN);
    prefs.getString("wifiSsid", config.wifiSsid, WIFI_SSID_LEN);
    prefs.getString("wifiPassword", config.wifiPassword, WIFI_PASS_LEN);

    // Misc settings
    config.speakerTick = prefs.getBool("speakerTick", SPEAKER_TICK);
    config.ledTick = prefs.getBool("ledTick", LED_TICK);
    config.showDisplay = prefs.getBool("showDisplay", SHOW_DISPLAY);

    // Transmission settings
    config.sendToBle = prefs.getBool("sendToBle", SEND2BLE);

    // LoRa settings
    config.sendToLora = prefs.getBool("sendToLora", SEND2LORA);
    prefs.getString("devaddr", config.devaddr, LORA_KEY_LEN);
    prefs.getString("nwkskey", config.nwkskey, LORA_KEY_LEN);
    prefs.getString("appskey", config.appskey, LORA_KEY_LEN);

    // MQTT settings
    config.sendToMqtt = prefs.getBool("sendToMqtt", SEND2MQTT);
    prefs.getString("mqttHost", config.mqttHost, MQTT_HOST_LEN);
    config.mqttPort = prefs.getUShort("mqttPort", MQTT_PORT);
    config.mqttUseTls = prefs.getBool("mqttUseTls", MQTT_USE_TLS);
    config.mqttRetain = prefs.getBool("mqttRetain", MQTT_RETAIN);
    prefs.getString("mqttUsername", config.mqttUsername, MQTT_USER_LEN);
    prefs.getString("mqttPassword", config.mqttPassword, MQTT_PASS_LEN);
    prefs.getString("mqttBaseTopic", config.mqttBaseTopic, MQTT_TOPIC_LEN);

    // Alarm settings
    config.soundLocalAlarm = prefs.getBool("soundLocalAlarm", LOCAL_ALARM_SOUND);
    config.localAlarmThreshold = prefs.getFloat("alarmThreshold", LOCAL_ALARM_THRESHOLD);
    config.localAlarmFactor = prefs.getInt("alarmFactor", LOCAL_ALARM_FACTOR);

    // HTTP Auth settings
    size_t userLen = prefs.getString("httpAuthUser", config.httpAuthUser, HTTP_AUTH_LEN);
    size_t passLen = prefs.getString("httpAuthPass", config.httpAuthPass, HTTP_AUTH_LEN);

    // DEBUG: Show what was loaded
    log(INFO, "[CONFIG DEBUG] After load - httpAuthUser: '%s' (len=%d, bytes_read=%d, first_byte=%d)",
        config.httpAuthUser, strlen(config.httpAuthUser), userLen, (int)config.httpAuthUser[0]);
    log(INFO, "[CONFIG DEBUG] After load - httpAuthPass: '%s' (len=%d, bytes_read=%d, first_byte=%d)",
        config.httpAuthPass, strlen(config.httpAuthPass), passLen, (int)config.httpAuthPass[0]);

    log(INFO, "Configuration loaded from NVS");
    return true;
}

bool ConfigService::save() {
    log(INFO, "[CONFIG DEBUG] Saving configuration to NVS...");
    log(INFO, "[CONFIG DEBUG] Will save httpAuthUser: '%s' (len=%d)",
        config.httpAuthUser, strlen(config.httpAuthUser));
    log(INFO, "[CONFIG DEBUG] Will save httpAuthPass: '%s' (len=%d)",
        config.httpAuthPass, strlen(config.httpAuthPass));

    // WiFi settings
    prefs.putString("deviceName", config.deviceName);
    prefs.putString("apPassword", config.apPassword);
    prefs.putString("wifiSsid", config.wifiSsid);
    prefs.putString("wifiPassword", config.wifiPassword);

    // Misc settings
    prefs.putBool("speakerTick", config.speakerTick);
    prefs.putBool("ledTick", config.ledTick);
    prefs.putBool("showDisplay", config.showDisplay);

    // Transmission settings
    prefs.putBool("sendToBle", config.sendToBle);

    // LoRa settings
    prefs.putBool("sendToLora", config.sendToLora);
    prefs.putString("devaddr", config.devaddr);
    prefs.putString("nwkskey", config.nwkskey);
    prefs.putString("appskey", config.appskey);

    // MQTT settings
    prefs.putBool("sendToMqtt", config.sendToMqtt);
    prefs.putString("mqttHost", config.mqttHost);
    prefs.putUShort("mqttPort", config.mqttPort);
    prefs.putBool("mqttUseTls", config.mqttUseTls);
    prefs.putBool("mqttRetain", config.mqttRetain);
    prefs.putString("mqttUsername", config.mqttUsername);
    prefs.putString("mqttPassword", config.mqttPassword);
    prefs.putString("mqttBaseTopic", config.mqttBaseTopic);

    // Alarm settings
    prefs.putBool("soundLocalAlarm", config.soundLocalAlarm);
    prefs.putFloat("alarmThreshold", config.localAlarmThreshold);
    prefs.putInt("alarmFactor", config.localAlarmFactor);

    // HTTP Auth settings
    size_t userBytes = prefs.putString("httpAuthUser", config.httpAuthUser);
    size_t passBytes = prefs.putString("httpAuthPass", config.httpAuthPass);

    log(INFO, "[CONFIG DEBUG] Saved httpAuthUser: %d bytes", userBytes);
    log(INFO, "[CONFIG DEBUG] Saved httpAuthPass: %d bytes", passBytes);

    log(INFO, "Configuration saved to NVS");
    return true;
}

void ConfigService::reset() {
    log(INFO, "Resetting configuration to defaults");
    config.setDefaults();
    save();
}

bool ConfigService::hasWifiConfig() const {
    return config.wifiSsid[0] != '\0';
}
