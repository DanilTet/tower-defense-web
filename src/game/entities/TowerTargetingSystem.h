#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Tower.h"

class Enemy;
class Grid;

class TowerTargetingSystem {
public:
    static Enemy* selectTarget(
        const glm::vec2& towerCenter,
        float rangePixels,
        TargetMode mode,
        const std::vector<std::unique_ptr<Enemy>>& enemies,
        const Grid& grid
    );

    static bool updateAim(
        float& currentAngle,
        const glm::vec2& towerCenter,
        const glm::vec2& targetCenter,
        float rotationSpeed,
        float dt
    );
};

