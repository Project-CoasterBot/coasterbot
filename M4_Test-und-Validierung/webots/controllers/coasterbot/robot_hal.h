#ifndef ROBOT_HAL_H
#define ROBOT_HAL_H

// ---------------------------------------------------------------------
// Hardware-Abstraktionsschicht (HAL)
//
// Dieses Interface kennt WEDER Webots NOCH Arduino. Es beschreibt nur,
// welche Faehigkeiten der Roboter hat (Motoren, Sensoren, Zeit).
//
//   - Fuer Webots: siehe WebotsHAL (webots_hal.h/.cpp)
//   - Fuer Arduino: eine ArduinoHAL-Klasse mit identischer Signatur
//     implementieren (z.B. Motoren ueber PWM/Ultraschall ueber
//     NewPing/Sensoren ueber analogRead()).
//
// RobotLogic (robot_logic.h/.cpp) verwendet NUR dieses Interface und
// kann daher unveraendert von Webots auf Arduino uebernommen werden.
// ---------------------------------------------------------------------
// Rad-Indizes - nur zum Auslesen der Encoder (getWheelAngle). Angesteuert
// werden die Raeder nur seitenweise: die zwei Motoren einer Seite sind
// gekoppelt und laufen immer zusammen (wie beim realen Coasterbot).
enum WheelId {
    WHEEL_FL = 0,
    WHEEL_FR = 1,
    WHEEL_RL = 2,
    WHEEL_RR = 3
};

class RobotHAL {
public:
    virtual ~RobotHAL() {}

    // Muss einmal pro Regelzyklus aufgerufen werden.
    // Webots: entspricht robot->step(timeStep) != -1
    // Arduino: kann z.B. einfach "return true;" liefern
    virtual bool step() = 0;

    // Antrieb: linke bzw. rechte Seite (rad/s, positiv = vorwaerts-drehend).
    // Beide Raeder der jeweiligen Seite laufen zwangslaeufig zusammen
    // (Skid-Steer, gekoppelte Motoren). Lenken ueber die Drehzahldifferenz.
    virtual void setLeftSpeed(float radPerSec) = 0;
    virtual void setRightSpeed(float radPerSec) = 0;

    // Distanzsensoren. Rueckgabewert-Semantik ist bewusst simpel gehalten
    // (siehe Kommentare in der jeweiligen HAL-Implementierung):
    //   getUltrasonicDistance() -> Meter, kleiner = naeher
    //   getEdge*()              -> groesser = "kein Boden erkannt" (Kante!)
    virtual float getUltrasonicDistance() = 0;
    virtual float getEdgeFrontLeft() = 0;
    virtual float getEdgeFrontRight() = 0;
    virtual float getEdgeRearLeft() = 0;
    virtual float getEdgeRearRight() = 0;

    // IMU
    virtual float getYaw() = 0;    // Rotation um die Hochachse [rad]
    virtual float getGyroZ() = 0;  // Winkelgeschwindigkeit um Hochachse [rad/s]

    // Odometrie: akkumulierter Drehwinkel EINES Rades [rad], positiv =
    // vorwaerts. Webots: PositionSensor am Radgelenk.
    // Arduino: akkumulierte Encoder-Ticks * (2*pi / Ticks-pro-Umdrehung).
    virtual float getWheelAngle(WheelId wheel) = 0;

    // Laufzeit seit Programmstart [s] - fuer zeitbasierte Zustandsuebergaenge
    virtual float getTime() = 0;
};

#endif  // ROBOT_HAL_H
