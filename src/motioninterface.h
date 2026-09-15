#ifndef SRC_MOTORDRIVERINTERFACE_H
#define SRC_MOTORDRIVERINTERFACE_H

#include <Arduino.h>

/** This class implements the pico output interface to the motor drivers for the 4 wheel coaster bot.
 *
 * The coasterbot uses 2 motor drivers TB6612, which both control two wheels.
 * One driver is used for both wheels on one side, but the movement speed of both sides is identical,
 * since the pwm pin is interconnected. The standby pin is shared as well, so the driver can only be
 * enabled or disabled for both sides at once.
 *
 * through this convention, the following movements are possible:
 * - standing still, both in a blocking and nonblocking mode
 * - forward/backward (both sides turn in the same direction)
 * - turn left/right on the spot(left and right turn in opposite directions)
 *
 * a forward curve motion is currently not possible, this would require a wheel speed from both sides
 * in different speeds and with more respect to the geometry of the wheel distances etc.
 * For now, turning on the spot is simple enough for most of out purposes.
 *
 * Nonblocking usage: the main loop only states which motion it wants via stop(), forward(),
 * backward(), turnLeft() or turnRight() - these just record the request and return immediately.
 * Reversing a running motor would stress the gearbox and the driver, so a request is never wired
 * straight through: update() runs the brake -> dead time -> engage sequence in between, one step per
 * call, without any delay().
 *
 * Call begin() once from setup() and update() in every main loop iteration.
 */
template<int pin_pwm, int pin_standby,
  int pin_L1, int pin_L2, int pin_R1, int pin_R2,
  int wait_time_stop = 500, int wait_time_dir_change = 100,
  bool invert_right = true, bool invert_direction = true
>
class MotionController {
private:

  enum Motion { STOPPED, FORWARD, BACKWARD, TURNINGLEFT, TURNINGRIGHT }; // Possible device motion

  enum Phase { // current state
    UNINITIALIZED, // begin() has not run yet, update() keeps its hands off the pins
    IDLE,          // at rest, driver on standby, no deadline
    BRAKING,       // both sides shorted with the driver still enabled, until _deadline
    COASTING,      // driver released, dead time after braking, until _deadline
    ARMING,        // direction pins already set, driver still released, until _deadline
    RUNNING        // driver enabled, the wheels do what was requested
  };

  Phase _phase {UNINITIALIZED};
  unsigned long _deadline {0}; // only meaningful in BRAKING, COASTING and ARMING

  Motion _requested {STOPPED}; // what the main loop asked for
  Motion _applied {STOPPED};   // what is currently wired to the driver inputs

  uint8_t _speed {160};

public:

  MotionController() {
    static_assert(pin_pwm >= 0 && pin_standby >= 0);
    static_assert(pin_L1 >= 0 && pin_L2 >= 0 && pin_R1 >= 0 && pin_R2 >= 0);
    static_assert(pin_L1 != pin_L2 && pin_R1 != pin_R2);
    static_assert(wait_time_stop >= 0 && wait_time_dir_change >= 0);
  }

  /// Configure the output pins and leave the robot at rest. Call once from setup().
  void begin(uint8_t speed = 128) {
    pinMode(pin_pwm, OUTPUT);
    pinMode(pin_standby, OUTPUT);
    pinMode(pin_L1, OUTPUT);
    pinMode(pin_L2, OUTPUT);
    pinMode(pin_R1, OUTPUT);
    pinMode(pin_R2, OUTPUT);

    digitalWrite(pin_standby, LOW); // driver off before anything else touches the inputs
    applyMotion(STOPPED);

    _requested = STOPPED;
    _speed = speed;
    analogWrite(pin_pwm, _speed);

    _phase = IDLE;
  }

public: // motion setters - these only record the request, update() carries it out

  /// Request a full stop. Braking starts with the next update().
  void stop() { _requested = STOPPED; }

