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

#### Wo soll der Roboter fahren?
- Herausforderung: Zielposition einer Person erreichen, (bekannte) Tischkarte
- A* (Mögliche Lösung)
  - Heuristische globale Pfadplanung
- Alternativen
  - Dijkstra: Referenz/optimal
  - BFS: ungewichtete Graphen

### Lokale Kollisionsvermeidung

#### Wie reagiert der Roboter kurzfristig?
- Herausforderung: Gläser, Teller, Personen blockieren den Fahrtweg
- DWA
  - Bewegungsdynamik berücksichtigen → lokale Reaktion
- Alternativen
  - Potentialfelder: einfach, aber lokale Minima möglich
  - Adaptive / neuronale DWA: dynamische Hindernisse

### Dynamische Fahrplanung

#### Was passiert bei Veränderung der Umgebung?
- D* Lite
  - Inkrementelle Neuberechnung
- D*
  - Dynamische Pfadplanung
- LPA*
  - Wiederholte Änderungen

### Manipulationsplanung

#### Wie wird der Untersetzer aufgenommen und abgelegt?
- Inverse Kinematik
  - Zielpose → Gelenkstellungen
  - Greifposition, präzise Ablage
- RRT*
  - kollisionsfreie Bewegungsplanung
  - z.B. MoveIt2 als Integration

### Lokalisierung

#### Wo befindet sich der Roboter?
- Sensorfusion (EKF / UKF)
  - IMU + Encoder + LiDAR / Kamera
- IMU / INS
  - Beschleunigung, Drehrate / Orientierung → aber Drift
- SLAM
  - Nur bei unbekannter Umgebung erforderlich, falls eine dauerhafte Karte erforderlich ist

### Mögliche Systemarchitektur
- A* → Globaler Pfad
- DWA → Lokale Kollisionsvermeidung
- D*Lite → Dynamische Pfadplanung
- IKT + RRT → Manipulationsplanung
- IMU + Enc + Sensorfusion → Lokalisierung


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

