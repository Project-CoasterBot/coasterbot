# Einleitung

## Ausgangssituation

Die Gastronomie steht seit mehreren Jahren vor erheblichen personellen Herausforderungen. Insbesondere der anhaltende Fachkräftemangel erschwert den wirtschaftlichen Betrieb vieler Restaurants, Bars und Cafés. Mitarbeitende müssen neben der eigentlichen Betreuung der Gäste zahlreiche wiederkehrende Tätigkeiten übernehmen, beispielsweise das Verteilen und Einsammeln von Getränkeuntersetzern. Obwohl diese Aufgaben keinen hohen fachlichen Anspruch besitzen, binden sie Arbeitszeit und reduzieren die verfügbare Zeit für den direkten Gästeservice.

Parallel dazu gewinnen autonome mobile Robotersysteme zunehmend an Bedeutung. Fortschritte in den Bereichen Sensorik, eingebettete Systeme und Machine Learning ermöglichen den Einsatz kleiner autonomer Roboter auch in dynamischen Umgebungen. Während bereits Serviceroboter für den Transport von Speisen oder Getränken existieren, existieren bislang nur wenige spezialisierte Systeme für kleinere Serviceaufgaben auf dem Tisch.

Vor diesem Hintergrund soll ein kompakter autonomer Roboter entwickelt werden, der Getränkeuntersetzer selbstständig zu Gästen bringt, unter Getränken platziert und nach Gebrauch wieder aufnimmt. Der entwickelte Prototyp dient als Machbarkeitsnachweis für dieses Konzept.

## Projektziel

Ziel dieses Projekts ist die Entwicklung eines Prototyp, nämlich des *Coasterbots*. Dabei handelt es sich um einen ca. handtellergroßen autonomen mobilen Roboter, der sich auf einer Tischoberfläche selbstständig bewegt und Getränkeuntersetzer automatisiert verteilt sowie wieder einsammelt.

Im Rahmen der Projektarbeit liegt der Schwerpunkt auf der Entwicklung einer geeigneten Systemarchitektur sowie der Konzeption der erforderlichen Softwarekomponenten. Zusätzlich werden die mechanischen und elektronischen Komponenten des Prototyps so ausgearbeitet, dass eine Fertigung grundsätzlich möglich ist. Zur Absicherung des Systementwurfs wird eine Simulation entwickelt, mit der die wesentlichen Funktionen des Roboters reproduzierbar getestet und validiert werden können.

Der Fokus der Arbeit liegt nicht auf der Entwicklung eines serienreifen Produkts, sondern auf dem Nachweis der technischen Realisierbarkeit der Kernfunktionen.

## Projektumfang

Der Projektumfang umfasst die Entwicklung eines Prototyps mit den für den Funktionsnachweis erforderlichen Eigenschaften. Hierzu gehören insbesondere:

* Konzeption der Sensorik und Aktorik.
* Erstellung der mechanischen Konstruktion einschließlich der erforderlichen STL-Dateien.
* Konzeption der Navigations- und Lokalisierungsfunktionen.
* Entwicklung einer modularen Softwarearchitektur.
* Entwicklung der Softwarekomponenten zur Steuerung des Roboters.
* Erstellung der elektronischen Schaltpläne.
* Entwicklung einer Simulationsumgebung zur Validierung der Software.
* Identifikation und Definition der aus den Anforderungen resultierenden Testfälle.

Die funktionalen Anforderungen konzentrieren sich auf die Kernfunktionen der autonomen Navigation, der Handhabung von Getränkeuntersetzern, der Systemsicherheit sowie der Systemüberwachung. Erweiterte Funktionen werden als zukünftige Ausbaustufen betrachtet.

## Projektabgrenzung

Nicht Bestandteil dieses Projekts sind Funktionen, die über den Nachweis der grundlegenden technischen Machbarkeit hinausgehen. Insbesondere sind folgende Themen nicht Gegenstand der Projektarbeit:

* Betrachtungen zur elektromagnetischen Verträglichkeit (EMV).
* Kartierung oder dauerhafte Modellierung der Umgebung.
* Einbindung des Systems in Netzwerke oder Cloud-Dienste.
* Marketing- und Wirtschaftlichkeitskonzepte.
* Koordination mehrerer Coasterbots (Schwarmverhalten).
* Vollständige Automatisierung von Bestell- oder Bezahlvorgängen.
* Automatisierte Tischreinigung.
* Getränketransport als Primärfunktion des Systems.

Die genannten Funktionen stellen mögliche Erweiterungen zukünftiger Entwicklungsstufen dar und werden im Rahmen dieser Projektarbeit nicht umgesetzt.

## Zielgruppe des Lastenhefts

Dieses Lastenheft beschreibt die Anforderungen an den zu entwickelnden Coasterbot aus Sicht des Auftraggebers. Es dient als Grundlage für die Systementwicklung sowie für die Erstellung des Pflichtenhefts, der Systemarchitektur, der Simulation und der späteren Testfälle. Darüber hinaus bildet es die Basis für die Nachverfolgbarkeit der Anforderungen während des gesamten Entwicklungsprozesses.

# Systemübersicht

## Systembeschreibung

Der Coasterbot ist ein autonomes, mobiles Robotersystem, das auf Tischoberflächen eingesetzt wird. Seine Hauptaufgabe besteht darin, Getränkeuntersetzer selbstständig zu Gästen zu transportieren, diese präzise unter Getränken zu platzieren und nach Gebrauch wieder aufzunehmen. Der Bot bewegt sich dabei eigenständig innerhalb des ihm zur Verfügung stehenden Arbeitsbereichs und erkennt Hindernisse sowie die Begrenzungen der Tischoberfläche.

