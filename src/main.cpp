/** CoasterBot - Firmware fuer das Raspberry Pi Pico 2 Board (RP2350).
 *
 * Framework: Arduino, Lang: C++, gebaut mit PlatformIO -> platformio.ini
 *
 */

#include <Arduino.h>

#include "boardconfig.h"
#include "ledinterface.h"
#include "buttoninterface.h"
#include "motioninterface.h"

static LEDController<board::PIN_LED1_R, board::PIN_LED1_G, board::PIN_LED1_B> led_board;
static ButtonController<board::PIN_USER_BUTTON> user_button;
static MotionController<board::PIN_MOTOR_PWM, board::PIN_MOTOR_STANDBY, board::PIN_MOTOR_LEFT_1, board::PIN_MOTOR_LEFT_2, board::PIN_MOTOR_RIGHT_1, board::PIN_MOTOR_RIGHT_2> motion_ctrl;

static unsigned long next_heartbeat = 0;

static int motion_state = 0;

void setup() {
    Serial.begin(115200);

    led_board.begin();
    led_board.setModeToBlinkGreen(255, 500);

    user_button.begin();
    motion_ctrl.begin();
    motion_ctrl.stop();

    next_heartbeat = millis();
}

void loop() {
    const unsigned long now = millis();
    unsigned long user_button_pressed_duration_millis;


    // 1. fetch new input

    bool user_button_pressed = user_button.update(now, user_button_pressed_duration_millis);



    // 2. Internal logic

    if (user_button_pressed) {
        Serial.printf("[ACT] press dur=%lu ms\n", user_button_pressed_duration_millis);
        led_board.setModeToRainbow(user_button_pressed_duration_millis);

        switch (motion_state % 5) {
        default:
        case 0:
            Serial.println("[Motion] Stop ");
            motion_ctrl.stop();
            break;
        case 1:
            Serial.println("[Motion] Forward ");
            motion_ctrl.forward();
            break;
        case 2:
            Serial.println("[Motion] Left ");
            motion_ctrl.turnLeft();
            break;
        case 3:
            Serial.println("[Motion] Right ");
            motion_ctrl.turnRight();
            break;
        case 4:
            Serial.println("[Motion] Backward ");
            motion_ctrl.backward();
            break;
        }

        motion_state++;
    }

    // 3. Output

    led_board.update();
    motion_ctrl.update();

    if (static_cast<long>(now - next_heartbeat) >= 0) {
        next_heartbeat = now + 1000;
        Serial.printf("[HEARTBEAT] t=%lu ms\n", now);
    }

    delay(1); // Regelzyklus ~1 kHz. mit entsprechender logik besser von delta zu now() abhängig.
}
