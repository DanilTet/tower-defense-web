#include "Pathfinder.h"
#include <cmath>
#include <algorithm>

Pathfinder::Pathfinder(int width, int height)
    : m_width(width), m_height(height) {
    m_nodes.resize(m_width * m_height);
}

int Pathfinder::getOctileDistance(glm::ivec2 a, glm::ivec2 b) const {
    int distX = std::abs(a.x - b.x);
    int distY = std::abs(a.y - b.y);
    if (distX > distY) std::swap(distX, distY);
    return 14 * distX + 10 * (distY - distX);
}

bool Pathfinder::isCellWalkable(const Grid& grid, int x, int y) {
    if (x >= 0 && x < grid.getWidth() && y >= 0 && y < grid.getHeight()) {
        CellType t = grid.getCellType(x, y);
        if (t == CellType::Chasm) return false;
        return t == CellType::Ground || t == CellType::Path || t == CellType::Spawner || t == CellType::Base || t == CellType::Rail;
    }

    if (!grid.hasSidePockets()) return false;

    CellType t = grid.getCellType(x, y);
    if (t == CellType::Empty || t == CellType::Scenery || t == CellType::Chasm) return false;
    return t == CellType::Ground || t == CellType::Platform || t == CellType::Path || t == CellType::Spawner || t == CellType::Base || t == CellType::Rail;
}

bool Pathfinder::canMove(const Grid& grid, glm::ivec2 from, glm::ivec2 to) {
    if (!isCellWalkable(grid, to.x, to.y)) return false;

    int dx = to.x - from.x;
    int dy = to.y - from.y;
    if (std::abs(dx) == 1 && std::abs(dy) == 1) {
        // Проверка срезания углов: обе ортогональные соседние клетки должны быть проходимыми
        if (!isCellWalkable(grid, from.x + dx, from.y) || !isCellWalkable(grid, from.x, from.y + dy)) {
            return false;
        }
    }
    return true;
}

namespace {
struct BoundingBox {
    int minX = 0;
    int minY = 0;
    int maxX = 0;
    int maxY = 0;
    int width() const { return maxX - minX + 1; }
    int height() const { return maxY - minY + 1; }
    int totalCells() const { return width() * height(); }
    int getIndex(int x, int y) const {
        return (y - minY) * width() + (x - minX);
    }
    glm::ivec2 getPos(int idx) const {
        int w = width();
        return glm::ivec2(minX + (idx % w), minY + (idx / w));
    }
    bool contains(int x, int y) const {
        return x >= minX && x <= maxX && y >= minY && y <= maxY;
    }
};

BoundingBox calculateBoundingBox(const Grid& grid, const std::vector<glm::ivec2>& points) {
    int minX = 0;
    int minY = 0;
    int maxX = grid.getWidth() - 1;
    int maxY = grid.getHeight() - 1;

    if (grid.hasSidePockets()) {
        for (const auto& [pos, type] : grid.getOuterCells()) {
            minX = std::min(minX, pos.first);
            maxX = std::max(maxX, pos.first);
            minY = std::min(minY, pos.second);
            maxY = std::max(maxY, pos.second);
        }
        for (const auto& p : points) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }
        minX -= 2;
        minY -= 2;
        maxX += 2;
        maxY += 2;
    }

    BoundingBox bbox;
    bbox.minX = minX;
    bbox.minY = minY;
    bbox.maxX = maxX;
    bbox.maxY = maxY;
    return bbox;
}
}

