---
title: Coasterbot – Elektrische Systemsicht
markmap:
  colorFreezeLevel: 2
  initialExpandLevel: 3
---

# Coasterbot – Elektrische Systemsicht

## Energieversorgung
### Komponenten
- Akku
- Ladeelektronik
- DC/DC-Wandler
- Sicherungen
- Leistungsverteilung
- Hauptschalter
### Fähigkeiten
- Stabile Versorgungsspannungen bereitstellen
- Akku laden & überwachen
- Spannungswandlung
- Sichere Abschaltung
### Auflagen
- Verpolschutz
- Überstrom-/Überspannungsschutz
- Thermische Grenzen
- Not-Aus trennt Leistungspfad
- Akkubetrieb lt. Lastenheft > ML1 (Abgrenzung prüfen)

## Recheneinheit / Steuerelektronik
### Komponenten
- Rechenboard
- GPIO-/Bus-Interfaces
- Spannungsregler
- Watchdog
### Fähigkeiten
- Steuerungssoftware ausführen
- Sensor-Fusion
- Echtzeitreaktion
- Bus-Kommunikation
### Auflagen
- Versorgungsspannung & Toleranz
- Stromaufnahme
- Definiertes Boot-/Reset-Verhalten
- Temperaturbereich
- I/O-Pegelkompatibilität

## Sensorik
### Komponenten
- Hindernissensor
- Tischkanten-/Kliffsensor
- Odometrie (Encoder/IMU)
- Akku-Strom-/Spannungsmessung
### Fähigkeiten
- Abstand/Hindernis erfassen
- Kante detektieren
- Lage schätzen
- Akkuzustand messen
### Auflagen
- Speisespannung & Signaltyp (analog/digital/Bus)
- Messbereich & Abtastrate
- Pegelanpassung / Levelshift
- Störabstand
- Ausfallerkennung

## Aktorik / Antrieb
### Komponenten
- Antriebsmotoren
- Motortreiber (H-Brücke)
- ggf. Servo Untersetzer-Mechanik
### Fähigkeiten
- PWM-Ansteuerung
- Richtung / Drehzahl
- Strommessung / -begrenzung
- Kontrollierter Stopp
### Auflagen
- Nenn-/Anlaufstrom & -spannung
- Freilaufdioden
- PWM-Frequenz
- Treiber-Thermik
- Not-Aus trennt Antrieb

## Bedienelemente
### Komponenten
- Ein/Aus-Schalter
- Aktionsknopf
- Not-Aus-Taster
### Fähigkeiten
- POWER_ON / POWER_OFF
- COASTER_EVENT
- EMERGENCY_STOP
- Kurz-/Langdruck unterscheiden
### Auflagen
- Entprellung (HW/SW)
- Pull-up / Pull-down
- Definierte Pegel
- Not-Aus zwangsöffnend & fail-safe
- Wirkt hardwareseitig auf Leistungspfad

## Statusanzeige
### Komponenten
- Status-LED(s)
### Fähigkeiten
- Betriebszustand optisch anzeigen (MON-003)
### Auflagen
- Strombegrenzung / Vorwiderstand
- Pegel
- Sichtbarkeit

## Kommunikation & Verdrahtung
### Komponenten
- I2C / SPI / UART / GPIO
- Steckverbinder
- Kabelbaum
### Fähigkeiten
- Datenaustausch zwischen Komponenten
- Signalverteilung
### Auflagen
- Buspegel / Terminierung
- Leitungslänge
- Steckerkodierung
- Masseführung / Sternpunkt
- Schirmung

## Schutz & Sicherheit (elektrisch)
### Fähigkeiten
- Leistungspfad sicher trennen
- Fehlerfälle beherrschen
- Sichere Zustände einnehmen
### Auflagen
- Not-Aus zwangsöffnend / fail-safe
- Definierte Reaktionszeit
- Sicherungen & Verpol-/Überstrom-/Überspannungsschutz
- Thermischer Schutz

## Übergreifende Auflagen & Normen
### Auflagen
- EMV lt. Lastenheft außerhalb Projektumfang
- Betriebsbedingungen (Innenraum, ebene Fläche, Temperatur)
- Modularität & definierte Schnittstellen (NFA-ARC)
- Kippsicherheit / Schwerpunkt (NFA-REL-004)
