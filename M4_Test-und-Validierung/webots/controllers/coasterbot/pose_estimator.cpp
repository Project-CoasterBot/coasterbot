#include "pose_estimator.h"
#include <cmath>

PoseEstimator::PoseEstimator(RobotHAL& hal) : hal_(hal) {}

float PoseEstimator::wrapAngle(float a) {
    while (a > static_cast<float>(M_PI))  a -= 2.0f * static_cast<float>(M_PI);
    while (a <= -static_cast<float>(M_PI)) a += 2.0f * static_cast<float>(M_PI);
    return a;
}

void PoseEstimator::reset(float x, float y, float theta) {
    x_ = x;
    y_ = y;
    theta_ = wrapAngle(theta);
    odometer_ = 0.0f;
    initialized_ = false;  // naechstes update() setzt die Radwinkel-Referenz neu
}

void PoseEstimator::update() {
    // Je ein Encoderwert pro Seite (RobotHAL::getWheelAngle) - real gibt es
    // hier ohnehin nur einen Sensor je Seite; eine etwaige Mittelung ueber
    // mehrere Raeder derselben Seite ist Sache der jeweiligen HAL-
    // Implementierung (siehe WebotsHAL::getWheelAngle).
    const float left  = hal_.getWheelAngle(WHEEL_LEFT);
    const float right = hal_.getWheelAngle(WHEEL_RIGHT);
    const float now   = hal_.getTime();

    if (!initialized_) {
        prevLeft_ = left;
        prevRight_ = right;
        prevTime_ = now;
        initialized_ = true;
        return;
    }

    const float dLeft  = (left  - prevLeft_)  * WHEEL_RADIUS;  // [m] Wegstrecke links
    const float dRight = (right - prevRight_) * WHEEL_RADIUS;  // [m] Wegstrecke rechts
    const float dt     = now - prevTime_;
    prevLeft_  = left;
    prevRight_ = right;
    prevTime_  = now;

    const float dCenter   = 0.5f * (dLeft + dRight);
    const float dThetaOdo = (dRight - dLeft) / TRACK_WIDTH;

    // Kurs-Inkrement: Fusion aus Rad-Odometrie und integrierter Gyro-Drehrate.
    float dThetaGyro = dThetaOdo;
    if (dt > 1e-6f)
        dThetaGyro = hal_.getGyroZ() * dt;
    const float dTheta = HEADING_ODO_WEIGHT * dThetaOdo
                       + (1.0f - HEADING_ODO_WEIGHT) * dThetaGyro;

    // Integration ueber den mittleren Kurs des Schritts (2nd-order Runge-Kutta).
    const float midTheta = theta_ + 0.5f * dTheta;
    x_ += dCenter * std::cos(midTheta);
    y_ += dCenter * std::sin(midTheta);
    theta_ = wrapAngle(theta_ + dTheta);
    odometer_ += std::fabs(dCenter);
}
