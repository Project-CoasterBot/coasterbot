#ifndef SRC_SERVO_CALIBRATION_H
#define SRC_SERVO_CALIBRATION_H

#include <Arduino.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "boardconfig.h"
#include "ledinterface.h"
#include "servointerface.h"
#include "serial_line_reader.h"
#include "webots/coaster_servo_angles.h"

/** Interaktive Normierung der Coaster-Servos ueber die serielle Konsole.
 *
 * Ersetzt die normale Firmware, wenn mit COASTERBOT_SERVO_CALIBRATION gebaut wird (siehe main.cpp,
 * platformio.ini Umgebung pico2_servo_calibration). Der Bot faehrt nicht (Motortreiber bleibt im
 * Standby), Sensoren werden nicht initialisiert.
 *
 * Ablauf:
 *  1. Auswurfarm identifizieren: die Servos werden einzeln eingeschaltet und bewegen sich leicht um
 *     ihren Startwinkel, der Nutzer meldet, was sich bewegt hat. Ein als Heber gemeldeter Servo wird
 *     sofort wieder losgelassen, damit er den Arm nicht ausdruecken kann.
 *  2. Auswurfarm normieren: Auswurfposition (ganz ausgeworfen) und Ruhestellung (ganz hinten, Stapel
 *     frei) anfahren und bestaetigen, danach haelt der Arm die Ruhestellung.
 *  3. Heber normieren: 0-Hub (ganz eingefahren) und maximalen Hub (ganz ausgefahren).
 *  4. Stapelhoehen: der Nutzer legt nacheinander Coaster ein und stellt den Heber jeweils so ein, dass
 *     der unterste ausgeworfen werden kann und die anderen drinbleiben, optional mit Testauswurf. Hat
 *     er keine Coaster mehr, wird die Skala mit dem letzten Abstand bis zur Ruhestellung extrapoliert.
 *  5. Ausgabe der Werte als #define-Zeilen fuer boardconfig_local.h.
 *
 * Mit "x" werden jederzeit beide Servos losgelassen und der Ablauf beginnt von vorn.
 */
class ServoCalibration {
    static constexpr int SERVO_COUNT = 2;
    static constexpr int WIGGLE_AMPLITUDE = 10;               // [deg] Ausschlag zur Identifikation
    static constexpr unsigned long WIGGLE_MS_PER_DEGREE = 30; // langsam, damit gut zu erkennen
    static constexpr unsigned long JOG_MS_PER_DEGREE = 15;    // Rampe beim Anfahren der Positionen
    // Testauswurf "Schlag"/"Haemmern": Arm ohne Rampe, also mit voller Servogeschwindigkeit, damit er mit
    // Schwung auf den Coaster trifft. Die Haltezeiten muessen die Fahrzeit des Servos abdecken (SG90 ~0.1 s/60 Grad).
    static constexpr unsigned long STRIKE_HOLD_MS = 500;       // nach dem Schlag in Auswurfposition druecken
    static constexpr unsigned long BACKOFF_HOLD_MS = 250;      // nach dem Zurueckfahren vor dem naechsten Schlag
    static constexpr int HAMMER_DEFAULT_STRIKES = 5;
    static constexpr int HAMMER_DEFAULT_BACKOFF = 25;          // [deg] Richtung Ruhestellung
    static constexpr int HAMMER_MAX_STRIKES = 20;

    // Index 0/1 = Anschluss laut boardconfig, welche Rolle er hat, ermittelt erst die Normierung
    ServoController<board::PIN_SERVO_LIFTER> _servo0;
    ServoController<board::PIN_SERVO_SPENDER> _servo1;
    LEDController<board::PIN_LED1_R, board::PIN_LED1_G, board::PIN_LED1_B> _led;

    enum class Role : uint8_t { UNKNOWN, NONE, LIFTER, SPENDER };

