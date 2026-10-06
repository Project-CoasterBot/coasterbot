#include "hw_impl_hal.h"

#include <Arduino.h>

bool HardwareImplementationHAL::userButtonReleased(unsigned& duration_ms) {
    if (! _button_pressed) return false;
    duration_ms = _button_pressed.value();
    return true;
}

bool HardwareImplementationHAL::movementPrevented(unsigned long& duration_ms) {
    if (! _movement_prevented_start_ms.has_value()) return false;
    duration_ms = millis() - _movement_prevented_start_ms.value();
    return true;
}

void HardwareImplementationHAL::initializeHardware() {
    led_board.begin();
    led_board.setModeToRainbow(3000);

    user_button.begin();
    motion_ctrl.begin();
    motion_ctrl.stop();

    encoder_left.begin();
    encoder_right.begin();

    servo1.begin();
    servo2.begin();

    edge_det_rear_left.begin();
    edge_det_rear_right.begin();
    edge_det_front_left.begin();
    edge_det_front_right.begin();

    obstcl_sensor.begin();

    inertial_sensor.begin();
}






void HardwareImplementationHAL::setTrackWidth(float track_width_in_meter) {
    if (! std::isfinite(track_width_in_meter) || track_width_in_meter <= 0.f) return;
    _track_width = track_width_in_meter;
}

void HardwareImplementationHAL::resetPose() {
    _pos_x = _pos_y = _heading = _distance_forward = 0.f;
}

void HardwareImplementationHAL::updateOdometry(float moved_left, float moved_right) {
    if (moved_left == 0.f && moved_right == 0.f) return;

    // differential drive: mean of both sides moves the center, difference turns the bot
    const float moved = 0.5f * (moved_left + moved_right);
    const float turned = (moved_right - moved_left) / _track_width;

    // integrate along the mean heading of this step
    const float mid_heading = _heading + 0.5f * turned;
    _pos_x += moved * std::cos(mid_heading);
    _pos_y += moved * std::sin(mid_heading);
    _distance_forward += moved;

    _heading = std::remainder(_heading + turned, 2.f * static_cast<float>(M_PI)); // keep in [-pi, pi]
}

bool HardwareImplementationHAL::step() {
    const unsigned long now = millis();
    unsigned long user_button_pressed_duration_millis;

    // 1. fetch new input

    bool user_button_released = user_button.update(now, user_button_pressed_duration_millis);
    bool button_is_currently_pressed = user_button.isPressed();

    _edge_rl = ! edge_det_rear_left.obstacleDetected();
    _edge_rr = ! edge_det_rear_right.obstacleDetected();
    _edge_fl = ! edge_det_front_left.obstacleDetected();
    _edge_fr = ! edge_det_front_right.obstacleDetected();

    float obstacle_dist = obstcl_sensor.getAccumulatedValue();

    // 2. Internal logic

    _button_is_pressed = button_is_currently_pressed;
    if (user_button_released) {
        _button_pressed = user_button_pressed_duration_millis;
        this->onUserButtonReleased(user_button_pressed_duration_millis);
    } else
        _button_pressed.reset();

    bool sensors_valid = true;
    if (std::isfinite(obstacle_dist) && obstacle_dist > 0.)
        _obstcl_dist = obstacle_dist;
    else {
        _obstcl_dist = -1.f;
        sensors_valid = false;
    }

    bool motion_valid = true;
    if (! std::isfinite(_speed_left) || ! std::isfinite(_speed_right)) {
        motion_ctrl.stop();
        motion_valid = false;
    } else if (std::abs(_speed_left) != std::abs(_speed_right)) {
        // left and right wheel need to turn at the same speed
        motion_ctrl.stop();
        motion_valid = false;
        led_board.setModeToBlinkOrange(250);
    } else if (std::abs(_speed_left) == 0.) { // no speed, but both equal
        motion_ctrl.stop();
    } else { // actual motion
        // eval motion
        if (_speed_left > 0 && _speed_right > 0)
            motion_ctrl.forward();
        else if (_speed_left < 0 && _speed_right < 0)
            motion_ctrl.backward();
        else if (_speed_left > 0 && _speed_right < 0)
            motion_ctrl.turnRight();
        else if (_speed_left < 0 && _speed_right > 0)
            motion_ctrl.turnLeft();

        // TODO motion_ctrl.setSpeed() from rad per second... turning is voltage dependant. control with inertial system?
    }

    // check if sensors allow movement, not sure if this is the place or if that should be controlled from outside logic.
    bool movement_prevented = false;
    if (motion_ctrl.forwardRequested() && (_edge_fl || _edge_fr || (_obstcl_dist >= 0. && _obstcl_dist < 0.075 ))) { // x cm min dist
        motion_ctrl.stop();
        movement_prevented = true;
    }
    if (motion_ctrl.backwardRequested() && (_edge_rl || _edge_rr)) {
        motion_ctrl.stop();
        movement_prevented = true;
    }
    if (motion_ctrl.turnLeftRequested() && (_edge_fl || _edge_fr || _edge_rl || _edge_rr)) {
        motion_ctrl.stop();
        movement_prevented = true;
    }
    if (motion_ctrl.turnRightRequested() && (_edge_fl || _edge_fr || _edge_rl || _edge_rr)) {
        motion_ctrl.stop();
        movement_prevented = true;
    }

    // if movement is prevented, store time since when
    if (! movement_prevented)
        _movement_prevented_start_ms.reset();
    else if (! _movement_prevented_start_ms.has_value())
        _movement_prevented_start_ms = millis();

    // state evaluation and indicator to the user
    if (! motion_valid)
        this->setUserLEDToMotionError();
    else if (! sensors_valid)
        this->setUserLEDToSensorError();
    else if (movement_prevented)
        this->setUserLEDToObstacleError();
    else
        this->setUserLEDToDefaultOperating();

    // 3. Output

    led_board.update();
    motion_ctrl.update();
    obstcl_sensor.update();

    // the encoders only see edges, the turning direction comes from what the motors actually do
    encoder_left.setCurrentDirection(motion_ctrl.leftDirection());
    encoder_right.setCurrentDirection(motion_ctrl.rightDirection());
    float moved_rad, moved_left, moved_right;
    encoder_left.getDistance(moved_rad, moved_left);
    _wheel_angle_left += moved_rad;
    encoder_right.getDistance(moved_rad, moved_right);
    _wheel_angle_right += moved_rad;
    updateOdometry(moved_left, moved_right);

    servo1.update();
    servo2.update();

    inertial_sensor.setMotion(motion_ctrl.isMoving() || motion_ctrl.isBusy() ? Motion::Driving :
        motion_ctrl.isTurning() ? Motion::Turning : Motion::Idle);
    inertial_sensor.loop();

    const unsigned long end = millis();
    if (end - now <= 0)
        delay(1); // enforce Regelzyklus ~1 kHz.

    return true;
}

