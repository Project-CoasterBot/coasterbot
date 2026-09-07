#ifndef DWA_PLANNER_H
#define DWA_PLANNER_H

#include "robot_hal.h"
#include "pose_estimator.h"
#include "occupancy_grid.h"

#include <cstddef>
#include <vector>

// ---------------------------------------------------------------------
// Dynamic Window Approach (DWA) - lokale Bewegungsplanung.
//
// Ersetzt den fruehen PathFollower. DWA folgt einem globalen A*-Pfad
// (Carrot-Punkt entlang des Pfades) und waehlt in jedem Zyklus das
// Geschwindigkeitspaar (v, omega) aus dem "dynamischen Fenster" (durch
// Beschleunigungsgrenzen erreichbare Werte), das eine gewichtete
// Bewertung aus
//   - Kurs zum Carrot-Punkt
//   - Abstand zu Hindernissen entlang der simulierten Trajektorie
//   - Fahrgeschwindigkeit
// maximiert. Hindernisse kommen aus dem Distanzfeld des Grids UND aus
// dem aktuellen Ultraschallwert (unkartierte / dynamische Hindernisse).
//
// Kennt nur RobotHAL / PoseEstimator / OccupancyGrid -> Arduino-portabel.
// ---------------------------------------------------------------------
class DwaPlanner {
public:
    DwaPlanner(RobotHAL& hal, PoseEstimator& pose, OccupancyGrid& grid);

    void setPath(const std::vector<Vec2>& waypoints);
    void update();                               // einmal pro Regelzyklus
    bool reachedGoal() const { return done_; }

    // Ab welchem Ultraschall-Abstand ein virtuelles Hindernis beruecksichtigt
    // wird. Klein setzen, wenn eine uebergeordnete Ebene Hindernisse selbst
    // in die Karte eintraegt (dann ist DWA nur noch Nahbereich-Sicherung).
    void setUltrasonicRange(float r) { usRange_ = r; }

    float commandedV() const { return curV_; }   // [m/s]
    float commandedW() const { return curW_; }   // [rad/s]
    std::size_t carrotIndex() const { return carrotIdx_; }

private:
    RobotHAL&      hal_;
    PoseEstimator& pose_;
    OccupancyGrid& grid_;
    std::vector<Vec2> path_;
    std::size_t    carrotIdx_ = 0;
    bool           done_ = false;
    float          curV_ = 0.0f, curW_ = 0.0f;
    float          prevTime_ = -1.0f;
    float          usRange_ = 1.0f;   // Ultraschall-Beruecksichtigung [m]

    // Kinematik-/Regelgrenzen
    static constexpr float MAX_V = 0.18f;   // [m/s]
    static constexpr float MAX_W = 2.6f;    // [rad/s]
    static constexpr float ACC_V = 0.5f;    // [m/s^2]
    static constexpr float ACC_W = 6.0f;    // [rad/s^2]
    static constexpr int   V_SAMPLES = 6;
    static constexpr int   W_SAMPLES = 25;

    // Trajektorien-Vorausschau
    static constexpr float SIM_TIME = 1.5f; // [s]
    static constexpr float SIM_DT   = 0.1f; // [s]

    static constexpr float ROBOT_RADIUS = 0.14f;  // [m] fuer rohe (Ultraschall-) Hindernisse
    static constexpr float GRID_MARGIN  = 0.03f;  // [m] Restpuffer (Grid ist schon aufgeblaeht)
    static constexpr float CLEAR_OK     = 0.15f;  // [m] darueber zaehlt "frei genug"
    static constexpr float CARROT_DIST  = 0.30f;  // [m] Lookahead entlang Pfad
    static constexpr float GOAL_TOL     = 0.05f;  // [m]

    // Bewertungsgewichte
    static constexpr float W_HEADING  = 1.0f;   // Ausrichtung zum Carrot
    static constexpr float W_PROGRESS = 2.0f;   // Annaeherung an den Carrot
    static constexpr float W_CLEAR    = 1.5f;   // Hindernisabstand
    static constexpr float W_VEL      = 0.3f;   // Fahrgeschwindigkeit

    static constexpr float WHEEL_RADIUS = 0.035f;
    static constexpr float TRACK_WIDTH  = 0.202f;

    Vec2  carrot(float px, float py);
    float trajectoryClearance(float px, float py, float th, float v, float w,
                              const Vec2& usObs, bool haveUs) const;
    void  driveVW(float v, float w);
};

#endif  // DWA_PLANNER_H
