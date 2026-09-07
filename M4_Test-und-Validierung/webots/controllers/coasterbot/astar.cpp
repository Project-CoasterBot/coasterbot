#include "astar.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace {

constexpr float kSqrt2 = 1.41421356f;

// Naechste freie Zelle per Ringsuche (falls start/goal im aufgeblaehten
// Hindernis oder knapp ausserhalb der Karte liegen).
Cell nearestFree(const OccupancyGrid& grid, Cell c) {
    if (!grid.occupied(c)) return c;
    const int maxR = std::max(grid.cols(), grid.rows());
    for (int r = 1; r <= maxR; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
                Cell n{c.cx + dx, c.cy + dy};
                if (!grid.occupied(n)) return n;
            }
        }
    }
    return c;
}

// Sichtlinie zwischen zwei Weltpunkten frei? (Abtastung mit halber Zellweite.)
bool lineOfSight(const OccupancyGrid& grid, Vec2 a, Vec2 b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const int steps = std::max(1, static_cast<int>(dist / (0.5f * grid.resolution())));
    for (int i = 0; i <= steps; ++i) {
        const float t = static_cast<float>(i) / steps;
        Vec2 p{a.x + t * dx, a.y + t * dy};
        if (grid.occupied(grid.worldToCell(p))) return false;
    }
    return true;
}

struct OpenNode {
    float f;
    int   index;
    bool operator>(const OpenNode& o) const { return f > o.f; }
};

}  // namespace

std::vector<Vec2> astarPlan(const OccupancyGrid& grid, Vec2 start, Vec2 goal) {
    const int cols = grid.cols();
    const int rows = grid.rows();
    const int n = cols * rows;

    const Cell startC = nearestFree(grid, grid.worldToCell(start));
    const Cell goalC  = nearestFree(grid, grid.worldToCell(goal));

    auto id = [cols](int cx, int cy) { return cy * cols + cx; };
    auto heur = [&](int cx, int cy) {
        const float ex = static_cast<float>(cx - goalC.cx);
        const float ey = static_cast<float>(cy - goalC.cy);
        return std::sqrt(ex * ex + ey * ey) * grid.resolution();
    };

    std::vector<float> gScore(n, std::numeric_limits<float>::infinity());
    std::vector<int>   cameFrom(n, -1);
    std::vector<unsigned char> closed(n, 0);
    std::priority_queue<OpenNode, std::vector<OpenNode>, std::greater<OpenNode>> open;

    const int startIdx = id(startC.cx, startC.cy);
    const int goalIdx  = id(goalC.cx, goalC.cy);
    gScore[startIdx] = 0.0f;
    open.push({heur(startC.cx, startC.cy), startIdx});

    const int dx8[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    const int dy8[8] = {0, 0, 1, -1, 1, -1, 1, -1};

    bool found = false;
    while (!open.empty()) {
        const int cur = open.top().index;
        open.pop();
        if (closed[cur]) continue;
        closed[cur] = 1;
        if (cur == goalIdx) { found = true; break; }

        const int cx = cur % cols;
        const int cy = cur / cols;
        for (int k = 0; k < 8; ++k) {
            const int nx = cx + dx8[k];
            const int ny = cy + dy8[k];
            if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) continue;
            if (grid.occupied({nx, ny})) continue;
            const bool diagonal = (dx8[k] != 0 && dy8[k] != 0);
            if (diagonal) {
                // kein "corner cutting" durch die Diagonale zulassen
                if (grid.occupied({cx + dx8[k], cy}) || grid.occupied({cx, cy + dy8[k]}))
                    continue;
            }
            const int ni = id(nx, ny);
            if (closed[ni]) continue;
            const float step = (diagonal ? kSqrt2 : 1.0f) * grid.resolution();
            const float tentative = gScore[cur] + step;
            if (tentative < gScore[ni]) {
                gScore[ni] = tentative;
                cameFrom[ni] = cur;
                open.push({tentative + heur(nx, ny), ni});
            }
        }
    }

    if (!found) return {};

    // Zell-Pfad rekonstruieren (Ziel -> Start) und umdrehen.
    std::vector<Vec2> raw;
    for (int cur = goalIdx; cur != -1; cur = cameFrom[cur])
        raw.push_back(grid.cellToWorld({cur % cols, cur / cols}));
    std::reverse(raw.begin(), raw.end());

    // Exakte Endpunkte verwenden.
    if (!raw.empty()) { raw.front() = start; raw.back() = goal; }
    return smoothPath(grid, raw);
}

std::vector<Vec2> smoothPath(const OccupancyGrid& grid, const std::vector<Vec2>& raw) {
    if (raw.size() < 3) return raw;

    // "String pulling": von jedem Anker den entferntesten noch sichtbaren
    // Wegpunkt behalten.
    std::vector<Vec2> smooth;
    smooth.push_back(raw.front());
    size_t anchor = 0;
    while (anchor + 1 < raw.size()) {
        size_t next = anchor + 1;
        for (size_t j = raw.size() - 1; j > anchor + 1; --j) {
            if (lineOfSight(grid, raw[anchor], raw[j])) { next = j; break; }
        }
        smooth.push_back(raw[next]);
        anchor = next;
    }
    return smooth;
}

std::vector<Vec2> centerPath(const OccupancyGrid& grid, const std::vector<Vec2>& path,
                             float maxShift) {
    if (path.size() < 3) return path;
    std::vector<Vec2> out = path;
    const float step = grid.resolution();
    for (size_t i = 1; i + 1 < path.size(); ++i) {
        float tx = path[i + 1].x - path[i - 1].x;
        float ty = path[i + 1].y - path[i - 1].y;
        const float len = std::sqrt(tx * tx + ty * ty);
        if (len < 1e-6f) continue;
        const float px = -ty / len, py = tx / len;   // Normale zur Fahrtrichtung

        Vec2  best = path[i];
        float bestClr = grid.clearance(path[i]);
        for (float d = -maxShift; d <= maxShift + 1e-4f; d += step) {
            const Vec2 p{path[i].x + d * px, path[i].y + d * py};
            const float c = grid.clearance(p);
            if (c > bestClr + 1e-4f) { bestClr = c; best = p; }
        }
        out[i] = best;
    }
    return out;
}