Der Coasterbot ist als kompakter Prototyp konzipiert und dient dem Nachweis der technischen Machbarkeit des beschriebenen Anwendungsfalls. Die Entwicklung umfasst sowohl die mechanische Konstruktion als auch die Elektronik sowie die Softwarearchitektur einschließlich einer Simulationsumgebung zur Validierung des Systemverhaltens.

## Systemkontext

Der Coasterbot wird in gastronomischen Einrichtungen wie Restaurants, Cafés oder Bars eingesetzt. Während des Betriebs befindet sich der Roboter ausschließlich auf einer Tischoberfläche und interagiert mit Gästen sowie mit den auf dem Tisch befindlichen Objekten.

Die Systemgrenze umfasst sämtliche Hard- und Softwarekomponenten des Roboters. Nicht Bestandteil des Systems sind externe Informationssysteme wie Kassensysteme, Warenwirtschaftssysteme oder Cloud-Dienste. Ebenso gehören Personen und die physische Umgebung nicht zum System, sondern stellen externe Akteure beziehungsweise Randbedingungen dar.

Die Interaktion des Systems erfolgt mit folgenden externen Entitäten:

* Gäste, die Getränkeuntersetzer anfordern oder entgegennehmen.
* Servicepersonal, das den Bot startet, überwacht oder wartet.
* Getränke und Getränkeuntersetzer als zu handhabende Objekte.
* Tischoberfläche als Arbeitsbereich des Roboters.
* Hindernisse wie Gläser, Teller oder Dekorationselemente.

## Einsatzbereich

Der Coasterbot ist ausschließlich für den Betrieb auf ebenen Tischoberflächen in Innenräumen vorgesehen. Der Einsatz erfolgt in einer kontrollierten Umgebung mit ausreichend Beleuchtung und einer für gastronomische Einrichtungen üblichen Anordnung von Getränken und Gegenständen.

Der Roboter ist nicht für den Einsatz auf unebenen Flächen, im Außenbereich oder unter extremen Umgebungsbedingungen vorgesehen. Ebenso ist kein Betrieb auf mehreren Tischen oder ein Wechsel zwischen unterschiedlichen Tischen vorgesehen.

## Anwender

Das System besitzt unterschiedliche Nutzergruppen mit verschiedenen Aufgaben.

### Gäste

Gäste interagieren indirekt mit dem Coasterbot, indem sie Getränkeuntersetzer anfordern oder entgegennehmen. Darüber hinaus erhalten sie Statusinformationen über den aktuellen Arbeitszustand des Roboters.

### Servicepersonal

Das Servicepersonal nimmt den Roboter in Betrieb, überwacht dessen Funktion, stellt Getränkeuntersetzer bereit und führt Wartungs- oder Ladevorgänge durch. Zusätzlich kann das Personal Fehlermeldungen auswerten und den Roboter bei Bedarf außer Betrieb nehmen.

### Entwickler

Entwickler nutzen die Simulationsumgebung zur Validierung der Softwarearchitektur sowie zur Durchführung von Tests. Außerdem dienen die bereitgestellten Schaltpläne und Konstruktionsdaten der Weiterentwicklung des Systems.

## Betriebsbedingungen

Für einen ordnungsgemäßen Betrieb müssen folgende Rahmenbedingungen erfüllt sein:

* Der Roboter befindet sich auf einer ausreichend großen und ebenen Tischoberfläche.
* Die Tischkante ist für die eingesetzte Sensorik erkennbar.
* Getränkeuntersetzer besitzen definierte Abmessungen und Materialeigenschaften.
* Die Beleuchtungsverhältnisse ermöglichen eine zuverlässige Erkennung von Objekten.
* Die Batterie besitzt einen ausreichenden Ladezustand.
* Sensoren und Aktoren sind betriebsbereit.

Werden diese Bedingungen nicht erfüllt, kann die Funktion des Systems eingeschränkt sein oder der Roboter muss in einen sicheren Betriebszustand wechseln.

## Annahmen und Randbedingungen

Für die Entwicklung des Prototyps werden folgende Annahmen getroffen:

* Es wird ausschließlich ein einzelner Coasterbot betrachtet.
* Eine dauerhafte Kartierung der Umgebung erfolgt nicht.
* Eine Kommunikation mit externen Netzwerken oder Cloud-Diensten findet nicht statt.
* Getränkeuntersetzer besitzen ein standardisiertes Format.
* Die Tischoberfläche ist frei von größeren Unebenheiten.
* Hindernisse können während der Fahrt ihre Position verändern.
* Personen können sich jederzeit im unmittelbaren Arbeitsbereich des Roboters befinden.
* Die Entwicklung konzentriert sich auf die Kernfunktion des automatisierten Handlings von Getränkeuntersetzern.

## Ausbaustufen

Zur strukturierten Weiterentwicklung des Systems werden verschiedene Reifegrade definiert.

**Maturity Level 1 – Prototyp**

Nachweis der autonomen Navigation, Lokalisierung, sicheren Bewegung sowie der Aufnahme und Ausgabe von Getränkeuntersetzern.

**Maturity Level 2 – Erweiterter Funktionsumfang**

Erweiterung um Funktionen zur Energieverwaltung sowie zur automatisierten Tischreinigung.

**Maturity Level 3 – Servicefunktionen**

Integration von Funktionen zur digitalen Bestellaufnahme und Unterstützung von Bezahlvorgängen.

**Maturity Level 4 – Mensch-Roboter-Interaktion**

Erweiterung der Interaktion zwischen Gästen und Roboter, beispielsweise durch personenbezogene Zuordnung von Anfragen oder individuelle Statuskommunikation.

**Maturity Level 5 – Getränketransport**

Erweiterung des Systems um den sicheren autonomen Transport kompletter Getränke unter Berücksichtigung der fahrdynamischen Anforderungen.

# Funktionale Anforderungen

