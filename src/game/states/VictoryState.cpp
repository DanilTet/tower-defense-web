#include "VictoryState.h"
#include "GameStateManager.h"
#include "GameplayState.h"
#include "LevelSelectState.h"
#include "MainMenuState.h"
#include "../core/CampaignManager.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../ui/UICommon.h"
#include "../core/LocalizationManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

VictoryState::VictoryState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, const std::string& levelPath, bool isEditorTest, GameplayOrigin origin)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_levelPath(levelPath), m_isEditorTest(isEditorTest), m_origin(origin)
{
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_mousePressedLastFrame = true;
}

void VictoryState::updateLayout() {
    float scale = GetUIScale(m_width, m_height);
    m_uiScale = scale;

    m_windowSize = glm::vec2(
        std::clamp(420.0f * scale, 300.0f, 600.0f),
        std::clamp(320.0f * scale, 240.0f, 480.0f)
    );
    m_windowPos = glm::vec2(
        (static_cast<float>(m_width) - m_windowSize.x) * 0.5f,
        (static_cast<float>(m_height) - m_windowSize.y) * 0.5f
    );

    float btnW = std::clamp(300.0f * scale, 200.0f, m_windowSize.x - 40.0f);
    float btnH = std::clamp(48.0f * scale, 34.0f, 60.0f);
    float spacing = std::clamp(16.0f * scale, 10.0f, 24.0f);

    m_btnNext.size = glm::vec2(btnW, btnH);
    m_btnMenu.size = glm::vec2(btnW, btnH);

    m_btnNext.text = m_isEditorTest ? LOC("PAUSE_BACK_TO_EDITOR") : LOC("VICTORY_NEXT");
    m_btnMenu.text = m_isEditorTest ? LOC("PAUSE_BACK_TO_EDITOR") : LOC("VICTORY_MENU");

    float fTitle = std::clamp(0.95f * scale, 0.65f, 1.30f);
    float titleTopOffset = std::clamp(36.0f * scale, 24.0f, 50.0f);
    float startBtnY = m_windowPos.y + titleTopOffset + fTitle * 28.0f + std::clamp(28.0f * scale, 18.0f, 42.0f);

    float btnX = m_windowPos.x + (m_windowSize.x - btnW) * 0.5f;
    m_btnNext.pos = glm::vec2(btnX, startBtnY);
    m_btnMenu.pos = glm::vec2(btnX, startBtnY + btnH + spacing);
}

void VictoryState::init() {
    m_mousePressedLastFrame = true;
    m_btnNext.state = 0;
    m_btnMenu.state = 0;
    updateLayout();
}

void VictoryState::cleanup() {}

bool VictoryState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return (point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y);
}

void VictoryState::processInput(GLFWwindow* window, float dt) {
    float curScale = GetUIScale(m_width, m_height);
    if (std::abs(curScale - m_uiScale) > 0.001f) {
        updateLayout();
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glm::vec2 mousePos(mouseX, mouseY);

    auto updateButtonState = [&](UIButton& btn) {
        if (isPointInRect(mousePos, btn.pos, btn.size)) {
            btn.state = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ? 2 : 1;
        }
        else {
            btn.state = 0;
        }
    };

    updateButtonState(m_btnNext);
    updateButtonState(m_btnMenu);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        if (m_btnNext.state == 2) {
            if (m_isEditorTest) {
                std::cout << "[VictoryState] Returning to MapEditor..." << std::endl;
                m_stateManager.returnToMapEditor("", m_width, m_height, m_renderer, m_textRenderer);
            } else if (m_origin == GameplayOrigin::Campaign) {
                std::string nextFile = CampaignManager::getNextMissionFile(m_levelPath);
                if (!nextFile.empty()) {
                    std::string nextPath = "res/levels/" + nextFile;
                    std::cout << "[VictoryState] Launching next campaign mission: " << nextPath << std::endl;
                    m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, nextPath, false, GameplayOrigin::Campaign));
                } else {
                    std::cout << "[VictoryState] Campaign completed! Returning to Campaign menu..." << std::endl;
                    m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Campaign));
                }
            } else if (m_origin == GameplayOrigin::Custom) {
                m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Custom));
            } else {
                m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            }
        }
        else if (m_btnMenu.state == 2) {
            if (m_isEditorTest) {
                std::cout << "[VictoryState] Returning to MapEditor..." << std::endl;
                m_stateManager.returnToMapEditor("", m_width, m_height, m_renderer, m_textRenderer);
            } else if (m_origin == GameplayOrigin::Custom) {
                std::cout << "[VictoryState] Returning to Custom LevelSelect..." << std::endl;
                m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Custom));
            } else if (m_origin == GameplayOrigin::Campaign) {
                std::cout << "[VictoryState] Returning to Campaign LevelSelect..." << std::endl;
                m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Campaign));
            } else {
                m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            }
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}

