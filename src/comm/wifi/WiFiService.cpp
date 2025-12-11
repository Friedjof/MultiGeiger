/**
 * @file WiFiService.cpp
 * @brief WiFi connection and AP management implementation
 */

#include "WiFiService.hpp"
#include "core/core.hpp"

WiFiService* WiFiService::instance = nullptr;

WiFiService::WiFiService()
    : currentState(WIFI_STATE_BOOT),
      apModeForced(false),
      apClientConnected(false),
      apStartTime(0),
      connectionStartTime(0),
      apTimeoutMs(30000),  // Default 30 seconds
      connectionTimeoutMs(20000)  // Default 20 seconds
{
    instance = this;
    hostname[0] = '\0';
    apPassword[0] = '\0';
    lastSsid[0] = '\0';
    lastPassword[0] = '\0';
}

WiFiService::~WiFiService() {
    stopAP();
    WiFi.disconnect(true);
    instance = nullptr;
}

void WiFiService::begin(const char* deviceName, const char* apPass) {
    strncpy(hostname, deviceName, sizeof(hostname) - 1);
    hostname[sizeof(hostname) - 1] = '\0';

    strncpy(apPassword, apPass, sizeof(apPassword) - 1);
    apPassword[sizeof(apPassword) - 1] = '\0';

    WiFi.mode(WIFI_MODE_NULL);
    WiFi.setHostname(hostname);

    // Register WiFi event handlers
    WiFi.onEvent(onWiFiEvent);

    log(INFO, "WiFiService initialized (hostname: %s)", hostname);

    // Start in AP mode initially
    startAP();
}

void WiFiService::loop() {
    // Process DNS requests if in AP mode
    if (currentState == WIFI_STATE_AP_MODE) {
        dnsServer.processNextRequest();
    }

    updateState();
}

bool WiFiService::connectToWiFi(const char* ssid, const char* password) {
    if (ssid == nullptr || ssid[0] == '\0') {
        log(WARNING, "Cannot connect: SSID is empty");
        return false;
    }

    log(INFO, "Connecting to WiFi: %s", ssid);

    // Store credentials
    strncpy(lastSsid, ssid, sizeof(lastSsid) - 1);
    lastSsid[sizeof(lastSsid) - 1] = '\0';
    strncpy(lastPassword, password, sizeof(lastPassword) - 1);
    lastPassword[sizeof(lastPassword) - 1] = '\0';

    // Stop AP mode if running
    if (currentState == WIFI_STATE_AP_MODE) {
        stopAP();
    }

    // Set mode and connect
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    currentState = WIFI_STATE_CONNECTING;
    connectionStartTime = millis();

    return true;
}

bool WiFiService::startAP() {
    log(INFO, "Starting Access Point: %s", hostname);

    // Stop any existing connection
    WiFi.disconnect(true);
    delay(100);

    // Configure and start AP
    WiFi.mode(WIFI_AP);
    bool success = WiFi.softAP(hostname, apPassword);

    if (!success) {
        log(ERROR, "Failed to start Access Point");
        return false;
    }

    // Start DNS server for captive portal
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    currentState = WIFI_STATE_AP_MODE;
    apStartTime = millis();
    apClientConnected = false;

    log(INFO, "AP started - IP: %s", WiFi.softAPIP().toString().c_str());

    return true;
}

void WiFiService::stopAP() {
    if (currentState == WIFI_STATE_AP_MODE) {
        log(INFO, "Stopping Access Point");
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
    }
}

void WiFiService::forceApMode(bool force) {
    apModeForced = force;

    if (force && currentState != WIFI_STATE_AP_MODE) {
        startAP();
    }
}

void WiFiService::updateState() {
    switch (currentState) {
        case WIFI_STATE_BOOT:
            // Should not stay here - begin() moves to AP_MODE
            break;

        case WIFI_STATE_AP_MODE:
            handleApMode();
            break;

        case WIFI_STATE_CONNECTING:
            handleConnecting();
            break;

        case WIFI_STATE_ONLINE:
            handleOnline();
            break;

        case WIFI_STATE_OFF:
            // Nothing to do
            break;
    }
}

void WiFiService::handleApMode() {
    // Check if we have WiFi credentials and should try to connect
    if (!apModeForced && lastSsid[0] != '\0') {
        // Check AP timeout only if no client is connected
        if (!apClientConnected && apTimeoutMs > 0) {
            if (millis() - apStartTime > apTimeoutMs) {
                log(INFO, "AP timeout - switching to STA mode");
                connectToWiFi(lastSsid, lastPassword);
                return;
            }
        }
    }

    // Check if client connected/disconnected
    bool nowConnected = (WiFi.softAPgetStationNum() > 0);
    if (nowConnected != apClientConnected) {
        apClientConnected = nowConnected;
        if (nowConnected) {
            log(INFO, "Client connected to AP");
            // Disable timeout when client connects
            apTimeoutMs = 0;
        } else {
            log(INFO, "Client disconnected from AP");
            // If we have WiFi config, switch to STA mode
            if (!apModeForced && lastSsid[0] != '\0') {
                connectToWiFi(lastSsid, lastPassword);
            }
        }
    }
}

void WiFiService::handleConnecting() {
    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED) {
        log(INFO, "WiFi connected - IP: %s", WiFi.localIP().toString().c_str());
        currentState = WIFI_STATE_ONLINE;
        return;
    }

    // Check connection timeout
    if (millis() - connectionStartTime > connectionTimeoutMs) {
        log(WARNING, "WiFi connection timeout");
        WiFi.disconnect();

        // Fall back to AP mode
        if (!apModeForced) {
            log(INFO, "Falling back to AP mode");
            startAP();
        }
    }
}

void WiFiService::handleOnline() {
    // Check if still connected
    if (WiFi.status() != WL_CONNECTED) {
        log(WARNING, "WiFi disconnected");
        currentState = WIFI_STATE_CONNECTING;
        connectionStartTime = millis();

        // Try to reconnect
        if (lastSsid[0] != '\0') {
            WiFi.begin(lastSsid, lastPassword);
        }
    }
}

void WiFiService::onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (!instance) return;

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            log(INFO, "WiFi STA connected");
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            log(INFO, "WiFi STA disconnected (reason: %d)", info.wifi_sta_disconnected.reason);
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            log(INFO, "WiFi got IP: %s", WiFi.localIP().toString().c_str());
            break;

        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            log(INFO, "Client connected to AP");
            break;

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            log(INFO, "Client disconnected from AP");
            break;

        default:
            break;
    }
}
