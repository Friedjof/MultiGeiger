/**
 * @file WiFiService.hpp
 * @brief WiFi connection and AP management
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

/**
 * @brief WiFi connection states
 */
enum WiFiState {
    WIFI_STATE_BOOT,          // Initial boot state
    WIFI_STATE_AP_MODE,       // Access Point mode
    WIFI_STATE_CONNECTING,    // Attempting to connect to WiFi
    WIFI_STATE_ONLINE,        // Connected to WiFi
    WIFI_STATE_OFF            // WiFi disabled
};

/**
 * @brief WiFi service managing connection and AP mode
 */
class WiFiService {
public:
    WiFiService();
    ~WiFiService();

    /**
     * @brief Initialize WiFi service
     * @param deviceName Device name for AP and hostname
     * @param apPassword AP password
     */
    void begin(const char* deviceName, const char* apPassword);

    /**
     * @brief Process WiFi state machine and DNS requests
     * Must be called regularly from loop()
     */
    void loop();

    /**
     * @brief Connect to WiFi network
     * @param ssid WiFi SSID
     * @param password WiFi password
     * @return true if connection initiated
     */
    bool connectToWiFi(const char* ssid, const char* password);

    /**
     * @brief Store WiFi credentials without connecting immediately
     * @param ssid WiFi SSID
     * @param password WiFi password
     */
    void setWiFiCredentials(const char* ssid, const char* password);

    /**
     * @brief Start Access Point mode
     * @return true if AP started successfully
     */
    bool startAP();

    /**
     * @brief Stop Access Point mode
     */
    void stopAP();

    /**
     * @brief Force AP mode (disable auto-switching to client)
     * @param force true to force AP mode
     */
    void forceApMode(bool force);

    /**
     * @brief Get current WiFi state
     * @return Current WiFi state
     */
    WiFiState getState() const { return currentState; }

    /**
     * @brief Check if WiFi is connected
     * @return true if connected to WiFi network
     */
    bool isConnected() const { return currentState == WIFI_STATE_ONLINE; }

    /**
     * @brief Check if in AP mode
     * @return true if in AP mode
     */
    bool isApMode() const { return currentState == WIFI_STATE_AP_MODE; }

    /**
     * @brief Get device hostname
     * @return Hostname
     */
    const char* getHostname() const { return hostname; }

    /**
     * @brief Set AP timeout (how long to stay in AP mode if no client connects)
     * @param timeoutMs Timeout in milliseconds (0 = no timeout)
     */
    void setApTimeout(unsigned long timeoutMs) { apTimeoutMs = timeoutMs; }

    /**
     * @brief Set WiFi connection timeout
     * @param timeoutMs Timeout in milliseconds
     */
    void setConnectionTimeout(unsigned long timeoutMs) { connectionTimeoutMs = timeoutMs; }

private:
    WiFiState currentState;
    DNSServer dnsServer;
    char hostname[33];
    char apPassword[64];
    char lastSsid[33];
    char lastPassword[64];

    bool apModeForced;
    bool apClientConnected;
    unsigned long apStartTime;
    unsigned long connectionStartTime;
    unsigned long apTimeoutMs;
    unsigned long connectionTimeoutMs;

    static const byte DNS_PORT = 53;

    void updateState();
    void handleApMode();
    void handleConnecting();
    void handleOnline();

    static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
    static WiFiService* instance;  // For static event handler
};
