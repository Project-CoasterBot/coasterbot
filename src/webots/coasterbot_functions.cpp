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
      dstar_(std::make_unique<DStarLite>(*grid_)),
      navigationStall_(hal, NAVIGATION_STALL_MIN_TIME, NAVIGATION_STALL_MIN_EXPECTED,
                       NAVIGATION_STALL_MIN_MOVED) {}

void CoasterbotFunctions::update() {
	// These components have state that must be refreshed once per control cycle.
	this->pose_.update();
	//this->RobotLogic::update();
	this->safety_.update();
}

void CoasterbotFunctions::calcCoasterPositions() {
	const float x = std::max(0.0f, tableWidth_ / 2.0f - COASTER_EDGE_MARGIN);
	const float y = std::max(0.0f, tableHeight_ / 2.0f - COASTER_EDGE_MARGIN);
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
		stallInterruptions_ = 0;
		stallStopLatched_ = false;
		navigationStall_.restart();
		alignWatchActive_ = false;
		alignStopLatched_ = false;
	}

	// Vorgabe fuer alle Ausstiege mit stop(); die anderen setzen ihren Zustand selbst.
	navState_ = NavState::BLOCKED;

	if (alignStopLatched_) {
		navState_ = alignStopState_;
		this->stop();
		return;
	}
	if (edgeStopLatched_ || stallStopLatched_) {
		navState_ = edgeStopLatched_ ? NavState::EDGE_LATCHED : NavState::STALL_LATCHED;
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
			navState_ = NavState::EDGE_LATCHED;
			this->stop();
			std::cout << "[NAV] three edge interruptions reached -> navigation stopped" << std::endl;
			return;
		}

		if (grid_ && tableLearned) {
			correctPoseAtEdge(safety_.frontEdge());
			markBlockedArea(safety_.frontEdge() ? 1.0f : -1.0f);
			navigationPlannerInitialized_ = false;
			edgeReroutePending_ = true;
			std::cout << "[NAV] edge recorded in map -> fresh route will be planned after recovery" << std::endl;
		}
	}

	if (safety_.overriding()) {
		navState_ = NavState::SAFETY;
		return;
	}
	if (edgeReroutePending_) {
		edgeReroutePending_ = false;
		navigationPlannerInitialized_ = false;
		std::cout << "[NAV] edge recovery complete -> replanning from current position" << std::endl;
	}
	if (edgeActive) {
		navState_ = NavState::SAFETY;
		this->stop();
		return;
	}

    if (!tableLearned || !grid_ || !dstar_) {
        this->stop();
        return;
    }

	// Fahrbefehl des letzten Zyklus ohne Radbewegung: die reale HAL unterdrueckt die Fahrt
	// (z.B. Hindernis naeher als ihr Mindestabstand) oder der Roboter haengt fest. Ohne
	// Reaktion wuerde derselbe Befehl endlos wiederholt -> blockierten Bereich eintragen, neu planen.
	navigationStall_.update();
	if (navigationStall_.stalled()) {
		const float cmdLeft = hal_.getCommandedSpeed(WHEEL_LEFT);
		const float cmdRight = hal_.getCommandedSpeed(WHEEL_RIGHT);
		const bool drivingForward = cmdLeft > 0.0f && cmdRight > 0.0f;
		const bool drivingBackward = cmdLeft < 0.0f && cmdRight < 0.0f;
		++stallInterruptions_;
		std::cout << "[NAV] no wheel motion despite drive command (left=" << cmdLeft
		          << ", right=" << cmdRight << " rad/s) -> stall " << stallInterruptions_
		          << "/" << MAX_STALL_INTERRUPTIONS << std::endl;
		this->stop();
		navigationStall_.restart();
		if (stallInterruptions_ >= MAX_STALL_INTERRUPTIONS) {
			stallStopLatched_ = true;
			navState_ = NavState::STALL_LATCHED;
			std::cout << "[NAV] three stalls reached -> navigation stopped" << std::endl;
			return;
		}
		// beim Drehen auf der Stelle ist keine Richtung zuordenbar -> nur neu planen
		if (drivingForward || drivingBackward) {
			markBlockedArea(drivingForward ? 1.0f : -1.0f);
			std::cout << "[NAV] blocked area recorded " << (drivingForward ? "ahead" : "behind")
			          << " -> replanning" << std::endl;
		}
		navigationPlannerInitialized_ = false;
		navigationAligning_ = false;
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
		std::max(NAVIGATION_MIN_DRIVE_SPEED, distToGoal * NAVIGATION_DISTANCE_GAIN));
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
		const bool obstacleDetected = std::isfinite(us) && us >= 0.0f && us < ULTRASONIC_MAP_RANGE;
		if (grid_->inBounds(startCell) && grid_->occupied(startCell) && !obstacleDetected) {
			std::cout << "[NAV] current cell is occupied but ultrasonic is clear -> refreshing dynamic obstacle map"
			          << std::endl;
			this->stop();
			grid_->setOccupied(startCell, false);
			rebuildNavigationGrid();
			navigationTargetInitialized_ = false;
			navigationPlannerInitialized_ = false;
			edgeWasActive_ = false;
			std::cout << "[NAV] navigation state reset -> fresh D* plan on next cycle" << std::endl;
			return;
		}
		if (grid_->inBounds(startCell) && grid_->occupied(startCell) && obstacleDetected) {
			navState_ = NavState::ESCAPE_TURN;
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
	if (!grid_->inBounds(goalCell)) {
		if (logNav) {
			std::cout << "[NAV] goal cell is outside the grid: cell=("
			          << goalCell.cx << ", " << goalCell.cy << ")" << std::endl;
		}
		this->stop();
		return;
	}
	if (grid_->occupied(goalCell)) {
		if (logNav) {
			std::cout << "[NAV] goal cell is occupied: cell=(" << goalCell.cx << ", "
			          << goalCell.cy << "), clearance=" << grid_->clearance(target) << "m" << std::endl;
		}
		this->stop();
		return;
	}
	const float us = hal_.getUltrasonicDistance();
	bool mapChanged = false;
	std::vector<Cell> changedCells;
	const bool obstacleDetected = std::isfinite(us) && us >= 0.0f && us < ULTRASONIC_MAP_RANGE;
	// Kartieren nur bei Geradeausfahrt: beim Drehen auf der Stelle verschmiert der Sensor ueber den
	// ganzen Umkreis. Jeder Treffer in Drehrichtung sperrte den Weg, D* wich zur Seite aus, der
	// Roboter drehte hinterher - endlos. Treffer ausserhalb des Tisches (Wand, Personen) ignorieren.
	const bool drivingStraight = hal_.getCommandedSpeed(WHEEL_LEFT) > 0.0f &&
		hal_.getCommandedSpeed(WHEEL_RIGHT) > 0.0f;
	const float hitX = pose_.getX() + (us + 0.10f) * std::cos(pose_.getTheta());
	const float hitY = pose_.getY() + (us + 0.10f) * std::sin(pose_.getTheta());
	if (obstacleDetected && drivingStraight && grid_->inBounds(grid_->worldToCell({hitX, hitY}))) {
		const float hx = hitX;
		const float hy = hitY;
		const float obstacleHalfLength = useRobotBoundingBox
			? std::fabs(robotBoundingBoxLength_) * 0.5f + ULTRASONIC_OBSTACLE_PADDING
			: grid_->resolution();
		const float obstacleHalfWidth = useRobotBoundingBox
			? std::fabs(robotBoundingBoxWidth_) * 0.5f + ULTRASONIC_OBSTACLE_PADDING
			: grid_->resolution();
		const Cell minCell = grid_->worldToCell(
			{hx - obstacleHalfLength, hy - obstacleHalfWidth});
		const Cell maxCell = grid_->worldToCell(
			{hx + obstacleHalfLength, hy + obstacleHalfWidth});
		for (int cy = minCell.cy; cy <= maxCell.cy; ++cy) {
			for (int cx = minCell.cx; cx <= maxCell.cx; ++cx) {
				const Cell cell{cx, cy};
				if (!grid_->inBounds(cell)) {
					continue;
				}
				const Vec2 cellCenter = grid_->cellToWorld(cell);
				if (std::fabs(cellCenter.x - hx) > obstacleHalfLength + grid_->resolution() * 0.5f ||
				    std::fabs(cellCenter.y - hy) > obstacleHalfWidth + grid_->resolution() * 0.5f) {
					continue;
				}
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

	const Vec2 currentPosition{pose_.getX(), pose_.getY()};
	const float minimumPathClearance = useRobotBoundingBox ? NAVIGATION_PATH_CLEARANCE : 0.0f;
	const auto lineIsClear = [this, minimumPathClearance](Vec2 from, Vec2 to) {
		const float dx = to.x - from.x;
		const float dy = to.y - from.y;
		const float distance = std::hypot(dx, dy);
		const float sampleSpacing = grid_->resolution() * 0.5f;
		const int samples = std::max(1, static_cast<int>(std::ceil(distance / sampleSpacing)));
		for (int sample = 0; sample <= samples; ++sample) {
			const float fraction = static_cast<float>(sample) / static_cast<float>(samples);
			const Vec2 samplePosition{from.x + fraction * dx, from.y + fraction * dy};
			const Cell cell = grid_->worldToCell(samplePosition);
			if (!grid_->inBounds(cell) || grid_->occupied(cell) ||
			    grid_->clearance(samplePosition) < minimumPathClearance) {
				return false;
			}
		}
		return true;
	};

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
	if (distToGoal <= ACCEPTED_BIAS) {
		navState_ = NavState::REACHED;
		this->stop();
		return;
	}

	// Endanflug: Ziel nahe und auf der Fahrtlinie -> nur vor-/rueckwaerts, nicht mehr drehen.
	// Ein Ueberfahren (Nachlauf beim Bremsen) wird rueckwaerts korrigiert statt mit einer Wende.
	{
		const float th = pose_.getTheta();
		const float dx = target.x - pose_.getX();
		const float dy = target.y - pose_.getY();
		const float along = dx * std::cos(th) + dy * std::sin(th);
		const float lateral = -dx * std::sin(th) + dy * std::cos(th);
		if (distToGoal <= NAVIGATION_FINAL_APPROACH_RADIUS &&
		    std::fabs(lateral) <= NAVIGATION_FINAL_APPROACH_LATERAL) {
			navigationAligning_ = false;
			navState_ = NavState::FINAL_APPROACH;
			if (along >= 0.0f) {
				this->forward(commandSpeed);
			} else {
				this->backward(commandSpeed);
			}
			return;
		}
	}

	if (path.empty()) {
		navState_ = NavState::NO_PATH;
		if (logNav) {
			std::cout << "[NAV] no path -> turning to retry D*" << std::endl;
		}
		const float recoveryTurnSpeed = std::min(commandSpeed, 2.0f);
		if (obstacleDetected && us > ULTRASONIC_TURN_DISTANCE) {
			const float approachSpeed = std::min(driveSpeed, 1.0f);
			this->forward(approachSpeed);
			if (logNav) {
				std::cout << "[NAV] no route yet; mapped obstacle at " << us
				          << "m, approaching slowly until turn threshold "
				          << ULTRASONIC_TURN_DISTANCE << "m" << std::endl;
			}
			return;
		}
		const float targetHeading = std::atan2(target.y - pose_.getY(), target.x - pose_.getX());
		const float headingError = std::atan2(std::sin(targetHeading - pose_.getTheta()),
		                                       std::cos(targetHeading - pose_.getTheta()));
		if (lineIsClear(currentPosition, target)) {
			if (std::fabs(headingError) <= NAVIGATION_TURN_ENTER_ERROR) {
				this->forward(recoveryTurnSpeed);
			} else {
				const float turnDirection = headingError > 0.0f ? 1.0f : -1.0f;
				this->turnLeft(turnDirection * recoveryTurnSpeed);
			}
			return;
		}
		this->turnLeft(recoveryTurnSpeed);
		if (logNav) {
			std::cout << "[NAV] direct corridor blocked; scanning left at "
			          << recoveryTurnSpeed << " rad/s" << std::endl;
		}
		return;
	}
	Vec2 waypoint = path.size() > 1 ? path[1] : target;
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
		// Nachlauf beim Bremsen vorwegnehmen, sonst landet der Roboter jenseits der Eintrittsschwelle
		// und dreht zurueck (Pendeln).
		const float brakeLead = std::min(std::fabs(hal_.getGyroZ()) * NAVIGATION_TURN_BRAKE_LEAD,
		                                 NAVIGATION_TURN_MAX_BRAKE_LEAD);
		if (absoluteAngleError <= NAVIGATION_TURN_EXIT_ERROR + brakeLead) {
			navigationAligning_ = false;
		}
	} else if (absoluteAngleError >= NAVIGATION_TURN_ENTER_ERROR) {
		navigationAligning_ = true;
	}

	// Vor einer groesseren Drehung nahe der Tischkante erst gerade wegfahren (siehe TURN_EDGE_CLEARANCE).
	// Nur bevor die Drehung beginnt: waehrend des Drehens aendert sich der Abstand nicht.
	if (navigationAligning_ && !alignWatchActive_ && absoluteAngleError > TURN_CLEARANCE_MIN_ANGLE) {
		const float hx = std::cos(pose_.getTheta());
		const float hy = std::sin(pose_.getTheta());
		const struct { float distance, facing; } edges[] = {
			{tableWidth_ * 0.5f - pose_.getX(),   hx},   // +x
			{tableWidth_ * 0.5f + pose_.getX(),  -hx},   // -x
			{tableHeight_ * 0.5f - pose_.getY(),  hy},   // +y
			{tableHeight_ * 0.5f + pose_.getY(), -hy},   // -y
		};
		bool needBackward = false;
		bool needForward = false;
		for (const auto& edge : edges) {
			if (edge.distance >= TURN_EDGE_CLEARANCE) continue;
			if (edge.facing >= TURN_CLEARANCE_MIN_FACING) needBackward = true;
			if (edge.facing <= -TURN_CLEARANCE_MIN_FACING) needForward = true;
		}
		// in einer Ecke mit Front und Heck je zu einer Kante hilft gerades Fahren nicht -> trotzdem drehen
		if (needBackward != needForward) {
			if (!turnClearanceActive_) {
				std::cout << "[NAV] edge closer than " << TURN_EDGE_CLEARANCE << "m -> "
				          << (needBackward ? "backing off" : "moving forward") << " before turning" << std::endl;
			}
			turnClearanceActive_ = true;
			navState_ = NavState::TURN_CLEARANCE;
			// Anfahren aus dem Stand mit voller Fahrgeschwindigkeit: mit der Mindestgeschwindigkeit (PWM ~100)
			// liefen die Motoren bei schwacher Batterie nicht mehr an -> Stall-Stopp. Der Weg ist nur wenige cm.
			if (needBackward) {
				this->backward(driveSpeed);
			} else {
				this->forward(driveSpeed);
			}
			return;
		}
	}
	turnClearanceActive_ = false;

	if (navigationAligning_) {
		const float theta = pose_.getTheta();
		const float wheelLeft = hal_.getWheelAngle(WHEEL_LEFT);
		const float wheelRight = hal_.getWheelAngle(WHEEL_RIGHT);
		if (!alignWatchActive_) {
			alignWatchActive_ = true;
			alignWatchStartedAt_ = now;
			alignWatchPrevTheta_ = theta;
			alignWatchTurned_ = 0.0f;
			alignWatchWheelTurned_ = 0.0f;
		} else {
			alignWatchTurned_ += std::fabs(std::atan2(std::sin(theta - alignWatchPrevTheta_),
			                                          std::cos(theta - alignWatchPrevTheta_)));
			alignWatchPrevTheta_ = theta;
			alignWatchWheelTurned_ += (std::fabs(wheelLeft - alignWatchPrevLeft_) +
			                           std::fabs(wheelRight - alignWatchPrevRight_)) *
			                          pose_.getWheelRadius() / pose_.getTrackWidth();
		}
		alignWatchPrevLeft_ = wheelLeft;
		alignWatchPrevRight_ = wheelRight;
		// Kurs laeuft dem Ziel hinterher, obwohl er sich aendert -> Regelung/Sollkurs
		const bool overrun = alignWatchTurned_ > ALIGN_MAX_TURN;
		// Raeder drehen weit mehr als der Kurs -> die Kursschaetzung folgt der Drehung nicht (Gyro)
		const bool headingLost = alignWatchWheelTurned_ > ALIGN_MAX_WHEEL_TURN;
		const bool stuck = now - alignWatchStartedAt_ > ALIGN_STUCK_TIME &&
			alignWatchTurned_ < ALIGN_STUCK_MIN_TURN;
		if (overrun || headingLost || stuck) {
			alignStopLatched_ = true;
			alignStopState_ = overrun ? NavState::TURN_OVERRUN : NavState::HEADING_STUCK;
			navState_ = alignStopState_;
			this->stop();
			std::cout << "[NAV] align watchdog: turned=" << alignWatchTurned_ << " rad (wheels "
			          << alignWatchWheelTurned_ << " rad) in " << now - alignWatchStartedAt_
			          << " s, error=" << angleError << " rad -> navigation stopped" << std::endl;
			return;
		}
	} else {
		alignWatchActive_ = false;
	}

	navState_ = navigationAligning_ ? NavState::ALIGNING : NavState::DRIVING;
	if (navigationAligning_) {
		const float turnSpeed = std::max(NAVIGATION_MIN_TURN_SPEED,
			std::min({commandSpeed, NAVIGATION_MAX_TURN_SPEED, absoluteAngleError * NAVIGATION_HEADING_GAIN}));
		this->turnLeft(angleError > 0.0f ? turnSpeed : -turnSpeed);
	} else {
		this->forward(commandSpeed);
	}
}

const char* CoasterbotFunctions::navStateName(NavState state) {
	switch (state) {
	case NavState::IDLE:           return "IDLE";
	case NavState::DRIVING:        return "DRIVING";
	case NavState::ALIGNING:       return "ALIGNING";
	case NavState::TURN_CLEARANCE: return "TURN_CLEARANCE";
	case NavState::FINAL_APPROACH: return "FINAL_APPROACH";
	case NavState::REACHED:        return "REACHED";
	case NavState::NO_PATH:        return "NO_PATH";
	case NavState::ESCAPE_TURN:    return "ESCAPE_TURN";
	case NavState::BLOCKED:        return "BLOCKED";
	case NavState::SAFETY:         return "SAFETY";
	case NavState::EDGE_LATCHED:   return "EDGE_LATCHED";
	case NavState::STALL_LATCHED:  return "STALL_LATCHED";
	case NavState::TURN_OVERRUN:   return "TURN_OVERRUN";
	case NavState::HEADING_STUCK:  return "HEADING_STUCK";
	case NavState::SPEND_SIGNAL:   return "SPEND_SIGNAL";
	case NavState::COLLECT_SIGNAL: return "COLLECT_SIGNAL";
	}
	return "?";
}

RobotHAL::LED_COLORS CoasterbotFunctions::navLedColor() const {
	switch (navState_) {
	case NavState::DRIVING:        return RobotHAL::LED_COLORS::GREEN;
	case NavState::ALIGNING:       return RobotHAL::LED_COLORS::CYAN;
	case NavState::TURN_CLEARANCE: return RobotHAL::LED_COLORS::BLUE;
	case NavState::FINAL_APPROACH: return RobotHAL::LED_COLORS::YELLOW;
	case NavState::REACHED:        return RobotHAL::LED_COLORS::WHITE;
	case NavState::NO_PATH:        return RobotHAL::LED_COLORS::PURPLE;
	case NavState::ESCAPE_TURN:    return RobotHAL::LED_COLORS::PINK;
	case NavState::BLOCKED:        return RobotHAL::LED_COLORS::BLINK_ORANGE;
	case NavState::SAFETY:         return RobotHAL::LED_COLORS::ORANGE;
	case NavState::EDGE_LATCHED:   return RobotHAL::LED_COLORS::BLINK_RED;
	case NavState::STALL_LATCHED:  return RobotHAL::LED_COLORS::BLINK_PURPLE;
	case NavState::TURN_OVERRUN:   return RobotHAL::LED_COLORS::BLINK_CYAN;
	case NavState::HEADING_STUCK:  return RobotHAL::LED_COLORS::BLINK_YELLOW;
	case NavState::SPEND_SIGNAL:   return RobotHAL::LED_COLORS::BLINK_GREEN_FAST;
	case NavState::COLLECT_SIGNAL: return RobotHAL::LED_COLORS::BLINK_BLUE_FAST;
	case NavState::IDLE:           break;
	}
	return RobotHAL::LED_COLORS::OFF;
}

void CoasterbotFunctions::markBlockedArea(float direction) {
	const float blockedDistance = 0.20f;
	const float blockedX = pose_.getX() + direction * blockedDistance * std::cos(pose_.getTheta());
	const float blockedY = pose_.getY() + direction * blockedDistance * std::sin(pose_.getTheta());
	const float halfWidth = std::fabs(robotBoundingBoxWidth_) * 0.5f + 0.05f;
	const float halfLength = std::fabs(robotBoundingBoxLength_) * 0.5f + 0.05f;
	grid_->addObstacleRect(blockedX - halfLength, blockedY - halfWidth,
	                       blockedX + halfLength, blockedY + halfWidth, 0.0f);
	grid_->buildDistanceField();
}

// Ein Kantensensor loest nur an der Tischkante aus, und deren Lage ist seit der Vermessung bekannt.
// Die Meldung ist also eine Messung der Position senkrecht zu dieser Kante: die Pose wird auf die
// Kartengrenze gesetzt. Laengs der Kante und im Kurs bleibt sie unveraendert.
void CoasterbotFunctions::correctPoseAtEdge(bool frontEdge) {
	const float direction = frontEdge ? 1.0f : -1.0f;
	const float dx = direction * std::cos(pose_.getTheta());
	const float dy = direction * std::sin(pose_.getTheta());
	const float halfWidth = tableWidth_ * 0.5f;
	const float halfHeight = tableHeight_ * 0.5f;
	// Kante, auf die der ausloesende Sensor (Front oder Heck) am ehesten zeigt
	const struct { float facing; bool alongX; float sign; } edges[] = {
		{ dx, true,  1.0f}, {-dx, true, -1.0f},
		{ dy, false, 1.0f}, {-dy, false, -1.0f},
	};
	const auto* best = &edges[0];
	for (const auto& edge : edges) {
		if (edge.facing > best->facing) best = &edge;
	}
	if (best->facing < EDGE_CORRECTION_MIN_FACING) {
		std::cout << "[NAV] edge: no table edge in sensor direction -> pose unchanged" << std::endl;
		return;
	}
	const bool turning = std::fabs(hal_.getGyroZ()) > EDGE_CORRECTION_TURN_RATE;
	const float depth = turning ? EDGE_CORRECTION_TURN_DEPTH : 0.0f;
	const float current = best->alongX ? pose_.getX() : pose_.getY();
	const float corrected = best->sign * ((best->alongX ? halfWidth : halfHeight) - depth);
	if (std::fabs(corrected - current) > EDGE_CORRECTION_MAX) {
		std::cout << "[NAV] edge: correction " << corrected - current << "m implausible -> pose unchanged"
		          << std::endl;
		return;
	}
	std::cout << "[NAV] edge: pose " << (best->alongX ? "x" : "y") << " " << current << " -> " << corrected
	          << " m (" << (turning ? "turning" : "straight") << ")" << std::endl;
	if (best->alongX) {
		pose_.reset(corrected, pose_.getY(), pose_.getTheta());
	} else {
		pose_.reset(pose_.getX(), corrected, pose_.getTheta());
	}
}

void CoasterbotFunctions::rebuildNavigationGrid(bool preserveDynamicObstacles) {
	const float x0 = -tableWidth_ / 2.0f;
	const float x1 =  tableWidth_ / 2.0f;
	const float y0 = -tableHeight_ / 2.0f;
	const float y1 =  tableHeight_ / 2.0f;
	std::vector<Vec2> occupiedCellCenters;
	if (preserveDynamicObstacles && grid_) {
		for (int cy = 0; cy < grid_->rows(); ++cy) {
			for (int cx = 0; cx < grid_->cols(); ++cx) {
				const Cell cell{cx, cy};
				if (grid_->occupied(cell)) {
					const Vec2 center = grid_->cellToWorld(cell);
					const bool inLegacyBorderBand =
						center.x <= x0 + 0.16f || center.x >= x1 - 0.16f ||
						center.y <= y0 + 0.16f || center.y >= y1 - 0.16f;
					if (!inLegacyBorderBand) {
						occupiedCellCenters.push_back(center);
					}
				}
			}
		}
	}

	grid_ = std::make_unique<OccupancyGrid>(x0, y0, x1, y1, NAVIGATION_GRID_RESOLUTION);
	grid_->addObstacleRect(x0, y0, x0, y1, TABLE_EDGE_INFLATION);
	grid_->addObstacleRect(x1, y0, x1, y1, TABLE_EDGE_INFLATION);
	grid_->addObstacleRect(x0, y0, x1, y0, TABLE_EDGE_INFLATION);
	grid_->addObstacleRect(x0, y1, x1, y1, TABLE_EDGE_INFLATION);
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

	enum class State { RESET, SQUARE_APPROACH, SQUARE_RIGHT, SQUARE_LEFT, SQUARE_ALIGN,
						FORWARD, BACKWARD, CENTER, TURN, SET_POSE,
						EDGE_DETECTED, DONE, IDLE, DEFAULT };
	static State state = State::RESET;
	static State lastState = State::RESET;
	static float speed = 5.0f;
	static float gap2Edge = 0.10f;
	enum class LearningPhase { PHASE_X, PHASE_Y };
	static LearningPhase phase = LearningPhase::PHASE_X;
	static float distance = 0.0f;
	static float lastOdo = 0.0f;
	// Lage des Robotermittelpunkts beim Ausloesen des vorderen bzw. hinteren Kantensensors, vorzeichenrichtig
	// entlang der gerade vermessenen Achse. Der Abstand ist der Bereich, den der Mittelpunkt befahren kann.
	static float edgeForward = 0.0f;
	static float edgeBackward = 0.0f;
	static float previousCenterCoordinate = 0.0f;
	static bool buildGrid;
	// Ausrichten an der ersten Kante, damit die x-Achse unabhaengig von der Startausrichtung senkrecht
	// zur Kante liegt: Auf der Stelle nach rechts drehen, bis der linke vordere Sensor ausloest, dann
	// nach links, bis der rechte ausloest. Bei unveraenderter Lage des Mittelpunkts liegen beide
	// Ausloesekurse symmetrisch zur Kantennormalen, die Mitte ist also senkrecht zur Kante. Gleicher
	// Nachlauf in beide Richtungen faellt heraus, Abstand und Lage der Sensoren gehen nicht ein.
	static float squareStartYaw = 0.0f;
	static float squareYawLeft = 0.0f;   // Kurs beim Ausloesen vorne links (Drehung nach rechts)
	static float squareTargetYaw = 0.0f;
	static bool squareArmed = false;     // vorne links war waehrend SQUARE_RIGHT schon einmal frei
	constexpr float squareTurnSpeed = 0.5f;          // [rad/s], wie das Feindrehen in TURN
	constexpr float squareMaxTurn = 1.05f;           // [rad] ~60 Grad je Richtung, sonst Abbruch
	constexpr float squareTolerance = _HALF_PI * 0.01f;
	constexpr float radToDeg = 90.0f / _HALF_PI;
	const auto wrap = [](float a) { return std::atan2(std::sin(a), std::cos(a)); };
	const auto axisCoordinate = [this]() {
		return phase == LearningPhase::PHASE_X ? pose_.getX() : pose_.getY();
	};

	switch (state) {
	case State::RESET:
		if(!this->blocklessWait(2.0f)) break;
		pose_.reset(0.0f, 0.0f, 0.0f);
		
		buildGrid = false;

		state = State::SQUARE_APPROACH;
		break;
	case State::SQUARE_APPROACH:
		this->forward(speed);
		if (this->edgeDetected()) {
			std::cout << "[LEARN] Edge reached, squaring up: fl=" << hal_.getEdgeFrontLeft()
			          << " fr=" << hal_.getEdgeFrontRight() << std::endl;
			this->stop();
			squareStartYaw = hal_.getYaw();
			squareArmed = false;
			state = State::SQUARE_RIGHT;
		}
		break;
	case State::SQUARE_RIGHT:
		// Der Kurs im Moment des Ausloesens zaehlt, nicht der nach dem Bremsen. Hat vorne links schon beim
		// Anfahren ausgeloest, gilt das nicht: der Mittelpunkt ist danach noch nachgelaufen, die Messung von
		// vorne rechts erfolgt aber erst nach dem Nachlauf. Dann erst nach links freidrehen.
		if (!squareArmed) {
			if (hal_.getEdgeFrontLeft()) {
				this->turnLeft(squareTurnSpeed);
			} else {
				squareArmed = true;
			}
		}
		if (!squareArmed) {
			if (std::fabs(wrap(hal_.getYaw() - squareStartYaw)) > squareMaxTurn) {
				std::cout << "[LEARN] squaring failed (front left stays on the edge) -> keeping start heading"
				          << std::endl;
				this->stop();
				squareTargetYaw = squareStartYaw;
				state = State::SQUARE_ALIGN;
			}
		} else if (hal_.getEdgeFrontLeft()) {
			squareYawLeft = hal_.getYaw();
			squareStartYaw = squareYawLeft;
			state = State::SQUARE_LEFT;
		} else if (std::fabs(wrap(hal_.getYaw() - squareStartYaw)) > squareMaxTurn) {
			std::cout << "[LEARN] squaring failed (front left never triggered) -> keeping start heading"
			          << std::endl;
			this->stop();
			squareTargetYaw = squareStartYaw;
			state = State::SQUARE_ALIGN;
		} else {
			this->turnLeft(-squareTurnSpeed);
		}
		break;
	case State::SQUARE_LEFT:
		if (hal_.getEdgeFrontRight()) {
			const float squareYawRight = hal_.getYaw();
			squareTargetYaw = wrap(squareYawLeft + 0.5f * wrap(squareYawRight - squareYawLeft));
			std::cout << "[LEARN] squaring: yaw left=" << squareYawLeft * radToDeg
			          << " deg, right=" << squareYawRight * radToDeg
			          << " deg -> normal=" << squareTargetYaw * radToDeg << " deg" << std::endl;
			state = State::SQUARE_ALIGN;
		} else if (std::fabs(wrap(hal_.getYaw() - squareStartYaw)) > 2.0f * squareMaxTurn) {
			std::cout << "[LEARN] squaring failed (front right never triggered) -> using left trigger heading"
			          << std::endl;
			this->stop();
			squareTargetYaw = squareYawLeft;
			state = State::SQUARE_ALIGN;
		} else {
			this->turnLeft(squareTurnSpeed);
		}
		break;
	case State::SQUARE_ALIGN:
		{
			const float error = wrap(squareTargetYaw - hal_.getYaw());
			if (std::fabs(error) <= squareTolerance) {
				this->stop();
				if (!this->blocklessWait(1.0f)) break;
				// ab hier zeigt theta = 0 senkrecht auf die erste Kante, die x-Achse liegt parallel zur Tischkante
				pose_.reset(0.0f, 0.0f, 0.0f);
				std::cout << "[LEARN] squared to edge, residual=" << error * radToDeg
				          << " deg. Start learning X." << std::endl;
				state = State::FORWARD;
			} else {
				this->turnLeft(error > 0.0f ? squareTurnSpeed : -squareTurnSpeed);
			}
		}
		break;
	case State::FORWARD:
		this->forward(speed);
		if (this->edgeDetected()) {
			std::cout << "[LEARN] Edge detected while moving forward." << std::endl;
			state = State::EDGE_DETECTED;
			lastState = State::FORWARD;
			lastOdo = pose_.getOdometer();
			edgeForward = axisCoordinate();
		}
		break;
	case State::BACKWARD:
		this->backward(speed);
		if (this->edgeDetected()) {
			std::cout << "[LEARN] Edge detected while moving backward." << std::endl;
			state = State::EDGE_DETECTED;
			lastState = State::BACKWARD;
			lastOdo = pose_.getOdometer();
			edgeBackward = axisCoordinate();
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
				state = State::BACKWARD;
			}
			break;
		case State::BACKWARD:
			if ((pose_.getOdometer() - lastOdo) < gap2Edge) {
				this->forward(speed);
			} else {
				lastOdo = 0.0f;
				// Nicht ueber den Odometer: der zaehlt Nachlauf und Zuruecksetzen als Betrag mit und ergab
				// ~10 cm zu viel. Die Pose ist vorzeichenrichtig, Nachlauf hebt sich heraus.
				distance = edgeForward - edgeBackward;
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
			this->stop();
			if(!this->blocklessWait(2.0f)) break;
			// Nullpunkt = Mitte zwischen den beiden Ausloesepunkten. Frueher wurde die aktuelle Lage
			// (gap2Edge vor dem hinteren Ausloesepunkt) als -Breite/2 gesetzt -> Karte um gap2Edge verschoben.
			// Kurs und Querlage behalten: Faehrt der Roboter leicht im Bogen, hat er sich wirklich gedreht.
			// Das fruehere theta = 0 (und 90 Grad nach TURN und in Y) drehte das Kartensystem jedes Mal um
			// diesen Rest gegen den Tisch, zusammen ~5 Grad -> ~6 cm Versatz an den Tischenden.
			pose_.reset(pose_.getX() - (edgeForward + edgeBackward) / 2.0f, pose_.getY(), pose_.getTheta());
			previousCenterCoordinate = pose_.getX();
			std::cout << "[LEARN] set x to: " << pose_.getX() << " m" << std::endl;
			std::cout << "[LEARN] Move to center." << std::endl;
			state = State::CENTER;
		} else {
			tableHeight_ = distance;
			this->stop();
			if(!this->blocklessWait(2.0f)) break;
			pose_.reset(pose_.getX(), pose_.getY() - (edgeForward + edgeBackward) / 2.0f, pose_.getTheta());
			previousCenterCoordinate = pose_.getY();
			std::cout << "[LEARN] set y to: " << pose_.getY() << " m" << std::endl;
			state = State::CENTER;
		}
		break;
	case State::TURN:
		{
			// auf den absoluten Kurs 90 Grad im Tischsystem drehen, nicht um 90 Grad relativ: der Kurs nach
			// dem Zentrieren weicht um die Bogenfahrt von 0 ab
			const float turnError = std::atan2(std::sin(_HALF_PI - pose_.getTheta()),
			                                   std::cos(_HALF_PI - pose_.getTheta()));
			if (std::fabs(turnError) <= _HALF_PI * 0.01f) {
			this->stop();
			std::cout << "[LEARN] Turn accepted: theta="
			          << pose_.getTheta() * 180.0f / (_HALF_PI * 2.0f)
			          << " deg (target=90). Move forward." << std::endl;
			state = State::FORWARD;
			} else {
				const float turnSpeed = std::fabs(turnError) <= _HALF_PI / 18.0f
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
			grid_ = std::make_unique<OccupancyGrid>(x0, y0, x1, y1, NAVIGATION_GRID_RESOLUTION);
			grid_->addObstacleRect(x0, y0, x0, y1, TABLE_EDGE_INFLATION);
			grid_->addObstacleRect(x1, y0, x1, y1, TABLE_EDGE_INFLATION);
			grid_->addObstacleRect(x0, y0, x1, y0, TABLE_EDGE_INFLATION);
			grid_->addObstacleRect(x0, y1, x1, y1, TABLE_EDGE_INFLATION);
			grid_->buildDistanceField();
			dstar_ = std::make_unique<DStarLite>(*grid_);
			std::cout << "[LEARN] table grid initialized: width=" << tableWidth_
				      << "m, height=" << tableHeight_ << "m" << std::endl;
			std::cout << "[LEARN] Learning completed. Centering robot for navigation." << std::endl;
			buildGrid = true;
		}
		if(!this->blocklessWait(2.0f)) break;
		// Kein Ruecksetzen auf (0, 0): Der Nullpunkt liegt seit SET_POSE schon in der Tischmitte, der
		// Roboter haelt beim Zentrieren aber erst innerhalb von 5 cm und laeuft nach. Das frueher hier
		// gesetzte (0, 0) verschob die Karte um diesen Rest.
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
		navState_ = NavState::REACHED;
		std::cout << "[SPEND] At coaster " << coasterRoutineIndex_ << ", dispensing" << std::endl;
		return false;
	}

	if (coasterRoutinePhase_ == CoasterRoutinePhase::SETTLE) {
		bool coasterDone;
		if (SERVO_ACTIONS_ENABLED) {
			coasterDone = spendCoaster(coasterRoutineIndex_);
		} else {
			navState_ = NavState::SPEND_SIGNAL;
			coasterDone = now - coasterRoutinePhaseStartedAt_ >= COASTER_SIGNAL_TIME;
		}
		if (coasterDone) {
			++coasterRoutineIndex_;
			coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
			if (coasterRoutineIndex_ >= MAX_COASTERS) {
				coasterRoutineActive_ = false;
				std::cout << "[SPEND] All coasters dispensed" << std::endl;
				return true;
			}
		}
		return false;
	}

	return false;
}

bool CoasterbotFunctions::spendCoaster(int coasterIndex) {
	if (coasterIndex < 0 || coasterIndex >= MAX_COASTERS) {
		std::cout << "[SPEND] Invalid coaster index: " << coasterIndex << std::endl;
		return true;
	}
	
	switch (coasterSpending_) {
		case SpendingStates::IDLE:
			if( (coasterIndex+1) - coastersSpend == 1 and coastersSpend < MAX_COASTERS) {
				coasterSpending_ = SpendingStates::SET_LIFTER_DOWN;
				return false;
			}
			else {
				std::cout << "[SPEND] Invalid coaster index: " << coasterIndex << std::endl;
				std::cout << "[SPEND] Coaster out: " << coastersSpend << std::endl;
				std::cout << "[SPEND] Coaster MAX: " << MAX_COASTERS << std::endl;
				return true;
			}
		break;
		case SpendingStates::SET_LIFTER_DOWN:
			hal_.setServoPosition(static_cast<int>(ServoId::LIFTER), SERVO_POS_LIFTER[coasterIndex], SERVO_LIFTER_RAMP_MS);

			if(blocklessWait(2.0f)) {
				coasterSpending_ = SpendingStates::SET_SPENDER_SPEND;
			}
			else {
				return false;
			}
		break;
		case SpendingStates::SET_SPENDER_SPEND:
			hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[0], SERVO_SPENDER_RAMP_MS);

			if(blocklessWait(2.0f)) {
				coasterSpending_ = SpendingStates::SET_SPENDER_HOME;
			}
			else {
				return false;
			}
		break;
		case SpendingStates::SET_SPENDER_HOME:
			hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[1], SERVO_SPENDER_RAMP_MS);

			if(blocklessWait(2.0f)) {
				coasterSpending_ = SpendingStates::SET_LIFTER_UP;
			}
			else {
				return false;
			}
		break;
		case SpendingStates::SET_LIFTER_UP:
			hal_.setServoPosition(static_cast<int>(ServoId::LIFTER), 180.0f, SERVO_LIFTER_RAMP_MS);

			if(blocklessWait(2.0f)) {
				coasterSpending_ = SpendingStates::IDLE;
				coastersSpend++;
				return true;
			}
			else {
				return false;
			}
		break;
	}

	return false;
	
}

void CoasterbotFunctions::resetSpendCoasterSequence() {
	coasterSpending_ = SpendingStates::IDLE;
	coasterCollecting_ = CollectingStates::IDLE;
	coastersSpend = 0;
	resetLastTimer_ = true;
}

void CoasterbotFunctions::abortRoutine() {
	this->stop();
	coasterRoutineActive_ = false;
	coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
	// Servo-Ablaeufe neu beginnen; coastersSpend bleibt, die ausgegebenen Coaster liegen ja noch
	coasterSpending_ = SpendingStates::IDLE;
	coasterCollecting_ = CollectingStates::IDLE;
	resetLastTimer_ = true;
	centerSettling_ = false;
	navigationTargetInitialized_ = false;  // setzt beim naechsten Ziel auch alle Navigationsstopps zurueck
	navigationPlannerInitialized_ = false;
	navigationAligning_ = false;
	navState_ = NavState::IDLE;
	std::cout << "[NAV] routine aborted by user" << std::endl;
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
		rebuildNavigationGrid(false);
		navigationTargetInitialized_ = false;
		navigationPlannerInitialized_ = false;
		navigationAligning_ = false;
		centerSettling_ = false;
		std::cout << "[NAV] center re-anchored after settling, residual="
		          << settledDistance << "m; cleared dynamic obstacle map" << std::endl;
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
		navState_ = NavState::REACHED;
		std::cout << "[GET] At coaster " << coasterRoutineIndex_ << ", picking up" << std::endl;
		return true;
	}

	// Aufnahme analog zu runSpendCoasters(): Ablauf komplett in getCoaster()
	if (coasterRoutinePhase_ == CoasterRoutinePhase::SETTLE) {
		bool coasterDone;
		if (SERVO_ACTIONS_ENABLED) {
			coasterDone = getCoaster(coasterRoutineIndex_);
		} else {
			navState_ = NavState::COLLECT_SIGNAL;
			coasterDone = now - coasterRoutinePhaseStartedAt_ >= COASTER_SIGNAL_TIME;
		}
		if (coasterDone) {
			--coasterRoutineIndex_;
			coasterRoutinePhase_ = CoasterRoutinePhase::TRAVEL;
			if (coasterRoutineIndex_ < 0) {
				coasterRoutineActive_ = false;
				std::cout << "[GET] All coasters collected" << std::endl;
				return false;
			}
		}
		return true;
	}

	return true;
}

