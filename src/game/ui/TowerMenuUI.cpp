#include "TowerMenuUI.h"
#include "../entities/Tower.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../world/Grid.h"
#include "../gameplay/BuildManager.h"
#include "../world/GameWorld.h"
#include <iostream>
#include <string>

bool TowerMenuUI::processClick(
    double mouseX, double mouseY,
    Tower* selectedTower,
    BuildManager* buildManager,
    GameWorld& world,
    Tower*& outSelectedTower)
{
    if (!selectedTower) return false;

    float cellSize = world.grid->getCellSize();
    glm::vec2 towerPos = world.grid->gridToPixel(selectedTower->getGridX(), selectedTower->getGridY());

    // размеры и позиции кнопок
    glm::vec2 btnSize(cellSize * 0.5f, cellSize * 0.5f);
    glm::vec2 targetBtnSize(cellSize, cellSize * 0.5f);

    glm::vec2 upgPos = towerPos + glm::vec2(0.0f, -btnSize.y - 4.0f);
    glm::vec2 sellPos = towerPos + glm::vec2(cellSize * 0.5f, -btnSize.y - 4.0f);
    glm::vec2 targetPos = towerPos + glm::vec2(0.0f, cellSize + 4.0f);

    // клик по кнопке UPGRADE?
    if (mouseX >= upgPos.x && mouseX <= upgPos.x + btnSize.x && mouseY >= upgPos.y && mouseY <= upgPos.y + btnSize.y) {
        int currentLevel = selectedTower->getLevel();
        int maxAllowed = world.levelData.getMaxTierForTower(selectedTower->getType());
        bool isMaxTier = (currentLevel >= maxAllowed) || (currentLevel >= selectedTower->getMaxLevel());

        if (isMaxTier) {
            return true;
        }

        selectedTower->upgrade(world.playerStats.money);
        return true; // клик обработан
    }

    // клик по кнопке SELL?
    if (mouseX >= sellPos.x && mouseX <= sellPos.x + btnSize.x && mouseY >= sellPos.y && mouseY <= sellPos.y + btnSize.y) {
        buildManager->sellTower(selectedTower, world);
        outSelectedTower = nullptr; // снимаем выделение после продажи
        return true; // клик обработан
    }

    // клик по кнопке TARGET MODE? (для Поршня выбор приоритета цели не применим)
    bool hasTargetMode = (selectedTower->getType() != "Piston");
    if (hasTargetMode && mouseX >= targetPos.x && mouseX <= targetPos.x + targetBtnSize.x && mouseY >= targetPos.y && mouseY <= targetPos.y + targetBtnSize.y) {
        selectedTower->toggleTargetMode(); // переключаем режимчик
        return true;
    }

    return false; // клик был мимо кнопок меню
}

void TowerMenuUI::render(
    Tower* selectedTower,
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    const Texture2D* uiTexture,
    const Grid& gameGrid,
    int maxUpgradeTier)
{
    if (!selectedTower || !renderer || !textRenderer) return;

    float cellSize = gameGrid.getCellSize();
    glm::vec2 towerPos = gameGrid.gridToPixel(selectedTower->getGridX(), selectedTower->getGridY());

    //размерчики и позиции кнопок
    glm::vec2 btnSize(cellSize * 0.5f, cellSize * 0.5f);
    glm::vec2 targetBtnSize(cellSize, cellSize * 0.5f);

    glm::vec2 upgPos = towerPos + glm::vec2(0.0f, -btnSize.y - 4.0f);
    glm::vec2 sellPos = towerPos + glm::vec2(cellSize * 0.5f, -btnSize.y - 4.0f);
    glm::vec2 targetPos = towerPos + glm::vec2(0.0f, cellSize + 4.0f);

    int currentLevel = selectedTower->getLevel();
    int effectiveMaxTier = maxUpgradeTier > 0 ? maxUpgradeTier : 3;
    bool isMaxTier = (currentLevel >= effectiveMaxTier) || (currentLevel >= selectedTower->getMaxLevel());
    glm::vec3 upgColor = isMaxTier ? glm::vec3(0.40f, 0.44f, 0.48f) : glm::vec3(0.2f, 0.8f, 0.2f);

    bool hasTargetMode = (selectedTower->getType() != "Piston");

    // рисуем подложки
    renderer->drawSprite(uiTexture, upgPos, btnSize, 0.0f, upgColor);
    renderer->drawSprite(uiTexture, sellPos, btnSize, 0.0f, glm::vec3(0.8f, 0.2f, 0.2f));
    if (hasTargetMode) {
        renderer->drawSprite(uiTexture, targetPos, targetBtnSize, 0.0f, glm::vec3(0.2f, 0.4f, 0.8f));
    }

    renderer->flush();

    auto drawCenteredText = [&](const std::string& text, glm::vec2 btnPos, glm::vec2 size, glm::vec3 textColor = glm::vec3(1.0f)) {
        float baseWidth = textRenderer->CalculateTextWidth(text, 1.0f);
        if (baseWidth == 0.0f) return;
        float scale = (size.x * 0.80f) / baseWidth;
        if (scale > 0.55f) scale = 0.55f;
        float finalWidth = baseWidth * scale;
        float textX = btnPos.x + (size.x - finalWidth) / 2.0f;
        float textHeight = textRenderer->Characters['H'].Size.y * scale;
        float textY = btnPos.y + (size.y - textHeight) / 2.0f;

        textRenderer->RenderText(text, textX, textY, scale, textColor);
    };

    std::string upgText = "MAX";
    if (!isMaxTier) {
        int cost = selectedTower->getUpgradeCost();
        upgText = (cost <= 0) ? "MAX" : "$" + std::to_string(cost);
    }
    glm::vec3 upgTextColor = isMaxTier ? glm::vec3(0.75f, 0.80f, 0.85f) : glm::vec3(1.0f);
    drawCenteredText(upgText, upgPos, btnSize, upgTextColor);
    drawCenteredText("SELL", sellPos, btnSize);
    if (hasTargetMode) {
        drawCenteredText(selectedTower->getTargetModeString(), targetPos, targetBtnSize);
    }
}