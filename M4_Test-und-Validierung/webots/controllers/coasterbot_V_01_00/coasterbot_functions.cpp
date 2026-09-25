#include "coasterbot_functions.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iomanip>

CoasterbotFunctions::CoasterbotFunctions(RobotHAL& hal,
                                         PoseEstimator& pose,
                                         SafetyMonitor& safety)
    : RobotLogic(hal),
      hal_(hal), pose_(pose), safety_(safety),
      grid_(std::make_unique<OccupancyGrid>(-1.5f, -1.5f, 1.5f, 1.5f, 0.05f)),
      dstar_(std::make_unique<DStarLite>(*grid_)) {}

void CoasterbotFunctions::update() {
	// These components have state that must be refreshed once per control cycle.
	this->pose_.update();
	//this->RobotLogic::update();
	this->safety_.update();
}

void CoasterbotFunctions::calcCoasterPositions() {
	constexpr float edgeMargin = 0.35f;
	const float x = std::max(0.0f, tableWidth_ / 2.0f - edgeMargin);
	const float y = std::max(0.0f, tableHeight_ / 2.0f - edgeMargin);
	const std::array<Vec2, 4> targets{{
		{-x, -y},
		{ x, -y},
		{ x,  y},
		{-x,  y}
	}};
	for (size_t i = 0; i < targets.size(); ++i) {
		this->coasterPositions[i] = targets[i];
	}
}

void CoasterbotFunctions::runNavigateTargets() {
	if (!tableLearned) {
		this->stop();
		return;
	}

	if (navigationSequenceComplete_) {
		this->stop();
		return;
	}

	constexpr float edgeMargin = 0.35f;
	const float x = std::max(0.0f, tableWidth_ / 2.0f - edgeMargin);
	const float y = std::max(0.0f, tableHeight_ / 2.0f - edgeMargin);
	const std::array<Vec2, 4> targets{{
		{-x, -y},
		{ x, -y},
		{ x,  y},
		{-x,  y}
	}};
	const Vec2 target = targets[navigationPointIndex_];

	navigateToWithObstacleAvoidance(target, 5.6f);

	const float distanceToTarget = std::hypot(target.x - pose_.getX(), target.y - pose_.getY());
	if (distanceToTarget <= 0.08f) {
		if (navigationPointIndex_ == targets.size() - 1) {
			navigationSequenceComplete_ = true;
			std::cout << "[NAV] four-point navigation completed" << std::endl;
		} else {
			++navigationPointIndex_;
			std::cout << "[NAV] proceeding to point " << navigationPointIndex_ + 1 << "/4" << std::endl;
		}
	}
}

void CoasterbotFunctions::navigateTo(Vec2 target, float speed) {
	navigateToInternal(target, speed, false);
}

void CoasterbotFunctions::navigateToWithObstacleAvoidance(Vec2 target, float speed) {
	navigateToInternal(target, speed, true);
}

