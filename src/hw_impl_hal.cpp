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

    // start in the rest position of the mechanism, not at 0 deg (lifter fully extended)
    servo1.begin(board::PIN_COASTER_SERVO_1_INITIAL_ANGLE);
    servo2.begin(board::PIN_COASTER_SERVO_2_INITIAL_ANGLE);

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
    _pos_x = _pos_y = _heading = _heading_encoder = _distance_forward = 0.f;
    _gyro_heading_last = inertial_sensor.heading; // only the change of the gyro heading is used
}

void HardwareImplementationHAL::updateOdometry(float moved_left, float moved_right, std::optional<float> turned_gyro) {
    constexpr float pi2 = 2.f * static_cast<float>(M_PI);

    // differential drive: mean of both sides moves the center, difference turns the bot
    const float moved = 0.5f * (moved_left + moved_right);
    const float turned_encoder = (moved_right - moved_left) / _track_width;
    const float turned = turned_gyro.value_or(turned_encoder);
    if (moved == 0.f && turned == 0.f && turned_encoder == 0.f) return;

    // integrate along the mean heading of this step
    const float mid_heading = _heading + 0.5f * turned;
    _pos_x += moved * std::cos(mid_heading);
    _pos_y += moved * std::sin(mid_heading);
    _distance_forward += moved;

    _heading = std::remainder(_heading + turned, pi2); // keep in [-pi, pi]
    _heading_encoder = std::remainder(_heading_encoder + turned_encoder, pi2);
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

        const bool turning = (_speed_left > 0) != (_speed_right > 0);
        motion_ctrl.setSpeed(pwmForSpeed(_speed_left, turning));

        // TODO motion_ctrl.setSpeed() from rad per second... turning is voltage dependant. control with inertial system?
    }

    // check if sensors allow movement, not sure if this is the place or if that should be controlled from outside logic.
    bool movement_prevented = false;
    if (motion_ctrl.forwardRequested() && frontBlocked()) {
        motion_ctrl.stop();
        movement_prevented = true;
    }
    if (motion_ctrl.backwardRequested() && (_edge_rl || _edge_rr)) {
        motion_ctrl.stop();
        movement_prevented = true;
    }
    // turning on the spot swings two diagonal corners outward and the other two inward: left (ccw) moves
    // front right and rear left outward, right (cw) front left and rear right. A turn is only allowed if it
    // moves every active sensor back onto the table, needed to square up at an edge (runLearnTable).
    if (motion_ctrl.turnLeftRequested() && (_edge_fr || _edge_rl)) {
        motion_ctrl.stop();
        movement_prevented = true;
    }
    if (motion_ctrl.turnRightRequested() && (_edge_fl || _edge_rr)) {
        motion_ctrl.stop();
        movement_prevented = true;
    }

    // if movement is prevented, store time since when
    if (! movement_prevented)
        _movement_prevented_start_ms.reset();
    else if (! _movement_prevented_start_ms.has_value()) {
        _movement_prevented_start_ms = millis();
        // debug: which sensor blocked the motion
        Serial.printf("[HAL] movement prevented: edge fl=%d fr=%d rl=%d rr=%d obstacle=%.3f m\n",
                      _edge_fl, _edge_fr, _edge_rl, _edge_rr, _obstcl_dist);
    }

    // state evaluation and indicator to the user
    if (_user_led_trigger_by_obstacles) {
        if (!motion_valid)
            this->setUserLEDToMotionError();
        else if (!sensors_valid)
            this->setUserLEDToSensorError();
        else if (movement_prevented)
            this->setUserLEDToObstacleError();
        else
            this->setUserLEDToDefaultOperating();
    }

    // 3. Output

    led_board.update();
    motion_ctrl.update();
    obstcl_sensor.update();

    servo1.update();
    servo2.update();

    // isTurning() first: isMoving() and isBusy() are true while turning as well
    inertial_sensor.setMotion(motion_ctrl.isTurning() ? Motion::Turning :
        motion_ctrl.isMoving() || motion_ctrl.isBusy() ? Motion::Driving : Motion::Idle);
    inertial_sensor.loop();

    // rotation since the last step from the gyro, its heading is in deg and wraps at +-180
    std::optional<float> turned_gyro;
    if (inertial_sensor.ok()) {
        const float gyro_heading = inertial_sensor.heading;
        turned_gyro = std::remainder(gyro_heading - _gyro_heading_last, 360.f) * DEG_TO_RAD;
        _gyro_heading_last = gyro_heading;
    }

    // the encoders only see edges, the turning direction comes from what the motors actually do
    encoder_left.setCurrentDirection(motion_ctrl.leftDirection());
    encoder_right.setCurrentDirection(motion_ctrl.rightDirection());
    float moved_rad, moved_left, moved_right;
    encoder_left.getDistance(moved_rad, moved_left);
    _wheel_angle_left += moved_rad;
    encoder_right.getDistance(moved_rad, moved_right);
    _wheel_angle_right += moved_rad;
    updateOdometry(moved_left, moved_right, turned_gyro);

    const unsigned long end = millis();
    if (end - now <= 0)
        delay(1); // enforce Regelzyklus ~1 kHz.

    return true;
}

