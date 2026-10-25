#ifndef SRC_COASTER_MECHANISM_HAL_H
#define SRC_COASTER_MECHANISM_HAL_H

#include <Arduino.h>

#include "robot_hal.h"
#include "boardconfig.h"
#include "ledinterface.h"
#include "servointerface.h"
#include "console_recovery.h"

/** RobotHAL nur fuer den Coaster-Mechanismus: Servos und LED.
 *
 * Fuer Tests der Servo-Ablaeufe von CoasterbotFunctions ohne den restlichen Bot. Der Motortreiber
 * bleibt im Standby, Sensoren werden nicht initialisiert und melden "keine Kante, kein Hindernis".
 * Fahrbefehle werden verworfen.
 */
class CoasterMechanismHAL : public RobotHAL {
    // servoId 0 = Heber, 1 = Auswurfarm, wie HardwareImplementationHAL
    ServoController<board::PIN_SERVO_LIFTER> servo_lifter;
    ServoController<board::PIN_SERVO_SPENDER> servo_spender;
    LEDController<board::PIN_LED1_R, board::PIN_LED1_G, board::PIN_LED1_B> led_board;

public:
    void initializeHardware() {
        // Motortreiber sicher aus, unabhaengig vom Reset-Zustand der Pins
        pinMode(board::PIN_MOTOR_STANDBY, OUTPUT);
        digitalWrite(board::PIN_MOTOR_STANDBY, LOW);

        led_board.begin();

        // in der Ruhestellung des Mechanismus starten
        servo_lifter.begin(board::SERVO_LIFTER_ANGLE_REST);
        servo_spender.begin(board::SERVO_SPENDER_ANGLE_IDLE);
    }

    auto& ledBoard() { return led_board; }

    bool servosMoving() const { return servo_lifter.isMoving() || servo_spender.isMoving(); }

public: // RobotHAL

    bool step() override {
        recoverConsoleOutput();
        led_board.update();
        servo_lifter.update();
        servo_spender.update();
        return true;
    }

    void setLeftSpeed(float) override {}
    void setRightSpeed(float) override {}
    float getCommandedSpeed(WheelId) override { return 0.0f; }

    float getUltrasonicDistance() override { return 10.0f; } // kein Hindernis
    bool getEdgeFrontLeft() override { return false; }
    bool getEdgeFrontRight() override { return false; }
    bool getEdgeRearLeft() override { return false; }
    bool getEdgeRearRight() override { return false; }

    float getYaw() override { return 0.0f; }
    float getGyroZ() override { return 0.0f; }
    float getForwardAcceleration() override { return 0.0f; }
    float getWheelAngle(WheelId) override { return 0.0f; }

    float getTime() override { return static_cast<float>(static_cast<double>(time_us_64()) * 1e-6); }

    bool getButtonState() override { return false; }
    bool simulateButtonPress(float) override { return false; }

    void setServoPosition(int servoId, int position, unsigned long duration_ms) override {
        switch (servoId) {
        case 0: servo_lifter.moveTo(position, duration_ms); break;
        case 1: servo_spender.moveTo(position, duration_ms); break;
        }
    }

    // die Servo-Ablaeufe setzen keine LED, die Anzeige steuert der Test ueber ledBoard()
    void led(LED_COLORS) override {}
};

#endif // SRC_COASTER_MECHANISM_HAL_H
