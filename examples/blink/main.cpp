#include <Arduino.h>

namespace {
constexpr uint8_t kBuiltInLed = 2;
}

void setup() {
    pinMode(kBuiltInLed, OUTPUT);
    Serial.begin(115200);
    Serial.println("KilnManager FireBeetle blink test");
}

void loop() {
    digitalWrite(kBuiltInLed, HIGH);
    Serial.println("LED ON");
    delay(500);
    digitalWrite(kBuiltInLed, LOW);
    Serial.println("LED OFF");
    delay(500);
}
