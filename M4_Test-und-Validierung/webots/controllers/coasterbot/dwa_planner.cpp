#include "dwa_planner.h"

#include <algorithm>
#include <cmath>

namespace {
float wrapAngle(float a) {
    while (a > static_cast<float>(M_PI))  a -= 2.0f * static_cast<float>(M_PI);
    while (a <= -static_cast<float>(M_PI)) a += 2.0f * static_cast<float>(M_PI);
    return a;
}
float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
}  // namespace

DwaPlanner::DwaPlanner(RobotHAL& hal, PoseEstimator& pose, OccupancyGrid& grid)
    : hal_(hal), pose_(pose), grid_(grid) {}

void DwaPlanner::setPath(const std::vector<Vec2>& waypoints) {
    path_ = waypoints;
    carrotIdx_ = 0;
    done_ = path_.empty();
}

void DwaPlanner::driveVW(float v, float w) {
    const float wl = (v - w * TRACK_WIDTH * 0.5f) / WHEEL_RADIUS;
    const float wr = (v + w * TRACK_WIDTH * 0.5f) / WHEEL_RADIUS;
    hal_.setLeftSpeed(wl);
    hal_.setRightSpeed(wr);
}

Vec2 DwaPlanner::carrot(float px, float py) {
    if (path_.size() < 2) return path_.empty() ? Vec2{px, py} : path_.back();

    // 1. Naechsten Punkt auf dem Pfad-Polygonzug finden (Projektion).
    float  bestD2 = 1e18f;
    size_t bestSeg = 0;
    float  bestT = 0.0f;
    for (size_t i = 0; i + 1 < path_.size(); ++i) {
        const Vec2 a = path_[i], b = path_[i + 1];
        const float dx = b.x - a.x, dy = b.y - a.y;
        const float len2 = dx * dx + dy * dy;
        float t = (len2 > 1e-9f) ? ((px - a.x) * dx + (py - a.y) * dy) / len2 : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        const float cx = a.x + t * dx, cy = a.y + t * dy;
        const float d2 = (px - cx) * (px - cx) + (py - cy) * (py - cy);
        if (d2 < bestD2) { bestD2 = d2; bestSeg = i; bestT = t; }
    }
    carrotIdx_ = bestSeg;   // fuer die Anzeige

    // 2. CARROT_DIST Meter entlang des Pfades vorruecken (Pure Pursuit).
    //    Der Carrot bleibt so immer auf dem (kollisionsfreien) A*-Pfad.
    float  remain = CARROT_DIST;
    size_t seg = bestSeg;
    float  t = bestT;
    while (seg + 1 < path_.size()) {
        const Vec2 a = path_[seg], b = path_[seg + 1];
        const float segLen = static_cast<float>(std::hypot(b.x - a.x, b.y - a.y));
        const float avail = segLen * (1.0f - t);
        if (avail >= remain || seg + 2 == path_.size()) {
            const float tt = std::min(1.0f, t + remain / std::max(segLen, 1e-6f));
            return {a.x + tt * (b.x - a.x), a.y + tt * (b.y - a.y)};
        }
        remain -= avail;
        ++seg;
        t = 0.0f;
    }
    return path_.back();
}

float DwaPlanner::trajectoryClearance(float px, float py, float th, float v, float w,
                                      const Vec2& usObs, bool haveUs) const {
    float sx = px, sy = py, sth = th;
    float minClr = 1e6f;
    for (float t = 0.0f; t < SIM_TIME; t += SIM_DT) {
        sx  += v * std::cos(sth) * SIM_DT;
        sy  += v * std::sin(sth) * SIM_DT;
        sth += w * SIM_DT;

        // grid_.clearance ist der Abstand zum bereits um die Roboter-
        // Halbbreite aufgeblaehten Hindernis -> nur kleinen Restpuffer abziehen.
        float c = grid_.clearance({sx, sy}) - GRID_MARGIN;
        if (haveUs) {
            // Ultraschall liefert einen ROHEN Punkt -> vollen Radius abziehen.
            const float cu = static_cast<float>(std::hypot(sx - usObs.x, sy - usObs.y))
                             - ROBOT_RADIUS;
            c = std::min(c, cu);
        }
        if (c < minClr) minClr = c;
        if (minClr <= 0.0f) return 0.0f;   // Kollision
    }
    return minClr;
}

