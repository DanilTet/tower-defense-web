#include "Pathfinder.h"
#include <algorithm>
#include <cmath>

Pathfinder::Pathfinder(int width, int height)
    : m_width(width), m_height(height), m_currentEpoch(0)
{
    m_nodes.resize(m_width * m_height);
}

int Pathfinder::getOctileDistance(glm::ivec2 a, glm::ivec2 b) const {
    int distX = std::abs(a.x - b.x);
    int distY = std::abs(a.y - b.y);
    if (distX > distY) {
        return 14 * distY + 10 * (distX - distY);
    }
    return 14 * distX + 10 * (distY - distX);
}

bool Pathfinder::isCellWalkable(const Grid& grid, int x, int y) {
    if (x < 0 || x >= grid.getWidth() || y < 0 || y >= grid.getHeight()) return false;
    CellType t = grid.getCellType(x, y);
    if (t == CellType::Chasm) return false; // Обрыв/шурф непроходим для обычного поиска пути
    return t == CellType::Ground || t == CellType::Path || t == CellType::Spawner || t == CellType::Base || t == CellType::Rail;
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

DijkstraMap Pathfinder::generateDijkstraMap(const Grid& grid, const std::vector<glm::ivec2>& targetBases) const {
    DijkstraMap dmap;
    dmap.width = m_width;
    dmap.height = m_height;
    int totalCells = m_width * m_height;
    dmap.distances.assign(totalCells, 999999);
    dmap.bestNext.assign(totalCells, glm::ivec2(-1, -1));

    if (targetBases.empty()) return dmap;

    typedef std::pair<int, int> PQElement;
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;

    for (const auto& base : targetBases) {
        if (base.x >= 0 && base.x < m_width && base.y >= 0 && base.y < m_height) {
            int idx = base.y * m_width + base.x;
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

        int ux = uIdx % m_width;
        int uy = uIdx / m_width;
        glm::ivec2 uPos(ux, uy);

        for (int i = 0; i < 8; ++i) {
            int vx = ux + dx[i];
            int vy = uy + dy[i];

            if (vx < 0 || vx >= m_width || vy < 0 || vy >= m_height) continue;

            glm::ivec2 vPos(vx, vy);
            // В обратном поиске от базы проверяем возможность движения от v к u
            if (!canMove(grid, vPos, uPos)) continue;

            int newCost = cost + stepCosts[i];
            int vIdx = vy * m_width + vx;

            if (newCost < dmap.distances[vIdx]) {
                dmap.distances[vIdx] = newCost;
                dmap.bestNext[vIdx] = uPos; // Следующий шаг из v в сторону базы ведет в u
                pq.push({newCost, vIdx});
            }
        }
    }

    return dmap;
}

std::vector<glm::ivec2> Pathfinder::tracePath(const DijkstraMap& dmap, const Grid& grid, glm::ivec2 startPos) const {
    std::vector<glm::ivec2> path;
    if (!dmap.isReachable(startPos.x, startPos.y)) {
        return path;
    }

    int totalCells = dmap.width * dmap.height;
    glm::ivec2 curr = startPos;
    int steps = 0;

    // Двигаемся по цепочке bestNext, пока не достигнем базы (где distance == 0)
    while (dmap.getDistance(curr.x, curr.y) > 0 && steps < totalCells) {
        glm::ivec2 next = dmap.getNext(curr.x, curr.y);
        if (next.x < 0 || next == curr) break;
        path.push_back(next);
        curr = next;
        steps++;
    }

    return path;
}

std::vector<glm::ivec2> Pathfinder::findPathToAny(const Grid& grid, glm::ivec2 startPos, const std::vector<glm::ivec2>& targetBases, int& outTotalCost) {
    outTotalCost = 999999;
    std::vector<glm::ivec2> path;

    if (startPos.x < 0 || startPos.x >= m_width || startPos.y < 0 || startPos.y >= m_height || targetBases.empty()) {
        return path;
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

    int startIdx = startPos.y * m_width + startPos.x;
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

        int ux = uIdx % m_width;
        int uy = uIdx / m_width;
        glm::ivec2 uPos(ux, uy);

        // Проверяем, достигнута ли любая из целевых баз
        for (const auto& b : targetBases) {
            if (uPos == b) {
                outTotalCost = uNode.gCost;
                glm::ivec2 cur = uPos;
                while (cur != startPos && cur != glm::ivec2(-1, -1)) {
                    path.push_back(cur);
                    int curIdx = cur.y * m_width + cur.x;
                    cur = m_nodes[curIdx].parent;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }
        }

        for (int i = 0; i < 8; ++i) {
            int vx = ux + dx[i];
            int vy = uy + dy[i];
            if (vx < 0 || vx >= m_width || vy < 0 || vy >= m_height) continue;

            glm::ivec2 vPos(vx, vy);
            if (!canMove(grid, uPos, vPos)) continue;

            int vIdx = vy * m_width + vx;
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