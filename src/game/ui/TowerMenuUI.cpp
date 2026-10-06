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
        int cost = selectedTower->getUpgradeCost();
        if (selectedTower->upgrade(world.playerStats.money)) {
            std::cout << "Tower upgraded!" << std::endl;
        }
        else {
            std::cout << "Not enough money for upgrade. Need: $" << cost << std::endl;
        }
        return true; // клик обработан
    }

    // клик по кнопке SELL?
    if (mouseX >= sellPos.x && mouseX <= sellPos.x + btnSize.x && mouseY >= sellPos.y && mouseY <= sellPos.y + btnSize.y) {
        buildManager->sellTower(selectedTower, world);
        outSelectedTower = nullptr; // снимаем выделение после продажи
        return true; // клик обработан
    }

    // клик по кнопке TARGET MODE?
    if (mouseX >= targetPos.x && mouseX <= targetPos.x + targetBtnSize.x && mouseY >= targetPos.y && mouseY <= targetPos.y + targetBtnSize.y) {
        selectedTower->toggleTargetMode(); // переключаем режимчик
        std::cout << "Target mode switched!" << std::endl;
        return true;
    }

    return false; // клик был мимо кнопок меню
}

void TowerMenuUI::render(
    Tower* selectedTower,
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    std::shared_ptr<Texture2D> uiTexture,
    const Grid& gameGrid)
{
    if (!selectedTower) return;

    float cellSize = gameGrid.getCellSize();
    glm::vec2 towerPos = gameGrid.gridToPixel(selectedTower->getGridX(), selectedTower->getGridY());

    //размерчики и позиции кнопок
    glm::vec2 btnSize(cellSize * 0.5f, cellSize * 0.5f);
    glm::vec2 targetBtnSize(cellSize, cellSize * 0.5f);

    glm::vec2 upgPos = towerPos + glm::vec2(0.0f, -btnSize.y - 4.0f);
    glm::vec2 sellPos = towerPos + glm::vec2(cellSize * 0.5f, -btnSize.y - 4.0f);
    glm::vec2 targetPos = towerPos + glm::vec2(0.0f, cellSize + 4.0f);

    // рисуем подложки
    renderer->drawSprite(uiTexture, upgPos, btnSize, 0.0f, glm::vec3(0.2f, 0.8f, 0.2f));
    renderer->drawSprite(uiTexture, sellPos, btnSize, 0.0f, glm::vec3(0.8f, 0.2f, 0.2f));
    renderer->drawSprite(uiTexture, targetPos, targetBtnSize, 0.0f, glm::vec3(0.2f, 0.4f, 0.8f));

    renderer->flush();

    auto drawCenteredText = [&](const std::string& text, glm::vec2 btnPos, glm::vec2 size) {
        float baseWidth = textRenderer->CalculateTextWidth(text, 1.0f);
        if (baseWidth == 0.0f) return;
        float scale = (size.x * 0.80f) / baseWidth;
        if (scale > 0.55f) scale = 0.55f;
        float finalWidth = baseWidth * scale;
        float textX = btnPos.x + (size.x - finalWidth) / 2.0f;
        float textHeight = textRenderer->Characters['H'].Size.y * scale;
        float textY = btnPos.y + (size.y - textHeight) / 2.0f;

        textRenderer->RenderText(text, textX, textY, scale, glm::vec3(1.0f));
    };

    int cost = selectedTower->getUpgradeCost();
    std::string upgText = (cost == 0) ? "MAX" : "$" + std::to_string(cost);
    drawCenteredText(upgText, upgPos, btnSize);
    drawCenteredText("SELL", sellPos, btnSize);
    drawCenteredText(selectedTower->getTargetModeString(), targetPos, targetBtnSize);
}