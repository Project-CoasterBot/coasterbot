#ifndef SRC_GYROSENSOR_H
#define SRC_GYROSENSOR_H


#include <Arduino.h>
#include <Wire.h> // i2c lib

enum class Motion { Idle, Driving, Turning }; // hint from the main loop what the motors do

/** abstraction for the MPU-6050 (accelerometer + gyroscope) via I2C, direct register access.
 *
 * On top of the raw readings this class does a simple 2D dead reckoning on the table plane:
 * - heading: integrated gyro z rate
 * - position: accel x/y rotated into the table frame by the heading, integrated twice
 *
 * Mounting: the sensor may be mounted in any orientation. calibrate() measures the gravity vector at rest
 * and derives a rotation that levels the sensor frame (z up, x/y parallel to the table). All accel and gyro
 * values used for the dead reckoning are rotated by it first. The rotation around the vertical axis is not
 * observable from gravity, so the table frame x axis is the leveled sensor x axis at startup - not
 * necessarily the driving direction. Heading is ccw positive.
 *
 * Double integration of a MEMS accelerometer drifts quadratically - a residual bias of 0.01 g is already
 * ~0.5 m after 3 s. To keep that bounded:
 * - begin() calibrates the biases, the robot must stand still for ~1 s after power up
 * - zero velocity update: as soon as the sensor reads still for a while, velocity is clamped to zero and
 *   the biases are tracked slowly. Drift only accumulates while moving, also when pushed by hand.
 *   setMotion(Motion::Driving) from the main loop suppresses this - at constant speed the robot
 *   reads exactly like standing still.
 * - setMotion(Motion::Turning) while turning on the spot: only the heading is integrated, the position is
 *   held. The sensor is not in the center of rotation, so it sees a centripetal acceleration that would
 *   otherwise run off as soon as the heading is slightly wrong.
 */
template<int scl_pin, int sda_pin, uint8_t i2c_address = 0x68>
class GyroSensor {

    // RP2350: I2C0 uses SDA on GPIO 4n / SCL on GPIO 4n+1, I2C1 uses SDA on GPIO 4n+2 / SCL on GPIO 4n+3
    static constexpr bool _on_i2c0 = (sda_pin % 4 == 0);
    static_assert(sda_pin >= 0 && scl_pin >= 0 && sda_pin != scl_pin);
    static_assert(sda_pin % 2 == 0 && scl_pin % 4 == sda_pin % 4 + 1, "sda/scl pins must belong to the same I2C peripheral");

    unsigned long _last_sample_us {0};
    unsigned long _sample_every_us {5000}; // 200 Hz, integration needs a steady and fast sample rate
    unsigned long _last_print {0};
    unsigned long _print_every_ms {1500};
    bool _ok {false};

private:
    Motion _motion {Motion::Idle};
    bool _stationary {true};   // result of the still detection
    unsigned _still_samples {0};

