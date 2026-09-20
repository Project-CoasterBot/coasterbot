#include "webots_hal.h"
#include <cmath>
#include <limits>

// Device-Namen muessen exakt zu den "name"-Feldern im Coasterbot.proto passen.
WebotsHAL::WebotsHAL() {
    timeStep_ = static_cast<int>(robot_.getBasicTimeStep());

    // Ein Handle pro Seite. Im PROTO sind die zwei Motoren einer Seite
    // gekoppelt ("coupled motors", gemeinsamer Name vor "::"); ein Befehl an
    // "::front" treibt automatisch auch "::rear" derselben Seite.
    motorLeft_  = robot_.getMotor("wheel_left::front");
    motorRight_ = robot_.getMotor("wheel_right::front");

    // Position auf +unendlich -> Motor laeuft im Geschwindigkeitsmodus
    const double inf = std::numeric_limits<double>::infinity();
    motorLeft_->setPosition(inf);
    motorRight_->setPosition(inf);
    motorLeft_->setVelocity(0.0);
    motorRight_->setVelocity(0.0);

    encFL_ = robot_.getPositionSensor("wheel_front_left_sensor");
    encFR_ = robot_.getPositionSensor("wheel_front_right_sensor");
    encRL_ = robot_.getPositionSensor("wheel_rear_left_sensor");
    encRR_ = robot_.getPositionSensor("wheel_rear_right_sensor");
    encFL_->enable(timeStep_);
    encFR_->enable(timeStep_);
    encRL_->enable(timeStep_);
    encRR_->enable(timeStep_);

    ultrasonic_ = robot_.getDistanceSensor("ultrasonic");
    edgeFL_ = robot_.getDistanceSensor("edge_front_left");
    edgeFR_ = robot_.getDistanceSensor("edge_front_right");
    edgeRL_ = robot_.getDistanceSensor("edge_rear_left");
    edgeRR_ = robot_.getDistanceSensor("edge_rear_right");

    ultrasonic_->enable(timeStep_);
    edgeFL_->enable(timeStep_);
    edgeFR_->enable(timeStep_);
    edgeRL_->enable(timeStep_);
    edgeRR_->enable(timeStep_);

    imu_ = robot_.getInertialUnit("inertial_unit");
    imu_->enable(timeStep_);
    gyro_ = robot_.getGyro("gyro");
    gyro_->enable(timeStep_);

    // Start-Kalibrierung des simulierten GY-521/MPU-6050-Bias HIER im
    // Konstruktor, nicht erst beim ersten getGyroZ()-Aufruf: an dieser
    // Stelle wurde noch kein Fahrbefehl gegeben (Motoren stehen auf 0.0,
    // siehe oben), der Roboter steht also wirklich still - wie ein reales
    // Exemplar beim Einschalten, bevor es losfaehrt. Wuerde man stattdessen
    // erst waehrend der ersten Regelzyklen kalibrieren, in denen (z.B. in
    // MODE_SIM_TEST) bereits gefahren wird, wuerde die echte Drehung
    // faelschlich in den Bias-Schaetzwert einfliessen und ihn dauerhaft
    // verfaelschen (in der Simulation beobachtet: der Roboter blieb dann
    // an einer falschen Pose haengen).
    calibrateGyro();
}

WebotsHAL::~WebotsHAL() {}

void WebotsHAL::calibrateGyro() {
    float sum = 0.0f;
    int n = 0;
    for (; n < GYRO_CAL_SAMPLES; ++n) {
        if (robot_.step(timeStep_) == -1) break;  // Simulation endet bereits
        const double* v = gyro_->getValues();
        const float raw = static_cast<float>(v[1]);
        sum += raw + GYRO_BIAS_RAD + gyroNoise_(gyroRng_);
    }
    if (n > 0) gyroBiasEstimate_ = sum / static_cast<float>(n);
}

bool WebotsHAL::step() {
    return robot_.step(timeStep_) != -1;
}

void WebotsHAL::setLeftSpeed(float radPerSec) {
    // HAL-Konvention: positiv = vorwaerts (Front = -Z, die Seite mit dem
    // Ultraschallhalter). Bei positiver Motorgeschwindigkeit rollt das
    // Radmodell aus dem PROTO den Roboter jedoch nach +Z (hinten), daher
    // hier das Vorzeichen umdrehen. Treibt beide linken Raeder (gekoppelt).
    motorLeft_->setVelocity(-static_cast<double>(radPerSec));
}

