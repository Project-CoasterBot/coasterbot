/*
 * ===================================================================
 *  Ansteuerung von 2x SG90 Servomotoren über die serielle Schnittstelle
 *  Board: Arduino Nano
 * ===================================================================
 *
 *  VERKABELUNG:
 *  -------------------------------------------------------------
 *  SG90 Servo 1:
 *    - Signal (orange)  -> Arduino Nano Pin D9
 *    - VCC    (rot)     -> Arduino Nano 5V (bei Bedarf externe 5V-Quelle nutzen)
 *    - GND    (braun)   -> Arduino Nano GND
 *
 *  SG90 Servo 2:
 *    - Signal (orange)  -> Arduino Nano Pin D10
 *    - VCC    (rot)     -> Arduino Nano 5V (bei Bedarf externe 5V-Quelle nutzen)
 *    - GND    (braun)   -> Arduino Nano GND
 *
 *  Hinweis: Werden beide Servos gleichzeitig stark belastet, empfiehlt
 *  sich eine externe 5V-Stromversorgung für die Servos (GND der externen
 *  Quelle unbedingt mit dem Arduino-GND verbinden = gemeinsame Masse!).
 *
 * -------------------------------------------------------------------
 *  BEDIENUNG ÜBER DEN SERIELLEN MONITOR (115200 Baud, "Newline" aktivieren):
 * -------------------------------------------------------------------
 *
 *  Eingabeformat:   <Kürzel><Leerzeichen><Winkel>
 *  Beispiele:
 *    S1 90        -> Servo 1 auf 90°
 *    S2 45        -> Servo 2 auf 45°
 *
 *  Die Kürzel "S1" und "S2" können unten in den globalen Variablen
 *  SERVO1_LABEL / SERVO2_LABEL frei angepasst werden (z.B. "A" und "B").
 *
 *  Ungültige Eingaben (falsches Kürzel, Winkel < 0 oder > 180, falsches
 *  Format) werden erkannt und mit einer Fehlermeldung quittiert, ohne
 *  dass der Servo bewegt wird.
 * ===================================================================
 */

#include <Arduino.h>
#include <Servo.h>

// --------------------------------------------------------------
// PIN-DEFINITIONEN
// --------------------------------------------------------------
const uint8_t SERVO1_PIN = 9;   // Signalleitung Servo 1 -> D9
const uint8_t SERVO2_PIN = 10;  // Signalleitung Servo 2 -> D10

// --------------------------------------------------------------
// GLOBALE VARIABLEN: Kürzel für die Servos
// Hier können die Bezeichner frei angepasst werden, z.B. "A"/"B"
// oder "LINKS"/"RECHTS" etc.
// --------------------------------------------------------------
String SERVO1_LABEL = "S1";
String SERVO2_LABEL = "S2";

// --------------------------------------------------------------
// Grenzwerte für den Servo-Winkel
// --------------------------------------------------------------
const int WINKEL_MIN = 0;
const int WINKEL_MAX = 180;

// Servo-Objekte
Servo servo1;
Servo servo2;

// Puffer für eingehende serielle Daten
String eingabePuffer = "";

// --------------------------------------------------------------
// Funktionsprototypen
// --------------------------------------------------------------
void verarbeiteEingabe(String zeile);
bool istGueltigeZahl(const String &text);

void setup() {
  Serial.begin(115200);

  // Servos an die definierten Pins binden
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  // Startposition (optional, hier Mittelstellung 90°)
  servo1.write(90);
  servo2.write(90);

  Serial.println(F("=== Servo-Steuerung bereit ==="));
  Serial.print(F("Servo 1 Kuerzel: "));
  Serial.println(SERVO1_LABEL);
  Serial.print(F("Servo 2 Kuerzel: "));
  Serial.println(SERVO2_LABEL);
  Serial.println(F("Eingabeformat:  <Kuerzel> <Winkel>   z.B.  S1 90"));
}

