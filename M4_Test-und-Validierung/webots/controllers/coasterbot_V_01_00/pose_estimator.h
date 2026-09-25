#ifndef POSE_ESTIMATOR_H
#define POSE_ESTIMATOR_H

#include "robot_hal.h"

// ---------------------------------------------------------------------
// Schaetzt die Pose (x, y, theta) des Roboters auf der Tischebene.
//
// Grundlage: Differential-/Skid-Steer-Odometrie aus den zwei seitenweisen
// Radwinkeln (RobotHAL::getWheelAngle, ein Wert je Seite) plus Fusion des
// Kurses mit der integrierten Gyro-Drehrate (RobotHAL::getGyroZ) ueber
// einen Komplementaerfilter.
//
// Kennt NUR das RobotHAL-Interface -> unveraendert auf einen Arduino
// portierbar (dort liefern Radencoder die Winkel, ein MPU-6050 die
// Drehrate).
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

    // Gesamte gefahrene Wegstrecke ("Kilometerzaehler") [m].
    float getOdometer() const { return odometer_; }

    // Pose auf einen bekannten Wert setzen (z.B. Startpose aus der Welt).
    void reset(float x = 0.0f, float y = 0.0f, float theta = 0.0f);

private:
    RobotHAL& hal_;

    // Radgeometrie aus Coasterbot.proto.
    static constexpr float WHEEL_RADIUS = 0.035f;  // [m]
    static constexpr float TRACK_WIDTH  = 0.202f;  // [m], Abstand linke<->rechte Raeder

    // Komplementaerfilter: Gewicht der Rad-Odometrie am Kurs-Inkrement.
    // 0 = nur Gyro, 1 = nur Odometrie. Skid-Steer schlupft beim Drehen,
    // daher ueberwiegend dem Gyro vertrauen.
    static constexpr float HEADING_ODO_WEIGHT = 0.05f;

    bool  initialized_ = false;
    float prevLeft_ = 0.0f;   // [rad] gemittelte linke Radwinkel
    float prevRight_ = 0.0f;  // [rad] gemittelte rechte Radwinkel
    float prevTime_ = 0.0f;   // [s]

    float x_ = 0.0f;
    float y_ = 0.0f;
    float theta_ = 0.0f;
    float odometer_ = 0.0f;

    static float wrapAngle(float a);
};

#endif  // POSE_ESTIMATOR_H
