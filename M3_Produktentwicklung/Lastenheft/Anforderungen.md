# Anforderungen

| Maturity Level | Definition |
|----------------|------------|
| 1              | Prototyp vor Marktreife |
| 2              | MVP zur Markteinführung |
| 3              | Komfortfunktionen       |
| 4              | HMI Individualisierung  |
| 5              | Getränketransport       |



## Funktionale Anforderungen

### Navigation und Lokalisierung (Maturity Level: 1)

* Der Bot muss Hindernisse auf dem Tisch erkennen.
* Der Bot muss Hindernissen selbstständig ausweichen.
* Der Bot muss den Tisch und dessen Ende erkennen.
* Der Bot muss auf dem Tisch bleiben, ohne von diesem zu fallen.
* Der Bot muss seine Fahrtroute dynamisch anpassen.
* Der Bot muss seine aktuelle Position auf dem Tisch bestimmen können.
* Der Bot muss seine Orientierung auf dem Tisch bestimmen können.
* Der Bot muss seinen Zielpunkt mit einer definierten Genauigkeit erreichen.
* Der Bot muss sich selbstständig bewegen, um das Getränk nicht zu verschütten.
* Der Bot muss selbstständig zum Kunden fahren.
* Der Bot darf andere Gegenstände auf dem Tisch nicht verschieben.
* Der Bot darf keine Personen berühren.
* Der Bot kann erkennen welche Getränke auf dem Tisch stehen.

### Getränkeuntersetzer (Maturity Level: 1)

* Der Bot muss einen Getränkeuntersetzer aufnehmen können.
* Der Bot muss einen Getränkeuntersetzer transportieren können.
* Der Bot muss einen Getränkeuntersetzer präzise unter einem Getränk platzieren.
* Der Bot muss einen Getränkeuntersetzer wieder aufnehmen können.
* Der Bot muss erkennen, ob sich ein Untersetzer erfolgreich unter dem Getränk befindet.
* Der Bot muss erkennen, ob ein Untersetzer aufgenommen wurde.

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

### Sicherheit (Maturity Level: 1)

* Der Bot muss bei Gefahr sofort anhalten können.
* Der Bot muss bei einem Sensorfehler in einen sicheren Zustand wechseln.
* Der Bot darf den Tisch nicht verlassen.
* Der Bot darf nicht mit hoher Geschwindigkeit in Hindernisse fahren.
* Der Bot muss Kollisionen vermeiden.
* Der Bot muss einen Not-Aus unterstützen.

### Energieversorgung (Maturity Level: 2)

* Der Bot muss seinen Akkuladestand überwachen.
* Der Bot muss einen niedrigen Akkuladestand erkennen.
* Der Bot muss seine Aufgabe kontrolliert abbrechen können.
* Der Bot muss selbstständig zu einer Ladestation fahren können (optional).
* Der Bot muss während des Betriebs seinen Energieverbrauch überwachen.

### Systemüberwachung (Maturity Level: 1)

* Der Bot muss Fehler erkennen.
* Der Bot muss Fehler protokollieren.
* Der Bot muss seinen Betriebszustand anzeigen.
* Der Bot muss den Erfolg einer Aufgabe erkennen.
* Der Bot muss fehlgeschlagene Aufgaben melden.

## Nichtfunktionale Anforderungen

* Der Bot muss modular aufgebaut sein.
* Die Software muss komponentenbasiert entwickelt werden.
* Die Softwarearchitektur muss erweiterbar sein.
* Die Software muss simuliert werden können.
* Die Software muss testbar sein.
* Alle Kernfunktionen müssen automatisiert testbar sein.
* Sensoren und Aktoren müssen austauschbar sein.
* Die Software muss Hardwareabstraktion unterstützen.

## Anforderungen für die Simulation (Definition of Done)

* Alle Fahrmanöver müssen in der Simulation reproduzierbar sein.
* Sensoren können in der Simulation modelliert werden.
* Aktoren können in der Simulation modelliert werden.
* Hindernisse müssen in der Simulation berücksichtigt werden.
* Fehlersituationen können simuliert werden können.
* Die Simulation soll alle identifizierten Testfälle unterstützen.
* Das Verhalten der Simulation soll mit dem erwarteten Verhalten des Bots übereinstimmen.
