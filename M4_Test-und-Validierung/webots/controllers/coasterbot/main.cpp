#include "webots_hal.h"
#include "robot_logic.h"
#include "pose_estimator.h"
#include "occupancy_grid.h"
#include "astar.h"
#include "dstar_lite.h"
#include "dwa_planner.h"
#include "safety_monitor.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------
// MODE steuert, was main() tut. So ist immer klar nachvollziehbar,
// welche Bewegung gerade WARUM passiert:
//
//   MODE_WHEEL_TEST : je Seite 2s die Raeder drehen (links, dann rechts),
//                      Konsole zeigt an welche -> zeigt, ob eine Seite
//                      falsch/gar nicht dreht. Einzelne Raeder sind nicht
//                      ansteuerbar (Motoren je Seite gekoppelt).
//   MODE_MANUAL     : feste Befehlsfolge ueber die expliziten
//                      RobotLogic-Methoden (forward/turnLeft/...)
//   MODE_AUTONOMOUS : der Zustandsautomat aus robot_logic.cpp
//   MODE_ODOMETRY_TEST : definierte Fahrt (gerade + Drehung) und dabei
//                      die geschaetzte Pose ausgeben -> Odometrie pruefen
//   MODE_NAVIGATE   : A*-Pfad auf bekannter Karte planen, mit DWA abfahren
//                      (nur mit worlds/coasterbot-astar.wbt sinnvoll)
//   MODE_NAVIGATE_DSTAR : wie oben, aber der Planer ist D* Lite; ein per
//                      Ultraschall entdecktes, unkartiertes Hindernis loest
//                      eine inkrementelle Umplanung aus
//                      (nur mit worlds/coasterbot-dstar.wbt sinnvoll)
//   MODE_SIM_TEST   : kombinierter Simulationstest auf dem Tisch -
//                      Wegfindung + Hindernisvermeidung + Kantenerkennung,
//                      Bewertung nach Testkonzept ST-SIM-001/002/003/004
//                      (nur mit worlds/coasterbot-simtest.wbt sinnvoll)
// ---------------------------------------------------------------------
enum Mode { MODE_WHEEL_TEST, MODE_MANUAL, MODE_AUTONOMOUS, MODE_ODOMETRY_TEST,
            MODE_NAVIGATE, MODE_NAVIGATE_DSTAR, MODE_SIM_TEST };
static const Mode MODE = MODE_SIM_TEST;  // <-- hier umschalten

static void printPose(const char* tag, float t, const PoseEstimator& pose) {
    std::cout << tag << " t=" << t << "s  pose x=" << pose.getX()
              << " y=" << pose.getY()
              << " theta=" << pose.getTheta() * 180.0f / static_cast<float>(M_PI)
              << " deg  odo=" << pose.getOdometer() << " m" << std::endl;
}

static void runWheelTest(WebotsHAL& hal) {
    // Je Seite 2s drehen: die zwei Raeder einer Seite sind gekoppelt und
    // laufen zusammen (wie beim realen Coasterbot).
    const char* names[2] = {"LINKS", "RECHTS"};
    const float TEST_SPEED = 4.0f;   // rad/s
    const float PHASE_TIME = 2.0f;   // s pro Seite

    int active = -1;
    while (hal.step()) {
        const float t = hal.getTime();
        const int phase = static_cast<int>(t / PHASE_TIME) % 2;
        if (phase != active) {
            active = phase;
            std::cout << "[WHEEL_TEST] t=" << t << "s -> teste Seite "
                      << names[active] << " (" << TEST_SPEED << " rad/s, beide Raeder)"
                      << std::endl;
        }
        hal.setLeftSpeed(active == 0 ? TEST_SPEED : 0.0f);
        hal.setRightSpeed(active == 1 ? TEST_SPEED : 0.0f);

        static float lastEnc = -1.0f;
        if (t - lastEnc >= 1.0f) {
            // Nur ein Encoderwert je Seite - die reale Elektronik hat
            // ebenfalls nur einen Encoder je Seite (siehe RobotHAL::WheelId).
            std::cout << "           Encoder [L " << hal.getWheelAngle(WHEEL_LEFT)
                      << "  R " << hal.getWheelAngle(WHEEL_RIGHT) << "]" << std::endl;
            lastEnc = t;
        }
    }
}

