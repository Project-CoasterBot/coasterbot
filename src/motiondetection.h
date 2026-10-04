#ifndef SRC_MOTIONDETECTION_H
#define SRC_MOTIONDETECTION_H
#include <cmath>
#include <cstdint>

#include <Arduino.h>

/** Wheel encoder via LM393 fork light barrier and a slotted disc on the gearbox output shaft.
 *
 * Every level change of the sensor (rising and falling edge) is counted in an interrupt, so a
 * slotted disc with N slots yields 2*N counts per wheel revolution.
 *
 * A single channel cannot tell the turning direction. The main loop therefore has to call
 * setCurrentDirection() in every iteration with the direction the motor controller currently drives
 * (or coasts down from). The interrupt counts up or down according to that direction.
 *
 * The counter state is static per template instance (the ISR has no context pointer), so only one
 * instance per signal pin must exist.
 */
template<int signal_pin, unsigned long debounce_us = 500>
class MotionDetectionOptoCoupling {

    static constexpr int _wheel_slots {20};

    static inline volatile int32_t _diff_count {0};
    static inline volatile int8_t _direction {1};          // +1 forward, -1 backward
    static inline volatile unsigned long _last_edge_us {0};

    float _wheel_circ { 0.067f * static_cast<float>(M_PI) }; // default my TT wheel = 6.7 cm diameter

    static void onEdge() {
        const unsigned long now = micros();
        if (now - _last_edge_us < debounce_us) return; // edge bounce of the LM393 comparator
        _last_edge_us = now;
        _diff_count += _direction;
    }

public:

    void begin() {
        pinMode(signal_pin, INPUT);
        delay(100);
        _diff_count = 0;
        _last_edge_us = micros();
        attachInterrupt(digitalPinToInterrupt(signal_pin), onEdge, CHANGE);
    }

    void setWheelDiameter(float diameter_in_meter) {
        if (! std::isfinite(diameter_in_meter) || diameter_in_meter <= 0.f) return;
        setWheelCircumference(static_cast<float>(M_PI) * diameter_in_meter);
    }
    void setWheelCircumference(float circ_in_meter) {
        if (! std::isfinite(circ_in_meter) || circ_in_meter <= 0.f) return;
        _wheel_circ = circ_in_meter;
    }

    /// Current turning direction of the wheel: > 0 forward, < 0 backward. 0 (standing) keeps the last
    /// direction, so ticks of a wheel still rolling out are counted with the sign it was driven in.
    void setCurrentDirection(int direction) {
        if (direction > 0) _direction = 1;
        else if (direction < 0) _direction = -1;
    }

    /// Movement since the last call, in rad of the wheel and in meter on the ground. Positive = forward.
    void getDistance(float& moved_rad, float& moved_distance) {
        noInterrupts();
        const int32_t count = _diff_count;
        _diff_count = 0;
        interrupts();

        constexpr float pi2 = 2.f * static_cast<float>(M_PI);
        constexpr float norm = 1.f / static_cast<float>(_wheel_slots * 2); // 1 / how many switches per full turn.
        moved_rad = static_cast<float>(count) * pi2 * norm;
        moved_distance = static_cast<float>(count) * _wheel_circ * norm;
    }
};

#endif //SRC_MOTIONDETECTION_H
