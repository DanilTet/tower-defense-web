#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "entities/Tower.h"

class SpriteRenderer;
class Texture2D;
class Grid;
class Enemy;
struct PlayerStats;

class PlacementUI {
private:

public:
    void renderHologram(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> mainAtlas,// текстурка башни
        std::shared_ptr<Texture2D> radiusTexture, // текстурка радиуса атаки
        const Grid& gameGrid, // ссылка на сетку поля
        glm::vec2 currentMousePos, // курсор позиция
        const std::string& selectedTower, // выбраная башня
        const PlayerStats& stats, // деняк
        glm::vec2 panelPos, // позиция менбшки
        bool hasValidPath, // есть ли путь чтобы пк не взорвался
        const std::vector<std::unique_ptr<Enemy>>* activeEnemies = nullptr,
        float placementAngle = 270.0f // угол ориентации (для поршня)
    );
};