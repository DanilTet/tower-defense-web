#include "BuildPanel.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer/SpriteRenderer.h"
#include "renderer/TextRenderer.h"
#include "textures/Texture2D.h"
#include "core/ConfigManager.h"
#include "gameplay/PlayerStats.h"
#include "UICommon.h"

void Buildpanel::initPanelData() {
    // получаем типы башен один раз и сохраняем в кеш
    m_cachedTowers = ConfigManager::getAllTowerTypes();
    // рассичивыем базовую ширину
    m_cachedPanelWidth = calculatePanelWidth(m_cachedTowers.size());
}

float Buildpanel::getBottomBarHeight(int windowWidth, int windowHeight) {
    float scale = GetUIScale(windowWidth, windowHeight);
    // Высота черной полосы дока: высота панели + гарантированный отступ 16px (по 8px сверху и снизу)
    return std::max((UI_PANEL_HEIGHT + 16.0f) * scale, 85.0f);
}

glm::vec2 Buildpanel::getUIPanelPos(int windowWidth, int windowHeight) const {
    float scale = GetUIScale(windowWidth, windowHeight);
    float barH = getBottomBarHeight(windowWidth, windowHeight);
    float panelW = m_cachedPanelWidth * scale;
    float x = (static_cast<float>(windowWidth) - panelW) * 0.5f;
    // Размещаем карточку строго внутри дока с отступом 8px от верхней разделительной полосы
    float barY = static_cast<float>(windowHeight) - barH;
    float y = barY + 8.0f * scale;
    return glm::vec2(x, y);
}

glm::vec2 Buildpanel::getTowerIconPos(int index, int windowWidth, int windowHeight) const {
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);
    float startX = (m_cachedTowers.size() == 1) ? (m_cachedPanelWidth - UI_ICON_SIZE) * 0.5f : UI_OFFSET_X;
    return glm::vec2(
        panelPos.x + (startX * scale) + (index * UI_ICON_PADDING * scale),
        panelPos.y + (UI_OFFSET_Y * scale)
    );
}

void Buildpanel::BuildRenderUI(
    const PlayerStats& playerStats,
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    std::shared_ptr<Texture2D> mainAtlas,
    std::shared_ptr<Texture2D> uiTexture,
    int windowWidth,
    int windowHeight,
    const std::string& selectedTower)
{
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight); // позиция менюшки

    // рисуем фон панели
    renderer->drawSprite(uiTexture, panelPos, glm::vec2(m_cachedPanelWidth * scale, UI_PANEL_HEIGHT * scale), 0.0f, glm::vec3(0.12f, 0.12f, 0.15f));

    // режем дефолтную башню
    SpriteUV towerUV = ConfigManager::getUV("main_atlas", "tower_basic");

    // рисуем каждую башню
    for (size_t i = 0; i < m_cachedTowers.size(); ++i) {
        std::string currentType = m_cachedTowers[i];
        TowerStats towerstats = ConfigManager::getTowerStats(currentType);

        // позиция иконок
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);

        bool canAfford = (playerStats.money >= towerstats.cost); // определяем хватает ли денег

        // цвет башни
        glm::vec3 drawColor;

        if (canAfford) {
            drawColor = towerstats.color;
        }
        else {
            drawColor = glm::vec3(0.2f, 0.2f, 0.2f);
        }

        // выделяем желтой рамкой выбраную башню
        if (selectedTower == currentType) {
            renderer->drawSprite(uiTexture, iconPos - glm::vec2(4.0f * scale), glm::vec2((UI_ICON_SIZE + 8.0f) * scale), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f));
        }

        // рисуем иконку башни
        std::string regionName = "tower_basic";
        if (currentType == "Mercury") regionName = "tower_mercury";
        else if (currentType == "Piston") regionName = "tower_piston";
        SpriteUV towerUV = ConfigManager::getUV("main_atlas", regionName);
        renderer->drawSprite(mainAtlas, iconPos, glm::vec2(UI_ICON_SIZE * scale), 0.0f, drawColor, towerUV);
    }

    renderer->flush(); //рисуем батч на экран

    for (size_t i = 0; i < m_cachedTowers.size(); ++i) {
        std::string currentType = m_cachedTowers[i];
        TowerStats towerstats = ConfigManager::getTowerStats(currentType);
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);

        bool canAfford = (playerStats.money >= towerstats.cost);

        glm::vec3 textColor;

        if (canAfford) {
            textColor = glm::vec3(0.4f, 1.0f, 0.4f);
        }
        else {
            textColor = glm::vec3(1.0f, 0.3f, 0.3f);
        }

        std::string costText = currentType + ": $" + std::to_string(towerstats.cost);
        float fontScale = 0.44f * scale;
        float textWidth = textRenderer->CalculateTextWidth(costText, fontScale);
        float textX = iconPos.x + (UI_ICON_SIZE * scale * 0.5f) - (textWidth * 0.5f);
        textRenderer->RenderText(costText,
            textX, iconPos.y + (UI_ICON_SIZE * scale) + (6.0f * scale), fontScale, textColor);

        if (i < 9) {
            std::string hotkeyText = "[" + std::to_string(i + 1) + "]";
            float hkScale = 0.38f * scale;
            textRenderer->RenderText(hotkeyText, iconPos.x + 2.0f * scale, iconPos.y + 2.0f * scale, hkScale, glm::vec3(0.95f, 0.85f, 0.4f));
        }
    }
}


bool Buildpanel::checkClick(float mouseX, float mouseY, int windowWidth, int windowHeight, std::string& selectedTower) {
    
    //проверка клика по Ui
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);

    // если мышка вне прямоугольника панели
    if (mouseX < panelPos.x || mouseX > panelPos.x + (m_cachedPanelWidth * scale) ||
        mouseY < panelPos.y || mouseY > panelPos.y + (UI_PANEL_HEIGHT * scale)) {
        return false;
    }

    // проверяем, по какой именно башне кликнули
    for (int i = 0; i < m_cachedTowers.size(); ++i) {
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);
        float iconSizeScaled = UI_ICON_SIZE * scale;

        if (mouseX >= iconPos.x && mouseX <= iconPos.x + iconSizeScaled &&
            mouseY >= iconPos.y && mouseY <= iconPos.y + iconSizeScaled) {
            // записуем имя башни из списка
            selectedTower = m_cachedTowers[i];
            return true;
            //AudioManager::playSound("res/sounds/build.wav", 0.5f); // Звук клика по кнопке
        }
    }

    return true;
}

bool Buildpanel::selectTowerByIndex(size_t index, std::string& selectedTower) {
    if (index < m_cachedTowers.size()) {
        selectedTower = m_cachedTowers[index];
        return true;
    }
    return false;
}
