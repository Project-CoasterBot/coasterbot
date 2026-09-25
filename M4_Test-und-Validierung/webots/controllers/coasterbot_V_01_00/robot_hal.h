#ifndef ROBOT_HAL_H
#define ROBOT_HAL_H

#include <cstdint>

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
// Seiten-Index - fuer Encoder (getWheelAngle). Angesteuert UND gemessen wird
// nur seitenweise: die zwei Motoren einer Seite sind gekoppelt und laufen
// immer zusammen (wie beim realen Coasterbot), und die reale Elektronik
// besitzt ebenfalls nur EINEN Encoder je Seite, nicht einen je Rad.
enum WheelId {
    WHEEL_LEFT = 0,
    WHEEL_RIGHT = 1
};

class RobotHAL {
public:
    enum class LED_COLORS : uint8_t { OFF, RED, GREEN, BLUE, ORANGE };

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

    // Distanzsensoren.
    //   getUltrasonicDistance() -> Meter, kleiner = naeher (analoger Sensor)
    //   getEdge*()              -> true = Kante erkannt (kein Boden unter dem
    //                              Sensor). Die Kantensensoren liefern ein
    //                              DIGITALES Signal (das reale Modul hat
    //                              einen eingebauten Komparator, keine
    //                              analoge Rohspannung). Ob "Kante" am
    //                              Sensor/Pin elektrisch HIGH oder LOW
    //                              bedeutet (aktiv-high/aktiv-low je nach
    //                              Sensormodul), ist Aufgabe der jeweiligen
    //                              HAL-Implementierung und wird dort auf
    //                              dieses semantische bool abgebildet - die
    //                              Anwendungslogik sieht nur noch "Kante
    //                              ja/nein". Siehe die Kommentare in
    //                              webots_hal.cpp.
    virtual float getUltrasonicDistance() = 0;
    virtual bool  getEdgeFrontLeft() = 0;
    virtual bool  getEdgeFrontRight() = 0;
    virtual bool  getEdgeRearLeft() = 0;
    virtual bool  getEdgeRearRight() = 0;

    // IMU
    virtual float getYaw() = 0;    // Rotation um die Hochachse [rad]
    virtual float getGyroZ() = 0;  // Winkelgeschwindigkeit um Hochachse [rad/s]

    // Odometrie: akkumulierter Drehwinkel EINER Seite [rad], positiv =
    // vorwaerts. Webots: WebotsHAL mittelt intern die zwei PositionSensor
    // dieser Seite (das PROTO bietet sie kostenlos); real liest dies genau
    // einen Encoder pro Seite.
    // Arduino: akkumulierte Encoder-Ticks * (2*pi / Ticks-pro-Umdrehung).
    virtual float getWheelAngle(WheelId wheel) = 0;

    // Laufzeit seit Programmstart [s] - fuer zeitbasierte Zustandsuebergaenge
    virtual float getTime() = 0;

    // Weitere Hilfsfunktionen, die die aktuelle Implementierung bereits nutzt.
    virtual bool getButtonState() = 0;
    virtual bool simulateButtonPress(float duration) = 0;
    virtual void wait(float duration) = 0;
    virtual void setServoPosition(int servoId, int position) = 0;
    virtual void led(LED_COLORS color) = 0;
};

#endif  // ROBOT_HAL_H
