#ifndef SRC_ECHOSENSORINTERFACE_H
#define SRC_ECHOSENSORINTERFACE_H

#include <Arduino.h>

#include <optional>

/** Ultrasonic distance sensor HC-SR04, nonblocking.
 *
 * update() fires the trigger every fetch_interval_milliseconds. The echo pulse is timed by an
 * interrupt on both edges of the echo pin, so the main loop never waits for it. The next call of
 * update() after the falling edge evaluates the pulse.
 *
 * Without an echo the HC-SR04 holds the echo pin high for ~38 ms (clones longer), that would read as
 * ~6.5 m. Every distance is therefore clamped to max_range_millimeter before it enters the floating
 * average, so "nothing in range" is max range and not a random far value mixed into real ones.
 *
 * If the sensor stops answering (no complete pulse for several triggers in a row) the value is
 * dropped and getAccumulatedValue() returns -1 again, like before the first measurement.
 *
 * The interrupt state is static per template instance (the ISR has no context pointer), so only
 * one instance per echo pin must exist.
 */
template<int pin_echo, int pin_trigger, int fetch_interval_milliseconds = 100, int max_range_millimeter = 2000>
class EchosensorInterface {

    static inline volatile unsigned long _rise_us {0};
    static inline volatile unsigned long _pulse_us {0};
    static inline volatile bool _pulse_ready {false};

    static void onEcho() {
        const unsigned long now = micros();
        if (digitalRead(pin_echo) == HIGH) {
            _rise_us = now;
        } else {
            _pulse_us = now - _rise_us;
            _pulse_ready = true;
        }
    }

    static constexpr float _max_range { static_cast<float>(max_range_millimeter) * 0.001f }; // in m
    static constexpr unsigned _max_missed {3}; // triggers without complete pulse until the value is dropped

    unsigned long _last_trigger {0};
    bool _triggered {false};     // a trigger was sent and its pulse is still outstanding
    unsigned _missed {0};
    std::optional<float> _curval;
    float _alpha { 0.7 };

public:
    EchosensorInterface() {
        static_assert(pin_echo >= 0 && pin_trigger >= 0 && pin_echo != pin_trigger);
        // the echo of the previous trigger (max ~38 ms) has to be over before the next one
        static_assert(fetch_interval_milliseconds >= 50);
        static_assert(max_range_millimeter > 0);
    }

    void begin() {
        pinMode(pin_trigger, OUTPUT);
        digitalWrite(pin_trigger, LOW);
        pinMode(pin_echo, INPUT);

        _pulse_ready = false;
        attachInterrupt(digitalPinToInterrupt(pin_echo), onEcho, CHANGE);
    }

    /// Floating average of the recent measurements in meter, -1 if there is no valid value
    /// (no measurement yet or the sensor does not answer). Never blocks.
    float getAccumulatedValue() const { return _curval.value_or(-1.f); }

    void update() {
        if (_pulse_ready) {
            noInterrupts();
            const unsigned long pulse = _pulse_us;
            _pulse_ready = false;
            interrupts();

            float measured = (static_cast<float>(pulse) * 0.000343f) / 2.f; // microseconds * speed of sound in m/us, there and back
            if (measured > _max_range) measured = _max_range;

            _curval = _curval.has_value() ? measured * _alpha + (1.f - _alpha) * _curval.value() : measured;
            _triggered = false;
            _missed = 0;
        }

        const unsigned long now = millis();
        if (now - _last_trigger < static_cast<unsigned long>(fetch_interval_milliseconds)) return; // wait till time passed

        if (_triggered && ++_missed >= _max_missed) _curval.reset(); // sensor does not answer
        _last_trigger = now;
        _triggered = true;

        digitalWrite(pin_trigger, HIGH);
        delayMicroseconds(10);
        digitalWrite(pin_trigger, LOW);
    }
};

#endif //SRC_ECHOSENSORINTERFACE_H
