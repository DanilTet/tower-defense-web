#include "PauseState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "GameplayState.h"
#include "LevelSelectState.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/SpriteRenderer.h"
#include "../resources/ResourceManager.h"
#include "../core/SettingsManager.h"
#include "../core/LocalizationManager.h"
#include "../ui/UICommon.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

PauseState::PauseState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, GameplayState* gameplayState)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_gameplayState(gameplayState), m_mousePressedLastFrame(false) {

    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
}

void PauseState::updateLayout() {
    float scale = GetUIScale(m_width, m_height);
    m_uiScale = scale;

    m_headerHeight = std::clamp(44.0f * scale, 34.0f, 64.0f);

    float btnW = std::clamp(280.0f * scale, 200.0f, 420.0f);
    float btnH = std::clamp(44.0f  * scale, 32.0f, 60.0f);
    float spacing = std::clamp(14.0f * scale, 10.0f, 22.0f);
    float padX = std::clamp(30.0f * scale, 20.0f, 48.0f);

    float winW = std::clamp(btnW + 2.0f * padX, 320.0f * scale, std::min(static_cast<float>(m_width) - 40.0f, 620.0f));

    m_volumeWidget.setScale(scale);
    m_volumeWidget.setWidth(winW - 2.0f * padX);
    m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));

    float volH = m_volumeWidget.getHeight();

    m_btnResume.size = glm::vec2(btnW, btnH);
    m_btnSave.size   = glm::vec2(btnW, btnH);
    m_btnExit.size   = glm::vec2(btnW, btnH);

    m_btnResume.text = LOC("PAUSE_RESUME");
    m_btnSave.text   = LOC("PAUSE_SAVE");
    if (m_gameplayState && m_gameplayState->isEditorTest()) {
        m_btnExit.text = LOC("PAUSE_BACK_TO_EDITOR");
    } else {
        m_btnExit.text = LOC("PAUSE_EXIT");
    }

    float currY = m_headerHeight + spacing;
    m_resumeRelY = currY;
    currY += btnH + spacing;

    m_saveRelY = currY;
    currY += btnH + spacing;

    m_volRelY = currY;
    currY += volH + spacing;

    m_exitRelY = currY;
    currY += btnH + spacing;

    float winH = currY;
    m_windowSize = glm::vec2(winW, winH);

    if (!m_isDragging) {
        m_windowPos = glm::vec2((static_cast<float>(m_width) - winW) * 0.5f,
                                (static_cast<float>(m_height) - winH) * 0.5f);
    }

    updateButtonPositions();
}

void PauseState::updateButtonPositions() {
    float padX = std::clamp(30.0f * m_uiScale, 20.0f, 48.0f);
    m_btnResume.pos = m_windowPos + glm::vec2((m_windowSize.x - m_btnResume.size.x) * 0.5f, m_resumeRelY);
    m_btnSave.pos   = m_windowPos + glm::vec2((m_windowSize.x - m_btnSave.size.x)   * 0.5f, m_saveRelY);
    m_volumeWidget.setPosition(m_windowPos + glm::vec2(padX, m_volRelY));
    m_btnExit.pos   = m_windowPos + glm::vec2((m_windowSize.x - m_btnExit.size.x)   * 0.5f, m_exitRelY);
}

void PauseState::init() {
    m_isDragging = false;
    m_dragOffset = glm::vec2(0.0f);
    m_mousePressedLastFrame = true; // Защита от клик-сквозняка (td-ui rule 4)
    m_escPressedLastFrame = true;

    m_btnResume.state = 0;
    m_btnSave.state = 0;
    m_btnExit.state = 0;

    m_volumeWidget = VolumeSliderWidget(glm::vec2(0.0f), 320.0f, true, LOC("SETTINGS_VOLUME"));

    updateLayout();
}

void PauseState::cleanup() {}

