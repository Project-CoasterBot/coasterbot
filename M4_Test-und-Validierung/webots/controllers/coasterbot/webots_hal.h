#ifndef WEBOTS_HAL_H
#define WEBOTS_HAL_H

#include "robot_hal.h"
#include <webots/Robot.hpp>
#include <webots/Motor.hpp>
#include <webots/PositionSensor.hpp>
#include <webots/DistanceSensor.hpp>
#include <webots/InertialUnit.hpp>
#include <webots/Gyro.hpp>
#include <random>

// ---------------------------------------------------------------------
// Implementiert RobotHAL mit der Webots-C++-API.
//
// Das ist die EINZIGE Datei, die bei einem spaeteren Arduino-Port durch
// eine ArduinoHAL ersetzt werden muesste (analoges Interface, aber mit
// Arduino-Bibliotheken wie Servo/AFMotor, NewPing, MPU6050 etc.).
// robot_logic.h/.cpp bleiben dabei unveraendert.
// ---------------------------------------------------------------------
class WebotsHAL : public RobotHAL {
public:
    WebotsHAL();
    ~WebotsHAL() override;

    bool step() override;
    void setLeftSpeed(float radPerSec) override;
    void setRightSpeed(float radPerSec) override;
    float getUltrasonicDistance() override;
    bool  getEdgeFrontLeft() override;
    bool  getEdgeFrontRight() override;
    bool  getEdgeRearLeft() override;
    bool  getEdgeRearRight() override;
    float getYaw() override;
    float getGyroZ() override;
    float getWheelAngle(WheelId wheel) override;
    float getTime() override;

    // ---------------------------------------------------------------
    // Offener Punkt "Erreichbarkeit einer definierten Winkelgeschwindigkeit"
    // (Softwaredokumentation, Abschnitt 4.3): simuliert und vergleichbar
    // gemacht, siehe Kommentar bei updateMotorControl() weiter unten.
    // Diese Erweiterung ist bewusst NICHT Teil von RobotHAL: eine echte
    // ArduinoHAL trifft die Open-/Closed-Loop-Entscheidung fest in ihrer
    // Implementierung, ein Umschalter zur Laufzeit ergibt auf realer
    // Hardware keinen Sinn (es gibt dort nur die eine physische Wahrheit).
    // Hier dient er ausschliesslich dem Vergleichstest MODE_MOTOR_CONTROL_TEST.
    // ---------------------------------------------------------------
    enum class MotorControlMode { OPEN_LOOP, CLOSED_LOOP };
    void setMotorControlMode(MotorControlMode mode) { motorMode_ = mode; }
    MotorControlMode motorControlMode() const { return motorMode_; }

    // Tatsaechlich gemessene Radgeschwindigkeit je Seite [rad/s], aus der
    // Encoder-Aenderung des letzten Regelzyklus (positiv = vorwaerts, wie
    // setLeftSpeed/setRightSpeed). Fuer den Vergleichstest: zeigt, wie gut
    // Kommando und Wirkung tatsaechlich uebereinstimmen.
    float measuredLeftSpeed() const { return measuredLeft_; }
    float measuredRightSpeed() const { return measuredRight_; }

    // ---------------------------------------------------------------
    // Fehlerinjektion (Testkonzept ST-SIM-005/006). Simulation-only, wie
    // MotorControlMode oben - ein realer Sensor-/Aktorausfall entsteht
    // durch Hardware, nicht durch einen Laufzeit-Schalter, darum NICHT Teil
    // von RobotHAL. Die Anwendungslogik (main.cpp, FaultMonitor) sieht nur
    // das Symptom ueber die normale RobotHAL-Schnittstelle:
    // getUltrasonicDistance() liefert dann einen ausserhalb des
    // Sensorkontrakts liegenden Wert, das linke Rad bewegt sich trotz
    // Fahrbefehl nicht (siehe updateMotorControl()) - fuer den Monitor ist
    // das ein "echter" Ausfall, kein Test-Sonderfall.
    // ---------------------------------------------------------------
    void injectUltrasonicFault(bool active) { ultrasonicFault_ = active; }
    void injectLeftMotorFault(bool active)  { leftMotorFault_ = active; }

private:
    webots::Robot robot_;
    int timeStep_;