static void runManual(RobotLogic& logic, WebotsHAL& hal) {
    // Feste, nachvollziehbare Befehlsfolge: 3s vor, 2s Stopp, 2s links
    // drehen, 2s Stopp, 3s vor. Konsole zeigt jeden Wechsel.
    int lastPhase = -1;
    while (hal.step()) {
        float t = hal.getTime();
        int phase = (t < 3.0f) ? 0 : (t < 5.0f) ? 1 : (t < 7.0f) ? 2 : (t < 9.0f) ? 3 : 4;
        if (phase != lastPhase) {
            std::cout << "[MANUAL] t=" << t << "s -> Phase " << phase << std::endl;
            lastPhase = phase;
        }
        switch (phase) {
            case 0: logic.forward(6.0f); break;
            case 1: logic.stop(); break;
            case 2: logic.turnLeft(4.0f); break;
            case 3: logic.stop(); break;
            default: logic.forward(6.0f); break;
        }
    }
}

static void runAutonomous(RobotLogic& logic, PoseEstimator& pose, WebotsHAL& hal) {
    std::string lastState;
    float lastPosePrint = -1.0f;
    while (hal.step()) {
        pose.update();
        logic.update();

        const float t = hal.getTime();
        if (lastState != logic.getStateName()) {
            std::cout << "[AUTONOMOUS] t=" << t
                      << "s -> Zustand: " << logic.getStateName() << std::endl;
            lastState = logic.getStateName();
        }
        if (t - lastPosePrint >= 2.0f) {
            printPose("[POSE]", t, pose);
            lastPosePrint = t;
        }
    }
}

static void runOdometryTest(RobotLogic& logic, PoseEstimator& pose, WebotsHAL& hal) {
    // 0-5s geradeaus, 5-6s Stopp, 6-9s auf der Stelle links drehen, dann Stopp.
    int lastPhase = -1;
    float lastPosePrint = -1.0f;
    while (hal.step()) {
        pose.update();
        const float t = hal.getTime();
        const int phase = (t < 5.0f) ? 0 : (t < 6.0f) ? 1 : (t < 9.0f) ? 2 : 3;
        if (phase != lastPhase) {
            const char* names[4] = {"geradeaus", "stopp", "links drehen", "stopp"};
            std::cout << "[ODO_TEST] t=" << t << "s -> Phase: " << names[phase] << std::endl;
            printPose("[ODO_TEST]", t, pose);
            lastPhase = phase;
        }
        switch (phase) {
            case 0:  logic.forward(4.0f); break;
            case 2:  logic.turnLeft(3.0f); break;
            default: logic.stop(); break;
        }
        if (t - lastPosePrint >= 1.0f) {
            printPose("[POSE]", t, pose);
            lastPosePrint = t;
        }
    }
}

// ---------------------------------------------------------------------
// Szenario fuer MODE_NAVIGATE. Diese Werte MUESSEN mit
// worlds/coasterbot-astar.wbt uebereinstimmen (Weltkoordinaten, Z = oben).
// ---------------------------------------------------------------------
namespace nav_scenario {
    const Vec2  START{0.0f, -1.2f};
    const float START_HEADING = static_cast<float>(M_PI) / 2.0f;  // Blick +Y
    const Vec2  GOAL{0.0f, 1.2f};

