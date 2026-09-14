#ifndef ROBOT_LOGIC_H
#define ROBOT_LOGIC_H

#include "robot_hal.h"

// ---------------------------------------------------------------------
// Enthaelt die eigentliche Verhaltenslogik des Roboters
// (Hindernis- und Kantenvermeidung als einfacher Zustandsautomat).
//
// Kennt NUR das RobotHAL-Interface - dieser Code kann unveraendert auf
// einen Arduino uebernommen werden, sobald dort eine ArduinoHAL
// existiert, die dasselbe Interface implementiert.
// ---------------------------------------------------------------------
class RobotLogic {
public:
    explicit RobotLogic(RobotHAL& hal);

    // Einmal pro Regelzyklus aufrufen (autonomer Modus, Zustandsautomat).
    void update();

    // ------------------------------------------------------------------
    // Explizite, direkt aufrufbare Bewegungsbefehle. Diese kannst du in
    // main.cpp einzeln aufrufen, um genau nachzuvollziehen/zu testen,
    // welche Bewegung welchen Aufruf ausloest (unabhaengig vom
    // Zustandsautomaten oben).
    // ------------------------------------------------------------------
    void forward(float speed);
    void backward(float speed);
    void turnLeft(float speed);   // dreht auf der Stelle nach links
    void turnRight(float speed);  // dreht auf der Stelle nach rechts
    void stop();

    // Name des aktuellen Zustands (fuer Debug-Ausgaben in main.cpp)
    const char* getStateName() const;

private:
    RobotHAL& hal_;

    enum class State {
        FORWARD,        // normal vorwaerts fahren
        REVERSE,        // kurz zurücksetzen (nach Kantenerkennung)
        TURN,           // drehen (nach Rueckwaertsfahrt oder Hindernis)
        AVOID_OBSTACLE  // Hindernis erkannt -> drehen
    };

    State state_ = State::FORWARD;
    float stateEnteredAt_ = 0.0f;
    float turnDirection_ = -1.0f;  // +1 = links drehen, -1 = rechts drehen

    // Schwellenwerte / Parameter - an reale Sensorwerte und
    // Motor-/Getriebeuebersetzung anpassen. Die Kantensensoren liefern
    // bereits ein kalibriertes bool (siehe RobotHAL::getEdgeFrontLeft() &
    // Co.) - hier braucht es dafuer keinen eigenen Schwellenwert mehr.
    static constexpr float OBSTACLE_THRESHOLD_M = 0.22f; // Meter (> Wende-Schwenkradius ~0.16 m)
    static constexpr float CRUISE_SPEED = 6.0f;        // rad/s
    static constexpr float TURN_SPEED = 4.0f;          // rad/s
    static constexpr float REVERSE_DURATION = 0.6f;    // s
    static constexpr float TURN_DURATION = 0.8f;        // s

    bool edgeDetected() const;
    void enterState(State s);
    void driveForward();
    void driveReverse();
    void driveTurn();
};

#endif  // ROBOT_LOGIC_H
