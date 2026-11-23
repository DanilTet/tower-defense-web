#include "LevelSelectState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "../ui/level_select/CampaignTabView.h"
#include "../ui/level_select/CustomMapsTabView.h"
#include "../core/CampaignManager.h"
#include "../core/LocalizationManager.h"
#include "../core/SettingsManager.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../textures/Texture2D.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

LevelTab LevelSelectState::s_lastActiveTab = LevelTab::Campaign;

LevelSelectState::LevelSelectState(GameStateManager& stateManager, int width, int height,
                                   std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer,
                                   LevelTab initialTab)
    : m_stateManager(stateManager),
      m_width(width),
      m_height(height),
      m_renderer(renderer),
      m_textRenderer(textRenderer),
      m_activeTab(initialTab)
{
    m_whiteTexture = ResourceManager::getWhiteTexture();
    m_mousePressedLastFrame = true;
    CampaignManager::init();
    m_isDevMode = CampaignManager::isDevMode();
    s_lastActiveTab = m_activeTab;

    m_campaignTab = std::make_unique<CampaignTabView>(m_stateManager, m_renderer, m_textRenderer, m_whiteTexture);
    m_customTab = std::make_unique<CustomMapsTabView>(m_stateManager, m_renderer, m_textRenderer, m_whiteTexture);
}

LevelSelectState::~LevelSelectState() = default;

void LevelSelectState::init() {
    m_isDevMode = CampaignManager::isDevMode();
    float uiScale = SettingsManager::getUIScaleMultiplier();

    // Размеры и позиции вкладок
    float tabH = std::clamp(34.0f * uiScale, 26.0f, 44.0f);
    float tabY = std::clamp(14.0f + 20.0f * uiScale, 14.0f, 50.0f);
    float tabW = std::clamp(160.0f * uiScale, 110.0f, 220.0f);
    float tabGap = std::clamp(8.0f * uiScale, 6.0f, 14.0f);
    float startTabsX = std::clamp(28.0f * uiScale, 16.0f, 40.0f);

    m_tabCampaign.pos = glm::vec2(startTabsX, tabY);
    m_tabCampaign.size = glm::vec2(tabW, tabH);
    m_tabCampaign.text = LOC("LEVEL_TAB_CAMPAIGN");

    m_tabCustom.pos = glm::vec2(startTabsX + tabW + tabGap, tabY);
    m_tabCustom.size = glm::vec2(tabW, tabH);
    m_tabCustom.text = LOC("LEVEL_TAB_CUSTOM");

    // Бейдж-кнопка переключения Dev Mode
    float fDevBadge = std::clamp(0.44f * uiScale, 0.32f, 0.55f);
    std::string badgeText = m_isDevMode ? LOC("CAMPAIGN_DEV_MODE_ON") : "[ DEV MODE: ВЫКЛ ]";
    float bW = m_textRenderer ? (m_textRenderer->CalculateTextWidth(badgeText, fDevBadge) + 18.0f) : (140.0f * uiScale);
    float bX = m_tabCustom.pos.x + m_tabCustom.size.x + tabGap;
    m_btnDevModeToggle.pos = glm::vec2(bX, tabY);
    m_btnDevModeToggle.size = glm::vec2(bW, tabH);
    m_btnDevModeToggle.text = badgeText;

    // Нижняя навигация: кнопка Назад
    float navBtnH = std::clamp(38.0f * uiScale, 26.0f, 52.0f);
    float bottomY = static_cast<float>(m_height) - navBtnH - 14.0f;
    float backBtnW = std::clamp(160.0f * uiScale, 110.0f, 220.0f);
    m_btnBack.size = glm::vec2(backBtnW, navBtnH);
    m_btnBack.pos = glm::vec2(startTabsX, bottomY);
    m_btnBack.text = LOC("LEVEL_BTN_BACK");

    if (m_campaignTab) {
        m_campaignTab->init(m_width, m_height, tabY, tabH, bottomY, navBtnH, m_isDevMode);
    }
    if (m_customTab) {
        m_customTab->init(m_width, m_height, tabY, tabH, bottomY, navBtnH);
    }
}

void LevelSelectState::cleanup() {}

