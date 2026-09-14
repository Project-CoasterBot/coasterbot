# CLAUDE.md

Webots project directory holding several tutorial/experiment worlds plus the
**Coasterbot** robot (a skid-steer 4-wheeled tabletop robot imported from CAD).

**Location (2026-09-07):** this dir now lives inside the project repo at
`~/Documents/coasterbot/M4_Test-und-Validierung/webots/`; `~/Documents/webot` is
a symlink to it. Builds/worlds use relative paths + `$WEBOTS_HOME`, so either
path works.

## Layout

```
worlds/        *.wbt simulation worlds (+ hidden .wbproj perspective files)
controllers/   robot controllers, one folder per controller
protos/        local PROTO models (Coasterbot lives here)
libraries/  plugins/   Webots plugin/library stubs (unused so far)
```

Webots auto-discovers anything under `controllers/` and `protos/` (recursively),
so a controller named `"coasterbot"` resolves to `controllers/coasterbot/`.

### Git hygiene note
The repo's root `.gitignore` (`Makefile*`, `build/`, `*.o`, `*.obj`, …) needs
care here: it **ignores the controller `Makefile`s** (required to build!) and
`worlds/Coasterbot.obj`, while it would commit the compiled controller binaries
(`controllers/*/coasterbot`, `.../epuck_*`) and the unused 33 MB
`worlds/Coasterbot.dae`. Force-add the Makefiles, and ignore the binaries + big
CAD exports (see the webots-local `.gitignore` if one was added).

## Worlds

| World | Robot | Controller | Notes |
|-------|-------|------------|-------|
| `coasterbot-robot.wbt` | `Coasterbot` PROTO | `coasterbot` (C++) | The integrated Coasterbot demo. Robot on a `Table` with a few bottles/can as obstacles. |
| `coasterbot.wbt` | E-puck + `Coasterbot` PROTO | E-puck: `<none>`; Coasterbot: `coasterbot` | Original table scene; the `Coasterbot` was added on the table next to the E-puck. |
| `coasterbot-collision-avoidance.wbt` | E-puck | `epuck_avoid_collision` | |
| `coasterbot-4_wheeled_robot.wbt` | E-puck + hand-built `Robot` | `epuck_avoid_collision` | The `Robot` node here is a from-scratch 4-wheeler, not the PROTO. |
| `coasterbot-testfield.wbt` | `Coasterbot` PROTO | `coasterbot` | Open 4×4 m tiled floor, no obstacles — nav-stack bring-up (odometry, A\*, DWA). |
| `coasterbot-astar.wbt` | `Coasterbot` PROTO | `coasterbot` | A\*+DWA demo: 4×4 m floor, two offset walls (slalom: gap right, then gap left), green goal marker. Geometry **must** match `nav_scenario` in `main.cpp`. Run with `MODE_NAVIGATE`. |
| `coasterbot-dstar.wbt` | `Coasterbot` PROTO | `coasterbot` | D\* Lite demo: wall with two gaps; an **unmapped** orange box blocks the right gap, discovered by the ultrasonic → reroute through the left gap. Geometry **must** match `dstar_scenario` in `main.cpp`. Run with `MODE_NAVIGATE_DSTAR`. |
| `coasterbot-simtest.wbt` | `Coasterbot` PROTO | `coasterbot` | Combined simulation test on the **Table** (1.6×2.0) with a lavender `FlowerPot` + `BeerBottle`s: wayfinding + obstacle avoidance + table-edge safety. Geometry **must** match `sim_scenario` in `main.cpp`. Run with `MODE_SIM_TEST`. |
| `tutorial2.wbt` | – | – | Floor + wall + ball. |

`worlds/Coasterbot.obj` / `Coasterbot.dae` are large raw CAD exports, not used by
any world.

## Coasterbot robot

### PROTO — `protos/coasterbot/Coasterbot.proto`
- Auto-derived from `Coasterbot-Assembly.step`; meshes in `protos/coasterbot/meshes/`.
- **Authored Y-up** (`#VRML_SIM R2023b`, header says "Webots ist Y-up"):
  local **+X = right, +Y = up, +Z = rear** (so **-Z = forward**).
  The current worlds are **Z-up (ENU)**, so instances need
  `rotation 1 0 0 1.5708` to stand upright (see `coasterbot-robot.wbt`).