    float _bias_ax {0}, _bias_ay {0}, _bias_gz {0}; // bias in the leveled frame, g resp. deg/s
    float _level[3][3] {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; // compensation for mounting direction

    /// Rotate a sensor frame vector into the leveled frame.
    void toLevel(float x, float y, float z, float &lx, float &ly, float &lz) const {
        lx = _level[0][0] * x + _level[0][1] * y + _level[0][2] * z;
        ly = _level[1][0] * x + _level[1][1] * y + _level[1][2] * z;
        lz = _level[2][0] * x + _level[2][1] * y + _level[2][2] * z;
    }

    /// Rotation that turns the measured gravity direction (gx, gy, gz) onto +z (Rodrigues formula).
    void computeLevel(float gx, float gy, float gz) {
        const float norm = sqrtf(gx * gx + gy * gy + gz * gz);
        const float ax = gx / norm, ay = gy / norm, az = gz / norm;

        if (az < -0.9999f) { // upside down, the formula degenerates: rotate 180 deg around x
            const float flip[3][3] {{1, 0, 0}, {0, -1, 0}, {0, 0, -1}};
            memcpy(_level, flip, sizeof(_level));
            return;
        }


        const float vx = ay, vy = -ax;
        const float k = 1.f / (1.f + az);
        const float r[3][3] { // matrix transposition for z down
            {1.f - vy * vy * k, vx * vy * k, vy},
            {vx * vy * k, 1.f - vx * vx * k, -vx},
            {-vy, vx, 1.f - (vx * vx + vy * vy) * k}};
        memcpy(_level, r, sizeof(_level));
    }

    static TwoWire &bus() { return _on_i2c0 ? Wire : Wire1; }

    bool writeRegister(uint8_t reg, uint8_t value) {
        bus().beginTransmission(i2c_address);
        bus().write(reg);
        bus().write(value);
        return bus().endTransmission() == 0;
    }

    bool readRegisters(uint8_t reg, uint8_t *buf, size_t len) {
        bus().beginTransmission(i2c_address);
        bus().write(reg);
        if (bus().endTransmission(false) != 0) return false;
        if (bus().requestFrom(i2c_address, len) != len) return false;
        for (size_t i = 0; i < len; ++i)
            buf[i] = bus().read();
        return true;
    }

public:
    float accel_x {0}, accel_y {0}, accel_z {0};
    float gyro_x {0}, gyro_y {0}, gyro_z {0};
    float temperature {0};

    float heading {0};// in degree, from -180 to 180
    float vel_x {0}, vel_y {0}; // in m/s
    float pos_x {0}, pos_y {0}; // in m

    void begin() {
        bus().setSDA(sda_pin);
        bus().setSCL(scl_pin);
        bus().setClock(400000);
        bus().begin();

        uint8_t who_am_i = 0;
        if (! readRegisters(0x75, &who_am_i, 1)) { // who am i register
            Serial.printf("[GYRO] no MPU-6050 found at 0x%02X (sda=%d scl=%d)\n", i2c_address, sda_pin, scl_pin);
            _ok = false;
            return;
        }
        if (who_am_i != 0x68) Serial.printf("[GYRO] unexpected WHO_AM_I 0x%02X (clone?)\n", who_am_i);

        // setup modes
        _ok = writeRegister(0x6B, 0x01)
              && writeRegister(0x1A, 0x03)
              && writeRegister(0x1B, 0x10)  // +-1000 deg/s, turning on the spot exceeds +-250
              && writeRegister(0x1C, 0x08); // +-4 g, skid steering jolts clip at +-2 g

        if (_ok) {
            delay(100); // let the sensor settle after wake up
            _ok = calibrate();
        }

        Serial.printf("[GYRO] init %s, bias acc=(%.3f, %.3f) g gyro_z=%.2f deg/s\n", _ok ? "ok" : "failed", _bias_ax, _bias_ay, _bias_gz);

        resetOrigin();
        _last_sample_us = micros();
    }

    /// Blocking measurement of mounting orientation and biases, the robot must not move meanwhile.
    bool calibrate(unsigned samples = 500) {
        double sum_ax = 0, sum_ay = 0, sum_az = 0, sum_gx = 0, sum_gy = 0, sum_gz = 0;
        unsigned n = 0;
        for (unsigned i = 0; i < samples; ++i) {
            if (read()) {
                sum_ax += accel_x;
                sum_ay += accel_y;
                sum_az += accel_z;
                sum_gx += gyro_x;
                sum_gy += gyro_y;
                sum_gz += gyro_z;
                ++n;
            }
            delay(2);
        }
        if (n < samples / 2) return false;

        const float mean_ax = sum_ax / n, mean_ay = sum_ay / n, mean_az = sum_az / n;
        if (fabsf(sqrtf(mean_ax * mean_ax + mean_ay * mean_ay + mean_az * mean_az) - 1.f) > 0.1f) {
            Serial.println("[GYRO] calibration: gravity magnitude off, robot moved?");
            return false;
        }
        computeLevel(mean_ax, mean_ay, mean_az);

        // biases in the leveled frame; accel x/y are ~0 here up to the sensor offset, gravity is on z now
        float unused;
        toLevel(mean_ax, mean_ay, mean_az, _bias_ax, _bias_ay, unused);
        toLevel(sum_gx / n, sum_gy / n, sum_gz / n, unused, unused, _bias_gz);
        return true;
    }

    /// Set the current position and heading as origin
    void resetOrigin() {
        heading = 0;
        vel_x = 0;
        vel_y = 0;
        pos_x = 0;
        pos_y = 0;
    }

    /// Hint what the motors do. Still detection only while Idle, position is held while Turning.
    void setMotion(Motion motion) { _motion = motion; }

    /// Blocking read of all measurements into the public members.
    bool read() {
        uint8_t raw[14];
        if (! readRegisters(0x3B, raw, sizeof(raw))) return false;

        auto to_int16 = [&raw](int i) { return static_cast<int16_t>((raw[i] << 8) | raw[i + 1]); };

        constexpr float accel_norm = 1.f / 8192.f;
        accel_x = to_int16(0) * accel_norm;
        accel_y = to_int16(2) * accel_norm;
        accel_z = to_int16(4) * accel_norm;

        constexpr float temp_norm = 1.f / 340.f;
        constexpr float temp_offset = 36.53f;
        temperature = to_int16(6) * temp_norm + temp_offset;

        constexpr float gyro_norm = 1.f / 32.8f;
        gyro_x = to_int16(8) * gyro_norm;
        gyro_y = to_int16(10) * gyro_norm;
        gyro_z = to_int16(12) * gyro_norm;
        return true;
    }

    void loop() {
        if (! _ok) return;

        const unsigned long now_us = micros();
        const unsigned long elapsed_us = now_us - _last_sample_us;
        if (elapsed_us < _sample_every_us) return;
        _last_sample_us = now_us;

        if (! read()) {
            Serial.println("[GYRO] read failed");
            return;
        }

        integrate(elapsed_us * 1e-6f);

        const unsigned long now = millis();
        if (now - _last_print >= _print_every_ms) {
            _last_print = now;
            Serial.printf("[GYRO] pos=(%.3f, %.3f) m  vel=(%.3f, %.3f) m/s  heading=%.1f deg%s\n",
                          pos_x, pos_y, vel_x, vel_y, heading, _stationary ? "  (stationary)" : "");
        }
    }

private:

    void integrate(float dt) {
        // sensor frame -> leveled frame, independent of how the sensor is mounted
        float lax, lay, laz, lgx, lgy, lgz;
        toLevel(accel_x, accel_y, accel_z, lax, lay, laz);
        toLevel(gyro_x, gyro_y, gyro_z, lgx, lgy, lgz);

        // body frame accel without bias and gravity share
        float ax = lax - _bias_ax;
        float ay = lay - _bias_ay;
        const float gz = lgz - _bias_gz;

        // still detection: every sample of the window must be quiet, a hand or motor jitter resets it
        constexpr float still_accel = 0.02f;
        constexpr float still_gyro = 1.5f;
        constexpr unsigned still_window = 40; // samples, 200 ms
        const bool quiet = fabsf(ax) < still_accel && fabsf(ay) < still_accel && fabsf(gz) < still_gyro;
        _still_samples = quiet ? (_still_samples + 1u) : 0;
        _stationary = _motion == Motion::Idle && _still_samples >= still_window;

        if (_stationary) { // zero velocity update, and follow slow bias changes (temperature)
            vel_x = 0;
            vel_y = 0;
            constexpr float bias_alpha = 0.005f;
            _bias_ax += bias_alpha * (lax - _bias_ax);
            _bias_ay += bias_alpha * (lay - _bias_ay);
            _bias_gz += bias_alpha * (lgz - _bias_gz);
            return;
        }

        // heading from the vertical rate, wrapped to [-180, 180)
        heading += gz * dt;
        if (heading >= 180.f) heading -= 360.f;
        else if (heading < -180.f) heading += 360.f;

        if (_motion == Motion::Turning) { // turning on the spot, the position does not change
            vel_x = vel_y = 0;
            return;
        }

        // rotate into the table frame, g -> m/s^2
        const float yaw = heading * DEG_TO_RAD;
        const float c = cosf(yaw), s = sinf(yaw);

        constexpr  float g_to_ms_squared = 9.80665f;
        const float wx = (c * ax - s * ay) * g_to_ms_squared;
        const float wy = (s * ax + c * ay) * g_to_ms_squared;

        pos_x += vel_x * dt + 0.5f * wx * dt * dt;
        pos_y += vel_y * dt + 0.5f * wy * dt * dt;
        vel_x += wx * dt;
        vel_y += wy * dt;
    }

};



#endif //SRC_GYROSENSOR_H
