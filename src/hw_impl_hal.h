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

    GyroSensor<board::PIN_INERTIAL_SCL, board::PIN_INERTIAL_SDA> inertial_sensor;

    ServoController<board::PIN_COASTER_SERVO_1> servo1;
    ServoController<board::PIN_COASTER_SERVO_2> servo2;

protected: // evaluated state
    bool _button_is_pressed {false};
    bool _edge_rl {false}, _edge_rr {false}, _edge_fl {false}, _edge_fr {false};
    float _obstcl_dist {-1.};

    float _speed_left {0.}, _speed_right {0.};

    std::optional<unsigned> _button_pressed;
    std::optional<unsigned long> _movement_prevented_start_ms;

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
