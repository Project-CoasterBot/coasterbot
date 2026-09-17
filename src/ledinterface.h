#ifndef SRC_LEDINTERFACE_H
#define SRC_LEDINTERFACE_H

#include <array>
#include <Arduino.h>

/** This class is used in the coasterbot project to control the RGB LED with user mode indicators.
 *
 * Instantiate with the according GPIO numbers as template arguments.
 *
 * Call begin() once from setup(). The constructor deliberately keeps its hands
 * off the hardware: global instances are constructed before the Arduino core
 * has set up system clock, PWM and millis().
 *
 * In every main loop iteration, call the update() method.
 */
template<int r_pin, int g_pin, int b_pin>
class LEDController {
protected:

    enum Mode {
        Off,
        Constant,
        Blinking,
        Rainbow
    } _mode {Off};

    int _r{0}, _g{0}, _b{0}; // currently set values. default = off
    unsigned long _last_time {0};

    unsigned long _next_action {0}; // timepoint when the next event needs to happen, used in non constant modes

    unsigned int _blink_iter {0};
    unsigned long _blink_iter_duration {500}; // milliseconds

    unsigned int _rainbow_duration {3000};
    unsigned long _rainbow_period_start {0};

public:

    LEDController() {
        static_assert(r_pin >= 0 && g_pin >= 0 && b_pin >= 0);
        static_assert(r_pin != g_pin && r_pin != b_pin && g_pin != b_pin);
    }

    /// Configure the output pins and switch the LED off. Call once from setup().
    void begin() {
        for (const int pin: std::array<int,3>{r_pin, g_pin, b_pin}) {
            pinMode(pin, OUTPUT);
        }

        setModeToOff();
        setOutputValues();
    }

public: // mode setters

    void setModeToOff() {
        _mode = Off;
        _r = 0;
        _g = 0;
        _b = 0;
        setOutputValues();
    }

    void setModeToConstant(int r, int g, int b) {
        if (_mode == Constant && colorsIdentical(r,g,b)) return; // nothing to do
        _mode = Constant;
        updateColors(r,g,b);
        setOutputValues();
    }

    void setModeToConstantRed(int v = 255) { setModeToConstant(v, 0,0); }
    void setModeToConstantGreen(int v = 255) { setModeToConstant(0, v,0); }
    void setModeToConstantBlue(int v = 255) { setModeToConstant(0,0,v); }
    void setModeToConstantOrange(int v = 255) { setModeToConstant((v*2)/3,v/3,0); }
    void setModeToConstantYellow(int v = 255) { setModeToConstant(v/2,v/2,0); }
    void setModeToConstantLime(int v = 255) { setModeToConstant(0, (v*2)/3,v/3); }
    void setModeToConstantCyan(int v = 255) { setModeToConstant(0, v/2,v/2); }
    void setModeToConstantLightblue(int v = 255) { setModeToConstant(0, v/3,(v*2)/3); }
    void setModeToConstantPurple(int v = 255) { setModeToConstant(v/3, 0, (v*2)/3); }
    void setModeToConstantPink(int v = 255) { setModeToConstant((v*2)/3, 0, v/3); }

    void setModeToBlink(int r, int g, int b, unsigned long duration = 500) {
        if (_mode == Blinking && colorsIdentical(r,g,b)) return; // nothing to do
        _mode = Blinking;
        updateColors(r,g,b);
        setOutputValues();
        _blink_iter_duration = duration;
        _next_action = now() + _blink_iter_duration;
    }

