#ifndef SRC_INFRAREDSENSORINTERRFACE_H
#define SRC_INFRAREDSENSORINTERRFACE_H

#include <Arduino.h>

template<int pin>
class InfraredSensorControl {
    public:

    InfraredSensorControl() { static_assert(pin >= 0); }

    void begin() {
        pinMode(pin, INPUT);
    }

    bool obstacleDetected() {
        int val = digitalRead(pin);
        return (val == LOW);
    }
};

#endif //SRC_INFRAREDSENSORINTERRFACE_H
