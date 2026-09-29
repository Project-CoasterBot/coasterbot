#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "coasterbot_functions.h"
#include "webots_hal.h"

enum states {
    COASTERBOT_SETUP,
    COASTERBOT_IDLE,
    COASTERBOT_LEARN_TABLE,
    COASTERBOT_SET_COASTERS,
    COASTERBOT_GET_COASTER,
    COASTERBOT_EMERGENCY_STOP
};

static void printPose(const char* tag, float t, const PoseEstimator& pose) {
    std::cout << tag << " t=" << t << "s  pose x=" << pose.getX()
              << " y=" << pose.getY()
              << " theta=" << pose.getTheta() * 180.0f / static_cast<float>(M_PI)
              << " deg  odo=" << pose.getOdometer() << " m" << std::endl;
}

int main() {
    WebotsHAL hal;
    PoseEstimator pose(hal);
    SafetyMonitor safety(hal);
    CoasterbotFunctions coasterbot(hal, pose, safety);

    bool btableLearned = false;
    bool bCoasterOut = false;
    states state = COASTERBOT_SETUP;
    float _lastPosePrint = -1.0f;
    while (hal.step()) {
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

    return 0;
}
