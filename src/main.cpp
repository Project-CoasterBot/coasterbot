/** CoasterBot - Firmware fuer das Raspberry Pi Pico 2 Board (RP2350).
 *
 * Framework: Arduino, Lang: C++, gebaut mit PlatformIO -> platformio.ini
 *
 * Mit COASTERBOT_SERVO_CALIBRATION gebaut (pio run -e pico2_servo_calibration) startet statt der
 * Firmware die Servo-Normierung ueber die serielle Konsole, der Bot faehrt dann nicht.
 * Mit COASTERBOT_COASTER_TEST gebaut (pio run -e pico2_coaster_test oder Projekt coaster_test/)
 * startet der Test von Coaster-Ausgabe und -Aufnahme, ebenfalls ueber die Konsole.
 */

#include <Arduino.h>

#ifdef COASTERBOT_SERVO_CALIBRATION

#include "servo_calibration.h"

static ServoCalibration _calibration;

void setup() {
    Serial.begin(115200);
    _calibration.begin();
}

void loop() {
    _calibration.loop();
}

#elif defined(COASTERBOT_COASTER_TEST)

#include "coaster_test.h"

static CoasterTest _coaster_test;

void setup() {
    Serial.begin(115200);
    _coaster_test.begin();
}

void loop() {
    _coaster_test.loop();
}

#else

#include "hw_impl_hal.h"
#include "hw_base_tests.h"
#include "hw_webots_test.h"

static HardwareImplementationHAL _bot;
//static hw_test::PosSequenceNav<HardwareImplementationHAL> _test;
static WebotsTest<HardwareImplementationHAL> _webots;

void setup() {
    Serial.begin(115200);

    _bot.initializeHardware();
    _bot.setUserLedIndicatorToAutomatic(false);

//    _test.begin(_bot);

    _webots.begin(_bot);
}


void loop() {
    _bot.step(); // bot loop, sensor eval etc.

//    _test.loop(_bot);

    _webots.loop(_bot);
}

#endif