Dieses Kapitel beschreibt die funktionalen Anforderungen an den Coasterbot. Die Anforderungen definieren das vom System erwartete Verhalten, ohne eine konkrete technische Umsetzung vorzugeben. Jede Anforderung erhält eine eindeutige Kennung, um eine spätere Rückverfolgbarkeit zu Architektur, Implementierung und Testfällen sicherzustellen.

Da Gegenstand dieser Projektarbeit die Entwicklung eines Prototyps ist, beziehen sich die verpflichtenden Anforderungen ausschließlich auf den **Maturity Level 1**. Anforderungen höherer Reifegrade werden als zukünftige Erweiterungen betrachtet.

## Navigation und Lokalisierung

| ID      | Anforderung                                                                                                  | Priorität               |
| ------- |:------------------------------------------------------------------------------------------------------------ |:------------------------|
| NAV-001 | Der Coasterbot muss Hindernisse auf der Tischoberfläche erkennen.                                            | Muss                    |
| NAV-002 | Der Coasterbot muss Hindernissen selbstständig ausweichen.                                                   | Muss                    |
| NAV-003 | Der Coasterbot muss die Begrenzung der Tischoberfläche erkennen.                                             | Muss                    |
| NAV-004 | Der Coasterbot darf die Tischoberfläche nicht verlassen.                                                     | Muss                    |
| NAV-005 | Der Coasterbot muss seine Fahrtroute während der Bewegung an erkannte Hindernisse anpassen können.           | Muss                    |
| NAV-006 | Der Coasterbot muss seine aktuelle Position innerhalb des Arbeitsbereichs bestimmen können.                  | Muss                    |
| NAV-007 | Der Coasterbot muss seine Orientierung bestimmen können.                                                     | Muss                    |
| NAV-008 | Der Coasterbot muss einen vorgegebenen Zielpunkt innerhalb einer definierten Positionsgenauigkeit erreichen. | Muss                    |
| NAV-009 | Der Coasterbot muss sich autonom auf der Tischoberfläche bewegen können.                                     | Muss                    |
| NAV-010 | Der Coasterbot darf Gegenstände auf der Tischoberfläche nicht unbeabsichtigt verschieben.                    | Muss                    |
| NAV-011 | Der Coasterbot darf Personen während des Betriebs nicht berühren.                                            | Muss                    |
| NAV-012 | Der Coasterbot soll Getränke auf der Tischoberfläche erkennen können.                                        | Soll                    |

## Handhabung von Getränkeuntersetzern

| ID      | Anforderung                                                                                                  | Priorität               |
| ------- |:------------------------------------------------------------------------------------------------------------ |:------------------------|
| CST-001 | Der Coasterbot muss einen Getränkeuntersetzer aufnehmen können.                                              | Muss                    |
| CST-002 | Der Coasterbot muss einen Getränkeuntersetzer transportieren können.                                         | Muss                    |
| CST-003 | Der Coasterbot muss einen Getränkeuntersetzer präzise unter einem Getränk platzieren können.                 | Muss                    |
| CST-004 | Der Coasterbot muss einen Getränkeuntersetzer wieder aufnehmen können.                                       | Muss                    |
| CST-005 | Der Coasterbot muss erkennen können, ob sich ein Getränkeuntersetzer erfolgreich unter dem Getränk befindet. | Muss                    |
| CST-006 | Der Coasterbot muss erkennen können, ob ein Getränkeuntersetzer erfolgreich aufgenommen wurde.               | Muss                    |

## Sicherheit

| ID      | Anforderung                                                                                                      | Priorität           |
| ------- |:------------------------------------------------------------------------------------------------------------ |:------------------------|
| SAF-001 | Der Coasterbot muss bei Gefahr unverzüglich anhalten können.                                                     | Muss                |
| SAF-002 | Der Coasterbot muss bei einem Ausfall sicherheitsrelevanter Sensoren in einen sicheren Betriebszustand wechseln. | Muss                |
| SAF-003 | Der Coasterbot muss Kollisionen mit Hindernissen vermeiden.                                                      | Muss                |
| SAF-004 | Der Coasterbot darf Hindernisse nicht mit einer Geschwindigkeit anfahren, die Schäden verursachen kann.          | Muss                |
| SAF-005 | Der Coasterbot muss einen Not-Aus unterstützen.                                                                  | Muss                |

## Systemüberwachung

| ID      | Anforderung                                                             | Priorität               |
| ------- |:----------------------------------------------------------------------- |:------------------------|
| MON-001 | Der Coasterbot muss auftretende Fehler erkennen.                        | Muss                    |
| MON-002 | Der Coasterbot muss Fehler protokollieren.                              | Muss                    |
| MON-003 | Der Coasterbot muss seinen aktuellen Betriebszustand anzeigen.          | Muss                    |
| MON-004 | Der Coasterbot muss den erfolgreichen Abschluss einer Aufgabe erkennen. | Muss                    |
| MON-005 | Der Coasterbot muss fehlgeschlagene Aufgaben erkennen und melden.       | Muss                    |

## Erweiterungsmöglichkeiten

Die folgenden Funktionen sind nicht Bestandteil des im Rahmen dieser Projektarbeit zu entwickelnden Prototyps. Sie dienen als Grundlage für zukünftige Entwicklungsstufen.

### Maturity Level 2

* Überwachung des Akkuladestands
* Automatisierte Rückkehr zur Ladestation
* Lokalisierung verschütteter Flüssigkeiten
* Automatisierte Tischreinigung

### Maturity Level 3

* Digitale Bestellaufnahme
* Verwaltung von Bestellungen
* Unterstützung von Bezahlvorgängen

### Maturity Level 4

* Erweiterte Mensch-Roboter-Interaktion
* Erkennung einzelner Gäste
* Personalisierte Kommunikation

### Maturity Level 5

* Vollständiger Getränketransport
* Dynamische Stabilisierung transportierter Getränke
* Fahrdynamische Anpassung an den Schwerpunkt des Getränks

# Nichtfunktionale Anforderungen