void WebotsHAL::setRightSpeed(float radPerSec) {
    motorRight_->setVelocity(-static_cast<double>(radPerSec));
}

float WebotsHAL::getUltrasonicDistance() {
    // lookupTable im PROTO ist bereits auf Meter gemappt
    return static_cast<float>(ultrasonic_->getValue());
}

// Liest das digitale Kantensignal eines Sensors und bildet es auf das
// semantische bool ab. Die Coasterbot.proto-lookupTable liefert bereits nur
// 0 oder 1 (kein analoger Rohwert, siehe dortiger Kommentar) - die einzige
// verbleibende Aufgabe hier ist, ob HIGH oder LOW "Kante erkannt" bedeutet
// (EDGE_ACTIVE_HIGH). Diese Abbildung findet bewusst HIER statt, nicht in
// RobotLogic/SafetyMonitor: die Anwendungslogik interessiert nur "Kante
// erkannt oder nicht", nicht die Pin-Polaritaet des verbauten Sensormoduls
// (siehe robot_hal.h). Eine ArduinoHAL bildet ihren digitalRead() ebenso hier
// ab und muss dafuer nur EDGE_ACTIVE_HIGH auf ihr reales Sensormodul anpassen.
bool WebotsHAL::digitalEdge(webots::DistanceSensor* sensor) const {
    const bool high = sensor->getValue() > 0.5;
    return high == EDGE_ACTIVE_HIGH;
}

bool WebotsHAL::getEdgeFrontLeft()  { return digitalEdge(edgeFL_); }
bool WebotsHAL::getEdgeFrontRight() { return digitalEdge(edgeFR_); }
bool WebotsHAL::getEdgeRearLeft()   { return digitalEdge(edgeRL_); }
bool WebotsHAL::getEdgeRearRight()  { return digitalEdge(edgeRR_); }

float WebotsHAL::getYaw() {
    const double* rpy = imu_->getRollPitchYaw();
    return static_cast<float>(rpy[2]);
}

float WebotsHAL::getGyroZ() {
    // Das PROTO ist Y-up und wird in der Z-up-Welt mit rotation "1 0 0 1.5708"
    // aufgestellt -> die Welt-Hochachse faellt mit der lokalen +Y-Achse des
    // Gyros zusammen. Die Yaw-Rate ist daher die Y-Komponente (v[1]).
    const double* v = gyro_->getValues();
    const float raw = static_cast<float>(v[1]);

    // GY-521/MPU-6050-Simulation: simulierten Bias + deterministisches
    // Rauschen aufpraegen, dann den im Konstruktor (calibrateGyro())
    // ermittelten Bias-Schaetzwert wieder abziehen (siehe Kommentar dort
    // und in webots_hal.h).
    const float measured = raw + GYRO_BIAS_RAD + gyroNoise_(gyroRng_);
    return measured - gyroBiasEstimate_;
}

float WebotsHAL::getWheelAngle(WheelId wheel) {
    // Konvention: positiv = vorwaerts. Der PositionSensor dreht mit dem
    // (invertierten) Motor mit, daher hier ebenfalls negieren.
    //
    // Die reale Elektronik hat nur EINEN Encoder je Seite (siehe RobotHAL).
    // Das PROTO liefert in der Simulation trotzdem vier unabhaengige
    // PositionSensor (einen je Rad) - das ist der Rasteraufbau des CAD-
    // Modells, kein Hinweis auf die reale Sensorbestueckung. Um dieselbe
    // Information wie die reale Elektronik nach aussen zu geben, wird hier
    // je Seite gemittelt: das naehert die Drehung der (real gekoppelten)
    // Seitenwelle an und daempft nebenbei simulationsbedingtes Radschlupf-
    // Rauschen zwischen den zwei Sensoren derselben Seite.
    switch (wheel) {
        case WHEEL_LEFT:
            return -0.5f * (static_cast<float>(encFL_->getValue())
                           + static_cast<float>(encRL_->getValue()));
        case WHEEL_RIGHT:
            return -0.5f * (static_cast<float>(encFR_->getValue())
                           + static_cast<float>(encRR_->getValue()));
    }
    return 0.0f;
}

float WebotsHAL::getTime() {
    return static_cast<float>(robot_.getTime());
}