    enum class Step : uint8_t {
        WAIT_CONSOLE,
        IDENT_START, IDENT_WIGGLE, IDENT_ASK,
        ARM_EJECT, ARM_IDLE,
        LIFTER_START, LIFTER_REST, LIFTER_MAX,
        STACK_PLACE, STACK_ADJUST, STACK_TEST,
        LIFTER_PARK,
        DONE
    };
    Step _step {Step::WAIT_CONSOLE};

    bool _started[SERVO_COUNT] {false, false};
    int _cmd[SERVO_COUNT] {0, 0}; // zuletzt kommandierter Winkel, ServoController rundet beim Zurueckrechnen
    Role _role[SERVO_COUNT] {Role::UNKNOWN, Role::UNKNOWN};
    int _candidate {1};           // Servo, der gerade identifiziert wird
    int _arm {-1};                // Index des Auswurfarms, -1 = noch nicht identifiziert

    int _wiggle_targets[5] {};
    int _wiggle_index {0};

    int _spender_idle {board::SERVO_SPENDER_ANGLE_IDLE};
    int _spender_eject {board::SERVO_SPENDER_ANGLE_EJECT};
    int _lifter_rest {board::SERVO_LIFTER_ANGLE_REST};
    int _lifter_max {board::SERVO_LIFTER_ANGLE_MAX};

    CoasterServoAngles _stack;  // vermessene Stapelhoehen, _stack.lifterStackLevels = bisher vermessen
    int _stack_n {1};           // Stapelhoehe, die gerade vermessen wird
    // Testauswurf als Folge von Armbewegungen, abgearbeitet in updateMotion()
    struct ArmMove {
        int angle;
        unsigned long ms_per_degree; // 0 = ohne Rampe
        unsigned long hold_ms;       // danach so lange stehen bleiben
    };
    ArmMove _seq[2 * HAMMER_MAX_STRIKES + 1] {};
    int _seq_len {0};
    int _seq_index {0};
    unsigned long _seq_hold_start {0};
    bool _seq_holding {false};
    bool _place_waiting {false}; // Heber ist zum Einlegen ganz ausgefahren, wartet auf Enter

    SerialLineReader _input;

public:

    void begin() {
        // Motortreiber sicher aus, unabhaengig vom Reset-Zustand der Pins
        pinMode(board::PIN_MOTOR_STANDBY, OUTPUT);
        digitalWrite(board::PIN_MOTOR_STANDBY, LOW);

        _led.begin();
        _led.setModeToBlinkPurple();
        // Servos bleiben aus, bis der Nutzer sie einzeln einschaltet
    }

    void loop() {
        _servo0.update();
        _servo1.update();
        _led.update();

        if (_step == Step::WAIT_CONSOLE) {
            if (!Serial) return; // Konsole noch nicht verbunden
            printIntro();
            restart();
            return;
        }

        if (const char* line = _input.poll()) handleLine(line);
        updateMotion();
    }

protected: // Servozugriff ueber den Index

    int lifter() const { return 1 - _arm; }

    static int pinOf(int i) { return i == 0 ? board::PIN_SERVO_LIFTER : board::PIN_SERVO_SPENDER; }

    static int clampAngle(int angle) { return angle < 0 ? 0 : (angle > 180 ? 180 : angle); }

    /// Servo einschalten. Beim ersten Mal springt er auf den Winkel, danach wird gerampt.
    bool servoStart(int i, int angle) {
        angle = clampAngle(angle);
        if (!_started[i]) {
            const bool ok = (i == 0) ? _servo0.begin(angle) : _servo1.begin(angle);
            if (!ok) {
                Serial.printf("FEHLER: Servo an GP%d konnte nicht gestartet werden (kein PIO frei)\n", pinOf(i));
                return false;
            }
            _started[i] = true;
            _cmd[i] = angle;
            return true;
        }
        servoMove(i, angle, JOG_MS_PER_DEGREE);
        return true;
    }

    void servoMove(int i, int angle, unsigned long ms_per_degree) {
        angle = clampAngle(angle);
        const unsigned long duration = static_cast<unsigned long>(abs(angle - _cmd[i])) * ms_per_degree;
        _cmd[i] = angle;
        if (i == 0) _servo0.moveTo(angle, duration);
        else _servo1.moveTo(angle, duration);
    }

