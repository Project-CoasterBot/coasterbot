# Alternativenbeschreibung

## Ziel und Vorgehensweise

Im Rahmen der Projektarbeit „Coasterbot – Entwicklung eines Prototyps“ wird ein handtellergroßer Roboter entwickelt, der sich selbstständig auf einem Tisch bewegen, Getränkeuntersetzer gezielt ausgeben und diese anschließend automatisiert wieder aufnehmen kann.

Für die Umsetzung des Prototyps bestehen für verschiedene Teilbereiche unterschiedliche technische Lösungsansätze. Im Rahmen der Alternativenbetrachtung werden diese Ansätze gegenübergestellt und hinsichtlich ihrer Eignung für das Projekt bewertet.

Die Betrachtung orientiert sich dabei an den funktionalen und technischen Anforderungen des Coasterbots. Berücksichtigt werden insbesondere die Bereiche Software bzw. Algorithmen, Recheneinheit, Sensorik, Aktorik, Energieversorgung und Mechanik.

Für jede betrachtete Funktion werden zunächst die zugrunde liegende Herausforderung und die mögliche Lösungsvariante beschrieben. Anschließend werden geeignete Alternativen aufgezeigt und hinsichtlich ihrer jeweiligen Vor- und Nachteile gegenübergestellt. Die letztendliche Auswahl einer Variante erfolgt unter Berücksichtigung der Anforderungen des Projekts, der Komplexität der Umsetzung sowie der Eignung für einen funktionsfähigen Prototyp.

# Systemarchitektur

Die Systemarchitektur des Coasterbots lässt sich grundsätzlich in die Bereiche Elektrik und Energieversorgung, Algorithmen und Mechanik unterteilen.

Auf Softwareseite umfasst das System vor allem die Recheneinheit sowie Algorithmen zur Navigation und Manipulationsplanung. Die Hardware umfasst unter anderem Sensorik zur Hindernis- und Tischkantenerkennung, Komponenten zur Positionsbestimmung, Antriebsmotoren sowie die Mechanik zur Aufnahme und Ablage der Untersetzer.

Eine mögliche Systemarchitektur sieht die Kombination einer Recheneinheit mit geeigneter Sensorik und Motorsteuerung vor. Die Auswahl der konkreten Komponenten und Verfahren wird in den folgenden Abschnitten anhand der jeweiligen Teilfunktionen betrachtet.

# Algorithmen

## Globale Fahrwegplanung

### Herausforderung

Der Coasterbot muss eine Zielposition auf dem Tisch erreichen. Dabei soll ausgehend von der aktuellen Position ein geeigneter globaler Fahrweg bestimmt werden. Als Grundlage kann dabei eine bekannte Tischkarte dienen.

### Mögliche Lösung: A*

Für die heuristische globale Pfadplanung kann der A*-Algorithmus eingesetzt werden. A* verbindet die bisher angefallenen Kosten eines Pfades mit einer Heuristik zur Abschätzung der noch verbleibenden Kosten. Dadurch kann gegenüber einer rein ungerichteten Suche gezielt ein geeigneter Weg zum Ziel bestimmt werden.

### Alternativen

**Dijkstra-Algorithmus**

Dijkstra stellt eine Alternative zur globalen Pfadplanung dar. Der Algorithmus bestimmt kürzeste Wege in einem gewichteten Graphen und kann als Referenz für eine optimale Pfadplanung dienen. Im Vergleich zu A* wird jedoch keine zielgerichtete Heuristik verwendet.

**Breitensuche (BFS)**

Die Breitensuche kann für ungewichtete Graphen eingesetzt werden. Sie ist konzeptionell einfach und eignet sich insbesondere dann, wenn alle Bewegungen dieselben Kosten besitzen.

### Bewertung

Für den Coasterbot ist zu bewerten, wie effizient der Algorithmus einen Weg zur Zielposition bestimmen kann und welche Anforderungen an die zugrunde liegende Karte gestellt werden.

Die Auswahl zwischen A*, Dijkstra und BFS erfolgt daher anhand der Eigenschaften der Tischkarte, der Gewichtung der Bewegungen und der Anforderungen an die Effizienz der Pfadplanung.

## Lokale Kollisionsvermeidung

### Herausforderung

Während der Fahrt können Hindernisse wie Gläser, Teller oder Personen den ursprünglich geplanten Fahrweg blockieren. Der Roboter muss daher kurzfristig auf Veränderungen seiner direkten Umgebung reagieren können.

