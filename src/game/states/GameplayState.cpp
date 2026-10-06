#include "GameplayState.h"
#include "GameStateManager.h"
#include "PauseState.h"
#include "MainMenuState.h"
#include "GameOverState.h"
#include "VictoryState.h"
#include "LevelSelectState.h"
#include "../core/ConfigManager.h"
#include "../core/LevelManager.h"
#include "../core/CampaignManager.h"
#include "../core/EventBus.h"
#include "../core/SaveManager.h"
#include "../core/SettingsManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../resources/ResourceManager.h"
#include "../textures/Texture2D.h"
#include "../../audio/AudioEventSystem.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

GameplayState::GameplayState(GameStateManager& stateManager, int windowWidth, int windowHeight, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, std::string levelPath, bool isEditorTest, GameplayOrigin origin)
    : m_stateManager(stateManager),
    width(windowWidth),
    height(windowHeight),
    m_renderer(renderer),
    m_textRenderer(textRenderer),
    m_selectedTowerType(""),
    m_currentLevelPath(levelPath),
    m_isEditorTest(isEditorTest),
    m_origin(origin)
{
    if (m_isEditorTest) {
        m_origin = GameplayOrigin::EditorTest;
    }
}

void GameplayState::cleanup() {
    EventBus::clear();
    AudioEventSystem::cleanup();
}

void GameplayState::setupUI() {
    m_buildPanel = std::make_unique<Buildpanel>();
    m_buildPanel->initPanelData();
    m_placementUI = std::make_unique<PlacementUI>();
    m_pathVisualizer = std::make_unique<PathVisualizer>();
    m_statsPanel = std::make_unique<StatsPanel>();
    m_towerMenuUI = std::make_unique<TowerMenuUI>();
    m_buildManager = std::make_unique<BuildManager>();
}

void GameplayState::setupEventListeners() {
    EventBus::clear();

    AudioEventSystem::init();

    EventBus::subscribe(EventType::EnemyDied, [this](const Event& e) {
        this->m_world->playerStats.money += e.value1;
        this->m_world->playerStats.score += e.value2;
    });

    EventBus::subscribe(EventType::EnemyReachedBase, [this](const Event& e) {
        this->m_world->playerStats.baseHealth -= e.value1;
        if (this->m_world->playerStats.baseHealth <= 0) {
            this->m_world->playerStats.baseHealth = 0;
            std::cout << "GAME OVER!" << std::endl;
        }
    });
}

void GameplayState::init() {
    ConfigManager::loadConfigs("res/configs/towers.json", "res/configs/enemies.json", "res/configs/particles.json");
    ConfigManager::loadTextureConfig("res/levels/textures.json");

    m_gameplayRenderer = std::make_unique<GameplayRenderer>(m_renderer, m_textRenderer);
    m_inputHandler = std::make_unique<GameplayInputHandler>();

    if (!m_saveToLoad.empty()) {
        bool success = loadSavedGame(m_saveToLoad);
        m_saveToLoad = "";

        if (!success) {
            std::cerr << "[ERROR] Файл сохранения не найден! Загрузка отменена." << std::endl;
            m_isValid = false;
            return;
        }
        return;
    }

    m_world = std::make_unique<GameWorld>();
    m_world->loadLevel(m_currentLevelPath, this->width, this->height);

    setupUI();
    setupEventListeners();
    AudioEventSystem::playThemeMusic("res/sounds/background.mp3");
    m_editorBtnPressedLastFrame = true;
}

void GameplayState::processInput(GLFWwindow* window, float dt) {
    if (!m_isValid || !m_inputHandler || !m_world) return;

    if (m_isEditorTest) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        float s = SettingsManager::getUIScaleMultiplier();
        float fontScale = std::clamp(0.50f * s, 0.38f, 0.70f);
        std::string btnText = "[ В редактор ]";
        float textW = m_textRenderer ? m_textRenderer->CalculateTextWidth(btnText, fontScale) : 100.0f;
        float btnX = 20.0f;
        float btnY = 20.0f;
        float btnW = std::max(150.0f * s, textW + 24.0f * s);
        float btnH = 40.0f * s;

        bool lmbDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        if (mouseX >= btnX && mouseX <= btnX + btnW &&
            mouseY >= btnY && mouseY <= btnY + btnH) {
            if (lmbDown && !m_editorBtnPressedLastFrame && !m_isExitingToEditor) {
                m_isExitingToEditor = true;
                m_editorBtnPressedLastFrame = true;
                std::cout << "[GameplayState] Returning to MapEditor..." << std::endl;
                m_stateManager.returnToMapEditor(m_currentLevelPath, width, height, m_renderer, m_textRenderer);
                return;
            }
        }
        m_editorBtnPressedLastFrame = lmbDown;
    }

    m_inputHandler->processInput(
        window,
        dt,
        this->width,
        this->height,
        *m_world,
        *m_buildPanel,
        *m_towerMenuUI,
        *m_buildManager,
        m_selectedTowerType,
        m_selectedTowerOnMap,
        [this]() {
            m_stateManager.pushState(std::make_unique<PauseState>(m_stateManager, width, height, m_renderer, m_textRenderer, this));
        },
        [this]() {
            startNextWave();
        },
        [this]() {
            togglePause();
        },
        [this]() {
            cycleTimeScale();
        },
        [this](float speed) {
            setTimeScale(speed);
        }
    );
}