DijkstraMap Pathfinder::generateDijkstraMap(const Grid& grid, const std::vector<glm::ivec2>& targetBases) const {
    BoundingBox bbox = calculateBoundingBox(grid, targetBases);

    DijkstraMap dmap;
    dmap.minX = bbox.minX;
    dmap.minY = bbox.minY;
    dmap.width = bbox.width();
    dmap.height = bbox.height();
    int totalCells = bbox.totalCells();
    dmap.distances.assign(totalCells, 999999);
    dmap.bestNext.assign(totalCells, glm::ivec2(-1, -1));

    if (targetBases.empty()) return dmap;

    typedef std::pair<int, int> PQElement;
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;

    for (const auto& base : targetBases) {
        if (bbox.contains(base.x, base.y)) {
            int idx = bbox.getIndex(base.x, base.y);
            dmap.distances[idx] = 0;
            dmap.bestNext[idx] = base;
            pq.push({0, idx});
        }
    }

    const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int stepCosts[8] = {14, 10, 14, 10, 10, 14, 10, 14};

    while (!pq.empty()) {
        auto top = pq.top();
        int cost = top.first;
        int uIdx = top.second;
        pq.pop();

        if (cost > dmap.distances[uIdx]) continue;

        glm::ivec2 uPos = bbox.getPos(uIdx);

        for (int i = 0; i < 8; ++i) {
            int vx = uPos.x + dx[i];
            int vy = uPos.y + dy[i];

            if (!bbox.contains(vx, vy)) continue;

            glm::ivec2 vPos(vx, vy);
            if (!canMove(grid, vPos, uPos)) continue;

            int newCost = cost + stepCosts[i];
            int vIdx = bbox.getIndex(vx, vy);

            if (newCost < dmap.distances[vIdx]) {
                dmap.distances[vIdx] = newCost;
                dmap.bestNext[vIdx] = uPos;
                pq.push({newCost, vIdx});
            }
        }
    }

    return dmap;
}

std::vector<glm::ivec2> Pathfinder::tracePath(
    const DijkstraMap& dmap,
    const Grid& grid,
    glm::ivec2 startPos,
    glm::ivec2 preferredDir
) const {
    std::vector<glm::ivec2> path;
    if (!dmap.isReachable(startPos.x, startPos.y)) {
        return path;
    }

    // Фиксированный детерминированный порядок: сначала 4 прямых (ортогональных) шага,
    // затем 4 диагональных — гарантирует слияние путей от разных спавнеров без скрещивания.
    const int dx[8] = { 1, 0, -1,  0,  1,  1, -1, -1 };
    const int dy[8] = { 0, 1,  0, -1,  1, -1,  1, -1 };
    const int stepCosts[8] = { 10, 10, 10, 10, 14, 14, 14, 14 };

    int totalCells = dmap.width * dmap.height;
    glm::ivec2 curr = startPos;
    int steps = 0;

    // Переданный preferredDir применяем только на первых 1–2 шагах (пока путь совпадает с ним),
    // не перезаписывая его последующими шагами трассировки.
    int preferredStepsRemaining = 0;
    if (preferredDir.x != 0 || preferredDir.y != 0) {
        preferredStepsRemaining = 2;
    }

    while (dmap.getDistance(curr.x, curr.y) > 0 && steps < totalCells) {
        int currDist = dmap.getDistance(curr.x, curr.y);
        int bestTotalCost = 999999;
        int bestPriority = -1000;
        bool foundNext = false;
        glm::ivec2 bestNextCell(-1, -1);
        glm::ivec2 bestStepDir(0, 0);

        for (int i = 0; i < 8; ++i) {
            glm::ivec2 candidate(curr.x + dx[i], curr.y + dy[i]);
            if (!dmap.isReachable(candidate.x, candidate.y)) {
                continue;
            }

            int candDist = dmap.getDistance(candidate.x, candidate.y);
            if (candDist >= currDist) {
                continue;
            }

            if (!canMove(grid, curr, candidate)) {
                continue;
            }

            int totalCost = candDist + stepCosts[i];
            if (totalCost > bestTotalCost) {
                continue;
            }

            // Единый детерминированный приоритет: прямые шаги выше диагональных в фиксированном порядке
            int priority = 0;
            if (i < 4) {
                priority = 100 - i;
            } else {
                priority = 50 - i;
            }

            // На стартовых 1–2 шагах отдаём приоритет исходному направлению движения врага
            if (preferredStepsRemaining > 0 && dx[i] == preferredDir.x && dy[i] == preferredDir.y) {
                priority = 1000;
            }

            if (!foundNext || totalCost < bestTotalCost || (totalCost == bestTotalCost && priority > bestPriority)) {
                bestTotalCost = totalCost;
                bestPriority = priority;
                bestNextCell = candidate;
                bestStepDir = glm::ivec2(dx[i], dy[i]);
                foundNext = true;
            }
        }

        if (!foundNext) {
            glm::ivec2 fallbackNext = dmap.getNext(curr.x, curr.y);
            if (fallbackNext.x == -1 && fallbackNext.y == -1) {
                break;
            }
            if (fallbackNext == curr) {
                break;
            }
            if (dmap.getDistance(fallbackNext.x, fallbackNext.y) >= currDist) {
                break;
            }
            bestNextCell = fallbackNext;
            bestStepDir = fallbackNext - curr;
        }

        if (preferredStepsRemaining > 0) {
            if (bestStepDir == preferredDir) {
                preferredStepsRemaining--;
            } else {
                preferredStepsRemaining = 0;
            }
        }

        path.push_back(bestNextCell);
        curr = bestNextCell;
        steps++;
    }

    return path;
}

