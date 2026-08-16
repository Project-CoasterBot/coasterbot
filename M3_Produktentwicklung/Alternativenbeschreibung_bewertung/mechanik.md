# Mechanik

In diesem Kapitel werden die verschiedenen Lösungsoptionen aus der MindMap in Abbildung *(REFERENZ)*  diskutiert und bewertet. Am Ende ist damit festgelegt, welche Lösungsoption für den Prototypen des Coasterbots verfolgt wird.

## Untersetzer aufnehmen/ablegen

Die beiden Kategorien Untersetzer aufnehmen und ablegen werden gemeinsam behandelt, da sich beide mit ähnlichen Lösungsoptionen abbilden lassen. Im folgende wird jede erkannte Option mit ihren Vor- und Nachteilen abgewogen.

### Roboter mit Sauger/Greifer

Bei der Option eines Roboters mit Sauger/Greifer ist ein Mehrachsiger Roboterarm bei dem der Manipulation ein Sauger oder ein Greifer sein kann. Vorteile bei dieser Lösungsoption ist die Flexibilität durch mehrere Achsen. Dieser Vorteil kommt jedoch mit einer erhöhten Komplexität einher. So muss für jede Achse ein Fahrprofil in der Programmierung berücksichtigt werden, damit ein Untersetzer korrekt aufgenommen bzw. abgelegt wird. Ein Weiterer Nachteil dieser Option ist der Mehrbedarf an mechanischen Komponenten. Für den Anwendungsfall wäre mindestens ein Roboter mit zwei Freiheitsgraden notwendig, sowie zusätzlicher Aktorik für den Manipulator (Sauger/Greifer). Das Bedeutet, dass minimal drei Aktoren bei dieser Lösung benötigt würden.

### Manuelle Einlage