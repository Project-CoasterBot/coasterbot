---
mainfont: DejaVuSerif.ttf
sansfont: DejaVuSans.ttf
monofont: DejaVuSansMono.ttf
mathfont: texgyredejavu-math.otf
---

# Forschungsüberblick der Algorithmen

## Klassische Graphalgorithmen

Die Grundlage vieler Verfahren zur autonomen Navigation bilden klassische
Graphalgorithmen. Dabei wird die Umgebung als Graph modelliert, dessen Knoten
diskrete Positionen oder Zustände repräsentieren, während Kanten mögliche
Bewegungen zwischen diesen beschreiben. Ziel ist die Bestimmung eines optimalen
Pfades hinsichtlich einer Kostenfunktion, beispielsweise der zurückgelegten
Distanz, Fahrzeit oder des Energieverbrauchs.

Ein Vertreter ist der Dijkstra-Algorithmus, der den kürzesten Pfad in Graphen
mit Kantengewichten berechnet. Der Algorithmus garantiert optimale Lösungen
insofern die Kantengewichte nicht-negativ sind und
stellt ein Referenzverfahren für die globale Pfadplanung in statischen
Umgebungen dar [@Dijkstra]. Seine mathematische Grundlage sowie Laufzeitanalyse werden
ausführlich in "Introduction to Algorithms" von Cornet et al. beschrieben [@IntroToAlgs].

Neben Dijkstra existieren weitere grundlegende Suchverfahren, wie die
Breitensuche (Breadth-First Search) und Tiefensuche (Depth-First Search,
DFS)[@IntroToAlgs].
Während BFS optimale Lösungen in ungewichteten Graphen liefert, dient DFS vor
allem der Strukturanalyse von Graphen und spielt in der Roboternavigation eine
untergeordnete Rolle.

Für Anwendungen mit einer vollständig bekannten und statischen Umgebung stellen
diese Algorithmen eine zuverlässige Grundlage der globalen Pfadplanung dar.

## Heuristische Verfahren

Da klassische Graphalgorithmen bei großen Suchräumen einen hohen Rechenaufwand
verursachen, werden in der mobilen Robotik häufig heuristische Suchverfahren
eingesetzt. Ein bedeutendes Verfahren ist der heusristische A* Algorithmus, um
die Suche gezielt in Richtung des Zielknotens zu lenken [@IntroToAlgs], [@AStar].

Die Bewertung eines Knotens erfolgt über

$$
f(n) = g(n)+h(n)
$$

Hierbei stellen $g(n)$ die wirklichen Kosten eines optimalen Pfades $s$ nach
$n$ und $h(n)$ die wirklichen Kosten eines optimalen Pfades von $n$ zu einem
präferierten Zielknoten von $n$ darstellt. Allgemein ist $h(n)$ eine Schätzung
der verbleibenden Distanz zum Ziel [@AStar]. Ist die Heuristik zulässig (admissible),
garantiert A* ebenfalls optimale Lösungen, untersucht jedoch weniger Knoten als
Dijkstra.

Für mobile Roboter kommen häufig euklidische oder Manhattan-Distanzen [@IntroToAlgs] als
Heuristik zum Einsatz. Dadurch eignet sich A* insbesondere für Gitternetze
(Occupancy Grids), wie sie in der Robotik genutzt werden[@RoboticMapping].

Aktuellere Varianten wie Weighted A* oder Anytime A* priorisieren kürzere
Rechenzeiten gegenüber der optimalen Lösung, weshalb deren Anwendung geeignet in
Echtzeitsystemen ist[@WeightedA], [@AnytimeA].

## Dynamische Pfadplanung

In realen Einsatzgebieten verändert sich die Umgebung häufig während der Fahrt.
Statische Algorithmen müssten den gesamten Pfad erneut berechnen. Dies führt zu
hohen Rechenzeiten.

Hier setzen Verfahren der dynamischen Pfadplanung an. Zu nennen sind unter
anderem D* (Dynamic A*) [@DAlg] sowie dessen Weiterentwicklung D* Lite [@DLite]. Beide
Verfahren aktualisieren ausschließlich die von einer Umweltveränderung
betroffenen Bereiche des Suchgraphen und vermeiden dadurch vollständige
Neuberechnungen.

Ein weiteres relevantes Verfahren ist Lifelong Planning A* (LPA*), welches
inkrementelle Änderungen effizient verarbeitet [@LifelongPlanningA]. Solche
Algorithmen werden insbesondere in autonomen Fahrzeugen und mobilen
Servicerobotern eingesetzt, deren Umgebungen durch Menschen oder bewegliche
Objekte beeinflusst werden.

Für einen Coaster Bot wären dynamische Planungsverfahren dann relevant, wenn
sich während der Navigation Gläser, Teller oder Personen im Arbeitsbereich
befinden.

