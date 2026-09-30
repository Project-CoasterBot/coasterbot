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
	constexpr float edgeMargin = 0.25f;
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
	const float now = hal_.getTime();
	if (navigationPointSettling_) {
		this->stop();
		if (now - navigationPointSettleStartedAt_ < 0.5f) {
			return;
		}
		navigationPointSettling_ = false;
		if (navigationPointIndex_ == targets.size() - 1) {
			navigationSequenceComplete_ = true;
			std::cout << "[NAV] four-point navigation completed" << std::endl;
		} else {
			++navigationPointIndex_;
			std::cout << "[NAV] proceeding to point " << navigationPointIndex_ + 1 << "/4" << std::endl;
		}
		return;
	}

	navigateToWithObstacleAvoidance(target, 5.6f);

	const float distanceToTarget = std::hypot(target.x - pose_.getX(), target.y - pose_.getY());
	if (distanceToTarget <= ACCEPTED_BIAS) {
		this->stop();
		navigationPointSettling_ = true;
		navigationPointSettleStartedAt_ = now;
		std::cout << "[NAV] point " << navigationPointIndex_ + 1
				  << " reached, estimated error=" << distanceToTarget
				  << "m, settling" << std::endl;
	}
}

void CoasterbotFunctions::navigateTo(Vec2 target, float speed) {
	navigateToInternal(target, speed, false);
}

void CoasterbotFunctions::navigateToWithObstacleAvoidance(Vec2 target, float speed) {
	navigateToInternal(target, speed, true);
}

