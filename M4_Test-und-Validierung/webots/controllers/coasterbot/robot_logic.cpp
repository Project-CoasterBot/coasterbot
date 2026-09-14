#include "robot_logic.h"
#include <cstdlib>

RobotLogic::RobotLogic(RobotHAL& hal) : hal_(hal) {}

bool RobotLogic::edgeDetected() const {
    return hal_.getEdgeFrontLeft()  || hal_.getEdgeFrontRight() ||
           hal_.getEdgeRearLeft()   || hal_.getEdgeRearRight();
}

void RobotLogic::enterState(State s) {
    state_ = s;
    stateEnteredAt_ = hal_.getTime();
    if (s == State::TURN || s == State::AVOID_OBSTACLE) {
        // Zufaellige Richtung, damit der Roboter nicht immer identisch reagiert
        turnDirection_ = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
    }
}

// --- Explizite Bewegungsbefehle (auch direkt aus main.cpp aufrufbar) ---

void RobotLogic::forward(float speed) {
    hal_.setLeftSpeed(speed);
    hal_.setRightSpeed(speed);
}

void RobotLogic::backward(float speed) {
    hal_.setLeftSpeed(-speed);
    hal_.setRightSpeed(-speed);
}

void RobotLogic::turnLeft(float speed) {
    hal_.setLeftSpeed(-speed);
    hal_.setRightSpeed(speed);
}

void RobotLogic::turnRight(float speed) {
    hal_.setLeftSpeed(speed);
    hal_.setRightSpeed(-speed);
}

void RobotLogic::stop() {
    hal_.setLeftSpeed(0.0f);
    hal_.setRightSpeed(0.0f);
}

const char* RobotLogic::getStateName() const {
    switch (state_) {
        case State::FORWARD:        return "FORWARD";
        case State::REVERSE:        return "REVERSE";
        case State::TURN:           return "TURN";
        case State::AVOID_OBSTACLE: return "AVOID_OBSTACLE";
    }
    return "?";
}

// --- Interne Nutzung durch den Zustandsautomaten in update() ---

void RobotLogic::driveForward() { forward(CRUISE_SPEED); }
void RobotLogic::driveReverse() { backward(CRUISE_SPEED); }

void RobotLogic::driveTurn() {
    if (turnDirection_ >= 0.0f) turnLeft(TURN_SPEED);
    else                        turnRight(TURN_SPEED);
}

void RobotLogic::update() {
    const float elapsed = hal_.getTime() - stateEnteredAt_;

    switch (state_) {
        case State::FORWARD:
            if (edgeDetected()) {
                enterState(State::REVERSE);
                break;
            }
            if (hal_.getUltrasonicDistance() < OBSTACLE_THRESHOLD_M) {
                enterState(State::AVOID_OBSTACLE);
                break;
            }
            driveForward();
            break;

        case State::REVERSE:
            driveReverse();
            if (elapsed > REVERSE_DURATION) {
                enterState(State::TURN);
            }
            break;

        case State::AVOID_OBSTACLE:
            // An Ort und Stelle weiterdrehen, bis der Ultraschall wirklich
            // frei ist (nicht nur fuer eine feste Zeit) - sonst faehrt der
            // Roboter wieder in dasselbe Hindernis.
            driveTurn();
            if (edgeDetected()) {
                enterState(State::REVERSE);
            } else if (elapsed > TURN_DURATION &&
                       hal_.getUltrasonicDistance() >= OBSTACLE_THRESHOLD_M) {
                enterState(State::FORWARD);
            }
            break;

        case State::TURN:
            driveTurn();
            if (elapsed > TURN_DURATION && !edgeDetected()) {
                enterState(State::FORWARD);
            }
            break;
    }
}
