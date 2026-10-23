#ifndef COASTERBOT_FUNCTIONS_H
#define COASTERBOT_FUNCTIONS_H

#include "robot_logic.h"
#include "pose_estimator.h"
#include "safety_monitor.h"
#include "occupancy_grid.h"
#include "dstar_lite.h"
#include "stall_detector.h"
#include <array>
#include <memory>
#include <vector>

struct Vec4Coasterbot {
    float x = 0.0f;
    float y = 0.0f;
    bool operator==(const Vec4Coasterbot& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec4Coasterbot& o) const { return !(*this == o); }
};


class CoasterbotFunctions : public RobotLogic {
    public:
    explicit CoasterbotFunctions(RobotHAL& hal, PoseEstimator& pose, SafetyMonitor& safety);

    // Einmal pro Regelzyklus aufrufen (autonomer Modus, Zustandsautomat).
    void update();


    void runLearnTable();
    void runNavigateTargets();
    void navigateTo(Vec2 destination, float speed);
    void navigateToWithObstacleAvoidance(Vec2 destination, float speed);
    bool isTableLearned() const { return this->tableLearned; }
    void setTableLearned(bool learned) { this->tableLearned = learned; }
    bool buttonPressed() const { return hal_.getButtonState(); }
    bool safetyOverriding() const { return safety_.overriding(); }
    bool pathExists() const { return dstar_ && dstar_->pathExists(); }
    bool runSpendCoasters();
    bool spendCoaster(int coasterIndex);
    void resetSpendCoasterSequence();
    bool runGetCoasters();
    bool getCoaster(int coasterIndex);
    bool runCollectCoasters() { return runGetCoasters(); }
    bool goToCenter();
    // Laufende Ausgabe/Aufnahme/Rueckfahrt abbrechen und anhalten. Tischvermessung und Karte bleiben,
    // der naechste Start beginnt die Routine von vorn (Navigationsstopps sind zurueckgesetzt).
    void abortRoutine();
    bool coasterRoutineFailed() const { return this->coasterRoutineFailed_; }
    void runDriveTest();
    bool blocklessWait(float duration);
    void led(RobotHAL::LED_COLORS color) { hal_.led(color); }

    // Was die Navigation im letzten Zyklus getan hat - Diagnose ohne serielle Konsole.
    enum class NavState : uint8_t {
        IDLE,           // noch nicht navigiert
        DRIVING,        // faehrt vorwaerts Richtung Wegpunkt
        ALIGNING,       // dreht auf der Stelle zum Wegpunkt
        TURN_CLEARANCE, // vor dem Drehen gerade von der nahen Tischkante wegfahren
        FINAL_APPROACH, // Endanflug, nur vor-/rueckwaerts
        REACHED,        // Ziel innerhalb ACCEPTED_BIAS (auch waehrend der Servos)
        NO_PATH,        // D* findet keinen Weg -> direkt anfahren bzw. links drehend suchen
        ESCAPE_TURN,    // Startzelle belegt und Ultraschall sieht Hindernis -> drehen
        BLOCKED,        // steht: Start/Ziel ungueltig oder belegt, Kante aktiv, Stall-Neuplanung
        SAFETY,         // SafetyMonitor weicht der Kante aus
        EDGE_LATCHED,   // 3 Kantenunterbrechungen -> Navigation dauerhaft gestoppt
        STALL_LATCHED,  // 3x keine Radbewegung -> Navigation dauerhaft gestoppt
        TURN_OVERRUN,   // Ausrichten hat > 1 Umdrehung gedauert -> Navigation gestoppt
        HEADING_STUCK,  // Ausrichten ohne Aenderung des Kurses (Gyro/Pose) -> Navigation gestoppt
        SPEND_SIGNAL,   // statt Coaster ablegen: LED-Signal (Servos deaktiviert)
        COLLECT_SIGNAL  // statt Coaster aufnehmen: LED-Signal (Servos deaktiviert)
    };
    NavState navState() const { return navState_; }
    // LED-Farbe zum Navigationszustand:
    //   gruen = fahren, cyan = ausrichten, blau = vor dem Drehen von der Kante weg, gelb = Endanflug, weiss = Ziel erreicht/Servos,
    //   lila = kein Pfad (Suchdrehung), pink = Fluchtdrehung (Startzelle belegt),
    //   orange blinkend = blockiert/steht, orange = Kantenausweichen,
    //   rot blinkend = Kanten-Stopp, lila blinkend = Stall-Stopp,
    //   cyan blinkend = Drehung > 1 Umdrehung, gelb blinkend = Kurs aendert sich beim Drehen nicht,
    //   gruen schnell blinkend = hier wuerde abgelegt, blau schnell blinkend = hier wuerde aufgenommen,
    //   aus = idle
    RobotHAL::LED_COLORS navLedColor() const;
    static const char* navStateName(NavState state);
    std::vector<Vec2> getCoasterPositions(int coasterIndex);

