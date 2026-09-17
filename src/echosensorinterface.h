#ifndef SRC_ECHOSENSORINTERFACE_H
#define SRC_ECHOSENSORINTERFACE_H

#include <Arduino.h>

template<int pin_echo, int pin_trigger, int fetch_interval_milliseconds = 100>
class EchosensorInterface {

    unsigned long _last_read {0};
    std::optional<double> _curval;
    double _alpha { 0.7 };

public:
    EchosensorInterface() { static_assert(pin_echo >= 0 && pin_trigger >= 0 && pin_echo != pin_trigger); }

    void begin() {
        pinMode(pin_trigger, OUTPUT);
        pinMode(pin_echo, INPUT);

        delay(1);
        digitalWrite(pin_trigger, LOW);
    }

    /// Blocking read of the raw sensor value in meter
    double readCurrentValue() {
        digitalWrite(pin_trigger, LOW);
        delayMicroseconds(2);
        digitalWrite(pin_trigger, HIGH);
        delayMicroseconds(10);
        digitalWrite(pin_trigger, LOW);

        unsigned long duration = pulseIn(pin_echo, HIGH);
        double distance = (static_cast<double>(duration) * 0.000343) / 2.; // microseconds / sound of speed in m/sec

        return distance;
    }

    /// Get the averaged value from the most recent measurements nonblockingly.
    double getAccumulatedValue() {
        if (! _curval.has_value()) {
            _curval = readCurrentValue();
            _last_read = millis();
        }
        return _curval.value();
    }

    void update() {
        unsigned long now = millis();

        if (! _curval.has_value()) {
            _curval = readCurrentValue();
            _last_read = now;
            return;
        }

        if (now - _last_read < fetch_interval_milliseconds) return; // wait till time passed

        // floating average of measurement.
        double measured = readCurrentValue();
        _curval = measured * _alpha + (1. - _alpha) * _curval.value();
        _last_read = now;
    }
};

#endif //SRC_ECHOSENSORINTERFACE_H
