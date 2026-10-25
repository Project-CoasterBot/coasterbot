#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include "robot_hal.h"
#include "stall_detector.h"

// ---------------------------------------------------------------------
// Sicherheitskomponente (Lastenheft "Sicherheit", ML1).
//
// Wertet in jedem Zyklus die vier Kantensensoren aus. Erkennt ein Sensor
// die Tischkante (kein Boden), uebernimmt der Monitor sofort die Aktorik:
//   - Kante vorne  -> zurueck setzen
//   - Kante hinten -> vorwaerts
//   - Bewegen sich die Raeder dabei nicht (z.B. am Coaster haengen geblieben,
//     Hindernis in Ausweichrichtung), werden der Reihe nach alle Manoever
//     versucht, die die aktiven Kantensensoren erlauben: dieselbe Richtung mit
//     voller Kraft, dann Drehen rechts/links. Gerade auf die Kante zu nie.
//   - Kante vorne UND hinten -> nur noch Drehen, sonst anhalten (eingeklemmt)
//   - alle Manoever MAX_ROUNDS mal ohne Radbewegung -> festgefahren (stuck()),
//     anhalten, bis keine Kante mehr erkannt wird oder reset()
// Nach erfolgreichem Ausweichen kurze Pause, dann Freigabe an die Navigation.
//
// Die erlaubten Richtungen entsprechen der Sperrlogik der realen HAL
// (HardwareImplementationHAL::step): vorwaerts nicht bei Kante vorne,
// rueckwaerts nicht bei Kante hinten, Drehen nur, wenn es jeden aktiven
// Sensor zurueck auf den Tisch schwenkt.
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
    // Kein Ausweichmanoever bewegt die Raeder mehr -> steht, bis die Kante frei ist oder reset().
    bool stuck() const { return state_ == State::STUCK; }
    // Zurueck auf CLEAR, z.B. wenn der Nutzer die Fahrt abbricht. Bei aktiver Kante greift der
    // Monitor im naechsten Zyklus erneut, mit allen Manoevern von vorn.
    void reset();
    bool edgeEventThisTick() const { return edgeEvent_; }   // steigende Flanke
    bool frontEdge() const;
    bool rearEdge() const;
    const char* stateName() const;
    int  eventCount() const { return eventCount_; }

private:
    enum class State { CLEAR, EVADE, PAUSE, PINNED, BLOCKED, STUCK };

    struct Maneuver {
        float left;      // [rad/s]
        float right;     // [rad/s]
        float duration;  // [s]
        const char* name;
    };
    static constexpr int MAX_MANEUVERS = 6;

    RobotHAL& hal_;
    State  state_ = State::CLEAR;
    float  stateEnteredAt_ = 0.0f;
    bool   edgeEvent_ = false;
    int    eventCount_ = 0;
    Maneuver maneuver_ {0.0f, 0.0f, 0.0f, ""};
    int    attempt_ = 0;        // Index in der Manoeverliste des aktuellen Ausweichens
    int    round_ = 0;          // wie oft alle Manoever ohne Radbewegung durchlaufen wurden
    StallDetector evadeStall_;

    // Kantensensoren liefern bereits ein kalibriertes bool (siehe
    // RobotHAL::getEdgeFrontLeft() & Co.) - kein eigener Schwellenwert noetig.
    static constexpr float EVADE_RADS     = 3.0f;    // rad/s (~0.10 m/s)
    static constexpr float EVADE_STRONG_RADS = 6.0f; // rad/s, ueber der Nenngeschwindigkeit -> volle PWM
    static constexpr float EVADE_TURN_RADS = 3.0f;   // rad/s je Seite beim Drehen
    static constexpr float EVADE_TIME     = 0.7f;    // s
    static constexpr float EVADE_TURN_TIME = 1.0f;   // s
    static constexpr float PAUSE_TIME     = 0.3f;    // s
    static constexpr float BLOCKED_RETRY_TIME = 0.5f; // s bis zum naechsten Manoever
    static constexpr int   MAX_ROUNDS     = 2;       // Durchlaeufe aller Manoever bis stuck()

    // Stillstand waehrend EVADE: Fenster beginnt einen Zyklus nach Eintritt
    // (erst dann liegt der Ausweichbefehl an), daher etwas unter EVADE_TIME.
    // Real sind ~0.3 s davon Brems-/Umschaltzeit des Motortreibers.
    static constexpr float EVADE_STALL_MIN_TIME     = 0.6f;  // s
    static constexpr float EVADE_STALL_MIN_EXPECTED = 1.0f;  // rad, beide Seiten
    static constexpr float EVADE_STALL_MIN_MOVED    = 0.3f;  // rad, ~2 Encoderflanken

    bool anyFront() const;
    bool anyRear() const;
    void drive(float left, float right);
    // Erlaubte Manoever fuer die aktuellen Kantensensoren, das naheliegendste zuerst.
    int maneuvers(Maneuver (&out)[MAX_MANEUVERS]) const;
    // Naechstes Manoever starten (attempt_), false wenn keins erlaubt ist.
    bool enterEvade(float t);
};

#endif  // SAFETY_MONITOR_H