- Devices (names are the contract with the controller):
  - Motors: **coupled per side** (Webots "coupled motors" — same name before
    `::`): `wheel_left::front` + `wheel_left::rear`, `wheel_right::front` +
    `wheel_right::rear`. A command to one drives both wheels of that side —
    mirrors the real Coasterbot (two TT motors per side, driven together).
    The controller only ever addresses `wheel_left::front` / `wheel_right::front`.
    Individual wheel drive is not possible.
  - Encoders: `wheel_{front,rear}_{left,right}_sensor` PositionSensors — the
    PROTO still exposes 4 (one per wheel joint), but the real electronics only
    has **one encoder per side** (2026-09-14 hardware feedback), so
    `RobotHAL::WheelId` only has `WHEEL_LEFT`/`WHEEL_RIGHT`; `WebotsHAL`
    averages front+rear per side internally before exposing it. All hinges
    about local X → skid-steer.
  - `ultrasonic` DistanceSensor (sonar, forward, **analog**). lookupTable
    returns **metres** (`0 0 0 / 4 4 0.02`) — matches `OBSTACLE_THRESHOLD_M`.
    (The original `4 4000` returned mm, so obstacle detection never fired —
    recalibrated.)
  - `edge_front_left/right`, `edge_rear_left/right` DistanceSensors (infra-red,
    point down, **digital** — 2026-09-14 hardware feedback: real sensor module
    has a built-in comparator, no analog voltage). lookupTable is a near-step
    function (`0 0 / 0.098 0 / 0.10 1 / 4 1`, 2mm transition) returning a
    clean 0/1, not the earlier analog 0..1000 ramp. `WebotsHAL::digitalEdge()`
    maps that through `EDGE_ACTIVE_HIGH` to the semantic `bool` — the only
    place the sensor's electrical polarity is known. `RobotHAL::getEdge*()`
    return `bool` (not `float`); `RobotLogic`/`SafetyMonitor` no longer have
    their own `EDGE_THRESHOLD` — that calibration lives solely in `WebotsHAL`.
  - `inertial_unit`, `gyro`, `accelerometer` near the CoM.
- Chassis `boundingObject` is deliberately two stacked boxes (narrow lower part
  fits between the wheels) to stop the robot jittering — don't "simplify" it back
  to one box.

### Controller — `controllers/coasterbot/` (C++)
Three-layer design, keep it that way:
- `robot_hal.h` — pure abstract `RobotHAL` interface (no Webots, no Arduino).
- `webots_hal.{h,cpp}` — `WebotsHAL : RobotHAL`, the only Webots-aware file.
  Device name strings here must match the PROTO exactly. Holds one motor handle
  per side (`motorLeft_`/`motorRight_` = the coupled `::front` motors).
  `setLeftSpeed`/`setRightSpeed` negate the command: the PROTO wheel model rolls
  the robot toward +Z for a positive velocity, but the HAL contract is
  "positive = forward" and the robot's front is -Z (the ultrasonic-holder bump).
  The `RobotHAL` drive interface is per-side only (`setLeftSpeed`/`setRightSpeed`);
  there is no single-wheel command — the motors are coupled.
- `robot_logic.{h,cpp}` — behaviour (edge + obstacle avoidance state machine),
  talks only to `RobotHAL`. Intended to port unchanged to an Arduino `ArduinoHAL`.
- `main.cpp` — has a compile-time `MODE` switch:
  `MODE_WHEEL_TEST` (spin each wheel in turn), `MODE_MANUAL` (fixed move
  sequence), `MODE_AUTONOMOUS` (run `RobotLogic::update()` — **current default**),
  `MODE_ODOMETRY_TEST` (drive straight then turn in place, print pose — for
  checking `PoseEstimator` on `coasterbot-testfield.wbt`),
  `MODE_NAVIGATE` (plan an A\* path on `nav_scenario`'s map and follow it — for
  `coasterbot-astar.wbt`),
  `MODE_NAVIGATE_DSTAR` (D\* Lite + sensor-discovered obstacle → reroute — for
  `coasterbot-dstar.wbt`),
  `MODE_SIM_TEST` (combined test on a table — for `coasterbot-simtest.wbt`).
  Change the `static const Mode MODE = …;` line to switch.

