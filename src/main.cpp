/** CoasterBot - Firmware fuer das Raspberry Pi Pico 2 Board (RP2350).
 *
 * Framework: Arduino, Lang: C++, gebaut mit PlatformIO -> platformio.ini
 *
 */

#include <Arduino.h>

#include "hw_impl_hal.h"
#include "hw_base_tests.h"

static HardwareImplementationHAL _bot;
static hw_test::PosSequenceNav<HardwareImplementationHAL> _test;

void setup() {
    Serial.begin(115200);

    _bot.initializeHardware();
    _bot.setUserLedIndicatorToAutomatic(false);

    _test.begin(_bot);
}


void loop() {
    _bot.step(); // bot loop, sensor eval etc.

    _test.loop(_bot);
}
