#include "dstar_lite.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
constexpr float INF = 1e18f;
constexpr float EPS = 1e-6f;

bool keyLess(float a1, float a2, float b1, float b2) {
    if (a1 != b1) return a1 < b1;
    return a2 < b2;
}
}  // namespace

DStarLite::DStarLite(const OccupancyGrid& grid)
    : grid_(grid),
      cols_(grid.cols()), rows_(grid.rows()), n_(grid.cols() * grid.rows()),
      startId_(0), goalId_(0), lastId_(0), km_(0.0f) {}

float DStarLite::heuristic(int a, int b) const {
    const float dx = static_cast<float>(a % cols_ - b % cols_);
    const float dy = static_cast<float>(a / cols_ - b / cols_);
    return std::sqrt(dx * dx + dy * dy) * grid_.resolution();
}

float DStarLite::edgeCost(int a, int b) const {
    const int ax = a % cols_, ay = a / cols_;
    const int bx = b % cols_, by = b / cols_;
    if (grid_.occupied({ax, ay}) || grid_.occupied({bx, by})) return INF;
    const bool diag = (ax != bx) && (ay != by);
    return grid_.resolution() * (diag ? 1.41421356f : 1.0f);
}

int DStarLite::neighborsOf(int u, int* out) const {
    const int ux = u % cols_, uy = u / cols_;
    int count = 0;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            const int nx = ux + dx, ny = uy + dy;
            if (nx < 0 || nx >= cols_ || ny < 0 || ny >= rows_) continue;
            out[count++] = ny * cols_ + nx;
        }
    return count;
}

DStarLite::Key DStarLite::calcKey(int u) const {
    const float m = std::min(g_[u], rhs_[u]);
    Key k;
    k.k2 = m;
    k.k1 = (m >= INF) ? INF : (m + heuristic(startId_, u) + km_);
    return k;
}

void DStarLite::queueInsert(int u, Key k) {
    open_.insert({k.k1, k.k2, u});
    inOpen_[u] = k;
}

void DStarLite::queueRemove(int u) {
    auto it = inOpen_.find(u);
    if (it == inOpen_.end()) return;
    open_.erase({it->second.k1, it->second.k2, u});
    inOpen_.erase(it);
}

void DStarLite::updateVertex(int u) {
    if (u != goalId_) {
        int nb[8];
        const int c = neighborsOf(u, nb);
        float best = INF;
        for (int i = 0; i < c; ++i) {
            const float ec = edgeCost(u, nb[i]);
            if (ec >= INF || g_[nb[i]] >= INF) continue;
            best = std::min(best, ec + g_[nb[i]]);
        }
        rhs_[u] = best;
    }
    queueRemove(u);
    if (std::fabs(g_[u] - rhs_[u]) > EPS)
        queueInsert(u, calcKey(u));
}

void DStarLite::plan(Cell start, Cell goal) {
    startId_ = id(start.cx, start.cy);
    goalId_  = id(goal.cx, goal.cy);
    lastId_  = startId_;
    km_ = 0.0f;
    g_.assign(n_, INF);
    rhs_.assign(n_, INF);
    open_.clear();
    inOpen_.clear();
    rhs_[goalId_] = 0.0f;
    queueInsert(goalId_, calcKey(goalId_));

    // Erstplanung: die GESAMTE erreichbare Karte vom Ziel aus expandieren.
    // Wir haben eine vollstaendige Karte, und ein vollstaendiges g-Feld sorgt
    // dafuer, dass eine spaetere inkrementelle Aenderung auch dann repariert
    // wird, wenn der Ausweichweg vorher nicht "auf dem Weg" lag.
    expandAll();
}

void DStarLite::expandAll() {
    long guard = 0;
    const long guardMax = 80L * n_ + 1000L;
    while (!open_.empty()) {
        if (++guard > guardMax) break;
        const auto top = *open_.begin();
        const int   u   = std::get<2>(top);
        const Key   knew = calcKey(u);
        if (keyLess(std::get<0>(top), std::get<1>(top), knew.k1, knew.k2)) {
            queueRemove(u);
            queueInsert(u, knew);
            continue;
        }
        if (g_[u] > rhs_[u] + EPS) {
            g_[u] = rhs_[u];
            queueRemove(u);
        } else {
            g_[u] = INF;
            updateVertex(u);
        }
        int nb[8];
        const int c = neighborsOf(u, nb);
        for (int i = 0; i < c; ++i) updateVertex(nb[i]);
    }
}

void DStarLite::setStart(Cell start) {
    const int s = id(start.cx, start.cy);
    if (s == startId_) return;
    km_ += heuristic(lastId_, s);
    lastId_  = s;
    startId_ = s;
}

void DStarLite::cellChanged(Cell c) {
    const int u = id(c.cx, c.cy);
    updateVertex(u);
    int nb[8];
    const int cnt = neighborsOf(u, nb);
    for (int i = 0; i < cnt; ++i) updateVertex(nb[i]);
}

void DStarLite::computeShortestPath() {
    long guard = 0;
    const long guardMax = 80L * n_ + 1000L;

    while (!open_.empty()) {
        if (++guard > guardMax) {
            std::fprintf(stderr, "[DStarLite] computeShortestPath: guard hit (%ld)\n", guard);
            break;
        }

        const auto top = *open_.begin();
        const float tk1 = std::get<0>(top);
        const float tk2 = std::get<1>(top);
        const int   u   = std::get<2>(top);

        const Key ks = calcKey(startId_);
        const bool topLess = keyLess(tk1, tk2, ks.k1, ks.k2);
        if (!topLess && std::fabs(rhs_[startId_] - g_[startId_]) <= EPS)
            break;

        const Key knew = calcKey(u);
        if (keyLess(tk1, tk2, knew.k1, knew.k2)) {
            queueRemove(u);
            queueInsert(u, knew);
        } else if (g_[u] > rhs_[u] + EPS) {
            g_[u] = rhs_[u];
            queueRemove(u);
            int nb[8];
            const int c = neighborsOf(u, nb);
            for (int i = 0; i < c; ++i) updateVertex(nb[i]);
        } else {
            g_[u] = INF;
            updateVertex(u);
            int nb[8];
            const int c = neighborsOf(u, nb);
            for (int i = 0; i < c; ++i) updateVertex(nb[i]);
        }
    }
}

bool DStarLite::pathExists() const {
    return g_[startId_] < INF;
}

int DStarLite::finiteGCount() const {
    int c = 0;
    for (float v : g_) if (v < INF) ++c;
    return c;
}

std::vector<Vec2> DStarLite::extractPath() const {
    std::vector<Vec2> out;
    if (g_[startId_] >= INF) return out;

    int cur = startId_;
    out.push_back(grid_.cellToWorld({cur % cols_, cur / cols_}));
    for (int steps = 0; cur != goalId_ && steps < n_; ++steps) {
        int nb[8];
        const int c = neighborsOf(cur, nb);
        int   bestN = -1;
        float bestVal = INF;
        for (int i = 0; i < c; ++i) {
            const float ec = edgeCost(cur, nb[i]);
            if (ec >= INF || g_[nb[i]] >= INF) continue;
            const float val = ec + g_[nb[i]];
            if (val < bestVal) { bestVal = val; bestN = nb[i]; }
        }
        if (bestN < 0) return {};   // Sackgasse
        cur = bestN;
        out.push_back(grid_.cellToWorld({cur % cols_, cur / cols_}));
    }
    if (cur != goalId_) return {};
    return out;
}