void loop() {
  // Serielle Daten zeichenweise einlesen, bis ein Zeilenumbruch kommt
  while (Serial.available() > 0) {
    char eingehendesZeichen = (char)Serial.read();

    if (eingehendesZeichen == '\n') {
      eingabePuffer.trim();  // Leerzeichen/'\r' am Rand entfernen
      if (eingabePuffer.length() > 0) {
        verarbeiteEingabe(eingabePuffer);
      }
      eingabePuffer = "";  // Puffer für nächste Eingabe leeren
    } else if (eingehendesZeichen != '\r') {
      eingabePuffer += eingehendesZeichen;
    }
  }
}

// --------------------------------------------------------------
// Verarbeitet eine vollständige Eingabezeile im Format
// "<Kuerzel> <Winkel>", prüft auf Gültigkeit und bewegt den
// entsprechenden Servo.
// --------------------------------------------------------------
void verarbeiteEingabe(String zeile) {
  // Position des ersten Leerzeichens suchen (Trenner zwischen Kürzel und Winkel)
  int trennerIndex = zeile.indexOf(' ');

  if (trennerIndex == -1) {
    Serial.println(F("FEHLER: Ungueltiges Format. Erwartet: <Kuerzel> <Winkel>  z.B. S1 90"));
    return;
  }

  String kuerzel = zeile.substring(0, trennerIndex);
  String winkelText = zeile.substring(trennerIndex + 1);
  winkelText.trim();

  // Prüfen, ob der Winkel-Teil eine gültige (ganze) Zahl ist
  if (!istGueltigeZahl(winkelText)) {
    Serial.print(F("FEHLER: '"));
    Serial.print(winkelText);
    Serial.println(F("' ist keine gueltige Zahl fuer den Winkel."));
    return;
  }

  int winkel = winkelText.toInt();

  // Winkel-Bereich prüfen (0-180 Grad, Servo-Limit von SG90)
  if (winkel < WINKEL_MIN || winkel > WINKEL_MAX) {
    Serial.print(F("FEHLER: Winkel "));
    Serial.print(winkel);
    Serial.print(F(" ausserhalb des gueltigen Bereichs ("));
    Serial.print(WINKEL_MIN);
    Serial.print(F("-"));
    Serial.print(WINKEL_MAX);
    Serial.println(F(" Grad)."));
    return;
  }

  // Passenden Servo anhand des Kürzels auswählen
  if (kuerzel.equalsIgnoreCase(SERVO1_LABEL)) {
    servo1.write(winkel);
    Serial.print(SERVO1_LABEL);
    Serial.print(F(" -> Winkel auf "));
    Serial.print(winkel);
    Serial.println(F(" Grad gesetzt."));
  } else if (kuerzel.equalsIgnoreCase(SERVO2_LABEL)) {
    servo2.write(winkel);
    Serial.print(SERVO2_LABEL);
    Serial.print(F(" -> Winkel auf "));
    Serial.print(winkel);
    Serial.println(F(" Grad gesetzt."));
  } else {
    Serial.print(F("FEHLER: Unbekanntes Servo-Kuerzel '"));
    Serial.print(kuerzel);
    Serial.print(F("'. Gueltige Kuerzel sind: "));
    Serial.print(SERVO1_LABEL);
    Serial.print(F(", "));
    Serial.println(SERVO2_LABEL);
  }
}

// --------------------------------------------------------------
// Prüft, ob ein String eine gültige ganze Zahl darstellt
// (optional mit führendem '-', nur Ziffern danach, nicht leer).
// --------------------------------------------------------------
bool istGueltigeZahl(const String &text) {
  if (text.length() == 0) {
    return false;
  }

  size_t startIndex = 0;
  if (text[0] == '-') {
    startIndex = 1;
    if (text.length() == 1) {
      return false;  // nur "-" ist keine Zahl
    }
  }

  for (size_t i = startIndex; i < text.length(); i++) {
    if (!isDigit(text[i])) {
      return false;
    }
  }

  return true;
}
