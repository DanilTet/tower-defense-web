#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <cstdint>
#include "entities/Tower.h"
#include "core/LevelManager.h"
#include <vector>

class SpriteRenderer;
class Texture2D;
class Grid;
class Enemy;
struct PlayerStats;
class Pathfinder;

// Данные для проверки блокировки маршрута (невладеющие указатели)
struct PlacementPathContext {
    Pathfinder* pathfinder = nullptr;
    const std::vector<SpawnerData>* spawners = nullptr;
    const std::vector<BaseData>* bases = nullptr;
    const std::vector<std::vector<glm::ivec2>>* currentPaths = nullptr;
};

class PlacementUI {
private:
    glm::ivec2 m_cachedGridPos = glm::ivec2(-9999, -9999);
    const Grid* m_cachedGridPtr = nullptr;
    uint64_t m_cachedGridVersion = 0;
    bool m_cachedBlocksPath = false;
    bool m_hasCachedPlacement = false;
    bool m_cachedHadEnemies = false;
    double m_lastValidationTime = 0.0;
    static constexpr double VALIDATION_REFRESH_INTERVAL = 0.15;

public:
    void invalidateCache();

    void renderHologram(
        SpriteRenderer* renderer,
        const Texture2D* mainAtlas,// текстурка башни
        const Texture2D* radiusTexture, // текстурка радиуса атаки
        Grid& gameGrid, // ссылка на сетку поля
        glm::vec2 currentMousePos, // курсор позиция
        const std::string& selectedTower, // выбраная башня
        const PlayerStats& stats, // деняк
        glm::vec2 panelPos, // позиция менбшки
        bool hasValidPath, // есть ли путь чтобы пк не взорвался
        const std::vector<std::unique_ptr<Enemy>>* activeEnemies = nullptr,
        float placementAngle = 270.0f, // угол ориентации (для поршня)
        const PlacementPathContext* pathContext = nullptr // для подсветки блокировки маршрута
    );
};