    private:
    RobotHAL& hal_;
    PoseEstimator& pose_;
    SafetyMonitor& safety_;
    std::unique_ptr<OccupancyGrid> grid_;
    std::unique_ptr<DStarLite> dstar_;

    bool tableLearned = false;
    NavState navState_ = NavState::IDLE;
    float tableWidth_ = 0.0f;
    float tableHeight_ = 0.0f;
    Vec2 navigationTarget_{};
    Vec2 navigationWaypoint_{};
    Vec2 navigationSegmentStart_{};
    Cell navigationPreviousCell_{};
    float navigationSegmentHeading_ = 0.0f;   // [rad]
    float navigationSegmentLength_ = 0.0f;    // [m]
    bool navigationTargetInitialized_ = false;
    bool navigationPlannerInitialized_ = false;
    bool navigationAligning_ = false;
    bool navigationWaypointInitialized_ = false;
    bool navigationWaypointIsFinal_ = false;
    int edgeInterruptions_ = 0;
    bool edgeStopLatched_ = false;
    bool edgeWasActive_ = false;
    bool edgeReroutePending_ = false;
    StallDetector navigationStall_;
    int stallInterruptions_ = 0;
    bool stallStopLatched_ = false;
    // Drehwaechter fuer das Ausrichten (siehe ALIGN_MAX_TURN / ALIGN_STUCK_TIME)
    bool alignWatchActive_ = false;
    bool turnClearanceActive_ = false;  // faehrt vor dem Drehen von der Kante weg (nur fuers Log)
    float alignWatchStartedAt_ = 0.0f;
    float alignWatchPrevTheta_ = 0.0f;
    float alignWatchTurned_ = 0.0f;   // [rad] Betrag, aufsummiert
    float alignWatchWheelTurned_ = 0.0f;  // [rad] Drehung aus den Radwegen, Betrag, aufsummiert
    float alignWatchPrevLeft_ = 0.0f;
    float alignWatchPrevRight_ = 0.0f;
    bool alignStopLatched_ = false;
    NavState alignStopState_ = NavState::TURN_OVERRUN;
    bool navigationPointSettling_ = false;
    float navigationPointSettleStartedAt_ = 0.0f;
    bool centerSettling_ = false;
    float centerSettleStartedAt_ = 0.0f;
    size_t navigationPointIndex_ = 0;
    bool navigationSequenceComplete_ = false;
    bool coasterRoutineActive_ = false;
    bool coasterRoutineFailed_ = false;
    int coasterRoutineIndex_ = 0;
    enum class CoasterRoutinePhase {
        TRAVEL,
        SETTLE,
        SPEND_WAIT_LIFTER,
        SPEND_WAIT_SPENDER,
        GET_WAIT_SPENDER_OUT,
        GET_WAIT_SPENDER_HOME
    };
    CoasterRoutinePhase coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
    float coasterRoutinePhaseStartedAt_ = 0.0f;
    float robotBoundingBoxWidth_ = 0.2f;  // [m]
    float robotBoundingBoxLength_ = 0.2f; // [m]
    float obstacleRecoveryDirection_ = 1.0f;
    float lastTimer_ = 0.0f;
    bool resetLastTimer_ = true;

