#ifndef FAULT_MONITOR_H
#define FAULT_MONITOR_H

#include "robot_hal.h"

// ---------------------------------------------------------------------
// Fehlerueberwachung (Testkonzept ST-SIM-005 Sensorausfall, ST-SIM-006
// Aktorfehler; Lastenheft SAF-002, MON-001, MON-005).
//
// Prueft in jedem Regelzyklus:
//   - Ultraschall: liefert der Sensor einen physikalisch unmoeglichen Wert
//     (ausserhalb [0, ULTRASONIC_MAX_VALID], siehe RobotHAL-Kontrakt und
//     Coasterbot.proto-lookupTable), gilt er als ausgefallen/"nicht
//     verfuegbar".
//   - Antrieb: kommandierte vs. tatsaechlich gemessene Radgeschwindigkeit
//     (aus der Aenderung von RobotHAL::getWheelAngle()) je Seite. Weicht
//     eine Seite trotz deutlichem Kommando laenger als FAULT_DEBOUNCE_TIME
//     ab, gilt der Motor dieser Seite als ausgefallen. Die Debounce-Zeit
//     verhindert Fehlalarm durch die normale Beschleunigungsrampe.
//
// Bestaetigt sich ein Fehler, uebernimmt der Monitor sofort die Aktorik
// (drive(0)) und haelt sie - wie SafetyMonitor kennt er nur RobotHAL und
// ist damit Arduino-portabel. Anders als SafetyMonitor ist der sichere
// Zustand hier VERRIEGELT: ein Sensor-/Aktorausfall behebt sich nicht von
// selbst, es gibt also keine automatische Rueckkehr nach NORMAL.
// ---------------------------------------------------------------------
class FaultMonitor {
public:
    explicit FaultMonitor(RobotHAL& hal);

    // Einmal pro Regelzyklus aufrufen, NACHDEM der Aufrufer die gewuenschten
    // Radgeschwindigkeiten bestimmt hat (Vergleichswert fuer die
    // Aktor-Erkennung) - siehe RobotHAL::setLeftSpeed/setRightSpeed.
    // Rueckgabe: true, wenn der Monitor die Aktorik haelt (uebernimmt
    // drive(0) selbst). Der Aufrufer darf dann keine eigenen Fahrbefehle
    // mehr geben, analog SafetyMonitor::update().
    bool update(float commandedLeft, float commandedRight);

    bool overriding() const { return state_ != State::NORMAL; }
    bool sensorFault() const { return state_ == State::SENSOR_FAULT; }
    bool actuatorFault() const { return state_ == State::ACTUATOR_FAULT; }
    bool leftDeviated() const { return leftDeviated_; }
    bool rightDeviated() const { return rightDeviated_; }
    const char* stateName() const;

private:
    enum class State { NORMAL, SENSOR_FAULT, ACTUATOR_FAULT };

    RobotHAL& hal_;
    State state_ = State::NORMAL;

    // Ultraschall-Kontrakt (RobotHAL::getUltrasonicDistance() /
    // Coasterbot.proto-lookupTable "0,02 0,02 0 / 4 4 0,02"): der Nennbereich
    // ist [0,02; 4] m, aber der letzte Eintrag traegt bereits 2% Rauschen
    // (Gauss-verteilt um 4 m) - einzelne Messungen liegen also legitim auch
    // etwas UEBER 4 m (beobachtet: bis ~4,03 m). ULTRASONIC_MAX_VALID braucht
    // daher deutlich Marge zu diesem Rauschen, sonst loest die normale
    // Sensorstreuung faelschlich SENSOR_FAULT aus. Der injizierte
    // "nicht verfuegbar"-Wert (WebotsHAL: -1 m) bleibt davon unberuehrt, weit
    // ausserhalb jeder realistischen Streuung.
    static constexpr float ULTRASONIC_MAX_VALID = 4.5f;

    static constexpr float MIN_COMMANDED_RADS   = 0.5f;  // unterhalb: kein Fahrbefehl, keine Pruefung
    static constexpr float SPEED_DEVIATION_RADS = 1.0f;  // rad/s, ab hier gilt eine Seite als abweichend
    static constexpr float FAULT_DEBOUNCE_TIME  = 0.3f;  // s, so lange muss die Abweichung anhalten

    bool  actuatorInit_ = false;
    float prevAngleLeft_ = 0.0f, prevAngleRight_ = 0.0f;
    float prevTime_ = 0.0f;
    float deviatingSince_ = -1.0f;
    bool  leftDeviated_ = false, rightDeviated_ = false;

    void drive(float left, float right);
    bool ultrasonicImplausible() const;
    bool actuatorDeviating(float commandedLeft, float commandedRight);
};

#endif  // FAULT_MONITOR_H
