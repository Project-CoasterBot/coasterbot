#ifndef SRC_NAVIGATION_H
#define SRC_NAVIGATION_H

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include <Arduino.h>

/** Simple waypoint navigation on top of the wheel odometry.
 *
 * The bot can only drive straight (forward/backward) or turn on the spot, both sides always run at the
 * same speed. A waypoint is therefore approached in single moves: turn towards the point, drive towards
 * it, stop, let the bot settle, then evaluate the pose again. If the bot is not within the position
 * tolerance yet, the next move corrects the remaining error.
 *
 * The obstacle sensor only looks forward, so the bot drives forward wherever possible, a point behind it is
 * approached by turning around. Driving backward is only used while the front is blocked (edge or obstacle,
 * see frontBlocked() of the bot): straight back to the point if it lies behind, otherwise a short escape
 * move after which the bot can turn again.
 *
 * Points are given in m in the odometry frame of the bot (see HardwareImplementationHAL::getPosition()),
 * i.e. relative to the start point or the last resetPose(), +x = initial driving direction, +y = left.
 *
 * Usage:
 *   nav.setPoints({{0.1f, 0.f}, {0.1f, -0.1f}, {0.f, 0.f}});
 *   nav.start();
 *   loop: _bot.step(); nav.run(_bot); switch (nav.state()) { case Navigation::State::Completed: ...;
 *         case Navigation::State::Failed: nav.outputErrorToSerial(); ... }
 *
 * run() only sets the wheel speeds while the navigation is active, the bot itself is stepped by the caller.
 * The bot type needs getPosition(), getHeading(), setLeftSpeed(), setRightSpeed(), movementPrevented() and
 * frontBlocked().
 */
class Navigation {
public:
    struct Point {
        float x, y; // in m
    };

    enum class Error : uint8_t {
        None,
        MovementPrevented,  // edge or obstacle sensor blocked the requested motion
        NoProgress,         // motion requested, but the odometry did not change (stalled, encoder?)
        TooManyCorrections  // waypoint not reached within the allowed number of moves
    };

    /// Current state, see state(). Plan, Turning, Driving and Settling mean the navigation is running.
    enum class State : uint8_t {
        Idle,      // not started, aborted or reset, run() does not touch the bot
        Plan,      // bot is at rest, decide the next move
        Turning,   // turning on the spot towards _target_heading
        Driving,   // driving straight towards _drive_target, the current point or an escape position
        Settling,  // stopped, waiting until brake and coasting are over and the pose is stable
        Completed, // all points reached
        Failed     // error, see error() and outputErrorToSerial()
    };

private:

    std::vector<Point> _points;
    size_t _index {0};      // current point
    unsigned _moves {0};    // moves spent on the current point
    unsigned _stall_retries {0}; // consecutive moves that stalled (NoProgress), reset by any progress

    State _state {State::Idle};
    Error _error {Error::None};

    float _target_heading {0.f}; // in rad, only while turning
    Point _drive_target {0.f, 0.f}; // in m, only while driving
    int _direction {1};          // turning: +1 left (ccw), -1 right. driving: +1 forward, -1 backward

    // pose tracking for settle and stall detection
    float _last_x {0.f}, _last_y {0.f}, _last_heading {0.f}; // pose at the last significant change
    unsigned long _last_change_ms {0}; // last time the pose changed significantly, settle and stall detection
    // below these changes the pose counts as unchanged: the gyro heading creeps by noise and bias while standing
    static constexpr float _still_position {0.002f}; // in m, below one encoder tick (~5 mm)
    static constexpr float _still_heading {0.02f};   // in rad, ~1 deg
    unsigned long _stop_ms {0};        // time the last move was stopped

    // pose and point when the error occurred, for outputErrorToSerial()
    float _error_x {0.f}, _error_y {0.f}, _error_heading {0.f};
    size_t _error_index {0};

    // parameters
    float _position_tolerance {0.02f};   // in m, point counts as reached within this distance
    float _heading_tolerance {0.07f};    // in rad (~4 deg), no turn needed if aligned within this angle
    float _turn_stop_ahead {0.f};        // in rad, stop turning this much early to compensate the brake overshoot
    float _drive_stop_ahead {0.005f};    // in m, stop driving this much early to compensate the rolling out
    float _escape_distance {0.05f};      // in m, backward move away from a blocked front before turning
    unsigned _max_moves {8};             // moves per point before giving up
    unsigned long _settle_min_ms {650};  // brake (500 ms) + coasting (100 ms) of the motion controller + margin
    unsigned long _settle_still_ms {150};// pose must not change for this long to count as settled
    unsigned long _stall_timeout_ms {2000};    // no pose change for this long while moving -> retry or NoProgress
    unsigned _max_stall_retries {2};           // a stalled move is repeated this often before NoProgress
    unsigned long _prevented_timeout_ms {300}; // movement prevented for this long -> MovementPrevented

