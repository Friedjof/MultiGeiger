/**
 * @file ConfigService.hpp
 * @brief Configuration management using ESP32 Preferences (NVS)
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Config value lengths
#define WIFI_SSID_LEN 33
#define WIFI_PASS_LEN 64
#define DEVICE_NAME_LEN 33
#define AP_PASS_LEN 64
#define MQTT_HOST_LEN 64
#define MQTT_USER_LEN 32
#define MQTT_PASS_LEN 32
#define MQTT_TOPIC_LEN 64
#define LORA_KEY_LEN 33
#define HTTP_AUTH_LEN 32

/**
 * @brief Configuration data class
 */
class Config {
public:
    // WiFi settings
    char deviceName[DEVICE_NAME_LEN];
    char apPassword[AP_PASS_LEN];
    char wifiSsid[WIFI_SSID_LEN];
    char wifiPassword[WIFI_PASS_LEN];

    // Misc settings
    bool speakerTick;
    bool ledTick;
    bool showDisplay;

    // Transmission settings
    bool sendToBle;

    // LoRa settings
    bool sendToLora;
    char devaddr[LORA_KEY_LEN];
    char nwkskey[LORA_KEY_LEN];
    char appskey[LORA_KEY_LEN];

    // MQTT settings
    bool sendToMqtt;
    char mqttHost[MQTT_HOST_LEN];
    uint16_t mqttPort;
    bool mqttUseTls;
    bool mqttRetain;
    char mqttUsername[MQTT_USER_LEN];
    char mqttPassword[MQTT_PASS_LEN];
    char mqttBaseTopic[MQTT_TOPIC_LEN];

    // Alarm settings
    bool soundLocalAlarm;
    float localAlarmThreshold;
    int localAlarmFactor;

    // HTTP Auth settings
    char httpAuthUser[HTTP_AUTH_LEN];
    char httpAuthPass[HTTP_AUTH_LEN];

    Config();
    void setDefaults();
};

/**
 * @brief Configuration service for persistent storage
 */
class ConfigService {
public:
    ConfigService();
    ~ConfigService();

    /**
     * @brief Initialize config service and load configuration
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Load configuration from NVS
     * @return true if successful
     */
    bool load();

    /**
     * @brief Save configuration to NVS
     * @return true if successful
     */
    bool save();

    /**
     * @brief Reset configuration to defaults
     */
    void reset();

    /**
     * @brief Check if WiFi credentials are configured
     * @return true if WiFi SSID is set
     */
    bool hasWifiConfig() const;

    /**
     * @brief Get configuration object
     * @return Reference to config
     */
    Config& getConfig() { return config; }

private:
    Preferences prefs;
    Config config;
    static const char* NAMESPACE;
};

// Global config service instance
extern ConfigService configService;
