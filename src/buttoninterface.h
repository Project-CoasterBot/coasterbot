#ifndef SRC_BUTTONINTERFACE_H
#define SRC_BUTTONINTERFACE_H

#include <Arduino.h>
#include <optional>

template<int pin, bool active_low = true>
class ButtonController {
    std::optional<unsigned long> _press_event;

public:

    ButtonController() { static_assert(pin >= 0); }

    // call in setup method.
    void begin() { pinMode(pin, active_low ? INPUT_PULLUP : INPUT_PULLDOWN); }

    // returns true when the button was just released, press_time is set to the duration the button was pressed.
    bool update(unsigned long curtime, unsigned long& press_time) {
        const bool was_pressed = _press_event.has_value();
        const bool currentlypressed = (digitalRead(pin) == active_low ? LOW : HIGH); // active low

        if (was_pressed == currentlypressed) return false;

        if (! was_pressed) {
            _press_event = curtime;
            return false;
        }

        press_time = curtime - _press_event.value();
        _press_event.reset();

        return true;
    }

    /// Returns true while the button is pressed.
    bool isPressed() const { return _press_event.has_value(); }
};

#endif //SRC_BUTTONINTERFACE_H
