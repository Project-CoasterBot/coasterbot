#ifndef SRC_HW_WEBOTS_TEST_H
#define SRC_HW_WEBOTS_TEST_H

#include "hw_impl_hal.h"

#include "webots/coasterbot_functions.h"
#include "coaster_servo_config.h"

template<typename Bot>
class WebotsTest {
protected:
    CoasterbotFunctions* _coasterbot {nullptr};
    PoseEstimator *_pose {nullptr};
    SafetyMonitor *_safety {nullptr};

    bool btableLearned = false;
    bool bCoasterOut = false;
    float _lastPosePrint = -1.0f;
    CoasterbotFunctions::NavState _lastNavState = CoasterbotFunctions::NavState::IDLE;

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
        // the webots program commands real speeds (nav. 5 rad/s, slower near the target), not only a direction
        _bot.setSpeedScaling(5.0f);
        _coasterbot = new CoasterbotFunctions(_bot, *_pose, *_safety);
        _coasterbot->setServoAngles(boardCoasterServoAngles());
    }

    void loop(Bot& _bot) {
        // alias, dadurch bleibt die restliche funktion identisch zur webots implementierung
        auto& hal = _bot;
        auto& coasterbot = *_coasterbot;
        auto& pose = *_pose;
        //auto& safety = *_safety;

        // webot main loop:

        coasterbot.update();

        // Taster als Ereignis beim Loslassen, nicht als Zustand: ein gehaltener Taster darf nicht durch
        // mehrere Zustaende durchschalten (z.B. IDLE -> Ausgabe -> Not-Stopp -> IDLE -> Vermessen).
        // Wie BaseBotHandler: kurz = naechster Schritt, lang = laufende Fahrt abbrechen.
        unsigned press_ms = 0;
        const bool buttonReleased = _bot.userButtonReleased(press_ms);
        const bool buttonClicked = buttonReleased && press_ms <= LONG_PRESS_MS;
        const bool buttonLongPress = buttonReleased && press_ms > LONG_PRESS_MS;

        if (buttonLongPress && (state == COASTERBOT_SET_COASTERS || state == COASTERBOT_GET_COASTER)) {
            // Vermessung bleibt erhalten; bCoasterOut bleibt, der naechste kurze Druck startet dieselbe Routine neu
            coasterbot.abortRoutine();
            Serial.println("[TEST] long press -> routine aborted, back to idle");
            state = COASTERBOT_IDLE;
        }

        float now = hal.getTime();
        if (now - _lastPosePrint >= 2.0f) {
            printPose("[POSE]", now, pose);
            _lastPosePrint = now;
        }

        // jeder Wechsel des Navigationszustands (= LED-Farbe) mit Pose, Gyro und Fahrbefehl
        const auto navState = coasterbot.navState();
        if (navState != _lastNavState) {
            Serial.printf("[NAVSTATE] t=%.2f s %s -> %s  x=%.3f y=%.3f theta=%.1f deg  yaw=%.1f deg rate=%.1f deg/s"
                          "  cmd=(%.2f, %.2f) rad/s  gyro faults=%u\n",
                          now, CoasterbotFunctions::navStateName(_lastNavState),
                          CoasterbotFunctions::navStateName(navState),
                          pose.getX(), pose.getY(), pose.getTheta() * RAD_TO_DEG,
                          hal.getYaw() * RAD_TO_DEG, hal.getGyroZ() * RAD_TO_DEG,
                          hal.getCommandedSpeed(WHEEL_LEFT), hal.getCommandedSpeed(WHEEL_RIGHT),
                          _bot.inertialSensor().faultCount());
            _lastNavState = navState;
        }

        switch (state) {
            case COASTERBOT_SETUP:
                btableLearned = false;
                bCoasterOut = false;
                state = COASTERBOT_IDLE;
                break;

            case COASTERBOT_IDLE:
                // weiss blinkend: der Gyro ist seit dem Start mindestens einmal ausgefallen (siehe GyroSensor::loop)
                coasterbot.led(_bot.inertialSensor().faultCount() > 0 ? RobotHAL::LED_COLORS::BLINK_WHITE
                                                                      : RobotHAL::LED_COLORS::OFF);

                if (!buttonClicked) break;

                if (!btableLearned) {
                    state = COASTERBOT_LEARN_TABLE;
                } else if (!bCoasterOut) {
                    state = COASTERBOT_SET_COASTERS;
                } else {
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
                // keine Konsole auf dem Testfeld: LED zeigt den Navigationszustand (Farbtabelle: navLedColor())
                coasterbot.led(coasterbot.navLedColor());

                // Kanten sind kein Not-Stopp: SafetyMonitor setzt zurueck, die Navigation korrigiert die
                // Pose an der bekannten Kante und plant neu (CoasterbotFunctions::correctPoseAtEdge)
                if (coasterbot.coasterRoutineFailed()) {
                    enterEmergencyStop(false);
                } else if (bCoasterOut && coasterbot.goToCenter()) {
                    state = COASTERBOT_IDLE;
                }
                break;

            case COASTERBOT_GET_COASTER:
                if (bCoasterOut) {
                    bCoasterOut = coasterbot.runCollectCoasters();
                }
                // keine Konsole auf dem Testfeld: LED zeigt den Navigationszustand (Farbtabelle: navLedColor())
                coasterbot.led(coasterbot.navLedColor());

                // Kanten sind kein Not-Stopp: SafetyMonitor setzt zurueck, die Navigation korrigiert die
                // Pose an der bekannten Kante und plant neu (CoasterbotFunctions::correctPoseAtEdge)
                if (coasterbot.coasterRoutineFailed()) {
                    enterEmergencyStop(false);
                } else if (!bCoasterOut && coasterbot.goToCenter()) {
                    state = COASTERBOT_IDLE;
                }
                break;

            case COASTERBOT_EMERGENCY_STOP:
                // bleibt stehen, bis der Taster erneut gedrueckt und losgelassen wird:
                // rot = Kante (SafetyMonitor), rot blinkend = Ausgabe/Aufnahme fehlgeschlagen
                coasterbot.led(_emergencyBySafety ? RobotHAL::LED_COLORS::RED
                                                  : RobotHAL::LED_COLORS::BLINK_RED);
                coasterbot.stop();
                btableLearned = false;
                coasterbot.setTableLearned(false);

                if (buttonReleased) { // kurz oder lang, beides quittiert
                    state = COASTERBOT_IDLE;
                }
                break;
        }
    }

protected:
    static constexpr unsigned LONG_PRESS_MS {1500};
    bool _emergencyBySafety {false};

    void enterEmergencyStop(bool bySafety) {
        _emergencyBySafety = bySafety;
        state = COASTERBOT_EMERGENCY_STOP;
        Serial.printf("[TEST] emergency stop: %s\n", bySafety ? "safety monitor (edge)" : "coaster routine failed");
    }

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