## Kollisionsvermeidung

Während globale Pfadplanung den optimalen Weg bestimmen, übernimmt die
Kollisionsvermeidung die kurzfristige Reaktion auf Hindernisse.

Ein Ansatz sind Potentialfeldmethoden, bei denen das Ziel eine anziehende Kraft
und Hindernisse abstoßende Kräfte erzeugen. Diese Methoden sind einfach
implementierbar, können jedoch in lokalen Minima stecken bleiben
[@Potentialfeldmethoden].

In mobilen Robotersystemen wird u.a. der Dynamic Window Approach (DWA)
verwendet, der die Bewegungsdynamik des Roboters selbst einbezieht
[@DynamicWindowApproach]. Diese Strategie ist grundsätzlich ein statischer
Ansatz, der die Eigenschaften von Veränderungen in der Umgebung nicht
berücksichtigt. Durch den Einsatz von Neuronalen Netzen kann DWA, durch kurze
Observationen der sich verändernden Umgebung, die Bewegungen der Hindernisse
modellieren und somit auf Veränderungen der Umwelt reagieren
[@DynamicAdaptiveDynamicWindowApproach].

Für komplexe Konfigurationsräume kommen außerdem samplingbasierte Verfahren wie
Rapidly-exploring Random Trees (RRT) [@RRT] sowie RRT* [@RRTStar] zum Einsatz.
Während RRT schnell zulässige Pfade findet, verbessert RRT* diese schrittweise
bis zum asympotischen Optimum [@RRTStar].

Somit bildet die Kombination aus globalem Pfadplaner und lokaler
Kollisionsvermeidung einen möglichen Ansatz zur Bewegung des Coaster Bots dar.

## SLAM und Lokalisierung

Zur verbesserten Navigation kann ein Roboter seine eigene Position bestimmen und
gleichzeitig eine Karte seiner Umgebung aufbauen. Dieses Problem wird als
Simultaneous Localization and Mapping (SLAM) bezeichnet [@SLAM].

Zu den klassischen Verfahren gehören Extended Kalman Filter (EKF-SLAM),
FastSLAM auf Basis von Paketfiltern sowie graphbasierte SLAM Verfahren [@EKF],
[@EKF-SLAM], [@FastSLAM].

Besonders graphbasierte Optimierungsverfahren habe sich aufgrund ihrer hohen
Genauigkeit etabliert und bilden die Grundlage von Robotik-Frameworks wie ROS2
[@ROS2].

Für den Coaster Bot hängt die Bedeutung von SLAM vom Einsatzszenario und den
verwendeten Sensoren ab. Wird ausschließlich auf einem bekannten Tisch
gearbeitet, genügt meist eine vorher definierte Karte. Soll das System autonom
unbekannte Arbeitsbereiche erfassen oder sich frei im Raum bewegen können, ist
ein SLAM Verfahren vorteilhaft.

## Manipulationsplanung

Neben der Navigation stellt die Manipulationsplanung einen wesentlichen
Bestandteil autonomer Serviceroboter dar. Sie umfasst die Planung und Steuerung
von Greifbewegungen sowie das sichere Aufnehmen und Ablegen von Objekten.
Hierbei müssen sowohl die Kinematik des Roboters als auch mögliche Kollisionen
zwischen Greifer, Objekt und Umgebung berücksichtigt werden
[@ManipulationCBiRRT], [@MotionPlanning].

Zur Bewegungsplanung von Roboterarmen werden häufig samplingbasierte Verfahren
wie RRT [@RRT], RRT* [@RRTStar] oder Probabilistic Roadmaps (PRM) [@PRM]
eingesetzt. Moderne Robotik-Frameworks wie MoveIt 2 [@MoveIt2] kombinieren diese
Planungsverfahren mit Kollisionsprüfungen und inverser Kinematik.

Für den Coaster Bot besteht die Manipulationsaufgabe darin, Getränkedeckel
präzise aufzunehmen und an einer definierten Zielposition abzulegen. Neben
einer genauen Positionierung des Roboters müssen dabei Greifstrategie,
Objektorientierung sowie eine zuverlässige Ablage berücksichtigt werden.

## Inertialnavigation

Die Inertialnavigation (Inertial Navigation System, INS) beschreibt Verfahren
zur Bestimmung der Position, Orientierung und Bewegung eines mobilen Systems
ausschließlich auf Grundlage von Trägheitssensoren. Im Gegensatz zu
kamerabasierten oder laserbasierten Lokalisierungsmethoden ist die
Inertialnavigation nicht auf externe Referenzpunkte oder eine Sichtverbindung
zur Umgebung angewiesen. Dadurch eignet sie sich insbesondere für Anwendungen,
in denen optische Sensoren nur eingeschränkt eingesetzt werden können oder
zeitweise keine zuverlässigen Umgebungsinformationen zur Verfügung stehen
[@IntegratedNavigationSystems], [@ModernRobotics].

