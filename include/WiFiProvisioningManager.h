#pragma once

#include <Arduino.h>
#include <IPAddress.h>

class WiFiProvisioningManager final {
public:
    enum class State {
        SetupAccessPoint,
        Scanning,
        Connecting,
        Connected,
        RecoveryAccessPoint
    };

    void begin();
    void update();
    bool startScan();
    bool scanInProgress() const;
    String scanResultsJson() const;
    bool connect(const String& ssid, const String& password);
    void clearCredentials();

    State state() const;
    const char* stateName() const;
    bool isSetupMode() const;
    String ipAddress() const;
    String setupSsid() const;
    String setupPassword() const;

private:
    static constexpr uint32_t kConnectionTimeoutMs = 15000;
    static constexpr uint32_t kReconnectIntervalMs = 30000;

    void startAccessPoint(State state);
    void startStationConnection();
    bool validCredentials(const String& ssid, const String& password) const;
    String chipSuffix() const;

    State state_ = State::SetupAccessPoint;
    uint32_t stateChangedAtMs_ = 0;
    uint32_t reconnectAtMs_ = 0;
    String setupSsid_;
    String setupPassword_;
    String stationSsid_;
    String stationPassword_;
    int scanResultCount_ = -2;
    bool setupApActive_ = false;
    uint32_t setupApDisableAtMs_ = 0;
};