#include <Arduino.h>
#include <WiFi.h>
#include "Ct1780ThermocoupleReader.h"
#include "KilnController.h"
#include "KilnWebServer.h"

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
KilnWebServer web(kiln);
}

void setup() {
    Serial.begin(115200);
    kiln.begin();

    WiFi.mode(WIFI_AP);
    WiFi.softAP("KilnManager");
    Serial.print("Kiln Manager AP: ");
    Serial.println(WiFi.softAPIP());

    web.begin();
}

void loop() {
    const uint32_t now = millis();
    kiln.update(now);
    web.handleClient();
    delay(10);
}