    // Karte: 3 x 3 m um den Ursprung, 5 cm Aufloesung. Zwei versetzte
    // Querwaende -> Slalom: Luecke rechts, dann Luecke links.
    OccupancyGrid makeGrid() {
        OccupancyGrid g(-1.5f, -1.5f, 1.5f, 1.5f, 0.05f);
        const float infl = 0.16f;  // ~ Roboter-Halbbreite
        g.addObstacleRect(-1.5f, -0.55f, 0.40f, -0.45f, infl);  // Wand 1, Luecke rechts
        g.addObstacleRect(-0.40f,  0.45f, 1.50f,  0.55f, infl);  // Wand 2, Luecke links
        return g;
    }
}

static void runNavigate(PoseEstimator& pose, WebotsHAL& hal) {
    using namespace nav_scenario;

    pose.reset(START.x, START.y, START_HEADING);
    OccupancyGrid grid = makeGrid();
    grid.buildDistanceField();

    std::vector<Vec2> path = astarPlan(grid, START, GOAL);
    std::cout << "[NAV] A* -> " << path.size() << " Wegpunkte:";
    for (const Vec2& w : path)
        std::cout << " (" << w.x << "," << w.y << ")";
    std::cout << std::endl;
    if (path.empty()) {
        std::cout << "[NAV] KEIN PFAD - Abbruch." << std::endl;
        return;
    }

    DwaPlanner dwa(hal, pose, grid);
    dwa.setPath(path);

    float lastPrint = -1.0f;
    while (hal.step()) {
        pose.update();
        dwa.update();

        const float t = hal.getTime();
        if (t - lastPrint >= 1.0f) {
            printPose("[NAV]", t, pose);
            std::cout << "        -> Carrot-Wegpunkt " << dwa.carrotIndex()
                      << "/" << path.size()
                      << "  v=" << dwa.commandedV() << " w=" << dwa.commandedW()
                      << std::endl;
            lastPrint = t;
        }
        if (dwa.reachedGoal()) {
            hal.setLeftSpeed(0.0f);
            hal.setRightSpeed(0.0f);
            const float ex = pose.getX() - GOAL.x;
            const float ey = pose.getY() - GOAL.y;
            std::cout << "[NAV] Ziel erreicht bei t=" << t << "s, Restfehler "
                      << std::sqrt(ex * ex + ey * ey) << " m" << std::endl;
            printPose("[NAV]", t, pose);
            hal.step();
            break;
        }
    }
}

// ---------------------------------------------------------------------
// Szenario fuer MODE_NAVIGATE_DSTAR. Bekannte Karte: eine Querwand mit
// ZWEI Luecken (links/rechts). Der Roboter plant durch die rechte Luecke;
// dort steht ein NICHT kartiertes Hindernis, das er per Ultraschall
// entdeckt -> D* Lite plant auf die linke Luecke um.
// Muss mit worlds/coasterbot-dstar.wbt uebereinstimmen.
// ---------------------------------------------------------------------
namespace dstar_scenario {
    const Vec2  START{0.7f, -1.2f};
    const float START_HEADING = static_cast<float>(M_PI) / 2.0f;  // Blick +Y
    const Vec2  GOAL{0.7f, 1.2f};

    // infl: Sicherheitsabstand. Der Planer bekommt einen groesseren Wert
    // (Pfad haelt Abstand von Ecken), DWA einen kleineren (Roboterradius).
    OccupancyGrid makeGrid(float infl) {
        OccupancyGrid g(-1.5f, -1.5f, 1.5f, 1.5f, 0.05f);
        // Querwand y in [-0.05, 0.05], drei Segmente -> breite Luecken bei
        // x in [-1.3,-0.1] (links) und x in [0.1,1.3] (rechts).
        g.addObstacleRect(-1.50f, -0.05f, -1.30f, 0.05f, infl);
        g.addObstacleRect(-0.10f, -0.05f,  0.10f, 0.05f, infl);
        g.addObstacleRect( 1.30f, -0.05f,  1.50f, 0.05f, infl);
        return g;
    }

