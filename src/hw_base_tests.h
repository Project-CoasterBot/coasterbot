#ifndef SRC_HW_BASE_TESTS_H
#define SRC_HW_BASE_TESTS_H

#include <Arduino.h>

#include "navigation.h"

namespace hw_test {

template <typename Bot>
void outputCurrentPosition(Bot& _bot) {
    Serial.print("[COORD] heading=");
    Serial.print(_bot.getHeading());
    Serial.print("\t");
    float x,y;
    _bot.getPosition(x, y);
    Serial.print(x);
    Serial.print("\t/ ");
    Serial.print(y);
    Serial.print("\tenc=");
    Serial.print(_bot.getEncoderHeading()); // heading from the encoders only, compare to see the wheel slip
    Serial.println(" ");
}

    // bot faehrt nur vorwaerts/rueckwearts und gibt position aus.
template <typename Bot>
class BaseBotHandler {
protected:
    unsigned long next_heartbeat = 0;

protected:
    virtual void onBegin(Bot& _bot) {}
    virtual void onHeartbeat(Bot& _bot) {}
    virtual void onReset(Bot& _bot) {}
    virtual void onButtonPress(Bot& _bot) {}
    virtual void work(Bot& _bot) {}

public:
    BaseBotHandler() {}
    virtual ~BaseBotHandler() {}

    void begin(Bot& _bot) {
        this->onBegin(_bot);
    }

    void loop(Bot& _bot) {
        unsigned press_duration_ms;

        if (static_cast<long>(millis() - next_heartbeat) >= 0) {
            next_heartbeat += 500;
            this->onHeartbeat(_bot);
        }

        // button eval, could also be implemented via virtual method of bot
        if (_bot.userButtonReleased(press_duration_ms)) {
            if (press_duration_ms > 1500)
                this->onReset(_bot);
            else
                this->onButtonPress(_bot);
        }

        this->work(_bot);

    }
};









// bot faehrt nur vorwaerts/rueckwearts und gibt position aus.
template <typename Bot>
class FwdBckwd : public BaseBotHandler<Bot>{
    unsigned motion_state {0};
    bool move_fwd {true};
protected:

    void onHeartbeat(Bot& _bot) {
        outputCurrentPosition(_bot);
    }

    void onReset(Bot& _bot) override {
        Serial.println("[Motion] Reset and stop ");
        motion_state = 0;
        move_fwd = true;
        _bot.resetPose();
    }

    void onButtonPress(Bot& _bot) override {
        motion_state++;
    }

    void work(Bot& _bot) override {
        unsigned long movement_prevented_duration_ms;

        if (motion_state % 2u == 1) {
            float speed_left = 0, speed_right = 0;
            if (_bot.movementPrevented(movement_prevented_duration_ms)) {
                if (movement_prevented_duration_ms > 2500) {
                    move_fwd = !move_fwd;
                    outputCurrentPosition(_bot);
                }
            }

            speed_left = move_fwd ? 1. : -1.;
            speed_right = speed_left;

            _bot.setLeftSpeed(speed_left);
            _bot.setRightSpeed(speed_right);
        }
        else {
            _bot.setLeftSpeed(0);
            _bot.setRightSpeed(0);
        }
    }
};





// bot faehrt nur vorwaerts/rueckwearts und gibt position aus.
template <typename Bot>
class CoasterMechanismus : public BaseBotHandler<Bot>{
    unsigned motion_state {0};
protected:

    void onReset(Bot& _bot) override {
        Serial.println("[Motion] Reset and stop ");
        motion_state = 0;
        _bot.resetPose();
    }

    void onButtonPress(Bot& _bot) override {
        motion_state++;
    }

    void work(Bot& _bot) override {
        const unsigned arm_servo = 1;
        const unsigned heber_servo = 0;

        switch (motion_state % 8) {
        default:
        case 0:
        case 2:
        case 7:
            _bot.setServoPosition(arm_servo, 180, 1000);
            _bot.setServoPosition(heber_servo, 0, 4000);
            break;
        case 1:
            _bot.setServoPosition(arm_servo, 0, 2500);
            _bot.setServoPosition(heber_servo, 0, 0);
            break;
        case 3:
            _bot.setServoPosition(arm_servo, 180, 1000);
            _bot.setServoPosition(heber_servo, 45, 2000);
            break;
        case 4:
            _bot.setServoPosition(arm_servo, 180, 1000);
            _bot.setServoPosition(heber_servo, 90, 2000);
            break;
        case 5:
            _bot.setServoPosition(arm_servo, 180, 1000);
            _bot.setServoPosition(heber_servo, 135, 2000);
            break;
        case 6:
            _bot.setServoPosition(arm_servo, 180, 1000);
            _bot.setServoPosition(heber_servo, 180, 2000);
            break;

        }

        _bot.setLeftSpeed(0);
        _bot.setRightSpeed(0);
    }
};






// bot faehrt nur vorwaerts/rueckwearts und gibt position aus.
template <typename Bot>
class PosSequenceNav : public BaseBotHandler<Bot>{
    unsigned motion_state {0};

    Navigation _nav;
    Navigation::State _prev_state = Navigation::State::Idle;

    void initTargetPositions(Bot& _bot) {
        _nav.reset(_bot);
        const float s = 0.15; // meter

        for (int rounds = 0; rounds < 5; rounds++) {
            _nav.addPoint(s, 0.0); // vorwaerts
            _nav.addPoint(s, s); // nach links
            _nav.addPoint(s, 0.0); // zurueck
            _nav.addPoint(0.0, 0.0); // nach rechts
        }
    }

protected:

    void onBegin(Bot& _bot) override {
        initTargetPositions(_bot);
    }

    void onReset(Bot& _bot) override {
        Serial.println("[Motion] Reset and stop ");
        motion_state = 0;

        initTargetPositions(_bot);
        _bot.resetPose();
    }

    void onButtonPress(Bot& _bot) override {
        motion_state++;

        if (motion_state % 2 == 1) { // begin motion
            Serial.println("[Motion] Begin new navigation");
            _nav.start();
        }
    }

    void work(Bot& _bot) override {
        if (motion_state % 2 == 0) { // dont move when button not pressed or pressed twice
            _bot.setUserLEDToWaiting();
            _bot.setLeftSpeed(0);
            _bot.setRightSpeed(0);
            return;
        }

        // else move

        switch (_nav.state()) {
        case Navigation::State::Idle: // nothing to do
            _bot.setUserLEDToWaiting();
            break;
        case Navigation::State::Completed:
            if (_prev_state != Navigation::State::Completed)
                Serial.println("[Motion] completed navigation");
            _bot.setUserLEDToTaskFinished();
            break;
        case Navigation::State::Failed:
            if (_prev_state != Navigation::State::Failed) {
                Serial.println("[Motion] Navigation error");
                _nav.outputErrorToSerial();
            }
            _bot.setUserLEDToMotionError();
            break;
        default:
            _bot.setUserLEDToDefaultOperating();
            break;
        }

        _prev_state = _nav.state();
        _nav.run(_bot);
    }
};


}

#endif //SRC_HW_BASE_TESTS_H