Dieses Kapitel beschreibt die nichtfunktionalen Anforderungen an den Coasterbot. Im Gegensatz zu den funktionalen Anforderungen definieren diese nicht, **welche** Aufgaben das System erfüllt, sondern **welche Qualitätsmerkmale** bei der Entwicklung und dem Betrieb des Systems eingehalten werden müssen. Die Anforderungen bilden die Grundlage für eine wartbare, erweiterbare und testbare Software- und Systemarchitektur.

## Softwarearchitektur

Die Software des Coasterbots muss so entwickelt werden, dass sie zukünftige Erweiterungen unterstützt und eine klare Trennung der einzelnen Systemfunktionen ermöglicht.

| ID          | Anforderung                                                                                              | Priorität               |
| ------- |:------------------------------------------------------------------------------------------------------------ |:------------------------|
| NFA-ARC-001 | Die Software muss komponentenbasiert aufgebaut sein.                                                     | Muss                    |
| NFA-ARC-002 | Die Softwarearchitektur muss modular aufgebaut sein.                                                     | Muss                    |
| NFA-ARC-003 | Die Softwarearchitektur muss um zusätzliche Funktionen erweiterbar sein, ohne bestehende Kernkomponenten grundlegend ändern zu müssen.               | Muss      |
| NFA-ARC-004 | Komponenten müssen über eindeutig definierte Schnittstellen miteinander kommunizieren.                                                               | Muss      |
| NFA-ARC-005 | Die Software muss eine Hardwareabstraktion bereitstellen, sodass Sensoren und Aktoren unabhängig von der Anwendungslogik ausgetauscht werden können. | Muss      |

## Wartbarkeit und Erweiterbarkeit

Da der Coasterbot als Prototyp für zukünftige Entwicklungsstufen dient, muss seine Architektur eine einfache Wartung und Erweiterung ermöglichen.

| ID          | Anforderung                                                                               | Priorität               |
| ------- |:--------------------------------------------------------------------------------------------- |:------------------------|
| NFA-MAI-001 | Sensoren müssen ohne Änderungen an der Anwendungslogik austauschbar sein.                 | Muss                    |
| NFA-MAI-002 | Aktoren müssen ohne Änderungen an der Anwendungslogik austauschbar sein.                  | Muss                    |
| NFA-MAI-003 | Neue Softwarekomponenten müssen über definierte Schnittstellen integriert werden können.  | Muss                    |
| NFA-MAI-004 | Einzelne Komponenten müssen unabhängig voneinander entwickelt und getestet werden können. | Muss                    |

## Testbarkeit

Die entwickelte Software muss so ausgelegt werden, dass ihre Funktionen systematisch überprüft und validiert werden können.

| ID          | Anforderung                                                               | Priorität              |
| ------- |:----------------------------------------------------------------------------- |:-----------------------|
| NFA-TST-001 | Kernfunktionen müssen automatisiert testbar sein.                    | Muss                        |
| NFA-TST-002 | Softwarekomponenten müssen unabhängig voneinander getestet werden können. | Muss                   |
| NFA-TST-003 | Fehlerzustände müssen gezielt erzeugt und überprüft werden können.        | Muss                   |
| NFA-TST-004 | Die Ergebnisse automatisierter Tests müssen reproduzierbar sein.          | Muss                   |

## Simulationsfähigkeit

Da die Simulation Bestandteil der Projektarbeit ist, muss die Software unabhängig von der Zielhardware ausgeführt werden können.

| ID          | Anforderung                                                                              | Priorität               |
| ------- |:-------------------------------------------------------------------------------------------- |:------------------------|
| NFA-SIM-001 | Die Software muss in einer Simulationsumgebung ausführbar sein.                          | Muss                    |
| NFA-SIM-002 | Sensoren müssen innerhalb der Simulation modelliert werden können.                       | Muss                    |
| NFA-SIM-003 | Aktoren müssen innerhalb der Simulation modelliert werden können.                        | Muss                    |
| NFA-SIM-004 | Simulationskomponenten müssen dieselben Schnittstellen wie die reale Hardware verwenden. | Muss                    |
| NFA-SIM-005 | Die Simulationsumgebung muss reproduzierbare Ergebnisse liefern.                         | Muss                    |

## Zuverlässigkeit

Der Coasterbot muss auch bei Fehlern einen sicheren und kontrollierten Betrieb gewährleisten.

| ID          | Anforderung                                                                                          | Priorität               |
| ------- |:-------------------------------------------------------------------------------------------------------- |:------------------------|
| NFA-REL-001 | Das System muss Fehler erkennen und protokollieren können.                                                         | Muss      |
| NFA-REL-002 | Das System muss nach erkannten Fehlern in einen sicheren Betriebszustand wechseln.                                 | Muss      |
| NFA-REL-003 | Ein Ausfall einzelner Softwarekomponenten darf nicht zu einem unkontrollierten Verhalten des Gesamtsystems führen. | Muss      |

## Dokumentation

Für die Nachvollziehbarkeit der Entwicklung müssen alle wesentlichen Artefakte dokumentiert werden.

| ID          | Anforderung                                                                                 | Priorität               |
| ------- |:----------------------------------------------------------------------------------------------- |:------------------------|
| NFA-DOC-001 | Die Softwarearchitektur muss dokumentiert werden.                                           | Muss                    |
| NFA-DOC-002 | Die Schnittstellen zwischen den Softwarekomponenten müssen dokumentiert werden.             | Muss                    |
| NFA-DOC-003 | Die mechanische Konstruktion und die elektronischen Schaltpläne müssen dokumentiert werden. | Muss                    |
| NFA-DOC-004 | Für alle identifizierten Anforderungen müssen geeignete Testfälle dokumentiert werden.      | Muss                    |

## Qualitätsziele

Die Entwicklung des Coasterbots verfolgt die folgenden Qualitätsziele:

* **Modularität:** Das System soll aus klar abgegrenzten Komponenten bestehen, die unabhängig voneinander entwickelt und erweitert werden können.
* **Erweiterbarkeit:** Neue Funktionen sollen mit möglichst geringem Anpassungsaufwand integriert werden können.
* **Wartbarkeit:** Änderungen und Fehlerbehebungen sollen durch die strukturierte Architektur vereinfacht werden.
* **Testbarkeit:** Kernfunktionen sollen automatisiert überprüfbar und validierbar sein.
* **Wiederverwendbarkeit:** Einzelne Softwarekomponenten sollen auch in zukünftigen Entwicklungsstufen eingesetzt werden können.
* **Simulierbarkeit:** Die Software soll unabhängig von der realen Hardware entwickelt und validiert werden können.
* **Zuverlässigkeit:** Das System soll Fehler erkennen und auf diese kontrolliert reagieren, um einen sicheren Betrieb zu gewährleisten.

# Systemgrenzen und Schnittstellen

Dieses Kapitel beschreibt die Systemgrenzen des Coasterbots sowie die Schnittstellen zu seiner Umgebung. Ziel ist es, den Verantwortungsbereich des Systems eindeutig festzulegen und die Interaktionen mit externen Akteuren und Komponenten zu definieren.

## Systemgrenzen

Der Coasterbot umfasst sämtliche Hard- und Softwarekomponenten, die zur autonomen Durchführung seiner Kernaufgabe erforderlich sind. Dazu zählen insbesondere:

* die mechanische Konstruktion des Roboters,
* die elektronischen Komponenten einschließlich Sensorik und Aktorik,
* die Energieversorgung,
* die Steuerungssoftware,
* die Software zur Navigation und Lokalisierung,
* die Software zur Handhabung von Getränkeuntersetzern,
* die Simulationsumgebung zur Validierung der Systemfunktionen.

Nicht Bestandteil des Systems sind externe IT-Systeme, Cloud-Dienste oder die Infrastruktur gastronomischer Betriebe. Ebenso gehören Gäste, Servicepersonal und die physische Umgebung nicht zum System.

## Externe Akteure

Während des Betriebs interagiert der Coasterbot mit verschiedenen externen Akteuren.

### Gäste

Gäste nehmen Getränkeuntersetzer entgegen und können mit dem Roboter interagieren, beispielsweise indem sie ihn anfordern oder Statusinformationen wahrnehmen. Gäste stellen keine direkten Steuerbefehle an die internen Systemkomponenten.

### Servicepersonal

Das Servicepersonal ist für die Inbetriebnahme, das Bereitstellen von Getränkeuntersetzern, das Laden des Akkus sowie Wartungsarbeiten verantwortlich. Darüber hinaus kann das Personal Fehlermeldungen auswerten und den Roboter außer Betrieb nehmen.

### Entwicklungs- und Testpersonal

Entwickler und Tester verwenden die Simulationsumgebung zur Validierung der Software sowie zur Durchführung definierter Testfälle.

## Physische Schnittstellen

Der Coasterbot besitzt Schnittstellen zu seiner physischen Umgebung.

### Tischoberfläche

Die Tischoberfläche bildet den Arbeitsbereich des Roboters. Das System muss deren Begrenzungen erkennen und darf diese nicht überschreiten.

### Getränkeuntersetzer

Getränkeuntersetzer stellen das primäre Transportobjekt des Systems dar. Der Coasterbot muss diese aufnehmen, transportieren, positionieren und wieder aufnehmen können.

### Getränke

Getränke befinden sich auf der Tischoberfläche und dienen als Referenz für die Positionierung der Getränkeuntersetzer. Sie stellen gleichzeitig Hindernisse dar, die während der Navigation berücksichtigt werden müssen.

### Hindernisse

Hindernisse können beispielsweise Gläser, Teller, Besteck, Dekoration oder andere Gegenstände auf dem Tisch sein. Diese dürfen durch den Roboter weder beschädigt noch unbeabsichtigt verschoben werden.

## Hardware-Schnittstellen

Die Software kommuniziert über definierte Schnittstellen mit den Hardwarekomponenten des Roboters.

Zu den Hardwarekomponenten gehören insbesondere:

* Sensoren zur Erfassung der Umgebung,
* Sensoren zur Erkennung der Tischkante,
* Sensoren zur Positions- und Orientierungsermittlung,
* Aktoren zur Fortbewegung,
* Aktoren zur Aufnahme und Ausgabe von Getränkeuntersetzern,
* Energieversorgung einschließlich Akku und Ladeelektronik.

Die konkrete Auswahl der Sensoren und Aktoren ist nicht Bestandteil des Lastenhefts und wird im Pflichtenheft festgelegt.

## Softwareschnittstellen

Die Softwarearchitektur muss klar definierte Schnittstellen zwischen den einzelnen Komponenten bereitstellen.

Die wichtigsten Softwarekomponenten umfassen:

* Navigationskomponente,
* Lokalisierungskomponente,
* Bewegungssteuerung,
* Komponenten zur Handhabung von Getränkeuntersetzern,
* Sicherheitskomponente,
* Systemüberwachung,
* Simulationskomponenten.

Die Kommunikation zwischen den Komponenten muss über wohldefinierte Schnittstellen erfolgen, um eine unabhängige Entwicklung, Wartung und Erweiterung zu ermöglichen.

## Schnittstellen zur Simulation

Die Simulationsumgebung dient der Validierung der entwickelten Software und muss dieselben logischen Schnittstellen bereitstellen wie das reale System.

Insbesondere müssen folgende Elemente simuliert werden können:

* Sensorwerte,
* Aktorbefehle,
* Hindernisse,
* Tischbegrenzungen,
* Getränke,
* Getränkeuntersetzer,
* Fehlersituationen.