    static float wrapAngle(float angle) {
        return std::remainder(angle, 2.f * static_cast<float>(M_PI)); // [-pi, pi]
    }

public: // waypoints

    void setPoints(std::initializer_list<Point> points) { _points.assign(points.begin(), points.end()); }
    void setPoints(const Point* points, size_t count) { _points.assign(points, points + count); }
    void addPoint(float x, float y) { _points.push_back({x, y}); }
    void clearPoints() { _points.clear(); }
    size_t pointCount() const { return _points.size(); }

    /// Index of the point currently approached, == pointCount() once completed.
    size_t currentPoint() const { return _index; }

public: // parameters

    void setPositionTolerance(float meter) { if (std::isfinite(meter) && meter > 0.f) _position_tolerance = meter; }
    void setHeadingTolerance(float rad) { if (std::isfinite(rad) && rad > 0.f) _heading_tolerance = rad; }
    void setTurnStopAhead(float rad) { if (std::isfinite(rad) && rad >= 0.f) _turn_stop_ahead = rad; }
    void setDriveStopAhead(float meter) { if (std::isfinite(meter) && meter >= 0.f) _drive_stop_ahead = meter; }
    void setEscapeDistance(float meter) { if (std::isfinite(meter) && meter > 0.f) _escape_distance = meter; }
    void setMaxMovesPerPoint(unsigned moves) { if (moves > 0) _max_moves = moves; }
    void setPreventedTimeout(unsigned long ms) { _prevented_timeout_ms = ms; }
    void setMaxStallRetries(unsigned retries) { _max_stall_retries = retries; }

public: // control

    /// Start approaching the points from the first one. Also clears a previous error.
    void start() {
        _index = 0;
        _moves = 0;
        _stall_retries = 0;
        _error = Error::None;
        _state = _points.empty() ? State::Completed : State::Plan;
    }

    /// Stop the bot and end the navigation, state() is Idle afterwards.
    template<typename BotImpl>
    void abort(BotImpl& bot) {
        stopWheels(bot);
        _state = State::Idle;
    }

    /// Stop the bot and go back to the initial state: no error, no points, run() idle until start().
    /// The odometry is not touched (see resetPose() of the bot).
    template<typename BotImpl>
    void reset(BotImpl& bot) {
        abort(bot);
        _points.clear();
        _index = 0;
        _moves = 0;
        _stall_retries = 0;
        _error = Error::None;
    }

    State state() const { return _state; }
    bool running() const { return _state != State::Idle && _state != State::Completed && _state != State::Failed; }
    bool completed() const { return _state == State::Completed; }
    Error error() const { return _error; }

    void outputErrorToSerial() const {
        if (_error == Error::None) return;
        const char* reason = "unknown";
        switch (_error) {
        case Error::None: break;
        case Error::MovementPrevented: reason = "movement prevented (edge or obstacle)"; break;
        case Error::NoProgress: reason = "no progress, odometry did not change while moving"; break;
        case Error::TooManyCorrections: reason = "point not reached within the allowed moves"; break;
        }
        Serial.printf("[NAV] error: %s at point %u", reason, static_cast<unsigned>(_error_index));
        if (_error_index < _points.size())
            Serial.printf(" (%.3f / %.3f)", _points[_error_index].x, _points[_error_index].y);
        Serial.printf(", pose %.3f / %.3f heading %.1f deg\n", _error_x, _error_y, _error_heading * RAD_TO_DEG);
    }

    /// Call in every main loop iteration, after bot.step().
    template<typename BotImpl>
    void run(BotImpl& bot) {
        if (! running()) return;

        const unsigned long now = millis();
        float x, y;
        bot.getPosition(x, y);
        const float heading = bot.getHeading();

        if (std::hypot(x - _last_x, y - _last_y) > _still_position
            || std::fabs(wrapAngle(heading - _last_heading)) > _still_heading) {
            _last_x = x;
            _last_y = y;
            _last_heading = heading;
            _last_change_ms = now;
            if (_state == State::Turning || _state == State::Driving) _stall_retries = 0; // the move got going
        }

        switch (_state) {
        case State::Plan:
            plan(bot, now, x, y, heading);
            break;

        case State::Turning: {
            if (checkFailures(bot, now, x, y, heading)) break;
            const float remaining = wrapAngle(_target_heading - heading) * static_cast<float>(_direction);
            if (remaining <= _turn_stop_ahead) { // also catches an overshoot, remaining gets negative
                stopMove(bot, now);
                break;
            }
            bot.setLeftSpeed(static_cast<float>(-_direction));
            bot.setRightSpeed(static_cast<float>(_direction));
            break;
        }

        case State::Driving: {
            if (checkFailures(bot, now, x, y, heading)) break;
            const Point& p = _drive_target;
            const float along = std::cos(heading) * (p.x - x) + std::sin(heading) * (p.y - y);
            if (along * static_cast<float>(_direction) <= _drive_stop_ahead) {
                stopMove(bot, now);
                break;
            }
            bot.setLeftSpeed(static_cast<float>(_direction));
            bot.setRightSpeed(static_cast<float>(_direction));
            break;
        }

        case State::Settling:
            if (now - _stop_ms >= _settle_min_ms && now - _last_change_ms >= _settle_still_ms)
                _state = State::Plan;
            break;

        default:
            break;
        }
    }

private:

    /// Bot is at rest: decide whether the point is reached, or which move comes next.
    template<typename BotImpl>
    void plan(BotImpl& bot, unsigned long now, float x, float y, float heading) {
        if (_index >= _points.size()) {
            _state = State::Completed;
            return;
        }

        const Point& p = _points[_index];
        const float dx = p.x - x, dy = p.y - y;
        if (std::hypot(dx, dy) <= _position_tolerance) {
            Serial.printf("[NAV] reached point %u (%.3f / %.3f), pose %.3f / %.3f\n",
                          static_cast<unsigned>(_index), p.x, p.y, x, y);
            ++_index;
            _moves = 0;
            if (_index >= _points.size()) _state = State::Completed;
            return; // next point is planned in the next call
        }

        if (_moves >= _max_moves) {
            fail(bot, Error::TooManyCorrections, x, y, heading);
            return;
        }
        ++_moves;

        // target in the bot frame: along = ahead, lateral = to the left
        const float c = std::cos(heading), s = std::sin(heading);
        const float along = c * dx + s * dy;
        const float lateral = -s * dx + c * dy;
        const float to_target = std::atan2(dy, dx);
        const float err_forward = wrapAngle(to_target - heading);
        const float err_backward = wrapAngle(err_forward + static_cast<float>(M_PI));

        const bool on_line = std::fabs(lateral) <= _position_tolerance;
        const bool ahead = along > 0.f && (on_line || std::fabs(err_forward) <= _heading_tolerance);
        const bool behind = along < 0.f && (on_line || std::fabs(err_backward) <= _heading_tolerance);

        if (! bot.frontBlocked()) {
            if (ahead) { // straight ahead, no turn needed
                startDriving(p, 1);
            } else { // face the point, also if it is behind: the obstacle sensor only looks forward
                _target_heading = to_target;
                _direction = err_forward > 0.f ? 1 : -1;
                _state = State::Turning;
            }
        } else if (behind) { // front blocked, the point is straight behind
            startDriving(p, -1);
        } else if (ahead) { // front blocked, the point is straight ahead: no way to get there
            fail(bot, Error::MovementPrevented, x, y, heading);
            return;
        } else { // front blocked, back off a bit, then the next move can turn towards the point
            startDriving({x - c * _escape_distance, y - s * _escape_distance}, -1);
        }
        _last_change_ms = now; // stall timer starts with the move
    }

    void startDriving(const Point& target, int direction) {
        _drive_target = target;
        _direction = direction;
        _state = State::Driving;
    }

    template<typename BotImpl>
    bool checkFailures(BotImpl& bot, unsigned long now, float x, float y, float heading) {
        unsigned long prevented_ms;
        if (bot.movementPrevented(prevented_ms) && prevented_ms >= _prevented_timeout_ms) {
            fail(bot, Error::MovementPrevented, x, y, heading);
            return true;
        }
        if (now - _last_change_ms >= _stall_timeout_ms) {
            if (_stall_retries >= _max_stall_retries) {
                fail(bot, Error::NoProgress, x, y, heading);
                return true;
            }
            // stop, let the motors rest through brake and settle, then plan the same move again
            ++_stall_retries;
            if (_moves > 0) --_moves; // the stalled move did nothing, it does not count against the point
            Serial.printf("[NAV] no progress at point %u, retry %u of %u\n",
                          static_cast<unsigned>(_index), _stall_retries, _max_stall_retries);
            stopMove(bot, now);
            return true;
        }
        return false;
    }

    template<typename BotImpl>
    void stopMove(BotImpl& bot, unsigned long now) {
        stopWheels(bot);
        _stop_ms = now;
        _state = State::Settling;
    }

    template<typename BotImpl>
    void fail(BotImpl& bot, Error error, float x, float y, float heading) {
        stopWheels(bot);
        _error = error;
        _error_index = _index;
        _error_x = x;
        _error_y = y;
        _error_heading = heading;
        _state = State::Failed;
    }

    template<typename BotImpl>
    static void stopWheels(BotImpl& bot) {
        bot.setLeftSpeed(0.f);
        bot.setRightSpeed(0.f);
    }
};

#endif //SRC_NAVIGATION_H
