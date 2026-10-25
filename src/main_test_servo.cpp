#include <Arduino.h>

#include <iostream>
#include <streambuf>

#include "hw_impl_hal.h"
#include "webots/coasterbot_functions.h"

namespace {

class SerialStreamBuf : public std::streambuf {
protected:
    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof())) {
            return traits_type::not_eof(ch);
        }

        Serial.write(static_cast<uint8_t>(traits_type::to_char_type(ch)));
        return ch;
    }

    std::streamsize xsputn(const char* data, std::streamsize size) override {
        return static_cast<std::streamsize>(
            Serial.write(reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(size)));
    }
};

HardwareImplementationHAL bot;
PoseEstimator pose(bot);
SafetyMonitor safety(bot);
CoasterbotFunctions coasterbot(bot, pose, safety);
SerialStreamBuf serialStreamBuf;

bool hardwareReady = false;
int coasterTestIndex = 0;
bool buttonWasPressed = false;

enum class HardwareTestPhase {
    WaitToSpend,
    Spending,
    WaitToCollect,
    Collecting,
    Complete
};

HardwareTestPhase hardwareTestPhase = HardwareTestPhase::WaitToSpend;

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(5000);
    std::cout.rdbuf(&serialStreamBuf);

    bot.initializeHardware();
    bot.setUserLedIndicatorToAutomatic(false);

    hardwareReady = bot.inertialSensor().ok();
    if (!hardwareReady) {
        bot.setLeftSpeed(0.0f);
        bot.setRightSpeed(0.0f);
        Serial.println("[ERROR] MPU-6050 initialization failed; robot remains stopped.");
        bot.led(RobotHAL::LED_COLORS::RED);
        return;
    }

    bot.led(RobotHAL::LED_COLORS::ORANGE);
    coasterbot.resetSpendCoasterSequence();
    Serial.println("[HW TEST] Ready. Press the button to spend coaster 0.");
}

void loop() {
    bot.step();
    coasterbot.update();

    const bool pressed = coasterbot.buttonPressed();
    const bool buttonEvent = pressed && !buttonWasPressed;
    buttonWasPressed = pressed;

    if (hardwareReady && buttonEvent) {
        if (hardwareTestPhase == HardwareTestPhase::WaitToSpend) {
            hardwareTestPhase = HardwareTestPhase::Spending;
            Serial.printf("[HW TEST] Starting dispense of coaster %d.\n", coasterTestIndex);
        } else if (hardwareTestPhase == HardwareTestPhase::WaitToCollect) {
            hardwareTestPhase = HardwareTestPhase::Collecting;
            Serial.printf("[HW TEST] Starting collection of coaster %d.\n", coasterTestIndex);
        } else if (hardwareTestPhase == HardwareTestPhase::Complete) {
            coasterbot.resetSpendCoasterSequence();
            coasterTestIndex = 0;
            hardwareTestPhase = HardwareTestPhase::WaitToSpend;
            Serial.println("[HW TEST] Test reset. Press the button to spend coaster 0.");
        }
    }

    if (hardwareTestPhase == HardwareTestPhase::Spending) {
        if (coasterbot.spendCoaster(coasterTestIndex)) {
            Serial.printf("[HW TEST] Spent coaster %d.\n", coasterTestIndex);
            ++coasterTestIndex;

            if (coasterTestIndex >= CoasterbotFunctions::maxCoasters()) {
                coasterTestIndex = CoasterbotFunctions::maxCoasters() - 1;
                hardwareTestPhase = HardwareTestPhase::WaitToCollect;
                Serial.println("[HW TEST] All coasters spent. Press the button to collect the last coaster.");
            } else {
                hardwareTestPhase = HardwareTestPhase::WaitToSpend;
                Serial.printf("[HW TEST] Press the button to spend coaster %d.\n", coasterTestIndex);
            }
        }
    } else if (hardwareTestPhase == HardwareTestPhase::Collecting) {
        if (coasterbot.getCoaster(coasterTestIndex)) {
            Serial.printf("[HW TEST] Collected coaster %d.\n", coasterTestIndex);
            --coasterTestIndex;

            if (coasterTestIndex < 0) {
                hardwareTestPhase = HardwareTestPhase::Complete;
                Serial.println("[HW TEST] All coasters collected. Press the button to reset the test.");
            } else {
                hardwareTestPhase = HardwareTestPhase::WaitToCollect;
                Serial.printf("[HW TEST] Press the button to collect coaster %d.\n", coasterTestIndex);
            }
        }
    }

    delay(20);
}