Dadurch wird sichergestellt, dass Softwarekomponenten ohne Änderungen sowohl in der Simulation als auch auf der realen Hardware ausgeführt werden können.

## Systemgrenzen zukünftiger Ausbaustufen

Die folgenden Schnittstellen sind nicht Bestandteil des im Rahmen dieser Projektarbeit entwickelten Prototyps und bleiben zukünftigen Entwicklungsstufen vorbehalten:

* Anbindung an Kassensysteme,
* Integration in lokale Netzwerke oder Cloud-Dienste,
* Kommunikation zwischen mehreren Coasterbots,
* mobile Anwendungen zur Steuerung des Systems,
* digitale Bestell- und Bezahlsysteme,
* automatische Verwaltung mehrerer Tische.

Diese Funktionen stellen potenzielle Erweiterungen dar und werden im Rahmen des vorliegenden Projekts nicht betrachtet.

# Abnahmekriterien und Verifikation

Dieses Kapitel beschreibt die Verfahren zur Überprüfung der im Lastenheft definierten Anforderungen. Ziel der Verifikation ist der Nachweis, dass der entwickelte Prototyp die spezifizierten Anforderungen erfüllt. Die Verifikation erfolgt im Rahmen der Projektarbeit überwiegend durch Simulation, Tests sowie die Überprüfung der erstellten Entwicklungsartefakte.

## Verifikationsverfahren

Zur Überprüfung der Anforderungen werden die folgenden Verifikationsverfahren eingesetzt:

| Verfahren             | Beschreibung                                                                                                                    |
|:----------------------|:------------------------------------------------------------------------------------------------------------------------------- |
| **Inspektion**            | Überprüfung von Dokumenten, Schaltplänen, Konstruktionszeichnungen oder Softwareartefakten ohne Ausführung des Systems.         |
| **Analyse**               | Nachweis einer Anforderung durch Berechnungen, Modellierung oder theoretische Betrachtungen.                                    |
| **Simulation**            | Nachweis des Systemverhaltens innerhalb der entwickelten Simulationsumgebung.                                                   |
| **Test**                  | Praktische Überprüfung einer Anforderung durch Ausführung definierter Testfälle auf dem Prototyp oder innerhalb der Simulation. |

Je nach Anforderung können mehrere Verifikationsverfahren kombiniert werden.

## Verifikation der funktionalen Anforderungen

Die funktionalen Anforderungen werden entsprechend ihrer Eigenschaften mit geeigneten Verfahren überprüft.

| Anforderungsbereich                 | Verifikation              |
| ----------------------------------- | ------------------------- |
| Navigation und Lokalisierung        | Simulation, Test          |
| Handhabung von Getränkeuntersetzern | Simulation, Test          |
| Sicherheit                          | Analyse, Simulation, Test |
| Systemüberwachung                   | Test                      |

Die einzelnen Testfälle werden im Pflichtenheft bzw. im Testkonzept eindeutig den jeweiligen Anforderungen zugeordnet.

## Verifikation der nichtfunktionalen Anforderungen

Die nichtfunktionalen Anforderungen werden überwiegend durch Inspektion, Analyse sowie Tests nachgewiesen.

| Anforderungsbereich  | Verifikation        |
| -------------------- | ------------------- |
| Softwarearchitektur  | Inspektion          |
| Modularität          | Inspektion          |
| Erweiterbarkeit      | Analyse, Inspektion |
| Testbarkeit          | Test                |
| Simulationsfähigkeit | Simulation          |
| Dokumentation        | Inspektion          |

## Abnahmekriterien

Die Entwicklung des Coasterbots gilt als erfolgreich abgeschlossen, wenn die folgenden Kriterien erfüllt sind:

* Die mechanische Konstruktion des Prototyps ist vollständig ausgearbeitet.
* Die erforderlichen STL-Dateien zur Fertigung des Prototyps liegen vor.
* Die elektronischen Schaltpläne wurden erstellt.
* Die Softwarearchitektur ist dokumentiert.
* Die wesentlichen Softwarekomponenten wurden konzipiert.
* Eine Simulationsumgebung wurde entwickelt.
* Die Simulation bildet das erwartete Verhalten des Systems ausreichend genau ab.
* Alle identifizierten Testfälle wurden definiert.
* Die definierten Testfälle können innerhalb der Simulation ausgeführt werden.
* Die Ergebnisse der Simulation stimmen mit dem erwarteten Systemverhalten überein.

## Anforderungen an die Simulation

Da die Simulation Bestandteil der Definition of Done ist, muss sie folgende Eigenschaften erfüllen:

| ID      | Anforderung                                                                                       |
| ------- | ------------------------------------------------------------------------------------------------- |
| SIM-001 | Fahrmanöver müssen reproduzierbar simuliert werden können.                                   |
| SIM-002 | Sensoren müssen modelliert werden können.                                                         |
| SIM-003 | Aktoren müssen modelliert werden können.                                                          |
| SIM-004 | Hindernisse müssen in der Simulation berücksichtigt werden.                                       |
| SIM-005 | Fehlersituationen müssen simuliert werden können.                                                 |
| SIM-006 | Die Simulation muss die identifizierten Testfälle unterstützen.                                  |
| SIM-007 | Das Verhalten der Simulation muss mit dem erwarteten Verhalten des realen Systems übereinstimmen. |

## Rückverfolgbarkeit

Alle Anforderungen dieses Lastenhefts müssen während des Entwicklungsprozesses eindeutig nachverfolgbar sein. Hierzu erhält jede Anforderung eine eindeutige Kennung, die in allen Entwicklungsartefakten verwendet wird.

Die Rückverfolgbarkeit umfasst mindestens folgende Beziehungen:

* Anforderung → Softwarekomponente
* Anforderung → Hardwarekomponente
* Anforderung → Testfall
* Anforderung → Simulation
* Anforderung → Verifikationsergebnis