void VictoryState::update(float dt) {}

void VictoryState::render() {
    float curScale = GetUIScale(m_width, m_height);
    if (std::abs(curScale - m_uiScale) > 0.001f) {
        updateLayout();
    }

    m_renderer->beginBatch();
    m_renderer->drawSpriteRGBA(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.0f, 0.1f, 0.2f, 0.8f));
    m_renderer->drawSprite(m_uiTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.12f, 0.15f, 0.2f));

    glm::vec3 nextColor = (m_btnNext.state == 1) ? glm::vec3(0.3f, 0.5f, 0.3f) : ((m_btnNext.state == 2) ? glm::vec3(0.1f, 0.3f, 0.1f) : glm::vec3(0.2f, 0.4f, 0.2f));
    glm::vec3 menuColor = (m_btnMenu.state == 1) ? glm::vec3(0.5f) : ((m_btnMenu.state == 2) ? glm::vec3(0.2f) : glm::vec3(0.3f));

    m_renderer->drawSprite(m_uiTexture, m_btnNext.pos, m_btnNext.size, 0.0f, nextColor);
    m_renderer->drawSprite(m_uiTexture, m_btnMenu.pos, m_btnMenu.size, 0.0f, menuColor);
    m_renderer->endBatch();

    float fTitle = std::clamp(0.95f * m_uiScale, 0.65f, 1.30f);
    std::string titleText = LOC("VICTORY_TITLE");
    float titleW = m_textRenderer->CalculateTextWidth(titleText, fTitle);
    float titleTopOffset = std::clamp(36.0f * m_uiScale, 24.0f, 50.0f);
    m_textRenderer->RenderText(titleText, m_windowPos.x + (m_windowSize.x - titleW) * 0.5f, m_windowPos.y + titleTopOffset, fTitle, glm::vec3(1.0f, 0.8f, 0.2f));

    float fBtn = std::clamp(0.55f * m_uiScale, 0.38f, 0.76f);

    float twNext = m_textRenderer->CalculateTextWidth(m_btnNext.text, fBtn);
    float txNext = m_btnNext.pos.x + (m_btnNext.size.x - twNext) * 0.5f;
    float tyNext = m_btnNext.pos.y + (m_btnNext.size.y - fBtn * 28.0f) * 0.5f + 2.0f;
    m_textRenderer->RenderText(m_btnNext.text, txNext, tyNext, fBtn, glm::vec3(0.95f));

    float twMenu = m_textRenderer->CalculateTextWidth(m_btnMenu.text, fBtn);
    float txMenu = m_btnMenu.pos.x + (m_btnMenu.size.x - twMenu) * 0.5f;
    float tyMenu = m_btnMenu.pos.y + (m_btnMenu.size.y - fBtn * 28.0f) * 0.5f + 2.0f;
    m_textRenderer->RenderText(m_btnMenu.text, txMenu, tyMenu, fBtn, glm::vec3(0.95f));
}

void VictoryState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    updateLayout();
}