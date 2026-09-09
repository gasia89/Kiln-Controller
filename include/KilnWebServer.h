#pragma once

#include <WebServer.h>
#include "KilnController.h"
#include "IHeatingProfileRepository.h"
#include "WiFiProvisioningManager.h"

class KilnWebServer final {
public:
    KilnWebServer(KilnController& controller, WiFiProvisioningManager& wifi, IHeatingProfileRepository& profiles);
    void begin();
    void handleClient();

private:
    void sendStatus();
    void sendProfiles();
    void sendProfile();
    void sendProfileExport();
    void handleProfileSave();
    void handleProfileImport();
    void handleProfileDelete();
    void handleProfileStart();
    void handleProfilePause();
    void handleProfileResume();
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
    IHeatingProfileRepository& profiles_;
    WebServer server_{80};
};