void CoasterbotFunctions::navigateToInternal(Vec2 target, float speed, bool useRobotBoundingBox) {
	if (!navigationTargetInitialized_ || target != navigationTarget_) {
		navigationTarget_ = target;
		navigationTargetInitialized_ = true;
		navigationPlannerInitialized_ = false;
		navigationAligning_ = false;
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
			navigationPlannerInitialized_ = false;
			edgeReroutePending_ = true;
			std::cout << "[NAV] edge recorded in map -> fresh route will be planned after recovery" << std::endl;
		}
	}

	if (safety_.overriding()) {
		return;
	}
	if (edgeReroutePending_) {
		edgeReroutePending_ = false;
		navigationPlannerInitialized_ = false;
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
	const float commandSpeed = std::min(driveSpeed,
		std::max(0.12f, distToGoal * NAVIGATION_DISTANCE_GAIN));
	const float now = hal_.getTime();
	static float lastNavLog = -5.0f;
	const bool logNav = (now - lastNavLog) >= 5.0f;
	if (logNav) {
		lastNavLog = now;
	}

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
			  << std::endl;
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
		const float us = hal_.getUltrasonicDistance();
		const bool obstacleDetected = std::isfinite(us) && us >= 0.0f && us < 0.75f;
		if (grid_->inBounds(startCell) && grid_->occupied(startCell) && !obstacleDetected) {
			std::cout << "[NAV] current cell is occupied but ultrasonic is clear -> refreshing dynamic obstacle map"
			          << std::endl;
			this->stop();
			rebuildNavigationGrid();
			navigationTargetInitialized_ = false;
			navigationPlannerInitialized_ = false;
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
	if (!grid_->inBounds(goalCell) || grid_->occupied(goalCell)) {
		if (logNav) {
			std::cout << "[NAV] goal cell is outside the grid or occupied; choose a point farther from the table border" << std::endl;
		}
		this->stop();
		return;
	}
	const float us = hal_.getUltrasonicDistance();
	bool mapChanged = false;
	std::vector<Cell> changedCells;
	const bool obstacleDetected = std::isfinite(us) && us >= 0.0f && us < 0.75f;
	if (obstacleDetected) {
		const float th = pose_.getTheta();
		const float hx = pose_.getX() + (us + 0.10f) * std::cos(th);
		const float hy = pose_.getY() + (us + 0.10f) * std::sin(th);
		const float sampleStep = grid_->resolution() * 0.5f;
		for (float y = hy - 0.12f; y <= hy + 0.12f; y += sampleStep) {
			for (float x = hx - 0.35f; x <= hx + 0.35f; x += sampleStep) {
				const Cell cell = grid_->worldToCell({x, y});
				if (grid_->reportOccupied(cell)) {
					changedCells.push_back(cell);
				}
			}
		}
		if (!changedCells.empty()) {
			grid_->buildDistanceField();
			mapChanged = true;
			std::cout << "[DSTAR] obstacle at (" << hx << ", " << hy << "), "
			          << changedCells.size() << " cells occupied, t=" << now << "s" << std::endl;
		}
	}

	std::vector<Vec2> path;
	if (!navigationPlannerInitialized_) {
		dstar_->plan(startCell, goalCell);
		navigationPreviousCell_ = startCell;
		navigationPlannerInitialized_ = true;
		path = dstar_->extractPath();
		if (logNav) {
			std::cout << "[DSTAR] initial plan -> " << path.size() << " waypoints" << std::endl;
		}
	} else {
		for (const Cell& cell : changedCells) {
			dstar_->cellChanged(cell);
		}
		if (startCell.cx != navigationPreviousCell_.cx || startCell.cy != navigationPreviousCell_.cy) {
			dstar_->setStart(startCell);
			dstar_->computeShortestPath();
			navigationPreviousCell_ = startCell;
		}
		if (mapChanged) {
			dstar_->setStart(startCell);
			dstar_->computeShortestPath();
			path = dstar_->extractPath();
			if (path.empty()) {
				dstar_->plan(startCell, goalCell);
				path = dstar_->extractPath();
				std::cout << "[DSTAR] planned corridor blocked -> reinitialized" << std::endl;
			}
			std::cout << "[DSTAR] incremental repair -> " << path.size()
			          << " waypoints, t=" << now << "s" << std::endl;
		}
	}
	if (path.empty()) {
		path = dstar_->extractPath();
	}
	if (path.empty()) {
		if (logNav) {
			std::cout << "[NAV] no path -> turning to retry D*" << std::endl;
		}
		const float recoveryTurnSpeed = std::min(commandSpeed, 2.0f);
		const float turnDirection = (std::sin(pose_.getTheta()) >= 0.0f) ? 1.0f : -1.0f;
		hal_.setLeftSpeed(-turnDirection * recoveryTurnSpeed);
		hal_.setRightSpeed(turnDirection * recoveryTurnSpeed);
		return;
	}
	if (distToGoal <= ACCEPTED_BIAS) {
		this->stop();
		return;
	}

	Vec2 waypoint = target;
	const Vec2 currentPosition{pose_.getX(), pose_.getY()};
	const auto lineIsClear = [this](Vec2 from, Vec2 to) {
		const float dx = to.x - from.x;
		const float dy = to.y - from.y;
		const float distance = std::hypot(dx, dy);
		const float sampleSpacing = grid_->resolution() * 0.5f;
		const int samples = std::max(1, static_cast<int>(std::ceil(distance / sampleSpacing)));
		for (int sample = 0; sample <= samples; ++sample) {
			const float fraction = static_cast<float>(sample) / static_cast<float>(samples);
			const Cell cell = grid_->worldToCell({from.x + fraction * dx, from.y + fraction * dy});
			if (!grid_->inBounds(cell) || grid_->occupied(cell)) {
				return false;
			}
		}
		return true;
	};
	for (size_t i = path.size(); i > 1; --i) {
		if (lineIsClear(currentPosition, path[i - 1])) {
			waypoint = path[i - 1];
			break;
		}
	}
	if (lineIsClear(currentPosition, target)) {
		waypoint = target;
	}

    const float dx = waypoint.x - pose_.getX();
    const float dy = waypoint.y - pose_.getY();
    const float desiredTheta = std::atan2(dy, dx);
    const float angleError = std::atan2(std::sin(desiredTheta - pose_.getTheta()),
                                       std::cos(desiredTheta - pose_.getTheta()));
	const float absoluteAngleError = std::fabs(angleError);
	if (navigationAligning_) {
		if (absoluteAngleError <= NAVIGATION_TURN_EXIT_ERROR) {
			navigationAligning_ = false;
		}
	} else if (absoluteAngleError >= NAVIGATION_TURN_ENTER_ERROR) {
		navigationAligning_ = true;
	}

	if (navigationAligning_) {
		const float turnSpeed = std::min(commandSpeed,
			std::max(0.12f, absoluteAngleError * NAVIGATION_HEADING_GAIN));
		this->turnLeft(angleError > 0.0f ? turnSpeed : -turnSpeed);
	} else {
		this->forward(commandSpeed);
	}
}

void CoasterbotFunctions::rebuildNavigationGrid() {
	std::vector<Vec2> occupiedCellCenters;
	if (grid_) {
		for (int cy = 0; cy < grid_->rows(); ++cy) {
			for (int cx = 0; cx < grid_->cols(); ++cx) {
				const Cell cell{cx, cy};
				if (grid_->occupied(cell)) {
					occupiedCellCenters.push_back(grid_->cellToWorld(cell));
				}
			}
		}
	}

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
	for (const Vec2& center : occupiedCellCenters) {
		const Cell cell = grid_->worldToCell(center);
		if (grid_->inBounds(cell)) {
			grid_->setOccupied(cell, true);
		}
	}
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
	static float previousCenterCoordinate = 0.0f;
	static float turnStartYaw = 0.0f;
	static bool buildGrid;

	switch (state) {
	case State::RESET:
		if(!this->blocklessWait(2.0f)) break;
		pose_.reset(0.0f, 0.0f, 0.0f);
		
		buildGrid = false;

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
			const float currentCoordinate = pose_.getX();
			const bool crossedCenter = previousCenterCoordinate < 0.0f && currentCoordinate >= 0.0f;
			previousCenterCoordinate = currentCoordinate;
			if (std::fabs(currentCoordinate) < 0.05f || crossedCenter) {
				std::cout << "[LEARN] Reached center. Turn 90 degrees. Start learning Y." << std::endl;
				this->stop();
				phase = LearningPhase::PHASE_Y;
				turnStartYaw = hal_.getYaw();
				state = State::TURN;
			}
		} else {
			const float currentCoordinate = pose_.getY();
			const bool crossedCenter = previousCenterCoordinate < 0.0f && currentCoordinate >= 0.0f;
			previousCenterCoordinate = currentCoordinate;
			if (std::fabs(currentCoordinate) < 0.05f || crossedCenter) {
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
			tableWidth_ = distance - (robotBoundingBoxWidth_/2.0f);
			this->stop();
			if(!this->blocklessWait(2.0f)) break;
			pose_.reset(-tableWidth_ / 2.0f, 0.0f, 0.0f);
			previousCenterCoordinate = pose_.getX();
			std::cout << "[LEARN] set x to: " << -tableWidth_ / 2.0f << " m" << std::endl;
			std::cout << "[LEARN] Move to center." << std::endl;
			state = State::CENTER;
		} else {
			tableHeight_ = distance - (robotBoundingBoxLength_/2.0f);
			this->stop();
			if(!this->blocklessWait(2.0f)) break;
			pose_.reset(pose_.getX(), -tableHeight_ / 2.0f, HALF_PI);
			previousCenterCoordinate = pose_.getY();
			std::cout << "[LEARN] set y to: " << -tableHeight_ / 2.0f << " m" << std::endl;
			state = State::CENTER;
		}
		break;
	case State::TURN:
		{
			const float yawChange = std::atan2(std::sin(hal_.getYaw() - turnStartYaw),
			                                   std::cos(hal_.getYaw() - turnStartYaw));
			const float turnError = std::atan2(std::sin(HALF_PI - yawChange),
			                                   std::cos(HALF_PI - yawChange));
			if (std::fabs(turnError) <= HALF_PI * 0.01f) {
			this->stop();
			pose_.reset(pose_.getX(), pose_.getY(), HALF_PI);
			std::cout << "[LEARN] Turn accepted: yaw change="
			          << yawChange * 180.0f / (HALF_PI * 2.0f)
			          << " deg (target=90). Move forward." << std::endl;
			state = State::FORWARD;
			} else {
				const float turnSpeed = std::fabs(turnError) <= HALF_PI / 18.0f
					? speed * 0.1f
					: speed * 0.5f;
				this->turnLeft(turnError > 0.0f ? turnSpeed : -turnSpeed);
			}
		}
		break;
	case State::DONE:
		this->stop();
		if(!buildGrid){
			const float x0 = -tableWidth_ / 2.0f;
			const float x1 =  tableWidth_ / 2.0f;
			const float y0 = -tableHeight_ / 2.0f;
			const float y1 =  tableHeight_ / 2.0f;
			const float resolution = 0.05f;
			const float safetyMargin = 0.01f;
			grid_ = std::make_unique<OccupancyGrid>(x0, y0, x1, y1, resolution);
			grid_->addObstacleRect(x0, y0, x0, y1, safetyMargin);
			grid_->addObstacleRect(x1, y0, x1, y1, safetyMargin);
			grid_->addObstacleRect(x0, y0, x1, y0, safetyMargin);
			grid_->addObstacleRect(x0, y1, x1, y1, safetyMargin);
			grid_->buildDistanceField();
			dstar_ = std::make_unique<DStarLite>(*grid_);
			std::cout << "[LEARN] table grid initialized: width=" << tableWidth_
				      << "m, height=" << tableHeight_ << "m" << std::endl;
			std::cout << "[LEARN] Learning completed. Centering robot for navigation." << std::endl;
			buildGrid = true;
		}
		if(!this->blocklessWait(2.0f)) break;
		pose_.reset(0.0f, 0.0f, HALF_PI);
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
		coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
		const Vec2& firstPosition = coasterPositions[coasterRoutineIndex_];
		std::cout << "[SPEND] Navigating to coaster " << coasterRoutineIndex_
				  << ": (" << firstPosition.x << ", " << firstPosition.y << ")" << std::endl;
	}

	if (coasterRoutineIndex_ >= MAX_COASTERS) {
		coasterRoutineActive_ = false;
		return true;
	}

	const float now = hal_.getTime();
	if (coasterRoutinePhase_ == CoasterRoutinePhase::TRAVEL) {
		const Vec2& pos = coasterPositions[coasterRoutineIndex_];
		this->navigateToWithObstacleAvoidance(pos, 5.0f);
		const float distanceToCoaster = std::hypot(pos.x - pose_.getX(), pos.y - pose_.getY());
		if (distanceToCoaster > ACCEPTED_BIAS) {
			return false;
		}
		this->stop();
		coasterRoutinePhase_ = CoasterRoutinePhase::SETTLE;
		coasterRoutinePhaseStartedAt_ = now;
		return false;
	}

	if (coasterRoutinePhase_ == CoasterRoutinePhase::SETTLE) {
		if (now - coasterRoutinePhaseStartedAt_ < 2.0f) {
			return false;
		}
		std::cout << "[SPEND] At coaster " << coasterRoutineIndex_ << ", dispensing" << std::endl;
		hal_.setServoPosition(static_cast<int>(ServoId::LIFTER), SERVO_POS_LIFTER[coasterRoutineIndex_]);
		coasterRoutinePhase_ = CoasterRoutinePhase::SPEND_WAIT_LIFTER;
		coasterRoutinePhaseStartedAt_ = now;
		return false;
	}

	if (coasterRoutinePhase_ == CoasterRoutinePhase::SPEND_WAIT_LIFTER) {
		if (now - coasterRoutinePhaseStartedAt_ < 2.0f) {
			return false;
		}
		hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[1]);
		coasterRoutinePhase_ = CoasterRoutinePhase::SPEND_WAIT_SPENDER;
		coasterRoutinePhaseStartedAt_ = now;
		return false;
	}

	if (now - coasterRoutinePhaseStartedAt_ < 2.0f) {
		return false;
	}
	hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[0]);
	++coasterRoutineIndex_;
	coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
	if (coasterRoutineIndex_ >= MAX_COASTERS) {
		coasterRoutineActive_ = false;
		std::cout << "[SPEND] All coasters dispensed" << std::endl;
		return true;
	}
	return false;
}

