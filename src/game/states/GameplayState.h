#pragma once
#include "IGameState.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "../world/GameWorld.h"
#include "../ui/BuildPanel.h"
#include "../ui/PlacementUI.h"
#include "../ui/StatsPanel.h"
#include "../ui/TowerMenuUI.h"
#include "../ui/PathRenderer.h"
#include "../gameplay/BuildManager.h"
#include "GameplayInputHandler.h"
#include "../../renderer/GameplayRenderer.h"

class SpriteRenderer;
class TextRenderer;
class GameStateManager;
class Tower;

class GameplayState : public IGameState {
private:
    GameStateManager& m_stateManager;

    int width, height;
    std::string m_selectedTowerType;
    Tower* m_selectedTowerOnMap = nullptr;
    std::string m_currentLevelPath;

    // Игровой мир и менеджеры
    std::unique_ptr<GameWorld> m_world;
    std::unique_ptr<BuildManager> m_buildManager;

    // Выделенные подсистемы (декомпозиция God Object)
    std::unique_ptr<GameplayInputHandler> m_inputHandler;
    std::unique_ptr<GameplayRenderer> m_gameplayRenderer;

    // UI элементы
    std::unique_ptr<Buildpanel> m_buildPanel;
    std::unique_ptr<PlacementUI> m_placementUI;
    std::unique_ptr<PathVisualizer> m_pathVisualizer;
    std::unique_ptr<StatsPanel> m_statsPanel;
    std::unique_ptr<TowerMenuUI> m_towerMenuUI;

    // Рендереры для передачи в дочерние стейты
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;

    std::string m_saveToLoad = "";
    bool m_isValid = true;
    bool m_isEditorTest = false;
    GameplayOrigin m_origin = GameplayOrigin::Campaign;
    bool m_editorBtnPressedLastFrame = true;
    bool m_isExitingToEditor = false;

    float m_timeScale = 1.0f;
    bool m_isPaused = false;

    void setupUI();
    void setupEventListeners();

public:
    GameplayState(GameStateManager& stateManager, int windowWidth, int windowHeight, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, std::string levelPath, bool isEditorTest = false, GameplayOrigin origin = GameplayOrigin::Campaign);

    bool isEditorTest() const { return m_isEditorTest; }
    GameplayOrigin getOrigin() const { return m_origin; }
    const std::string& getCurrentLevelPath() const { return m_currentLevelPath; }

    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;

    void cycleTimeScale();
    void setTimeScale(float scale);
    void togglePause();
    float getTimeScale() const { return m_timeScale; }
    bool isPaused() const { return m_isPaused; }

    void startNextWave();
    void restartGame();

    bool loadSavedGame(const std::string& saveName);
    void setSaveToLoad(const std::string& saveName) { m_saveToLoad = saveName; }
    void saveGame(const std::string& saveName);
};