void CoasterbotFunctions::navigateToInternal(Vec2 target, float speed, bool useRobotBoundingBox) {
	const float distanceAtCommand = std::hypot(target.x - pose_.getX(), target.y - pose_.getY());
	if (!navigationTargetInitialized_ || target != navigationTarget_) {
		navigationTarget_ = target;
		navigationInitialDistance_ = distanceAtCommand;
		navigationTargetInitialized_ = true;
		edgeInterruptions_ = 0;
		edgeStopLatched_ = false;
		edgeWasActive_ = false;
	}

	if (edgeStopLatched_) {
		this->stop();
		return;
	}

	const bool edgeActive = this->edgeDetected();
	const bool newEdgeInterruption = safety_.edgeEventThisTick() ||
		(edgeActive && !edgeWasActive_);
	edgeWasActive_ = edgeActive;
	if (newEdgeInterruption) {
		++edgeInterruptions_;
		std::cout << "[NAV] edge interruption " << edgeInterruptions_ << "/3" << std::endl;
		if (edgeInterruptions_ >= 3) {
			edgeStopLatched_ = true;
			this->stop();
			std::cout << "[NAV] three edge interruptions reached -> navigation stopped" << std::endl;
			return;
		}

		if (grid_ && tableLearned) {
			const bool frontEdge = safety_.frontEdge();
			const float direction = frontEdge ? 1.0f : -1.0f;
			const float edgeDistance = 0.20f;
			const float edgeX = pose_.getX() + direction * edgeDistance * std::cos(pose_.getTheta());
			const float edgeY = pose_.getY() + direction * edgeDistance * std::sin(pose_.getTheta());
			const float halfWidth = std::fabs(robotBoundingBoxWidth_) * 0.5f + 0.05f;
			const float halfLength = std::fabs(robotBoundingBoxLength_) * 0.5f + 0.05f;
			grid_->addObstacleRect(edgeX - halfLength, edgeY - halfWidth,
			                       edgeX + halfLength, edgeY + halfWidth, 0.0f);
			grid_->buildDistanceField();
			edgeReroutePending_ = true;
			std::cout << "[NAV] edge recorded in map -> fresh route will be planned after recovery" << std::endl;
		}
	}

	if (safety_.overriding()) {
		return;
	}
	if (edgeReroutePending_) {
		edgeReroutePending_ = false;
		std::cout << "[NAV] edge recovery complete -> replanning from current position" << std::endl;
	}
	if (edgeActive) {
		this->stop();
		return;
	}

    if (!tableLearned || !grid_ || !dstar_) {
        this->stop();
        return;
    }

	const float driveSpeed = std::fabs(speed);
	if (driveSpeed <= 0.0f) {
		this->stop();
		return;
	}

    const Cell startCell = grid_->worldToCell({pose_.getX(), pose_.getY()});
    const Cell goalCell = grid_->worldToCell(target);
	const float distToGoal = std::hypot(target.x - pose_.getX(), target.y - pose_.getY());
	const float slowDownDistance = navigationInitialDistance_ * 0.10f;
	const bool inSlowDownZone = distToGoal <= slowDownDistance;
	const float commandSpeed = driveSpeed * (inSlowDownZone ? 0.5f : 1.0f);
	const float obstacleThreshold = 0.55f; // [m] -> replanning threshold
	const float us = hal_.getUltrasonicDistance();
	const bool obstacleDetected = std::isfinite(us) && us >= 0.0f && us < obstacleThreshold;
	const float now = hal_.getTime();
	static float lastNavLog = -5.0f;
	static bool obstacleWasDetected = false;
	static bool obstacleScanActive = false;
	static float obstacleScanStartTheta = 0.0f;
	static float obstacleScanPreviousTheta = 0.0f;
	static float obstacleScanTurnedAngle = 0.0f;
	static int obstacleAvoidancePhase = 0;
	static float obstacleAvoidanceUntil = 0.0f;
	static float obstacleAvoidanceDirection = 1.0f;
	constexpr float fullTurnAngle = 6.28318530718f;
	const bool logNav = (now - lastNavLog) >= 5.0f;
	if (logNav) {
		lastNavLog = now;
	}

	auto obstacleNearRobotBoundingBox = [&]() {
		const float halfWidth = std::fabs(robotBoundingBoxWidth_) * 0.5f;
		const float halfLength = std::fabs(robotBoundingBoxLength_) * 0.5f;
		const float x = pose_.getX();
		const float y = pose_.getY();
		const float theta = pose_.getTheta();
		const float cosTheta = std::cos(theta);
		const float sinTheta = std::sin(theta);

		const std::array<Vec2, 8> scanPoints{{
			{ x, y },
			{ x + cosTheta * halfLength, y + sinTheta * halfLength },
			{ x - cosTheta * halfLength, y - sinTheta * halfLength },
			{ x + sinTheta * halfWidth, y - cosTheta * halfWidth },
			{ x - sinTheta * halfWidth, y + cosTheta * halfWidth },
			{ x + cosTheta * halfLength + sinTheta * halfWidth,
			  y + sinTheta * halfLength - cosTheta * halfWidth },
			{ x + cosTheta * halfLength - sinTheta * halfWidth,
			  y + sinTheta * halfLength + cosTheta * halfWidth },
			{ x - cosTheta * halfLength + sinTheta * halfWidth,
			  y - sinTheta * halfLength - cosTheta * halfWidth }
		}};

		for (const Vec2& probe : scanPoints) {
			const Cell probeCell = grid_->worldToCell(probe);
			if (grid_->inBounds(probeCell) && grid_->occupied(probeCell)) {
				return true;
			}
		}
		return false;
	};

	if (logNav) {
		std::cout << std::fixed << std::setprecision(3)
			  << "[NAV] grid: cols=" << grid_->cols() << ", rows=" << grid_->rows()
			  << ", resolution=" << grid_->resolution() << "m"
			  << ", valid points: startCell=(" << startCell.cx << ", " << startCell.cy << ")"
			  << ", goalCell=(" << goalCell.cx << ", " << goalCell.cy << ")"
			  << ", startValid=" << (grid_->inBounds(startCell) && !grid_->occupied(startCell) ? "yes" : "no")
			  << ", goalValid=" << (grid_->inBounds(goalCell) && !grid_->occupied(goalCell) ? "yes" : "no")
			  << ", currentPos=(" << pose_.getX() << ", " << pose_.getY() << ", " << pose_.getTheta() << ")"
			  << ", destination=(" << target.x << ", " << target.y << ")"
			  << ", distanceToGoal=" << distToGoal << "m"
			  << ", ultrasonic=" << us << "m"
			  << ", obstacleCandidate=" << (obstacleDetected ? "yes" : "no") << std::endl;
	}

	if (obstacleAvoidancePhase == 0 && obstacleDetected) {
		const float obstacleDistance = std::max(us + 0.12f, 0.12f);
		const float theta = pose_.getTheta();
		const float obstacleX = pose_.getX() + obstacleDistance * std::cos(theta);
		const float obstacleY = pose_.getY() + obstacleDistance * std::sin(theta);
		const float halfWidth = std::fabs(robotBoundingBoxWidth_) * 0.5f ;
		const float halfLength = std::fabs(robotBoundingBoxLength_) * 0.5f ;
		grid_->addObstacleRect(obstacleX - halfLength, obstacleY - halfWidth,
		                       obstacleX + halfLength, obstacleY + halfWidth, 0.0f);
		grid_->buildDistanceField();
		obstacleAvoidanceDirection = -obstacleAvoidanceDirection;
		obstacleAvoidancePhase = 1;
		obstacleAvoidanceUntil = now + 0.35f;
		std::cout << "[NAV] obstacle detected at " << us
		          << "m -> reverse and turn before replanning" << std::endl;
	}

	if (obstacleAvoidancePhase == 1) {
		const float reverseSpeed = std::clamp(commandSpeed * 0.45f, 1.5f, 2.5f);
		hal_.setLeftSpeed(-reverseSpeed);
		hal_.setRightSpeed(-reverseSpeed);
		if (now >= obstacleAvoidanceUntil) {
			obstacleAvoidancePhase = 2;
			obstacleAvoidanceUntil = now + 1.25f;
			std::cout << "[NAV] reverse complete -> persistent escape turn" << std::endl;
		}
		return;
	}

	if (obstacleAvoidancePhase == 2) {
		const float turnSpeed = std::clamp(commandSpeed * 0.55f, 1.8f, 3.0f);
		hal_.setLeftSpeed(-obstacleAvoidanceDirection * turnSpeed);
		hal_.setRightSpeed(obstacleAvoidanceDirection * turnSpeed);
		if (now >= obstacleAvoidanceUntil) {
			obstacleAvoidancePhase = 3;
			obstacleAvoidanceUntil = now + 0.40f;
			std::cout << "[NAV] escape turn complete -> replanning around obstacle" << std::endl;
		}
		return;
	}

	if (obstacleAvoidancePhase == 3) {
		this->stop();
		if (now < obstacleAvoidanceUntil) {
			return;
		}
		obstacleAvoidancePhase = 0;
	}

	if (!grid_->inBounds(goalCell) || grid_->occupied(goalCell)) {
		if (logNav) {
			std::cout << "[NAV] goal cell is outside the grid or occupied; choose a point farther from the table border" << std::endl;
		}
		//this->stop();
		this->turnLeft(1.0f);
		rebuildNavigationGrid();
		return;
	}

    dstar_->plan(startCell, goalCell);
    std::vector<Vec2> path = dstar_->extractPath();
	if (logNav && !path.empty()) {
		std::cout << "[NAV] valid path points=" << path.size() << ": first=("
			  << path.front().x << ", " << path.front().y << ")"
			  << ", last=(" << path.back().x << ", " << path.back().y << ")"
			  << ", waypoint=(" << ((path.size() > 1) ? path[1].x : path.back().x)
			  << ", " << ((path.size() > 1) ? path[1].y : path.back().y) << ")" << std::endl;
	} else if (logNav) {
		std::cout << "[NAV] valid path points=0 (no route found)" << std::endl;
	}

	if (!path.empty()) {
		obstacleWasDetected = false;
	} else {
		if (!obstacleScanActive) {
			obstacleScanActive = true;
			obstacleScanStartTheta = pose_.getTheta();
			obstacleScanPreviousTheta = pose_.getTheta();
			obstacleScanTurnedAngle = 0.0f;
			rebuildNavigationGrid();
			std::cout << "[NAV] no path found -> refreshed dynamic map and starting full scan around the robot bounding box" << std::endl;
		}

		const float headingStep = std::atan2(std::sin(pose_.getTheta() - obstacleScanPreviousTheta),
										 std::cos(pose_.getTheta() - obstacleScanPreviousTheta));
		obstacleScanTurnedAngle += std::fabs(headingStep);
		obstacleScanPreviousTheta = pose_.getTheta();
		const bool fullTurnComplete = obstacleScanTurnedAngle >= (fullTurnAngle - 0.35f);
		const float scanSpeed = std::clamp(commandSpeed * 0.65f, 0.8f, 2.0f);

		if (obstacleNearRobotBoundingBox()) {
			std::cout << "[NAV] obstacle detected within robot bounding box during scan" << std::endl;
		}

		if (!fullTurnComplete) {
			hal_.setLeftSpeed(-scanSpeed);
			hal_.setRightSpeed(scanSpeed);
			return;
		}

		obstacleScanActive = false;
		obstacleWasDetected = true;
		std::cout << "[NAV] full bounding-box scan complete -> trying to replan again" << std::endl;
		dstar_->plan(startCell, goalCell);
		path = dstar_->extractPath();
	}

	if (!grid_->inBounds(startCell) || grid_->occupied(startCell)) {
		if (logNav) {
			const Vec2 startCellCenter = grid_->cellToWorld(startCell);
			std::cout << "[NAV] invalid start: inBounds=" << (grid_->inBounds(startCell) ? "yes" : "no")
			          << ", occupied=" << (grid_->occupied(startCell) ? "yes" : "no")
			          << ", cellCenter=(" << startCellCenter.x << ", " << startCellCenter.y << ")"
			          << ", clearance=" << grid_->clearance({pose_.getX(), pose_.getY()}) << "m"
			          << std::endl;
		}
		if (grid_->inBounds(startCell) && grid_->occupied(startCell) && !obstacleDetected) {
			std::cout << "[NAV] current cell is occupied but ultrasonic is clear -> refreshing dynamic obstacle map"
			          << std::endl;
			this->turnLeft(5.0f);
			rebuildNavigationGrid();
			navigationTargetInitialized_ = false;
			edgeWasActive_ = false;
			std::cout << "[NAV] navigation state reset -> fresh D* plan on next cycle" << std::endl;
			return;
		}
		if (grid_->inBounds(startCell) && grid_->occupied(startCell) && obstacleDetected) {
			const float recoverySpeed = std::min(commandSpeed, 2.0f);
			hal_.setLeftSpeed(-obstacleRecoveryDirection_ * recoverySpeed);
			hal_.setRightSpeed(obstacleRecoveryDirection_ * recoverySpeed);
			if (logNav) {
				std::cout << "[NAV] start cell occupied while obstacle is detected -> turning to escape" << std::endl;
			}
			return;
		}
		this->stop();
		return;
	}

	if (path.empty()) {
		if (logNav) {
			std::cout << "[NAV] no path after obstacle update -> turning to retry D*" << std::endl;
		}
		const float recoveryTurnSpeed = std::min(commandSpeed, 2.0f);
		const float turnDirection = (std::sin(pose_.getTheta()) >= 0.0f) ? 1.0f : -1.0f;
		hal_.setLeftSpeed(-turnDirection * recoveryTurnSpeed);
		hal_.setRightSpeed(turnDirection * recoveryTurnSpeed);
		return;
	}

	Vec2 waypoint = target;
	for (size_t i = 1; i < path.size(); ++i) {
		const float pathDx = path[i].x - pose_.getX();
		const float pathDy = path[i].y - pose_.getY();
		if (std::hypot(pathDx, pathDy) >= 0.10f) {
			waypoint = path[i];
			break;
		}
	}

    const float dx = waypoint.x - pose_.getX();
    const float dy = waypoint.y - pose_.getY();
    const float desiredTheta = std::atan2(dy, dx);
    float angleError = std::atan2(std::sin(desiredTheta - pose_.getTheta()),
                                 std::cos(desiredTheta - pose_.getTheta()));

    if (std::fabs(angleError) > 0.12f) {
		const float turnSpeed = commandSpeed;
		const float turnDirection = angleError > 0.0f ? 1.0f : -1.0f;
		hal_.setLeftSpeed(-turnDirection * turnSpeed);
		hal_.setRightSpeed(turnDirection * turnSpeed);
        return;
    }

	if (distToGoal > 0.08f) {
		hal_.setLeftSpeed(commandSpeed);
		hal_.setRightSpeed(commandSpeed);
        return;
    }

	if (logNav) {
		std::cout << "[NAV] reached hardcoded goal" << std::endl;
	}
    this->stop();
}