    bool servoMoving(int i) const { return i == 0 ? _servo0.isMoving() : _servo1.isMoving(); }

    void servoRelease(int i) {
        if (i == 0) _servo0.release();
        else _servo1.release();
    }

    void releaseAll() {
        servoRelease(0);
        servoRelease(1);
    }

protected: // Ablauf

    void restart() {
        releaseAll();
        _role[0] = _role[1] = Role::UNKNOWN;
        _arm = -1;
        _candidate = 1; // zuerst der Anschluss, der laut boardconfig der Auswurfarm ist
        _stack.lifterStackLevels = 0;
        _stack_n = 1;
        _place_waiting = false;
        _led.setModeToBlinkPurple();
        setStep(Step::IDENT_START);
    }

    void setStep(Step step) {
        _step = step;
        prompt();
    }

    void startWiggle(int i) {
        const int center = _cmd[i];
        const int lo = clampAngle(center - WIGGLE_AMPLITUDE);
        const int hi = clampAngle(center + WIGGLE_AMPLITUDE);
        const int targets[5] = { hi, lo, hi, lo, center };
        memcpy(_wiggle_targets, targets, sizeof(targets));
        _wiggle_index = 0;
        servoMove(i, _wiggle_targets[0], WIGGLE_MS_PER_DEGREE);
        _step = Step::IDENT_WIGGLE;
        Serial.printf("GP%d bewegt sich ...\n", pinOf(_candidate));
    }

    /// Bewegungen, die ohne Eingabe weiterlaufen.
    void updateMotion() {
        switch (_step) {
        case Step::IDENT_WIGGLE:
            if (servoMoving(_candidate)) return;
            if (++_wiggle_index < 5) {
                servoMove(_candidate, _wiggle_targets[_wiggle_index], WIGGLE_MS_PER_DEGREE);
                return;
            }
            setStep(Step::IDENT_ASK);
            break;
        case Step::STACK_TEST:
            if (servoMoving(_arm)) return;
            if (!_seq_holding) {
                _seq_holding = true;
                _seq_hold_start = millis();
            }
            if (millis() - _seq_hold_start < _seq[_seq_index].hold_ms) return;
            if (++_seq_index < _seq_len) {
                startArmMove();
                return;
            }
            Serial.println("Testauswurf fertig. Ausgeworfenen Coaster wieder einlegen, wenn die Stufe noch nicht passt.");
            setStep(Step::STACK_ADJUST);
            break;
        case Step::LIFTER_PARK:
            if (servoMoving(lifter())) return;
            _led.setModeToConstantGreen();
            setStep(Step::DONE);
            break;
        default:
            break;
        }
    }