bool CoasterbotFunctions::getCoaster(int coasterIndex) {
	if (coasterIndex < 0 || coasterIndex >= MAX_COASTERS) {
		std::cout << "[SPEND] Invalid coaster index: " << coasterIndex << std::endl;
		return true;
	}
	
	switch (coasterCollecting_) {
		case CollectingStates::IDLE:
			if (coastersSpend == coasterIndex + 1) {
				coasterCollecting_ = CollectingStates::SET_SPENDER_ASIDE;
				return false;
			}
			else {
				std::cout << "[SPEND] Invalid coaster index: " << coasterIndex << std::endl;
				std::cout << "[SPEND] Coaster out: " << coastersSpend << std::endl;
				std::cout << "[SPEND] Coaster MAX: " << MAX_COASTERS << std::endl;
				return true;
			}
		break;
		case CollectingStates::SET_SPENDER_ASIDE:
			hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[2], SERVO_SPENDER_RAMP_MS);

			if(blocklessWait(2.0f)) {
				coasterCollecting_ = CollectingStates::SET_LIFTER_DOWN;
			}
			else {
				return false;
			}
		case CollectingStates::SET_LIFTER_DOWN:
			hal_.setServoPosition(static_cast<int>(ServoId::LIFTER), SERVO_POS_LIFTER[coasterIndex], SERVO_LIFTER_RAMP_MS);

			if(blocklessWait(5.0f)) {
				coasterCollecting_ = CollectingStates::SET_LIFTER_UP;
			}
			else {
				return false;
			}
		break;
		
		case CollectingStates::SET_LIFTER_UP:
			hal_.setServoPosition(static_cast<int>(ServoId::LIFTER), 180.0f, SERVO_LIFTER_RAMP_MS);

			if(blocklessWait(2.0f)) {
				coasterCollecting_ = CollectingStates::SET_SPENDER_HOME;
				
			}
			else {
				return false;
			}
		break;

		case CollectingStates::SET_SPENDER_HOME:
			hal_.setServoPosition(static_cast<int>(ServoId::SPENDER), SERVO_POS_SPENDER[1], SERVO_SPENDER_RAMP_MS);

			if(blocklessWait(2.0f)) {
				coasterCollecting_ = CollectingStates::IDLE;
				coastersSpend--;
				return true;
			}
			else {
				return false;
			}
		break;
	}

	return false;
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
