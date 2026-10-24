#ifndef SRC_SERIAL_LINE_READER_H
#define SRC_SERIAL_LINE_READER_H

#include <Arduino.h>
#include <cstdlib>

/** Liest Zeilen von der seriellen Konsole, ohne zu blockieren.
 *
 * Die Eingabe wird zurueckgeschrieben (pio device monitor zeigt sie sonst nicht an), Backspace
 * loescht das letzte Zeichen. CR, LF und CRLF beenden eine Zeile, CRLF zaehlt dabei nur einmal.
 */
class SerialLineReader {
    char _line[32] {};
    size_t _len {0};
    bool _last_was_cr {false};

public:
    /// In jedem Loop aufrufen. Liefert eine fertige Zeile ohne Zeilenende, sonst nullptr.
    /// Die Zeile ist bis zum naechsten Aufruf gueltig.
    const char* poll() {
        while (Serial.available() > 0) {
            const int c = Serial.read();
            if (c == '\r' || c == '\n') {
                const bool skip = (c == '\n' && _last_was_cr);
                _last_was_cr = (c == '\r');
                if (skip) continue;
                Serial.println();
                _line[_len] = '\0';
                _len = 0;
                return _line;
            }
            _last_was_cr = false;
            if (c == 0x08 || c == 0x7f) { // Backspace
                if (_len > 0) {
                    --_len;
                    Serial.print("\b \b");
                }
                continue;
            }
            if (_len + 1 < sizeof(_line) && c >= 0x20 && c < 0x7f) {
                _line[_len++] = static_cast<char>(c);
                Serial.write(static_cast<char>(c));
            }
        }
        return nullptr;
    }

    /// Ganze Zahl ohne weitere Zeichen, z.B. "12" oder "-5".
    static bool parseInt(const char* text, int& value) {
        char* end = nullptr;
        const long v = strtol(text, &end, 10);
        if (end == text || *end != '\0') return false;
        value = static_cast<int>(v);
        return true;
    }
};

#endif // SRC_SERIAL_LINE_READER_H