bool PauseState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void PauseState::processInput(GLFWwindow* window, float dt) {
    // ESC клавиша для быстрого закрытия паузы (Resume)
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!m_escPressedLastFrame) {
            m_escPressedLastFrame = true;
            m_stateManager.popState();
            return;
        }
    } else {
        m_escPressedLastFrame = false;
    }

    // Проверяем динамическое изменение масштаба
    float curScale = GetUIScale(m_width, m_height);
    if (std::abs(curScale - m_uiScale) > 0.001f) {
        updateLayout();
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    bool isPressed = (mouseState == GLFW_PRESS);
    bool justPressed = (isPressed && !m_mousePressedLastFrame);

    if (justPressed) {
        m_mousePressedLastFrame = true;

        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, justPressed)) {
            return;
        }

        // проверяем клик по шапке окна
        glm::vec2 headerSize(m_windowSize.x, m_headerHeight);
        if (isPointInRect(m_currentMousePos, m_windowPos, headerSize)) {
            m_isDragging = true;
            m_dragOffset = m_currentMousePos - m_windowPos;
            return;
        }

        // проверяем клик по кнопкам
        if (isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size)) m_btnResume.state = 2;
        if (isPointInRect(m_currentMousePos, m_btnSave.pos, m_btnSave.size)) m_btnSave.state = 2;
        if (isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)) m_btnExit.state = 2;
    }
    else if (isPressed) {
        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, false)) {
            return;
        }
        if (m_isDragging) {
            m_windowPos = m_currentMousePos - m_dragOffset;
            m_windowPos.x = std::clamp(m_windowPos.x, 0.0f, std::max(0.0f, static_cast<float>(m_width) - m_windowSize.x));
            m_windowPos.y = std::clamp(m_windowPos.y, 0.0f, std::max(0.0f, static_cast<float>(m_height) - m_windowSize.y));
            updateButtonPositions();
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_volumeWidget.handleInput(m_currentMousePos, false, false);
        m_isDragging = false;

        // если отпустили кнопку над Resume
        if (m_btnResume.state == 2 && isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size)) {
            m_stateManager.popState();
            return;
        }
        // если отпустили кнопку над Save
        if (m_btnSave.state == 2 && isPointInRect(m_currentMousePos, m_btnSave.pos, m_btnSave.size)) {
            if (m_gameplayState) {
                m_gameplayState->saveGame("savegame");
            }
            m_btnSave.state = 0;
            m_mousePressedLastFrame = false;
            return;
        }
        // если отпустили кнопку над Exit
        if (m_btnExit.state == 2 && isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)) {
            if (m_gameplayState && m_gameplayState->isEditorTest()) {
                std::cout << "[PauseState] Returning to MapEditor..." << std::endl;
                std::string lvlPath = m_gameplayState ? m_gameplayState->getCurrentLevelPath() : "";
                m_stateManager.returnToMapEditor(lvlPath, m_width, m_height, m_renderer, m_textRenderer);
                return;
            } else if (m_gameplayState && m_gameplayState->getOrigin() == GameplayOrigin::Custom) {
                std::cout << "[PauseState] Returning to Custom LevelSelect..." << std::endl;
                m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Custom));
                return;
            } else if (m_gameplayState && m_gameplayState->getOrigin() == GameplayOrigin::Campaign) {
                std::cout << "[PauseState] Returning to Campaign LevelSelect..." << std::endl;
                m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Campaign));
                return;
            } else {
                m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
                return;
            }
        }

        m_btnResume.state = isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size) ? 1 : 0;
        m_btnSave.state   = isPointInRect(m_currentMousePos, m_btnSave.pos, m_btnSave.size)   ? 1 : 0;
        m_btnExit.state   = isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)   ? 1 : 0;
        m_mousePressedLastFrame = false;
    }
}

void PauseState::update(float dt) {}

void PauseState::render() {
    float curScale = GetUIScale(m_width, m_height);
    if (std::abs(curScale - m_uiScale) > 0.001f) {
        updateLayout();
    }

    m_renderer->beginBatch(); // открываем пакет
    // затемнение игры
    m_renderer->drawSpriteRGBA(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.65f));
    // рисуем окно
    m_renderer->drawSprite(m_uiTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.12f, 0.12f, 0.15f));
    // рисуем шапку
    m_renderer->drawSprite(m_uiTexture, m_windowPos, glm::vec2(m_windowSize.x, m_headerHeight), 0.0f, glm::vec3(0.18f, 0.18f, 0.22f));

    // кнопки рисуем зависимо от стейта
    auto drawButton = [&](UIButton& btn, glm::vec3 color) {
        m_renderer->drawSprite(m_uiTexture, btn.pos, btn.size, 0.0f, color);
    };

    glm::vec3 resumeColor = glm::vec3(0.15f, 0.35f, 0.15f); // Idle
    if (m_btnResume.state == 1) resumeColor = glm::vec3(0.22f, 0.55f, 0.22f); // Hover
    if (m_btnResume.state == 2) resumeColor = glm::vec3(0.10f, 0.25f, 0.10f); // Pressed

    glm::vec3 saveColor = glm::vec3(0.15f, 0.30f, 0.45f);
    if (m_btnSave.state == 1) saveColor = glm::vec3(0.20f, 0.40f, 0.60f);
    if (m_btnSave.state == 2) saveColor = glm::vec3(0.10f, 0.20f, 0.30f);

    glm::vec3 exitColor = glm::vec3(0.45f, 0.15f, 0.15f); // Idle
    if (m_btnExit.state == 1) exitColor = glm::vec3(0.65f, 0.20f, 0.20f); // Hover
    if (m_btnExit.state == 2) exitColor = glm::vec3(0.30f, 0.10f, 0.10f); // Pressed

    drawButton(m_btnResume, resumeColor);
    drawButton(m_btnSave, saveColor);
    drawButton(m_btnExit, exitColor);

    m_renderer->endBatch(); // рисуем

    float fTitle = std::clamp(0.85f * m_uiScale, 0.58f, 1.20f);
    float fBtn   = std::clamp(0.52f * m_uiScale, 0.38f, 0.74f);

    // текст заголовочный с идеальным вертикальным центрированием
    std::string titleText = LOC("PAUSE_TITLE");
    float titleW = m_textRenderer->CalculateTextWidth(titleText, fTitle);
    float titleX = m_windowPos.x + (m_windowSize.x - titleW) * 0.5f;
    float titleY = m_windowPos.y + (m_headerHeight - fTitle * 28.0f) * 0.5f + 2.0f;
    m_textRenderer->RenderText(titleText, titleX, titleY, fTitle, glm::vec3(1.0f, 0.75f, 0.0f));

    // текст на кнопках (каноническое центрирование по td-ui)
    auto drawBtnText = [&](const UIButton& btn, const std::string& text) {
        float tw = m_textRenderer->CalculateTextWidth(text, fBtn);
        float tx = btn.pos.x + (btn.size.x - tw) * 0.5f;
        float ty = btn.pos.y + (btn.size.y - fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(text, tx, ty, fBtn, glm::vec3(0.95f, 0.95f, 0.95f));
    };

    drawBtnText(m_btnResume, m_btnResume.text);
    drawBtnText(m_btnSave, m_btnSave.text);
    drawBtnText(m_btnExit, m_btnExit.text);

    // Отрисовка виджета громкости
    m_volumeWidget.render(m_renderer.get(), m_textRenderer, m_whiteTexture);
}

void PauseState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    updateLayout();
}