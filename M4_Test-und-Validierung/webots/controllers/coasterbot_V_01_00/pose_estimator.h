#ifndef POSE_ESTIMATOR_H
#define POSE_ESTIMATOR_H

#include "robot_hal.h"

// ---------------------------------------------------------------------
// Schaetzt die Pose (x, y, theta) des Roboters auf der Tischebene.
//
// Die Vorwaertsbewegung wird durch Integration der IMU-Beschleunigung
// (RobotHAL::getForwardAcceleration) geschaetzt; der Kurs kommt aus der
// integrierten Gyro-Drehrate (RobotHAL::getGyroZ). Die doppelte Integration
// der Beschleunigung ist driftanfaellig.
//
// Kennt NUR das RobotHAL-Interface -> unveraendert auf einen Arduino
// portierbar (keine Encoder, ein MPU-6050 liefer die Drehrate).
//
// Konvention: x nach vorne (lokale Startausrichtung), y nach links,
// theta = 0 in Startrichtung, positiv = Linksdrehung (mathematisch
// positiv, gegen den Uhrzeigersinn von oben gesehen).
// ---------------------------------------------------------------------
class PoseEstimator {
public:
    explicit PoseEstimator(RobotHAL& hal);

    // Einmal pro Regelzyklus aufrufen (nach hal.step()).
    void update();

    // Aktuelle Schaetzung.
    float getX() const { return x_; }             // [m]
    float getY() const { return y_; }             // [m]
    float getTheta() const { return theta_; }     // [rad], (-pi, pi]
    float getYawRate() const { return prevGyro_; } // [rad/s], bias-korrigiert

    // Aus der IMU-Beschleunigung integrierte Wegstrecke [m].
    float getOdometer() const { return odometer_; }

    // Pose auf einen bekannten Wert setzen (z.B. Startpose aus der Welt).
    void reset(float x = 0.0f, float y = 0.0f, float theta = 0.0f);

private:
    RobotHAL& hal_;

    static constexpr float WHEEL_RADIUS = 0.0325f;   // [m]

    // --- Tuning-Parameter ---
    // m/s pro (WHEEL_RADIUS * rad/s Sollwert). Sim (Closed-Loop): 1.0.
    // Real per Fahrtest kalibrieren: 1 m fahren, SPEED_SCALE = gemessen/berechnet.
    static constexpr float SPEED_SCALE    = 1.0f;
    static constexpr float MOTOR_LAG_TAU  = 0.15f;   // [s] Motor-/Traegheitsverzoegerung
    static constexpr float VELOCITY_TAU   = 0.4f;    // [s] Zeitkonstante Modell-Korrektur
                                                     //  klein = Modell dominiert, gross = IMU dominiert
    static constexpr float STILL_SETTLE_S = 0.3f;    // [s] Ruhe nach Sollwert 0 abwarten
    static constexpr float STILL_GYRO_MAX = 0.05f;   // [rad/s] Gyro muss ruhig sein
    static constexpr float BIAS_TAU       = 1.0f;    // [s] Nachfuehrung der Biase im Stand

    bool  initialized_ = false;
    float prevTime_ = 0.0f;
    float prevAccel_ = 0.0f;       // [m/s^2] biaskorrigiert
    float prevGyro_ = 0.0f;        // [rad/s] biaskorrigiert
    float stillSince_ = 0.0f;
    float accelBias_ = 0.0f;       // [m/s^2]
    float gyroBias_ = 0.0f;        // [rad/s] Restbias zusaetzlich zur HAL-Kalibrierung
    float forwardVelocity_ = 0.0f; // [m/s]
    float velocityModel_ = 0.0f;   // [m/s] geglaettete Modellgeschwindigkeit

    float x_ = 0.0f, y_ = 0.0f, theta_ = 0.0f, odometer_ = 0.0f;
    static float wrapAngle(float a);
};

#endif  // POSE_ESTIMATOR_H
