#ifndef SRC_HW_WEBOTS_TEST_H
#define SRC_HW_WEBOTS_TEST_H

#include "hw_impl_hal.h"

#include "webots/coasterbot_functions.h"
#include "coaster_servo_config.h"
#include "persistent_settings.h"

#include <cmath>

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
    PersistentSettings _settings;

    // Anzahl Coaster einstellen (COASTERBOT_SET_COUNT)
    int _countSelection = 0;
    float _countLastInput = 0.0f;
    int _countLedOn = -1; // zuletzt gesetzter Zustand des Blinkmusters, -1 = noch keiner
    // kurze Rueckmeldung im Ruhezustand: weiss = Anzahl uebernommen, rot = Einstellen nicht moeglich
    float _ledFeedbackUntil = 0.0f;
    RobotHAL::LED_COLORS _ledFeedbackColor = RobotHAL::LED_COLORS::OFF;

    enum states {
        COASTERBOT_SETUP,
        COASTERBOT_IDLE,
        COASTERBOT_LEARN_TABLE,
        COASTERBOT_SET_COASTERS,
        COASTERBOT_GET_COASTER,
        COASTERBOT_EMERGENCY_STOP,
        COASTERBOT_SET_COUNT
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
        _coasterbot->setActiveCoasters(_settings.coasterCount(CoasterbotFunctions::maxCoasters(),
                                                              CoasterbotFunctions::maxCoasters()));
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
        bool buttonLongPress = buttonReleased && press_ms > LONG_PRESS_MS;

        if (buttonLongPress && (state == COASTERBOT_SET_COASTERS || state == COASTERBOT_GET_COASTER)) {
            // Vermessung bleibt erhalten; bCoasterOut bleibt, der naechste kurze Druck startet dieselbe Routine neu
            coasterbot.abortRoutine();
            Serial.println("[TEST] long press -> routine aborted, back to idle");
            state = COASTERBOT_IDLE;
            buttonLongPress = false; // verbraucht, im Ruhezustand nicht gleich das Einstellen oeffnen
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
                if (now < _ledFeedbackUntil) {
                    coasterbot.led(_ledFeedbackColor);
                } else {
                    // weiss blinkend: der Gyro ist seit dem Start mindestens einmal ausgefallen (siehe GyroSensor::loop)
                    coasterbot.led(_bot.inertialSensor().faultCount() > 0 ? RobotHAL::LED_COLORS::BLINK_WHITE
                                                                          : RobotHAL::LED_COLORS::OFF);
                }

                if (buttonLongPress) {
                    // Anzahl nur aendern, solange kein Coaster auf dem Tisch liegt
                    if (bCoasterOut || coasterbot.coastersSpent() > 0) {
                        Serial.println("[COUNT] coasters are out, collect them before changing the count");
                        showFeedback(now, RobotHAL::LED_COLORS::RED);
                        break;
                    }
                    _countSelection = coasterbot.activeCoasters();
                    _countLastInput = now;
                    _countLedOn = -1;
                    Serial.printf("[COUNT] set coaster count, current %d (short press = +1, long press = save)\n",
                                  _countSelection);
                    state = COASTERBOT_SET_COUNT;
                    break;
                }

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

            case COASTERBOT_SET_COUNT:
                if (buttonClicked) {
                    _countSelection = _countSelection % CoasterbotFunctions::maxCoasters() + 1; // 1..max, dann wieder 1
                    _countLastInput = now;
                    _countLedOn = -1; // Muster neu beginnen, damit die neue Zahl von vorn geblinkt wird
                    Serial.printf("[COUNT] %d\n", _countSelection);
                }
                if (buttonLongPress || now - _countLastInput >= COUNT_TIMEOUT_S) {
                    coasterbot.setActiveCoasters(_countSelection);
                    _settings.saveCoasterCount(_countSelection);
                    Serial.printf("[COUNT] saved: %d coasters\n", _countSelection);
                    showFeedback(now, RobotHAL::LED_COLORS::WHITE);
                    state = COASTERBOT_IDLE;
                    break;
                }
                showCountPattern(coasterbot, now);
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
    static constexpr float COUNT_TIMEOUT_S = 10.0f;   // ohne Tastendruck wird die Anzahl uebernommen
    static constexpr float COUNT_BLINK_S = 0.4f;      // ein Blink: 0.2 s an, 0.2 s aus
    static constexpr float COUNT_PAUSE_S = 1.2f;      // Pause nach N Blinks
    static constexpr float FEEDBACK_S = 1.0f;

    static RobotHAL::LED_COLORS countColor(int count) {
        switch (count) {
        case 1: return RobotHAL::LED_COLORS::RED;
        case 2: return RobotHAL::LED_COLORS::YELLOW;
        case 3: return RobotHAL::LED_COLORS::GREEN;
        default: return RobotHAL::LED_COLORS::BLUE;
        }
    }

    /// N Blinks in der Farbe der Anzahl, dann Pause, wiederholt ab der letzten Eingabe.
    void showCountPattern(CoasterbotFunctions& coasterbot, float now) {
        const float period = _countSelection * COUNT_BLINK_S + COUNT_PAUSE_S;
        const float phase = std::fmod(now - _countLastInput, period);
        const bool on = phase < _countSelection * COUNT_BLINK_S && std::fmod(phase, COUNT_BLINK_S) < COUNT_BLINK_S / 2.0f;
        if (static_cast<int>(on) == _countLedOn) return;
        _countLedOn = on;
        coasterbot.led(on ? countColor(_countSelection) : RobotHAL::LED_COLORS::OFF);
    }

    void showFeedback(float now, RobotHAL::LED_COLORS color) {
        _ledFeedbackColor = color;
        _ledFeedbackUntil = now + FEEDBACK_S;
    }
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
