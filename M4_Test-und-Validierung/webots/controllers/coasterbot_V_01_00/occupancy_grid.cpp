#include "occupancy_grid.h"
#include <algorithm>
#include <cmath>
#include <limits>

OccupancyGrid::OccupancyGrid(float minX, float minY, float maxX, float maxY, float resolution)
    : minX_(minX), minY_(minY), res_(resolution) {
    cols_ = std::max(1, static_cast<int>(std::ceil((maxX - minX) / res_)));
    rows_ = std::max(1, static_cast<int>(std::ceil((maxY - minY) / res_)));
    occ_.assign(static_cast<size_t>(cols_) * rows_, 0);
    confidence_.assign(static_cast<size_t>(cols_) * rows_, 0.0f);
}

void OccupancyGrid::addObstacleRect(float ax, float ay, float bx, float by, float inflation) {
    const float x0 = std::min(ax, bx) - inflation;
    const float x1 = std::max(ax, bx) + inflation;
    const float y0 = std::min(ay, by) - inflation;
    const float y1 = std::max(ay, by) + inflation;

    int cx0 = static_cast<int>(std::floor((x0 - minX_) / res_));
    int cx1 = static_cast<int>(std::floor((x1 - minX_) / res_));
    int cy0 = static_cast<int>(std::floor((y0 - minY_) / res_));
    int cy1 = static_cast<int>(std::floor((y1 - minY_) / res_));

    cx0 = std::max(0, cx0);
    cy0 = std::max(0, cy0);
    cx1 = std::min(cols_ - 1, cx1);
    cy1 = std::min(rows_ - 1, cy1);

    for (int cy = cy0; cy <= cy1; ++cy)
        for (int cx = cx0; cx <= cx1; ++cx)
            occ_[idx(cx, cy)] = 1;

    dist_.clear();  // Distanzfeld ist jetzt veraltet
}

bool OccupancyGrid::setOccupied(Cell c, bool value) {
    if (!inBounds(c)) return false;
    unsigned char& cell = occ_[idx(c.cx, c.cy)];
    const unsigned char v = value ? 1 : 0;
    if (cell == v) return false;
    cell = v;
    dist_.clear();  // Distanzfeld ist jetzt veraltet
    return true;
}

bool OccupancyGrid::reportOccupied(Cell c, float smoothingFactor) {
    if (!inBounds(c)) return false;
    float& conf = confidence_[idx(c.cx, c.cy)];
    // Exponentielle Glaettung: naehert sich 1 an, erreicht es aber nie in
    // einem Schritt - ein einzelner Ausreisser reicht damit nicht, um die
    // Schwelle zu ueberschreiten.
    conf += smoothingFactor * (1.0f - conf);
    if (conf >= OCC_COMMIT_THRESHOLD)
        return setOccupied(c, true);
    return false;
}

void OccupancyGrid::buildDistanceField() {
    const float big = 1e6f;
    dist_.assign(static_cast<size_t>(cols_) * rows_, big);
    for (int i = 0; i < cols_ * rows_; ++i)
        if (occ_[i]) dist_[i] = 0.0f;

    const float o = res_;                 // orthogonaler Schritt
    const float d = res_ * 1.41421356f;   // diagonaler Schritt

    // Vorwaertsdurchlauf
    for (int cy = 0; cy < rows_; ++cy) {
        for (int cx = 0; cx < cols_; ++cx) {
            float v = dist_[idx(cx, cy)];
            if (cx > 0)               v = std::min(v, dist_[idx(cx - 1, cy)] + o);
            if (cy > 0)               v = std::min(v, dist_[idx(cx, cy - 1)] + o);
            if (cx > 0 && cy > 0)     v = std::min(v, dist_[idx(cx - 1, cy - 1)] + d);
            if (cx < cols_ - 1 && cy > 0) v = std::min(v, dist_[idx(cx + 1, cy - 1)] + d);
            dist_[idx(cx, cy)] = v;
        }
    }
    // Rueckwaertsdurchlauf
    for (int cy = rows_ - 1; cy >= 0; --cy) {
        for (int cx = cols_ - 1; cx >= 0; --cx) {
            float v = dist_[idx(cx, cy)];
            if (cx < cols_ - 1)          v = std::min(v, dist_[idx(cx + 1, cy)] + o);
            if (cy < rows_ - 1)          v = std::min(v, dist_[idx(cx, cy + 1)] + o);
            if (cx < cols_ - 1 && cy < rows_ - 1) v = std::min(v, dist_[idx(cx + 1, cy + 1)] + d);
            if (cx > 0 && cy < rows_ - 1) v = std::min(v, dist_[idx(cx - 1, cy + 1)] + d);
            dist_[idx(cx, cy)] = v;
        }
    }
}

float OccupancyGrid::clearance(Vec2 p) const {
    Cell c = worldToCell(p);
    if (!inBounds(c)) return 0.0f;         // ausserhalb = "an einem Hindernis"
    if (dist_.empty()) return occupied(c) ? 0.0f : 1e6f;
    return dist_[idx(c.cx, c.cy)];
}

bool OccupancyGrid::inBounds(Cell c) const {
    return c.cx >= 0 && c.cx < cols_ && c.cy >= 0 && c.cy < rows_;
}

bool OccupancyGrid::occupied(Cell c) const {
    if (!inBounds(c)) return true;
    return occ_[idx(c.cx, c.cy)] != 0;
}

Cell OccupancyGrid::worldToCell(Vec2 p) const {
    Cell c;
    c.cx = static_cast<int>(std::floor((p.x - minX_) / res_));
    c.cy = static_cast<int>(std::floor((p.y - minY_) / res_));
    return c;
}

Vec2 OccupancyGrid::cellToWorld(Cell c) const {
    Vec2 p;
    p.x = minX_ + (c.cx + 0.5f) * res_;
    p.y = minY_ + (c.cy + 0.5f) * res_;
    return p;
}