### Mögliche Lösung: Dynamic Window Approach (DWA)

Der Dynamic Window Approach berücksichtigt die Bewegungsdynamik des Roboters und ermöglicht eine lokale Reaktion auf Hindernisse. Dadurch kann der Roboter auf kurzfristige Veränderungen der Umgebung reagieren, ohne den globalen Fahrweg vollständig neu berechnen zu müssen.

### Alternativen

**Potentialfelder**

Bei der Methode der Potentialfelder wird das Ziel als anziehendes und Hindernisse als abstoßendes Potential modelliert. Die Methode ist vergleichsweise einfach, kann jedoch lokale Minima aufweisen.

**Adaptive bzw. neuronale DWA**

Eine Erweiterung des DWA-Ansatzes kann insbesondere bei dynamischen Hindernissen eingesetzt werden. Dabei kann die lokale Fahrplanung an veränderte Umgebungsbedingungen angepasst werden.

### Bewertung

Bei der Auswahl sind die Reaktionsfähigkeit gegenüber Hindernissen, die Komplexität der Implementierung sowie das Verhalten bei dynamischen Hindernissen zu berücksichtigen.

## Dynamische Fahrplanung

### Herausforderung

Verändert sich die Umgebung während der Fahrt wiederholt, kann der ursprünglich berechnete globale Fahrweg ungültig werden. Der Roboter benötigt daher ein Verfahren, mit dem Änderungen der Umgebung bei der Pfadplanung berücksichtigt werden können.

### Mögliche Lösung: D* Lite

D* Lite ermöglicht eine inkrementelle Neuberechnung eines Pfades. Anstatt bei jeder Veränderung die gesamte Planung von Grund auf neu durchzuführen, können Änderungen gezielt in die bestehende Planung einbezogen werden.

### Alternativen

**D***

D* stellt einen alternativen Ansatz für die dynamische Pfadplanung dar und wurde ebenfalls für Umgebungen entwickelt, in denen sich die verfügbaren Wege verändern können.

**LPA***

LPA* (Lifelong Planning A*) stellt einen weiteren inkrementellen Ansatz zur Pfadplanung dar. Der Algorithmus ist insbesondere für Situationen geeignet, in denen wiederholt Änderungen am zugrunde liegenden Graphen auftreten.

### Bewertung

Die Verfahren unterscheiden sich hinsichtlich ihrer Vorgehensweise bei der inkrementellen Neuberechnung und ihrer Eignung für wiederholte Änderungen der Umgebung.

Für den Coasterbot ist zu prüfen, welches Verfahren bei den erwarteten Veränderungen der Tischumgebung ein angemessenes Verhältnis zwischen Planungsaufwand und Reaktionsfähigkeit bietet.

## Manipulationsplanung

### Herausforderung

Der Coasterbot muss Getränkeuntersetzer aufnehmen und gezielt wieder ablegen können. Dabei muss sowohl die gewünschte Greif- bzw. Aufnahmeposition als auch die spätere Ablageposition berücksichtigt werden.

### Mögliche Lösung: Inverse Kinematik und RRT*

Eine mögliche Lösung besteht in der Kombination aus inverser Kinematik und RRT*. Die inverse Kinematik kann dazu verwendet werden, aus einer gewünschten Zielpose geeignete Gelenkstellungen abzuleiten. RRT* kann anschließend für eine kollisionsfreie Bewegungsplanung eingesetzt werden.

### Alternativen

**Roboter mit Sauger**

Die Aufnahme des Untersetzers kann alternativ über einen Sauger erfolgen. Dabei wird der Untersetzer durch eine entsprechende Saugvorrichtung aufgenommen und anschließend an der gewünschten Position abgelegt.

**Roboter mit Greifer**

Eine weitere Möglichkeit ist die Verwendung eines mechanischen Greifers. Dieser kann den Untersetzer greifen und kontrolliert bewegen.

**Manuelle Einlage**

Als einfachste Alternative kann der Untersetzer manuell in eine dafür vorgesehene Aufnahme des Roboters eingelegt werden.

**Magnetischer Heber**

Falls geeignete magnetische Untersetzer verwendet werden, kann alternativ ein magnetischer Heber eingesetzt werden.

### Bewertung

Die Varianten unterscheiden sich hinsichtlich mechanischer Komplexität, Zuverlässigkeit bei der Aufnahme und Ablage sowie der Anforderungen an die Steuerung.

## Lokalisierung

