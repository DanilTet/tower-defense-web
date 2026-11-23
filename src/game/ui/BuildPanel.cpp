#include "BuildPanel.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer/SpriteRenderer.h"
#include "renderer/TextRenderer.h"
#include "textures/Texture2D.h"
#include "core/ConfigManager.h"
#include "gameplay/PlayerStats.h"
#include "UICommon.h"

void Buildpanel::initPanelData(const std::vector<std::string>& allowedTowers) {
    auto allTypes = ConfigManager::getAllTowerTypes();
    if (allowedTowers.empty()) {
        m_cachedTowers = allTypes;
    } else {
        m_cachedTowers.clear();
        for (const auto& t : allowedTowers) {
            if (std::find(allTypes.begin(), allTypes.end(), t) != allTypes.end()) {
                m_cachedTowers.push_back(t);
            }
        }
        if (m_cachedTowers.empty()) {
            m_cachedTowers = allTypes;
        }
    }
    m_cachedPanelWidth = calculatePanelWidth(m_cachedTowers.size());
}

void Buildpanel::setAllowedTowers(const std::vector<std::string>& allowedTowers) {
    initPanelData(allowedTowers);
}

float Buildpanel::getBottomBarHeight(int windowWidth, int windowHeight) {
    float scale = GetUIScale(windowWidth, windowHeight);
    return (UI_PANEL_HEIGHT + DOCK_BOTTOM_MARGIN) * scale;
}

glm::vec2 Buildpanel::getUIPanelPos(int windowWidth, int windowHeight) const {
    float scale = GetUIScale(windowWidth, windowHeight);
    float panelW = m_cachedPanelWidth * scale;
    float panelH = UI_PANEL_HEIGHT * scale;
    float x = (static_cast<float>(windowWidth) - panelW) * 0.5f;
    float y = static_cast<float>(windowHeight) - panelH - (DOCK_BOTTOM_MARGIN * scale);
    return glm::vec2(x, y);
}

glm::vec2 Buildpanel::getTowerIconPos(int index, int windowWidth, int windowHeight) const {
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);
    float cardX = panelPos.x + (DOCK_PADDING * scale) + index * ((CARD_WIDTH + DOCK_GAP) * scale);
    float cardW = CARD_WIDTH * scale;
    float iconSize = UI_ICON_SIZE * scale;
    float iconX = cardX + (cardW - iconSize) * 0.5f;
    float iconY = panelPos.y + (DOCK_PADDING * scale) + (18.0f * scale);
    return glm::vec2(iconX, iconY);
}

