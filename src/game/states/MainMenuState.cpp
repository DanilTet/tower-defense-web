#include "MainMenuState.h"
#include "GameStateManager.h"
#include "LevelSelectState.h"
#include "GameplayState.h"
#include "MapEditorState.h"
#include "SettingsState.h"
#include "../core/CampaignManager.h"
#include "../core/LocalizationManager.h"
#include "../core/SettingsManager.h"
#include "../renderer/TextRenderer.h"
#include <GLFW/glfw3.h>
#include "../resources/ResourceManager.h"
#include <iostream>
#include <algorithm>

MainMenuState::MainMenuState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_mousePressedLastFrame(false), m_suppressClickUntilRelease(true) {
}

void MainMenuState::init() {
    std::cout << "[MainMenu] Initialized (resolution: " << m_width << "x" << m_height << ")" << std::endl;
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");
}

void MainMenuState::cleanup() {}

MainMenuState::MenuLayout MainMenuState::calculateLayout() const {
    MenuLayout layout;
    float uiScale = SettingsManager::getUIScaleMultiplier();

    layout.btnFontScale = std::clamp(1.2f * uiScale, 0.70f, 1.65f);
    layout.titleFontScale = std::clamp(1.5f * uiScale, 0.85f, 2.10f);

    layout.btnH = std::clamp(38.0f * uiScale, 24.0f, 54.0f);
    float btnSpacing = std::clamp(55.0f * uiScale, 34.0f, 75.0f);

    layout.btnW = std::clamp(280.0f * uiScale, 180.0f, static_cast<float>(m_width) - 40.0f);
    layout.btnX = (m_width - layout.btnW) * 0.5f;

#ifdef __EMSCRIPTEN__
    float totalH = 3.0f * btnSpacing + layout.btnH;
#else
    float totalH = 4.0f * btnSpacing + layout.btnH;
#endif
    layout.startBtnY = (m_height - totalH) * 0.5f + 30.0f * uiScale;
    layout.loadBtnY = layout.startBtnY + btnSpacing;
    layout.editorBtnY = layout.startBtnY + 2.0f * btnSpacing;
    layout.settingsBtnY = layout.startBtnY + 3.0f * btnSpacing;
    layout.exitBtnY = layout.startBtnY + 4.0f * btnSpacing;

    layout.titleY = layout.startBtnY - 70.0f * uiScale;
    return layout;
}

bool MainMenuState::isButtonClicked(double mouseX, double mouseY, float btnX, float btnY, float btnW, float btnH) {
    return mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH;
}

