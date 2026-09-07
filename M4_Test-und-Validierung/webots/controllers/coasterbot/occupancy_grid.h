#ifndef OCCUPANCY_GRID_H
#define OCCUPANCY_GRID_H

#include <vector>

// Punkt / Zelle. Weltkoordinaten sind die X-Y-Ebene der Webots-Welt (Z = oben),
// d.h. dieselbe Ebene, in der die PoseEstimator-Pose lebt.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }
};
struct Cell { int cx = 0; int cy = 0; };

// ---------------------------------------------------------------------
// Statische Belegungskarte ("occupancy grid") des Tisches / der Flaeche.
// Achsenparalleles Rechteck, feste Aufloesung. Hindernisse werden als
// Rechtecke mit Sicherheitsabstand (Roboterradius) eingetragen.
//
// Entspricht der "bekannten Tischkarte" aus der Projekt-Doku und ist die
// Eingabe fuer den A*-Planer.
// ---------------------------------------------------------------------
class OccupancyGrid {
public:
    OccupancyGrid(float minX, float minY, float maxX, float maxY, float resolution);

    // Achsenparalleles Hindernis (Weltkoordinaten, beliebige Eckreihenfolge)
    // um "inflation" Meter aufgeblaeht eintragen.
    void addObstacleRect(float ax, float ay, float bx, float by, float inflation);

    // Einzelne Zelle setzen/loeschen. Rueckgabe: true, wenn sich der Wert
    // geaendert hat (fuer inkrementelle Replanung mit D* Lite).
    bool setOccupied(Cell c, bool value);

    int   cols() const { return cols_; }
    int   rows() const { return rows_; }
    float resolution() const { return res_; }

    bool  inBounds(Cell c) const;
    bool  occupied(Cell c) const;          // ausserhalb der Karte => belegt
    Cell  worldToCell(Vec2 p) const;
    Vec2  cellToWorld(Cell c) const;       // Zellmittelpunkt

    // Distanzfeld (Chamfer-Approximation) fuer schnelle Abstandsabfragen,
    // z.B. fuer die Kollisionsbewertung im DWA-Planer. Muss nach dem
    // Eintragen aller Hindernisse einmal aufgerufen werden; addObstacleRect
    // markiert das Feld automatisch als veraltet.
    void  buildDistanceField();
    float clearance(Vec2 p) const;         // [m] Abstand zum naechsten Hindernis (>= 0)

private:
    float minX_, minY_, res_;
    int   cols_, rows_;
    std::vector<unsigned char> occ_;       // 0 = frei, 1 = belegt
    std::vector<float>         dist_;      // [m], leer bis buildDistanceField()

    int idx(int cx, int cy) const { return cy * cols_ + cx; }
};

#endif  // OCCUPANCY_GRID_H