### Herausforderung

Damit der Coasterbot seine Fahrbewegung planen und ausführen kann, muss seine aktuelle Position beziehungsweise Orientierung bestimmt werden.

### Mögliche Lösung: IMU, Encoder und Sensorfusion

Eine mögliche Lösung besteht aus der Kombination von IMU und Encoder-Daten. Durch eine Fusion können die Informationen der verschiedenen Sensoren kombiniert werden. Als mögliche Verfahren kommen beispielsweise der Extended Kalman Filter (EKF) oder der Unscented Kalman Filter (UKF) infrage.

### Alternativen

**IMU / INS**

Eine inertiale Messeinheit kann Beschleunigungen, Drehraten und Orientierung erfassen. Ein wesentlicher Nachteil besteht darin, dass sich Messfehler über die Zeit aufintegrieren und dadurch ein Drift entstehen kann.

**SLAM**

SLAM ermöglicht eine gleichzeitige Lokalisierung und Kartierung der Umgebung. Für den vorgesehenen Anwendungsfall ist dieser Ansatz dann relevant, wenn die Umgebung unbekannt ist und dauerhaft eine Karte erstellt bzw. aktualisiert werden soll.

**IMU + Encoder + LiDAR/Kamera**

Eine weitere Möglichkeit besteht in der Kombination von IMU und Encoder-Daten mit zusätzlicher visueller oder LiDAR-basierter Sensorik. Dadurch können zusätzliche Informationen zur Positionsbestimmung gewonnen werden.

### Bewertung

Da der Coasterbot innerhalb einer begrenzten Tischumgebung eingesetzt wird, ist zu untersuchen, welcher Lokalisierungsansatz den Anforderungen des Prototyps mit angemessenem technischen Aufwand gerecht wird.

# Mechanik

## Chassis

### Anforderungen

Das Chassis bildet die mechanische Grundlage des Coasterbots und muss die notwendigen Komponenten aufnehmen. Gleichzeitig soll der Roboter handtellergroß ausgeführt werden.

### Mögliche Varianten

**Quaderförmiges Chassis**

Ein quaderförmiger Aufbau ermöglicht eine vergleichsweise einfache Anordnung und Befestigung der Komponenten.

**Zylindrisches Chassis**

Ein zylindrischer Aufbau kann eine alternative Geometrie darstellen und insbesondere im Hinblick auf die Bewegungsrichtung und das Design des Roboters betrachtet werden.

### Materialalternativen

Für das Chassis beziehungsweise einzelne Gehäusekomponenten kommen unter anderem folgende Materialien infrage:

* Kunststoff
* Holz
* Metall
* Harz

Die Materialauswahl beeinflusst unter anderem Gewicht, Stabilität, Fertigungsaufwand und Möglichkeiten zur Integration der Komponenten.

### Modularität

Zur flexiblen Befestigung der Komponenten kann das Chassis modular aufgebaut werden. Als Möglichkeiten sind beispielsweise austauschbare Seitenwände oder zusätzliche Zwischenebenen vorgesehen.

## Untersetzer aufnehmen und ablegen

Die Mechanik zur Aufnahme und Ablage der Untersetzer stellt eine zentrale Funktion des Coasterbots dar.

Als mögliche Varianten sind vorgesehen:

* Roboter mit Sauger
* Roboter mit Greifer
* manuelle Einlage
* magnetischer Heber bei geeigneten Untersetzern
* Ausschieben des Untersetzers

Die Varianten sind hinsichtlich ihrer mechanischen Komplexität, Zuverlässigkeit und Eignung für die automatisierte Ausgabe und Aufnahme zu bewerten.

# Elektrik und Energieversorgung

## Recheneinheit

Für die Steuerung des Coasterbots kommen verschiedene Mikrocontroller bzw. Recheneinheiten infrage.

Als mögliche Varianten wurden identifiziert:

* Arduino Uno
* Raspberry Pi Pico 2
* ESP32

Die Auswahl ist anhand der benötigten Rechenleistung, Schnittstellen, verfügbaren Ressourcen und Eignung für die vorgesehenen Algorithmen zu treffen.

## 5.2 Energieversorgung

Für die Energieversorgung ist eine geeignete Spannungsversorgung der einzelnen Komponenten erforderlich.

Als mögliche Komponenten sind unter anderem vorgesehen:

* Akku
* Ladeelektronik
* DC/DC-Wandler
* Spannungsversorgung über einen 78x05-IC für eine Umsetzung von 6–18 V auf 5 V Vcc
* Messung von Akku-Strom und -Spannung

