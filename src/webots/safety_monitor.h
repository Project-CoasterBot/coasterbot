#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include "robot_hal.h"

// ---------------------------------------------------------------------
// Sicherheitskomponente (Lastenheft "Sicherheit", ML1).
//
// Wertet in jedem Zyklus die vier Kantensensoren aus. Erkennt ein Sensor
// die Tischkante (kein Boden), uebernimmt der Monitor sofort die Aktorik:
//   - Kante vorne  -> zurueck setzen
//   - Kante hinten -> vorwaerts
//   - Kante vorne UND hinten -> anhalten (eingeklemmt)
// Danach kurze Pause, dann Freigabe zurueck an die Navigation.
//
// Solange overriding() true ist, DARF die Navigation keine eigenen
// Fahrbefehle geben. Kennt nur RobotHAL -> Arduino-portabel.
// ---------------------------------------------------------------------
class SafetyMonitor {
public:
    explicit SafetyMonitor(RobotHAL& hal);

    // Einmal pro Regelzyklus VOR der Navigation aufrufen.
    // Rueckgabe: true, wenn die Sicherheitskomponente die Aktorik haelt.
    bool update();

    bool overriding() const { return state_ != State::CLEAR; }
    bool edgeEventThisTick() const { return edgeEvent_; }   // steigende Flanke
    bool frontEdge() const;
    bool rearEdge() const;
    const char* stateName() const;
    int  eventCount() const { return eventCount_; }

private:
    enum class State { CLEAR, EVADE, PAUSE, PINNED };

    RobotHAL& hal_;
    State  state_ = State::CLEAR;
    float  stateEnteredAt_ = 0.0f;
    bool   edgeEvent_ = false;
    int    eventCount_ = 0;
    float  evadeDir_ = -1.0f;   // -1 = rueckwaerts, +1 = vorwaerts

    // Kantensensoren liefern bereits ein kalibriertes bool (siehe
    // RobotHAL::getEdgeFrontLeft() & Co.) - kein eigener Schwellenwert noetig.
    static constexpr float EVADE_RADS     = 3.0f;    // rad/s (~0.10 m/s)
    static constexpr float EVADE_TIME     = 0.7f;    // s
    static constexpr float PAUSE_TIME     = 0.3f;    // s

    bool anyFront() const;
    bool anyRear() const;
    void drive(float radsPerSide);
};

#endif  // SAFETY_MONITOR_H
