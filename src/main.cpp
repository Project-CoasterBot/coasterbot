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
#include "infraredsensorinterrface.h"
#include "echosensorinterface.h"
#include "servointerface.h"

static LEDController<board::PIN_LED1_R, board::PIN_LED1_G, board::PIN_LED1_B> led_board;
static ButtonController<board::PIN_USER_BUTTON> user_button;

static EchosensorInterface<board::PIN_ULTRASONIC_ECHO, board::PIN_ULTRASONIC_TRIGGER> obstcl_sensor;

static InfraredSensorControl<board::PIN_EDGE_REAR_LEFT> edge_det_rear_left;
static InfraredSensorControl<board::PIN_EDGE_REAR_RIGHT> edge_det_rear_right;

static MotionController<board::PIN_MOTOR_PWM, board::PIN_MOTOR_STANDBY, board::PIN_MOTOR_LEFT_1, board::PIN_MOTOR_LEFT_2, board::PIN_MOTOR_RIGHT_1, board::PIN_MOTOR_RIGHT_2> motion_ctrl;

static ServoController<board::PIN_COASTER_SERVO_1> servo1;
static ServoController<board::PIN_COASTER_SERVO_2> servo2;


static unsigned long next_heartbeat = 0;
static int motion_state = 0;

void setup() {
    Serial.begin(115200);

    led_board.begin();
    led_board.setModeToBlinkGreen(255, 500);

    user_button.begin();
    motion_ctrl.begin();
    motion_ctrl.stop();

    servo1.begin();
    servo2.begin();

    edge_det_rear_left.begin();
    edge_det_rear_right.begin();

    obstcl_sensor.begin();

    next_heartbeat = millis();
}

void loop() {
    const unsigned long now = millis();
    unsigned long user_button_pressed_duration_millis;


    // 1. fetch new input

    bool user_button_pressed = user_button.update(now, user_button_pressed_duration_millis);

    bool edge_rear_left = ! edge_det_rear_left.obstacleDetected();
    bool edge_rear_right = ! edge_det_rear_right.obstacleDetected();

    // 2. Internal logic

    if (user_button_pressed) {
        Serial.printf("[ACT] press dur=%lu ms\n", user_button_pressed_duration_millis);
        led_board.setModeToRainbow(user_button_pressed_duration_millis);

        /*
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
        }*/

        switch (motion_state % 4) {
        default:
        case 0:
            break;
        case 1:
            servo2.moveTo(0, 1500);
            break;
        case 2:
            servo2.moveTo(180, 1500);
            break;
        case 3:
            servo2.moveTo(0, 1500);
            break;
        }

        motion_state++;
    }

    // 3. Output

    led_board.update();
    motion_ctrl.update();
    obstcl_sensor.update();
    servo1.update();
    servo2.update();

    if (static_cast<long>(now - next_heartbeat) >= 0) {
        next_heartbeat = now + 1000;
        Serial.printf("[HEARTBEAT] t=%lu ms\n", now);

        if (edge_rear_left) Serial.printf("[SENSOR] edge rear left detected\n");
        if (edge_rear_right) Serial.printf("[SENSOR] edge rear right detected\n");

        // only read in heartbeat since it blocks
        double distance = obstcl_sensor.getAccumulatedValue();

        Serial.printf("[SENSOR] obstcl dist=");
        Serial.print(distance, 3);
        Serial.printf("\n");
    }

    delay(1); // Regelzyklus ~1 kHz. mit entsprechender logik besser von delta zu now() abhängig.
}