bool CoasterbotFunctions::goToCenter(){
	const Vec2 returnTarget{0.0f, 0.0f};
	const float now = hal_.getTime();
	if (centerSettling_) {
		this->stop();
		if (now - centerSettleStartedAt_ < CENTER_SETTLE_TIME) {
			return false;
		}
		const float settledDistance = std::hypot(pose_.getX(), pose_.getY());
		if (settledDistance > CENTER_CAPTURE_RADIUS) {
			centerSettling_ = false;
			return false;
		}
		pose_.reset(0.0f, 0.0f, pose_.getTheta());
		navigationTargetInitialized_ = false;
		navigationPlannerInitialized_ = false;
		navigationAligning_ = false;
		centerSettling_ = false;
		std::cout << "[NAV] center re-anchored after settling, residual="
		          << settledDistance << "m" << std::endl;
		return true;
	}

	this->navigateToWithObstacleAvoidance(returnTarget, 5.0f);
	const float distanceToCenter = std::hypot(pose_.getX(), pose_.getY());
	if (distanceToCenter > CENTER_CAPTURE_RADIUS) {
		return false;
	}
	this->stop();
	centerSettling_ = true;
	centerSettleStartedAt_ = now;
	return false;
}

bool CoasterbotFunctions::runGetCoasters() {
	if (!coasterRoutineActive_) {
		coasterRoutineActive_ = true;
		coasterRoutineFailed_ = false;
		coasterRoutineIndex_ = MAX_COASTERS - 1;
		coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
		const Vec2& firstPosition = coasterPositions[coasterRoutineIndex_];
		std::cout << "[GET] Navigating to coaster " << coasterRoutineIndex_
				  << ": (" << firstPosition.x << ", " << firstPosition.y << ")" << std::endl;
	}

	if (coasterRoutineIndex_ < 0) {
		coasterRoutineActive_ = false;
		return false;
	}

	const float now = hal_.getTime();
	if (coasterRoutinePhase_ == CoasterRoutinePhase::TRAVEL) {
		const Vec2& pos = coasterPositions[coasterRoutineIndex_];
		this->navigateToWithObstacleAvoidance(pos, 5.0f);
		const float distanceToCoaster = std::hypot(pos.x - pose_.getX(), pos.y - pose_.getY());
		if (distanceToCoaster > ACCEPTED_BIAS) {
			return true;
		}
		this->stop();
		coasterRoutinePhase_ = CoasterRoutinePhase::SETTLE;
		coasterRoutinePhaseStartedAt_ = now;
		return true;
	}

	if (coasterRoutinePhase_ == CoasterRoutinePhase::SETTLE) {
		if (now - coasterRoutinePhaseStartedAt_ < 2.0f) {
			return true;
		}
		std::cout << "[GET] At coaster " << coasterRoutineIndex_ << ", picking up" << std::endl;
		hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[2]);
		coasterRoutinePhase_ = CoasterRoutinePhase::GET_WAIT_SPENDER_OUT;
		coasterRoutinePhaseStartedAt_ = now;
		return true;
	}

	if (coasterRoutinePhase_ == CoasterRoutinePhase::GET_WAIT_SPENDER_OUT) {
		if (now - coasterRoutinePhaseStartedAt_ < 2.0f) {
			return true;
		}
		hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[0]);
		coasterRoutinePhase_ = CoasterRoutinePhase::GET_WAIT_SPENDER_HOME;
		coasterRoutinePhaseStartedAt_ = now;
		return true;
	}

	if (now - coasterRoutinePhaseStartedAt_ < 2.0f) {
		return true;
	}
	hal_.setServoPosition(static_cast<int>(ServoId::LIFTER), SERVO_POS_LIFTER[coasterRoutineIndex_]);
	--coasterRoutineIndex_;
	coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
	if (coasterRoutineIndex_ < 0) {
		coasterRoutineActive_ = false;
		std::cout << "[GET] All coasters collected" << std::endl;
		return false;
	}
	return true;
}

std::vector<Vec2> CoasterbotFunctions::getCoasterPositions(int coasterIndex) {
	if (coasterIndex < 0 || coasterIndex >= MAX_COASTERS)
		return {};
	return { coasterPositions[coasterIndex] };
}

bool CoasterbotFunctions::blocklessWait(float seconds) {
	if(this->resetLastTimer_) {
		this->lastTimer_ = hal_.getTime();
		this->resetLastTimer_ = false;
	}
	if (hal_.getTime() - this->lastTimer_ < seconds) {
		return false;
		
	}
	this->resetLastTimer_ = true;
	return true;
}
