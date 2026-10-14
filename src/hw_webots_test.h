#ifndef SRC_HW_WEBOTS_TEST_H
#define SRC_HW_WEBOTS_TEST_H

#include "hw_impl_hal.h"

#include "webots/coasterbot_functions.h"

template<typename Bot>
class WebotsTest {
protected:
    CoasterbotFunctions* _coasterbot {nullptr};
    PoseEstimator *_pose {nullptr};
    SafetyMonitor *_safety {nullptr};

    bool btableLearned = false;
    bool bCoasterOut = false;
    float _lastPosePrint = -1.0f;

    enum states {
        COASTERBOT_SETUP,
        COASTERBOT_IDLE,
        COASTERBOT_LEARN_TABLE,
        COASTERBOT_SET_COASTERS,
        COASTERBOT_GET_COASTER,
        COASTERBOT_EMERGENCY_STOP
    } state = COASTERBOT_SETUP;

public:

    void begin(Bot& _bot) {
        // TT-Raeder 6.7 cm (wie MotionDetectionOptoCoupling), nicht die Webots-Raeder
        _pose = new PoseEstimator(_bot, 0.067f / 2.0f, 0.202f);
        _safety = new SafetyMonitor(_bot);
        _coasterbot = new CoasterbotFunctions(_bot, *_pose, *_safety);
    }

    void loop(Bot& _bot) {
        // alias, dadurch bleibt die restliche funktion identisch zur webots implementierung
        auto& hal = _bot;
        auto& coasterbot = *_coasterbot;
        auto& pose = *_pose;
        //auto& safety = *_safety;

        // webot main loop:

        coasterbot.update();

        float now = hal.getTime();
        if (now - _lastPosePrint >= 2.0f) {
            printPose("[POSE]", now, pose);
            _lastPosePrint = now;
        }

        switch (state) {
            case COASTERBOT_SETUP:
                btableLearned = false;
                bCoasterOut = false;
                state = COASTERBOT_IDLE;
                break;

            case COASTERBOT_IDLE:
                coasterbot.led(RobotHAL::LED_COLORS::OFF);

                //nur für die Simulation
                if(!coasterbot.blocklessWait(5.0f)) break;

                if (!btableLearned && coasterbot.buttonPressed()) {
                    state = COASTERBOT_LEARN_TABLE;
                } else if (btableLearned && !bCoasterOut && coasterbot.buttonPressed()) {
                    state = COASTERBOT_SET_COASTERS;
                } else if (btableLearned && bCoasterOut && coasterbot.buttonPressed()) {
                    state = COASTERBOT_GET_COASTER;
                }
                break;

            case COASTERBOT_LEARN_TABLE:
                coasterbot.runLearnTable();
                coasterbot.led(RobotHAL::LED_COLORS::ORANGE);
                if (coasterbot.isTableLearned()) {
                    btableLearned = true;
                    state = COASTERBOT_IDLE;
                }
                break;

            case COASTERBOT_SET_COASTERS:
                if (!bCoasterOut) {
                    bCoasterOut = coasterbot.runSpendCoasters();
                }
                coasterbot.led(RobotHAL::LED_COLORS::GREEN);

                if (coasterbot.safetyOverriding() || coasterbot.coasterRoutineFailed()) {
                    state = COASTERBOT_EMERGENCY_STOP;
                } else if (bCoasterOut && coasterbot.goToCenter()) {
                    state = COASTERBOT_IDLE;
                }
                break;

            case COASTERBOT_GET_COASTER:
                if (bCoasterOut) {
                    bCoasterOut = coasterbot.runCollectCoasters();
                }
                coasterbot.led(RobotHAL::LED_COLORS::BLUE);

                if (coasterbot.safetyOverriding() || coasterbot.coasterRoutineFailed()) {
                    state = COASTERBOT_EMERGENCY_STOP;
                } else if (!bCoasterOut && coasterbot.goToCenter()) {
                    state = COASTERBOT_IDLE;
                }
                break;

            case COASTERBOT_EMERGENCY_STOP:
                coasterbot.led(RobotHAL::LED_COLORS::RED);
                coasterbot.stop();
                btableLearned = false;
                coasterbot.setTableLearned(false);

                if (coasterbot.buttonPressed()) {
                    state = COASTERBOT_IDLE;
                }
                break;
        }
    }

protected:
    void printPose(const char* tag, float t, const PoseEstimator& pose) {
        Serial.print(tag);
        Serial.print(" t= ");
        Serial.print(t);
        Serial.print(" s\t x= ");
        Serial.print(pose.getX());
        Serial.print("\t y= ");
        Serial.print(pose.getY());
        Serial.print("\t theta= ");
        Serial.print(pose.getTheta() * 180.0f / static_cast<float>(M_PI));
        Serial.print(" deg\t odo= ");
        Serial.print(pose.getOdometer());
        Serial.println(" ");
    }

};

#endif //SRC_HW_WEBOTS_TEST_H