bool LevelSelectState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void LevelSelectState::drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const Texture2D* tex) {
    if (m_renderer) {
        m_renderer->drawSprite(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

void LevelSelectState::drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const Texture2D* tex) {
    if (m_renderer) {
        m_renderer->drawSpriteRGBA(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

void LevelSelectState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_mousePos = glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY));

    bool leftDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    if (m_suppressClickUntilRelease) {
        if (!leftDown) {
            m_suppressClickUntilRelease = false;
            m_mousePressedLastFrame = false;
        } else {
            return;
        }
    }

    // Хоткей переключения Dev Mode: Ctrl + Shift + D
    bool ctrlDown = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
    bool shiftDown = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool dDown = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);

    if (ctrlDown && shiftDown && dDown) {
        if (!m_ctrlShiftDWasDown) {
            m_ctrlShiftDWasDown = true;
            CampaignManager::toggleDevMode();
            m_isDevMode = CampaignManager::isDevMode();
            std::cout << "[LevelSelect] Dev Mode toggled via shortcut: " << (m_isDevMode ? "ON" : "OFF") << std::endl;
            init();
            return;
        }
    } else {
        m_ctrlShiftDWasDown = false;
    }

    bool leftPressed = (leftDown && !m_mousePressedLastFrame);

    // Сначала даем обработать ввод активной вкладке (модалки, карточки, поиск)
    if (m_activeTab == LevelTab::Campaign && m_campaignTab) {
        if (m_campaignTab->processInput(window, m_mousePos, leftDown, leftPressed, dt)) {
            m_mousePressedLastFrame = leftDown;
            return;
        }
    } else if (m_activeTab == LevelTab::Custom && m_customTab) {
        if (m_customTab->processInput(window, m_mousePos, leftDown, leftPressed, dt)) {
            m_mousePressedLastFrame = leftDown;
            return;
        }
    }

    // Если модалки не перехватили Escape, выходим в главное меню
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
        return;
    }

    // Обработка общих кликов
    if (leftPressed) {
        m_mousePressedLastFrame = true;

        // Переключение вкладок
        if (isPointInRect(m_mousePos, m_tabCampaign.pos, m_tabCampaign.size)) {
            m_activeTab = LevelTab::Campaign;
            s_lastActiveTab = m_activeTab;
            if (m_campaignTab) m_campaignTab->resetPage();
            init();
            return;
        }
        if (isPointInRect(m_mousePos, m_tabCustom.pos, m_tabCustom.size)) {
            m_activeTab = LevelTab::Custom;
            s_lastActiveTab = m_activeTab;
            if (m_customTab) m_customTab->resetPage();
            init();
            return;
        }

        // Клик по бейджу переключения Dev Mode
        if (isPointInRect(m_mousePos, m_btnDevModeToggle.pos, m_btnDevModeToggle.size)) {
            CampaignManager::toggleDevMode();
            m_isDevMode = CampaignManager::isDevMode();
            std::cout << "[LevelSelect] Dev Mode toggled via click: " << (m_isDevMode ? "ON" : "OFF") << std::endl;
            init();
            return;
        }

        // Кнопка Назад
        if (isPointInRect(m_mousePos, m_btnBack.pos, m_btnBack.size)) {
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }
    } else if (!leftDown) {
        m_mousePressedLastFrame = false;
    }
}

void LevelSelectState::update(float dt) {
    if (m_activeTab == LevelTab::Campaign && m_campaignTab) {
        m_campaignTab->update(dt);
    } else if (m_activeTab == LevelTab::Custom && m_customTab) {
        m_customTab->update(dt);
    }
}