Durch diese Zuordnung kann jederzeit nachvollzogen werden, wie eine Anforderung umgesetzt und überprüft wurde.

## Abnahmeentscheidung

Die Abnahme des Projekts erfolgt auf Grundlage der im Lastenheft definierten Anforderungen sowie der in der Definition of Done beschriebenen Ergebnisse. Als erfüll gilt das Projekt, wenn die verpflichtenden Anforderungen des **Maturity Level 1** erfüllt und durch geeignete Verifikationsverfahren nachgewiesen wurden. Anforderungen höherer Maturity Levels sind nicht Bestandteil der Abnahme und dienen ausschließlich als Grundlage für zukünftige Erweiterungen des Systems.

# 7 Glossar

Dieses Glossar definiert die im Lastenheft verwendeten Fachbegriffe. Ziel ist es, ein einheitliches Verständnis der verwendeten Terminologie sicherzustellen und Mehrdeutigkeiten zu vermeiden.

| Begriff                                                           | Definition                                                                                                                                                                                |
|:------------------------------------------------------------------| ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Aktor**                                                         | Hardwarekomponente, die Steuerbefehle der Software in eine physische Bewegung oder Aktion umsetzt, beispielsweise ein Elektromotor oder ein Greifmechanismus.                             |
| **Arbeitsbereich**                                                | Der Bereich der Tischoberfläche, auf dem sich der Coasterbot autonom bewegen darf.                                                                                                        |
| **Autonome Navigation**                                           | Fähigkeit des Coasterbots, sich ohne direkte Steuerung durch einen Benutzer innerhalb seines Arbeitsbereichs zu bewegen und dabei Hindernisse sowie Tischbegrenzungen zu berücksichtigen. |
| **Coasterbot**                                                    | Der im Rahmen dieser Projektarbeit entwickelte autonome mobile Roboter zur Ausgabe und Aufnahme von Getränkeuntersetzern.                                                                 |
| **Definition of Done (DoD)**                                      | Kriterien, die erfüllt sein müssen, damit die Projektarbeit als erfolgreich abgeschlossen gilt.                                                                                           |
| **Funktionale Anforderung**                                       | Anforderung, die beschreibt, welche Funktionen das System bereitstellen muss.                                                                                                             |
| **Getränkeuntersetzer**                                           | Standardisierter Untersetzer, der vom Coasterbot aufgenommen, transportiert, unter einem Getränk platziert und anschließend wieder aufgenommen werden kann.                               |
| **Hardwareabstraktion**                                           | Softwareschicht, welche die konkrete Hardware von der Anwendungslogik entkoppelt und dadurch den Austausch von Sensoren oder Aktoren erleichtert.                                         |
| **Hindernis**                                                     | Objekt innerhalb des Arbeitsbereichs, das während der Navigation erkannt und umfahren werden muss, beispielsweise Gläser, Teller oder Besteck.                                            |
| **Lokalisierung**                                                 | Bestimmung der aktuellen Position des Coasterbots innerhalb seines Arbeitsbereichs.                                                                                                       |
| **Maturity Level**                                                | Reifegrad des Systems, der den Funktionsumfang einer Entwicklungsstufe beschreibt. Im Rahmen dieser Projektarbeit wird ausschließlich der Maturity Level 1 umgesetzt.                     |
| **Nichtfunktionale Anforderung**                                  | Anforderung, die Qualitätsmerkmale des Systems beschreibt, beispielsweise Modularität, Testbarkeit oder Wartbarkeit.                                                                      |
| **Not-Aus**                                                       | Sicherheitsfunktion, durch die der Coasterbot im Gefahrenfall unmittelbar in einen sicheren Zustand überführt werden kann.                                                                |
| **Orientierung**                                                  | Ausrichtung des Coasterbots relativ zu seinem Arbeitsbereich, beschrieben durch seine Fahrtrichtung.                                                                                      |
| **Pflichtenheft**                                                 | Dokument, das beschreibt, wie die im Lastenheft definierten Anforderungen technisch umgesetzt werden.                                                                                     |
| **Prototyp**                                                      | Funktionsfähiges Demonstrationssystem, das den Nachweis der technischen Machbarkeit der definierten Kernfunktionen erbringt, ohne den Anspruch eines serienreifen Produkts zu erfüllen.   |
| **Sensor**                                                        | Hardwarekomponente zur Erfassung von Informationen aus der Umgebung oder dem Systemzustand, beispielsweise Abstandssensoren, Kameras oder Inertialsensoren.                               |
| **Simulation**                                                    | Softwaregestützte Nachbildung des Systemverhaltens zur Entwicklung, Verifikation und Validierung des Coasterbots ohne Einsatz der realen Hardware.                                        |
| **Sicherer Zustand**                                              | Betriebszustand, in dem vom Coasterbot keine Gefährdung für Personen, Gegenstände oder sich selbst ausgeht.                                                                               |
| **Softwarekomponente**                                            | Eigenständiger Teil der Software mit einer klar definierten Aufgabe und eindeutig beschriebenen Schnittstellen.                                                                           |
| **STL-Datei**                                                     | Dateiformat zur Beschreibung dreidimensionaler Geometrien, das insbesondere für die Fertigung mittels 3D-Druck verwendet wird.                                                            |
| **Systemarchitektur**                                             | Struktur des Gesamtsystems einschließlich seiner Hardware- und Softwarekomponenten sowie deren Beziehungen und Schnittstellen.                                                            |
| **Systemgrenze**                                                  | Abgrenzung zwischen den Bestandteilen des Coasterbots und seiner externen Umgebung.                                                                                                       |
| **Testfall**                                                      | Definierte Abfolge von Eingaben, Aktionen und erwarteten Ergebnissen zur Überprüfung einer oder mehrerer Anforderungen.                                                                   |
| **Verifikation**                                                  | Nachweis, dass eine Anforderung korrekt umgesetzt wurde, beispielsweise durch Test, Simulation, Analyse oder Inspektion.                                                                  |
| **Validierung**                                                   | Nachweis, dass das entwickelte System den vorgesehenen Anwendungszweck erfüllt und die Anforderungen des Auftraggebers erfüllt.                                                           |

