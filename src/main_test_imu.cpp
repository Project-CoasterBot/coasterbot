#include <Arduino.h>

#include "boardconfig.h"
#include "gyrosensor.h"

namespace {

GyroSensor<board::PIN_INERTIAL_SCL, board::PIN_INERTIAL_SDA> imu;
unsigned long lastPrintMs = 0;
bool imuReady = false;
bool deviceAt68 = false;
bool deviceAt69 = false;

TwoWire& imuI2cBus() {
    return board::PIN_INERTIAL_SDA % 4 == 0 ? Wire : Wire1;
}

bool probeI2cAddress(uint8_t address) {
    imuI2cBus().beginTransmission(address);
    return imuI2cBus().endTransmission() == 0;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(5000);

    Serial.println();
    Serial.println("[IMU TEST] Starting MPU-6050 test");
    Serial.print("[IMU TEST] I2C SDA=GP");
    Serial.print(board::PIN_INERTIAL_SDA);
    Serial.print(" SCL=GP");
    Serial.println(board::PIN_INERTIAL_SCL);
    Serial.println("[IMU TEST] Keep the robot still during startup calibration.");

    imu.begin();
    imuReady = imu.ok();
    deviceAt68 = probeI2cAddress(0x68);
    deviceAt69 = probeI2cAddress(0x69);

    if (imuReady) {
        // In test mode, integrate heading while the sensor is rotated by hand.
        imu.setMotion(Motion::Turning);
        Serial.println("[IMU TEST] Ready. Rotate the sensor by hand and watch heading.");
    } else {
        Serial.println("[IMU TEST] Initialization failed; check the [GYRO] messages above.");
    }
}

void loop() {
    if (imuReady) {
        imu.loop();
    }

    const unsigned long now = millis();
    if (now - lastPrintMs < 250UL) {
        return;
    }
    lastPrintMs = now;

    if (!imuReady) {
        Serial.print("[IMU TEST] unavailable; I2C 0x68=");
        Serial.print(deviceAt68 ? "found" : "not found");
        Serial.print(", 0x69=");
        Serial.println(deviceAt69 ? "found" : "not found");
        return;
    }

    Serial.print("[IMU TEST] heading=");
    Serial.print(imu.heading, 1);
    Serial.print(" deg rateZ=");
    Serial.print(imu.rate_z, 1);
    Serial.print(" deg/s | gyro xyz=");
    Serial.print(imu.gyro_x, 1);
    Serial.print(',');
    Serial.print(imu.gyro_y, 1);
    Serial.print(',');
    Serial.print(imu.gyro_z, 1);
    Serial.print(" deg/s | accel xyz=");
    Serial.print(imu.accel_x, 2);
    Serial.print(',');
    Serial.print(imu.accel_y, 2);
    Serial.print(',');
    Serial.print(imu.accel_z, 2);
    Serial.println(" g");
}