    // Zellen in einem Weltrechteck belegen; geaenderte Zellen zurueckgeben.
    std::vector<Cell> markRect(OccupancyGrid& g, float x0, float y0, float x1, float y1) {
        std::vector<Cell> changed;
        const float step = g.resolution() * 0.5f;
        for (float y = y0; y <= y1; y += step)
            for (float x = x0; x <= x1; x += step) {
                const Cell c = g.worldToCell({x, y});
                if (g.setOccupied(c, true)) changed.push_back(c);
            }
        return changed;
    }
}

static void runNavigateDstar(PoseEstimator& pose, WebotsHAL& hal) {
    using namespace dstar_scenario;

    pose.reset(START.x, START.y, START_HEADING);
    OccupancyGrid planGrid = makeGrid(0.30f);  // Planer: grosszuegig
    OccupancyGrid dwaGrid  = makeGrid(0.16f);  // DWA: nur Roboterradius
    dwaGrid.buildDistanceField();

    DStarLite planner(planGrid);
    planner.plan(planGrid.worldToCell(START), planGrid.worldToCell(GOAL));
    std::vector<Vec2> path = smoothPath(planGrid, planner.extractPath());
    std::cout << "[DSTAR] Erstplanung -> " << path.size() << " Wegpunkte" << std::endl;
    if (path.empty()) { std::cout << "[DSTAR] KEIN PFAD - Abbruch." << std::endl; return; }

    DwaPlanner dwa(hal, pose, dwaGrid);
    dwa.setPath(path);

    Cell  prevCell = planGrid.worldToCell(START);
    float lastPrint = -1.0f;

    while (hal.step()) {
        pose.update();
        const Cell here = planGrid.worldToCell({pose.getX(), pose.getY()});

        // --- Sensorbasierte Kartenaktualisierung --------------------------
        // Der Ultraschall entdeckt ein unkartiertes Hindernis -> Zellen quer
        // ueber die Luecke in BEIDEN Karten belegen und D* Lite informieren.
        bool mapChanged = false;
        const float usd = hal.getUltrasonicDistance();
        if (usd < 0.75f) {
            const float th = pose.getTheta();
            const float hx = pose.getX() + (usd + 0.10f) * std::cos(th);
            const float hy = pose.getY() + (usd + 0.10f) * std::sin(th);
            std::vector<Cell> changed =
                markRect(planGrid, hx - 0.35f, hy - 0.12f, hx + 0.35f, hy + 0.12f);
            markRect(dwaGrid, hx - 0.35f, hy - 0.12f, hx + 0.35f, hy + 0.12f);
            if (!changed.empty()) {
                for (const Cell& c : changed) planner.cellChanged(c);
                std::cout << "[DSTAR] Hindernis bei (" << hx << "," << hy << "), "
                          << changed.size() << " Zellen belegt, t=" << hal.getTime()
                          << "s" << std::endl;
                mapChanged = true;
            }
        }

        // --- D* Lite Schritt --------------------------------------------
        // Bei jedem Zellwechsel setStart + computeShortestPath, damit km und
        // das g-Feld gueltig bleiben (Standard-D*-Lite-Hauptschleife).
        if (here.cx != prevCell.cx || here.cy != prevCell.cy) {
            planner.setStart(here);
            planner.computeShortestPath();
            prevCell = here;
        }

        // Nur bei einer Kartenaenderung wird der Pfad neu bestimmt.
        if (mapChanged) {
            planner.setStart(here);
            planner.computeShortestPath();          // inkrementelle Reparatur
            std::vector<Vec2> raw = planner.extractPath();
            if (raw.empty()) {
                // Der komplette geplante Korridor ist blockiert -> D* Lite aus
                // der aktuellen Position vollstaendig neu aufsetzen.
                planner.plan(here, planGrid.worldToCell(GOAL));
                raw = planner.extractPath();
                std::cout << "[DSTAR] geplanter Weg komplett blockiert"
                          << " -> Neuinitialisierung" << std::endl;
            }
            dwaGrid.buildDistanceField();           // DWA braucht das frische Feld

            std::vector<Vec2> np = smoothPath(planGrid, raw);
            if (!np.empty()) {
                path = np;
                dwa.setPath(path);
                std::cout << "[DSTAR] Umplanung -> " << path.size()
                          << " Wegpunkte, t=" << hal.getTime() << "s" << std::endl;
            }
        }

        dwa.update();

        const float t = hal.getTime();
        if (t - lastPrint >= 2.0f) {
            printPose("[DSTAR]", t, pose);
            lastPrint = t;
        }
        if (dwa.reachedGoal()) {
            hal.setLeftSpeed(0.0f);
            hal.setRightSpeed(0.0f);
            const float ex = pose.getX() - GOAL.x;
            const float ey = pose.getY() - GOAL.y;
            std::cout << "[DSTAR] Ziel erreicht bei t=" << t << "s, Restfehler "
                      << std::sqrt(ex * ex + ey * ey) << " m" << std::endl;
            printPose("[DSTAR]", t, pose);
            hal.step();
            break;
        }
    }
}