void CoasterbotFunctions::rebuildNavigationGrid() {
	const float x0 = -tableWidth_ / 2.0f;
	const float x1 =  tableWidth_ / 2.0f;
	const float y0 = -tableHeight_ / 2.0f;
	const float y1 =  tableHeight_ / 2.0f;
	constexpr float resolution = 0.05f;
	constexpr float safetyMargin = 0.16f;

	grid_ = std::make_unique<OccupancyGrid>(x0, y0, x1, y1, resolution);
	grid_->addObstacleRect(x0, y0, x0, y1, safetyMargin);
	grid_->addObstacleRect(x1, y0, x1, y1, safetyMargin);
	grid_->addObstacleRect(x0, y0, x1, y0, safetyMargin);
	grid_->addObstacleRect(x0, y1, x1, y1, safetyMargin);
	grid_->buildDistanceField();
	dstar_ = std::make_unique<DStarLite>(*grid_);
}

void CoasterbotFunctions::runLearnTable() {
	// Keep the proven edge-recovery logic from the main learning routine:
	// detect the edge, back off, measure distance, reset pose, then continue.
	// Safety monitor may still override briefly; if so, stop and wait for it to clear.
	/*if (safety_.overriding()) {
		this->stop();
		return;
	}*/

	enum class State { RESET, FORWARD, BACKWARD, CENTER, TURN, SET_POSE,
						EDGE_DETECTED, DONE, IDLE, DEFAULT };
	static State state = State::RESET;
	static State lastState = State::RESET;
	static float speed = 5.0f;
	static float gap2Edge = 0.10f;
	enum class LearningPhase { PHASE_X, PHASE_Y };
	static LearningPhase phase = LearningPhase::PHASE_X;
	static float distance = 0.0f;
	static float lastOdo = 0.0f;
	static float tmpTime = -1.0f;

	switch (state) {
	case State::RESET:
		pose_.reset(0.0f, 0.0f, 0.0f);
		state = State::FORWARD;
		break;
	case State::FORWARD:
		this->forward(speed);
		if (this->edgeDetected()) {
			std::cout << "[LEARN] Edge detected while moving forward." << std::endl;
			state = State::EDGE_DETECTED;
			lastState = State::FORWARD;
			lastOdo = pose_.getOdometer();
		}
		break;
	case State::BACKWARD:
		this->backward(speed);
		if (this->edgeDetected()) {
			std::cout << "[LEARN] Edge detected while moving backward." << std::endl;
			state = State::EDGE_DETECTED;
			lastState = State::BACKWARD;
			lastOdo = pose_.getOdometer();
		}
		break;
	case State::CENTER:
		this->forward(speed);
		if (phase == LearningPhase::PHASE_X) {
			if (std::fabs(pose_.getX()) < 0.05f) {
				std::cout << "[LEARN] Reached center. Turn 90 degrees. Start learning Y." << std::endl;
				this->stop();
				phase = LearningPhase::PHASE_Y;
				state = State::TURN;
			}
		} else {
			if (std::fabs(pose_.getY()) < 0.05f) {
				std::cout << "[LEARN] Reached center." << std::endl;
				this->stop();
				state = State::DONE;
			}
		}
		break;
	case State::EDGE_DETECTED:
		switch (lastState) {
		case State::FORWARD:
			if ((pose_.getOdometer() - lastOdo) < gap2Edge) {
				this->backward(speed);
			} else {
				lastOdo = 0.0f;
				distance = pose_.getOdometer();
				state = State::BACKWARD;
			}
			break;
		case State::BACKWARD:
			if ((pose_.getOdometer() - lastOdo) < gap2Edge) {
				this->forward(speed);
			} else {
				lastOdo = 0.0f;
				distance = pose_.getOdometer() - distance;
				state = State::SET_POSE;
				std::cout << "[LEARN] distance: " << distance << " m" << std::endl;
			}
			break;
		default:
			std::cout << "[LEARN] Edge detected in unknown state." << std::endl;
			state = State::DONE;
			break;
		}
		break;
	case State::SET_POSE:
		if (phase == LearningPhase::PHASE_X) {
			tableWidth_ = distance;
			pose_.reset(-tableWidth_ / 2.0f, 0.0f, 0.0f);
			std::cout << "[LEARN] set x to: " << -tableWidth_ / 2.0f << " m" << std::endl;
			std::cout << "[LEARN] Move to center." << std::endl;
			state = State::CENTER;
		} else {
			tableHeight_ = distance;
			pose_.reset(pose_.getX(), -tableHeight_ / 2.0f, HALF_PI);
			std::cout << "[LEARN] set y to: " << -tableHeight_ / 2.0f << " m" << std::endl;
			state = State::CENTER;
		}
		break;
	case State::TURN:
		this->turnLeft(speed);
		if (pose_.getTheta() > 1.56f && pose_.getTheta() < 1.58f) {
			std::cout << "[LEARN] Turned 90 degrees. Move forward." << std::endl;
			state = State::FORWARD;
		}
		break;
	case State::DONE:
		this->stop();
		{
			const float x0 = -tableWidth_ / 2.0f;
			const float x1 =  tableWidth_ / 2.0f;
			const float y0 = -tableHeight_ / 2.0f;
			const float y1 =  tableHeight_ / 2.0f;
			const float resolution = 0.05f;
			const float safetyMargin = 0.16f;
			grid_ = std::make_unique<OccupancyGrid>(x0, y0, x1, y1, resolution);
			grid_->addObstacleRect(x0, y0, x0, y1, safetyMargin);
			grid_->addObstacleRect(x1, y0, x1, y1, safetyMargin);
			grid_->addObstacleRect(x0, y0, x1, y0, safetyMargin);
			grid_->addObstacleRect(x0, y1, x1, y1, safetyMargin);
			grid_->buildDistanceField();
			dstar_ = std::make_unique<DStarLite>(*grid_);
			std::cout << "[LEARN] table grid initialized: width=" << tableWidth_
				      << "m, height=" << tableHeight_ << "m" << std::endl;
		}
		std::cout << "[LEARN] Learning completed. Centering robot for navigation." << std::endl;
		pose_.reset(0.0f, 0.0f, 0.0f);
		state = State::IDLE;
		this->tableLearned = true;

		this->calcCoasterPositions();
		std::cout << "[LEARN] Coaster positions calculated. Positions: " << coasterPositions.size() << std::endl;
		for (size_t i = 0; i < coasterPositions.size(); ++i) {
			const Vec2& pos = coasterPositions[i];
			std::cout << "[LEARN] Coaster " << i << ": (" << pos.x << ", " << pos.y << ")" << std::endl;
		}
		break;
	case State::IDLE:
		if(!this->tableLearned) {
			std::cout << "[LEARN] Table not learned yet. Resetting state." << std::endl;
			state = State::RESET;
			phase = LearningPhase::PHASE_X;
		}
		break;
	case State::DEFAULT:
		this->stop();
		break;
	}
}