# Anhang A – Referenzen

Die nachfolgend aufgeführten Quellen bilden die fachliche Grundlage für die Erstellung des Lastenhefts sowie für die Entwicklung des Coasterbots. Sie umfassen Normen, Fachliteratur und wissenschaftliche Veröffentlichungen aus den Bereichen Requirements Engineering, Softwarearchitektur, Robotik und Qualitätssicherung.

## A.1 Normen

**ISO/IEC/IEEE 29148:2018**
*Systems and software engineering – Life cycle processes – Requirements engineering.*
International Organization for Standardization (ISO), International Electrotechnical Commission (IEC) und Institute of Electrical and Electronics Engineers (IEEE), 2018.

> Grundlage für die Erstellung und Strukturierung von Anforderungen sowie Lasten- und Pflichtenheften.

---

**ISO/IEC 25010:2011**
*Systems and software engineering – Systems and software Quality Requirements and Evaluation (SQuaRE) – System and software quality models.*
ISO, Genf, 2011.

> Definiert Qualitätsmerkmale wie Wartbarkeit, Zuverlässigkeit, Erweiterbarkeit und Testbarkeit.

---

**IEEE Std 1012-2016**
*IEEE Standard for System, Software, and Hardware Verification and Validation.*

> Grundlage für die Planung von Verifikation und Validierung.

---

**IEEE Std 830-1998** *(historisch)*
*IEEE Recommended Practice for Software Requirements Specifications.*

> Historischer Standard für Softwareanforderungen; heute weitgehend durch ISO/IEC/IEEE 29148 ersetzt.

---

## A.2 Nationale Richtlinien

**V-Modell XT**

Bundesministerium des Innern und für Heimat (BMI)

> Deutscher Standard für die Entwicklung komplexer IT-Systeme und öffentliche IT-Projekte. Enthält Vorgehensmodelle für Requirements Engineering, Systementwurf und Test.

---

## A.3 Fachliteratur

Pohl, K. (2022)

**Requirements Engineering – Grundlagen, Prinzipien, Techniken**

dpunkt.verlag.

> Standardwerk zum Requirements Engineering mit Methoden zur Erhebung, Dokumentation und Verwaltung von Anforderungen.

---

Sommerville, I. (2016)

**Software Engineering** (10th Edition)

Pearson.

> Grundlagen der Softwareentwicklung, Softwarearchitektur und Qualitätssicherung.

---

Gamma, E.; Helm, R.; Johnson, R.; Vlissides, J. (1994)

**Design Patterns – Elements of Reusable Object-Oriented Software**

Addison-Wesley.

> Referenzwerk zur Entwicklung modularer und erweiterbarer Softwarearchitekturen.

---

Martin, R. C. (2018)

**Clean Architecture – A Craftsman's Guide to Software Structure and Design**

Prentice Hall.

> Konzepte für lose gekoppelte und wartbare Softwaresysteme.

---

## A.4 Robotik

Siciliano, B.; Khatib, O. (Hrsg.) (2016)

**Springer Handbook of Robotics**

2nd Edition, Springer.

> Umfassendes Nachschlagewerk zu mobiler Robotik, Navigation, Sensorik und Aktorik.

---

Corke, P. (2017)

**Robotics, Vision and Control**

Second Edition, Springer.

> Grundlagen der mobilen Robotik, Computer Vision und autonomen Navigation.

---

Thrun, S.; Burgard, W.; Fox, D. (2005)

**Probabilistic Robotics**

MIT Press.

> Standardwerk zur Lokalisierung, Navigation und Sensorfusion autonomer Roboter.

---

## A.5 Softwarearchitektur

Richards, M.; Ford, N. (2020)

**Fundamentals of Software Architecture**

O'Reilly Media.

> Methoden zur Entwicklung moderner Softwarearchitekturen.

---

Bass, L.; Clements, P.; Kazman, R. (2022)

**Software Architecture in Practice**

4th Edition, Addison-Wesley.

> Architekturmethoden für komplexe Softwaresysteme.

---

## A.6 Test und Qualitätssicherung

Myers, G. J.; Sandler, C.; Badgett, T. (2011)

**The Art of Software Testing**

3rd Edition, Wiley.

> Grundlagen systematischer Softwaretests.

---

Ammann, P.; Offutt, J. (2016)

**Introduction to Software Testing**

2nd Edition, Cambridge University Press.

> Methoden zur Planung und Durchführung von Softwaretests.

---

## A.7 Wissenschaftliche Quellen des Projekts

Deutsche Industrie- und Handelskammer (DIHK)

**Fachkräftemangel in Deutschland**

> Grundlage für die Motivation des Projekts hinsichtlich der Entlastung des Servicepersonals.

---

„Guiding Manufacturing Companies Towards Digitalization – A Methodology for Supporting Manufacturing Companies in Defining Their Digitalization Roadmap“

> Grundlage für die Definition der verwendeten Maturity Levels.

---

Quellen zur zunehmenden Bedeutung von Online-Bewertungen und deren Einfluss auf die Restaurantwahl.

> Grundlage für die Motivation des Projekts hinsichtlich der Attraktivität gastronomischer Betriebe.

---

## A.8 Projektspezifische Dokumente

Die folgenden Dokumente entstehen im Verlauf des Projekts und ergänzen das Lastenheft:

* Pflichtenheft
* Softwarearchitekturdokument
* Systemarchitekturdiagramme
* Schaltpläne
* Mechanische Konstruktion (CAD)
* STL-Dateien
* Simulationsmodell
* Testkonzept
* Testfallspezifikation
* Testprotokolle
* Projektdokumentation
