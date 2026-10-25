#ifndef SRC_CONSOLE_RECOVERY_H
#define SRC_CONSOLE_RECOVERY_H

#include <Arduino.h>
#include <cstdio>
#include <iostream>

/// std::cout/printf laufen ueber Serial (DEBUG_RP2040_PORT). Solange keine Konsole verbunden ist,
/// schreibt Serial nichts, stdout und std::cout merken sich das als Fehler und verwerfen danach jede
/// weitere Ausgabe ([NAV], [SAFETY], ...). In jedem Loop aufrufen: setzt den Fehler zurueck, sobald
/// die Konsole verbunden ist.
inline void recoverConsoleOutput() {
    if (!Serial) return;
    if (std::cout.fail() || ferror(stdout)) {
        clearerr(stdout);
        std::cout.clear();
    }
}

#endif // SRC_CONSOLE_RECOVERY_H