bool CoasterbotFunctions::runSpendCoasters() {
	if (!coasterRoutineActive_) {
		coasterRoutineActive_ = true;
		coasterRoutineFailed_ = false;
		coasterRoutineIndex_ = 0;
		const Vec2& firstPosition = coasterPositions[coasterRoutineIndex_];
		std::cout << "[SPEND] Navigating to coaster " << coasterRoutineIndex_
				  << ": (" << firstPosition.x << ", " << firstPosition.y << ")" << std::endl;
	}

	if (coasterRoutineIndex_ >= MAX_COASTERS) {
		coasterRoutineActive_ = false;
		return true;
	}

	const Vec2& pos = coasterPositions[coasterRoutineIndex_];
	this->navigateToWithObstacleAvoidance(pos, 5.0f);

	const float distanceToCoaster = std::hypot(pos.x - pose_.getX(), pos.y - pose_.getY());
	if (distanceToCoaster > 0.08f) {
		return false;
	}

	this->stop();
	this->hal_.wait(2.0f); // Wait for the servo to move

	std::cout << "[SPEND] At coaster " << coasterRoutineIndex_ << ", dispensing" << std::endl;
	if (!this->spendCoaster(coasterRoutineIndex_)) {
		std::cout << "[SPEND] Failed to spend coaster " << coasterRoutineIndex_ << std::endl;
		coasterRoutineActive_ = false;
		coasterRoutineFailed_ = true;
		return false;
	}

	++coasterRoutineIndex_;
	if (coasterRoutineIndex_ >= MAX_COASTERS) {
		coasterRoutineActive_ = false;
		std::cout << "[SPEND] All coasters dispensed" << std::endl;
		return true;
	}
	return false;
}

