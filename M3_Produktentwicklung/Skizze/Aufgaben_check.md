# Pflichtenheft

Dieses Dokument soll eine Übersicht über die Komponenten des Coasterbots geben und welche Anteile des Lastenhefts jeweils mit dieser Komponente erfüllt werden. Im derzeitigen Konzept sind einige Anforderungen noch nicht abgedeckt. Diese sind in einem separaten Reiter abgelegt


# Sensoren

## Tischkantensensor 4x IR Infrared Obstacle Avoidance Sensor, Montage nach unten

* Der Bot muss den Tisch und dessen Ende erkennen.
 
## Hindernis Detektion 1x bis 3x nach vorne Ultraschall Sensor 

* Der Bot muss Hindernisse auf dem Tisch erkennen.
* Der Bot darf keine Personen berühren.
 
## Inertial system für navigation z.B. MPU-6050 Sensor
 
* Der Bot muss seine aktuelle Position auf dem Tisch bestimmen können. 
    * Teilweise möglich. Muss dafür die Umgebung vermessen, dann kann er es relativ bestimmen
* Der Bot muss seine Orientierung auf dem Tisch bestimmen können.
* Der Bot muss seinen Zielpunkt mit einer definierten Genauigkeit erreichen.




 
# Aktoren:

## Hauptantrieb 4x TT Motor 

* Der Bot muss sich selbstständig bewegen, um das Getränk nicht zu verschütten.
 
## Coaster Auswurf aus 1x Servo SG90
 
 
## Coaster Aufnahme über Arm mit 1x Servo SG90

* Der Bot muss einen Getränkeuntersetzer aufnehmen können.
* Der Bot muss einen Getränkeuntersetzer wieder aufnehmen können.



# Chassis:

* Der Bot muss einen Getränkeuntersetzer transportieren können.



# Logik mittels Microcontroller Platform (z.B. Arduino Uno)

* Der Bot muss Hindernissen selbstständig ausweichen.
* Der Bot muss auf dem Tisch bleiben, ohne von diesem zu fallen.
* Der Bot muss seine Fahrtroute dynamisch anpassen.
* Der Bot darf andere Gegenstände auf dem Tisch nicht verschieben.
* Der Bot muss bei Gefahr sofort anhalten können.
* Der Bot muss bei einem Sensorfehler in einen sicheren Zustand wechseln.
* Der Bot darf den Tisch nicht verlassen.
* Der Bot darf nicht mit hoher Geschwindigkeit in Hindernisse fahren.
* Der Bot muss Kollisionen vermeiden.
* Der Bot muss einen Not-Aus unterstützen.
* Der Bot muss Fehler erkennen.
* Der Bot muss Fehler protokollieren.
* Der Bot muss seinen Betriebszustand anzeigen.
* Der Bot muss fehlgeschlagene Aufgaben melden.



# Unklar

* Der Bot muss erkennen, ob ein Untersetzer aufgenommen wurde.



# Nicht erfüllte Anforderungen

* Der Bot muss selbstständig zum Kunden fahren.
    * Der Bot müsste dazu die Position der Kunden kennen
* Der Bot kann erkennen welche Getränke auf dem Tisch stehen.
* Der Bot muss einen Getränkeuntersetzer präzise unter einem Getränk platzieren.
    * Bisher keine Logik um Ziele zu erkennen
* Der Bot muss erkennen, ob sich ein Untersetzer erfolgreich unter dem Getränk befindet.
* Der Bot muss den Erfolg einer Aufgabe erkennen.

### Getränketransport (Maturity Level: 5)

* Der Bot muss Getränke sicher transportieren können.
* Der Bot muss den Schwerpunkt des Getränks berücksichtigen.
* Der Bot muss Beschleunigungen begrenzen.
* Der Bot muss Kurven mit angepasster Geschwindigkeit fahren.
* Der Bot muss anhalten können, ohne das Getränk zu verschütten.
* Der Bot muss erkennen, wenn ein Getränk verrutscht.
* Der Bot kann sich selbstständig unter einem Getränk platzieren.


### Mensch-Roboter-Interaktion (Maturity Level: 4)

* Der Bot muss einen Kunden erkennen.
* Der Bot muss erkennen, welcher Kunde ihn gerufen hat.
* Der Bot muss mehrere Kunden unterscheiden können.
* Der Bot muss Rückmeldungen über seinen Status geben.
* Der Bot muss dem Kunden den erfolgreichen Abschluss einer Aufgabe anzeigen.
* Der Bot muss den Empfang eines Rufsignals bestätigen.
* Der Bot kann vom Kunden zu seiner geografischen Lage gerufen werden.
* Der Bot kann vom Kunden zur geografischen Lage anderer Menschen gerufen werden.

### Bestellung und Bezahlung (Maturity Level: 3)

* Der Bot muss Bestellungen entgegennehmen.
* Der Bot muss Bestellungen digital erfassen.
* Der Bot muss Bestellungen bestätigen.
* Der Bot muss Bestellungen eindeutig einer Person oder einem Tisch zuordnen.
* Der Bot muss einen Bezahlvorgang starten können.
* Der Bot darf keine Zahlungsdaten dauerhaft speichern.
* Der Bot muss den Abschluss einer Zahlung erkennen.

### Reinigung (Maturity Level: 2)

* Der Bot muss verschüttete Flüssigkeiten lokalisieren.
* Der Bot muss den Reinigungsbereich bestimmen.
* Der Bot muss erkennen, ob die Reinigung erfolgreich war.
* Der Bot darf trockene Bereiche nicht unnötig reinigen.
* Der Bot muss erkennen, wenn sein Reinigungselement gesättigt ist.


### Energieversorgung (Maturity Level: 2)

* Der Bot muss seinen Akkuladestand überwachen.
* Der Bot muss einen niedrigen Akkuladestand erkennen.
* Der Bot muss seine Aufgabe kontrolliert abbrechen können.
* Der Bot muss selbstständig zu einer Ladestation fahren können (optional).
* Der Bot muss während des Betriebs seinen Energieverbrauch überwachen.




