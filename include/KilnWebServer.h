#pragma once

#include <WebServer.h>
#include "KilnController.h"
#include "WiFiProvisioningManager.h"

class KilnWebServer final {
public:
    KilnWebServer(KilnController& controller, WiFiProvisioningManager& wifi);
    void begin();
    void handleClient();

private:
    void sendStatus();
    void handleMode();
    void handleTarget();
    void handleCoils();
    void handleStop();
    void sendWiFiStatus();
    void sendWiFiScan();
    void handleWiFiScan();
    void handleWiFiConnect();
    void handleWiFiReset();

    KilnController& controller_;
    WiFiProvisioningManager& wifi_;
    WebServer server_{80};
};
