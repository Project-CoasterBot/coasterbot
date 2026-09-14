#ifndef WEBOTS_HAL_H
#define WEBOTS_HAL_H

#include "robot_hal.h"
#include <webots/Robot.hpp>
#include <webots/Motor.hpp>
#include <webots/PositionSensor.hpp>
#include <webots/DistanceSensor.hpp>
#include <webots/InertialUnit.hpp>
#include <webots/Gyro.hpp>

// ---------------------------------------------------------------------
// Implementiert RobotHAL mit der Webots-C++-API.
//
// Das ist die EINZIGE Datei, die bei einem spaeteren Arduino-Port durch
// eine ArduinoHAL ersetzt werden muesste (analoges Interface, aber mit
// Arduino-Bibliotheken wie Servo/AFMotor, NewPing, MPU6050 etc.).
// robot_logic.h/.cpp bleiben dabei unveraendert.
// ---------------------------------------------------------------------
class WebotsHAL : public RobotHAL {
public:
    WebotsHAL();
    ~WebotsHAL() override;

    bool step() override;
    void setLeftSpeed(float radPerSec) override;
    void setRightSpeed(float radPerSec) override;
    float getUltrasonicDistance() override;
    bool  getEdgeFrontLeft() override;
    bool  getEdgeFrontRight() override;
    bool  getEdgeRearLeft() override;
    bool  getEdgeRearRight() override;
    float getYaw() override;
    float getGyroZ() override;
    float getWheelAngle(WheelId wheel) override;
    float getTime() override;

private:
    webots::Robot robot_;
    int timeStep_;

    // Je ein Handle pro Seite. Die Motoren einer Seite sind im PROTO
    // gekoppelt ("coupled motors"), ein Befehl an dieses Handle treibt
    // beide Raeder der Seite.
    webots::Motor* motorLeft_;
    webots::Motor* motorRight_;

    // Vier PositionSensor, weil das PROTO sie je Rad bereitstellt - die
    // reale Elektronik hat nur einen Encoder je Seite. getWheelAngle()
    // mittelt Front/Heck je Seite und gibt nur noch WHEEL_LEFT/WHEEL_RIGHT
    // nach aussen; diese vier Handles sind reines Simulationsdetail.
    webots::PositionSensor* encFL_;
    webots::PositionSensor* encFR_;
    webots::PositionSensor* encRL_;
    webots::PositionSensor* encRR_;

    webots::DistanceSensor* ultrasonic_;
    webots::DistanceSensor* edgeFL_;
    webots::DistanceSensor* edgeFR_;
    webots::DistanceSensor* edgeRL_;
    webots::DistanceSensor* edgeRR_;

    webots::InertialUnit* imu_;
    webots::Gyro* gyro_;

    // Kalibrierter Schwellenwert fuer die Kantensensoren (Rohwert der
    // Coasterbot.proto-lookupTable: ~0 auf einer Flaeche, ~1000 ueber einer
    // Kante). Die Kalibrierung ist hardwarespezifisch und bleibt deshalb
    // hier gekapselt - RobotHAL gibt nach aussen nur noch bool zurueck.
    static constexpr double EDGE_THRESHOLD = 500.0;
};

#endif  // WEBOTS_HAL_H
