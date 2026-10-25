#include "safety_monitor.h"
#include <iostream>

SafetyMonitor::SafetyMonitor(RobotHAL& hal)
    : hal_(hal),
      evadeStall_(hal, EVADE_STALL_MIN_TIME, EVADE_STALL_MIN_EXPECTED, EVADE_STALL_MIN_MOVED) {}

bool SafetyMonitor::anyFront() const {
    return hal_.getEdgeFrontLeft() || hal_.getEdgeFrontRight();
}

bool SafetyMonitor::anyRear() const {
    return hal_.getEdgeRearLeft() || hal_.getEdgeRearRight();
}

bool SafetyMonitor::frontEdge() const { return anyFront(); }
bool SafetyMonitor::rearEdge()  const { return anyRear(); }

void SafetyMonitor::drive(float left, float right) {
    hal_.setLeftSpeed(left);
    hal_.setRightSpeed(right);
}

void SafetyMonitor::reset() {
    drive(0.0f, 0.0f);
    state_ = State::CLEAR;
    attempt_ = 0;
    round_ = 0;
}

int SafetyMonitor::maneuvers(Maneuver (&out)[MAX_MANEUVERS]) const {
    const bool fl = hal_.getEdgeFrontLeft();
    const bool fr = hal_.getEdgeFrontRight();
    const bool rl = hal_.getEdgeRearLeft();
    const bool rr = hal_.getEdgeRearRight();
    const bool front = fl || fr;
    const bool rear = rl || rr;
    // Drehen links schwenkt vorne rechts und hinten links nach aussen, rechts vorne links und hinten rechts
    const bool leftAllowed = !fr && !rl;
    const bool rightAllowed = !fl && !rr;

    int n = 0;
    if (front != rear) { // weg von der Kante, erst normal, dann mit voller Kraft
        const float dir = front ? -1.0f : 1.0f;
        out[n++] = { dir * EVADE_RADS, dir * EVADE_RADS, EVADE_TIME, front ? "backward" : "forward" };
        out[n++] = { dir * EVADE_STRONG_RADS, dir * EVADE_STRONG_RADS, EVADE_TIME,
                     front ? "backward strong" : "forward strong" };
    }
    // Drehen bevorzugt so, dass der aktive Sensor zurueck auf den Tisch schwenkt
    const bool preferRight = fl || rr;
    for (int i = 0; i < 2; ++i) {
        const bool right = (i == 0) == preferRight;
        if (right && rightAllowed)
            out[n++] = { EVADE_TURN_RADS, -EVADE_TURN_RADS, EVADE_TURN_TIME, "turn right" };
        if (!right && leftAllowed)
            out[n++] = { -EVADE_TURN_RADS, EVADE_TURN_RADS, EVADE_TURN_TIME, "turn left" };
    }
    return n;
}

bool SafetyMonitor::enterEvade(float t) {
    Maneuver list[MAX_MANEUVERS];
    const int n = maneuvers(list);
    if (n == 0) return false;

    if (attempt_ >= n) { // alle durch
        attempt_ = 0;
        if (++round_ >= MAX_ROUNDS) {
            drive(0.0f, 0.0f);
            state_ = State::STUCK;
            stateEnteredAt_ = t;
            std::cout << "[SAFETY] no evade maneuver moves the wheels -> stuck" << std::endl;
            return true;
        }
    }
    maneuver_ = list[attempt_];
    state_ = State::EVADE;
    stateEnteredAt_ = t;
    evadeStall_.restart();
    std::cout << "[SAFETY] evade: " << maneuver_.name << " (attempt " << attempt_ + 1 << "/" << n
              << ", round " << round_ + 1 << "/" << MAX_ROUNDS << ")" << std::endl;
    return true;
}

const char* SafetyMonitor::stateName() const {
    switch (state_) {
        case State::CLEAR:  return "CLEAR";
        case State::EVADE:  return "EVADE";
        case State::PAUSE:  return "PAUSE";
        case State::PINNED: return "PINNED";
        case State::BLOCKED: return "BLOCKED";
        case State::STUCK:  return "STUCK";
    }
    return "?";
}

bool SafetyMonitor::update() {
    edgeEvent_ = false;
    const float t = hal_.getTime();
    const bool front = anyFront();
    const bool rear  = anyRear();

    switch (state_) {
        case State::CLEAR:
            if (front || rear) {
                drive(0.0f, 0.0f);               // sofort anhalten
                ++eventCount_;
                edgeEvent_ = true;
                attempt_ = 0;
                round_ = 0;
                if (!enterEvade(t)) {
                    state_ = State::PINNED;
                    stateEnteredAt_ = t;
                }
                return true;
            }
            return false;

        case State::EVADE:
            evadeStall_.update();   // vor drive(): bewertet den Befehl des letzten Zyklus
            drive(maneuver_.left, maneuver_.right);
            if (t - stateEnteredAt_ > maneuver_.duration) {
                drive(0.0f, 0.0f);
                if (evadeStall_.stalled()) {
                    // Raeder stehen -> naechstes Manoever nach kurzer Pause
                    std::cout << "[SAFETY] " << maneuver_.name << " without wheel motion" << std::endl;
                    ++attempt_;
                    state_ = State::BLOCKED;
                } else {
                    attempt_ = 0;
                    round_ = 0;
                    state_ = State::PAUSE;
                }
                stateEnteredAt_ = t;
            }
            return true;

        case State::PAUSE:
            drive(0.0f, 0.0f);
            if (t - stateEnteredAt_ > PAUSE_TIME) {
                stateEnteredAt_ = t;
                if (!front && !rear) state_ = State::CLEAR;
                else if (!enterEvade(t)) state_ = State::PINNED;
            }
            return true;

        case State::PINNED:
            drive(0.0f, 0.0f);
            // kein Manoever erlaubt: warten, bis sich die Sensoren aendern
            if (!front || !rear) { state_ = State::PAUSE; stateEnteredAt_ = t; }
            return true;

        case State::BLOCKED:
            drive(0.0f, 0.0f);
            if (t - stateEnteredAt_ > BLOCKED_RETRY_TIME) {
                stateEnteredAt_ = t;
                if (!front && !rear) state_ = State::CLEAR;
                else if (!enterEvade(t)) state_ = State::PINNED;
            }
            return true;

        case State::STUCK:
            drive(0.0f, 0.0f);
            if (!front && !rear) reset(); // z.B. von Hand befreit
            return true;
    }
    return false;
}
