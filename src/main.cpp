/** CoasterBot - Firmware fuer das Raspberry Pi Pico 2 Board (RP2350).
 *
 * Framework: Arduino, Lang: C++, gebaut mit PlatformIO -> platformio.ini
 *
 */

#include <Arduino.h>

#include "hw_impl_hal.h"

static HardwareImplementationHAL _bot;

static unsigned long next_heartbeat = 0;

// ---------------------------------------------------------------------
// Wheel actuator sanity test: short button press triggers a timed
// forward pulse, a pause, then a timed backward pulse - just enough
// travel (a few mm) to confirm both sides actually turn and reverse,
// without needing a full MODE_WHEEL_TEST-style continuous run.
//
// PULSE_MS includes MotionController's ~100ms arming dead time (see
// motioninterface.h, wait_time_dir_change) before the driver is
// actually enabled - the wheels only really turn for the last part of
// that window. Tune PULSE_MS on the actual robot: too short and
// nothing moves (never leaves ARMING), too long and it travels more
// than "a few mm". Start here and adjust based on what you observe.
// ---------------------------------------------------------------------
enum TestPhase { WAIT_BUTTON, PULSE_FWD, PAUSE, PULSE_BACK, DONE };
static TestPhase test_phase = WAIT_BUTTON;
static unsigned long phase_start = 0;

static const unsigned long PULSE_MS = 250;  // tune this first
static const unsigned long PAUSE_MS = 1000; // breathing room to observe where it stopped

void setup() {
    Serial.begin(115200);

    _bot.initializeHardware();

    next_heartbeat = millis();
}

void loop() {
    const unsigned long now = millis();
    unsigned press_duration_ms;

    // bot loop, sensor eval etc.
    _bot.step();

    if (test_phase == WAIT_BUTTON && _bot.userButtonReleased(press_duration_ms)) {
        Serial.println("[WheelTest] forward pulse");
        _bot.setLeftSpeed(1.f);
        _bot.setRightSpeed(1.f);
        test_phase = PULSE_FWD;
        phase_start = now;
    }

    switch (test_phase) {
        case PULSE_FWD:
            if (now - phase_start >= PULSE_MS) {
                _bot.setLeftSpeed(0.f);
                _bot.setRightSpeed(0.f);
                Serial.println("[WheelTest] stopped, pausing");
                test_phase = PAUSE;
                phase_start = now;
            }
            break;
        case PAUSE:
            if (now - phase_start >= PAUSE_MS) {
                Serial.println("[WheelTest] backward pulse");
                _bot.setLeftSpeed(-1.f);
                _bot.setRightSpeed(-1.f);
                test_phase = PULSE_BACK;
                phase_start = now;
            }
            break;
        case PULSE_BACK:
            if (now - phase_start >= PULSE_MS) {
                _bot.setLeftSpeed(0.f);
                _bot.setRightSpeed(0.f);
                Serial.println("[WheelTest] done - press the button again to repeat");
                test_phase = DONE;
            }
            break;
        case DONE:
            if (_bot.userButtonReleased(press_duration_ms))
                test_phase = WAIT_BUTTON; // re-arm, next press starts another pulse pair
            break;
        default:
            break;
    }
}