std::vector<glm::ivec2> Pathfinder::findPathToAny(const Grid& grid, glm::ivec2 startPos, const std::vector<glm::ivec2>& targetBases, int& outTotalCost) {
    outTotalCost = 999999;
    std::vector<glm::ivec2> path;
    if (targetBases.empty()) return path;

    std::vector<glm::ivec2> allPoints = targetBases;
    allPoints.push_back(startPos);
    BoundingBox bbox = calculateBoundingBox(grid, allPoints);

    if (!bbox.contains(startPos.x, startPos.y)) {
        return path;
    }

    int totalCells = bbox.totalCells();
    if (m_nodes.size() < static_cast<size_t>(totalCells)) {
        m_nodes.resize(totalCells);
    }

    m_currentEpoch++;
    uint32_t epoch = m_currentEpoch;

    auto getH = [&](glm::ivec2 pos) -> int {
        int minH = 999999;
        for (const auto& b : targetBases) {
            int d = getOctileDistance(pos, b);
            if (d < minH) minH = d;
        }
        return minH;
    };

    int startIdx = bbox.getIndex(startPos.x, startPos.y);
    AStarNode& sNode = m_nodes[startIdx];
    sNode.epoch = epoch;
    sNode.gCost = 0;
    sNode.hCost = getH(startPos);
    sNode.parent = glm::ivec2(-1, -1);
    sNode.isClosed = false;

    typedef std::pair<int, int> PQNode;
    std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> openQueue;
    openQueue.push({sNode.fCost(), startIdx});

    const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int stepCosts[8] = {14, 10, 14, 10, 10, 14, 10, 14};

    while (!openQueue.empty()) {
        auto top = openQueue.top();
        int uIdx = top.second;
        openQueue.pop();

        AStarNode& uNode = m_nodes[uIdx];
        if (uNode.epoch != epoch || uNode.isClosed) continue;
        uNode.isClosed = true;

        glm::ivec2 uPos = bbox.getPos(uIdx);

        for (const auto& b : targetBases) {
            if (uPos == b) {
                outTotalCost = uNode.gCost;
                glm::ivec2 cur = uPos;
                while (cur != startPos && cur != glm::ivec2(-1, -1)) {
                    path.push_back(cur);
                    int curIdx = bbox.getIndex(cur.x, cur.y);
                    cur = m_nodes[curIdx].parent;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }
        }

        for (int i = 0; i < 8; ++i) {
            int vx = uPos.x + dx[i];
            int vy = uPos.y + dy[i];
            if (!bbox.contains(vx, vy)) continue;

            glm::ivec2 vPos(vx, vy);
            if (!canMove(grid, uPos, vPos)) continue;

            int vIdx = bbox.getIndex(vx, vy);
            AStarNode& vNode = m_nodes[vIdx];

            if (vNode.epoch != epoch) {
                vNode.epoch = epoch;
                vNode.gCost = 999999;
                vNode.hCost = getH(vPos);
                vNode.parent = glm::ivec2(-1, -1);
                vNode.isClosed = false;
            }

            if (vNode.isClosed) continue;

            int newG = uNode.gCost + stepCosts[i];
            if (newG < vNode.gCost) {
                vNode.gCost = newG;
                vNode.parent = uPos;
                openQueue.push({vNode.fCost(), vIdx});
            }
        }
    }

    return path;
}

std::vector<glm::ivec2> Pathfinder::findPath(const Grid& grid, glm::ivec2 startPos, glm::ivec2 targetPos, int& outTotalCost) {
    return findPathToAny(grid, startPos, {targetPos}, outTotalCost);
}
