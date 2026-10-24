#ifndef SRC_PERSISTENT_SETTINGS_H
#define SRC_PERSISTENT_SETTINGS_H

#include <Arduino.h>
#include <EEPROM.h>

/** Einstellungen, die einen Neustart ueberleben, im Flash des Pico (EEPROM-Emulation von arduino-pico).
 *
 * Ein Kennbyte markiert gueltige Daten: ein frisch geflashtes Board liefert die Defaults. Jedes save()
 * schreibt einen Flash-Sektor, also nur bei echten Aenderungen aufrufen, nicht zyklisch.
 */
class PersistentSettings {
    static constexpr int SIZE = 256;
    static constexpr uint8_t MAGIC = 0xC5;
    static constexpr int ADDR_MAGIC = 0;
    static constexpr int ADDR_COASTER_COUNT = 1;

    bool _begun {false};

    void begin() {
        if (_begun) return;
        EEPROM.begin(SIZE);
        _begun = true;
    }

public:
    /// Gespeicherte Anzahl Coaster, fallback wenn nichts oder Unsinn gespeichert ist.
    int coasterCount(int fallback, int max_count) {
        begin();
        if (EEPROM.read(ADDR_MAGIC) != MAGIC) return fallback;
        const int count = EEPROM.read(ADDR_COASTER_COUNT);
        return (count >= 1 && count <= max_count) ? count : fallback;
    }

    void saveCoasterCount(int count) {
        begin();
        if (EEPROM.read(ADDR_MAGIC) == MAGIC && EEPROM.read(ADDR_COASTER_COUNT) == count) return; // unveraendert
        EEPROM.write(ADDR_MAGIC, MAGIC);
        EEPROM.write(ADDR_COASTER_COUNT, static_cast<uint8_t>(count));
        if (!EEPROM.commit()) Serial.println("[SETTINGS] saving to flash failed");
    }
};

#endif // SRC_PERSISTENT_SETTINGS_H
