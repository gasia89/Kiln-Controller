#pragma once

#include <WebServer.h>
#include "KilnController.h"

class KilnWebServer final {
public:
    explicit KilnWebServer(KilnController& controller);
    void begin();
    void handleClient();

private:
    void sendStatus();
    void handleMode();
    void handleTarget();
    void handleCoils();
    void handleStop();

    KilnController& controller_;
    WebServer server_{80};
};
