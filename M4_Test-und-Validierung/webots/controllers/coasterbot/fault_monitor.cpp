#include "fault_monitor.h"
#include <algorithm>
#include <cmath>

FaultMonitor::FaultMonitor(RobotHAL& hal) : hal_(hal) {}

void FaultMonitor::drive(float left, float right) {
    hal_.setLeftSpeed(left);
    hal_.setRightSpeed(right);
}

bool FaultMonitor::ultrasonicImplausible() const {
    const float d = hal_.getUltrasonicDistance();
    return d < 0.0f || d > ULTRASONIC_MAX_VALID;
}

bool FaultMonitor::actuatorDeviating(float commandedLeft, float commandedRight) {
    const float now = hal_.getTime();
    const float nowLeft  = hal_.getWheelAngle(WHEEL_LEFT);
    const float nowRight = hal_.getWheelAngle(WHEEL_RIGHT);
    if (!actuatorInit_) {
        prevAngleLeft_ = nowLeft;
        prevAngleRight_ = nowRight;
        prevTime_ = now;
        actuatorInit_ = true;
        return false;  // noch keine Winkelaenderung messbar
    }
    const float dt = std::max(now - prevTime_, 1e-4f);
    const float measuredLeft  = (nowLeft  - prevAngleLeft_)  / dt;
    const float measuredRight = (nowRight - prevAngleRight_) / dt;
    prevAngleLeft_ = nowLeft;
    prevAngleRight_ = nowRight;
    prevTime_ = now;

    leftDeviated_  = std::fabs(commandedLeft)  > MIN_COMMANDED_RADS
                   && std::fabs(commandedLeft  - measuredLeft)  > SPEED_DEVIATION_RADS;
    rightDeviated_ = std::fabs(commandedRight) > MIN_COMMANDED_RADS
                   && std::fabs(commandedRight - measuredRight) > SPEED_DEVIATION_RADS;
    return leftDeviated_ || rightDeviated_;
}

bool FaultMonitor::update(float commandedLeft, float commandedRight) {
    if (state_ != State::NORMAL) {
        drive(0.0f, 0.0f);   // verriegelt: bleibt im sicheren Zustand
        return true;
    }

    if (ultrasonicImplausible()) {
        state_ = State::SENSOR_FAULT;
        drive(0.0f, 0.0f);
        return true;
    }

    const float t = hal_.getTime();
    if (actuatorDeviating(commandedLeft, commandedRight)) {
        if (deviatingSince_ < 0.0f) deviatingSince_ = t;
        if (t - deviatingSince_ > FAULT_DEBOUNCE_TIME) {
            state_ = State::ACTUATOR_FAULT;
            drive(0.0f, 0.0f);
            return true;
        }
    } else {
        deviatingSince_ = -1.0f;
    }
    return false;
}

const char* FaultMonitor::stateName() const {
    switch (state_) {
        case State::NORMAL:         return "NORMAL";
        case State::SENSOR_FAULT:   return "SENSOR_FAULT";
        case State::ACTUATOR_FAULT: return "ACTUATOR_FAULT";
    }
    return "?";
}