    void prompt() {
        Serial.println();
        switch (_step) {
        case Step::IDENT_START:
            Serial.printf("[1/4] Auswurfarm identifizieren: Servo an GP%d wird bei %d Grad eingeschaltet\n"
                          "      und bewegt sich um +-%d Grad.\n"
                          "      Enter = starten, Zahl = anderer Startwinkel\n",
                          pinOf(_candidate), startAngleFor(_candidate), WIGGLE_AMPLITUDE);
            break;
        case Step::IDENT_ASK:
            Serial.printf("Was hat sich an GP%d bewegt?\n"
                          "      a = Auswurfarm, h = Heber, n = nichts, w = nochmal bewegen\n", pinOf(_candidate));
            break;
        case Step::ARM_EJECT:
            Serial.printf("[2/5] Auswurfarm (GP%d) ganz auswerfen: so weit, dass ein Coaster sicher\n"
                          "      ausgeschoben ist. Aktuell %d Grad.\n", pinOf(_arm), _cmd[_arm]);
            printJogHelp();
            break;
        case Step::ARM_IDLE:
            Serial.printf("[2/5] Auswurfarm (GP%d) ganz nach hinten in die Ruhestellung: der Arm darf den\n"
                          "      Stapel und den Heber nicht blockieren. Aktuell %d Grad.\n", pinOf(_arm), _cmd[_arm]);
            printJogHelp();
            break;
        case Step::LIFTER_START:
            Serial.printf("[3/5] Heber an GP%d wird bei %d Grad eingeschaltet, der Auswurfarm haelt\n"
                          "      die Ruhestellung.\n"
                          "      Enter = starten, Zahl = anderer Startwinkel\n",
                          pinOf(lifter()), startAngleFor(lifter()));
            break;
        case Step::LIFTER_REST:
            Serial.printf("[3/5] Heber (GP%d) auf 0-Hub fahren: ganz eingefahren, Ruhestellung im Bot.\n"
                          "      Aktuell %d Grad.\n", pinOf(lifter()), _cmd[lifter()]);
            printJogHelp();
            break;
        case Step::LIFTER_MAX:
            Serial.printf("[3/5] Heber (GP%d) auf maximalen Hub fahren (ganz ausgefahren): langsam annaehern, nicht gegen den\n"
                          "      Anschlag oder den Auswurfarm druecken. Aktuell %d Grad.\n",
                          pinOf(lifter()), _cmd[lifter()]);
            printJogHelp();
            break;
        case Step::STACK_PLACE:
            if (_stack_n == 1)
                Serial.printf("[4/5] Stapelhoehen: Heber steht ganz ausgefahren (%d Grad).\n"
                              "      Coaster 1 einlegen, dann Enter.\n", _cmd[lifter()]);
            else
                Serial.printf("[4/5] Coaster %d einlegen (Stapel dann %d Coaster).\n"
                              "      Enter = Heber faehrt zum Einlegen ganz aus, w = Heber bleibt bei %d Grad,\n"
                              "      f = fertig, keine Coaster mehr (Rest wird extrapoliert)\n",
                              _stack_n, _stack_n, _cmd[lifter()]);
            break;
        case Step::STACK_ADJUST:
            Serial.printf("[4/5] Stapel mit %d Coaster%s: Heber so einstellen, dass der unterste ausgeworfen\n"
                          "      werden kann%s. Aktuell %d Grad.\n",
                          _stack_n, _stack_n == 1 ? "" : "n",
                          _stack_n == 1 ? "" : " und die anderen drinbleiben", _cmd[lifter()]);
            Serial.println("      a = Testauswurf: Arm einmal ganz auswerfen und zurueck in die Ruhestellung");
            Serial.println("      as = Schlag: wie a, aber ohne Rampe, der Arm trifft mit Schwung");
            Serial.printf("      ah [N] [W] = Haemmern: N Schlaege (Standard %d), dazwischen W Grad zurueck (Standard %d)\n",
                          HAMMER_DEFAULT_STRIKES, HAMMER_DEFAULT_BACKOFF);
            printJogHelp();
            break;
        case Step::STACK_TEST:
            return; // startEjectTest() meldet selbst, was laeuft
        case Step::LIFTER_PARK:
            Serial.printf("Heber faehrt zurueck auf 0-Hub (%d Grad).\n", _lifter_rest);
            break;
        case Step::DONE:
            printResult();
            Serial.println("Enter = Normierung wiederholen");
            break;
        default:
            break;
        }
        Serial.print("> ");
    }

    void printIntro() {
        Serial.println();
        Serial.println("=== CoasterBot Servo-Normierung ===");
        Serial.println("Der Bot faehrt nicht, Sensoren sind aus.");
        Serial.println("Vorher alle Coaster aus dem Mechanismus nehmen!");
        Serial.println("Jederzeit: x = beide Servos loslassen und neu beginnen, ? = Hilfe");
    }

    void printJogHelp() {
        Serial.println("      + / - = 1 Grad, +N / -N = N Grad, Zahl = absoluter Winkel, ok = uebernehmen");
    }

