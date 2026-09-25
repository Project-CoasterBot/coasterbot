### Coasterbot C++ Controller Makefile
# Jede hier gelistete .cpp-Datei wird kompiliert.

CXX_SOURCES = main.cpp robot_logic.cpp pose_estimator.cpp \
              occupancy_grid.cpp astar.cpp dstar_lite.cpp dwa_planner.cpp \
              safety_monitor.cpp fault_monitor.cpp webots_hal.cpp coasterbot_functions.cpp

### Do not modify: this includes Webots global Makefile.include
null :=
space := $(null) $(null)
WEBOTS_HOME_PATH=$(subst $(space),\ ,$(strip $(subst \,/,$(WEBOTS_HOME))))
include $(WEBOTS_HOME_PATH)/resources/Makefile.include
