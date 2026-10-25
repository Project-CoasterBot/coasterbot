#include <Arduino.h>

#include "boardconfig.h"
#include "motioninterface.h"

namespace {

constexpr uint8_t MOTOR_TEST_PWM = 100;
constexpr unsigned long STARTUP_DELAY_MS = 3000;
constexpr unsigned long SPIN_DURATION_MS = 1500;
constexpr unsigned long PAUSE_DURATION_MS = 2000;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

using MotorController = MotionController<
    board::PIN_MOTOR_PWM,
    board::PIN_MOTOR_STANDBY,
    board::PIN_MOTOR_LEFT_1,
    board::PIN_MOTOR_LEFT_2,
    board::PIN_MOTOR_RIGHT_1,
    board::PIN_MOTOR_RIGHT_2
>;

MotorController motors;

volatile uint32_t left_edges = 0;
volatile uint32_t right_edges = 0;

void onLeftEncoderEdge() {
    left_edges = left_edges + 1;
}

void onRightEncoderEdge() {
    right_edges = right_edges + 1;
}

enum class TestPhase {
    Startup,
    SpinLeft,
    PauseAfterLeft,
    SpinRight,
    PauseAfterRight
};

TestPhase phase = TestPhase::Startup;
unsigned long phase_started_ms = 0;
unsigned long last_print_ms = 0;

}  // namespace

void setup() {
    Serial.begin(115200);

    motors.begin(MOTOR_TEST_PWM);
    motors.stop();

    pinMode(board::PIN_ENCODER_LEFT, INPUT);
    pinMode(board::PIN_ENCODER_RIGHT, INPUT);

    attachInterrupt(digitalPinToInterrupt(board::PIN_ENCODER_LEFT), onLeftEncoderEdge, CHANGE);
    attachInterrupt(digitalPinToInterrupt(board::PIN_ENCODER_RIGHT), onRightEncoderEdge, CHANGE);

    phase_started_ms = millis();
    Serial.println("CoasterBot motor and encoder test");
    Serial.println("Motors start after 3 seconds: left turn, pause, right turn, pause.");
    Serial.println("Keep the wheels clear of the table and other objects.");
}

void loop() {
    const unsigned long now = millis();

    switch (phase) {
    case TestPhase::Startup:
        if (now - phase_started_ms >= STARTUP_DELAY_MS) {
            motors.turnLeft();
            phase = TestPhase::SpinLeft;
            phase_started_ms = now;
            Serial.println("Motor test: turning left");
        }
        break;
    case TestPhase::SpinLeft:
        if (now - phase_started_ms >= SPIN_DURATION_MS) {
            motors.stop();
            phase = TestPhase::PauseAfterLeft;
            phase_started_ms = now;
            Serial.println("Motor test: stopped");
        }
        break;
    case TestPhase::PauseAfterLeft:
        if (now - phase_started_ms >= PAUSE_DURATION_MS) {
            motors.turnRight();
            phase = TestPhase::SpinRight;
            phase_started_ms = now;
            Serial.println("Motor test: turning right");
        }
        break;
    case TestPhase::SpinRight:
        if (now - phase_started_ms >= SPIN_DURATION_MS) {
            motors.stop();
            phase = TestPhase::PauseAfterRight;
            phase_started_ms = now;
            Serial.println("Motor test: stopped");
        }
        break;
    case TestPhase::PauseAfterRight:
        if (now - phase_started_ms >= PAUSE_DURATION_MS) {
            motors.turnLeft();
            phase = TestPhase::SpinLeft;
            phase_started_ms = now;
            Serial.println("Motor test: turning left");
        }
        break;
    }
    motors.update();

    if (now - last_print_ms < PRINT_INTERVAL_MS) {
        return;
    }
    last_print_ms = now;

    noInterrupts();
    const uint32_t left_count = left_edges;
    const uint32_t right_count = right_edges;
    interrupts();

    Serial.print("LEFT GP");
    Serial.print(board::PIN_ENCODER_LEFT);
    Serial.print(" level=");
    Serial.print(digitalRead(board::PIN_ENCODER_LEFT) == HIGH ? "HIGH" : "LOW");
    Serial.print(" edges=");
    Serial.print(left_count);

    Serial.print(" | RIGHT GP");
    Serial.print(board::PIN_ENCODER_RIGHT);
    Serial.print(" level=");
    Serial.print(digitalRead(board::PIN_ENCODER_RIGHT) == HIGH ? "HIGH" : "LOW");
    Serial.print(" edges=");
    Serial.println(right_count);
}
