#include "safety_monitor.h"

SafetyMonitor::SafetyMonitor(RobotHAL& hal) : hal_(hal) {}

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

const char* SafetyMonitor::stateName() const {
    switch (state_) {
        case State::CLEAR:  return "CLEAR";
        case State::EVADE:  return "EVADE";
        case State::PAUSE:  return "PAUSE";
        case State::PINNED: return "PINNED";
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
                    evadeDir_ = front ? -1.0f : 1.0f;
                    state_ = State::EVADE;
                }
                return true;
            }
            return false;

        case State::EVADE:
            drive(evadeDir_ * EVADE_RADS);
            if (t - stateEnteredAt_ > EVADE_TIME) {
                drive(0.0f);
                state_ = State::PAUSE;
                stateEnteredAt_ = t;
            }
            return true;

        case State::PAUSE:
            drive(0.0f);
            if (t - stateEnteredAt_ > PAUSE_TIME) {
                if (front && rear)      state_ = State::PINNED;
                else if (front || rear) { evadeDir_ = front ? -1.0f : 1.0f;
                                          state_ = State::EVADE; }
                else                    state_ = State::CLEAR;
                stateEnteredAt_ = t;
            }
            return true;

        case State::PINNED:
            drive(0.0f);
            if (!front || !rear) { state_ = State::PAUSE; stateEnteredAt_ = t; }
            return true;
    }
    return false;
}
