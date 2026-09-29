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

void setup() {
    Serial.begin(115200);

    _bot.initializeHardware();

    next_heartbeat = millis();
}

void loop() {
    const unsigned long now = millis();
    unsigned duration_ms;

    // bot loop, sensor eval etc.
    _bot.step();

    // button eval, could also be implemented via virtual method of bot
    if (_bot.userButtonReleased(duration_ms)) {
        if (duration_ms > 1500) {
            Serial.println("[Motion] Reset and stop ");
            motion_state = 0;
        } else
            motion_state++;

        float speed_left = 0, speed_right = 0;

        switch (motion_state % 3u) {
        default:
        case 0: break; // nothing to do
        case 1: speed_left = 1.; speed_right = 1.; break; // forward
        case 2: speed_left = -1.; speed_right = -1.; break; // backward
        }

        _bot.setLeftSpeed(speed_left);
        _bot.setRightSpeed(speed_right);
    }
}
