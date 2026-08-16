---
title: Coasterbot – Algorithmen
markmap:
  colorFreezeLevel: 2
  initialExpandLevel: 3
---

# Coasterbot


## Elektrik

### Energieversorgung
- Für Prototypphase 9V Block
- DC/DC-Wandler 78x05

### Recheneinheit
- Arduino Uno Microcontroller

### Sensorik
- Hinderniserkennung
  - Ultraschall Abstandssensor HC-SR04
  - Visuell, z.B. Kamera OV7670
- Tischkantenerkennung
  - Infrarot Ebksqs Obstacle Sensor
- Positionsbestimmung und Navigation kombiniert
  - Odometrie (Encoder/IMU), z.B. MPU-6050 GY-521 6DOF
  - Visuell, z.B. Kamera OV7670

### Aktorik / Antrieb
- Antrieb für Prototyp TT Mororen
- Motortreiber (H-Brücke) TB6612FNG Dual Motor Driver
- Untersetzer-Mechanik über HOTUT Micro Servo Motor

### Bedienelemente
- Ein/Aus-Schalter SPDT Slide Switch SS-12F15
- Aktionsknopf Taster am Gehäuse
- Not-Aus-Taster Taster am Gehäuse

### Statusanzeige
- Status RGB-LED am Gehäuse




## Algorithmen

### Globaler Fahrweg

- Heuristische globale Pfadplanung nach A*

### Lokale Kollisionsvermeidung

- lokale Reaktion durch DWA

### Dynamische Fahrplanung

- Inkrementelle Neuberechnung mittels D* Lite

### Manipulationsplanung

- Inverse Kinematik, Zielpose → Gelenkstellungen

### Lokalisierung

- Sensorfusion Odometrie in Verbindung mit Kamera

## Mechanik

### Untersetzer aufnehmen
- Magnetischer Heber mit magnetischen Untersetzer

### Untersetzer ablegen
- Untersetzer ausschieben

### Chassis

- Austauschbare Seitenwände zur Komponentenbefestigung
- Quaderförmig
- Kunststoff

