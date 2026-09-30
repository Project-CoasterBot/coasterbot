#include "pose_estimator.h"
#include <cmath>

PoseEstimator::PoseEstimator(RobotHAL& hal) : hal_(hal) {}

float PoseEstimator::wrapAngle(float a) {
    while (a > PI)  a -= 2.0f * PI;
    while (a <= -PI) a += 2.0f * PI;
    return a;
}

void PoseEstimator::reset(float x, float y, float theta) {
    x_ = x;
    y_ = y;
    theta_ = wrapAngle(theta);
    yawOffset_ = wrapAngle(theta_ - hal_.getYaw());
    odometer_ = 0.0f;
    stationaryTime_ = 0.0f;
    initialized_ = false;  // naechstes update() setzt die Radwinkel-Referenz neu
}

void PoseEstimator::update() {
    // Je ein Encoderwert pro Seite (RobotHAL::getWheelAngle) - real gibt es
    // hier ohnehin nur einen Sensor je Seite; eine etwaige Mittelung ueber
    // mehrere Raeder derselben Seite ist Sache der jeweiligen HAL-
    // Implementierung (siehe WebotsHAL::getWheelAngle).
    const float left  = hal_.getWheelAngle(WHEEL_LEFT);
    const float right = hal_.getWheelAngle(WHEEL_RIGHT);

    if (!initialized_) {
        prevLeft_ = left;
        prevRight_ = right;
        prevTime_ = hal_.getTime();
        initialized_ = true;
        return;
    }

    const float now = hal_.getTime();
    const float dt = now - prevTime_;
    float dLeft  = (left  - prevLeft_)  * WHEEL_RADIUS;  // [m] Wegstrecke links
    float dRight = (right - prevRight_)  * WHEEL_RADIUS;  // [m] Wegstrecke rechts
    prevLeft_  = left;
    prevRight_ = right;
    prevTime_ = now;

    const float gyroRate = hal_.getGyroZ();
    const bool motorsCommandedStopped =
        std::fabs(hal_.getCommandedSpeed(WHEEL_LEFT)) <= STATIONARY_COMMAND_THRESHOLD &&
        std::fabs(hal_.getCommandedSpeed(WHEEL_RIGHT)) <= STATIONARY_COMMAND_THRESHOLD;
    const bool wheelsNearlyStill =
        std::fabs(dLeft) <= STATIONARY_DISTANCE_THRESHOLD &&
        std::fabs(dRight) <= STATIONARY_DISTANCE_THRESHOLD;
    const bool gyroNearlyStill = std::fabs(gyroRate) <= STATIONARY_GYRO_THRESHOLD;
    const bool stationaryCandidate = motorsCommandedStopped && wheelsNearlyStill && gyroNearlyStill;
    if (stationaryCandidate && dt > 0.0f) {
        stationaryTime_ += dt;
    } else {
        stationaryTime_ = 0.0f;
    }
    const bool stationary = stationaryTime_ >= STATIONARY_DWELL_TIME;
    if (stationaryCandidate) {
        dLeft = 0.0f;
        dRight = 0.0f;
    }
    if (stationary) {
        yawOffset_ = wrapAngle(theta_ - hal_.getYaw());
    }

    const float dCenter   = 0.5f * (dLeft + dRight);
    const float dThetaOdo = (dRight - dLeft) / TRACK_WIDTH;
    const float dThetaGyro = stationaryCandidate
        ? 0.0f
        : (dt > 0.0f ? gyroRate * dt : dThetaOdo);
    const float dTheta = HEADING_ODO_WEIGHT * dThetaOdo
                       + (1.0f - HEADING_ODO_WEIGHT) * dThetaGyro;
    const float predictedHeading = wrapAngle(theta_ + dTheta);
    const float measuredHeading = wrapAngle(hal_.getYaw() + yawOffset_);
    const float headingError = wrapAngle(measuredHeading - predictedHeading);
    const float correction = dt > 0.0f
        ? 1.0f - std::exp(-dt / YAW_CORRECTION_TIME_CONSTANT)
        : 0.0f;
    const float correctedHeading = wrapAngle(predictedHeading + correction * headingError);
    const float correctedDTheta = wrapAngle(correctedHeading - theta_);

    const float midTheta = theta_ + 0.5f * correctedDTheta;
    x_ += dCenter * std::cos(midTheta);
    y_ += dCenter * std::sin(midTheta);
    theta_ = correctedHeading;
    odometer_ += std::fabs(dCenter);
}