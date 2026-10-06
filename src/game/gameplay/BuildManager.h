#pragma once

#include <glm/glm.hpp>
#include <string>

class Tower;
struct GameWorld;

class BuildManager {
public:
    void tryBuildOrUpgrade(
        glm::vec2 mousePos, // позиция мыши
        const std::string& selectedType,// выбраный тип башни
        GameWorld& world,
        float placementAngle = 270.0f
    );

    void sellTower(
        Tower* tower,
        GameWorld& world
    );
};
