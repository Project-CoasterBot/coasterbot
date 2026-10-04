#ifndef SRC_HW_IMPL_HAL_H
#define SRC_HW_IMPL_HAL_H

#include "robot_hal.h"

#include "boardconfig.h"
#include "ledinterface.h"
#include "buttoninterface.h"
#include "motioninterface.h"
#include "infraredsensorinterrface.h"
#include "echosensorinterface.h"
#include "gyrosensor.h"
#include "motiondetection.h"
#include "servointerface.h"

class HardwareImplementationHAL : public RobotHAL {
protected: // hw controllers
    LEDController<board::PIN_LED1_R, board::PIN_LED1_G, board::PIN_LED1_B> led_board;
    ButtonController<board::PIN_USER_BUTTON> user_button;

    EchosensorInterface<board::PIN_ULTRASONIC_ECHO, board::PIN_ULTRASONIC_TRIGGER> obstcl_sensor;

    InfraredSensorControl<board::PIN_EDGE_REAR_LEFT> edge_det_rear_left;
    InfraredSensorControl<board::PIN_EDGE_REAR_RIGHT> edge_det_rear_right;
    InfraredSensorControl<board::PIN_EDGE_FRONT_LEFT> edge_det_front_left;
    InfraredSensorControl<board::PIN_EDGE_FRONT_RIGHT> edge_det_front_right;

    MotionController<board::PIN_MOTOR_PWM, board::PIN_MOTOR_STANDBY, board::PIN_MOTOR_LEFT_1, board::PIN_MOTOR_LEFT_2, board::PIN_MOTOR_RIGHT_1, board::PIN_MOTOR_RIGHT_2> motion_ctrl;

    // counter state is static per pin, so the two sides must not share one
    static_assert(board::PIN_MOTION_DETECT_LEFT != board::PIN_MOTION_DETECT_RIGHT, "motion detection needs distinct pins");
    MotionDetectionOptoCoupling<board::PIN_MOTION_DETECT_LEFT> encoder_left;
    MotionDetectionOptoCoupling<board::PIN_MOTION_DETECT_RIGHT> encoder_right;

    GyroSensor<board::PIN_INERTIAL_SCL, board::PIN_INERTIAL_SDA> inertial_sensor;

    ServoController<board::PIN_COASTER_SERVO_1> servo1;
    ServoController<board::PIN_COASTER_SERVO_2> servo2;

protected: // evaluated state
    bool _button_is_pressed {false};
    bool _edge_rl {false}, _edge_rr {false}, _edge_fl {false}, _edge_fr {false};
    float _obstcl_dist {-1.};

    float _speed_left {0.}, _speed_right {0.};
    float _wheel_angle_left {0.}, _wheel_angle_right {0.}; // accumulated encoder angle in rad, positive = forward

    // odometry from the wheel encoders. Start pose: origin, heading along +x, heading counter-clockwise positive
    float _track_width {0.202f};  // distance between left and right wheels in m
    float _pos_x {0.}, _pos_y {0.}; // in m
    float _heading {0.};            // in rad, (-pi, pi], 0 = +x direction
    float _distance_forward {0.};   // driven distance in m, forward positive, backward negative

    std::optional<unsigned> _button_pressed;
    std::optional<unsigned long> _movement_prevented_start_ms;

    /// Integrates the distances both sides moved since the last call (in m, forward positive) into the pose.
    void updateOdometry(float moved_left, float moved_right);

public:
    virtual ~HardwareImplementationHAL() {}

public: // interface to dynamically react on events
    virtual void onUserButtonReleased(unsigned duration_ms) { /* reimplement for custom behaviour */ }

    // user indicator cases
    virtual void setUserLEDToDefaultOperating() { led_board.setModeToRainbow(2500); }
    virtual void setUserLEDToMotionError() { led_board.setModeToBlinkRed(250); }
    virtual void setUserLEDToSensorError() { led_board.setModeToBlinkOrange(250); }
    virtual void setUserLEDToObstacleError() { led_board.setModeToBlinkPurple(250); }

public: // getters

    bool userButtonReleased(unsigned& duration_ms);

    // debug: get inertial sensor instance
    GyroSensor<board::PIN_INERTIAL_SCL, board::PIN_INERTIAL_SDA>& inertialSensor() { return inertial_sensor; }

    bool movementPrevented(unsigned long& duration_ms);

public: // odometry from the wheel encoders

    /// Distance between the left and right wheels in m, used to derive the turning from the wheel distances.
    void setTrackWidth(float track_width_in_meter);

    /// Position in m relative to the start point (or the last resetPose()). At start the bot drives along +x.
    void getPosition(float& x, float& y) const { x = _pos_x; y = _pos_y; }
    /// Heading in rad, (-pi, pi]: 0 = +x, pi/2 = +y (counter-clockwise positive, i.e. turning left increases it).
    float getHeading() const { return _heading; }
    /// Heading as unit vector in the x/y system.
    void getHeadingVector(float& dx, float& dy) const { dx = std::cos(_heading); dy = std::sin(_heading); }
    /// Distance driven in m (mean of both sides), forward positive, backward negative. Turning on the spot adds ~0.
    float getDistanceForward() const { return _distance_forward; }

    /// Sets the current pose as new origin, heading along +x.
    void resetPose();

public: // robotHAL implementation

    /// This method needs to be called in the begin() function of the microcontroller startup
    void initializeHardware();

    /// This method needs to be called in each iteration of the main loop.
    bool step() override;

    void setLeftSpeed(float radPerSec) override;
    void setRightSpeed(float radPerSec) override;
    float getCommandedSpeed(WheelId wheel) override;

    float getUltrasonicDistance() override;
    bool  getEdgeFrontLeft() override;
    bool  getEdgeFrontRight() override;
    bool  getEdgeRearLeft() override;
    bool  getEdgeRearRight() override;

    float getYaw() override;
    float getGyroZ() override;
    float getForwardAcceleration() override;

    float getWheelAngle(WheelId wheel) override;

    float getTime() override;

    virtual bool getButtonState() override;
    virtual bool simulateButtonPress(float duration) override;
    virtual void setServoPosition(int servoId, int position) override;
    virtual void led(LED_COLORS color) override;

};

#endif //SRC_HW_IMPL_HAL_H