    void setModeToBlinkRed(int v = 255, unsigned long duration = 500) { setModeToBlink(v, 0,0, duration); }
    void setModeToBlinkGreen(int v = 255, unsigned long duration = 500) { setModeToBlink(0, v,0, duration); }
    void setModeToBlinkBlue(int v = 255, unsigned long duration = 500) { setModeToBlink(0,0,v, duration); }
    void setModeToBlinkOrange(int v = 255, unsigned long duration = 500) { setModeToBlink((v*2)/3,v/3,0, duration); }
    void setModeToBlinkYellow(int v = 255, unsigned long duration = 500) { setModeToBlink(v/2,v/2,0, duration); }
    void setModeToBlinkLime(int v = 255, unsigned long duration = 500) { setModeToBlink(0, (v*2)/3,v/3, duration); }
    void setModeToBlinkCyan(int v = 255, unsigned long duration = 500) { setModeToBlink(0, v/2,v/2, duration); }
    void setModeToBlinkLightblue(int v = 255, unsigned long duration = 500) { setModeToBlink(0, v/3,(v*2)/3, duration); }
    void setModeToBlinkPurple(int v = 255, unsigned long duration = 500) { setModeToBlink(v/3, 0, (v*2)/3, duration); }
    void setModeToBlinkPink(int v = 255, unsigned long duration = 500) { setModeToBlink((v*2)/3, 0, v/3, duration); }

    void setModeToRainbow(unsigned long full_period_milliseconds = 3000) {
        if (_mode == Rainbow && _rainbow_duration == full_period_milliseconds) return;
        _mode = Rainbow;
        setOutputValues(255,0,0); // start color
        _rainbow_period_start = now();
        _last_time = _rainbow_period_start;
        if (full_period_milliseconds < 50) // min 50 ms
            _rainbow_duration = 50;
        else if (full_period_milliseconds > 60*60*1000) // max 1 h
            _rainbow_duration = 60*60*1000;
        else
            _rainbow_duration = full_period_milliseconds;
    }


public: // main loop repeated call

    void update() { update(now()); }

    void update(unsigned long curtime) {
        switch (_mode) {
        default:
        case Constant:
        case Off:
            // In these states, there is nothing to do in the update method, since the values are already set up previously
            break;

        case Blinking:
            if (static_cast<long>(curtime - _next_action) >= 0) {
                if (_blink_iter % 2ull == 0) // was on, switch off
                    setOutputValues(0,0,0);
                else
                    setOutputValues();
                _blink_iter++;
                _next_action = curtime + _blink_iter_duration;
            } // else nothing to do, wait for next iteration
            break;

        case Rainbow: {
            unsigned long delta = curtime - _last_time;
            if (delta > 10) { // max update every n milliseconds
                _last_time = curtime;

                unsigned long progress = curtime - _rainbow_period_start;
                unsigned long nth_period = progress / _rainbow_duration;
                unsigned long dt = progress % _rainbow_duration;
                if (nth_period > 0)
                    _rainbow_period_start = curtime - dt;

                const unsigned int v = static_cast<unsigned int>( (static_cast<unsigned long long>(dt) * 3 * 256) / _rainbow_duration);

                const unsigned int col = v / 256; // which color transition, 0 is red to green, 1 is green to blue, ...
                const unsigned int p = v % 256; // progress inside the current color transition

                switch (col) {
                case 0:
                    setOutputValues(255-p,p,0);
                    break;
                case 1:
                    setOutputValues(0, 255-p,p);
                    break;
                default:
                case 2:
                    setOutputValues(p,0, 255-p);
                    break;
                }
            }
        }
            break;
        }
    }


protected:
    /// Milliseconds since program start. Wraps around, so only compare differences.
    static unsigned long now() {
        return millis();
    }

    bool colorsIdentical(int r, int g, int b) const { return (r == _r) && (g == _g) && (b == _b); }

    /// Check the internal color state and adapts the values if there is any change.
    bool updateColors(int r, int g, int b) {
        if (colorsIdentical(r,g,b)) return false; // nothing changed
        _r = r;
        _g = g;
        _b = b;
        return true;
    }

    /// Write the internal state values to the output pins without checks for changes.
    void setOutputValues() { setOutputValues(_r, _g, _b); }

    /// Write the passed values to the output independant of the currently buffered rgb values.
    void setOutputValues(int r, int g, int b) {
        setOutputValue<r_pin>(r);
        setOutputValue<g_pin>(g);
        setOutputValue<b_pin>(b);
    }

    /// Write the valuee to the output pin without check for changes.
    template<int pin>
    void setOutputValue(int color) {
        color = color < 0 ? 0 : (color > 255 ? 255 : color);
        analogWrite(pin, color);
    }

};

#endif //SRC_LEDINTERFACE_H