void GameplayState::update(float dt) {
    if (!m_isValid) {
        if (m_origin == GameplayOrigin::Custom) {
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, width, height, m_renderer, m_textRenderer, LevelTab::Custom));
        } else if (m_origin == GameplayOrigin::Campaign) {
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, width, height, m_renderer, m_textRenderer, LevelTab::Campaign));
        } else {
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, width, height, m_renderer, m_textRenderer));
        }
        return;
    }

    // Если активна тактическая пауза, мир и анимации таймеров не продвигаются
    if (m_isPaused) {
        return;
    }

    // Масштабируем deltaTime с защитой от скачков кадров (макс 50мс)
    float clampedDt = std::min(dt, 0.05f);
    float scaledDt = clampedDt * m_timeScale;

    // Субстеппинг с фиксированным шагом не более 16.6мс для предотвращения туннелирования коллизий
    const float maxSubstep = 0.0166667f;
    float remainingDt = scaledDt;
    while (remainingDt > 0.0001f) {
        float step = std::min(remainingDt, maxSubstep);

        if (m_world->grid && m_pathVisualizer) {
            m_pathVisualizer->update(step, m_world->grid->getCellSize());
        }

        m_world->update(step);
        remainingDt -= step;
    }

    if (m_world->playerStats.baseHealth <= 0) {
        m_world->playerStats.baseHealth = 0;
        if (m_isEditorTest) {
            m_stateManager.pushState(std::make_unique<GameOverState>(m_stateManager, width, height, m_renderer, m_textRenderer, m_currentLevelPath, true, m_origin));
        } else {
            m_stateManager.setState(std::make_unique<GameOverState>(m_stateManager, width, height, m_renderer, m_textRenderer, m_currentLevelPath, false, m_origin));
        }
        return;
    }

    if (m_world->waveManager->isAllWavesCompleted() && m_world->entityManager->getEnemies().empty()) {
        if (m_isEditorTest) {
            m_stateManager.pushState(std::make_unique<VictoryState>(m_stateManager, width, height, m_renderer, m_textRenderer, m_currentLevelPath, true, m_origin));
        } else {
            if (m_origin == GameplayOrigin::Campaign) {
                CampaignManager::completeMission(m_currentLevelPath);
            }
            m_stateManager.setState(std::make_unique<VictoryState>(m_stateManager, width, height, m_renderer, m_textRenderer, m_currentLevelPath, false, m_origin));
        }
        return;
    }
}

void GameplayState::render() {
    if (!m_isValid || !m_gameplayRenderer || !m_world) return;

    glm::vec2 mousePos = m_inputHandler ? m_inputHandler->getMousePos() : glm::vec2(0.0f);
    float pistonAngle = m_inputHandler ? m_inputHandler->getPistonPlacementAngle() : 270.0f;

    m_gameplayRenderer->renderFrame(
        *m_world,
        *m_buildPanel,
        *m_placementUI,
        *m_pathVisualizer,
        *m_statsPanel,
        *m_towerMenuUI,
        m_selectedTowerType,
        m_selectedTowerOnMap,
        mousePos,
        this->width,
        this->height,
        pistonAngle,
        m_timeScale,
        m_isPaused
    );

    if (m_isEditorTest && m_renderer && m_textRenderer) {
        float s = SettingsManager::getUIScaleMultiplier();
        float fontScale = std::clamp(0.50f * s, 0.38f, 0.70f);
        std::string btnText = "[ В редактор ]";
        float textW = m_textRenderer->CalculateTextWidth(btnText, fontScale);
        float btnX = 20.0f;
        float btnY = 20.0f;
        float btnW = std::max(150.0f * s, textW + 24.0f * s);
        float btnH = 40.0f * s;

        bool hovered = (mousePos.x >= btnX && mousePos.x <= btnX + btnW &&
                        mousePos.y >= btnY && mousePos.y <= btnY + btnH);

        auto whiteTex = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
        Texture2D* rawUiTex = ResourceManager::getTexture("uiBaseTexture");
        auto uiTex = rawUiTex ? std::shared_ptr<Texture2D>(rawUiTex, [](Texture2D*) {}) : whiteTex;

        m_renderer->beginBatch();

        glm::vec3 borderColor = hovered ? glm::vec3(0.40f, 0.75f, 1.0f) : glm::vec3(0.24f, 0.30f, 0.38f);
        glm::vec3 bgColor = hovered ? glm::vec3(0.20f, 0.28f, 0.38f) : glm::vec3(0.11f, 0.14f, 0.18f);

        // Рамка кнопки (1px наружу)
        m_renderer->drawSprite(whiteTex, glm::vec2(btnX - 1.0f, btnY - 1.0f), glm::vec2(btnW + 2.0f, btnH + 2.0f), 0.0f, borderColor);
        // Тело кнопки
        m_renderer->drawSprite(uiTex, glm::vec2(btnX, btnY), glm::vec2(btnW, btnH), 0.0f, bgColor);
        // Акцентная полоска слева
        glm::vec3 accentColor = hovered ? glm::vec3(0.35f, 0.85f, 1.0f) : glm::vec3(1.0f, 0.80f, 0.20f);
        m_renderer->drawSprite(whiteTex, glm::vec2(btnX, btnY), glm::vec2(4.0f * s, btnH), 0.0f, accentColor);

        m_renderer->endBatch();

        // Центрированный текст кнопки
        float textX = btnX + (btnW - textW) * 0.5f;
        float textY = btnY + (btnH - fontScale * 28.0f) * 0.5f + 2.0f;
        glm::vec3 textColor = hovered ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(0.90f, 0.92f, 0.95f);
        m_textRenderer->RenderText(btnText, textX, textY, fontScale, textColor);
    }
}

