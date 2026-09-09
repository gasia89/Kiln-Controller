#include <Arduino.h>
#include <WiFi.h>
#include "Ct1780ThermocoupleReader.h"
#include "KilnController.h"
#include "KilnWebServer.h"
#include "PreferencesHeatingProfileRepository.h"
#include "WiFiProvisioningManager.h"

namespace {
constexpr uint8_t kThermocoupleDataPin = 4;
constexpr uint8_t kCoilOnePin = 25;
constexpr uint8_t kCoilTwoPin = 26;
constexpr size_t kThermocoupleCount = 1;
constexpr uint32_t kSsrWindowMs = 2000;

Ct1780ThermocoupleReader thermocouples(kThermocoupleDataPin, kThermocoupleCount);
TimeProportionalSsr coilOne(kCoilOnePin, kSsrWindowMs);
TimeProportionalSsr coilTwo(kCoilTwoPin, kSsrWindowMs);
KilnController kiln(thermocouples, coilOne, coilTwo);
WiFiProvisioningManager wifi;
PreferencesHeatingProfileRepository profiles;
KilnWebServer web(kiln, wifi, profiles);
}

void setup() {
    Serial.begin(115200);
    kiln.begin();

    wifi.begin();
    Serial.print("Kiln Manager network state: ");
    Serial.println(wifi.stateName());
    Serial.print("Kiln Manager IP: ");
    Serial.println(wifi.ipAddress());
    if (wifi.isSetupMode()) {
        Serial.print("Kiln Manager setup SSID: ");
        Serial.println(wifi.setupSsid());
        Serial.print("Kiln Manager setup password: ");
        Serial.println(wifi.setupPassword());
    }

    web.begin();
}

void loop() {
    const uint32_t now = millis();
    wifi.update();
    kiln.update(now);
    web.handleClient();
    delay(10);
}