  void forward() { _requested = FORWARD; }
  void backward() { _requested = BACKWARD; }
  void turnLeft() { _requested = TURNINGLEFT; }
  void turnRight() { _requested = TURNINGRIGHT; }

  /// Speed of both sides, they share the pwm pin. Takes effect immediately, no transition needed.
  void setSpeed(uint8_t speed) {
    if (_speed == speed) return;
    _speed = speed;
    analogWrite(pin_pwm, _speed);
  }

public: // queries

  /// True while the transition sequence is still running, i.e. the request is not on the wheels yet.
  bool isBusy() const { return _phase == BRAKING || _phase == COASTING || _phase == ARMING; }

  /// True while the driver is enabled and driving the requested motion.
  bool isMoving() const { return _phase == RUNNING; }

  uint8_t speed() const { return _speed; }

public: // main loop repeated call

  void update() { update(millis()); }

  void update(unsigned long curtime) {
    switch (_phase) {
    case UNINITIALIZED:
      break; // begin() has not run, do not drive any pin

    case RUNNING:
      if (_requested == _applied) break; // already driving what was asked for
      applyMotion(STOPPED);              // short brake, the driver stays enabled to make it bite
      _phase = BRAKING;
      _deadline = curtime + wait_time_stop;
      break;

    case BRAKING:
      if (!deadlineReached(curtime)) break;
      digitalWrite(pin_standby, LOW); // release the driver before the inputs are switched
      _phase = COASTING;
      _deadline = curtime + wait_time_dir_change;
      break;

    case COASTING:
      if (!deadlineReached(curtime)) break;
      _phase = IDLE;
      [[fallthrough]]; // at rest now, so a pending request can be picked up in this same call

    case IDLE:
      if (_requested == STOPPED) break; // nothing requested, stay at rest
      applyMotion(_requested);          // driver is released, switching the inputs is safe here
      _phase = ARMING;
      _deadline = curtime + wait_time_dir_change;
      break;

    case ARMING:
      if (_requested == STOPPED) { // stop() during the dead time, never engage in the first place
        applyMotion(STOPPED);
        _phase = IDLE;
        break;
      }
      if (_requested != _applied) { // request changed again, still harmless while released
        applyMotion(_requested);
        _deadline = curtime + wait_time_dir_change;
        break;
      }
      if (!deadlineReached(curtime)) break;
      digitalWrite(pin_standby, HIGH);
      _phase = RUNNING;
      break;
    }
  }

private:

  bool deadlineReached(unsigned long curtime) const {
    return static_cast<long>(curtime - _deadline) >= 0; // wraparound safe, only the difference counts
  }

  /// Wire a motion to the driver inputs. Only safe while the driver is released, except for
  /// STOPPED - the short brake is meant to be applied with the driver still enabled.
  void applyMotion(Motion motion) {
    switch (motion) {
    default:
    case STOPPED:
      setOutputPins(HIGH, HIGH, HIGH, HIGH); // both inputs high = short brake on the TB6612
      break;

    case FORWARD:
      setDirection(true, true);
      break;

    case BACKWARD:
      setDirection(false, false);
      break;

    case TURNINGLEFT:
      setDirection(false, true);
      break;

    case TURNINGRIGHT:
      setDirection(true, false);
      break;
    }

    _applied = motion;
  }

  void setDirection(bool left_fwd, bool right_fwd) {
    if (invert_right) right_fwd = !right_fwd;
    if (invert_direction) {
      left_fwd = !left_fwd;
      right_fwd = !right_fwd;
    }
    setOutputPins(left_fwd ? HIGH : LOW, left_fwd ? LOW : HIGH, right_fwd ? HIGH : LOW, right_fwd ? LOW : HIGH);
  }

  void setOutputPins(int l1, int l2, int r1, int r2) {
    digitalWrite(pin_L1, l1);
    digitalWrite(pin_L2, l2);
    digitalWrite(pin_R1, r1);
    digitalWrite(pin_R2, r2);
  }

};

#endif //SRC_MOTORDRIVERINTERFACE_H
