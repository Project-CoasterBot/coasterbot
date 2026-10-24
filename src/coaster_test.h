#ifndef SRC_COASTER_TEST_H
#define SRC_COASTER_TEST_H

#include <Arduino.h>
#include <cstring>
#include <memory>

#include "boardconfig.h"
#include "coaster_mechanism_hal.h"
#include "coaster_servo_config.h"
#include "serial_line_reader.h"
#include "webots/coasterbot_functions.h"

/** Interaktiver Test von Ausgabe und Aufnahme der Coaster ueber die serielle Konsole.
 *
 * Ersetzt die normale Firmware, wenn mit COASTERBOT_COASTER_TEST gebaut wird (siehe main.cpp,
 * platformio.ini Umgebung pico2_coaster_test oder das Projekt coaster_test/). Der Bot faehrt nicht,
 * Sensoren sind aus. Die Ablaeufe sind die der Firmware (CoasterbotFunctions::spendCoaster() und
 * getCoaster()), mit den Servowinkeln aus boardconfig(_local).h.
 *
 * Zuerst fragt der Test, wie viele Coaster im Stapel sind, danach koennen einzelne oder mehrere
 * Coaster ausgegeben bzw. aufgenommen werden. Mit "x" wird ein laufender Ablauf abgebrochen und der
 * Mechanismus faehrt in die Ruhestellung.
 */
class CoasterTest {
    CoasterMechanismHAL _hal;
    std::unique_ptr<PoseEstimator> _pose;
    std::unique_ptr<SafetyMonitor> _safety;
    std::unique_ptr<CoasterbotFunctions> _coasterbot;
    SerialLineReader _input;

    enum class Step : uint8_t { WAIT_CONSOLE, ASK_COUNT, MENU, SPENDING, COLLECTING, PARKING };
    Step _step {Step::WAIT_CONSOLE};
    int _remaining {0}; // Coaster, die im laufenden Auftrag noch auszugeben bzw. aufzunehmen sind

public:

    void begin() {
        _hal.initializeHardware();
        _hal.ledBoard().setModeToBlinkPurple();

        _pose = std::make_unique<PoseEstimator>(_hal);
        _safety = std::make_unique<SafetyMonitor>(_hal);
        _coasterbot = std::make_unique<CoasterbotFunctions>(_hal, *_pose, *_safety);
        _coasterbot->setServoAngles(boardCoasterServoAngles());
    }

    void loop() {
        _hal.step();

        if (_step == Step::WAIT_CONSOLE) {
            if (!Serial) return; // Konsole noch nicht verbunden
            printIntro();
            setStep(Step::ASK_COUNT);
            return;
        }

        if (const char* line = _input.poll()) handleLine(line);
        updateRoutine();
    }

protected:

    // Kapazitaet aus der Normierung: vermessene plus extrapolierte Stufen
    int capacity() const { return _coasterbot->servoAngles().stackCapacity(); }
    int stacked() const { return _coasterbot->coastersInStack(); }

    void setStep(Step step) {
        _step = step;
        prompt();
    }

    void printIntro() {
        Serial.println();
        Serial.println("=== CoasterBot Coaster-Test ===");
        Serial.println("Der Bot faehrt nicht, Sensoren sind aus.");
        const auto& a = _coasterbot->servoAngles();
        Serial.printf("Servos: Heber GP%d (0-Hub %d, max. Hub %d Grad), Auswurfarm GP%d (Ruhe %d, Auswurf %d Grad, %s)\n",
                      board::PIN_SERVO_LIFTER, a.lifterRest, a.lifterMax,
                      board::PIN_SERVO_SPENDER, a.spenderIdle, a.spenderEject,
                      a.spenderEjectStrike ? "Schlag" : "Rampe");
        Serial.print("Heber je Stapelhoehe:");
        for (int n = 1; n <= capacity(); ++n)
            Serial.printf(" %d:%d%s", n, a.stackAngle(n), n > a.levels() ? "*" : "");
        Serial.println(extrapolatedNote(a));
        Serial.println("Jederzeit: x = Abbruch, Mechanismus in Ruhestellung, ? = Hilfe");
    }

    static const char* extrapolatedNote(const CoasterServoAngles& a) {
        return a.stackCapacity() > a.levels() ? "  (* = extrapoliert)" : "";
    }

