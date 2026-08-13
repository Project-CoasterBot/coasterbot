---
title: Coasterbot – Algorithmen
markmap:
  colorFreezeLevel: 2
  initialExpandLevel: 3
---

# Coasterbot


## Elektrik

### Energieversorgung
- Akku
- Ladeelektronik
- DC/DC-Wandler
  - 78x05 IC für 6..18V nach 5V Vcc

### Recheneinheit
- Microcontroller
  - Arduino Uno
  - Raspi Pico2
  - ESP32

### Sensorik
- Hinderniserkennung
  - Ultraschall Abstandssensor HC-SR04
  - Visuell, z.B. Kamera OV7670
  - Bumper Sensor, z.B. SPDT Limit Switch
- Tischkantenerkennung
  - Infrarot Ebksqs Obstacle Sensor
  - Visuell, z.B. Kamera OV7670
- Positionsbestimmung und Navigation
  - Odometrie (Encoder/IMU), z.B. MPU-6050 GY-521 6DOF
  - Visuell, z.B. Kamera OV7670
- Akku-Strom-/Spannungsmessung

### Aktorik / Antrieb
- Antriebsmotoren
  - TT Mororen
- Motortreiber (H-Brücke)
  - TB6612FNG Dual Motor Driver
- Untersetzer-Mechanik
  - HOTUT Micro Servo Motor

### Bedienelemente
- Ein/Aus-Schalter
  - SPDT Slide Switch SS-12F15
- Aktionsknopf
  - Taster zur Montage am Gehäuse
- Not-Aus-Taster
  - Taster zur Montage am Gehäuse

### Statusanzeige
- Status-LED(s)
  - RGB LED für verschiedene Statusmöglihckeiten
  - mehrere diskrete LEDs hinter Blende




## Algorithmen

### Globaler Fahrweg

- A* Heuristische globale Pfadplanung
- Dijkstra: Referenz/optimal
- BFS: ungewichtete Graphen

### Lokale Kollisionsvermeidung

- DWA
- Potentialfelder: einfach, aber lokale Minima möglich
- Adaptive / neuronale DWA: dynamische Hindernisse

### Dynamische Fahrplanung

- D* Lite (Inkrementelle Neuberechnung)
- D* (Dynamische Pfadplanung)
- LPA* (Wiederholte Änderungen)

### Manipulationsplanung

- Inverse Kinematik
- RRT* / MoveIt2

### Lokalisierung

- Sensorfusion Inertialsensor + LiDAR / Kamera
- Reines inertialsystem
- SLAM (Simultaneous Localization and Mapping)


## Mechanik

### Untersetzer aufnehmen
- Roboter mit Sauger
- Roboter mit Greifer
- Manuelle Einlage
- Magnetischer Heber mit magnetischen Untersetzer

### Untersetzer ablegen
- Untersetzer ausschieben
- Roboter mit Sauger
- Roboter mit Greifer

### Chassis

#### Modularität durch
- Austauschbare Seitenwände zur Komponentenbefestigung
- Zusätzliche Zwischenebenen zur Komponentenbefestigung

#### Form
- Quaderförmig
- Zylindrisch

#### Material
- Harz
- Metall
- Kunststoff
- Holz