    void printResult() {
        Serial.println("[5/5] Ergebnis, in boardconfig_local.h uebernehmen:");
        Serial.println();
        Serial.printf("#define BOARD_PIN_SERVO_LIFTER %d\n", pinOf(lifter()));
        Serial.printf("#define BOARD_PIN_SERVO_SPENDER %d\n", pinOf(_arm));
        Serial.printf("#define BOARD_SERVO_LIFTER_ANGLE_REST %d\n", _lifter_rest);
        Serial.printf("#define BOARD_SERVO_LIFTER_ANGLE_MAX %d\n", _lifter_max);
        Serial.printf("#define BOARD_SERVO_SPENDER_ANGLE_IDLE %d\n", _spender_idle);
        Serial.printf("#define BOARD_SERVO_SPENDER_ANGLE_EJECT %d\n", _spender_eject);
        Serial.print("#define BOARD_SERVO_LIFTER_STACK_ANGLES");
        for (int i = 0; i < _stack.lifterStackLevels; ++i)
            Serial.printf("%s %d", i == 0 ? "" : ",", _stack.lifterStack[i]);
        Serial.println();
        Serial.println();

        Serial.printf("Heber je Stapelhoehe (* = extrapoliert), Kapazitaet %d Coaster:\n", _stack.stackCapacity());
        for (int n = 1; n <= _stack.stackCapacity(); ++n)
            Serial.printf("  %2d Coaster: %3d Grad%s\n", n, _stack.stackAngle(n), n > _stack.levels() ? " *" : "");
        if (_stack.lifterStackLevels < 2)
            Serial.println("HINWEIS: nur eine Stufe vermessen, ohne zweite Stufe wird nicht extrapoliert");
        else if (_stack.extrapolationStep() == 0)
            Serial.println("WARNUNG: die Stufen laufen nicht Richtung Ruhestellung, es wird nicht extrapoliert");
        Serial.println();
        if (_lifter_rest == _lifter_max) Serial.println("WARNUNG: Heber 0-Hub und maximaler Hub sind gleich");
        if (_spender_idle == _spender_eject) Serial.println("WARNUNG: Auswurfarm Ruhestellung und Auswurf sind gleich");
    }

    /// Startwinkel = konfigurierte Ruhestellung der Rolle, die der Anschluss laut boardconfig hat.
    /// Nach normalem Betrieb stehen die Servos dort, beim Einschalten springen sie also nicht.
    int startAngleFor(int i) const {
        if (_started[i]) return _cmd[i];
        if (_arm >= 0) return i == _arm ? _spender_idle : _lifter_rest;
        return i == 0 ? board::SERVO_LIFTER_ANGLE_REST : board::SERVO_SPENDER_ANGLE_IDLE;
    }

protected: // Eingabe

    /// Feinpositionierung: liefert true bei "ok", sonst wird der Servo bewegt.
    bool jog(int i, const char* line) {
        if (strcmp(line, "ok") == 0) return true;

        int target = _cmd[i];
        int value = 0;
        if (strcmp(line, "+") == 0) target += 1;
        else if (strcmp(line, "-") == 0) target -= 1;
        else if ((line[0] == '+' || line[0] == '-') && SerialLineReader::parseInt(line, value)) target += value;
        else if (SerialLineReader::parseInt(line, value)) target = value;
        else {
            printJogHelp();
            Serial.print("> ");
            return false;
        }

        servoMove(i, target, JOG_MS_PER_DEGREE);
        Serial.printf("GP%d -> %d Grad\n> ", pinOf(i), _cmd[i]);
        return false;
    }