bool CoasterbotFunctions::runGetCoasters() {
	if (!coasterRoutineActive_) {
		coasterRoutineActive_ = true;
		coasterRoutineFailed_ = false;
		coasterRoutineIndex_ = MAX_COASTERS - 1;
		const Vec2& firstPosition = coasterPositions[coasterRoutineIndex_];
		std::cout << "[GET] Navigating to coaster " << coasterRoutineIndex_
				  << ": (" << firstPosition.x << ", " << firstPosition.y << ")" << std::endl;
	}

	if (coasterRoutineIndex_ < 0) {
		coasterRoutineActive_ = false;
		return false;
	}

	const Vec2& pos = coasterPositions[coasterRoutineIndex_];
	this->navigateToWithObstacleAvoidance(pos, 5.0f);

	const float distanceToCoaster = std::hypot(pos.x - pose_.getX(), pos.y - pose_.getY());
	if (distanceToCoaster > 0.08f) {
		return true;
	}

	this->stop();
	this->hal_.wait(2.0f); // Wait for the servo to move

	std::cout << "[GET] At coaster " << coasterRoutineIndex_ << ", picking up" << std::endl;
	if (!this->pickCoaster(coasterRoutineIndex_)) {
		std::cout << "[GET] Failed to get coaster " << coasterRoutineIndex_ << std::endl;
		coasterRoutineActive_ = false;
		coasterRoutineFailed_ = true;
		return true;
	}

	--coasterRoutineIndex_;
	if (coasterRoutineIndex_ < 0) {
		coasterRoutineActive_ = false;
		std::cout << "[GET] All coasters collected" << std::endl;
		return false;
	}
	return true;
}