### Nav stack — `occupancy_grid` / `astar` / `dstar_lite` / `dwa_planner`
`MODE_NAVIGATE` = `astarPlan` → `DwaPlanner`.
`MODE_NAVIGATE_DSTAR` = `DStarLite` → `DwaPlanner`, with the ultrasonic feeding
newly-seen obstacles back into the grid + planner.
- `OccupancyGrid` — static world-frame grid (X-Y, Z up). `addObstacleRect` inflates
  by the robot half-width (used for a priori known obstacles — hard, immediate).
  `buildDistanceField()` (chamfer) → `clearance(p)` for DWA. `reportOccupied(c,
  smoothingFactor)` (2026-09-14) is the smoothed counterpart for *live* sensor
  hits: exponentially tracks a per-cell confidence, only calls `setOccupied`
  once it crosses `OCC_COMMIT_THRESHOLD` (0.8) — a single noisy reading no
  longer flips the map / triggers a D* Lite replan (needs ~3 consistent ticks
  at the default 0.5 smoothing factor). `main.cpp`'s `markRect()` helpers (both
  `dstar_scenario` and `sim_scenario`) use this now instead of `setOccupied`
  directly. `DwaPlanner`'s own raw-ultrasonic reflex avoidance is untouched —
  only the deliberative D* Lite replan decision is smoothed.
- `astarPlan(grid, start, goal)` — 8-connected A\*, euclidean heuristic, no
  corner-cutting, then line-of-sight "string pulling". World-coord waypoints; `{}` = no path.
- `smoothPath()` (string-pulling) + `centerPath()` — the latter shifts interior
  waypoints toward higher clearance (perpendicular to travel) so paths run down
  corridor centres instead of hugging a corner (DWA can't follow corner-hugging
  paths — it stalls). `MODE_SIM_TEST` uses both; `centerPath` needs a grid with
  a distance field.
- `DwaPlanner` — Dynamic Window Approach. Each tick: dynamic window from accel
  limits → sample `(v, ω)` → forward-simulate `SIM_TIME` → score
  `W_HEADING·align + W_PROGRESS·closing + W_CLEAR·clearance + W_VEL·speed`, reject
  colliding / can't-brake trajectories → best `(v, ω)` → wheel speeds. Carrot =
  pure-pursuit point `CARROT_DIST` along the A\* polyline (stays on the
  collision-free path, no corner-cutting). Obstacles from the grid distance field
  **plus** the live ultrasonic as a virtual point (unmapped / dynamic obstacles).
  `!any` admissible → rotate in place to escape. Grid is pre-inflated so DWA only
  adds `GRID_MARGIN`; the raw ultrasonic point gets the full `ROBOT_RADIUS`.
- `DStarLite` — D\* Lite (Koenig & Likhachev), 8-connected, backward search from
  goal, `g`/`rhs` + a `set`-based priority queue. `plan()` does a full initial
  expansion (`expandAll`) so later local changes repair cleanly; `setStart` +
  `computeShortestPath` each cell as the robot drives (km accumulation);
  `cellChanged` queues affected vertices when the map changes. `MODE_NAVIGATE_DSTAR`
  re-extracts the path on a map change; if the whole planned corridor is severed
  (incremental repair returns no path) it re-initialises D\* Lite from the current
  pose — still cheaper than replanning every tick, and it finds the alternate gap.
- **Two grids** in `MODE_NAVIGATE_DSTAR`: `planGrid` inflated 0.30 m (keeps the
  path off corners) for D\* Lite, `dwaGrid` inflated 0.16 m (robot radius) for DWA
  clearance. Sensor hits are marked in both. This decoupling is what lets DWA
  actually follow the planner's corner-hugging shortest paths.