    void navigateToInternal(Vec2 target, float speed, bool useRobotBoundingBox);
    void rebuildNavigationGrid(bool preserveDynamicObstacles = true);
    // Robotergrosses Hindernis direkt vor (direction = +1) bzw. hinter (-1) dem Roboter eintragen.
    void markBlockedArea(float direction);
    void correctPoseAtEdge(bool frontEdge);
    void calcCoasterPositions();
    static constexpr int MAX_COASTERS = 4;
    static constexpr float _HALF_PI = 1.57079632679489661923f;
    // Toleranzen fuer die reale Hardware: ein Encoderschritt sind ~5 mm, jeder Wechsel der
    // Bewegungsart kostet ~300 ms Brems-/Totzeit (der Roboter laeuft so lange nach). Die frueheren
    // Webots-Werte (5 mm, 0.5 Grad) sind damit nicht erreichbar -> Ueberschwingen, Drehen auf der Stelle.
    static constexpr float ACCEPTED_BIAS = 0.03f;                    // [m]
    static constexpr float NAVIGATION_DISTANCE_GAIN = 20.0f;
    static constexpr float NAVIGATION_HEADING_GAIN = 3.0f;
    static constexpr float NAVIGATION_TURN_ENTER_ERROR = 0.10f;      // [rad] ~6 Grad
    static constexpr float NAVIGATION_TURN_EXIT_ERROR = 0.035f;      // [rad] ~2 Grad
    // Mindestsollwerte: darunter skaliert die HAL die PWM so weit herunter, dass die Motoren nicht
    // mehr anlaufen. 0.5 rad/s Drehen entspricht dem Feindrehen beim Vermessen.
    static constexpr float NAVIGATION_MIN_TURN_SPEED = 0.5f;         // [rad/s]
    static constexpr float NAVIGATION_MIN_DRIVE_SPEED = 2.5f;        // [rad/s]
    // Hoechstens so schnell wie die 90-Grad-Drehung beim Vermessen (speed * 0.5): volle Dreh-PWM
    // zieht Stromspitzen, bei denen Kantensensoren, Servos und Gyro gestoert wurden.
    static constexpr float NAVIGATION_MAX_TURN_SPEED = 2.5f;         // [rad/s]
    // Endanflug: liegt das Ziel nahe und (fast) auf der Fahrtlinie, wird nur noch vor-/rueckwaerts
    // korrigiert. Sonst dreht der Roboter nach jedem Ueberfahren des Ziels um 180 Grad.
    static constexpr float NAVIGATION_FINAL_APPROACH_RADIUS = 0.10f; // [m]
    static constexpr float NAVIGATION_FINAL_APPROACH_LATERAL = 0.02f; // [m]
    // Vorausschauendes Ende der Drehung: Der Motortreiber bremst 200 ms + Totzeit, der Roboter
    // dreht so lange nach. Das Ausrichten endet daher schon um gyroRate * Vorhalt vor dem Ziel.
    static constexpr float NAVIGATION_TURN_BRAKE_LEAD = 0.12f;       // [s]
    // Obergrenze des Vorhalts: Austritt (EXIT + Vorhalt) muss deutlich unter dem Eintritt bleiben,
    // sonst endet das Ausrichten schon jenseits der Eintrittsschwelle und Fahren/Ausrichten pendeln.
    // Ohne Grenze reichen ~31 Grad/s Drehrate (oder eine Spitze im ungefilterten Gyro) dafuer aus.
    static constexpr float NAVIGATION_TURN_MAX_BRAKE_LEAD = 0.03f;   // [rad] ~1.7 Grad
    // Drehwaechter: Ausrichten auf einen Kurs braucht hoechstens eine halbe Umdrehung. Mehr als
    // eine Umdrehung heisst, der Sollkurs laeuft mit oder die Kursschaetzung stimmt nicht.
    static constexpr float ALIGN_MAX_TURN = 2.0f * 3.14159265f * 1.25f; // [rad]
    static constexpr float ALIGN_STUCK_TIME = 6.0f;                  // [s]
    static constexpr float ALIGN_STUCK_MIN_TURN = 0.5f;              // [rad]
    // Zweiter Waechter unabhaengig vom Gyro: Drehung aus den Radwegen. Schlupf beim Drehen auf der
    // Stelle laesst die Raeder mehr Weg machen als der Roboter dreht, daher die hoehere Grenze.
    static constexpr float ALIGN_MAX_WHEEL_TURN = 2.0f * 3.14159265f * 2.5f; // [rad]
    // Servos voruebergehend aus: am Coasterpunkt nur LED-Signal, bis die Navigation steht.
    static constexpr bool SERVO_ACTIONS_ENABLED = false;
    static constexpr float COASTER_SIGNAL_TIME = 3.0f;               // [s]
    static constexpr float CENTER_CAPTURE_RADIUS = ACCEPTED_BIAS;
    static constexpr float CENTER_SETTLE_TIME = 0.5f;
    static constexpr float NAVIGATION_GRID_RESOLUTION = 0.05f;
    static constexpr float TABLE_EDGE_INFLATION = 0.025f;
    // Abstand der Coaster von der vermessenen Grenze. Die Grenze ist bereits die Lage des
    // Robotermittelpunkts beim Ausloesen eines Kantensensors (senkrecht angefahren), der Abstand
    // Mitte-Sensor steckt also schon darin. Der Rest deckt Nachlauf und schraeges Anfahren ab.
    static constexpr float COASTER_EDGE_MARGIN = 0.15f;              // [m]
    // Drehen auf der Stelle schwenkt die Ecken mit den Kantensensoren nach aussen (bis zur Diagonalen
    // statt nur bis zur Front). Liegt eine Kante naeher als TURN_EDGE_CLEARANCE und steht der Roboter mit
    // Front oder Heck zu ihr, faehrt er vor einer Drehung > TURN_CLEARANCE_MIN_ANGLE erst gerade weg.
    // Bewusst groesser als COASTER_EDGE_MARGIN: am Coaster wird also vor jeder Drehung zurueckgesetzt,
    // das faengt auch Fehler der Pose ab.
    static constexpr float TURN_EDGE_CLEARANCE = 0.22f;              // [m]
    // Korrektur der Pose beim Ausloesen eines Kantensensors (correctPoseAtEdge): Die Kartengrenze ist
    // die Lage des Mittelpunkts beim senkrechten Anfahren. Beim Drehen loest eine Ecke etwas weiter
    // innen aus (Diagonale statt Front), daher dort ein kleiner Abstand nach innen.
    static constexpr float EDGE_CORRECTION_TURN_DEPTH = 0.02f;       // [m]
    static constexpr float EDGE_CORRECTION_MAX = 0.25f;              // [m], groesser = andere Kante/Unsinn
    static constexpr float EDGE_CORRECTION_MIN_FACING = 0.5f;        // cos(60 Grad)
    static constexpr float EDGE_CORRECTION_TURN_RATE = 0.15f;        // [rad/s], darueber gilt "drehend"
    static constexpr float TURN_CLEARANCE_MIN_ANGLE = 0.26f;         // [rad] ~15 Grad
    static constexpr float TURN_CLEARANCE_MIN_FACING = 0.5f;         // cos(60 Grad): Front/Heck zeigt zur Kante
    // Mindestabstand des direkten Wegs zu belegten Zellen. Nicht mehr der halbe Roboter: Kartengrenze
    // (s.o.) und Ultraschall-Hindernisse (halbe Bounding Box + Padding) enthalten ihn schon, doppelt
    // gezaehlt galt jeder Weg zu randnahen Coastern als blockiert.
    // Knapp unter einer Zellbreite: die Nachbarzelle einer belegten Zelle bleibt befahrbar (Abstandsfeld = 0.05).
    static constexpr float NAVIGATION_PATH_CLEARANCE = 0.04f;        // [m]
    static constexpr float ULTRASONIC_OBSTACLE_PADDING = 0.02f;
    // Nur nahe Treffer kartieren: weiter entfernt sieht der Sensor ueber die Tischkante hinaus
    // (Wand, Personen) oder Echos der Tischplatte.
    static constexpr float ULTRASONIC_MAP_RANGE = 0.5f;
    static constexpr float ULTRASONIC_TURN_DISTANCE = 0.35f;
    // Stillstandserkennung: Fahrbefehl ohne Radbewegung (z.B. von der realen HAL unterdrueckt).
    // MIN_TIME deutlich ueber der Brems-/Umschaltzeit des realen Motortreibers (~0.3 s).
    static constexpr float NAVIGATION_STALL_MIN_TIME = 1.0f;      // [s]
    static constexpr float NAVIGATION_STALL_MIN_EXPECTED = 1.0f;  // [rad], beide Seiten
    static constexpr float NAVIGATION_STALL_MIN_MOVED = 0.3f;     // [rad], ~2 Encoderflanken
    static constexpr int MAX_STALL_INTERRUPTIONS = 3;
    std::array<Vec2, MAX_COASTERS> coasterPositions;

