#pragma once
#include <vector>
#include <queue>
#include <algorithm>
#include <glm/glm.hpp>
#include "Grid.h"

// Карта расстояний (Dijkstra Map / Flow Field) от целевых баз
struct DijkstraMap {
    int minX = 0;
    int minY = 0;
    int width = 0;
    int height = 0;
    std::vector<int> distances;        // Flat array: (y - minY) * width + (x - minX). 999999 = недостижимо
    std::vector<glm::ivec2> bestNext;  // Следующая оптимальная клетка в сторону базы

    int getIndex(int x, int y) const {
        return (y - minY) * width + (x - minX);
    }

    bool isReachable(int x, int y) const {
        if (x < minX || x >= minX + width || y < minY || y >= minY + height) return false;
        return distances[getIndex(x, y)] < 900000;
    }

    int getDistance(int x, int y) const {
        if (x < minX || x >= minX + width || y < minY || y >= minY + height) return 999999;
        return distances[getIndex(x, y)];
    }

    glm::ivec2 getNext(int x, int y) const {
        if (x < minX || x >= minX + width || y < minY || y >= minY + height) return glm::ivec2(-1, -1);
        return bestNext[getIndex(x, y)];
    }
};

class Pathfinder {
private:
    int m_width = 0;
    int m_height = 0;

    // Внутренняя структура узла для A* с меткой эпохи для O(1) сброса
    struct AStarNode {
        int gCost = 999999;
        int hCost = 0;
        glm::ivec2 parent = glm::ivec2(-1, -1);
        uint32_t epoch = 0;
        bool isClosed = false;

        int fCost() const { return gCost + hCost; }
    };

    mutable std::vector<AStarNode> m_nodes; // Flat vector
    mutable uint32_t m_currentEpoch = 0;

    int getOctileDistance(glm::ivec2 a, glm::ivec2 b) const;

public:
    Pathfinder(int gridWidth, int gridHeight);

    // Проверка проходимости клетки и срезания углов
    static bool isCellWalkable(const Grid& grid, int x, int y);
    static bool canMove(const Grid& grid, glm::ivec2 from, glm::ivec2 to);

    // Построение обратной карты расстояний Dijkstra от заданных баз (за 1 проход волны)
    DijkstraMap generateDijkstraMap(const Grid& grid, const std::vector<glm::ivec2>& targetBases) const;

    // Восстановление маршрута из карты расстояний (градиентный спуск за O(L))
    // preferredDir — предпочтительное направление движения (для сохранения прямой линии среди равнозначных соседей)
    std::vector<glm::ivec2> tracePath(
        const DijkstraMap& dmap,
        const Grid& grid,
        glm::ivec2 startPos,
        glm::ivec2 preferredDir = glm::ivec2(0, 0)
    ) const;

    // Высокопроизводительный A* к одной цели (обратная совместимость)
    std::vector<glm::ivec2> findPath(const Grid& grid, glm::ivec2 startPos, glm::ivec2 targetPos, int& outTotalCost);

    // Высокопроизводительный многоцелевой A* (останавливается на ПЕРВОЙ достигнутой базе)
    std::vector<glm::ivec2> findPathToAny(const Grid& grid, glm::ivec2 startPos, const std::vector<glm::ivec2>& targetBases, int& outTotalCost);
};