void HardwareImplementationHAL::setLeftSpeed(float radPerSec) { _speed_left = radPerSec; }
void HardwareImplementationHAL::setRightSpeed(float radPerSec) { _speed_right = radPerSec; }

float HardwareImplementationHAL::getCommandedSpeed(WheelId wheel) {
    switch (wheel) {
    case WHEEL_LEFT: return _speed_left;
    case WHEEL_RIGHT: return _speed_right;
    default: break;
    }
    return 0.;
}

float HardwareImplementationHAL::getUltrasonicDistance() { return _obstcl_dist; }
bool HardwareImplementationHAL::getEdgeFrontLeft() { return _edge_fl; }
bool HardwareImplementationHAL::getEdgeFrontRight() { return _edge_fr; }
bool HardwareImplementationHAL::getEdgeRearLeft() { return _edge_rl; }
bool HardwareImplementationHAL::getEdgeRearRight() { return _edge_rr; }

float HardwareImplementationHAL::getYaw() { return 0.f; /* sensor for picking up? */ }
float HardwareImplementationHAL::getGyroZ() { return 0.f; /* height? */}

float HardwareImplementationHAL::getForwardAcceleration() {
    float x = inertial_sensor.accel_x, y = inertial_sensor.accel_y;
    return sqrt(x*x + y*y);
}

float HardwareImplementationHAL::getWheelAngle(WheelId wheel) {
    switch (wheel) {
    case WHEEL_LEFT: return _wheel_angle_left;
    case WHEEL_RIGHT: return _wheel_angle_right;
    default: break;
    }
    return 0.;
}

float HardwareImplementationHAL::getTime() { return static_cast<float>(micros()) * 1e-6; }

bool HardwareImplementationHAL::getButtonState() { return user_button.isPressed(); }
bool HardwareImplementationHAL::simulateButtonPress(float duration) {
    // nothing to do, only simulation
    return false;
}

void HardwareImplementationHAL::setServoPosition(int servoId, int position, unsigned long duration_ms) {
    switch (servoId) {
    case 0: servo1.moveTo(position, duration_ms); break;
    case 1: servo2.moveTo(position, duration_ms); break;
    }
}

void HardwareImplementationHAL::led(LED_COLORS color) {
    switch (color) {
    case LED_COLORS::OFF:
        led_board.setModeToOff();
        break;
    case LED_COLORS::RED:
        led_board.setModeToConstantRed();
        break;
    case LED_COLORS::GREEN:
        led_board.setModeToConstantGreen();
        break;
    case LED_COLORS::BLUE:
        led_board.setModeToConstantBlue();
        break;
    case LED_COLORS::ORANGE:
        led_board.setModeToConstantOrange();
        break;

        default: break;
    }
}
