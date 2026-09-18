#ifndef COASTERBOT_FUNCTIONS_H
#define COASTERBOT_FUNCTIONS_H

#include "robot_hal.h"
#include "robot_logic.h"
#include "pose_estimator.h"
#include "safety_monitor.h"
#include "occupancy_grid.h"
#include "dstar_lite.h"
#include <memory>
#include <vector>

struct Vec4Coasterbot {
    float x = 0.0f;
    float y = 0.0f;
    bool operator==(const Vec4Coasterbot& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec4Coasterbot& o) const { return !(*this == o); }
};


class CoasterbotFunctions : RobotLogic {
    public:
    explicit CoasterbotFunctions(RobotHAL& hal, PoseEstimator& pose, SafetyMonitor& safety);

    // Einmal pro Regelzyklus aufrufen (autonomer Modus, Zustandsautomat).
    void update();


    void runLearnTable();
    void runNavigateTargets();
    void navigateTo(Vec2 destination, float speed);
    void navigateToWithObstacleAvoidance(Vec2 destination, float speed);
    bool isTableLearned() const { return tableLearned; }
    void runSpendCoaster();
    void runGetCoaster();
    void runDriveTest();
    std::vector<Vec4Coasterbot> getCoasterPositions(int coasterIndex);

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
    float robotBoundingBoxWidth_ = 0.35f;  // [m]
    float robotBoundingBoxLength_ = 0.35f; // [m]
    float obstacleRecoveryDirection_ = 1.0f;

    void navigateToInternal(Vec2 destination, float speed, bool useRobotBoundingBox);
    void rebuildNavigationGrid();
    static constexpr int MAX_COASTERS = 4;
    static constexpr float HALF_PI = 1.57079632679489661923f;
    std::vector<Vec4Coasterbot> coasterPositions[MAX_COASTERS];

};

#endif  // COASTERBOT_FUNCTIONS_H

