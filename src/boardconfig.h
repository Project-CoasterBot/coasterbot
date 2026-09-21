#ifndef SRC_BOARD_CONFIG_H
#define SRC_BOARD_CONFIG_H

/** Pinbelegung des CoasterBot auf dem Raspberry Pi Pico 2.
 *
 * entspricht dem Schaltplan des Protoboards 30x24.
 *
 * Die Angaben sind GPIO-Nummern (GPxx), NICHT die Pin-Nummern des Boards.
 * Der Arduino-Core fuer den RP2350 (arduino-pico) nutzt genau diese
 * GPIO-Nummern auch als Arduino-Pinnummern, die Werte koennen also direkt an
 * pinMode()/digitalRead()/analogWrite() uebergeben werden.
 */
// Optionale, lokale Abweichungen der Pinbelegung fuer einzelne Boards.
// boardconfig_local.h ist per .gitignore ausgeschlossen und kann z.B.
// "#define BOARD_PIN_LED1_R 12" enthalten.
#if __has_include("boardconfig_local.h")
#include "boardconfig_local.h"
#endif

#ifndef BOARD_PIN_LED1_R
#define BOARD_PIN_LED1_R 13 // GP13
#endif
#ifndef BOARD_PIN_LED1_G
#define BOARD_PIN_LED1_G 15 // GP15
#endif

namespace board {

// Sensoren
constexpr int PIN_EDGE_FRONT_LEFT  = 0; // GP0
constexpr int PIN_EDGE_FRONT_RIGHT = 1; // GP1
constexpr int PIN_EDGE_REAR_LEFT   = 2; // GP2
constexpr int PIN_EDGE_REAR_RIGHT  = 3; // GP3

constexpr int PIN_ULTRASONIC_ECHO    = 4; // GP4
constexpr int PIN_ULTRASONIC_TRIGGER = 5; // GP5

// Inertialsensor
constexpr int PIN_INERTIAL_SDA = 16; // GP16 = I2C0 SDA
constexpr int PIN_INERTIAL_SCL = 17; // GP17 = I2C0 SCL

// Motortreiber
constexpr int PIN_MOTOR_PWM     = 18; // GP18
// hier eventuell zweites PWM falls wir beide Seiten mit unterschiedlichen Geschwindigkeiten benötigen(Kurve im Fahren)
constexpr int PIN_MOTOR_STANDBY = 21; // GP21
constexpr int PIN_MOTOR_RIGHT_1 = 20; // GP20
constexpr int PIN_MOTOR_RIGHT_2 = 19; // GP19
constexpr int PIN_MOTOR_LEFT_1  = 27; // GP27
constexpr int PIN_MOTOR_LEFT_2  = 28; // GP28

// Coaster Servos
constexpr int PIN_COASTER_SERVO_1 = 6; // GP6
constexpr int PIN_COASTER_SERVO_2 = 7; // GP7

// UI
constexpr int PIN_USER_BUTTON = 10; // GP10, active low (gedrueckt = LOW)

constexpr int PIN_LED1_R = BOARD_PIN_LED1_R; // default GP13
constexpr int PIN_LED1_G = BOARD_PIN_LED1_G; // default GP15
constexpr int PIN_LED1_B = 14; // GP14

} // namespace board

#endif // SRC_BOARD_CONFIG_H