// =====================================================================
// MODE_SIM_TEST - kombinierter Simulationstest (Testkonzept, Kap. 5)
// =====================================================================
// Muss mit worlds/coasterbot-simtest.wbt uebereinstimmen. Weltkoordinaten,
// Z = oben, Tischoberflaeche bei z ~ 0.715 (Table-PROTO, size 1.6 x 2.0 x 0.74).
namespace sim_scenario {
    // Tischflaeche (Rand = Absturzkante) - Footprint des Table-PROTO.
    const float TX0 = -0.8f, TX1 = 0.8f, TY0 = -1.0f, TY1 = 1.0f;

    const Vec2  START{0.0f, -0.80f};
    const float START_HEADING = static_cast<float>(M_PI) / 2.0f;   // Blick +Y
    const Vec2  GOAL{0.0f, 0.80f};

    // MID: Bierflasche mittig auf der Route (bekannte Tischbelegung) -> D* Lite
    // plant um sie herum. DEKO1/DEKO2 (Blumentopf + Flasche) stehen in den
    // Ecken, nicht auf der Route.
    struct Rect { float x0, y0, x1, y1; };
    const Rect MID  {-0.04f, -0.04f, 0.04f, 0.04f};
    const Rect DEKO1{-0.74f, 0.64f, -0.62f, 0.76f};   // Blumentopf (Ecke)
    const Rect DEKO2{0.62f, -0.76f, 0.74f, -0.64f};   // Bierflasche (Ecke)
    const std::vector<Rect> OBSTACLES = {MID, DEKO1, DEKO2};

    OccupancyGrid makeGrid(float border) {
        OccupancyGrid g(TX0, TY0, TX1, TY1, 0.05f);
        g.addObstacleRect(TX0, TY0, TX0, TY1, border);   // linker Rand
        g.addObstacleRect(TX1, TY0, TX1, TY1, border);   // rechter Rand
        g.addObstacleRect(TX0, TY0, TX1, TY0, border);   // unterer Rand
        g.addObstacleRect(TX0, TY1, TX1, TY1, border);   // oberer Rand
        g.addObstacleRect(MID.x0, MID.y0, MID.x1, MID.y1, 0.15f);  // bekannt, auf Route
        // DEKO steht in den Ecken abseits der Route -> nur zur Sicherheit mit
        // Roboterradius eintragen.
        g.addObstacleRect(DEKO1.x0, DEKO1.y0, DEKO1.x1, DEKO1.y1, 0.14f);
        g.addObstacleRect(DEKO2.x0, DEKO2.y0, DEKO2.x1, DEKO2.y1, 0.14f);
        return g;
    }

