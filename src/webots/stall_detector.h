#ifndef STALL_DETECTOR_H
#define STALL_DETECTOR_H

#include "robot_hal.h"

// ---------------------------------------------------------------------
// Stillstandserkennung: Fahrbefehl liegt an, die Raeder drehen aber nicht.
//
// Vergleicht den kommandierten Sollwert (RobotHAL::getCommandedSpeed) mit
// dem gemessenen Radwinkel (RobotHAL::getWheelAngle). Typischer Fall auf
// der realen Hardware: die HAL-Implementierung unterdrueckt eine Bewegung
// (z.B. Hindernis zu nah), ohne dass die Anwendungslogik das ueber das
// Interface erfaehrt. Erkennt ebenso einen physisch festgefahrenen Roboter.
//
// Ein Messfenster beginnt neu, sobald sich die Art des Fahrbefehls aendert
// (Vorzeichen links/rechts) oder nicht gefahren wird. So zaehlt die
// Brems-/Umschaltzeit des Motortreibers beim Fahrtrichtungswechsel nicht
// als Stillstand. Ebenso nach jeder nachgewiesenen Bewegung, Stillstand
// wird also minTime nach der letzten Radbewegung gemeldet.
//
// Kennt nur RobotHAL -> Webots und Arduino identisch.
// ---------------------------------------------------------------------
class StallDetector {
public:
    // minTime [s]:       Mindestdauer eines Fensters vor der Auswertung
    // minExpected [rad]: Mindest-Sollwinkel (Summe beider Seiten) im Fenster,
    //                    verhindert Fehlalarme bei sehr langsamen Befehlen
    // minMoved [rad]:    gemessener Winkel (Summe beider Seiten), unter dem
    //                    das Fenster als Stillstand gilt
    StallDetector(RobotHAL& hal, float minTime, float minExpected, float minMoved);

    // Neues Messfenster ab der aktuellen Radstellung.
    void restart();

    // Einmal pro Regelzyklus VOR dem neuen Fahrbefehl aufrufen: bewertet den
    // zuletzt kommandierten Sollwert, der im vorigen hal.step() anlag.
    void update();

    // true, wenn im aktuellen Fenster gefahren werden sollte, die Raeder sich
    // aber (nahezu) nicht bewegt haben.
    bool stalled() const;

private:
    RobotHAL& hal_;
    const float minTime_;
    const float minExpected_;
    const float minMoved_;

    // Fenster-Referenz
    float startTime_ = 0.0f;
    float startLeft_ = 0.0f;
    float startRight_ = 0.0f;
    int   signLeft_ = 0;
    int   signRight_ = 0;

    float lastTime_ = -1.0f;
    float expected_ = 0.0f;   // [rad] Summe |Sollwinkel| beider Seiten
    float moved_ = 0.0f;      // [rad] Summe |Istwinkel| beider Seiten

    // Laenger als das ohne update() -> Fenster ist veraltet, neu beginnen.
    static constexpr float MAX_UPDATE_GAP = 0.1f;            // [s]
    static constexpr float STANDING_COMMAND_THRESHOLD = 0.01f; // [rad/s]

    static int commandSign(float radPerSec);
};

#endif  // STALL_DETECTOR_H