void Buildpanel::BuildRenderUI(
    const PlayerStats& playerStats,
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    const Texture2D* mainAtlas,
    const Texture2D* uiTexture,
    int windowWidth,
    int windowHeight,
    const std::string& selectedTower,
    const std::vector<std::string>& allowedTowers)
{
    if (!allowedTowers.empty() && allowedTowers != m_cachedTowers) {
        initPanelData(allowedTowers);
    }

    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);
    float panelW = m_cachedPanelWidth * scale;
    float panelH = UI_PANEL_HEIGHT * scale;

    // Плавающий остров дока (полупрозрачная подложка)
    renderer->drawSpriteRGBA(uiTexture, panelPos, glm::vec2(panelW, panelH), 0.0f, glm::vec4(0.06f, 0.08f, 0.11f, 0.75f));

    // Тонкая кайма плавающего острова
    float islandBorderT = 1.5f * scale;
    glm::vec4 islandBorderColor(0.22f, 0.28f, 0.38f, 0.65f);
    renderer->drawSpriteRGBA(uiTexture, panelPos, glm::vec2(panelW, islandBorderT), 0.0f, islandBorderColor);
    renderer->drawSpriteRGBA(uiTexture, panelPos + glm::vec2(0.0f, panelH - islandBorderT), glm::vec2(panelW, islandBorderT), 0.0f, islandBorderColor);
    renderer->drawSpriteRGBA(uiTexture, panelPos, glm::vec2(islandBorderT, panelH), 0.0f, islandBorderColor);
    renderer->drawSpriteRGBA(uiTexture, panelPos + glm::vec2(panelW - islandBorderT, 0.0f), glm::vec2(islandBorderT, panelH), 0.0f, islandBorderColor);

    // Отрисовка карточек каждой башни
    for (size_t i = 0; i < m_cachedTowers.size(); ++i) {
        const std::string& currentType = m_cachedTowers[i];
        const TowerStats& towerstats = ConfigManager::getTowerStats(currentType);

        float cardX = panelPos.x + (DOCK_PADDING * scale) + i * ((CARD_WIDTH + DOCK_GAP) * scale);
        float cardY = panelPos.y + (DOCK_PADDING * scale);
        float cardW = CARD_WIDTH * scale;
        float cardH = CARD_HEIGHT * scale;

        bool canAfford = (playerStats.money >= towerstats.cost);
        bool isSelected = (selectedTower == currentType);

        // Аккуратная темная полупрозрачная плашка карточки (RGBA 0.1, 0.12, 0.16, 0.85)
        glm::vec4 cardColor = isSelected ? glm::vec4(0.14f, 0.18f, 0.24f, 0.95f) : glm::vec4(0.10f, 0.12f, 0.16f, 0.85f);
        renderer->drawSpriteRGBA(uiTexture, glm::vec2(cardX, cardY), glm::vec2(cardW, cardH), 0.0f, cardColor);

        // Тонкая кайма карточки (золотистая при выборе, стальная при доступности, приглушенная при нехватке)
        float cardBorderT = isSelected ? 2.5f * scale : 1.5f * scale;
        glm::vec4 cardBorderColor;
        if (isSelected) {
            cardBorderColor = glm::vec4(1.0f, 0.86f, 0.25f, 1.0f);
        } else if (canAfford) {
            cardBorderColor = glm::vec4(0.24f, 0.32f, 0.44f, 0.85f);
        } else {
            cardBorderColor = glm::vec4(0.35f, 0.18f, 0.18f, 0.65f);
        }
        renderer->drawSpriteRGBA(uiTexture, glm::vec2(cardX, cardY), glm::vec2(cardW, cardBorderT), 0.0f, cardBorderColor);
        renderer->drawSpriteRGBA(uiTexture, glm::vec2(cardX, cardY + cardH - cardBorderT), glm::vec2(cardW, cardBorderT), 0.0f, cardBorderColor);
        renderer->drawSpriteRGBA(uiTexture, glm::vec2(cardX, cardY), glm::vec2(cardBorderT, cardH), 0.0f, cardBorderColor);
        renderer->drawSpriteRGBA(uiTexture, glm::vec2(cardX + cardW - cardBorderT, cardY), glm::vec2(cardBorderT, cardH), 0.0f, cardBorderColor);

        // Иконка башни по центру карточки
        float iconSize = UI_ICON_SIZE * scale;
        float iconX = cardX + (cardW - iconSize) * 0.5f;
        float iconY = cardY + 18.0f * scale;
        glm::vec3 drawColor = canAfford ? towerstats.color : glm::vec3(0.35f, 0.35f, 0.38f);

        std::string regionName = "tower_basic";
        if (currentType == "Mercury") regionName = "tower_mercury";
        else if (currentType == "Piston") regionName = "tower_piston";
        SpriteUV towerUV = ConfigManager::getUV("main_atlas", regionName);
        renderer->drawSprite(mainAtlas, glm::vec2(iconX, iconY), glm::vec2(iconSize), 0.0f, drawColor, towerUV);
    }

    renderer->flush();

    // Текстовый слой карточек
    for (size_t i = 0; i < m_cachedTowers.size(); ++i) {
        const std::string& currentType = m_cachedTowers[i];
        const TowerStats& towerstats = ConfigManager::getTowerStats(currentType);

        float cardX = panelPos.x + (DOCK_PADDING * scale) + i * ((CARD_WIDTH + DOCK_GAP) * scale);
        float cardY = panelPos.y + (DOCK_PADDING * scale);
        float cardW = CARD_WIDTH * scale;
        float cardH = CARD_HEIGHT * scale;

        bool canAfford = (playerStats.money >= towerstats.cost);

        // Хоткей [1], [2], [3] сверху карточки
        if (i < 9) {
            std::string hotkeyText = "[" + std::to_string(i + 1) + "]";
            float hkScale = 0.36f * scale;
            textRenderer->RenderText(hotkeyText, cardX + 6.0f * scale, cardY + 4.0f * scale, hkScale, glm::vec3(1.0f, 0.84f, 0.35f));
        }

        // Цена башни снизу зеленым цветом
        std::string costText = "$" + std::to_string(towerstats.cost);
        float fontScale = 0.44f * scale;
        float textWidth = textRenderer->CalculateTextWidth(costText, fontScale);
        float textX = cardX + (cardW - textWidth) * 0.5f;
        float textY = cardY + cardH - 17.0f * scale;
        glm::vec3 textColor = canAfford ? glm::vec3(0.35f, 1.0f, 0.45f) : glm::vec3(1.0f, 0.35f, 0.35f);
        textRenderer->RenderText(costText, textX, textY, fontScale, textColor);
    }
}


bool Buildpanel::checkClick(float mouseX, float mouseY, int windowWidth, int windowHeight, std::string& selectedTower) {
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);
    float panelW = m_cachedPanelWidth * scale;
    float panelH = UI_PANEL_HEIGHT * scale;

    // Если курсор вне плавающего острова дока
    if (mouseX < panelPos.x || mouseX > panelPos.x + panelW ||
        mouseY < panelPos.y || mouseY > panelPos.y + panelH) {
        return false;
    }

    // Проверяем клик по карточкам башен
    for (size_t i = 0; i < m_cachedTowers.size(); ++i) {
        float cardX = panelPos.x + (DOCK_PADDING * scale) + i * ((CARD_WIDTH + DOCK_GAP) * scale);
        float cardY = panelPos.y + (DOCK_PADDING * scale);
        float cardW = CARD_WIDTH * scale;
        float cardH = CARD_HEIGHT * scale;

        if (mouseX >= cardX && mouseX <= cardX + cardW &&
            mouseY >= cardY && mouseY <= cardY + cardH) {
            std::string clickedType = m_cachedTowers[i];
            if (selectedTower == clickedType) {
                selectedTower = ""; // повторный клик снимает выделение
            } else {
                selectedTower = clickedType;
            }
            return true;
        }
    }

    // Клик на отступах/фоне дока поглощается
    return true;
}

bool Buildpanel::selectTowerByIndex(size_t index, std::string& selectedTower) {
    if (index < m_cachedTowers.size()) {
        selectedTower = m_cachedTowers[index];
        return true;
    }
    return false;
}
