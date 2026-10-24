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
// Optionale, lokale Abweichungen fuer einzelne Boards.
// boardconfig_local.h ist per .gitignore ausgeschlossen und kann jedes der
// BOARD_*-Makros unten ueberschreiben, z.B.:
//     #define BOARD_PIN_LED1_R 12
//     #define BOARD_PIN_SERVO_LIFTER 7     // Heber und Auswurfarm vertauscht
//     #define BOARD_PIN_SERVO_SPENDER 6
//     #define BOARD_SERVO_LIFTER_ANGLE_REST 175
#if __has_include("boardconfig_local.h")
#include "boardconfig_local.h"
#endif

#ifndef BOARD_PIN_LED1_R
#define BOARD_PIN_LED1_R 13 // GP13
#endif
#ifndef BOARD_PIN_LED1_G
#define BOARD_PIN_LED1_G 15 // GP15
#endif

// Coaster-Servos: welcher Servo-Anschluss (GP6/GP7) Heber bzw. Auswurfarm ist
#ifndef BOARD_PIN_SERVO_LIFTER
#define BOARD_PIN_SERVO_LIFTER 6 // GP6
#endif
#ifndef BOARD_PIN_SERVO_SPENDER
#define BOARD_PIN_SERVO_SPENDER 7 // GP7
#endif

// Servowinkel [deg] des Coaster-Mechanismus, je nach Einbau der Servos verschieden.
// Heber: REST = kein Hub (ganz eingefahren), MAX = maximaler Hub (ganz ausgefahren).
#ifndef BOARD_SERVO_LIFTER_ANGLE_REST
#define BOARD_SERVO_LIFTER_ANGLE_REST 180
#endif
#ifndef BOARD_SERVO_LIFTER_ANGLE_MAX
#define BOARD_SERVO_LIFTER_ANGLE_MAX 0
#endif
// Heber je Stapelhoehe: Winkel, bei dem der unterste Coaster auf Ausschubhoehe steht, fuer 1, 2, 3, ...
// Coaster im Stapel (Servo-Normierung, pico2_servo_calibration). Ueber die vermessenen Stufen hinaus wird
// mit dem letzten Abstand bis zur Ruhestellung weitergerechnet (CoasterServoAngles::stackAngle).
// Ohne Angabe: 4 gleiche Stufen zwischen MAX und REST.
//     #define BOARD_SERVO_LIFTER_STACK_ANGLES 175, 150, 128, 104
// Auswurfarm: IDLE = Ruhestellung, gibt den Stapel frei, EJECT = schiebt den untersten Coaster aus.
#ifndef BOARD_SERVO_SPENDER_ANGLE_IDLE
#define BOARD_SERVO_SPENDER_ANGLE_IDLE 90
#endif
#ifndef BOARD_SERVO_SPENDER_ANGLE_EJECT
#define BOARD_SERVO_SPENDER_ANGLE_EJECT 0
#endif
// Auswurfmethode: 0 = mit Rampe (langsam schieben), 1 = Schlag (ohne Rampe, der Arm trifft den Coaster
// mit voller Servogeschwindigkeit, loest stark haftende Magnete besser). Testen mit "a"/"as" in der Normierung.
#ifndef BOARD_SERVO_SPENDER_EJECT_STRIKE
#define BOARD_SERVO_SPENDER_EJECT_STRIKE 0
#endif

#ifndef BOARD_PIN_ENCODER_LEFT
#define BOARD_PIN_ENCODER_LEFT 8 // GP8
#endif
#ifndef BOARD_PIN_ENCODER_RIGHT
#define BOARD_PIN_ENCODER_RIGHT 9 // GP9
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

constexpr int PIN_MOTION_DETECT_LEFT = 8; // GP8
constexpr int PIN_MOTION_DETECT_RIGHT = 9; // GP9

// Radencoder (LM393 Gabellichtschranke), je Seite einer - noch nicht im Schaltplan, Pins vorlaeufig
constexpr int PIN_ENCODER_LEFT  = BOARD_PIN_ENCODER_LEFT;  // default GP8
constexpr int PIN_ENCODER_RIGHT = BOARD_PIN_ENCODER_RIGHT; // default GP9

// Coaster Servos
constexpr int PIN_SERVO_LIFTER  = BOARD_PIN_SERVO_LIFTER;  // default GP6
constexpr int PIN_SERVO_SPENDER = BOARD_PIN_SERVO_SPENDER; // default GP7
static_assert(PIN_SERVO_LIFTER != PIN_SERVO_SPENDER, "lifter and spender servo need distinct pins");

constexpr int SERVO_LIFTER_ANGLE_REST    = BOARD_SERVO_LIFTER_ANGLE_REST;    // default 180 = ganz eingefahren
constexpr int SERVO_LIFTER_ANGLE_MAX     = BOARD_SERVO_LIFTER_ANGLE_MAX;     // default 0 = voller Hub
constexpr int SERVO_SPENDER_ANGLE_IDLE   = BOARD_SERVO_SPENDER_ANGLE_IDLE;   // default 90 = langer Arm zeigt ins Zentrum
constexpr int SERVO_SPENDER_ANGLE_EJECT  = BOARD_SERVO_SPENDER_ANGLE_EJECT;  // default 0
constexpr bool SERVO_SPENDER_EJECT_STRIKE = BOARD_SERVO_SPENDER_EJECT_STRIKE != 0; // default Rampe
static_assert(SERVO_LIFTER_ANGLE_REST >= 0 && SERVO_LIFTER_ANGLE_REST <= 180, "servo angle out of 0..180");
static_assert(SERVO_LIFTER_ANGLE_MAX >= 0 && SERVO_LIFTER_ANGLE_MAX <= 180, "servo angle out of 0..180");
static_assert(SERVO_SPENDER_ANGLE_IDLE >= 0 && SERVO_SPENDER_ANGLE_IDLE <= 180, "servo angle out of 0..180");
static_assert(SERVO_SPENDER_ANGLE_EJECT >= 0 && SERVO_SPENDER_ANGLE_EJECT <= 180, "servo angle out of 0..180");

#ifdef BOARD_SERVO_LIFTER_STACK_ANGLES
constexpr int SERVO_LIFTER_STACK_ANGLES[] = { BOARD_SERVO_LIFTER_STACK_ANGLES };
#else
constexpr int SERVO_LIFTER_STACK_ANGLES[] = {
    SERVO_LIFTER_ANGLE_MAX,
    SERVO_LIFTER_ANGLE_REST + (SERVO_LIFTER_ANGLE_MAX - SERVO_LIFTER_ANGLE_REST) * 3 / 4,
    SERVO_LIFTER_ANGLE_REST + (SERVO_LIFTER_ANGLE_MAX - SERVO_LIFTER_ANGLE_REST) * 2 / 4,
    SERVO_LIFTER_ANGLE_REST + (SERVO_LIFTER_ANGLE_MAX - SERVO_LIFTER_ANGLE_REST) * 1 / 4,
};
#endif
constexpr int SERVO_LIFTER_STACK_LEVELS = sizeof(SERVO_LIFTER_STACK_ANGLES) / sizeof(SERVO_LIFTER_STACK_ANGLES[0]);

// UI
constexpr int PIN_USER_BUTTON = 10; // GP10, active low (gedrueckt = LOW)

constexpr int PIN_LED1_R = BOARD_PIN_LED1_R; // default GP13
constexpr int PIN_LED1_G = BOARD_PIN_LED1_G; // default GP15
constexpr int PIN_LED1_B = 14; // GP14

} // namespace board

#endif // SRC_BOARD_CONFIG_H
