#ifndef COASTER_SERVO_ANGLES_H
#define COASTER_SERVO_ANGLES_H

#include <algorithm>
#include <array>
#include <cstdlib>

// Servowinkel [deg] des Coaster-Mechanismus. Haengen vom Einbau der Servos ab, die Firmware setzt sie
// aus boardconfig(_local).h (siehe coaster_servo_config.h), die Defaults entsprechen Dennis Aufbau.
struct CoasterServoAngles {
    static constexpr int MAX_STACK_LEVELS = 12;

    int lifterRest = 180;   // Heber ohne Hub (ganz eingefahren)
    int lifterMax = 0;      // Heber mit maximalem Hub (ganz ausgefahren)
    int spenderIdle = 90;   // Auswurfarm in Ruhestellung, Stapel frei
    int spenderEject = 0;   // Auswurfarm schiebt den untersten Coaster aus
    int spenderAside = 180;   // Auswurfarm in Ruhestellung, Stapel frei
    bool spenderEjectStrike = false; // true: Auswurf ohne Rampe (Schlag), false: gerampt

    // Heberwinkel, bei dem der unterste Coaster auf Ausschubhoehe steht, Index = Coaster im Stapel - 1.
    // Mit der Servo-Normierung vermessen, ueber lifterStackLevels hinaus extrapoliert (stackAngle).
    // Original Stackwerte: {0, 45, 90, 135}
    // Stackwerte von Dennis: {0, 40, 60, 90}
    std::array<int, MAX_STACK_LEVELS> lifterStack {0, 40, 60, 90};
    int lifterStackLevels = 4;

    int levels() const { return std::clamp(lifterStackLevels, 1, MAX_STACK_LEVELS); }

    /// Abstand je Coaster hinter der letzten vermessenen Stufe. 0, wenn er nicht Richtung Ruhestellung
    /// zeigt (nur eine Stufe vermessen oder Messung unplausibel), dann wird nicht extrapoliert.
    int extrapolationStep() const {
        const int n = levels();
        if (n < 2) return 0;
        const int last = lifterStack[n - 1];
        const int step = last - lifterStack[n - 2];
        const bool towards_rest = (step > 0) == (lifterRest > last);
        return (step != 0 && towards_rest && last != lifterRest) ? step : 0;
    }

    /// Heberwinkel fuer "coasters" Coaster im Stapel: vermessen oder mit dem letzten Abstand weiter
    /// gerechnet, hoechstens bis zur Ruhestellung.
    int stackAngle(int coasters) const {
        const int n = levels();
        if (coasters < 1) coasters = 1;
        if (coasters <= n) return lifterStack[coasters - 1];

        const int angle = lifterStack[n - 1] + extrapolationStep() * (coasters - n);
        return std::clamp(angle, std::min(lifterRest, lifterMax), std::max(lifterRest, lifterMax));
    }

    /// Wie viele Coaster der Stapel fasst: vermessene Stufen plus extrapolierte bis zur Ruhestellung.
    int stackCapacity() const {
        const int n = levels();
        const int step = extrapolationStep();
        if (step == 0) return n;
        return n + std::abs(lifterRest - lifterStack[n - 1]) / std::abs(step);
    }
};

#endif // COASTER_SERVO_ANGLES_H