void LevelSelectState::render() {
    if (!m_renderer || !m_textRenderer) return;

    float uiScale = SettingsManager::getUIScaleMultiplier();
    float fontTab = std::clamp(0.55f * uiScale, 0.40f, 0.72f);
    float fNav = std::clamp(0.54f * uiScale, 0.40f, 0.70f);

    // =========================================================================
    // PASS 1: ГЕОМЕТРИЯ (ВСЕ ФОНЫ, ПЛАШКИ И КНОПКИ В БАТЧЕ)
    // =========================================================================
    m_renderer->beginBatch();

    // 1. Верхние вкладки
    auto drawTabBg = [&](const UIButton& tab, bool active) {
        glm::vec3 bg = active ? glm::vec3(0.20f, 0.42f, 0.65f) : glm::vec3(0.12f, 0.14f, 0.20f);
        drawQuad(tab.pos, tab.size, bg, m_whiteTexture);
        drawQuad(glm::vec2(tab.pos.x, tab.pos.y + tab.size.y - 3.0f), glm::vec2(tab.size.x, 3.0f),
                 active ? glm::vec3(0.35f, 0.75f, 1.0f) : glm::vec3(0.08f, 0.10f, 0.14f), m_whiteTexture);
    };
    drawTabBg(m_tabCampaign, m_activeTab == LevelTab::Campaign);
    drawTabBg(m_tabCustom, m_activeTab == LevelTab::Custom);

    // 2. Dev Mode плашка-переключатель
    glm::vec3 devToggleBg = m_isDevMode ? glm::vec3(0.75f, 0.45f, 0.12f) : glm::vec3(0.16f, 0.18f, 0.24f);
    drawQuad(m_btnDevModeToggle.pos, m_btnDevModeToggle.size, devToggleBg, m_whiteTexture);

    // 3. Геометрия активной вкладки
    if (m_activeTab == LevelTab::Campaign && m_campaignTab) {
        m_campaignTab->renderGeometry(uiScale, m_isDevMode);
    } else if (m_activeTab == LevelTab::Custom && m_customTab) {
        m_customTab->renderGeometry(uiScale);
    }

    // 4. Кнопка Назад
    drawQuad(m_btnBack.pos, m_btnBack.size, glm::vec3(0.22f, 0.25f, 0.32f), m_whiteTexture);

    m_renderer->endBatch();

    // =========================================================================
    // PASS 2: ТЕКСТ (ПОВЕРХ ГЕОМЕТРИИ)
    // =========================================================================
    // 1. Текст верхних вкладок
    auto renderTabText = [&](const UIButton& tab, bool active) {
        float textW = m_textRenderer->CalculateTextWidth(tab.text, fontTab);
        float textX = tab.pos.x + (tab.size.x - textW) * 0.5f;
        float textY = tab.pos.y + (tab.size.y - fontTab * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(tab.text, textX, textY, fontTab, active ? glm::vec3(1.0f) : glm::vec3(0.70f));
    };
    renderTabText(m_tabCampaign, m_activeTab == LevelTab::Campaign);
    renderTabText(m_tabCustom, m_activeTab == LevelTab::Custom);

    // 2. Dev Mode текст
    float fDevBadge = std::clamp(0.48f * uiScale, 0.36f, 0.62f);
    float tw = m_textRenderer->CalculateTextWidth(m_btnDevModeToggle.text, fDevBadge);
    float tx = m_btnDevModeToggle.pos.x + (m_btnDevModeToggle.size.x - tw) * 0.5f;
    float ty = m_btnDevModeToggle.pos.y + (m_btnDevModeToggle.size.y - fDevBadge * 28.0f) * 0.5f + 2.0f;
    glm::vec3 devToggleFg = m_isDevMode ? glm::vec3(1.0f) : glm::vec3(0.70f, 0.72f, 0.78f);
    m_textRenderer->RenderText(m_btnDevModeToggle.text, tx, ty, fDevBadge, devToggleFg);

    // 3. Текст активной вкладки (и модальные окна)
    if (m_activeTab == LevelTab::Campaign && m_campaignTab) {
        m_campaignTab->renderText(uiScale, m_isDevMode);
    } else if (m_activeTab == LevelTab::Custom && m_customTab) {
        m_customTab->renderText(uiScale);
    }

    // 4. Текст кнопки Назад
    float bkw = m_textRenderer->CalculateTextWidth(m_btnBack.text, fNav);
    m_textRenderer->RenderText(m_btnBack.text, m_btnBack.pos.x + (m_btnBack.size.x - bkw) * 0.5f,
                               m_btnBack.pos.y + (m_btnBack.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(1.0f));
}

void LevelSelectState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    init();
}