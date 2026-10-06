#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"

class Grid;
class Pathfinder;
class Enemy;

class PathService {
public:
    static std::vector<glm::ivec2> getTargetBasePositions(
        const std::vector<BaseData>& bases,
        int targetBaseId
    );

    static std::vector<std::vector<glm::ivec2>> calculateAllPaths(
        const Grid& grid,
        Pathfinder& pathfinder,
        const std::vector<SpawnerData>& spawners,
        const std::vector<BaseData>& bases
    );

    static bool isPlacementValid(
        Grid& grid,
        Pathfinder& pathfinder,
        const std::vector<SpawnerData>& spawners,
        const std::vector<BaseData>& bases,
        int gridX,
        int gridY,
        std::vector<std::vector<glm::ivec2>>& outNewPaths,
        const std::vector<std::vector<glm::ivec2>>* currentPaths = nullptr,
        const std::vector<std::unique_ptr<Enemy>>* activeEnemies = nullptr
    );
};

