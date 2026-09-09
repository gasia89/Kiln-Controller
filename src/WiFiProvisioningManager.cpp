#include "WiFiProvisioningManager.h"

#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>

namespace {
constexpr char kPreferenceNamespace[] = "wifi";
constexpr char kHostName[] = "kilnmanager";
constexpr char kSsidKey[] = "ssid";
constexpr char kPasswordKey[] = "password";
}

void WiFiProvisioningManager::begin() {
    const String suffix = chipSuffix();
    setupSsid_ = "KilnManager-" + suffix;
    setupPassword_ = "KilnSetup-" + suffix;

    Preferences preferences;
    preferences.begin(kPreferenceNamespace, true);
    stationSsid_ = preferences.getString(kSsidKey, "");
    stationPassword_ = preferences.getString(kPasswordKey, "");
    preferences.end();

    if (stationSsid_.isEmpty()) {
        startAccessPoint(State::SetupAccessPoint);
        return;
    }
    startStationConnection();
}

void WiFiProvisioningManager::update() {
    const uint32_t now = millis();

    if (state_ == State::Scanning) {
        const int result = WiFi.scanComplete();
        if (result >= 0 || result == WIFI_SCAN_FAILED) {
            scanResultCount_ = result;
            state_ = State::SetupAccessPoint;
            stateChangedAtMs_ = now;
        }
        return;
    }

    if (state_ == State::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            MDNS.begin(kHostName);
            setupApDisableAtMs_ = setupApActive_ ? now + 5000 : 0;
            state_ = State::Connected;
            stateChangedAtMs_ = now;
            Serial.print("Kiln Manager connected. Station IP: ");
            Serial.println(WiFi.localIP());
            return;
        }
        if (now - stateChangedAtMs_ >= kConnectionTimeoutMs) {
            startAccessPoint(State::RecoveryAccessPoint);
            Serial.print("Kiln Manager connection failed. Recovery SSID: ");
            Serial.println(setupSsid_);
            Serial.print("Kiln Manager recovery password: ");
            Serial.println(setupPassword_);
            Serial.print("Kiln Manager recovery IP: ");
            Serial.println(WiFi.softAPIP());
        }
        return;
    }

    if (state_ == State::Connected && setupApActive_ && static_cast<int32_t>(now - setupApDisableAtMs_) >= 0) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        setupApActive_ = false;
        setupApDisableAtMs_ = 0;
    }

    if (state_ == State::Connected && WiFi.status() != WL_CONNECTED) {
        if (setupApActive_) {
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
            setupApActive_ = false;
            setupApDisableAtMs_ = 0;
        }
        if (reconnectAtMs_ == 0) {
            reconnectAtMs_ = now + kReconnectIntervalMs;
        } else if (static_cast<int32_t>(now - reconnectAtMs_) >= 0) {
            reconnectAtMs_ = 0;
            startStationConnection();
        }
    }
}

bool WiFiProvisioningManager::startScan() {
    if (!isSetupMode() || state_ == State::Scanning) {
        return false;
    }
    WiFi.scanDelete();
    scanResultCount_ = -2;
    if (WiFi.scanNetworks(true, true) == WIFI_SCAN_FAILED) {
        return false;
    }
    state_ = State::Scanning;
    stateChangedAtMs_ = millis();
    return true;
}

bool WiFiProvisioningManager::scanInProgress() const {
    return state_ == State::Scanning;
}

String WiFiProvisioningManager::scanResultsJson() const {
    if (scanResultCount_ < 0) {
        return "[]";
    }

    String json = "[";
    for (int index = 0; index < scanResultCount_; ++index) {
        if (index > 0) {
            json += ",";
        }
        String ssid = WiFi.SSID(index);
        ssid.replace("\\", "\\\\");
        ssid.replace("\"", "\\\"");
        json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(index)) + ",\"open\":" + String(WiFi.encryptionType(index) == WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    json += "]";
    return json;
}

bool WiFiProvisioningManager::connect(const String& ssid, const String& password) {
    if (!isSetupMode() || !validCredentials(ssid, password)) {
        return false;
    }

    Preferences preferences;
    preferences.begin(kPreferenceNamespace, false);
    preferences.putString(kSsidKey, ssid);
    preferences.putString(kPasswordKey, password);
    preferences.end();
    stationSsid_ = ssid;
    stationPassword_ = password;
    startStationConnection();
    return true;
}

void WiFiProvisioningManager::clearCredentials() {
    Preferences preferences;
    preferences.begin(kPreferenceNamespace, false);
    preferences.clear();
    preferences.end();
    stationSsid_.clear();
    stationPassword_.clear();
    startAccessPoint(State::SetupAccessPoint);
}

WiFiProvisioningManager::State WiFiProvisioningManager::state() const { return state_; }

const char* WiFiProvisioningManager::stateName() const {
    switch (state_) {
    case State::SetupAccessPoint: return "setup_ap";
    case State::Scanning: return "scanning";
    case State::Connecting: return "connecting";
    case State::Connected: return "connected";
    case State::RecoveryAccessPoint: return "recovery_ap";
    }
    return "unknown";
}

bool WiFiProvisioningManager::isSetupMode() const {
    return state_ == State::SetupAccessPoint || state_ == State::RecoveryAccessPoint || state_ == State::Scanning || (state_ == State::Connecting && setupApActive_);
}

String WiFiProvisioningManager::ipAddress() const {
    return isSetupMode() ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

String WiFiProvisioningManager::setupSsid() const { return setupSsid_; }
String WiFiProvisioningManager::setupPassword() const { return setupPassword_; }

void WiFiProvisioningManager::startAccessPoint(State state) {
    MDNS.end();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(setupSsid_.c_str(), setupPassword_.c_str());
    setupApActive_ = true;
    state_ = state;
    stateChangedAtMs_ = millis();
    reconnectAtMs_ = 0;
    setupApDisableAtMs_ = 0;
}

void WiFiProvisioningManager::startStationConnection() {
    const bool keepSetupAp = setupApActive_;
    if (keepSetupAp) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(setupSsid_.c_str(), setupPassword_.c_str());
    } else {
        WiFi.mode(WIFI_STA);
    }
    WiFi.setHostname("kilnmanager");
    WiFi.begin(stationSsid_.c_str(), stationPassword_.c_str());
    state_ = State::Connecting;
    stateChangedAtMs_ = millis();
    reconnectAtMs_ = 0;
}

bool WiFiProvisioningManager::validCredentials(const String& ssid, const String& password) const {
    return !ssid.isEmpty() && ssid.length() <= 32 && (password.isEmpty() || (password.length() >= 8 && password.length() <= 63));
}

String WiFiProvisioningManager::chipSuffix() const {
    const uint64_t chipId = ESP.getEfuseMac();
    char suffix[7] = {};
    snprintf(suffix, sizeof(suffix), "%06llX", chipId & 0xFFFFFFULL);
    return String(suffix);
}