- `nav_scenario` / `dstar_scenario` / `sim_scenario` (in `main.cpp`) —
  start/goal/map for the navigate + sim-test modes; each must match its world.

### Safety layer — `safety_monitor.{h,cpp}`
`SafetyMonitor` = the docs' "Sicherheitskomponente". Reads the 4 edge sensors
every tick; on a table-edge detection it **takes over the actuators**
(`overriding()` true) and reverses off a front edge / forwards off a rear edge /
stops if pinned, then pauses and hands back. `MODE_SIM_TEST` calls it *before*
the planner each tick and, on an edge event during navigation, also marks the
edge cells in the grids and replans. Nothing else may drive while it is
overriding.

### `MODE_SIM_TEST` — combined simulation test (Testkonzept ch. 5)
`SafetyMonitor` → `DStarLite` → `centerPath(smoothPath())` → `DwaPlanner`, on the
`Table` PROTO (1.6×2.0 m, top ≈ 0.715). `sim_scenario`: the grid is bounded to
the tabletop (so plans stay on it), border inflation `0.12`/`0.10` for
plan/DWA grids; a `BeerBottle` sits mid-table on the route (known, inflated 0.15
→ D\* Lite plans around it), a `FlowerPot` + a second bottle sit in the corners
off the route. `dwa.setUltrasonicRange(0.45)` — DWA is only the close-range
backstop; a *genuinely unmapped* object (not near a known one) still triggers a
D\* Lite replan. Phase 1 = navigate start→goal around the bottle. Phase 2 =
rotate to face +Y then drive slowly at the table edge; the SafetyMonitor must
stop it. Prints BESTANDEN/FEHLGESCHLAGEN for **ST-SIM-001** (reach goal ±tol),
**ST-SIM-002** (route weaves / replans + never touches an obstacle + stays on
table), **ST-SIM-003** (edge events in phase 2, never leaves the table).
**ST-SIM-004** (reproducibility) = run twice, diff the summary (verified
byte-identical).

Note: a single narrow forward ultrasonic can't reliably find small tabletop
objects that are slightly off-axis — that's why the pot/bottles are in the map
here (a "surveyed table"). The pure sensor-discovery + reroute case is the
`MODE_NAVIGATE_DSTAR` demo with a large obstacle. A wider FOV sensor or a
proper costmap would let the sim test do full sensor discovery.

### Pose estimator — `pose_estimator.{h,cpp}`
`PoseEstimator` (step 1 of the nav stack) fuses skid-steer wheel odometry
(`RobotHAL::getWheelAngle`, from the `wheel_*_sensor` encoders) with the
integrated gyro yaw rate (`getGyroZ`) via a complementary filter
(`HEADING_ODO_WEIGHT = 0.05`, i.e. mostly gyro — skid-steer scrubs on turns).
Exposes `getX/getY/getTheta` (robot start frame: x forward, y left, θ CCW+) and
`getOdometer()`. `reset(x,y,θ)` seeds it from a known world start pose.
`main.cpp` calls `pose.update()` every tick in `MODE_AUTONOMOUS` and
`MODE_ODOMETRY_TEST`, printing `[POSE] …` every 2 s.

Note: `getGyroZ()` reads the gyro's **Y** component — that IS the world vertical
here, because the Y-up PROTO is stood up with `rotation 1 0 0 1.5708`. It is
correct, not a bug. `getYaw()` (InertialUnit) is avoided in the estimator: with
that same 90° stand-up rotation the IMU sits near pitch gimbal-lock.

## Building / running

Webots lives at `~/Documents/webots/webots/` (R2025b nightly, commit
`033dbaea…` — same as the `EXTERNPROTO` pins). It is not on `PATH`.

```bash
export WEBOTS_HOME=~/Documents/webots/webots

# build just the controller
cd controllers/coasterbot && make

# run a world (builds the controller on first run)
$WEBOTS_HOME/webots worlds/coasterbot.wbt                       # GUI
$WEBOTS_HOME/webots --mode=fast --no-rendering --stdout --stderr worlds/coasterbot.wbt   # headless
```

