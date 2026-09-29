#include "pose_estimator.h"
#include <cmath>

PoseEstimator::PoseEstimator(RobotHAL& hal) : hal_(hal) {}

float PoseEstimator::wrapAngle(float a) { /* unveraendert */ 
    while (a > static_cast<float>(M_PI))  a -= 2.0f * static_cast<float>(M_PI);
    while (a <= -static_cast<float>(M_PI)) a += 2.0f * static_cast<float>(M_PI);
    return a;
}

void PoseEstimator::reset(float x, float y, float theta) {
    x_ = x; y_ = y; theta_ = wrapAngle(theta);
    odometer_ = 0.0f;
    forwardVelocity_ = 0.0f;
    velocityModel_ = 0.0f;
    initialized_ = false;   // Biase bleiben erhalten
}

void PoseEstimator::update() {
    const float now      = hal_.getTime();
    const float accelRaw = hal_.getForwardAcceleration();
    const float gyroRaw  = hal_.getGyroZ();
    const float cmdL     = hal_.getCommandedSpeed(WHEEL_LEFT);
    const float cmdR     = hal_.getCommandedSpeed(WHEEL_RIGHT);

    if (!initialized_) {
        prevTime_   = now;
        prevAccel_  = accelRaw - accelBias_;
        prevGyro_   = gyroRaw  - gyroBias_;
        stillSince_ = now;
        initialized_ = true;
        return;
    }
    const float dt = now - prevTime_;
    if (dt <= 1e-6f) return;
    prevTime_ = now;

    // --- Stillstandserkennung (Sollwert 0 + Beruhigungszeit + ruhiges Gyro) ---
    const bool cmdZero = std::fabs(cmdL) < 1e-3f && std::fabs(cmdR) < 1e-3f;
    if (!cmdZero) stillSince_ = now;
    const bool stationary = cmdZero
                         && (now - stillSince_) > STILL_SETTLE_S
                         && std::fabs(gyroRaw - gyroBias_) < STILL_GYRO_MAX;

    // ZUPT: im Stand sind wahre Beschleunigung und Drehrate 0 -> Messwert = Bias
    if (stationary) {
        const float a = dt / (BIAS_TAU + dt);
        accelBias_ += a * (accelRaw - accelBias_);
        gyroBias_  += a * (gyroRaw  - gyroBias_);
    }

    const float accel = accelRaw - accelBias_;
    const float gyro  = gyroRaw  - gyroBias_;

    // --- Kurs: Trapezintegration der biaskorrigierten Drehrate ---
    const float dTheta = 0.5f * (prevGyro_ + gyro) * dt;

    // --- Geschwindigkeit: IMU-Praediktion + Korrektur zum Motormodell ---
    const float vCmd = SPEED_SCALE * WHEEL_RADIUS * 0.5f * (cmdL + cmdR);
    velocityModel_ += (vCmd - velocityModel_) * (dt / (MOTOR_LAG_TAU + dt));

    const float vPrev = forwardVelocity_;
    const float vPred = vPrev + 0.5f * (prevAccel_ + accel) * dt;
    const float k = dt / (VELOCITY_TAU + dt);
    forwardVelocity_ = vPred + k * (velocityModel_ - vPred);
    if (stationary) { forwardVelocity_ = 0.0f; velocityModel_ = 0.0f; }

    const float dCenter = 0.5f * (vPrev + forwardVelocity_) * dt;

    prevAccel_ = accel;
    prevGyro_  = gyro;

    // --- Integration (RK2) ---
    const float midTheta = theta_ + 0.5f * dTheta;
    x_ += dCenter * std::cos(midTheta);
    y_ += dCenter * std::sin(midTheta);
    theta_ = wrapAngle(theta_ + dTheta);
    odometer_ += std::fabs(dCenter);
}