Ein Inertialnavigationssystem besteht zumeist aus einer Inertial
Measurement Unit (IMU), die Beschleunigungssensoren (Accelerometer) und
Drehratensensoren (Gyroskope) kombiniert. Moderne Systeme integrieren
zusätzlich Magnetometer zur Bestimmung der absoluten Orientierung. Aus den
gemessenen Beschleunigungen und Winkelgeschwindigkeiten werden durch numerische
Integration Geschwindigkeit, Orientierung und Position des Roboters berechnet.
Die mathematischen Grundlagen dieser Verfahren basieren auf den
Bewegungsgleichungen der klassischen Mechanik sowie auf Verfahren zur Schätzung
von Zustandsgrößen [@IntegratedNavigationSystems], [@ModernRobotics].

Ein Problem der Inertialnavigation stellt der Drift dar. Geringe Messfehler der
Sensoren werden durch die fortlaufende Integration akkumuliert und führen mit
zunehmender Betriebsdauer zu wachsenden Positions- und Orientierungsfehlern.
Aus diesem Grund wird eine reine Inertialnavigation in der mobilen Robotik nur
selten dauerhaft eingesetzt. Stattdessen erfolgt eine Kombination mit externen
Sensorsystemen wie LiDAR, Kameras oder Radencodern, um die Positionsschätzung
regelmäßig zu korrigieren [@IntegratedNavigationSystems],
[@ProbabilisticRobotics].

Zur Fusion verschiedener Sensordaten kommen häufig probabilistische
Schätzverfahren wie der Kalman-Filter, der Extended Kalman Filter (EKF) [@EKF]
oder der Unscented Kalman Filter (UKF) [@UKF] zum Einsatz. Diese Verfahren
kombinieren die Messdaten der IMU mit absoluten Positionsinformationen anderer
Sensoren und ermöglichen dadurch eine robustere Lokalisierung. In aktuellen
Robotersystemen wird zudem vermehrt auf graphbasierte Optimierungsverfahren und
faktorgraphbasierte Ansätze zurückgegriffen, welche mehrere Sensordatenquellen
gleichzeitig berücksichtigen und die Genauigkeit der Positionsbestimmung
verbessern [@ProbabilisticRobotics], [@HandbookOfRobotics].

Die Inertialnavigation findet in zahlreichen Bereichen der Robotik Anwendung.
Autonome Serviceroboter, fahrerlose Transportsysteme (AGV), Drohnen sowie
mobile Manipulatoren nutzen IMUs zur Stabilisierung der Bewegung, zur Schätzung
der Fahrzeuglage und zur kurzfristigen Positionsbestimmung. Besonders in
Innenräumen, in denen satellitengestützte Navigationssysteme wie GPS nicht
verfügbar sind, stellt die IMU eine wichtige Informationsquelle für die
Bewegungsregelung dar [@GPS]. Auch in IoT- und Embedded-Systemen werden aufgrund der
geringen Baugröße, des niedrigen Energieverbrauchs und der geringen Kosten
häufig kompakte Micro-Electro-Mechanical Systems Inertial Measurement Units
(MEMS-IMUs) eingesetzt [@HandbookOfRobotics], [@ModernRobotics].

Für den geplanten Coaster Bot kann die Inertialnavigation die globale
Pfadplanung und sensorbasierte Navigation ergänzen. Während Verfahren wie A*
oder Dijkstra den optimalen Fahrweg berechnen und LiDAR- oder
Kamerasysteme Hindernisse erkennen, liefert die IMU kontinuierlich
Informationen über Beschleunigung und Drehrichtung des Roboters [@AStar],
[@IntroToAlgs]. Dadurch können
Fahrbewegungen zwischen Sensormessungen präziser geschätzt sowie kurzfristige
Positionsänderungen erfasst werden. Beim Anfahren definierter Ablage- oder
Aufnahmepositionen verbessert die Orientierungsschätzung die
Wiederholgenauigkeit des Systems. Die Kombination einer IMU mit Verfahren der
globalen Pfadplanung entspricht dem in der mobilen Robotik etablierten Ansatz
einer mehrstufigen Navigation [@ModernRobotics], [@HandbookOfRobotics].

Da sich der Coaster Bot auf einer vergleichsweise kleinen Arbeitsfläche bewegt
und Bewegungen präzise erfolgen sollten,
ist eine alleinige Inertialnavigation aufgrund der Drift möglicherweise nicht
ausreichend. Eine mögliche Kombination aus mehreren Sensoren wie IMU,
Radencodern, einem LiDAR- oder kamerabasierten Lokalisierungssystem würden eine
effizientere Positionsbestimmung erreichen.

## Literatur