The Windows `.exe` files and `build/` dirs from the zip were removed on import —
Webots rebuilds the controller for the host platform automatically.

### Verified (2026-09-06)
Controller builds clean on Linux. In `coasterbot.wbt` the robot drives front-first
(ultrasonic-holder bump leading) and the state machine exercises all four states —
`FORWARD`, `AVOID_OBSTACLE` (ultrasonic sees the beer bottles), `REVERSE` + `TURN`
(edge sensors at the table rim). Console prints each transition
(`[AUTONOMOUS] t=… -> Zustand: …`).

## Project target — navigation stack

The wider Coasterbot project (docs repo: `~/Documents/coasterbot/`) wants this sim
to demonstrate a real navigation stack, not just the reactive wander that
`robot_logic.cpp` does today.

**Mission state machine** (docs `Pflichtenheft/Zustandsautomaten.svg`):
`IDLE → SEARCH_COASTER → PICK_UP → TRANSPORT → PLACE → VERIFY → DONE`.

**Chosen algorithms** (docs `Alternativenbeschreibung.md`, `MindMap/KonzeptMindMap-Algorithmen.svg`):

| Layer | Algorithm | Role |
|---|---|---|
| Global path | **A\*** | route on a known table occupancy grid to a goal pose |
| Dynamic replan | **D\* Lite** | incrementally repair the path when obstacles move/appear |
| Local avoidance | **DWA** | dynamics-aware `(v, ω)` choice each tick |
| Localization | wheel-encoder odometry + IMU, fused | supply `(x, y, θ)` |
| Manipulation | IK + RRT\* | coaster arm — out of scope for the drive sim for now |

**Build order (agreed 2026-09-06, easiest first):**
1. **Pose estimator** from wheel encoders + gyro → `(x, y, θ)`.  ✅ done — see
   `pose_estimator.{h,cpp}`; verified straight-line 0.699 m vs 0.70 m expected,
   no lateral drift, in-place turn tracked by gyro, stable when stopped.
2. **A\* global planner** + `coasterbot-astar.wbt` (map + goal).  ✅ done.
3. **DWA local planner** (`dwa_planner`, replaced `PathFollower`).  ✅ done —
   `MODE_NAVIGATE` drives the full slalom in 39 s, 5 cm residual, no wall contact.
4. **D\* Lite replanning** (`dstar_lite`).  ✅ done — `MODE_NAVIGATE_DSTAR`: robot
   heads for the right gap, ultrasonic finds the unmapped box, D\* Lite reroutes
   through the left gap, reaches goal in 24 s, 1.5 cm residual (headless).

Nav stack (steps 1–4) is functionally complete.

5. **Combined simulation test** (`MODE_SIM_TEST`, `safety_monitor`, `centerPath`).
   ✅ done — wayfinding + obstacle avoidance + edge detection on the `Table` with a
   lavender pot + beer bottles. Verified: ST-SIM-001 (goal, 5.8 cm), ST-SIM-002
   (routes around the bottle, 0.22 m clearance), ST-SIM-003 (4 edge events, stays
   on table) all BESTANDEN; ST-SIM-004 (reproducibility) byte-identical over two
   headless runs.

Follow-ups: soft (gradient) costmap so one grid serves planner + DWA (would
retire `centerPath` and the dual grids); incremental D\* Lite repair for the
fully-severed case; ST-SIM-005/006 fault injection (sensor/actuator failure);
the mission state machine (IDLE→…→DONE); the coaster arm (IK + RRT\*); an
**initial table-exploration behaviour** (2026-09-14 idea, not implemented) —
drive to all four table corners, place a coaster at each, return to start, so
later placement/pickup can target known coaster positions instead of object
detection. Blocked on the coaster-handling stack (out of scope so far) and
needs care around `SafetyMonitor`'s edge reaction near the corners. See
`Softwaredokumentation.tex` §16.1 for the write-up.

Odometry geometry: wheel radius ≈ 0.035 m, track width (L↔R anchors `x = ±0.101`)
= 0.202 m, wheelbase (F↔R anchors `z = ±0.0566`) = 0.113 m.
