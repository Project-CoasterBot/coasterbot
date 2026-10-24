#ifndef SRC_COASTER_SERVO_CONFIG_H
#define SRC_COASTER_SERVO_CONFIG_H

#include <Arduino.h>

#include "boardconfig.h"
#include "webots/coaster_servo_angles.h"

/// Servowinkel des Coaster-Mechanismus aus boardconfig(_local).h.
inline CoasterServoAngles boardCoasterServoAngles() {
    static_assert(board::SERVO_LIFTER_STACK_LEVELS >= 1
                  && board::SERVO_LIFTER_STACK_LEVELS <= CoasterServoAngles::MAX_STACK_LEVELS,
                  "BOARD_SERVO_LIFTER_STACK_ANGLES needs 1..MAX_STACK_LEVELS entries");
    for (int angle : board::SERVO_LIFTER_STACK_ANGLES)
        if (angle < 0 || angle > 180) Serial.printf("[CONFIG] lifter stack angle %d out of 0..180\n", angle);

    CoasterServoAngles a;
    a.lifterRest = board::SERVO_LIFTER_ANGLE_REST;
    a.lifterMax = board::SERVO_LIFTER_ANGLE_MAX;
    a.spenderIdle = board::SERVO_SPENDER_ANGLE_IDLE;
    a.spenderEject = board::SERVO_SPENDER_ANGLE_EJECT;
    a.spenderEjectStrike = board::SERVO_SPENDER_EJECT_STRIKE;
    a.lifterStackLevels = board::SERVO_LIFTER_STACK_LEVELS;
    for (int i = 0; i < board::SERVO_LIFTER_STACK_LEVELS; ++i)
        a.lifterStack[i] = board::SERVO_LIFTER_STACK_ANGLES[i];
    return a;
}

#endif // SRC_COASTER_SERVO_CONFIG_H
