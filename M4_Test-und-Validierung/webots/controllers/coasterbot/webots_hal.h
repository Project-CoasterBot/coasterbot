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
    void setWheelSpeed(WheelId wheel, float radPerSec) override;
    void setLeftSpeed(float radPerSec) override;
    void setRightSpeed(float radPerSec) override;
    float getUltrasonicDistance() override;
    float getEdgeFrontLeft() override;
    float getEdgeFrontRight() override;
    float getEdgeRearLeft() override;
    float getEdgeRearRight() override;
    float getYaw() override;
    float getGyroZ() override;
    float getWheelAngle(WheelId wheel) override;
    float getTime() override;

private:
    webots::Robot robot_;
    int timeStep_;

    webots::Motor* motorFL_;
    webots::Motor* motorFR_;
    webots::Motor* motorRL_;
    webots::Motor* motorRR_;

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
};

#endif  // WEBOTS_HAL_H
