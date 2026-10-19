#include "stall_detector.h"
#include <cmath>

StallDetector::StallDetector(RobotHAL& hal, float minTime, float minExpected, float minMoved)
    : hal_(hal), minTime_(minTime), minExpected_(minExpected), minMoved_(minMoved) {}

int StallDetector::commandSign(float radPerSec) {
    if (radPerSec >  STANDING_COMMAND_THRESHOLD) return 1;
    if (radPerSec < -STANDING_COMMAND_THRESHOLD) return -1;
    return 0;
}

void StallDetector::restart() {
    startTime_ = hal_.getTime();
    lastTime_ = startTime_;
    startLeft_ = hal_.getWheelAngle(WHEEL_LEFT);
    startRight_ = hal_.getWheelAngle(WHEEL_RIGHT);
    signLeft_ = commandSign(hal_.getCommandedSpeed(WHEEL_LEFT));
    signRight_ = commandSign(hal_.getCommandedSpeed(WHEEL_RIGHT));
    expected_ = 0.0f;
    moved_ = 0.0f;
}

void StallDetector::update() {
    const float now = hal_.getTime();
    const float cmdLeft = hal_.getCommandedSpeed(WHEEL_LEFT);
    const float cmdRight = hal_.getCommandedSpeed(WHEEL_RIGHT);
    const int signLeft = commandSign(cmdLeft);
    const int signRight = commandSign(cmdRight);

    const bool standing = signLeft == 0 && signRight == 0;
    const bool commandChanged = signLeft != signLeft_ || signRight != signRight_;
    const bool stale = lastTime_ < 0.0f || now - lastTime_ > MAX_UPDATE_GAP;
    if (standing || commandChanged || stale) {
        restart();
        return;
    }

    const float dt = now - lastTime_;
    lastTime_ = now;
    if (dt > 0.0f) {
        expected_ += (std::fabs(cmdLeft) + std::fabs(cmdRight)) * dt;
    }
    moved_ = std::fabs(hal_.getWheelAngle(WHEEL_LEFT) - startLeft_) +
             std::fabs(hal_.getWheelAngle(WHEEL_RIGHT) - startRight_);

    // Bewegung nachgewiesen -> Fenster ab hier neu, sonst wuerde eine spaetere
    // Blockade von der frueheren Bewegung im selben Fenster verdeckt.
    if (moved_ >= minMoved_) {
        restart();
    }
}

bool StallDetector::stalled() const {
    return hal_.getTime() - startTime_ >= minTime_ &&
           expected_ >= minExpected_ &&
           moved_ < minMoved_;
}