bool CoasterbotFunctions::spendCoaster(int coasterIndex) {
	if (coasterIndex < 0 || coasterIndex >= MAX_COASTERS)
		return false;
	this->hal_.setServoPosition(static_cast<int>(CoasterbotFunctions::ServoId::LIFTER), SERVO_POS_LIFTER[coasterIndex]); // Lower the coaster
	this->hal_.wait(2.0f); // Wait for the servo to move
	this->hal_.setServoPosition(static_cast<int>(CoasterbotFunctions::ServoId::SPENDER), SERVO_POS_SPENDER[1]); // Move to the spending position
	this->hal_.wait(2.0f); // Wait for the servo to move
	this->hal_.setServoPosition(static_cast<int>(CoasterbotFunctions::ServoId::SPENDER), SERVO_POS_SPENDER[0]); // Raise the coaster
	return true;
}

bool CoasterbotFunctions::pickCoaster(int coasterIndex) {
	if (coasterIndex < 0 || coasterIndex >= MAX_COASTERS)
		return false;
	this->hal_.setServoPosition(static_cast<int>(CoasterbotFunctions::ServoId::SPENDER), SERVO_POS_SPENDER[2]); // Move to the spending position
	this->hal_.wait(2.0f); // Wait for the servo to move
	this->hal_.setServoPosition(static_cast<int>(CoasterbotFunctions::ServoId::SPENDER), SERVO_POS_SPENDER[0]); // Raise the coaster
	this->hal_.wait(2.0f); // Wait for the servo to move
	this->hal_.setServoPosition(static_cast<int>(CoasterbotFunctions::ServoId::LIFTER), SERVO_POS_LIFTER[coasterIndex]); // Lower the coaster
	return true;
}

std::vector<Vec2> CoasterbotFunctions::getCoasterPositions(int coasterIndex) {
	if (coasterIndex < 0 || coasterIndex >= MAX_COASTERS)
		return {};
	return { coasterPositions[coasterIndex] };
}
