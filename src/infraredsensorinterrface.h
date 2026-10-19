#ifndef SRC_INFRAREDSENSORINTERRFACE_H
#define SRC_INFRAREDSENSORINTERRFACE_H

#include <Arduino.h>

/** IR reflex sensor (edge detection), debounced.
 *
 * A single raw sample is not trusted: motor start (current peak, supply dip) or a dark spot on the table can
 * flip the comparator output for a moment. "No obstacle" (= edge, no floor) is only reported after the raw
 * value stayed so for edge_confirm_milliseconds. Returning to "obstacle" (= floor) is reported immediately,
 * so a real edge is never held longer than necessary.
 *
 * At ~0.1 m/s, 15 ms are 1.5 mm of travel.
 */
template<int pin, unsigned edge_confirm_milliseconds = 15>
class InfraredSensorControl {
    unsigned long _no_obstacle_since {0};
    bool _no_obstacle_raw {false};

    public:

    InfraredSensorControl() { static_assert(pin >= 0); }

    void begin() {
        pinMode(pin, INPUT);
        _no_obstacle_raw = false;
    }

    bool obstacleDetected() {
        const unsigned long now = millis();
        const bool no_obstacle = digitalRead(pin) != LOW;

        if (!no_obstacle) {
            _no_obstacle_raw = false;
            return true;
        }
        if (!_no_obstacle_raw) {
            _no_obstacle_raw = true;
            _no_obstacle_since = now;
        }
        return now - _no_obstacle_since < edge_confirm_milliseconds;
    }
};

#endif //SRC_INFRAREDSENSORINTERRFACE_H
