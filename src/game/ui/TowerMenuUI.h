#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"

class Tower;
class SpriteRenderer;
class TextRenderer;
class Texture2D;
class BuildManager;
class Grid;
struct GameWorld;

class TowerMenuUI {
public:
    // обработка клика: возвращает true если клик пришелся по кнопке меню
    bool processClick(
        double mouseX, double mouseY,
        Tower* selectedTower,
        BuildManager* buildManager,
        GameWorld& world,
        Tower*& outSelectedTower
    );

    // отрисовка меню
    void render(
        Tower* selectedTower,
        SpriteRenderer* renderer,
        TextRenderer* textRenderer,
        std::shared_ptr<Texture2D> cellTexture, // используем базовую текстуру как заглушку
        const Grid& gameGrid
    );
};