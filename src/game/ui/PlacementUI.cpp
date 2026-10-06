#include "PlacementUI.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "world/Grid.h"
#include "core/ConfigManager.h"
#include "gameplay/PlayerStats.h"
#include "entities/Enemy.h"

void PlacementUI::renderHologram(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> mainAtlas,
    std::shared_ptr<Texture2D> radiusTexture,
    const Grid& gameGrid,
    glm::vec2 currentMousePos,
    const std::string& selectedTower,
    const PlayerStats& stats,
    glm::vec2 panelPos,
    bool hasValidPath,
    const std::vector<std::unique_ptr<Enemy>>* activeEnemies,
    float placementAngle) {

    // если рука пустая то выходим
    if (selectedTower.empty()) {
        return;
    }
    // если нету пути то не рисуем голограму
    if (!hasValidPath) return;

    // переводим пиксели мыши в координаты сетки
    glm::ivec2 gridPos = gameGrid.pixelToGrid(currentMousePos);

    // если мышка вне игровой сетки (включая нижнюю панель меню) — не рисуем голограмму
    if (gridPos.x < 0 || gridPos.x >= gameGrid.getWidth() ||
        gridPos.y < 0 || gridPos.y >= gameGrid.getHeight()) {
        return;
    }

    // получаем данные выбранной башни
    TowerStats towerstats = ConfigManager::getTowerStats(selectedTower);

    // проверяем, можно ли тут строить
    bool hasMoney = (stats.money >= towerstats.cost);
    bool canBuildHere = gameGrid.canBuildAt(gridPos.x, gridPos.y);

    // проверяем, не стоит ли на этой клетке живой враг
    bool hasEnemyHere = false;
    if (activeEnemies) {
        for (const auto& enemy : *activeEnemies) {
            if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;
            if (gameGrid.pixelToGrid(enemy->getPixelPos()) == gridPos) {
                hasEnemyHere = true;
                break;
            }
        }
    }

    // задаем цвет голограмы
    glm::vec3 holoColor;

    if (hasMoney && canBuildHere && !hasEnemyHere) {
        holoColor = glm::vec3(0.2f, 1.0f, 0.2f); // Зелёная голограмма все ок
    }
    else {
        holoColor = glm::vec3(1.0f, 0.2f, 0.2f); // Красная голограмма проблема
    }

    // вычисляем пиксельные координаты для центрирования
    float cellSize = gameGrid.getCellSize();
    glm::vec2 cellPixelPos = gameGrid.gridToPixel(gridPos.x, gridPos.y);
    glm::vec2 cellCenter = cellPixelPos + glm::vec2(cellSize / 2.0f);

    // включаем режим свечения
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (selectedTower == "Piston") {
        // Для поршня подсвечиваем строго 1 клетку удара перед бойком
        glm::ivec2 offset(0, -1);
        if (std::abs(placementAngle - 270.0f) < 5.0f || std::abs(placementAngle - (-90.0f)) < 5.0f) offset = glm::ivec2(0, -1);
        else if (std::abs(placementAngle - 0.0f) < 5.0f) offset = glm::ivec2(1, 0);
        else if (std::abs(placementAngle - 90.0f) < 5.0f) offset = glm::ivec2(0, 1);
        else if (std::abs(placementAngle - 180.0f) < 5.0f) offset = glm::ivec2(-1, 0);

        glm::ivec2 targetCell = gridPos + offset;
        if (targetCell.x >= 0 && targetCell.x < gameGrid.getWidth() &&
            targetCell.y >= 0 && targetCell.y < gameGrid.getHeight()) {
            glm::vec2 targetPixelPos = gameGrid.gridToPixel(targetCell.x, targetCell.y);
            renderer->drawSprite(radiusTexture, targetPixelPos, glm::vec2(cellSize), 0.0f, holoColor);
        }
    }
    else {
        // отрисовка радиуса атаки для обычных башен
        float currentPixelRange = towerstats.range * cellSize;
        glm::vec2 radiusSize(currentPixelRange * 2.0f, currentPixelRange * 2.0f);
        glm::vec2 radiusPos = cellCenter - glm::vec2(currentPixelRange);
        renderer->drawSprite(radiusTexture, radiusPos, radiusSize, 0.0f, holoColor);
    }

    // вырезаем башню из атласа
    std::string regionName = "tower_basic";
    if (selectedTower == "Mercury") regionName = "tower_mercury";
    else if (selectedTower == "Piston") regionName = "tower_piston";
    SpriteUV towerUV = ConfigManager::getUV("main_atlas", regionName);

    // угол отрисовки (для поршня поворачиваем спрайт согласно выбранному направлению)
    float drawAngle = 0.0f;
    if (selectedTower == "Piston") {
        drawAngle = placementAngle + 90.0f;
    }

    // отрисовка самой башни
    renderer->drawSprite(mainAtlas, cellPixelPos, glm::vec2(cellSize), drawAngle, holoColor, towerUV);

    // выключаем свечение
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}