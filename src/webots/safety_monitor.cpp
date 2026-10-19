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

void SafetyMonitor::drive(float radsPerSide) {
    hal_.setLeftSpeed(radsPerSide);
    hal_.setRightSpeed(radsPerSide);
}

void SafetyMonitor::enterEvade(bool front, float t) {
    evadeDir_ = front ? -1.0f : 1.0f;
    state_ = State::EVADE;
    stateEnteredAt_ = t;
    evadeStall_.restart();
}

const char* SafetyMonitor::stateName() const {
    switch (state_) {
        case State::CLEAR:  return "CLEAR";
        case State::EVADE:  return "EVADE";
        case State::PAUSE:  return "PAUSE";
        case State::PINNED: return "PINNED";
        case State::BLOCKED: return "BLOCKED";
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
                drive(0.0f);                     // sofort anhalten
                ++eventCount_;
                edgeEvent_ = true;
                stateEnteredAt_ = t;
                if (front && rear) {
                    state_ = State::PINNED;
                } else {
                    enterEvade(front, t);
                }
                return true;
            }
            return false;

        case State::EVADE:
            evadeStall_.update();   // vor drive(): bewertet den Befehl des letzten Zyklus
            drive(evadeDir_ * EVADE_RADS);
            if (t - stateEnteredAt_ > EVADE_TIME) {
                drive(0.0f);
                if (evadeStall_.stalled()) {
                    // Ausweichrichtung gesperrt (Hindernis, HAL-Sperre) -> nicht endlos weiterversuchen
                    std::cout << "[SAFETY] evade without wheel motion -> blocked, retry in "
                              << BLOCKED_RETRY_TIME << " s" << std::endl;
                    state_ = State::BLOCKED;
                } else {
                    state_ = State::PAUSE;
                }
                stateEnteredAt_ = t;
            }
            return true;

        case State::PAUSE:
            drive(0.0f);
            if (t - stateEnteredAt_ > PAUSE_TIME) {
                if (front && rear)      state_ = State::PINNED;
                else if (front || rear) enterEvade(front, t);
                else                    state_ = State::CLEAR;
                stateEnteredAt_ = t;
            }
            return true;

        case State::PINNED:
            drive(0.0f);
            if (!front || !rear) { state_ = State::PAUSE; stateEnteredAt_ = t; }
            return true;

        case State::BLOCKED:
            drive(0.0f);
            // Kante frei oder Wartezeit um -> ueber PAUSE neu bewerten (ggf. erneutes Ausweichen)
            if ((!front && !rear) || t - stateEnteredAt_ > BLOCKED_RETRY_TIME) {
                state_ = State::PAUSE;
                stateEnteredAt_ = t;
            }
            return true;
    }
    return false;
}