uint8_t HardwareImplementationHAL::pwmForSpeed(float rad_per_sec, bool turning) const {
    const uint8_t pwm_max = turning ? _pwm_turn : _pwm_drive;
    if (_speed_nominal <= 0.f) return pwm_max;

    const uint8_t pwm_min = std::min(turning ? _pwm_turn_min : _pwm_drive_min, pwm_max);
    const float ratio = std::min(1.f, std::abs(rad_per_sec) / _speed_nominal);
    return static_cast<uint8_t>(std::lround(pwm_min + ratio * static_cast<float>(pwm_max - pwm_min)));
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

// the MPU-6050 has no absolute yaw reference, heading is the integrated gyro rate (ccw positive)
float HardwareImplementationHAL::getYaw() { return inertial_sensor.ok() ? inertial_sensor.heading * DEG_TO_RAD : 0.f; }
float HardwareImplementationHAL::getGyroZ() { return inertial_sensor.ok() ? inertial_sensor.rate_z * DEG_TO_RAD : 0.f; }

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

// 64 bit timer: micros() wraps after ~71 min, every time based state (waits, safety) would stall then
float HardwareImplementationHAL::getTime() { return static_cast<float>(static_cast<double>(time_us_64()) * 1e-6); }

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
    case LED_COLORS::YELLOW:
        led_board.setModeToConstantYellow();
        break;
    case LED_COLORS::CYAN:
        led_board.setModeToConstantCyan();
        break;
    case LED_COLORS::PURPLE:
        led_board.setModeToConstantPurple();
        break;
    case LED_COLORS::PINK:
        led_board.setModeToConstantPink();
        break;
    case LED_COLORS::WHITE:
        led_board.setModeToConstant(170, 170, 170);
        break;
    case LED_COLORS::BLINK_RED:
        led_board.setModeToBlinkRed();
        break;
    case LED_COLORS::BLINK_ORANGE:
        led_board.setModeToBlinkOrange();
        break;
    case LED_COLORS::BLINK_PURPLE:
        led_board.setModeToBlinkPurple();
        break;
    case LED_COLORS::BLINK_CYAN:
        led_board.setModeToBlinkCyan();
        break;
    case LED_COLORS::BLINK_YELLOW:
        led_board.setModeToBlinkYellow();
        break;
    case LED_COLORS::BLINK_GREEN_FAST:
        led_board.setModeToBlinkGreen(255, 80);
        break;
    case LED_COLORS::BLINK_BLUE_FAST:
        led_board.setModeToBlinkBlue(255, 80);
        break;
    case LED_COLORS::BLINK_WHITE:
        led_board.setModeToBlink(170, 170, 170);
        break;

        default: break;
    }
}
