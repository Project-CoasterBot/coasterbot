/** CoasterBot - Firmware fuer das Raspberry Pi Pico 2 Board (RP2350).
 *
 * Framework: Arduino, Lang: C++, gebaut mit PlatformIO -> platformio.ini
 *
 */

#include <Arduino.h>

#include "hw_impl_hal.h"

static HardwareImplementationHAL _bot;

static unsigned long next_heartbeat = 0;
static unsigned motion_state = 0;

static bool move_fwd = true;

void setup() {
    Serial.begin(115200);

    _bot.initializeHardware();

    next_heartbeat = millis();
}

void loop() {
    unsigned press_duration_ms;
    unsigned long movement_prevented_duration_ms;

    // bot loop, sensor eval etc.
    _bot.step();

    // button eval, could also be implemented via virtual method of bot
    if (_bot.userButtonReleased(press_duration_ms)) {
        if (press_duration_ms > 1500) {
            Serial.println("[Motion] Reset and stop ");
            motion_state = 0;
            move_fwd = true;
        } else
            motion_state++;
    }

    if (motion_state % 2u == 1) { // "driving fwd/backward"
        float speed_left = 0, speed_right = 0;
        if (_bot.movementPrevented(movement_prevented_duration_ms)) {
            if (movement_prevented_duration_ms > 2500)
                move_fwd = ! move_fwd;
        }

        speed_left = move_fwd ? 1. : -1.;
        speed_right = speed_left;

        _bot.setLeftSpeed(speed_left);
        _bot.setRightSpeed(speed_right);
    }
}