    void prompt() {
        Serial.println();
        switch (_step) {
        case Step::ASK_COUNT:
            _hal.ledBoard().setModeToBlinkPurple();
            Serial.printf("Wie viele Coaster sind im Stapel? (0-%d)\n", capacity());
            break;
        case Step::MENU:
            _hal.ledBoard().setModeToConstantGreen();
            Serial.printf("Stapel: %d von %d Coastern.\n", stacked(), capacity());
            Serial.println("      aus [N] = N Coaster ausgeben (Standard 1)");
            Serial.println("      auf [N] = N Coaster aufnehmen (Standard 1)");
            Serial.println("      z = Anzahl im Stapel neu eingeben");
            break;
        case Step::SPENDING:
            _hal.ledBoard().setModeToBlinkCyan();
            Serial.printf("Gebe den untersten von %d Coastern aus, Heber auf %d Grad ...\n",
                          stacked(), _coasterbot->servoAngles().stackAngle(stacked()));
            return; // keine Eingabeaufforderung, laeuft von selbst
        case Step::COLLECTING:
            _hal.ledBoard().setModeToBlinkBlue();
            Serial.printf("Nehme Coaster %d auf, Heber auf %d Grad ...\n",
                          stacked() + 1, _coasterbot->servoAngles().stackAngle(stacked() + 1));
            return;
        case Step::PARKING:
            _hal.ledBoard().setModeToBlinkOrange();
            Serial.println("Mechanismus faehrt in die Ruhestellung ...");
            return;
        default:
            break;
        }
        Serial.print("> ");
    }

    /// Optionale Anzahl hinter dem Befehl, z.B. "aus 3". Liefert false bei ungueltiger Eingabe.
    static bool parseCount(const char* args, int& count) {
        while (*args == ' ') ++args;
        if (*args == '\0') { count = 1; return true; }
        return SerialLineReader::parseInt(args, count) && count > 0;
    }

    void handleLine(const char* line) {
        if (strcmp(line, "x") == 0) {
            abort();
            return;
        }
        if (strcmp(line, "?") == 0) {
            printIntro();
            prompt();
            return;
        }

        switch (_step) {
        case Step::ASK_COUNT: {
            int count = 0;
            if (!SerialLineReader::parseInt(line, count) || count < 0 || count > capacity()) {
                Serial.printf("Bitte eine Zahl von 0 bis %d eingeben.\n", capacity());
                prompt();
                return;
            }
            _coasterbot->setCoastersInStack(count);
            setStep(Step::MENU);
            break;
        }
        case Step::MENU:
            handleMenu(line);
            break;
        default:
            Serial.println("Ablauf laeuft, x = Abbruch");
            break;
        }
    }

    void handleMenu(const char* line) {
        int count = 0;
        if (strcmp(line, "z") == 0) {
            setStep(Step::ASK_COUNT);
        } else if (strncmp(line, "aus", 3) == 0 && parseCount(line + 3, count)) {
            if (stacked() == 0) {
                Serial.println("Der Stapel ist leer, nichts auszugeben.");
                prompt();
                return;
            }
            _remaining = min(count, stacked());
            if (_remaining < count) Serial.printf("Nur %d Coaster im Stapel.\n", _remaining);
            setStep(Step::SPENDING);
        } else if (strncmp(line, "auf", 3) == 0 && parseCount(line + 3, count)) {
            const int free_slots = capacity() - stacked();
            if (free_slots == 0) {
                Serial.println("Der Stapel ist voll, nichts aufzunehmen.");
                prompt();
                return;
            }
            _remaining = min(count, free_slots);
            if (_remaining < count) Serial.printf("Nur %d Plaetze frei.\n", _remaining);
            setStep(Step::COLLECTING);
        } else {
            prompt();
        }
    }

    void updateRoutine() {
        switch (_step) {
        case Step::SPENDING:
            if (!_coasterbot->spendFromStack()) return;
            routineStepDone("ausgegeben");
            break;
        case Step::COLLECTING:
            if (!_coasterbot->collectToStack()) return;
            routineStepDone("aufgenommen");
            break;
        case Step::PARKING:
            if (_hal.servosMoving()) return;
            Serial.println("Ruhestellung erreicht. Die Anzahl im Stapel ist nach dem Abbruch unklar.");
            setStep(Step::ASK_COUNT);
            break;
        default:
            break;
        }
    }

    void routineStepDone(const char* what) {
        Serial.printf("Coaster %s, Stapel: %d von %d.\n", what, stacked(), capacity());
        if (--_remaining > 0) {
            prompt(); // naechster Coaster im selben Schritt
            return;
        }
        setStep(Step::MENU);
    }

    void abort() {
        _coasterbot->abortRoutine();
        _remaining = 0;
        if (_step != Step::SPENDING && _step != Step::COLLECTING) {
            prompt();
            return;
        }
        Serial.println("Abgebrochen.");
        const auto& a = _coasterbot->servoAngles();
        // Arm in die Ruhestellung, Heber einfahren (beides entfernt sich vom Stapel)
        _hal.setServoPosition(1, a.spenderIdle, 1000);
        _hal.setServoPosition(0, a.lifterRest, 1500);
        setStep(Step::PARKING);
    }
};

#endif // SRC_COASTER_TEST_H