Der Betrieb mit einem aufladbaren Akkumulator ist gemäß Projektdefinition jedoch **Out of Scope**. Die konkrete Energieversorgungsvariante ist daher entsprechend der Projektabgrenzung festzulegen.

## Sensorik

### Hinderniserkennung

Für die Hinderniserkennung kommen verschiedene Sensorkonzepte infrage:

* Ultraschallsensor, beispielsweise HC-SR04
* visueller Sensor, beispielsweise Kamera OV7670
* Bumper-Sensor, beispielsweise SPDT Limit Switch
* Infrarot-Obstacle-Sensor

Die Varianten unterscheiden sich hinsichtlich Messprinzip, Reichweite, Komplexität und Eignung zur Erkennung von Hindernissen auf einem Tisch.

### Tischkantenerkennung

Zur Erkennung von Tischkanten können ebenfalls unterschiedliche Sensorkonzepte eingesetzt werden. Als mögliche Variante ist eine visuelle Erfassung vorgesehen.

### Positionsbestimmung

Für die Positionsbestimmung sind unter anderem folgende Ansätze vorgesehen:

* Odometrie über Encoder bzw. IMU
* IMU, beispielsweise MPU-6050 / GY-521
* visuelle Positionsbestimmung über eine Kamera

Die Auswahl ist im Zusammenhang mit dem vorgesehenen Lokalisierungsverfahren zu betrachten.

## Aktorik und Antrieb

Für den Antrieb des Coasterbots können TT-Motoren eingesetzt werden. Zur Ansteuerung der Motoren ist beispielsweise ein TB6612FNG Dual Motor Driver mit H-Brücke vorgesehen.

Für die Untersetzermechanik kann zusätzlich ein Micro-Servo-Motor eingesetzt werden.

Die konkrete Auswahl der Motoren und Motortreiber ist anhand des benötigten Drehmoments, der Baugröße, der Ansteuerbarkeit und der Anforderungen der Mechanik zu bewerten.

## Bedienelemente

Als Bedienelemente sind vorgesehen:

* Ein-/Aus-Schalter
* Aktionsknopf
* Not-Aus-Taster

Als mögliche Komponente für den Ein-/Aus-Schalter ist beispielsweise ein SPDT Slide Switch vorgesehen. Die übrigen Bedienelemente können über geeignete Taster realisiert und am Gehäuse montiert werden.

## Statusanzeige

Der Betriebszustand des Coasterbots soll für den Benutzer erkennbar sein.

Als mögliche Varianten sind vorgesehen:

**RGB-LED**

Eine RGB-LED ermöglicht die Darstellung verschiedener Zustände über unterschiedliche Farben.

**Mehrere diskrete LEDs**

Alternativ können mehrere einzelne LEDs hinter einer Blende angeordnet werden, wobei jeweils eine LED einen bestimmten Betriebszustand repräsentiert.

Die Varianten sind insbesondere hinsichtlich Informationsgehalt, Komplexität und Bedienbarkeit zu vergleichen.

# Zusammenfassende Bewertung

Die Alternativenbetrachtung zeigt, dass für die wesentlichen Funktionen des Coasterbots mehrere technische Lösungsansätze zur Verfügung stehen. Die Auswahl einer geeigneten Variante kann nicht isoliert für jede einzelne Komponente erfolgen, da zwischen den Bereichen Abhängigkeiten bestehen.

Die Auswahl der Recheneinheit, Sensorik und Aktorik beeinflusst die Umsetzung der Algorithmen zur Navigation und Lokalisierung. Ebenso muss die mechanische Umsetzung der Untersetzeraufnahme mit der vorgesehenen Steuerung und Aktorik abgestimmt werden.

Für die weitere Projektbearbeitung werden daher die identifizierten Alternativen anhand der für den Coasterbot relevanten Kriterien bewertet. Dazu zählen:

* Erfüllung der funktionalen Anforderungen
* technische Komplexität
* benötigte Rechenleistung
* mechanischer und elektrischer Aufwand
* Zuverlässigkeit
* Erweiterbarkeit
* Größe und Gewicht
* Fertigungsaufwand
* Eignung für die Umsetzung als Prototyp

Auf Grundlage dieser Bewertung wird für die einzelnen Teilbereiche jeweils eine geeignete Variante ausgewählt und anschließend in der weiteren Systementwicklung berücksichtigt.
