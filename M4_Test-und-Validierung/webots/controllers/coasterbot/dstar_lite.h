#ifndef DSTAR_LITE_H
#define DSTAR_LITE_H

#include "occupancy_grid.h"

#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------
// D* Lite (Koenig & Likhachev, 2002) - inkrementelle globale Planung.
//
// Sucht rueckwaerts vom Ziel und haelt fuer jede Zelle einen g- und
// rhs-Wert. Aendert sich die Karte waehrend der Fahrt (ein Hindernis
// taucht auf oder verschwindet), werden ueber cellChanged() nur die
// betroffenen Knoten neu in die Prioritaetswarteschlange gestellt;
// computeShortestPath() repariert dann den Pfad, ohne komplett neu zu
// planen. Genau der in der Projekt-Doku gewaehlte Ansatz fuer die
// dynamische Fahrplanung.
//
// Arbeitet auf demselben OccupancyGrid wie A* / DWA (8-fach verbunden).
// ---------------------------------------------------------------------
class DStarLite {
public:
    explicit DStarLite(const OccupancyGrid& grid);

    void plan(Cell start, Cell goal);      // Erstplanung
    void setStart(Cell start);             // Roboter hat sich bewegt
    void cellChanged(Cell c);              // Belegung von c hat sich geaendert
    void computeShortestPath();            // (Neu-)Berechnung

    std::vector<Vec2> extractPath() const; // start -> goal (leer = kein Pfad)
    bool pathExists() const;
    int  finiteGCount() const;             // Diagnose: Zellen mit endlichem g

private:
    struct Key { float k1 = 0.0f, k2 = 0.0f; };

    const OccupancyGrid& grid_;
    int cols_, rows_, n_;
    int startId_, goalId_, lastId_;
    float km_;
    std::vector<float> g_, rhs_;

    std::set<std::tuple<float, float, int>> open_;   // (k1, k2, id)
    std::unordered_map<int, Key> inOpen_;

    void  expandAll();   // vollstaendige Erstexpansion vom Ziel aus

    int   id(int cx, int cy) const { return cy * cols_ + cx; }
    int   neighborsOf(int u, int* out) const;   // returns count (<= 8)
    float edgeCost(int a, int b) const;
    float heuristic(int a, int b) const;
    Key   calcKey(int u) const;
    void  updateVertex(int u);
    void  queueInsert(int u, Key k);
    void  queueRemove(int u);
};

#endif  // DSTAR_LITE_H