    std::vector<Cell> markRect(OccupancyGrid& g, float x0, float y0, float x1, float y1) {
        std::vector<Cell> changed;
        const float s = g.resolution() * 0.5f;
        for (float y = y0; y <= y1; y += s)
            for (float x = x0; x <= x1; x += s) {
                const Cell c = g.worldToCell({x, y});
                if (g.setOccupied(c, true)) changed.push_back(c);
            }
        return changed;
    }

    bool onTable(float x, float y) {
        return x > TX0 && x < TX1 && y > TY0 && y < TY1;
    }

    // Abstand eines Punktes zum (achsenparallelen) Rechteck (0 innerhalb).
    float distToRect(float px, float py, float x0, float y0, float x1, float y1) {
        const float dx = std::max({x0 - px, 0.0f, px - x1});
        const float dy = std::max({y0 - py, 0.0f, py - y1});
        return std::sqrt(dx * dx + dy * dy);
    }
}

static void runSimTest(PoseEstimator& pose, WebotsHAL& hal) {
    using namespace sim_scenario;

    pose.reset(START.x, START.y, START_HEADING);
    OccupancyGrid planGrid = makeGrid(0.12f);
    OccupancyGrid dwaGrid  = makeGrid(0.10f);
    dwaGrid.buildDistanceField();

    DStarLite planner(planGrid);
    planner.plan(planGrid.worldToCell(START), planGrid.worldToCell(GOAL));
    std::vector<Vec2> path =
        centerPath(dwaGrid, smoothPath(planGrid, planner.extractPath()));
    std::cout << "[SIM] Erstplanung -> " << path.size() << " Wegpunkte" << std::endl;
    if (path.empty()) {
        std::cout << "[SIM] KEIN PFAD - Test abgebrochen." << std::endl;
        return;
    }

    const int initialWp = static_cast<int>(path.size());

    DwaPlanner     dwa(hal, pose, dwaGrid);
    SafetyMonitor  safety(hal);
    dwa.setUltrasonicRange(0.45f);   // DWA: Nahbereich-Sicherung
    dwa.setPath(path);

    // --- Messwerte fuer die Bewertung -------------------------------------
    bool  leftTable = false;
    float minObstDist = 1e9f;           // kleinste Distanz Roboterrand<->Hindernis
    bool  reachedGoal = false;
    float goalErr = 1e9f, goalTime = -1.0f;
    int   navReplans = 0;
    bool  obstacleSeen = false;
    int   edgeEventsNav = 0, edgeEventsEdgePhase = 0;

    enum Phase { NAV, EDGE, DONE } phase = NAV;
    float phaseStart = 0.0f;
    Cell  prevCell = planGrid.worldToCell(START);
    float lastPrint = -1.0f;
    const float ROBOT_R = 0.14f;

    while (hal.step()) {
        pose.update();
        const float t = hal.getTime();
        const float px = pose.getX(), py = pose.getY();

        if (!onTable(px, py)) leftTable = true;
        for (const Rect& o : OBSTACLES)
            minObstDist = std::min(minObstDist,
                                   distToRect(px, py, o.x0, o.y0, o.x1, o.y1) - ROBOT_R);

        // (1) Sicherheitskomponente hat immer Vorrang.
        const bool safetyHolds = safety.update();
        if (safety.edgeEventThisTick()) {
            std::cout << "[SIM] t=" << t << "s  KANTE erkannt ("
                      << (safety.frontEdge() ? "vorne" : "")
                      << (safety.rearEdge() ? " hinten" : "")
                      << ") -> " << safety.stateName() << std::endl;
            if (phase == NAV) {
                ++edgeEventsNav;
                // Kante in die Karte eintragen (~0.25 m voraus) und neu planen.
                const float th = pose.getTheta();
                const float ex = px + 0.25f * std::cos(th);
                const float ey = py + 0.25f * std::sin(th);
                std::vector<Cell> ch =
                    markRect(planGrid, ex - 0.12f, ey - 0.12f, ex + 0.12f, ey + 0.12f);
                markRect(dwaGrid, ex - 0.12f, ey - 0.12f, ex + 0.12f, ey + 0.12f);
                for (const Cell& c : ch) planner.cellChanged(c);
            } else {
                ++edgeEventsEdgePhase;
            }
        }

        if (!safetyHolds) {
            if (phase == NAV) {
                const Cell here = planGrid.worldToCell({px, py});
                bool mapChanged = safety.edgeEventThisTick();

                // (2) Ultraschall: nur ein UNKARTIERTES Objekt (nicht in der
                //     Naehe eines bekannten Hindernisses) loest eine Umplanung
                //     aus (ST-SIM-002 Sensor-Erkennung).
                const float usd = hal.getUltrasonicDistance();
                if (usd < 0.55f) {
                    const float th = pose.getTheta();
                    const float hx = px + (usd + 0.10f) * std::cos(th);
                    const float hy = py + (usd + 0.10f) * std::sin(th);
                    bool known = false;
                    for (const Rect& o : OBSTACLES)
                        if (distToRect(hx, hy, o.x0, o.y0, o.x1, o.y1) < 0.12f) known = true;
                    if (!known && onTable(hx, hy)) {
                        std::vector<Cell> cp =
                            markRect(planGrid, hx - 0.13f, hy - 0.13f, hx + 0.13f, hy + 0.13f);
                        std::vector<Cell> cd =
                            markRect(dwaGrid, hx - 0.13f, hy - 0.13f, hx + 0.13f, hy + 0.13f);
                        if (!cd.empty()) dwaGrid.buildDistanceField();
                        if (!cp.empty()) {
                            for (const Cell& c : cp) planner.cellChanged(c);
                            if (!obstacleSeen)
                                std::cout << "[SIM] t=" << t
                                          << "s  unkartiertes Hindernis erkannt bei ("
                                          << hx << "," << hy << ")" << std::endl;
                            obstacleSeen = true;
                            mapChanged = true;
                        }
                    }
                }

                // (3) D* Lite: Startzelle nachfuehren, bei Aenderung neu planen.
                if (here.cx != prevCell.cx || here.cy != prevCell.cy) {
                    planner.setStart(here);
                    planner.computeShortestPath();
                    prevCell = here;
                }
                if (mapChanged) {
                    planner.setStart(here);
                    planner.computeShortestPath();
                    std::vector<Vec2> raw = planner.extractPath();
                    if (raw.empty()) {
                        planner.plan(here, planGrid.worldToCell(GOAL));
                        raw = planner.extractPath();
                    }
                    dwaGrid.buildDistanceField();
                    std::vector<Vec2> np =
                        centerPath(dwaGrid, smoothPath(planGrid, raw));
                    if (!np.empty()) {
                        path = np;
                        dwa.setPath(path);
                        ++navReplans;
                        std::cout << "[SIM] t=" << t << "s  Umplanung -> "
                                  << path.size() << " Wegpunkte" << std::endl;
                    }
                }

                dwa.update();

                if (dwa.reachedGoal()) {
                    reachedGoal = true;
                    goalErr = static_cast<float>(std::hypot(px - GOAL.x, py - GOAL.y));
                    goalTime = t;
                    std::cout << "[SIM] t=" << t << "s  Ziel erreicht, Restfehler "
                              << goalErr << " m" << std::endl;
                    phase = EDGE;
                    phaseStart = t;
                    std::cout << "[SIM] --- Phase 2: Fahrt Richtung Tischkante (ST-SIM-003) ---"
                              << std::endl;
                }
                if (t > 90.0f) { std::cout << "[SIM] Zeitueberschreitung Navigation" << std::endl;
                                 phase = EDGE; phaseStart = t; }
            } else if (phase == EDGE) {
                // Erst auf +Y ausrichten, dann langsam geradeaus Richtung
                // Tischkante. Ohne Planer/DWA - nur die Sicherheitskomponente
                // darf hier noch eingreifen.
                const float th = pose.getTheta();
                const float err = std::atan2(std::sin(1.5708f - th),
                                             std::cos(1.5708f - th));
                if (std::fabs(err) > 0.10f) {
                    const float w = std::max(-1.2f, std::min(1.2f, 3.0f * err));
                    hal.setLeftSpeed(-w);
                    hal.setRightSpeed(w);
                } else {
                    const float w = 4.0f * err;
                    hal.setLeftSpeed(1.3f - w);   // ~0.045 m/s vorwaerts
                    hal.setRightSpeed(1.3f + w);
                }
                if (t - phaseStart > 15.0f) { phase = DONE; }
            }
        }

        if (phase == DONE) break;

        if (t - lastPrint >= 3.0f) {
            std::cout << "[SIM] t=" << t << "s  pose (" << px << "," << py
                      << ")  th=" << pose.getTheta() * 57.2958f << " deg  "
                      << (phase == NAV ? "NAV" : "KANTE") << "  safety="
                      << safety.stateName() << std::endl;
            lastPrint = t;
        }
    }

    hal.setLeftSpeed(0.0f);
    hal.setRightSpeed(0.0f);
    hal.step();

    // --- Bewertung ------------------------------------------------------
    const bool p1 = reachedGoal && goalErr < 0.10f;
    // ST-SIM-002: Route fuehrt um die Hindernisse (Slalom-Pfad oder Umplanung)
    // UND kein Hindernis wurde beruehrt/verschoben.
    const bool p2 = (initialWp > 2 || navReplans > 0) && minObstDist > -0.01f
                    && reachedGoal && !leftTable;
    const bool p3 = (edgeEventsEdgePhase > 0) && !leftTable && onTable(pose.getX(), pose.getY());
    auto verdict = [](bool ok) { return ok ? "BESTANDEN" : "FEHLGESCHLAGEN"; };

    std::cout << "\n========== Simulationstest - Ergebnis ==========\n";
    std::cout << "ST-SIM-001 Navigation zu einem Zielpunkt      : " << verdict(p1)
              << "  (Ziel erreicht=" << reachedGoal << ", Fehler=" << goalErr
              << " m, t=" << goalTime << " s)\n";
    std::cout << "ST-SIM-002 Dynamische Hindernisvermeidung     : " << verdict(p2)
              << "  (Pfad-Wegpunkte=" << initialWp << ", Umplanungen=" << navReplans
              << ", min. Hindernisabstand=" << minObstDist << " m)\n";
    std::cout << "ST-SIM-003 Verhindern des Tischverlassens     : " << verdict(p3)
              << "  (Kanten-Ereignisse gesamt=" << safety.eventCount()
              << ", Phase 1=" << edgeEventsNav << ", Phase 2=" << edgeEventsEdgePhase
              << ", Tisch verlassen=" << leftTable << ")\n";
    std::cout << "ST-SIM-004 Reproduzierbarkeit                 : "
              << "(deterministisch - zweiten Lauf vergleichen)\n";
    std::cout << "Endpose: (" << pose.getX() << ", " << pose.getY() << "), odo="
              << pose.getOdometer() << " m\n";
    std::cout << "===============================================" << std::endl;
}

int main() {
    WebotsHAL hal;
    RobotLogic logic(hal);
    PoseEstimator pose(hal);

    switch (MODE) {
        case MODE_WHEEL_TEST:      runWheelTest(hal); break;
        case MODE_MANUAL:          runManual(logic, hal); break;
        case MODE_AUTONOMOUS:      runAutonomous(logic, pose, hal); break;
        case MODE_ODOMETRY_TEST:   runOdometryTest(logic, pose, hal); break;
        case MODE_NAVIGATE:        runNavigate(pose, hal); break;
        case MODE_NAVIGATE_DSTAR:  runNavigateDstar(pose, hal); break;
        case MODE_SIM_TEST:        runSimTest(pose, hal); break;
    }
    return 0;
}