void DwaPlanner::update() {
    if (done_ || path_.empty()) { driveVW(0.0f, 0.0f); return; }

    const float px = pose_.getX();
    const float py = pose_.getY();
    const float th = pose_.getTheta();

    const float now = hal_.getTime();
    float dt = (prevTime_ < 0.0f) ? 0.016f : (now - prevTime_);
    if (dt < 1e-4f) dt = 1e-4f;
    prevTime_ = now;

    const Vec2 goal = path_.back();
    if (std::hypot(goal.x - px, goal.y - py) < GOAL_TOL) {
        done_ = true;
        driveVW(0.0f, 0.0f);
        return;
    }

    const Vec2  target = carrot(px, py);
    const float dCarrot0 = static_cast<float>(std::hypot(target.x - px, target.y - py));

    // Ultraschall als virtuelles Hindernis vor dem Roboter.
    bool haveUs = false;
    Vec2 usObs{};
    const float usd = hal_.getUltrasonicDistance();
    if (usd < usRange_) {
        haveUs = true;
        const float d = usd + 0.11f;   // Sensor sitzt ~0.11 m vor dem Zentrum
        usObs = {px + d * std::cos(th), py + d * std::sin(th)};
    }

    // Dynamisches Fenster (durch Beschleunigungsgrenzen erreichbare v, w).
    const float vLo = std::max(0.0f,   curV_ - ACC_V * dt);
    const float vHi = std::min(MAX_V,  curV_ + ACC_V * dt);
    const float wLo = std::max(-MAX_W, curW_ - ACC_W * dt);
    const float wHi = std::min(MAX_W,  curW_ + ACC_W * dt);

    float bestScore = -1e9f, bestV = 0.0f, bestW = 0.0f;
    bool any = false;

    for (int iv = 0; iv < V_SAMPLES; ++iv) {
        const float v = (V_SAMPLES == 1) ? vHi
                        : vLo + (vHi - vLo) * iv / (V_SAMPLES - 1);
        for (int iw = 0; iw < W_SAMPLES; ++iw) {
            const float w = (W_SAMPLES == 1) ? 0.0f
                            : wLo + (wHi - wLo) * iw / (W_SAMPLES - 1);

            const float clr = trajectoryClearance(px, py, th, v, w, usObs, haveUs);
            if (clr <= 0.0f) continue;                        // Kollision
            if (v > std::sqrt(2.0f * clr * ACC_V)) continue;  // kann nicht bremsen

            // Endpose der simulierten Trajektorie.
            float sx = px, sy = py, sth = th;
            for (float t = 0.0f; t < SIM_TIME; t += SIM_DT) {
                sx  += v * std::cos(sth) * SIM_DT;
                sy  += v * std::sin(sth) * SIM_DT;
                sth += w * SIM_DT;
            }
            const float dCarrot1 = static_cast<float>(std::hypot(target.x - sx, target.y - sy));
            const float bearing  = std::atan2(target.y - sy, target.x - sx);
            const float dHead    = std::fabs(wrapAngle(bearing - sth));

            const float headingScore  = 1.0f - dHead / static_cast<float>(M_PI);
            const float progressScore = clampf(
                (dCarrot0 - dCarrot1) / (0.5f * MAX_V * SIM_TIME), 0.0f, 1.0f);
            const float clearScore = (clr >= CLEAR_OK) ? 1.0f : clr / CLEAR_OK;
            const float velScore   = v / MAX_V;

            const float score = W_HEADING  * headingScore
                              + W_PROGRESS * progressScore
                              + W_CLEAR    * clearScore
                              + W_VEL      * velScore;
            if (score > bestScore) {
                bestScore = score;
                bestV = v;
                bestW = w;
                any = true;
            }
        }
    }

    if (!any) {
        // Sackgasse: langsam auf der Stelle drehen, um neue Optionen zu finden.
        curV_ = 0.0f;
        curW_ = 0.5f * MAX_W;
        driveVW(curV_, curW_);
        return;
    }

    curV_ = bestV;
    curW_ = bestW;
    driveVW(bestV, bestW);
}