    // Servowinkel [deg] wie in der Webots-Statemachine (Dennis Aufbau, siehe boardconfig.h):
    // Heber 180 = ganz eingefahren, Ausschub 90 = langer Arm zeigt ins Zentrum.
    const std::array<uint8_t, MAX_COASTERS> SERVO_POS_LIFTER = { 100, 60, 40, 0 }; // Heber je Coaster
    const std::array<uint8_t, 3> SERVO_POS_SPENDER = { 0, 90, 180 }; // Ausschub: auswerfen, home, beiseite
    // Rampen wie im Hardwaretest (CoasterMechanismus): ohne Rampe springt der Servo mit voller
    // Geschwindigkeit, der Kurbeltrieb des Hebers verklemmt bzw. die Versorgung bricht ein.
    // Kuerzer als die 2 s Wartezeit je Schritt in spendCoaster()/getCoaster(), damit der Servo
    // vor dem naechsten Schritt sicher angekommen ist.
    static constexpr unsigned long SERVO_LIFTER_RAMP_MS = 1500;
    static constexpr unsigned long SERVO_SPENDER_RAMP_MS = 1000;
    uint8_t coastersSpend = 0;

    enum class ServoId : uint8_t { LIFTER, SPENDER };

    enum class SpendingStates : uint8_t {
        IDLE,
        SET_LIFTER_DOWN,
        SET_SPENDER_SPEND,
        SET_SPENDER_HOME,
        SET_LIFTER_UP
    };
    SpendingStates coasterSpending_ = SpendingStates::IDLE;

    enum class CollectingStates : uint8_t {
        IDLE,
        SET_SPENDER_ASIDE,
        SET_LIFTER_DOWN,
        SET_SPENDER_HOME,
        SET_LIFTER_UP
    };
    CollectingStates coasterCollecting_ = CollectingStates::IDLE;

};

#endif  // COASTERBOT_FUNCTIONS_H

