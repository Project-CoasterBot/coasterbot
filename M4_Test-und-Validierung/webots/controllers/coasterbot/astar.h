#ifndef ASTAR_H
#define ASTAR_H

#include "occupancy_grid.h"
#include <vector>

// ---------------------------------------------------------------------
// Globale Pfadplanung mit A* (8-fach verbundenes Gitter, euklidische
// Heuristik). Entspricht der in der Projekt-Doku gewaehlten Loesung fuer
// die globale Fahrwegplanung.
//
// Rueckgabe: Wegpunkte in Weltkoordinaten von "start" nach "goal"
// (inklusive beider Endpunkte). Leerer Vektor => kein Pfad gefunden.
//
// Der Rohpfad wird per Sichtlinien-Test geglaettet ("string pulling"),
// damit der Pfadfolger keine 45-Grad-Treppen abfahren muss.
// ---------------------------------------------------------------------
std::vector<Vec2> astarPlan(const OccupancyGrid& grid, Vec2 start, Vec2 goal);

// Rohpfad (z.B. Gitter-Wegpunkte) per Sichtlinien-Test glaetten
// ("string pulling"). Von astarPlan intern genutzt; auch fuer den
// D*-Lite-Pfad verwendbar.
std::vector<Vec2> smoothPath(const OccupancyGrid& grid, const std::vector<Vec2>& raw);

// Innere Wegpunkte quer zur Fahrtrichtung in Richtung groesseren
// Hindernisabstands verschieben (max. maxShift Meter), damit der Pfad in
// engen Korridoren mittig verlaeuft statt an einer Ecke zu kleben.
// grid muss ein Distanzfeld haben (buildDistanceField()).
std::vector<Vec2> centerPath(const OccupancyGrid& grid, const std::vector<Vec2>& path,
                             float maxShift = 0.20f);

#endif  // ASTAR_H
