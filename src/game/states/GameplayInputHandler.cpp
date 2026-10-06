#include "GameplayInputHandler.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../world/GameWorld.h"
#include "../ui/BuildPanel.h"
#include "../ui/TowerMenuUI.h"
#include "../gameplay/BuildManager.h"
#include "../entities/Tower.h"
#include "../ui/UICommon.h"

bool GameplayInputHandler::isKeyJustPressed(GLFWwindow* window, int key) {
    if (key < 0 || key >= 1024) return false;

    if (glfwGetKey(window, key) == GLFW_PRESS) {
        if (!m_keysProcessed[key]) {
            m_keysProcessed[key] = true;
            return true;
        }
    }
    else {
        m_keysProcessed[key] = false;
    }

    return false;
}

void GameplayInputHandler::processInput(
    GLFWwindow* window,
    float dt,
    int windowWidth,
    int windowHeight,
    GameWorld& world,
    Buildpanel& buildPanel,
    TowerMenuUI& towerMenuUI,
    BuildManager& buildManager,
    std::string& selectedTowerType,
    Tower*& selectedTowerOnMap,
    std::function<void()> onPauseRequested,
    std::function<void()> onNextWaveRequested,
    std::function<void()> onTogglePauseRequested,
    std::function<void()> onCycleSpeedRequested,
    std::function<void(float)> onSetSpeedRequested)
{
    if (isKeyJustPressed(window, GLFW_KEY_ESCAPE)) {
        if (onPauseRequested) onPauseRequested();
        return;
    }

    // Пауза по клавишам Space или P
    if (isKeyJustPressed(window, GLFW_KEY_SPACE) || isKeyJustPressed(window, GLFW_KEY_P)) {
        if (onTogglePauseRequested) onTogglePauseRequested();
    }

    // Переключение множителя скорости времени по клавише Tab
    if (isKeyJustPressed(window, GLFW_KEY_TAB)) {
        if (onCycleSpeedRequested) onCycleSpeedRequested();
    }

    // Быстрый выбор башен из дока по клавишам 1, 2, 3
    if (isKeyJustPressed(window, GLFW_KEY_1)) {
        if (buildPanel.selectTowerByIndex(0, selectedTowerType)) {
            selectedTowerOnMap = nullptr;
        }
    }
    if (isKeyJustPressed(window, GLFW_KEY_2)) {
        if (buildPanel.selectTowerByIndex(1, selectedTowerType)) {
            selectedTowerOnMap = nullptr;
        }
    }
    if (isKeyJustPressed(window, GLFW_KEY_3)) {
        if (buildPanel.selectTowerByIndex(2, selectedTowerType)) {
            selectedTowerOnMap = nullptr;
        }
    }

    if (isKeyJustPressed(window, GLFW_KEY_ENTER)) {
        if (onNextWaveRequested) onNextWaveRequested();
    }

    if (isKeyJustPressed(window, GLFW_KEY_M)) {
        world.playerStats.money += 99999;
    }

    if (isKeyJustPressed(window, GLFW_KEY_R)) {
        if (selectedTowerType == "Piston") {
            if (std::abs(m_pistonPlacementAngle - 270.0f) < 5.0f || std::abs(m_pistonPlacementAngle - (-90.0f)) < 5.0f) {
                m_pistonPlacementAngle = 0.0f;
            }
            else if (std::abs(m_pistonPlacementAngle - 0.0f) < 5.0f) {
                m_pistonPlacementAngle = 90.0f;
            }
            else if (std::abs(m_pistonPlacementAngle - 90.0f) < 5.0f) {
                m_pistonPlacementAngle = 180.0f;
            }
            else {
                m_pistonPlacementAngle = 270.0f;
            }
        }
        else if (selectedTowerOnMap && selectedTowerOnMap->getType() == "Piston") {
            selectedTowerOnMap->rotate90();
        }
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        selectedTowerType = "";
    }

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        // Проверяем клик по панели управления временем в HUD (кнопки и фоновая плашка)
        UIRect panelRect = TimeControlUI::getPanelRect(windowWidth, windowHeight);
        if (panelRect.contains(static_cast<float>(mouseX), static_cast<float>(mouseY))) {
            UIRect pauseRect = TimeControlUI::getPauseButtonRect(windowWidth, windowHeight);
            if (pauseRect.contains(static_cast<float>(mouseX), static_cast<float>(mouseY))) {
                if (onTogglePauseRequested) onTogglePauseRequested();
                return;
            }

            UIRect s1Rect = TimeControlUI::getSpeed1xButtonRect(windowWidth, windowHeight);
            if (s1Rect.contains(static_cast<float>(mouseX), static_cast<float>(mouseY))) {
                if (onSetSpeedRequested) onSetSpeedRequested(1.0f);
                else if (onCycleSpeedRequested) onCycleSpeedRequested();
                return;
            }

            UIRect s2Rect = TimeControlUI::getSpeed2xButtonRect(windowWidth, windowHeight);
            if (s2Rect.contains(static_cast<float>(mouseX), static_cast<float>(mouseY))) {
                if (onSetSpeedRequested) onSetSpeedRequested(2.0f);
                else if (onCycleSpeedRequested) onCycleSpeedRequested();
                return;
            }

            UIRect s4Rect = TimeControlUI::getSpeed4xButtonRect(windowWidth, windowHeight);
            if (s4Rect.contains(static_cast<float>(mouseX), static_cast<float>(mouseY))) {
                if (onSetSpeedRequested) onSetSpeedRequested(4.0f);
                else if (onCycleSpeedRequested) onCycleSpeedRequested();
                return;
            }

            // Клик пришелся на поля или разделители панели — поглощаем его, чтобы не прокликивать карту
            return;
        }

        float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
        if (mouseY >= static_cast<double>(windowHeight) - bottomBarHeight) {
            if (buildPanel.checkClick(mouseX, mouseY, windowWidth, windowHeight, selectedTowerType)) {
                selectedTowerOnMap = nullptr;
            }
            return;
        }

        if (towerMenuUI.processClick(mouseX, mouseY, selectedTowerOnMap, &buildManager, world, selectedTowerOnMap)) {
            return;
        }

        if (world.grid && world.entityManager) {
            glm::ivec2 clickedCell = world.grid->pixelToGrid(glm::vec2(mouseX, mouseY));
            Tower* clickedTower = world.entityManager->getTowerAt(clickedCell.x, clickedCell.y);

            if (clickedTower != nullptr) {
                selectedTowerOnMap = clickedTower;
                selectedTowerType = "";
                return;
            }
            else {
                selectedTowerOnMap = nullptr;
            }
        }

        buildManager.tryBuildOrUpgrade(
            m_currentMousePos,
            selectedTowerType,
            world,
            m_pistonPlacementAngle
        );
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}
