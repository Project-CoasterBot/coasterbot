#include <Arduino.h>

#include "boardconfig.h"

struct SensorPin {
    const char* name;
    int gpio;
};

constexpr SensorPin sensor_pins[] = {
    {"EDGE_FRONT_LEFT", board::PIN_EDGE_FRONT_LEFT},
    {"EDGE_FRONT_RIGHT", board::PIN_EDGE_FRONT_RIGHT},
    {"EDGE_REAR_LEFT", board::PIN_EDGE_REAR_LEFT},
    {"EDGE_REAR_RIGHT", board::PIN_EDGE_REAR_RIGHT},
};

void setup() {
    Serial.begin(115200);

    for (const SensorPin& sensor : sensor_pins) {
        pinMode(sensor.gpio, INPUT);
    }

    Serial.println("CoasterBot - Pin-Test der Kantensensoren");
    Serial.println("GPIO-Zustaende werden alle 500 ms angezeigt:");
}

void loop() {
    for (const SensorPin& sensor : sensor_pins) {
        const int state = digitalRead(sensor.gpio);
        Serial.print(sensor.name);
        Serial.print(" (GP");
        Serial.print(sensor.gpio);
        Serial.print("): ");
        Serial.println(state == HIGH ? "HIGH" : "LOW");
    }

    Serial.println();
    delay(500);
}