    void handleLine(const char* line) {
        if (strcmp(line, "x") == 0) {
            Serial.println("Beide Servos losgelassen, Neustart.");
            restart();
            return;
        }
        if (strcmp(line, "?") == 0) {
            printIntro();
            prompt();
            return;
        }

        switch (_step) {
        case Step::IDENT_START: {
            int angle = startAngleFor(_candidate);
            if (line[0] != '\0' && !SerialLineReader::parseInt(line, angle)) { prompt(); return; }
            if (!servoStart(_candidate, angle)) { prompt(); return; }
            startWiggle(_candidate);
            break;
        }
        case Step::IDENT_WIGGLE:
            Serial.print("bitte warten ...\n> ");
            break;
        case Step::IDENT_ASK:
            handleIdentAnswer(line);
            break;
        case Step::ARM_EJECT:
            if (jog(_arm, line)) {
                _spender_eject = _cmd[_arm];
                setStep(Step::ARM_IDLE);
            }
            break;
        case Step::ARM_IDLE:
            if (jog(_arm, line)) {
                _spender_idle = _cmd[_arm]; // Arm bleibt hier und haelt
                setStep(Step::LIFTER_START);
            }
            break;
        case Step::LIFTER_START: {
            int angle = startAngleFor(lifter());
            if (line[0] != '\0' && !SerialLineReader::parseInt(line, angle)) { prompt(); return; }
            if (!servoStart(lifter(), angle)) { prompt(); return; }
            setStep(Step::LIFTER_REST);
            break;
        }
        case Step::LIFTER_REST:
            if (jog(lifter(), line)) {
                _lifter_rest = _cmd[lifter()];
                setStep(Step::LIFTER_MAX);
            }
            break;
        case Step::LIFTER_MAX:
            if (jog(lifter(), line)) {
                _lifter_max = _cmd[lifter()];
                _stack.lifterRest = _lifter_rest;
                _stack.lifterMax = _lifter_max;
                setStep(Step::STACK_PLACE); // Heber bleibt ganz ausgefahren zum Einlegen
            }
            break;
        case Step::STACK_PLACE:
            handleStackPlace(line);
            break;
        case Step::STACK_ADJUST:
            if (line[0] == 'a') {
                if (!startEjectTest(line)) prompt();
            } else if (jog(lifter(), line)) {
                _stack.lifterStack[_stack_n - 1] = _cmd[lifter()];
                _stack.lifterStackLevels = _stack_n;
                Serial.printf("Stapel mit %d Coaster%s: %d Grad\n", _stack_n, _stack_n == 1 ? "" : "n", _cmd[lifter()]);
                if (++_stack_n > CoasterServoAngles::MAX_STACK_LEVELS) {
                    finishStack();
                    return;
                }
                setStep(Step::STACK_PLACE);
            }
            break;
        case Step::STACK_TEST:
        case Step::LIFTER_PARK:
            Serial.print("bitte warten ...\n> ");
            break;
        case Step::DONE:
            restart();
            break;
        default:
            break;
        }
    }

    void handleStackPlace(const char* line) {
        if (_stack_n == 1) { // Heber steht schon ganz ausgefahren
            setStep(Step::STACK_ADJUST);
            return;
        }
        if (strcmp(line, "f") == 0) {
            finishStack();
            return;
        }
        if (_place_waiting) {
            if (line[0] != '\0') { Serial.print("Coaster einlegen, dann Enter.\n> "); return; }
            // mit einem Coaster mehr steht der unterste tiefer: von der vorigen Stufe aus annaehern
            _place_waiting = false;
            servoMove(lifter(), _stack.lifterStack[_stack_n - 2], JOG_MS_PER_DEGREE);
            setStep(Step::STACK_ADJUST);
            return;
        }
        if (strcmp(line, "w") == 0) {
            setStep(Step::STACK_ADJUST);
        } else if (line[0] == '\0') {
            _place_waiting = true;
            servoMove(lifter(), _lifter_max, JOG_MS_PER_DEGREE);
            Serial.print("Heber faehrt ganz aus. Coaster einlegen, dann Enter.\n> ");
        } else {
            prompt();
        }
    }