void MainMenuState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Хоткей переключения Dev Mode: Ctrl + Shift + D
    bool ctrlDown = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
    bool shiftDown = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool dDown = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);

    if (ctrlDown && shiftDown && dDown) {
        if (!m_ctrlShiftDWasDown) {
            m_ctrlShiftDWasDown = true;
            CampaignManager::toggleDevMode();
            std::cout << "[MainMenu] Dev Mode toggled: " << (CampaignManager::isDevMode() ? "ON" : "OFF") << std::endl;
        }
    } else {
        m_ctrlShiftDWasDown = false;
    }

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    // Защита от случайного клика-сквозь при переходе из другого стейта
    if (m_suppressClickUntilRelease) {
        if (mouseState == GLFW_RELEASE) {
            std::cout << "[MainMenu] Mouse released -> interaction ready" << std::endl;
            m_suppressClickUntilRelease = false;
            m_mousePressedLastFrame = false;
        } else {
            return; // Игнорируем нажатую кнопку мыши, пока её физически не отпустят
        }
    }

    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        MenuLayout layout = calculateLayout();

        // если клик по старт гейм
        if (isButtonClicked(mouseX, mouseY, layout.btnX, layout.startBtnY, layout.btnW, layout.btnH)) {
            std::cout << "[MainMenu] Clicked 'Start Game' (mouse=" << mouseX << "," << mouseY << ") -> LevelSelectState" << std::endl;
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelSelectState::getLastActiveTab()));
            return;
        }

        // Клик по Load Game
        if (isButtonClicked(mouseX, mouseY, layout.btnX, layout.loadBtnY, layout.btnW, layout.btnH)) {
            std::cout << "[MainMenu] Clicked 'Load Game' (mouse=" << mouseX << "," << mouseY << ") -> GameplayState" << std::endl;
            auto loadState = std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, "", false, GameplayOrigin::MainMenu);
            loadState->setSaveToLoad("savegame");

            m_stateManager.setState(std::move(loadState));
            return;
        }

        // Клик по Map Editor
        if (isButtonClicked(mouseX, mouseY, layout.btnX, layout.editorBtnY, layout.btnW, layout.btnH)) {
            std::cout << "[MainMenu] Clicked 'Map Editor' (mouse=" << mouseX << "," << mouseY << ") -> MapEditorState" << std::endl;
            m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, "", EditorOrigin::MainMenu));
            return;
        }

        // Клик по Settings
        if (isButtonClicked(mouseX, mouseY, layout.btnX, layout.settingsBtnY, layout.btnW, layout.btnH)) {
            std::cout << "[MainMenu] Clicked 'Settings' (mouse=" << mouseX << "," << mouseY << ") -> pushing SettingsState" << std::endl;
            m_stateManager.pushState(std::make_unique<SettingsState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

#ifndef __EMSCRIPTEN__
        // если выход
        if (isButtonClicked(mouseX, mouseY, layout.btnX, layout.exitBtnY, layout.btnW, layout.btnH)) {
            std::cout << "[MainMenu] Clicked 'Exit' (mouse=" << mouseX << "," << mouseY << ") -> Closing game" << std::endl;
            glfwSetWindowShouldClose(window, true);
        }
#endif
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}

void MainMenuState::update(float dt) {}

void MainMenuState::render() {
    m_renderer->beginBatch(); // открываем пакет

    MenuLayout layout = calculateLayout();

    // рисуем заголовок
    float titleW = m_textRenderer->CalculateTextWidth("Donbasyata Tower Defense", layout.titleFontScale);
    m_textRenderer->RenderText("Donbasyata Tower Defense", (m_width - titleW) * 0.5f, layout.titleY, layout.titleFontScale, glm::vec3(1.0f, 1.0f, 0.0f));

    // рисуем кнопки с автоцентрированием текста
    auto renderBtnText = [&](const std::string& str, float y, const glm::vec3& color) {
        float w = m_textRenderer->CalculateTextWidth(str, layout.btnFontScale);
        m_textRenderer->RenderText(str, (m_width - w) * 0.5f, y, layout.btnFontScale, color);
    };

    renderBtnText("> " + LOC("BTN_START_GAME") + " <", layout.startBtnY, glm::vec3(1.0f, 1.0f, 1.0f));
    renderBtnText("> " + LOC("BTN_LOAD_GAME") + " <", layout.loadBtnY, glm::vec3(0.2f, 0.8f, 1.0f));
    renderBtnText("> " + LOC("BTN_MAP_EDITOR") + " <", layout.editorBtnY, glm::vec3(0.9f, 0.8f, 0.2f));
    renderBtnText("> " + LOC("BTN_SETTINGS") + " <", layout.settingsBtnY, glm::vec3(0.75f, 0.88f, 1.0f));
#ifndef __EMSCRIPTEN__
    renderBtnText("> " + LOC("BTN_EXIT") + " <", layout.exitBtnY, glm::vec3(1.0f, 0.3f, 0.3f));
#endif

    if (CampaignManager::isDevMode()) {
        float fDev = std::clamp(0.44f * SettingsManager::getUIScaleMultiplier(), 0.32f, 0.55f);
        std::string devBadge = "[ DEV MODE: ON ]";
        float dbw = m_textRenderer->CalculateTextWidth(devBadge, fDev);
        m_textRenderer->RenderText(devBadge, static_cast<float>(m_width) - dbw - 20.0f, 20.0f, fDev, glm::vec3(0.95f, 0.65f, 0.20f));
    }

    m_renderer->endBatch(); // закрываем пакет
}

void MainMenuState::resize(int width, int height) {
    m_width = width;
    m_height = height;
}