    // Je ein Handle pro Seite. Die Motoren einer Seite sind im PROTO
    // gekoppelt ("coupled motors"), ein Befehl an dieses Handle treibt
    // beide Raeder der Seite.
    webots::Motor* motorLeft_;
    webots::Motor* motorRight_;

    // Vier PositionSensor, weil das PROTO sie je Rad bereitstellt - die
    // reale Elektronik hat nur einen Encoder je Seite. getWheelAngle()
    // mittelt Front/Heck je Seite und gibt nur noch WHEEL_LEFT/WHEEL_RIGHT
    // nach aussen; diese vier Handles sind reines Simulationsdetail.
    webots::PositionSensor* encFL_;
    webots::PositionSensor* encFR_;
    webots::PositionSensor* encRL_;
    webots::PositionSensor* encRR_;

    webots::DistanceSensor* ultrasonic_;
    webots::DistanceSensor* edgeFL_;
    webots::DistanceSensor* edgeFR_;
    webots::DistanceSensor* edgeRL_;
    webots::DistanceSensor* edgeRR_;

    webots::InertialUnit* imu_;
    webots::Gyro* gyro_;

    // Die Kantensensoren liefern in der Simulation bereits ein digitales
    // Signal (siehe Coasterbot.proto-lookupTable: 0 oder 1, kein analoger
    // Rohwert mehr). digitalEdge() liest diesen Pegel und bildet ihn ueber
    // EDGE_ACTIVE_HIGH auf das semantische bool ab - die einzige Stelle, an
    // der die Polaritaet des Sensors bekannt sein muss. Eine ArduinoHAL, die
    // tatsaechlich digitalRead() an einem Pin aufruft, braucht dieselbe
    // Abbildung (z.B. digitalRead(pin) == HIGH) und passt ggf. nur diese
    // Konstante an, falls das reale Sensormodul aktiv-low ist.
    static constexpr bool EDGE_ACTIVE_HIGH = true;
    bool digitalEdge(webots::DistanceSensor* sensor) const;

    // ---------------------------------------------------------------
    // GY-521/MPU-6050-Gyro-Simulation (BOM Pos. 11: "ICQUANZX GY-521
    // MPU6050; Navigationssensor"). Webots' Solid-Physik liefert eine
    // ideale, fehlerfreie Rate - Bias und Rauschen eines realen
    // MPU-6050 werden deshalb hier in Software nachgebildet:
    //   - GYRO_BIAS_RAD: simulierter Nullpunktfehler (Zero-Rate Output).
    //     Laut Datenblatt bis zu +-20 deg/s Werkstoleranz ohne
    //     Kalibrierung; als Demonstrationswert wird ein moderater fester
    //     Bias von 3 deg/s angenommen (kein gemessener Wert eines
    //     realen Exemplars - siehe Softwaredokumentation).
    //   - GYRO_NOISE_STD_RAD: Datenblatt "Total RMS Noise" bei FS_SEL=0
    //     (0.05 deg/s-rms), als Standardabweichung eines deterministisch
    //     erzeugten Gauss-Rauschens (fester Seed -> ST-SIM-004 bleibt
    //     reproduzierbar).
    //   - Start-Kalibrierung: mittelt die ersten GYRO_CAL_SAMPLES Messungen
    //     im Konstruktor (Annahme: Roboter steht zu Beginn still, wie auf
    //     realer Hardware beim Einschalten, bevor der erste Fahrbefehl
    //     kommt) und zieht diesen Schaetzwert von jeder folgenden Messung
    //     ab - genau das in der Softwaredokumentation beschriebene
    //     Kalibrierverfahren. calibrateGyro() ruft dafuer robot_.step()
    //     direkt auf (nicht ueber das oeffentliche step()), noch bevor
    //     main.cpp die erste eigene Bewegung anstossen kann.
    static constexpr float GYRO_BIAS_RAD      = 0.0523599f;  // 3 deg/s
    static constexpr float GYRO_NOISE_STD_RAD = 0.0008727f;  // 0.05 deg/s-rms
    static constexpr int   GYRO_CAL_SAMPLES   = 32;          // ~0.5 s bei 16 ms

