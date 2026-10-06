#pragma once
#include <glm/glm.hpp>
#include <string>
#include <functional>

struct GLFWwindow;

struct GameWorld;
class Buildpanel;
class TowerMenuUI;
class BuildManager;
class Tower;

class GameplayInputHandler {
private:
    bool m_keysProcessed[1024] = { false };
    bool m_mousePressedLastFrame = false;
    glm::vec2 m_currentMousePos{ 0.0f, 0.0f };
    float m_pistonPlacementAngle = 270.0f; // 270 = Вверх/Север по умолчанию

    bool isKeyJustPressed(GLFWwindow* window, int key);

public:
    GameplayInputHandler() = default;

    void processInput(
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
        std::function<void()> onTogglePauseRequested = nullptr,
        std::function<void()> onCycleSpeedRequested = nullptr,
        std::function<void(float)> onSetSpeedRequested = nullptr
    );

    glm::vec2 getMousePos() const { return m_currentMousePos; }
    float getPistonPlacementAngle() const { return m_pistonPlacementAngle; }
};
