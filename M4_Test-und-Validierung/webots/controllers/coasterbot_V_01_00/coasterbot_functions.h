#ifndef COASTERBOT_FUNCTIONS_H
#define COASTERBOT_FUNCTIONS_H

#include "webots_hal.h"
#include "robot_logic.h"
#include "pose_estimator.h"
#include "safety_monitor.h"
#include "occupancy_grid.h"
#include "dstar_lite.h"
#include <array>
#include <memory>
#include <vector>

struct Vec4Coasterbot {
    float x = 0.0f;
    float y = 0.0f;
    bool operator==(const Vec4Coasterbot& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec4Coasterbot& o) const { return !(*this == o); }
};


class CoasterbotFunctions : public RobotLogic {
    public:
    explicit CoasterbotFunctions(RobotHAL& hal, PoseEstimator& pose, SafetyMonitor& safety);

    // Einmal pro Regelzyklus aufrufen (autonomer Modus, Zustandsautomat).
    void update();


    void runLearnTable();
    void runNavigateTargets();
    void navigateTo(Vec2 destination, float speed);
    void navigateToWithObstacleAvoidance(Vec2 destination, float speed);
    bool isTableLearned() const { return this->tableLearned; }
    void setTableLearned(bool learned) { this->tableLearned = learned; }
    bool buttonPressed() const { return hal_.getButtonState(); }
    bool safetyOverriding() const { return safety_.overriding(); }
    bool pathExists() const { return dstar_ && dstar_->pathExists(); }
    bool runSpendCoasters();
    bool runGetCoasters();
    bool runCollectCoasters() { return runGetCoasters(); }
    bool coasterRoutineFailed() const { return this->coasterRoutineFailed_; }
    bool spendCoaster(int coasterIndex);
    bool pickCoaster(int coasterIndex);
    void runDriveTest();
    void led(RobotHAL::LED_COLORS color) { hal_.led(color); }
    std::vector<Vec2> getCoasterPositions(int coasterIndex);

    private:
    RobotHAL& hal_;
    PoseEstimator& pose_;
    SafetyMonitor& safety_;
    std::unique_ptr<OccupancyGrid> grid_;
    std::unique_ptr<DStarLite> dstar_;

    bool tableLearned = false;
    float tableWidth_ = 0.0f;
    float tableHeight_ = 0.0f;
    Vec2 navigationTarget_{};
    float navigationInitialDistance_ = 0.0f;  // [m]
    bool navigationTargetInitialized_ = false;
    int edgeInterruptions_ = 0;
    bool edgeStopLatched_ = false;
    bool edgeWasActive_ = false;
    bool edgeReroutePending_ = false;
    size_t navigationPointIndex_ = 0;
    bool navigationSequenceComplete_ = false;
    bool coasterRoutineActive_ = false;
    bool coasterRoutineFailed_ = false;
    int coasterRoutineIndex_ = 0;
    float robotBoundingBoxWidth_ = 0.1f;  // [m]
    float robotBoundingBoxLength_ = 0.1f; // [m]
    float obstacleRecoveryDirection_ = 1.0f;

    void navigateToInternal(Vec2 target, float speed, bool useRobotBoundingBox);
    void rebuildNavigationGrid();
    void calcCoasterPositions();
    static constexpr int MAX_COASTERS = 4;
    static constexpr float HALF_PI = 1.57079632679489661923f;
    std::array<Vec2, MAX_COASTERS> coasterPositions;

    const std::array<uint8_t, MAX_COASTERS> SERVO_POS_LIFTER = { 0, 45, 90, 135 }; // Servo positions for lifter (degrees)
    const std::array<uint8_t, MAX_COASTERS> SERVO_POS_SPENDER = { 0, 90, 180 }; // Servo positions for lifter (degrees)

    enum class ServoId : uint8_t { LIFTER, SPENDER };

};

#endif  // COASTERBOT_FUNCTIONS_H