    /// Testauswurf aus "a", "as" oder "ah [N] [W]" zusammenstellen und starten. false bei unbekanntem Befehl.
    bool startEjectTest(const char* line) {
        _seq_len = 0;
        if (strcmp(line, "a") == 0) {
            _seq[_seq_len++] = { _spender_eject, JOG_MS_PER_DEGREE, 0 };
            Serial.println("Testauswurf mit Rampe ...");
        } else if (strcmp(line, "as") == 0) {
            _seq[_seq_len++] = { _spender_eject, 0, STRIKE_HOLD_MS };
            Serial.println("Schlag ...");
        } else if (strncmp(line, "ah", 2) == 0) {
            int strikes = HAMMER_DEFAULT_STRIKES;
            int backoff = HAMMER_DEFAULT_BACKOFF;
            const int parsed = sscanf(line + 2, "%d %d", &strikes, &backoff);
            if (parsed < 0) { strikes = HAMMER_DEFAULT_STRIKES; backoff = HAMMER_DEFAULT_BACKOFF; } // nur "ah"
            if (strikes < 1 || strikes > HAMMER_MAX_STRIKES || backoff < 1 || backoff > 180) {
                Serial.printf("Haemmern: N = 1..%d Schlaege, W = 1..180 Grad\n", HAMMER_MAX_STRIKES);
                return false;
            }
            // zurueck heisst Richtung Ruhestellung, aber nicht ueber sie hinaus
            const int dir = (_spender_idle > _spender_eject) ? 1 : -1;
            const int travel = abs(_spender_idle - _spender_eject);
            const int back = _spender_eject + dir * (backoff < travel ? backoff : travel);
            for (int i = 0; i < strikes; ++i) {
                if (i > 0) _seq[_seq_len++] = { back, 0, BACKOFF_HOLD_MS };
                _seq[_seq_len++] = { _spender_eject, 0, STRIKE_HOLD_MS };
            }
            Serial.printf("Haemmern: %d Schlaege, zwischendurch zurueck auf %d Grad ...\n", strikes, back);
        } else {
            return false;
        }
        _seq[_seq_len++] = { _spender_idle, JOG_MS_PER_DEGREE, 0 }; // ruhig zurueck in die Ruhestellung
        _seq_index = 0;
        _step = Step::STACK_TEST;
        startArmMove();
        return true;
    }

    void startArmMove() {
        const ArmMove& m = _seq[_seq_index];
        _seq_holding = false;
        servoMove(_arm, m.angle, m.ms_per_degree);
    }

    void finishStack() {
        servoMove(lifter(), _lifter_rest, JOG_MS_PER_DEGREE);
        setStep(Step::LIFTER_PARK);
    }

    void handleIdentAnswer(const char* line) {
        const int other = 1 - _candidate;

        if (strcmp(line, "w") == 0) {
            startWiggle(_candidate);
            return;
        }
        if (strcmp(line, "a") == 0) {
            _role[_candidate] = Role::SPENDER;
            _arm = _candidate;
            _led.setModeToBlinkCyan();
            Serial.printf("Auswurfarm = GP%d, Heber = GP%d\n", pinOf(_arm), pinOf(other));
            setStep(Step::ARM_EJECT); // Arm bleibt eingeschaltet und haelt seine Position
            return;
        }
        if (strcmp(line, "h") == 0) {
            servoRelease(_candidate); // Heber sofort loslassen, er darf den Arm nicht ausdruecken
            _role[_candidate] = Role::LIFTER;
            Serial.printf("GP%d ist der Heber und wurde losgelassen.\n", pinOf(_candidate));
        } else if (strcmp(line, "n") == 0) {
            servoRelease(_candidate);
            _role[_candidate] = Role::NONE;
            Serial.printf("GP%d: keine Bewegung, losgelassen.\n", pinOf(_candidate));
        } else {
            prompt();
            return;
        }

        if (_role[other] == Role::UNKNOWN) {
            _candidate = other;
            setStep(Step::IDENT_START);
            return;
        }

        // beide geprueft, kein Auswurfarm gemeldet
        Serial.println("Kein Auswurfarm erkannt. Verkabelung pruefen oder einen anderen Startwinkel waehlen.");
        // beim naechsten Versuch nicht mit dem Heber beginnen
        _candidate = (_role[other] == Role::LIFTER) ? _candidate : other;
        _role[0] = _role[1] = Role::UNKNOWN;
        setStep(Step::IDENT_START);
    }
};

#endif // SRC_SERVO_CALIBRATION_H