void GameplayState::startNextWave() {
    if (m_world && m_world->waveManager) {
        m_world->waveManager->startNextWave();
    }
}

void GameplayState::cycleTimeScale() {
    if (m_timeScale < 1.5f) {
        m_timeScale = 2.0f;
    } else if (m_timeScale < 3.0f) {
        m_timeScale = 4.0f;
    } else {
        m_timeScale = 1.0f;
    }
    m_isPaused = false;
}

void GameplayState::setTimeScale(float scale) {
    m_timeScale = scale;
    m_isPaused = false;
}

void GameplayState::togglePause() {
    m_isPaused = !m_isPaused;
}


void GameplayState::restartGame() {
    m_world = std::make_unique<GameWorld>();
    m_world->loadLevel(m_currentLevelPath, this->width, this->height);
    m_selectedTowerType = "";
    m_selectedTowerOnMap = nullptr;
    std::cout << "Game Restarted!" << std::endl;
}

void GameplayState::resize(int windowWidth, int windowHeight) {
    this->width = windowWidth;
    this->height = windowHeight;

    if (!m_isValid) return;

    if (m_world) {
        m_world->resize(windowWidth, windowHeight);
    }
}

void GameplayState::saveGame(const std::string& saveName) {
    if (!m_world || !m_world->waveManager || !m_world->entityManager) return;

    SaveManager saveManager;
    saveManager.writeSave(saveName, m_currentLevelPath, m_world->playerStats, *m_world->waveManager, *m_world->entityManager);
    std::cout << "[GameplayState] Игра сохранена в слот: " << saveName << std::endl;
}

bool GameplayState::loadSavedGame(const std::string& saveName) {
    SaveManager saveManager;
    std::string loadedLevelPath;
    PlayerStats loadedStats;
    int loadedWaveIndex = 0;
    nlohmann::json restoredTowers;

    if (!saveManager.readSave(saveName, loadedLevelPath, loadedStats, loadedWaveIndex, restoredTowers)) {
        std::cout << "[LoadGame] Не удалось загрузить файл: " << saveName << std::endl;
        return false;
    }

    std::cout << "[LoadGame] Файл прочитан. Начинаем восстановление стейта..." << std::endl;

    cleanup();

    m_currentLevelPath = loadedLevelPath;
    m_selectedTowerType = "";
    m_selectedTowerOnMap = nullptr;

    m_world = std::make_unique<GameWorld>();
    m_world->loadLevel(m_currentLevelPath, this->width, this->height);

    m_world->playerStats = loadedStats;
    m_world->waveManager->setCurrentWaveIndex(loadedWaveIndex);

    setupUI();
    setupEventListeners();
    AudioEventSystem::playThemeMusic("res/sounds/background.mp3");

    for (const auto& tData : restoredTowers) {
        std::string type = tData.at("type").get<std::string>();
        int gx = tData.at("grid_x").get<int>();
        int gy = tData.at("grid_y").get<int>();
        int lvl = tData.at("level").get<int>();
        int modeInt = tData.at("target_mode").get<int>();

        auto restoredTower = std::make_unique<Tower>(gx, gy, type);
        restoredTower->forceLevel(lvl);
        restoredTower->setTargetMode(static_cast<TargetMode>(modeInt));

        m_world->entityManager->addTower(std::move(restoredTower));
        m_world->grid->setCellType(gx, gy, CellType::Tower);
    }

    m_world->recalculateAllPaths();

    std::cout << "[LoadGame] Мир и маршруты успешно восстановлены с учетом башен!" << std::endl;
    return true;
}