    std::mt19937 gyroRng_{12345u};  // fester Seed: Determinismus/ST-SIM-004
    std::normal_distribution<float> gyroNoise_{0.0f, GYRO_NOISE_STD_RAD};
    float gyroBiasEstimate_ = 0.0f;
    void calibrateGyro();

    // ---------------------------------------------------------------
    // Motorregelung: Open-Loop vs. Closed-Loop (Softwaredokumentation,
    // Abschnitt 4.3, "Erreichbarkeit einer definierten Winkelgeschwindigkeit").
    //
    // setLeftSpeed()/setRightSpeed() setzen nur noch das Kommando
    // (commandedLeft_/commandedRight_); die eigentliche Motoransteuerung
    // uebernimmt updateMotorControl(), einmal pro step(). Simuliert wird
    // dabei eine reale, unvermeidliche Fertigungsstreuung zwischen den
    // beiden Seiten (MOTOR_GAIN_LEFT_TRUE/MOTOR_GAIN_RIGHT_TRUE): Ein
    // Kommando von z.B. 4 rad/s erzeugt an der realen (simulierten) Seite
    // nicht exakt 4 rad/s, sondern 4 * Gain. Dieser Gain ist absichtlich
    // NICHT an die Regelung "durchgereicht", genau wie auf realer
    // Hardware kennt die Software ihn nicht, sie kann ihn nur ueber
    // Sensor-Rueckkopplung ausgleichen:
    //
    //   OPEN_LOOP:   appliedLeft_/appliedRight_ = commandedLeft_/commandedRight_
    //                unveraendert (wie eine feste, einmalig kalibrierte
    //                PWM-Kennlinie ohne Rueckkopplung). Der Gain-Fehler
    //                bleibt bestehen -> die Seiten laufen bei "identischem"
    //                Kommando unterschiedlich schnell, der Roboter zieht
    //                zur Seite statt geradeaus zu fahren.
    //   CLOSED_LOOP: appliedLeft_/appliedRight_ werden jeden Regelzyklus
    //                per Integralregler anhand der GEMESSENEN Radgeschwin-
    //                digkeit (measuredLeft_/measuredRight_, aus der
    //                Encoder-Aenderung) nachgefuehrt, bis diese dem
    //                Kommando entspricht, unabhaengig davon, wie gross
    //                der (der Regelung unbekannte) Gain-Fehler ist.
    //
    // CLOSED_LOOP ist der Default, damit alle anderen Betriebsarten
    // (MODE_SIM_TEST, MODE_NAVIGATE_DSTAR, ...) ihr bereits verifiziertes
    // Verhalten behalten; nur MODE_MOTOR_CONTROL_TEST schaltet bewusst auf
    // OPEN_LOOP um, um den Unterschied zu zeigen.
    // ---------------------------------------------------------------
    static constexpr float MOTOR_GAIN_LEFT_TRUE  = 0.95f;  // -5%, Fertigungsstreuung
    static constexpr float MOTOR_GAIN_RIGHT_TRUE = 1.05f;  // +5%, Fertigungsstreuung
    static constexpr float MOTOR_KI = 25.0f;               // Integralregler-Verstaerkung
    static constexpr float MOTOR_MAX_OMEGA = 20.0f;        // rad/s, Anti-Windup (= PROTO maxVelocity)

    MotorControlMode motorMode_ = MotorControlMode::CLOSED_LOOP;
    float commandedLeft_ = 0.0f, commandedRight_ = 0.0f;   // von setLeftSpeed/setRightSpeed
    float appliedLeft_ = 0.0f, appliedRight_ = 0.0f;       // tatsaechlich an den Motor gegeben
    float measuredLeft_ = 0.0f, measuredRight_ = 0.0f;     // aus Encoder-Aenderung gemessen
    float prevAngleLeft_ = 0.0f, prevAngleRight_ = 0.0f;
    float prevMotorCtrlTime_ = 0.0f;
    bool  motorCtrlInitialized_ = false;
    void updateMotorControl();

    bool ultrasonicFault_ = false;
    bool leftMotorFault_  = false;
};

#endif  // WEBOTS_HAL_H
