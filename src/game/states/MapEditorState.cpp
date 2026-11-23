#include "MapEditorState.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef APIENTRY
#include <windows.h>
#endif
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "LevelSelectState.h"
#include "GameplayState.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/BackgroundRenderer.h"
#include "../renderer/DecorationRenderer.h"
#include "../resources/ResourceManager.h"
#include "../textures/Texture2D.h"
#include "../world/PathService.h"
#include "../ui/UICommon.h"
#include "../ui/BuildPanel.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <queue>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include "../core/InputManager.h"
#include "../core/LocalizationManager.h"
#include "../core/CampaignManager.h"

static const std::vector<glm::ivec2> c_sizePresets = {
    { 8, 5 },
    { 10, 5 },
    { 12, 5 },
    { 10, 7 },
    { 16, 10 },
    { 20, 12 },
    { 24, 14 },
    { 30, 18 },
    { 40, 24 },
    { 50, 30 },
    { 64, 36 },
    { 80, 48 },
    { 100, 60 },
    { 128, 72 }
};

MapEditorState::MapEditorState(GameStateManager& stateManager, int width, int height,
                               std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer,
                               const std::string& levelToLoad, EditorOrigin origin)
    : m_stateManager(stateManager),
      m_width(width),
      m_height(height),
      m_renderer(renderer),
      m_textRenderer(textRenderer),
      m_origin(origin)
{
    if (!levelToLoad.empty()) {
        std::string fname = levelToLoad;
        size_t lastSlash = fname.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            fname = fname.substr(lastSlash + 1);
        }
        m_currentLevelFileName = fname;
    } else {
        m_mapsBrowserModal.open(m_currentLevelFileName, true);
    }
}

void MapEditorState::returnToOrigin() {
    if (m_origin == EditorOrigin::Campaign) {
        m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Campaign));
    } else if (m_origin == EditorOrigin::Custom) {
        m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Custom));
    } else {
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
    }
}

glm::vec3 MapEditorState::getIdColor(int id) const {
    return EditorToolbarUI::getIdColor(id);
}

void MapEditorState::init() {
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png");
    ResourceManager::loadTexture("particleTexture", "res/textures/particle.png");

    auto wrapNoDelete = [](const std::string& name) {
        Texture2D* tex = ResourceManager::getTexture(name);
        return tex;
    };

    m_uiTexture = wrapNoDelete("uiBaseTexture");
    m_arrowTexture = wrapNoDelete("arrowTexture");
    m_whiteTexture = ResourceManager::getWhiteTexture();

    m_gizmoController.initCursors();

    loadInitialMap();
    updateButtonLayout();
}

void MapEditorState::cleanup() {
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (currentWindow != nullptr) {
        glfwSetCursor(currentWindow, nullptr);
    }
    m_gizmoController.destroyCursors();

    glm::mat4 P_screen = glm::ortho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, -1.0f, 1.0f);
    if (m_renderer) m_renderer->setProjection(P_screen);
    if (m_textRenderer) m_textRenderer->updateProjection(P_screen);
}

void MapEditorState::updateCurrentLevelDisplayName() {
    std::string stem = m_currentLevelFileName;
    if (stem.length() >= 5 && stem.substr(stem.length() - 5) == ".json") {
        stem = stem.substr(0, stem.length() - 5);
    }
    if (stem == "level_editor") {
        m_currentLevelDisplayName = "MY MAP";
    } else {
        m_currentLevelDisplayName = stem;
    }
    m_editorSavePath = "res/levels/" + m_currentLevelFileName;
}

void MapEditorState::loadLevelByName(const std::string& fileName) {
    m_currentLevelFileName = LevelManager::sanitizeLevelFileName(fileName);
    updateCurrentLevelDisplayName();

    std::string path = "res/levels/" + m_currentLevelFileName;
    LevelMapData data = LevelManager::loadLevelMap(path);
    if (data.gridWidth <= 0 || data.gridHeight <= 0) {
        auto dirs = LevelManager::getLevelDirectories();
        for (const auto& d : dirs) {
            std::string p = (std::filesystem::path(d) / m_currentLevelFileName).string();
            data = LevelManager::loadLevelMap(p);
            if (data.gridWidth > 0 && data.gridHeight > 0) break;
        }
    }

    if (data.gridWidth <= 0 || data.gridHeight <= 0) {
        if (m_currentLevelFileName != "level_editor.json") {
            data = LevelManager::loadLevelMap("res/levels/level_1.json");
        }
    }

    if (data.gridWidth > 0 && data.gridHeight > 0) {
        m_gridWidth = data.gridWidth;
        m_gridHeight = data.gridHeight;
        m_cellSize = data.cellSize;
        m_spawners = data.spawners;
        m_bases = data.bases;
        m_rawLayout = data.layout;
        m_noBuildMask = data.noBuildMask;
        if (m_noBuildMask.empty() || (int)m_noBuildMask.size() != m_gridHeight) {
            m_noBuildMask.assign(m_gridHeight, std::vector<bool>(m_gridWidth, false));
        }
        for (auto& row : m_noBuildMask) {
            if ((int)row.size() != m_gridWidth) {
                row.resize(m_gridWidth, false);
            }
        }
        m_minecarts = data.minecarts;
        m_waves = data.waves;
        m_emitters = data.emitters;
        m_envParticleManager.setEmitters(m_emitters);
        m_decorations = data.decorations;
        m_selectedDecorationIndex = -1;
        m_isDraggingDecoration = false;
        m_selectedEntityIndex = -1;
        m_entityInspector.resetPosition();
        m_isCampaign = data.isCampaign;
        m_background = data.background.empty() ? "default" : data.background;
        m_hasSidePockets = data.hasSidePockets;
        m_cameraSettings = data.camera;
        m_isCameraConfigMode = false;
        m_tags = data.tags;
        m_undoStack.clear();
        m_redoStack.clear();
        m_currentStrokeAction.changes.clear();
        m_startingMoney = data.startingMoney;
        m_startingHealth = data.startingHealth;
        m_allowedTowers = data.allowedTowers;
        m_maxUpgradeTier = data.maxUpgradeTier;
        m_towerMaxTiers = data.towerMaxTiers;
        m_enemyBountyRules = data.enemyBountyRules;
        m_enemyHpRules = data.enemyHpRules;
        m_enemyGlobalBounties = data.enemyGlobalBounties;
        if (!data.name.empty()) {
            m_currentLevelDisplayName = data.name;
        }
        m_suppressPlacementUntilRelease = true;
    } else {
        m_gridWidth = 20;
        m_gridHeight = 12;
        m_cellSize = 64.0f;
        m_spawners.clear();
        m_bases.clear();
        m_bases.push_back(BaseData(17, 6, 0));
        SpawnerData sp;
        sp.pos = glm::ivec2(2, 6);
        sp.targetBaseIndex = 0;
        sp.groupId = 0;
        m_spawners.push_back(sp);
        m_rawLayout.assign(12, std::vector<int>(20, 0));
        m_noBuildMask.assign(12, std::vector<bool>(20, false));
        m_minecarts.clear();
        m_waves.clear();
        m_emitters.clear();
        m_envParticleManager.clear();
        m_decorations.clear();
        m_selectedDecorationIndex = -1;
        m_isDraggingDecoration = false;
        m_selectedEntityIndex = -1;
        m_entityInspector.resetPosition();
        m_isCampaign = false;
        m_background = "default";
        m_cameraSettings = LevelMapData::CameraSettings();
        m_isCameraConfigMode = false;
        m_tags = { "Тест" };
        m_startingMoney = 50;
        m_startingHealth = 20;
        m_allowedTowers = { "Basic", "Mercury", "Piston" };
        m_maxUpgradeTier = 3;
        m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
        m_enemyBountyRules.clear();
        m_enemyBountyRules["Coal"] = { { 1, 99, 1, 2 } };
        m_enemyBountyRules["Spark"] = { { 1, 99, 1, 1 } };
        m_enemyBountyRules["Mercury"] = { { 1, 99, 1, 1 } };
        m_enemyHpRules.clear();
        m_enemyGlobalBounties.clear();
        m_enemyGlobalBounties["Coal"] = { 1, 2 };
        m_enemyGlobalBounties["Spark"] = { 1, 1 };
        m_enemyGlobalBounties["Mercury"] = { 1, 1 };
        m_currentLevelDisplayName = "MY MAP";
    }

    if (m_minecarts.empty()) {
        m_minecarts.push_back(MinecartData{});
    }

    if (m_waves.empty()) {
        WaveConfig defWave;
        defWave.parts.push_back({ "Basic", 10, 0.8f, 2.0f });
        m_waves.push_back(defWave);
    }

    if (m_rawLayout.empty() || (int)m_rawLayout.size() != m_gridHeight) {
        m_rawLayout.assign(m_gridHeight, std::vector<int>(m_gridWidth, 0));
    }

    m_grid = std::make_unique<Grid>(m_gridWidth, m_gridHeight, m_cellSize);
    m_grid->setNoBuildMask(m_noBuildMask);
    m_grid->setHasSidePockets(m_hasSidePockets || m_background == "mine_entrance");
    updateGridDimensions();

    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            int cellVal = m_rawLayout[y][x];
            if (cellVal == 1) {
                m_grid->setCellType(x, y, CellType::Path);
            } else if (cellVal == 2) {
                m_grid->setCellType(x, y, CellType::Platform);
            } else if (cellVal == 3) {
                m_grid->setCellType(x, y, CellType::Scenery);
            } else if (cellVal == 4) {
                m_grid->setCellType(x, y, CellType::Chasm);
            } else if (cellVal == 5) {
                m_grid->setCellType(x, y, CellType::Rail);
            } else {
                m_grid->setCellType(x, y, CellType::Ground);
            }
        }
    }

    for (const auto& oc : data.outerCells) {
        CellType ct = CellType::Ground;
        if (oc.type == 1) ct = CellType::Path;
        else if (oc.type == 2) ct = CellType::Platform;
        else if (oc.type == 3) ct = CellType::Scenery;
        else if (oc.type == 4) ct = CellType::Chasm;
        else if (oc.type == 5) ct = CellType::Rail;
        else if (oc.type == 6) ct = CellType::Spawner;
        else if (oc.type == 7) ct = CellType::Base;
        else ct = CellType::Ground;
        m_grid->setCellType(oc.x, oc.y, ct);
        if (oc.noBuild) {
            m_grid->setNoBuildAt(oc.x, oc.y, true);
        }
    }

    for (const auto& sp : m_spawners) {
        if (sp.pos.x >= 0 && sp.pos.x < m_gridWidth && sp.pos.y >= 0 && sp.pos.y < m_gridHeight) {
            m_grid->setCellType(sp.pos.x, sp.pos.y, CellType::Spawner);
        } else if (m_grid->hasSidePockets()) {
            m_grid->setCellType(sp.pos.x, sp.pos.y, CellType::Spawner);
        }
    }

    for (const auto& b : m_bases) {
        if (b.x >= 0 && b.x < m_gridWidth && b.y >= 0 && b.y < m_gridHeight) {
            m_grid->setCellType(b.x, b.y, CellType::Base);
        } else if (m_grid->hasSidePockets()) {
            m_grid->setCellType(b.x, b.y, CellType::Base);
        }
    }

    m_grid->setNoBuildMask(m_noBuildMask);
    m_grid->saveOriginalGrid();
    m_pathfinder = std::make_unique<Pathfinder>(m_gridWidth, m_gridHeight);
    recalculatePaths();

    m_isDirty = false;

    glm::vec2 screenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
    if (m_cameraSettings.isCustom && m_cameraSettings.targetTile.x >= 0.0f && m_cameraSettings.targetTile.y >= 0.0f) {
        m_zoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
        m_cameraPan = screenCenter - getWorldCamCenter() * m_zoom;
    } else {
        m_zoom = 1.0f;
        glm::vec2 defCenter = getDefaultWorldCenter();
        m_cameraPan = screenCenter - defCenter * m_zoom;
    }

    updateButtonLayout();
}

void MapEditorState::loadInitialMap() {
    loadLevelByName(m_currentLevelFileName);
}

void MapEditorState::resizeMap(int newW, int newH, bool expandFromStart) {
    newW = std::clamp(newW, 1, 250);
    newH = std::clamp(newH, 1, 250);
    if (newW == m_gridWidth && newH == m_gridHeight) return;

    int oldW = m_gridWidth;
    int oldH = m_gridHeight;
    int offsetX = expandFromStart ? (newW - oldW) : 0;
    int offsetY = expandFromStart ? (newH - oldH) : 0;

    // Сохраняем существующие тайлы с учетом смещения
    std::vector<std::vector<int>> newLayout(newH, std::vector<int>(newW, 0));
    for (int y = 0; y < oldH; ++y) {
        int ny = y + offsetY;
        if (ny < 0 || ny >= newH) continue;
        for (int x = 0; x < oldW; ++x) {
            int nx = x + offsetX;
            if (nx < 0 || nx >= newW) continue;
            newLayout[ny][nx] = m_rawLayout[y][x];
        }
    }
    m_rawLayout = newLayout;

    std::vector<std::vector<bool>> newNoBuild(newH, std::vector<bool>(newW, false));
    for (int y = 0; y < oldH; ++y) {
        int ny = y + offsetY;
        if (ny < 0 || ny >= newH) continue;
        for (int x = 0; x < oldW; ++x) {
            int nx = x + offsetX;
            if (nx < 0 || nx >= newW) continue;
            if (y < (int)m_noBuildMask.size() && x < (int)m_noBuildMask[y].size()) {
                newNoBuild[ny][nx] = m_noBuildMask[y][x];
            }
        }
    }
    m_noBuildMask = newNoBuild;

    m_gridWidth = newW;
    m_gridHeight = newH;

    // Смещаем спавнеры и удаляем вышедшие за границы новой сетки
    for (auto& s : m_spawners) {
        s.pos += glm::ivec2(offsetX, offsetY);
    }
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [newW, newH](const SpawnerData& s) {
                           return s.pos.x < 0 || s.pos.x >= newW || s.pos.y < 0 || s.pos.y >= newH;
                       }),
        m_spawners.end()
    );

    // Смещаем базы и удаляем вышедшие за границы новой сетки
    for (auto& b : m_bases) {
        b.x += offsetX;
        b.y += offsetY;
    }
    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [newW, newH](const BaseData& b) {
                           return b.x < 0 || b.x >= newW || b.y < 0 || b.y >= newH;
                       }),
        m_bases.end()
    );

    // Смещаем вагонетки
    for (auto& mc : m_minecarts) {
        if (mc.start.x >= 0 && mc.start.y >= 0) {
            mc.start += glm::ivec2(offsetX, offsetY);
            if (mc.start.x < 0 || mc.start.x >= newW || mc.start.y < 0 || mc.start.y >= newH) {
                mc.start = glm::ivec2(-1, -1);
            }
        }
        if (mc.end.x >= 0 && mc.end.y >= 0) {
            mc.end += glm::ivec2(offsetX, offsetY);
            if (mc.end.x < 0 || mc.end.x >= newW || mc.end.y < 0 || mc.end.y >= newH) {
                mc.end = glm::ivec2(-1, -1);
            }
        }
    }

    // Смещаем эмиттеры частиц
    for (auto& e : m_emitters) {
        e.tilePos += glm::vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    }
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                       [newW, newH](const ParticleEmitterConfig& e) {
                           return e.tilePos.x < -20.0f || e.tilePos.x > (newW + 20.0f) ||
                                  e.tilePos.y < -20.0f || e.tilePos.y > (newH + 20.0f);
                       }),
        m_emitters.end()
    );
    m_envParticleManager.setEmitters(m_emitters);

    // Смещаем декорации
    for (auto& d : m_decorations) {
        d.tilePos += glm::vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    }

    // Смещаем кастомную камеру при наличии или при активном режиме настройки камеры
    if ((m_cameraSettings.isCustom || m_isCameraConfigMode) &&
        m_cameraSettings.targetTile.x >= 0.0f && m_cameraSettings.targetTile.y >= 0.0f) {
        if (expandFromStart) {
            m_cameraSettings.targetTile += glm::vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
        }
    }

    m_grid = std::make_unique<Grid>(m_gridWidth, m_gridHeight, m_cellSize);
    updateGridDimensions();

    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            int cellVal = m_rawLayout[y][x];
            if (cellVal == 1) m_grid->setCellType(x, y, CellType::Path);
            else if (cellVal == 2) m_grid->setCellType(x, y, CellType::Platform);
            else if (cellVal == 3) m_grid->setCellType(x, y, CellType::Scenery);
            else if (cellVal == 4) m_grid->setCellType(x, y, CellType::Chasm);
            else if (cellVal == 5) m_grid->setCellType(x, y, CellType::Rail);
            else m_grid->setCellType(x, y, CellType::Ground);
        }
    }

    for (const auto& sp : m_spawners) {
        m_grid->setCellType(sp.pos.x, sp.pos.y, CellType::Spawner);
    }
    for (const auto& b : m_bases) {
        m_grid->setCellType(b.x, b.y, CellType::Base);
    }

    // Обновляем мировую позицию декораций
    for (auto& d : m_decorations) {
        d.worldPos = m_grid->getOffset() + d.tilePos * m_grid->getCellSize();
    }

    m_grid->saveOriginalGrid();
    m_pathfinder = std::make_unique<Pathfinder>(m_gridWidth, m_gridHeight);
    recalculatePaths();

    m_isDirty = true;
    m_statusMessage = "Grid resized to " + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight);
    m_statusColor = glm::vec3(0.3f, 0.9f, 1.0f);
    m_statusTimer = 2.5f;

    updateButtonLayout();
}

void MapEditorState::shiftMap(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    if (m_gridWidth <= 0 || m_gridHeight <= 0) return;

    // Сдвиг сетки тайлов
    std::vector<std::vector<int>> newLayout(m_gridHeight, std::vector<int>(m_gridWidth, 0));
    for (int y = 0; y < m_gridHeight; ++y) {
        int ny = y + dy;
        if (ny < 0 || ny >= m_gridHeight) continue;
        for (int x = 0; x < m_gridWidth; ++x) {
            int nx = x + dx;
            if (nx < 0 || nx >= m_gridWidth) continue;
            newLayout[ny][nx] = m_rawLayout[y][x];
        }
    }
    m_rawLayout = newLayout;

    // Сдвиг спавнеров
    for (auto& s : m_spawners) {
        s.pos += glm::ivec2(dx, dy);
    }
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [this](const SpawnerData& s) {
                           return s.pos.x < 0 || s.pos.x >= m_gridWidth || s.pos.y < 0 || s.pos.y >= m_gridHeight;
                       }),
        m_spawners.end()
    );

    // Сдвиг баз
    for (auto& b : m_bases) {
        b.x += dx;
        b.y += dy;
    }
    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [this](const BaseData& b) {
                           return b.x < 0 || b.x >= m_gridWidth || b.y < 0 || b.y >= m_gridHeight;
                       }),
        m_bases.end()
    );

    // Сдвиг вагонеток
    for (auto& mc : m_minecarts) {
        if (mc.start.x >= 0 && mc.start.y >= 0) {
            mc.start += glm::ivec2(dx, dy);
            if (mc.start.x < 0 || mc.start.x >= m_gridWidth || mc.start.y < 0 || mc.start.y >= m_gridHeight) {
                mc.start = glm::ivec2(-1, -1);
            }
        }
        if (mc.end.x >= 0 && mc.end.y >= 0) {
            mc.end += glm::ivec2(dx, dy);
            if (mc.end.x < 0 || mc.end.x >= m_gridWidth || mc.end.y < 0 || mc.end.y >= m_gridHeight) {
                mc.end = glm::ivec2(-1, -1);
            }
        }
    }

    // Сдвиг эмиттеров частиц
    for (auto& e : m_emitters) {
        e.tilePos += glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
    }
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                       [this](const ParticleEmitterConfig& e) {
                           return e.tilePos.x < -20.0f || e.tilePos.x > (m_gridWidth + 20.0f) ||
                                  e.tilePos.y < -20.0f || e.tilePos.y > (m_gridHeight + 20.0f);
                       }),
        m_emitters.end()
    );
    m_envParticleManager.setEmitters(m_emitters);

    // Сдвиг декораций
    for (auto& d : m_decorations) {
        d.tilePos += glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
        if (m_grid) {
            d.worldPos = m_grid->getOffset() + d.tilePos * m_grid->getCellSize();
        }
    }

    // Сдвиг кастомной камеры
    if (m_cameraSettings.isCustom && m_cameraSettings.targetTile.x >= 0.0f && m_cameraSettings.targetTile.y >= 0.0f) {
        m_cameraSettings.targetTile += glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
    }

    // Перезаполняем ячейки Grid
    if (m_grid) {
        for (int y = 0; y < m_gridHeight; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                int cellVal = m_rawLayout[y][x];
                if (cellVal == 1) m_grid->setCellType(x, y, CellType::Path);
                else if (cellVal == 2) m_grid->setCellType(x, y, CellType::Platform);
                else if (cellVal == 3) m_grid->setCellType(x, y, CellType::Scenery);
                else if (cellVal == 4) m_grid->setCellType(x, y, CellType::Chasm);
                else if (cellVal == 5) m_grid->setCellType(x, y, CellType::Rail);
                else m_grid->setCellType(x, y, CellType::Ground);
            }
        }

        for (const auto& sp : m_spawners) {
            m_grid->setCellType(sp.pos.x, sp.pos.y, CellType::Spawner);
        }
        for (const auto& b : m_bases) {
            m_grid->setCellType(b.x, b.y, CellType::Base);
        }
        m_grid->saveOriginalGrid();
    }

    recalculatePaths();
    m_isDirty = true;

    std::string dirStr = "";
    if (dx < 0) dirStr = "влево";
    else if (dx > 0) dirStr = "вправо";
    else if (dy < 0) dirStr = "вверх";
    else if (dy > 0) dirStr = "вниз";
    showToast("Карта сдвинута " + dirStr + " на 1 кл.", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
}

void MapEditorState::cycleMapSizePreset() {
    int nextIdx = 0;
    for (size_t i = 0; i < c_sizePresets.size(); ++i) {
        if (c_sizePresets[i].x == m_gridWidth && c_sizePresets[i].y == m_gridHeight) {
            nextIdx = (i + 1) % c_sizePresets.size();
            break;
        }
    }
    resizeMap(c_sizePresets[nextIdx].x, c_sizePresets[nextIdx].y);
}

void MapEditorState::cycleSelectedId(int step) {
    if (m_currentBrush == EditorBrush::Base) {
        if (m_selectedId < 0) m_selectedId = 0;
        if (step > 0) {
            m_selectedId = (m_selectedId >= 5) ? 0 : (m_selectedId + 1);
        } else if (step < 0) {
            m_selectedId = (m_selectedId <= 0) ? 5 : (m_selectedId - 1);
        }
    } else {
        if (step > 0) {
            m_selectedId = (m_selectedId >= 5) ? -1 : (m_selectedId + 1);
        } else if (step < 0) {
            m_selectedId = (m_selectedId <= -1) ? 5 : (m_selectedId - 1);
        }
    }

    std::string idStr = (m_selectedId == -1) ? "Auto" : ("#" + std::to_string(m_selectedId));
    m_statusMessage = "Selected Active ID: " + idStr + (m_selectedId == -1 ? " (Nearest Base)" : "");
    m_statusColor = getIdColor(m_selectedId);
    m_statusTimer = 2.5f;

    updateButtonLayout();
}

float MapEditorState::getTopBarHeight() const {
    return EditorToolbarUI::calculateTopBarHeight(m_width, m_height);
}

float MapEditorState::getBottomDockHeight() const {
    return EditorToolbarUI::calculateBottomDockHeight(m_width, m_height);
}

void MapEditorState::showToast(const std::string& msg, const glm::vec4& color) {
    m_toastMessage = msg;
    m_toastColor = color;
    m_toastTimer = 2.8f;
}

EditorContext MapEditorState::buildEditorContext() const {
    EditorContext ctx;
    ctx.screenWidth = m_width;
    ctx.screenHeight = m_height;
    ctx.currentLevelDisplayName = m_currentLevelDisplayName;
    ctx.currentLevelFileName = m_currentLevelFileName;
    ctx.isCampaign = m_isCampaign;
    ctx.background = m_background;
    ctx.cameraIsCustom = m_cameraSettings.isCustom;
    ctx.cameraConfigMode = m_isCameraConfigMode;
    ctx.gridWidth = m_gridWidth;
    ctx.gridHeight = m_gridHeight;
    ctx.currentBrush = m_currentBrush;
    ctx.selectedId = m_selectedId;
    ctx.statusMessage = m_statusMessage;
    ctx.statusTimer = m_statusTimer;
    ctx.statusColor = m_statusColor;
    ctx.hasInvalidSpawner = m_hasInvalidSpawner;
    ctx.missingBaseWarning = m_missingBaseWarning;
    ctx.missingBaseId = m_missingBaseId;
    ctx.hasMinecartStartEnd = !m_minecarts.empty() && m_minecarts[0].hasStart() && m_minecarts[0].hasEnd();
    ctx.isRailPathValid = m_isRailPathValid;
    ctx.railPathSize = m_railPath.size();
    ctx.isWaveModalOpen = m_waveModal.isOpen();
    ctx.isMinecartModalOpen = m_minecartModal.isOpen();
    ctx.isHelpModalOpen = m_helpModal.isOpen();
    ctx.isLevelSettingsModalOpen = m_levelSettingsModal.isOpen();
    ctx.currentCategory = m_toolbarUI.getCategory();
    ctx.hudPreviewMode = m_hudPreviewMode;
    ctx.decorGridSnap = m_decorGridSnap;
    ctx.startingMoney = m_startingMoney;
    ctx.startingHealth = m_startingHealth;
    return ctx;
}

void MapEditorState::cycleHudPreviewMode() {
    switch (m_hudPreviewMode) {
        case HudPreviewMode::None:     m_hudPreviewMode = HudPreviewMode::Scale100; break;
        case HudPreviewMode::Scale100:  m_hudPreviewMode = HudPreviewMode::Scale125; break;
        case HudPreviewMode::Scale125:  m_hudPreviewMode = HudPreviewMode::Scale150; break;
        case HudPreviewMode::Scale150:  m_hudPreviewMode = HudPreviewMode::None;     break;
    }
    updateGridDimensions();
    updateButtonLayout();
    if (m_hudPreviewMode == HudPreviewMode::None) {
        showToast("HUD Preview: Выкл", glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
    } else if (m_hudPreviewMode == HudPreviewMode::Scale100) {
        showToast("HUD Preview: 100%", glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));
    } else if (m_hudPreviewMode == HudPreviewMode::Scale125) {
        showToast("HUD Preview: 125%", glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));
    } else if (m_hudPreviewMode == HudPreviewMode::Scale150) {
        showToast("HUD Preview: 150%", glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));
    }
}

void MapEditorState::updateGridDimensions() {
    if (!m_grid) return;
    if (m_isCameraConfigMode || m_hudPreviewMode != HudPreviewMode::None) {
        // Синхронизация с боем: используем те же отступы сетки, что и в GameplayState
        float bottomMargin = Buildpanel::getBottomBarHeight(m_width, m_height);
        float topMargin = TimeControlUI::getTopMargin(m_width, m_height);
        m_grid->updateCellSize(m_width, m_height, bottomMargin, topMargin);
    } else {
        // Обычный режим редактора: центрирование между верхним хедером и нижним доком редактора
        m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);
    }

    // Актуализируем мировые позиции всех декораций из их tilePos
    for (auto& d : m_decorations) {
        d.worldPos = m_grid->getOffset() + d.tilePos * m_grid->getCellSize();
    }
}

void MapEditorState::updateButtonLayout() {
    m_toolbarUI.updateLayout(m_textRenderer, buildEditorContext());
}

bool MapEditorState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void MapEditorState::recalculatePaths() {
    if (!m_grid || !m_pathfinder) return;

    m_activePaths = PathService::calculateAllPaths(*m_grid, *m_pathfinder, m_spawners, m_bases);

    bool wasInvalidSpawner = m_hasInvalidSpawner;
    m_hasInvalidSpawner = false;
    m_missingBaseWarning = false;
    m_missingBaseId = -1;

    for (size_t i = 0; i < m_spawners.size(); ++i) {
        if (i >= m_activePaths.size() || m_activePaths[i].empty()) {
            m_hasInvalidSpawner = true;
        }

        int targetId = m_spawners[i].targetBaseIndex;
        if (targetId >= 0) {
            bool found = false;
            for (const auto& b : m_bases) {
                if (b.id == targetId) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                m_missingBaseWarning = true;
                m_missingBaseId = targetId;
            }
        }
    }

    if (m_hasInvalidSpawner && !wasInvalidSpawner) {
        showToast("Ошибка пути: спавнер заблокирован!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
    }

    // Перераховуємо маршрут рейок після будь-якої зміни сітки
    recalculateRailPath();
}

// ---------------------------------------------------------------------------
// BFS-валідація маршруту рейок: від m_minecarts[0].start до .end
// ---------------------------------------------------------------------------
void MapEditorState::recalculateRailPath() {
    bool wasRailValid = m_isRailPathValid;
    m_railPath.clear();
    m_isRailPathValid = false;

    if (!m_grid || m_minecarts.empty()) return;
    const auto& mc = m_minecarts[0];
    if (!mc.hasStart() || !mc.hasEnd()) return;

    glm::ivec2 startPt = mc.start;
    glm::ivec2 endPt   = mc.end;

    // Клітинка вважається рейковою якщо rawLayout == 5
    auto isRail = [&](int x, int y) -> bool {
        if (x < 0 || x >= m_gridWidth || y < 0 || y >= m_gridHeight) return false;
        return m_rawLayout[y][x] == 5;
    };

    if (!isRail(startPt.x, startPt.y) || !isRail(endPt.x, endPt.y)) return;

    // BFS (4-напрямки)
    const glm::ivec2 dirs[4] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
    std::vector<std::vector<glm::ivec2>> parent(
        m_gridHeight, std::vector<glm::ivec2>(m_gridWidth, {-1, -1}));
    std::vector<std::vector<bool>> visited(
        m_gridHeight, std::vector<bool>(m_gridWidth, false));

    std::queue<glm::ivec2> q;
    q.push(startPt);
    visited[startPt.y][startPt.x] = true;
    parent[startPt.y][startPt.x] = startPt; // sentinel

    bool found = false;
    while (!q.empty()) {
        glm::ivec2 cur = q.front(); q.pop();
        if (cur.x == endPt.x && cur.y == endPt.y) { found = true; break; }
        for (const auto& d : dirs) {
            int nx = cur.x + d.x, ny = cur.y + d.y;
            if (isRail(nx, ny) && !visited[ny][nx]) {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                q.push({nx, ny});
            }
        }
    }

    if (!found) {
        if (wasRailValid && mc.hasStart() && mc.hasEnd()) {
            showToast("Ошибка рельсов: маршрут разорван!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        }
        return;
    }

    // Відновлення ланцюжка (зворотно від end до start)
    std::vector<glm::ivec2> path;
    glm::ivec2 cur = endPt;
    while (!(cur.x == startPt.x && cur.y == startPt.y)) {
        path.push_back(cur);
        cur = parent[cur.y][cur.x];
    }
    path.push_back(startPt);
    std::reverse(path.begin(), path.end());

    m_railPath = std::move(path);
    m_isRailPathValid = true;
}

void MapEditorState::applyBrush(int gridX, int gridY, EditorBrush brush) {
    bool inBounds = (gridX >= 0 && gridX < m_gridWidth && gridY >= 0 && gridY < m_gridHeight);
    bool allowOuter = (m_hasSidePockets || (m_grid && m_grid->hasSidePockets()));
    if (!inBounds && !allowOuter) return;
    if (!inBounds && (gridX < -50 || gridX > m_gridWidth + 50 || gridY < -50 || gridY > m_gridHeight + 50)) return;

    // Режим ластика:
    if (brush == EditorBrush::Eraser) {
        if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
            glm::vec2 worldPos = m_grid->getOffset() + glm::vec2(gridX + 0.5f, gridY + 0.5f) * m_grid->getCellSize();
            eraseEmitterNear(worldPos, m_grid->getCellSize() * 0.75f);
            return;
        }
        eraseCell(gridX, gridY);
        return;
    }

    // Режим эмиттеров частиц (Свищ пара, Капель, Искры, Дым, Туман)
    if (brush == EditorBrush::EmitterSteamJet || brush == EditorBrush::EmitterWaterDrip ||
        brush == EditorBrush::EmitterSparks || brush == EditorBrush::EmitterSmoke ||
        brush == EditorBrush::EmitterFog) {
        std::string eType = "steam_jet";
        std::string eName = "Свищ пара";
        float pMin = 6.0f, pMax = 14.0f, bDur = 2.5f;

        if (brush == EditorBrush::EmitterWaterDrip) {
            eType = "water_drip";
            eName = "Капель";
            pMin = 1.0f; pMax = 3.0f; bDur = 0.5f;
        } else if (brush == EditorBrush::EmitterSparks) {
            eType = "sparks";
            eName = "Искры";
            pMin = 4.0f; pMax = 10.0f; bDur = 0.8f;
        } else if (brush == EditorBrush::EmitterSmoke) {
            eType = "smoke";
            eName = "Дым";
            pMin = 3.0f; pMax = 8.0f; bDur = 3.0f;
        } else if (brush == EditorBrush::EmitterFog) {
            eType = "fog";
            eName = "Туман";
            pMin = 0.5f; pMax = 1.0f; bDur = 10.0f;
        }

        glm::vec2 targetCenter = glm::vec2(gridX + 0.5f, gridY + 0.5f);
        bool found = false;
        for (auto& em : m_emitters) {
            if (glm::distance(em.tilePos, targetCenter) < 0.6f) {
                em.type = eType;
                em.periodMin = pMin;
                em.periodMax = pMax;
                em.burstDuration = bDur;
                m_envParticleManager.setEmitters(m_emitters);
                found = true;
                break;
            }
        }

        if (!found) {
            ParticleEmitterConfig cfg;
            cfg.id = static_cast<int>(m_emitters.size()) + 1;
            cfg.type = eType;
            cfg.tilePos = targetCenter;
            cfg.angleDeg = m_defaultEmitterAngle;
            cfg.periodMin = pMin;
            cfg.periodMax = pMax;
            cfg.burstDuration = bDur;
            m_emitters.push_back(cfg);
            EditorEmitterRenderer::sortByZIndex(m_emitters, static_cast<int>(m_emitters.size()) - 1);
            m_envParticleManager.setEmitters(m_emitters);
        }

        m_statusMessage = "Эмиттер [" + eName + "] установлен на (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
        m_statusColor = glm::vec3(0.5f, 0.85f, 1.0f);
        m_statusTimer = 2.0f;
        showToast("Эмиттер: " + eName, glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        m_isDirty = true;
        return;
    }

    // Особый случай: клик по УЖЕ существующему спавнеру
    if (brush == EditorBrush::Spawner) {
        for (size_t i = 0; i < m_spawners.size(); ++i) {
            auto& sp = m_spawners[i];
            if (sp.pos.x == gridX && sp.pos.y == gridY) {
                m_selectedEntityType = EditorEntityType::Spawner;
                m_selectedEntityIndex = static_cast<int>(i);
                m_selectedDecorationIndex = -1;
                m_selectedEmitterIndex = -1;
                std::string idStr = (sp.targetBaseIndex == -1) ? "Auto" : ("#" + std::to_string(sp.targetBaseIndex));
                m_statusMessage = "Spawner at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") selected, target Base ID: " + idStr;
                m_statusColor = getIdColor(sp.targetBaseIndex);
                m_statusTimer = 2.5f;
                return;
            }
        }
    }

    // Особый случай: клик по УЖЕ существующей базе
    if (brush == EditorBrush::Base) {
        for (size_t i = 0; i < m_bases.size(); ++i) {
            auto& b = m_bases[i];
            if (b.x == gridX && b.y == gridY) {
                m_selectedEntityType = EditorEntityType::Base;
                m_selectedEntityIndex = static_cast<int>(i);
                m_selectedDecorationIndex = -1;
                m_selectedEmitterIndex = -1;
                m_statusMessage = "Base at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") selected, ID: #" + std::to_string(b.id);
                m_statusColor = getIdColor(b.id);
                m_statusTimer = 2.5f;
                return;
            }
        }
    }

    int changeIdx = recordCellBefore(gridX, gridY);

    if (brush != EditorBrush::NoBuild) {
    // Иначе удаляем старый спавнер или базу в этой точке
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [gridX, gridY](const SpawnerData& s) { return s.pos.x == gridX && s.pos.y == gridY; }),
        m_spawners.end()
    );

    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [gridX, gridY](const BaseData& b) { return b.x == gridX && b.y == gridY; }),
        m_bases.end()
    );

    // Сброс маркеров вагонетки, если на их место ставится не-рейковый тайл
    if (brush != EditorBrush::Rail && brush != EditorBrush::RailStart && brush != EditorBrush::RailEnd) {
        if (!m_minecarts.empty()) {
            if (m_minecarts[0].start == glm::ivec2(gridX, gridY)) m_minecarts[0].start = glm::ivec2(-1, -1);
            if (m_minecarts[0].end == glm::ivec2(gridX, gridY)) m_minecarts[0].end = glm::ivec2(-1, -1);
        }
    }
    }

    CellType cellTypeToSet = CellType::Ground;
    int rawVal = 0;
    bool isCellTypeBrush = false;

    switch (brush) {
        case EditorBrush::Ground:
            rawVal = 0;
            cellTypeToSet = CellType::Ground;
            isCellTypeBrush = true;
            break;
        case EditorBrush::Wall:
            rawVal = 3;
            cellTypeToSet = CellType::Scenery;
            isCellTypeBrush = true;
            break;
        case EditorBrush::Platform:
            rawVal = 2;
            cellTypeToSet = CellType::Platform;
            isCellTypeBrush = true;
            break;
        case EditorBrush::Path:
            rawVal = 1;
            cellTypeToSet = CellType::Path;
            isCellTypeBrush = true;
            break;
        case EditorBrush::Chasm:
            rawVal = 4;
            cellTypeToSet = CellType::Chasm;
            isCellTypeBrush = true;
            break;
        case EditorBrush::Rail:
            rawVal = 5;
            cellTypeToSet = CellType::Rail;
            isCellTypeBrush = true;
            break;
        case EditorBrush::RailStart:
            if (m_minecarts.empty()) m_minecarts.push_back(MinecartData{});
            m_minecarts[0].start = glm::ivec2(gridX, gridY);
            rawVal = 5;
            cellTypeToSet = CellType::Rail;
            isCellTypeBrush = true;
            m_statusMessage = "Minecart Depot [Start] set at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
            m_statusColor = glm::vec3(1.0f, 0.75f, 0.2f);
            m_statusTimer = 2.0f;
            break;
        case EditorBrush::RailEnd:
            if (m_minecarts.empty()) m_minecarts.push_back(MinecartData{});
            m_minecarts[0].end = glm::ivec2(gridX, gridY);
            rawVal = 5;
            cellTypeToSet = CellType::Rail;
            isCellTypeBrush = true;
            m_statusMessage = "Minecart Buffer [End] set at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
            m_statusColor = glm::vec3(1.0f, 0.35f, 0.35f);
            m_statusTimer = 2.0f;
            break;
        case EditorBrush::Spawner:
            rawVal = 0;
            cellTypeToSet = CellType::Spawner;
            isCellTypeBrush = true;
            m_spawners.push_back(SpawnerData(glm::ivec2(gridX, gridY), m_selectedId, m_selectedId, false, true));
            m_selectedEntityType = EditorEntityType::Spawner;
            m_selectedEntityIndex = static_cast<int>(m_spawners.size()) - 1;
            m_selectedDecorationIndex = -1;
            m_selectedEmitterIndex = -1;
            {
                std::string idStr = (m_selectedId == -1) ? "Auto" : ("#" + std::to_string(m_selectedId));
                m_statusMessage = "Placed Spawner with target Base ID: " + idStr;
                m_statusColor = getIdColor(m_selectedId);
                m_statusTimer = 2.0f;
            }
            break;
        case EditorBrush::Base:
            {
                int baseId = (m_selectedId < 0) ? 0 : m_selectedId;
                rawVal = 0;
                cellTypeToSet = CellType::Base;
                isCellTypeBrush = true;
                m_bases.push_back(BaseData(gridX, gridY, baseId, false, true));
                m_selectedEntityType = EditorEntityType::Base;
                m_selectedEntityIndex = static_cast<int>(m_bases.size()) - 1;
                m_selectedDecorationIndex = -1;
                m_selectedEmitterIndex = -1;
                m_statusMessage = "Placed Base with ID: #" + std::to_string(baseId);
                m_statusColor = getIdColor(baseId);
                m_statusTimer = 2.0f;
            }
            break;
        case EditorBrush::NoBuild:
            if (inBounds) {
                if (gridY < (int)m_noBuildMask.size() && gridX < (int)m_noBuildMask[gridY].size()) {
                    m_noBuildMask[gridY][gridX] = true;
                }
            }
            m_grid->setNoBuildAt(gridX, gridY, true);
            m_statusMessage = "No-Build zone set at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
            m_statusColor = glm::vec3(1.0f, 0.35f, 0.35f);
            m_statusTimer = 1.5f;
            break;
        default:
            break;
    }

    if (isCellTypeBrush) {
        if (inBounds) {
            m_rawLayout[gridY][gridX] = rawVal;
        }
        m_grid->setCellType(gridX, gridY, cellTypeToSet);
    }

    recordCellAfter(gridX, gridY, changeIdx);

    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_isDirty = true;
}

void MapEditorState::eraseCell(int gridX, int gridY) {
    bool inBounds = (gridX >= 0 && gridX < m_gridWidth && gridY >= 0 && gridY < m_gridHeight);
    bool allowOuter = (m_hasSidePockets || (m_grid && m_grid->hasSidePockets()));
    if (!inBounds && !allowOuter) return;
    if (!inBounds && (gridX < -50 || gridX > m_gridWidth + 50 || gridY < -50 || gridY > m_gridHeight + 50)) return;

    int changeIdx = recordCellBefore(gridX, gridY);

    if (m_selectedEntityIndex >= 0) {
        if (m_selectedEntityType == EditorEntityType::Spawner && m_selectedEntityIndex < static_cast<int>(m_spawners.size())) {
            if (m_spawners[m_selectedEntityIndex].pos.x == gridX && m_spawners[m_selectedEntityIndex].pos.y == gridY) {
                m_selectedEntityIndex = -1;
            }
        } else if (m_selectedEntityType == EditorEntityType::Base && m_selectedEntityIndex < static_cast<int>(m_bases.size())) {
            if (m_bases[m_selectedEntityIndex].x == gridX && m_bases[m_selectedEntityIndex].y == gridY) {
                m_selectedEntityIndex = -1;
            }
        }
    }

    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [gridX, gridY](const SpawnerData& s) { return s.pos.x == gridX && s.pos.y == gridY; }),
        m_spawners.end()
    );

    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [gridX, gridY](const BaseData& b) { return b.x == gridX && b.y == gridY; }),
        m_bases.end()
    );

    if (!m_minecarts.empty()) {
        if (m_minecarts[0].start == glm::ivec2(gridX, gridY)) {
            m_minecarts[0].start = glm::ivec2(-1, -1);
        }
        if (m_minecarts[0].end == glm::ivec2(gridX, gridY)) {
            m_minecarts[0].end = glm::ivec2(-1, -1);
        }
    }

    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                       [gridX, gridY](const ParticleEmitterConfig& e) {
                           return static_cast<int>(std::round(e.tilePos.x)) == gridX &&
                                  static_cast<int>(std::round(e.tilePos.y)) == gridY;
                       }),
        m_emitters.end()
    );
    m_envParticleManager.removeEmitterAt(gridX, gridY);

    if (inBounds) {
        m_rawLayout[gridY][gridX] = 0;
        if (gridY < (int)m_noBuildMask.size() && gridX < (int)m_noBuildMask[gridY].size()) {
            m_noBuildMask[gridY][gridX] = false;
            m_grid->setNoBuildAt(gridX, gridY, false);
        }
        m_grid->setCellType(gridX, gridY, CellType::Ground);
    } else {
        m_grid->setNoBuildAt(gridX, gridY, false);
        m_grid->setCellType(gridX, gridY, CellType::Empty);
    }

    recordCellAfter(gridX, gridY, changeIdx);

    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_isDirty = true;
}

void MapEditorState::removeNoBuildAt(int gridX, int gridY) {
    bool inBounds = (gridX >= 0 && gridX < m_gridWidth && gridY >= 0 && gridY < m_gridHeight);
    bool allowOuter = (m_hasSidePockets || (m_grid && m_grid->hasSidePockets()));
    if (!inBounds && !allowOuter) return;
    if (!inBounds && (gridX < -50 || gridX > m_gridWidth + 50 || gridY < -50 || gridY > m_gridHeight + 50)) return;

    if (inBounds) {
        if (gridY < (int)m_noBuildMask.size() && gridX < (int)m_noBuildMask[gridY].size()) {
            if (m_noBuildMask[gridY][gridX]) {
                int changeIdx = recordCellBefore(gridX, gridY);
                m_noBuildMask[gridY][gridX] = false;
                m_grid->setNoBuildAt(gridX, gridY, false);
                recordCellAfter(gridX, gridY, changeIdx);
                m_isDirty = true;
                showToast("Зона запрета стройки удалена", glm::vec4(0.3f, 0.85f, 0.4f, 1.0f));
            }
        }
    } else if (m_grid && m_grid->isNoBuild(gridX, gridY)) {
        int changeIdx = recordCellBefore(gridX, gridY);
        m_grid->setNoBuildAt(gridX, gridY, false);
        recordCellAfter(gridX, gridY, changeIdx);
        m_isDirty = true;
        showToast("Зона запрета стройки удалена", glm::vec4(0.3f, 0.85f, 0.4f, 1.0f));
    }
}

int MapEditorState::findNearestEmitterIndex(glm::vec2 worldPos, float maxDist) const {
    if (!m_grid || m_emitters.empty()) return -1;
    float cellSize = m_grid->getCellSize();
    int bestIdx = -1;
    float bestDistSq = maxDist * maxDist;

    for (size_t i = 0; i < m_emitters.size(); ++i) {
        glm::vec2 emWorld = m_grid->getOffset() + m_emitters[i].tilePos * cellSize;
        float dSq = glm::dot(worldPos - emWorld, worldPos - emWorld);
        if (dSq < bestDistSq) {
            bestDistSq = dSq;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

void MapEditorState::eraseEmitterNear(glm::vec2 worldPos, float maxDist) {
    int idx = findNearestEmitterIndex(worldPos, maxDist);
    if (idx >= 0 && idx < static_cast<int>(m_emitters.size())) {
        std::string eName = m_emitters[idx].type;
        m_emitters.erase(m_emitters.begin() + idx);
        m_envParticleManager.setEmitters(m_emitters);
        if (m_selectedEmitterIndex == idx) {
            m_selectedEmitterIndex = -1;
            m_isDraggingEmitter = false;
        } else if (m_selectedEmitterIndex > idx) {
            m_selectedEmitterIndex--;
        }
        m_isDirty = true;
        showToast("Эмиттер удален: " + eName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
    }
}

int MapEditorState::findNearestDecorationIndex(glm::vec2 worldPos, float maxDist) const {
    if (m_currentBrush == EditorBrush::NoBuild) return -1;
    if (m_decorations.empty()) return -1;

    // Сначала проверяем уже выделенный декор (приоритет прямого захвата для свободного перемещения)
    if (m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
        const auto& selDec = m_decorations[m_selectedDecorationIndex];
        glm::vec2 selWorld = m_grid ? (m_grid->getOffset() + selDec.tilePos * m_grid->getCellSize()) : selDec.worldPos;
        float baseDim = (selDec.type == "fog") ? 65.0f : ((selDec.type == "grass_field") ? 32.0f : ((selDec.type == "bush") ? 28.0f : (selDec.type == "puddle" ? 26.0f : 21.0f)));
        float selRadius = std::max(maxDist, baseDim * selDec.scale * std::max(selDec.scaleX, selDec.scaleY));
        if (glm::distance(worldPos, selWorld) <= selRadius) {
            return m_selectedDecorationIndex;
        }
    }

    int bestIdx = -1;
    float bestScore = 1e9f;

    for (size_t i = 0; i < m_decorations.size(); ++i) {
        const auto& d = m_decorations[i];
        glm::vec2 decWorld = m_grid ? (m_grid->getOffset() + d.tilePos * m_grid->getCellSize()) : d.worldPos;
        float dist = glm::distance(worldPos, decWorld);
        float baseDim = (d.type == "fog") ? 65.0f : ((d.type == "grass_field") ? 32.0f : ((d.type == "bush") ? 28.0f : (d.type == "puddle" ? 26.0f : 21.0f)));
        float effectiveRadius = std::max(maxDist, baseDim * d.scale * std::max(d.scaleX, d.scaleY));
        if (dist <= effectiveRadius) {
            float score = dist / effectiveRadius;
            if (score < bestScore) {
                bestScore = score;
                bestIdx = static_cast<int>(i);
            }
        }
    }
    return bestIdx;
}

void MapEditorState::eraseDecorationNear(glm::vec2 worldPos, float maxDist) {
    int idx = findNearestDecorationIndex(worldPos, maxDist);
    if (idx >= 0 && idx < static_cast<int>(m_decorations.size())) {
        std::string dName = m_decorations[idx].type;
        m_decorations.erase(m_decorations.begin() + idx);
        if (m_selectedDecorationIndex == idx) {
            m_selectedDecorationIndex = -1;
            m_isDraggingDecoration = false;
        } else if (m_selectedDecorationIndex > idx) {
            m_selectedDecorationIndex--;
        }
        m_isDirty = true;
        showToast("Декор удален: " + dName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
    }
}

void MapEditorState::saveMap() {
    if (m_isCameraConfigMode) {
        syncCameraSettingsFromCurrentView();
    }
    LevelMapData data;
    data.name = m_currentLevelDisplayName;
    data.isCampaign = m_isCampaign;
    data.background = m_background;
    data.hasSidePockets = m_hasSidePockets;
    data.camera = m_cameraSettings;
    data.tags = m_tags;
    data.gridWidth = m_gridWidth;
    data.gridHeight = m_gridHeight;
    data.cellSize = m_grid->getCellSize();
    data.offsetX = m_grid->getOffset().x;
    data.offsetY = m_grid->getOffset().y;
    data.spawners = m_spawners;
    data.bases = m_bases;
    data.layout = m_rawLayout;
    data.noBuildMask = m_noBuildMask;

    if (m_grid) {
        const auto& outerCells = m_grid->getOuterCells();
        const auto& outerNoBuild = m_grid->getOuterNoBuild();
        std::set<std::pair<int, int>> allOuterCoords;
        for (const auto& [pos, _] : outerCells) allOuterCoords.insert(pos);
        for (const auto& pos : outerNoBuild) allOuterCoords.insert(pos);

        for (const auto& pos : allOuterCoords) {
            LevelMapData::OuterCellData oc;
            oc.x = pos.first;
            oc.y = pos.second;
            auto itC = outerCells.find(pos);
            CellType ct = (itC != outerCells.end()) ? itC->second : CellType::Ground;
            int typeVal = 0;
            if (ct == CellType::Path) typeVal = 1;
            else if (ct == CellType::Platform) typeVal = 2;
            else if (ct == CellType::Scenery) typeVal = 3;
            else if (ct == CellType::Chasm) typeVal = 4;
            else if (ct == CellType::Rail) typeVal = 5;
            else if (ct == CellType::Spawner) typeVal = 6;
            else if (ct == CellType::Base) typeVal = 7;
            oc.type = typeVal;
            oc.noBuild = (outerNoBuild.find(pos) != outerNoBuild.end());
            data.outerCells.push_back(oc);
        }
    }

    std::vector<MinecartData> activeCarts;
    for (const auto& mc : m_minecarts) {
        if (mc.hasStart() || mc.hasEnd()) {
            activeCarts.push_back(mc);
        }
    }
    data.minecarts = activeCarts;
    data.waves = m_waves;
    data.emitters = m_emitters;
    if (m_grid) {
        for (auto& dec : m_decorations) {
            dec.worldPos = m_grid->getOffset() + dec.tilePos * m_grid->getCellSize();
        }
    }
    data.decorations = m_decorations;
    data.startingMoney = m_startingMoney;
    data.startingHealth = m_startingHealth;
    data.allowedTowers = m_allowedTowers;
    data.maxUpgradeTier = m_maxUpgradeTier;
    data.towerMaxTiers = m_towerMaxTiers;
    data.enemyBountyRules = m_enemyBountyRules;
    data.enemyHpRules = m_enemyHpRules;
    data.enemyGlobalBounties = m_enemyGlobalBounties;

    bool ok = LevelManager::saveLevel(m_currentLevelFileName, data);

    if (ok) {
        m_isDirty = false;
        m_statusMessage = "Map saved: " + m_currentLevelDisplayName + " (" + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight) + ") [$" + std::to_string(m_startingMoney) + " | " + std::to_string(m_startingHealth) + " HP]!";
        m_statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
        showToast("Карта сохранена: $" + std::to_string(m_startingMoney) + " | " + std::to_string(m_startingHealth) + " HP", glm::vec4(0.2f, 0.8f, 0.3f, 1.0f));
    } else {
        m_statusMessage = "Error saving map file!";
        m_statusColor = glm::vec3(1.0f, 0.3f, 0.3f);
        showToast("Ошибка сохранения файла!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
    }
    m_statusTimer = 3.5f;
}

void MapEditorState::clearMap() {
    m_spawners.clear();
    m_bases.clear();
    m_emitters.clear();
    m_envParticleManager.clear();
    m_decorations.clear();
    m_startingMoney = 50;
    m_startingHealth = 20;
    m_allowedTowers = { "Basic", "Mercury", "Piston" };
    m_maxUpgradeTier = 3;
    m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
    m_enemyBountyRules.clear();
    m_enemyBountyRules["Coal"] = { { 1, 99, 1, 2 } };
    m_enemyBountyRules["Spark"] = { { 1, 99, 1, 1 } };
    m_enemyBountyRules["Mercury"] = { { 1, 99, 1, 1 } };
    m_enemyHpRules.clear();
    m_enemyGlobalBounties.clear();
    m_enemyGlobalBounties["Coal"] = { 1, 2 };
    m_enemyGlobalBounties["Spark"] = { 1, 1 };
    m_enemyGlobalBounties["Mercury"] = { 1, 1 };
    m_selectedDecorationIndex = -1;
    m_isDraggingDecoration = false;
    m_selectedEntityIndex = -1;
    m_entityInspector.resetPosition();
    m_minecarts.clear();
    m_minecarts.push_back(MinecartData{});
    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            m_rawLayout[y][x] = 0;
            m_grid->setCellType(x, y, CellType::Ground);
        }
    }
    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_undoStack.clear();
    m_redoStack.clear();
    m_currentStrokeAction.changes.clear();
    m_statusMessage = "Map cleared to Ground.";
    m_statusColor = glm::vec3(0.9f, 0.9f, 0.3f);
    m_statusTimer = 2.0f;
    showToast("Карта очищена", glm::vec4(0.9f, 0.9f, 0.3f, 1.0f));
}

static int cellTypeToTileCode(CellType ct) {
    switch (ct) {
        case CellType::Path:     return 1;
        case CellType::Platform: return 2;
        case CellType::Scenery:  return 3;
        case CellType::Chasm:    return 4;
        case CellType::Rail:     return 5;
        case CellType::Spawner:  return 6;
        case CellType::Base:     return 7;
        case CellType::Ground:   return 0;
        case CellType::Empty:
        default:                 return -1;
    }
}

static CellType tileCodeToCellType(int code) {
    switch (code) {
        case 1:  return CellType::Path;
        case 2:  return CellType::Platform;
        case 3:  return CellType::Scenery;
        case 4:  return CellType::Chasm;
        case 5:  return CellType::Rail;
        case 6:  return CellType::Spawner;
        case 7:  return CellType::Base;
        case 0:  return CellType::Ground;
        case -1:
        default: return CellType::Empty;
    }
}

int MapEditorState::recordCellBefore(int x, int y) {
    if (m_isApplyingUndoRedo) return -1;
    bool inBounds = (x >= 0 && x < m_gridWidth && y >= 0 && y < m_gridHeight);
    bool allowOuter = (m_hasSidePockets || (m_grid && m_grid->hasSidePockets()));
    if (!inBounds && !allowOuter) return -1;
    if (!inBounds && (x < -50 || x > m_gridWidth + 50 || y < -50 || y > m_gridHeight + 50)) return -1;

    for (size_t i = 0; i < m_currentStrokeAction.changes.size(); ++i) {
        if (m_currentStrokeAction.changes[i].x == x && m_currentStrokeAction.changes[i].y == y) {
            return static_cast<int>(i);
        }
    }

    CellChange change;
    change.x = x;
    change.y = y;
    if (inBounds) {
        change.oldTile = (y < (int)m_rawLayout.size() && x < (int)m_rawLayout[y].size()) ? m_rawLayout[y][x] : 0;
        change.oldNoBuild = (y < (int)m_noBuildMask.size() && x < (int)m_noBuildMask[y].size()) ? m_noBuildMask[y][x] : false;
    } else {
        change.oldTile = m_grid ? cellTypeToTileCode(m_grid->getCellType(x, y)) : -1;
        change.oldNoBuild = m_grid ? m_grid->isNoBuild(x, y) : false;
    }

    for (const auto& sp : m_spawners) {
        if (sp.pos.x == x && sp.pos.y == y) change.oldSpawners.push_back(sp);
    }
    for (const auto& b : m_bases) {
        if (b.x == x && b.y == y) change.oldBases.push_back(b);
    }
    if (!m_minecarts.empty()) {
        change.hadOldMinecartStart = (m_minecarts[0].start == glm::ivec2(x, y));
        change.hadOldMinecartEnd = (m_minecarts[0].end == glm::ivec2(x, y));
    }

    m_currentStrokeAction.changes.push_back(change);
    return static_cast<int>(m_currentStrokeAction.changes.size()) - 1;
}

void MapEditorState::recordCellAfter(int x, int y, int changeIndex) {
    if (m_isApplyingUndoRedo) return;
    if (changeIndex < 0 || changeIndex >= static_cast<int>(m_currentStrokeAction.changes.size())) return;

    bool inBounds = (x >= 0 && x < m_gridWidth && y >= 0 && y < m_gridHeight);
    CellChange& change = m_currentStrokeAction.changes[changeIndex];
    if (inBounds) {
        change.newTile = (y < (int)m_rawLayout.size() && x < (int)m_rawLayout[y].size()) ? m_rawLayout[y][x] : 0;
        change.newNoBuild = (y < (int)m_noBuildMask.size() && x < (int)m_noBuildMask[y].size()) ? m_noBuildMask[y][x] : false;
    } else {
        change.newTile = m_grid ? cellTypeToTileCode(m_grid->getCellType(x, y)) : -1;
        change.newNoBuild = m_grid ? m_grid->isNoBuild(x, y) : false;
    }

    change.newSpawners.clear();
    for (const auto& sp : m_spawners) {
        if (sp.pos.x == x && sp.pos.y == y) change.newSpawners.push_back(sp);
    }
    change.newBases.clear();
    for (const auto& b : m_bases) {
        if (b.x == x && b.y == y) change.newBases.push_back(b);
    }
    if (!m_minecarts.empty()) {
        change.hadNewMinecartStart = (m_minecarts[0].start == glm::ivec2(x, y));
        change.hadNewMinecartEnd = (m_minecarts[0].end == glm::ivec2(x, y));
    } else {
        change.hadNewMinecartStart = false;
        change.hadNewMinecartEnd = false;
    }
}

void MapEditorState::commitStroke() {
    if (m_isApplyingUndoRedo) return;

    // Filter out no-op changes
    m_currentStrokeAction.changes.erase(
        std::remove_if(m_currentStrokeAction.changes.begin(), m_currentStrokeAction.changes.end(),
            [](const CellChange& c) {
                return c.oldTile == c.newTile &&
                       c.oldNoBuild == c.newNoBuild &&
                       c.oldSpawners.empty() && c.newSpawners.empty() &&
                       c.oldBases.empty() && c.newBases.empty() &&
                       c.hadOldMinecartStart == c.hadNewMinecartStart &&
                       c.hadOldMinecartEnd == c.hadNewMinecartEnd;
            }),
        m_currentStrokeAction.changes.end()
    );

    if (!m_currentStrokeAction.changes.empty()) {
        m_undoStack.push_back(std::move(m_currentStrokeAction));
        m_currentStrokeAction.changes.clear();
        m_redoStack.clear();
        while (m_undoStack.size() > 100) {
            m_undoStack.pop_front();
        }
    }
}

void MapEditorState::undo() {
    if (m_undoStack.empty()) return;

    EditorAction action = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    m_isApplyingUndoRedo = true;
    for (auto it = action.changes.rbegin(); it != action.changes.rend(); ++it) {
        const auto& c = *it;
        bool inBounds = (c.x >= 0 && c.x < m_gridWidth && c.y >= 0 && c.y < m_gridHeight);
        bool allowOuter = (m_hasSidePockets || (m_grid && m_grid->hasSidePockets()));
        if (!inBounds && !allowOuter) continue;
        if (!inBounds && (c.x < -50 || c.x > m_gridWidth + 50 || c.y < -50 || c.y > m_gridHeight + 50)) continue;

        if (inBounds) {
            m_rawLayout[c.y][c.x] = (c.oldTile >= 0 ? c.oldTile : 0);
            if (c.y < (int)m_noBuildMask.size() && c.x < (int)m_noBuildMask[c.y].size()) {
                m_noBuildMask[c.y][c.x] = c.oldNoBuild;
            }
        }
        m_grid->setNoBuildAt(c.x, c.y, c.oldNoBuild);

        m_spawners.erase(
            std::remove_if(m_spawners.begin(), m_spawners.end(),
                           [&c](const SpawnerData& s) { return s.pos.x == c.x && s.pos.y == c.y; }),
            m_spawners.end()
        );
        for (const auto& sp : c.oldSpawners) {
            m_spawners.push_back(sp);
        }

        m_bases.erase(
            std::remove_if(m_bases.begin(), m_bases.end(),
                           [&c](const BaseData& b) { return b.x == c.x && b.y == c.y; }),
            m_bases.end()
        );
        for (const auto& b : c.oldBases) {
            m_bases.push_back(b);
        }

        if (!m_minecarts.empty()) {
            if (c.hadOldMinecartStart) {
                m_minecarts[0].start = glm::ivec2(c.x, c.y);
            } else if (m_minecarts[0].start == glm::ivec2(c.x, c.y)) {
                m_minecarts[0].start = glm::ivec2(-1, -1);
            }

            if (c.hadOldMinecartEnd) {
                m_minecarts[0].end = glm::ivec2(c.x, c.y);
            } else if (m_minecarts[0].end == glm::ivec2(c.x, c.y)) {
                m_minecarts[0].end = glm::ivec2(-1, -1);
            }
        }

        bool hasSpawner = false;
        for (const auto& sp : m_spawners) {
            if (sp.pos.x == c.x && sp.pos.y == c.y) { hasSpawner = true; break; }
        }
        bool hasBase = false;
        for (const auto& b : m_bases) {
            if (b.x == c.x && b.y == c.y) { hasBase = true; break; }
        }

        if (hasSpawner) {
            m_grid->setCellType(c.x, c.y, CellType::Spawner);
        } else if (hasBase) {
            m_grid->setCellType(c.x, c.y, CellType::Base);
        } else {
            if (inBounds) {
                switch (c.oldTile) {
                    case 1:  m_grid->setCellType(c.x, c.y, CellType::Path); break;
                    case 2:  m_grid->setCellType(c.x, c.y, CellType::Platform); break;
                    case 3:  m_grid->setCellType(c.x, c.y, CellType::Scenery); break;
                    case 4:  m_grid->setCellType(c.x, c.y, CellType::Chasm); break;
                    case 5:  m_grid->setCellType(c.x, c.y, CellType::Rail); break;
                    default: m_grid->setCellType(c.x, c.y, CellType::Ground); break;
                }
            } else {
                m_grid->setCellType(c.x, c.y, tileCodeToCellType(c.oldTile));
            }
        }
    }
    m_isApplyingUndoRedo = false;

    m_grid->saveOriginalGrid();
    recalculatePaths();
    recalculateRailPath();
    m_isDirty = true;

    showToast("Отмена (Undo): " + std::to_string(action.changes.size()) + " яч.", glm::vec4(0.85f, 0.85f, 0.35f, 1.0f));
    m_redoStack.push_back(std::move(action));
}

void MapEditorState::redo() {
    if (m_redoStack.empty()) return;

    EditorAction action = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    m_isApplyingUndoRedo = true;
    for (const auto& c : action.changes) {
        bool inBounds = (c.x >= 0 && c.x < m_gridWidth && c.y >= 0 && c.y < m_gridHeight);
        bool allowOuter = (m_hasSidePockets || (m_grid && m_grid->hasSidePockets()));
        if (!inBounds && !allowOuter) continue;
        if (!inBounds && (c.x < -50 || c.x > m_gridWidth + 50 || c.y < -50 || c.y > m_gridHeight + 50)) continue;

        if (inBounds) {
            m_rawLayout[c.y][c.x] = (c.newTile >= 0 ? c.newTile : 0);
            if (c.y < (int)m_noBuildMask.size() && c.x < (int)m_noBuildMask[c.y].size()) {
                m_noBuildMask[c.y][c.x] = c.newNoBuild;
            }
        }
        m_grid->setNoBuildAt(c.x, c.y, c.newNoBuild);

        m_spawners.erase(
            std::remove_if(m_spawners.begin(), m_spawners.end(),
                           [&c](const SpawnerData& s) { return s.pos.x == c.x && s.pos.y == c.y; }),
            m_spawners.end()
        );
        for (const auto& sp : c.newSpawners) {
            m_spawners.push_back(sp);
        }

        m_bases.erase(
            std::remove_if(m_bases.begin(), m_bases.end(),
                           [&c](const BaseData& b) { return b.x == c.x && b.y == c.y; }),
            m_bases.end()
        );
        for (const auto& b : c.newBases) {
            m_bases.push_back(b);
        }

        if (!m_minecarts.empty()) {
            if (c.hadNewMinecartStart) {
                m_minecarts[0].start = glm::ivec2(c.x, c.y);
            } else if (m_minecarts[0].start == glm::ivec2(c.x, c.y)) {
                m_minecarts[0].start = glm::ivec2(-1, -1);
            }

            if (c.hadNewMinecartEnd) {
                m_minecarts[0].end = glm::ivec2(c.x, c.y);
            } else if (m_minecarts[0].end == glm::ivec2(c.x, c.y)) {
                m_minecarts[0].end = glm::ivec2(-1, -1);
            }
        }

        bool hasSpawner = false;
        for (const auto& sp : m_spawners) {
            if (sp.pos.x == c.x && sp.pos.y == c.y) { hasSpawner = true; break; }
        }
        bool hasBase = false;
        for (const auto& b : m_bases) {
            if (b.x == c.x && b.y == c.y) { hasBase = true; break; }
        }

        if (hasSpawner) {
            m_grid->setCellType(c.x, c.y, CellType::Spawner);
        } else if (hasBase) {
            m_grid->setCellType(c.x, c.y, CellType::Base);
        } else {
            if (inBounds) {
                switch (c.newTile) {
                    case 1:  m_grid->setCellType(c.x, c.y, CellType::Path); break;
                    case 2:  m_grid->setCellType(c.x, c.y, CellType::Platform); break;
                    case 3:  m_grid->setCellType(c.x, c.y, CellType::Scenery); break;
                    case 4:  m_grid->setCellType(c.x, c.y, CellType::Chasm); break;
                    case 5:  m_grid->setCellType(c.x, c.y, CellType::Rail); break;
                    default: m_grid->setCellType(c.x, c.y, CellType::Ground); break;
                }
            } else {
                m_grid->setCellType(c.x, c.y, tileCodeToCellType(c.newTile));
            }
        }
    }
    m_isApplyingUndoRedo = false;

    m_grid->saveOriginalGrid();
    recalculatePaths();
    recalculateRailPath();
    m_isDirty = true;

    showToast("Повтор (Redo): " + std::to_string(action.changes.size()) + " яч.", glm::vec4(0.35f, 0.85f, 0.45f, 1.0f));
    m_undoStack.push_back(std::move(action));
}

void MapEditorState::testMap() {
    if (m_spawners.empty() || m_bases.empty()) {
        m_statusMessage = "Cannot test: Place at least 1 Spawner and 1 Base!";
        m_statusColor = glm::vec3(1.0f, 0.3f, 0.3f);
        m_statusTimer = 4.0f;
        showToast("Нельзя тестировать: добавьте спавнер и базу!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        return;
    }

    if (m_hasInvalidSpawner) {
        m_statusMessage = "Cannot test: One or more Spawners have no path to a Base!";
        m_statusColor = glm::vec3(1.0f, 0.2f, 0.2f);
        m_statusTimer = 4.0f;
        showToast("Ошибка валидации: спавнер заблокирован!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        return;
    }

    saveMap();
    std::string testPath = "res/levels/" + m_currentLevelFileName;
    m_suppressPlacementUntilRelease = true;
    m_stateManager.pushState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, testPath, /*isEditorTest=*/true, GameplayOrigin::EditorTest));
}

void MapEditorState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glm::vec2 mousePos(static_cast<float>(mouseX), static_cast<float>(mouseY));
    m_mousePos = mousePos;

    bool leftDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    bool rightDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    // Подавляем клики от предыдущего состояния до полного отпускания кнопок мыши
    if (m_suppressClickUntilRelease) {
        if (!leftDown && !rightDown) {
            m_suppressClickUntilRelease = false;
            m_isLeftMouseDown = false;
            m_isRightMouseDown = false;
        } else {
            m_isLeftMouseDown = leftDown;
            m_isRightMouseDown = rightDown;
            return;
        }
    }

    // Если открыто модальное окно подтверждения выхода
    if (m_exitModal.isOpen()) {
        int key = 0;
        bool keyReleased = false;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
            key = GLFW_KEY_ESCAPE;
            keyReleased = true;
        } else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            key = GLFW_KEY_ESCAPE;
            keyReleased = false;
        } else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) {
            key = GLFW_KEY_ENTER;
            keyReleased = false;
        }

        bool mouseClicked = leftDown && !m_isLeftMouseDown;
        ExitModalAction action = m_exitModal.handleInput(mousePos, mouseClicked, key, keyReleased);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;

        if (action == ExitModalAction::SaveAndExit) {
            saveMap();
            returnToOrigin();
            return;
        } else if (action == ExitModalAction::DiscardAndExit) {
            returnToOrigin();
            return;
        } else if (action == ExitModalAction::Cancel) {
            return;
        }
        return;
    }

    // Если открыто модальное окно управления картами (список карт / переименование)
    if (m_mapsBrowserModal.isOpen()) {
        bool handled = m_mapsBrowserModal.handleInput(
            window, mousePos, leftDown, dt, m_width, m_height,
            /* onLoadLevel */ [this](const std::string& fileName) {
                loadLevelByName(fileName);
                m_statusMessage = "Loaded map: " + m_currentLevelDisplayName;
                m_statusColor = glm::vec3(0.3f, 0.9f, 1.0f);
                m_statusTimer = 3.0f;
            },
            /* onNewLevel */ [this]() {
                std::string newF = LevelManager::createNewLevel("custom_map");
                loadLevelByName(newF);
                m_statusMessage = "Created and loaded: " + m_currentLevelDisplayName;
                m_statusColor = glm::vec3(0.2f, 1.0f, 0.4f);
                m_statusTimer = 3.5f;
            },
            /* onRenameLevel */ [this](const std::string& oldFile, const std::string& newFile, const std::string& newName) {
                if (oldFile == m_currentLevelFileName) {
                    m_currentLevelFileName = newFile;
                    m_currentLevelDisplayName = newName;
                    m_editorSavePath = "res/levels/" + m_currentLevelFileName;
                }
                m_statusMessage = "Renamed: " + newName;
                m_statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
                m_statusTimer = 3.5f;
                updateButtonLayout();
            },
            /* onReturnToOrigin */ [this]() {
                returnToOrigin();
            }
        );
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        m_suppressPlacementUntilRelease = true;
        if (handled) return;
    }

    // Если открыто модальное окно настроек вагонетки
    if (m_minecartModal.isOpen()) {
        bool handled = m_minecartModal.handleInput(window, mousePos, leftDown, m_width, m_height,
                                                   m_minecarts, m_isDirty,
                                                   [this]() { recalculateRailPath(); });
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Если открыто модальное окно настроек уровня и экономики
    if (m_levelSettingsModal.isOpen()) {
        bool keyEscCheck = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        bool handled = m_levelSettingsModal.handleInput(window, mousePos, leftDown, !leftDown && m_isLeftMouseDown,
                                                       keyEscCheck && !m_keyEscPressedLastFrame, dt,
                                                       m_width, m_height,
                                                       m_startingMoney, m_startingHealth,
                                                       m_allowedTowers, m_towerMaxTiers,
                                                       m_maxUpgradeTier, m_enemyBountyRules, m_enemyHpRules, m_hasSidePockets, m_isDirty);
        if (handled && m_grid) {
            m_grid->setHasSidePockets(m_hasSidePockets || m_background == "mine_entrance");
            recalculatePaths();
        }
        if (keyEscCheck && !m_keyEscPressedLastFrame) {
            m_keyEscPressedLastFrame = keyEscCheck;
        }
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        m_suppressPlacementUntilRelease = true;
        if (handled) return;
    }

    // Если открыто модальное окно справки по управлению
    if (m_helpModal.isOpen()) {
        bool keyEscCheck = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        bool handled = m_helpModal.handleInput(window, mousePos, leftDown, !leftDown && m_isLeftMouseDown, keyEscCheck && !m_keyEscPressedLastFrame);
        if (keyEscCheck && !m_keyEscPressedLastFrame) {
            m_keyEscPressedLastFrame = keyEscCheck;
        }
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        m_suppressPlacementUntilRelease = true;
        if (handled) return;
    }

    // Горячая клавиша Waves (W)
    bool keyW = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
    if (keyW && !m_keyWPressedLastFrame) {
        m_waveModal.toggle(m_waves);
        if (m_waveModal.isOpen()) {
            m_statusMessage = "Wave Editor: Configure waves, sub-waves and pacing.";
            m_statusColor = glm::vec3(0.3f, 0.85f, 1.0f);
            m_statusTimer = 2.0f;
        }
    }
    m_keyWPressedLastFrame = keyW;

    // Горячая клавиша Справки (H или F1)
    bool keyH = (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS);
    bool keyF1 = (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);
    if ((keyH && !m_keyHPressedLastFrame) || (keyF1 && !m_keyF1PressedLastFrame)) {
        m_helpModal.toggle();
        m_suppressPlacementUntilRelease = true;
    }
    m_keyHPressedLastFrame = keyH;
    m_keyF1PressedLastFrame = keyF1;

    // Горячая клавиша Escape
    bool keyEsc = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    if (keyEsc && !m_keyEscPressedLastFrame) {
        if (m_helpModal.isOpen()) {
            m_helpModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_selectedDecorationIndex >= 0) {
            m_selectedDecorationIndex = -1;
            m_isDraggingDecoration = false;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_selectedEmitterIndex >= 0) {
            m_selectedEmitterIndex = -1;
            m_isDraggingEmitter = false;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_selectedEntityIndex >= 0) {
            m_selectedEntityIndex = -1;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_currentBrush != EditorBrush::None) {
            m_currentBrush = EditorBrush::None;
            m_isDraggingDecoration = false;
            m_isDraggingEmitter = false;
            showToast("Кисть сброшена (нейтральный курсор)");
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_exitModal.isOpen()) {
            closeExitModal();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_mapsBrowserModal.isOpen()) {
            m_mapsBrowserModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_waveModal.isOpen()) {
            m_waveModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_minecartModal.isOpen()) {
            m_minecartModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_levelSettingsModal.isOpen()) {
            m_levelSettingsModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        openExitModal();
        m_keyEscPressedLastFrame = keyEsc;
        return;
    }
    m_keyEscPressedLastFrame = keyEsc;

    // Если открыт редактор волн, перехватываем ввод
    if (m_waveModal.isOpen()) {
        bool handled = m_waveModal.handleInput(window, mousePos, leftDown, dt,
                                               m_width, m_height, getTopBarHeight(), getBottomDockHeight(),
                                               m_waves, m_isDirty);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    bool isShift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool isAlt   = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
    bool isCtrl  = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

    // Переключатель видимости стрелок путей (V)
    bool keyV = (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS);
    if (keyV && !m_keyVPressedLastFrame) {
        m_showPathArrows = !m_showPathArrows;
        showToast(m_showPathArrows ? "Стрелки путей: ВКЛ" : "Стрелки путей: ВЫКЛ",
                  m_showPathArrows ? glm::vec4(0.3f, 0.85f, 0.4f, 1.0f) : glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
    }
    m_keyVPressedLastFrame = keyV;

    // Быстрое переключение категорий палитры (клавиша TAB или Ctrl+1 / Ctrl+2)
    bool keyTab = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
    if (keyTab && !m_keyTabPressedLastFrame) {
        if (m_toolbarUI.getCategory() == PaletteCategory::Tiles) {
            m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
            showToast("Палитра: Спец-объекты");
        } else if (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects) {
            m_toolbarUI.setCategory(PaletteCategory::Particles);
            showToast("Палитра: Партиклы");
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
            m_toolbarUI.setCategory(PaletteCategory::Decorations);
            showToast("Палитра: Декор");
        } else {
            m_toolbarUI.setCategory(PaletteCategory::Tiles);
            showToast("Палитра: Тайлы");
        }
        updateButtonLayout();
    }
    m_keyTabPressedLastFrame = keyTab;

    if (isCtrl) {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::Tiles) {
                m_toolbarUI.setCategory(PaletteCategory::Tiles);
                updateButtonLayout();
                showToast("Палитра: Тайлы");
            }
        } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                updateButtonLayout();
                showToast("Палитра: Спец-объекты");
            }
        } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::Particles) {
                m_toolbarUI.setCategory(PaletteCategory::Particles);
                updateButtonLayout();
                showToast("Палитра: Партиклы");
            }
        } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
                m_toolbarUI.setCategory(PaletteCategory::Decorations);
                updateButtonLayout();
                showToast("Палитра: Декор");
            }
        }
    } else if (!isAlt) {
        // Цифровые горячие клавиши внутри активной вкладки
        bool key1 = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
        bool key2 = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
        bool key3 = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
        bool key4 = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS);
        bool key5 = (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS);
        bool key6 = (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS);
        bool key7 = (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS);
        bool key8 = (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS);
        bool key9 = (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS);
        bool key0 = (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);

        if (key0) {
            m_currentBrush = EditorBrush::Eraser;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Tiles) {
            if (key1) m_currentBrush = EditorBrush::Ground;
            if (key2) m_currentBrush = EditorBrush::Wall;
            if (key3) m_currentBrush = EditorBrush::Platform;
            if (key4) m_currentBrush = EditorBrush::Path;
            if (key5) m_currentBrush = EditorBrush::Chasm;
            if (key6) {
                m_currentBrush = EditorBrush::NoBuild;
                m_selectedDecorationIndex = -1;
                m_selectedEntityIndex = -1;
                m_selectedEmitterIndex = -1;
            }
        } else if (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects) {
            if (key1) m_currentBrush = EditorBrush::Spawner;
            if (key2) {
                m_currentBrush = EditorBrush::Base;
                if (m_selectedId < 0) {
                    m_selectedId = 0;
                    updateButtonLayout();
                }
            }
            if (key3) m_currentBrush = EditorBrush::Rail;
            if (key4) m_currentBrush = EditorBrush::RailStart;
            if (key5) m_currentBrush = EditorBrush::RailEnd;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
            if (key1) m_currentBrush = EditorBrush::EmitterSteamJet;
            if (key2) m_currentBrush = EditorBrush::EmitterWaterDrip;
            if (key3) m_currentBrush = EditorBrush::EmitterSparks;
            if (key4) m_currentBrush = EditorBrush::EmitterSmoke;
            if (key5) m_currentBrush = EditorBrush::EmitterFog;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Decorations) {
            if (m_toolbarUI.getDecorPage() == 0) {
                if (key1) m_currentBrush = EditorBrush::DecorBush;
                if (key2) m_currentBrush = EditorBrush::DecorGrass;
                if (key3) m_currentBrush = EditorBrush::DecorGrassField;
                if (key4) m_currentBrush = EditorBrush::DecorFlower;
                if (key5) m_currentBrush = EditorBrush::DecorStone;
                if (key6) m_currentBrush = EditorBrush::DecorHelmet;
                if (key7) m_currentBrush = EditorBrush::DecorPickaxe;
                if (key8) m_currentBrush = EditorBrush::DecorPuddle;
                if (key9) m_currentBrush = EditorBrush::DecorCrack;
                if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) m_currentBrush = EditorBrush::DecorFog;
            } else {
                if (key1) m_currentBrush = EditorBrush::DecorWoodenCrate;
                if (key2) m_currentBrush = EditorBrush::DecorBrokenCart;
                if (key3) m_currentBrush = EditorBrush::DecorScrapDebris;
                if (key4) m_currentBrush = EditorBrush::DecorDeadTree;
                if (key5) m_currentBrush = EditorBrush::DecorDryGrass;
                if (key6) m_currentBrush = EditorBrush::DecorOreDebris;
            }
        }

        // Прямые клавиши 6, 7, 8 с переключением категории при необходимости (если не на вкладке Декор)
        if (m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
            if (key6) {
                m_currentBrush = EditorBrush::Base;
                if (m_selectedId < 0) {
                    m_selectedId = 0;
                }
                if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                    m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                    updateButtonLayout();
                }
            }
            if (key7) {
                m_currentBrush = EditorBrush::Chasm;
                if (m_toolbarUI.getCategory() != PaletteCategory::Tiles) {
                    m_toolbarUI.setCategory(PaletteCategory::Tiles);
                    updateButtonLayout();
                }
            }
            if (key8) {
                m_currentBrush = EditorBrush::Rail;
                if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                    m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                    updateButtonLayout();
                }
            }
        }
    }

    // Горячие клавиши переключения ID: [ и ] или - и =
    bool keyLeftBracket = (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS);
    if (keyLeftBracket && !m_keyLeftBracketLastFrame) {
        cycleSelectedId(-1);
    }
    m_keyLeftBracketLastFrame = keyLeftBracket;

    bool keyRightBracket = (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS);
    if (keyRightBracket && !m_keyRightBracketLastFrame) {
        cycleSelectedId(+1);
    }
    m_keyRightBracketLastFrame = keyRightBracket;

    // Горячая клавиша Save (Ctrl + S или S)
    bool keyS = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
    if (keyS && !m_keySPressedLastFrame) {
        saveMap();
    }
    m_keySPressedLastFrame = keyS;

    // Горячая клавиша Test (T)
    bool keyT = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);
    if (keyT && !m_keyTPressedLastFrame) {
        testMap();
        return;
    }
    m_keyTPressedLastFrame = keyT;

    // Горячая клавиша привязки декораций к сетке (G)
    bool keyG = (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS);
    if (keyG && !m_keyGPressedLastFrame && !isCtrl && !isAlt) {
        m_decorGridSnap = !m_decorGridSnap;
        updateButtonLayout();
        showToast(m_decorGridSnap ? "Привязка к сетке: ВКЛ [G]" : "Привязка к сетке: ВЫКЛ [G]", glm::vec4(0.4f, 0.9f, 0.5f, 1.0f));
    }
    m_keyGPressedLastFrame = keyG;

    // Горячие клавиши отмены и повтора (Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z)
    int mods = 0;
    if (isCtrl) mods |= GLFW_MOD_CONTROL;
    if (isShift) mods |= GLFW_MOD_SHIFT;

#ifdef __EMSCRIPTEN__
    static const int scancodeZ = 0;
    static const int scancodeY = 0;
#else
    static const int scancodeZ = glfwGetKeyScancode(GLFW_KEY_Z);
    static const int scancodeY = glfwGetKeyScancode(GLFW_KEY_Y);
#endif
    (void)scancodeZ;
    (void)scancodeY;

    bool keyZ = (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS);
#ifdef _WIN32
    if (!keyZ) {
        keyZ = (GetAsyncKeyState('Z') & 0x8000) != 0;
    }
#endif

    bool keyY = (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS);
#ifdef _WIN32
    if (!keyY) {
        keyY = (GetAsyncKeyState('Y') & 0x8000) != 0;
    }
#endif

    if (mods & GLFW_MOD_CONTROL) {
        if (keyZ && !m_keyZPressedLastFrame) {
            if (mods & GLFW_MOD_SHIFT) {
                redo();
            } else {
                undo();
            }
        } else if (keyY && !m_keyYPressedLastFrame) {
            redo();
        }
    }
    m_keyZPressedLastFrame = keyZ;
    m_keyYPressedLastFrame = keyY;

    // Горячая клавиша Камеры (C)
    bool keyC = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
    if (keyC && !m_keyCPressedLastFrame) {
        m_isCameraConfigMode = !m_isCameraConfigMode;
        if (m_isCameraConfigMode) {
            if (!m_cameraSettings.isCustom) {
                syncCameraSettingsFromCurrentView();
            }
            showToast("Режим камеры: сдвиньте/зуммируйте и нажмите [Зафиксировать камеру]", glm::vec4(0.35f, 0.85f, 1.0f, 1.0f));
        } else {
            showToast("Режим камеры выключен", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        }
        updateGridDimensions();
        updateButtonLayout();
    }
    m_keyCPressedLastFrame = keyC;

    // Горячая клавиша предпросмотра боевого HUD (U)
    bool keyU = (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS);
    if (keyU && !m_keyUPressedLastFrame) {
        cycleHudPreviewMode();
    }
    m_keyUPressedLastFrame = keyU;

    // Горячая клавиша поворота направления струи эмиттера или декора (R)
    bool keyR = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
    if (keyR && !m_keyRPressedLastFrame) {
        glm::vec2 mousePosLocal = glm::vec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        glm::vec2 worldPos = screenToWorld(mousePosLocal);
        if (m_toolbarUI.getCategory() == PaletteCategory::Decorations) {
            int targetIdx = m_selectedDecorationIndex;
            if (targetIdx < 0 || targetIdx >= static_cast<int>(m_decorations.size())) {
                targetIdx = findNearestDecorationIndex(worldPos, 28.0f);
            }

            if (targetIdx >= 0 && targetIdx < static_cast<int>(m_decorations.size())) {
                m_decorations[targetIdx].rotation = std::fmod(m_decorations[targetIdx].rotation + 45.0f, 360.0f);
                if (m_decorations[targetIdx].rotation < 0.0f) m_decorations[targetIdx].rotation += 360.0f;
                m_selectedDecorationIndex = targetIdx;
                showToast("Угол декора: " + std::to_string(static_cast<int>(m_decorations[targetIdx].rotation)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                m_isDirty = true;
            } else {
                m_defaultDecorationRotation = std::fmod(m_defaultDecorationRotation + 45.0f, 360.0f);
                if (m_defaultDecorationRotation < 0.0f) m_defaultDecorationRotation += 360.0f;
                showToast("Угол по умолчанию: " + std::to_string(static_cast<int>(m_defaultDecorationRotation)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
            }
        } else {
            int targetIdx = m_selectedEmitterIndex;
            if (targetIdx < 0 || targetIdx >= static_cast<int>(m_emitters.size())) {
                targetIdx = findNearestEmitterIndex(worldPos, 28.0f);
            }

            if (targetIdx >= 0 && targetIdx < static_cast<int>(m_emitters.size())) {
                m_emitters[targetIdx].angleDeg = std::fmod(m_emitters[targetIdx].angleDeg + 45.0f, 360.0f);
                if (m_emitters[targetIdx].angleDeg < 0.0f) m_emitters[targetIdx].angleDeg += 360.0f;
                m_envParticleManager.setEmitters(m_emitters);
                m_selectedEmitterIndex = targetIdx;
                showToast("Угол струи: " + std::to_string(static_cast<int>(m_emitters[targetIdx].angleDeg)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                m_isDirty = true;
            } else {
                m_defaultEmitterAngle = std::fmod(m_defaultEmitterAngle + 45.0f, 360.0f);
                if (m_defaultEmitterAngle < 0.0f) m_defaultEmitterAngle += 360.0f;
                showToast("Угол струи по умолчанию: " + std::to_string(static_cast<int>(m_defaultEmitterAngle)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
            }
        }
    }
    m_keyRPressedLastFrame = keyR;

    // Стрелочки клавиатуры для ювелирного сдвига выделенного декора (1px / 8px со Shift)
    if (!isCtrl && m_toolbarUI.getCategory() == PaletteCategory::Decorations &&
        m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
        float stepPx = isShift ? 8.0f : 1.0f;
        float cellSize = (m_grid && m_grid->getCellSize() > 0.001f) ? m_grid->getCellSize() : 64.0f;
        float stepTile = stepPx / cellSize;

        bool arrowMoved = false;
        int arrowKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        glm::vec2 arrowDeltas[4] = {
            glm::vec2(-stepTile, 0.0f),
            glm::vec2(+stepTile, 0.0f),
            glm::vec2(0.0f, -stepTile),
            glm::vec2(0.0f, +stepTile)
        };

        for (int k = 0; k < 4; ++k) {
            int key = arrowKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.20f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.03f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                m_decorations[m_selectedDecorationIndex].tilePos += arrowDeltas[k];
                if (m_grid) {
                    m_decorations[m_selectedDecorationIndex].worldPos = m_grid->getOffset() + m_decorations[m_selectedDecorationIndex].tilePos * m_grid->getCellSize();
                }
                arrowMoved = true;
            }
        }

        if (arrowMoved) {
            m_isDirty = true;
        }

        // Хоткеи изменения слоя декора: PageUp / ']' (Layer +1), PageDown / '[' (Layer -1)
        bool keyLayerUp = (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS);
        bool keyLayerDown = (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS);
        if (keyLayerUp && !m_keyLayerUpLastFrame) {
            m_decorations[m_selectedDecorationIndex].zIndex = std::clamp(m_decorations[m_selectedDecorationIndex].zIndex + 1, -10, 10);
            m_selectedDecorationIndex = DecorationRenderer::sortByZIndex(m_decorations, m_selectedDecorationIndex);
            m_isDirty = true;
            showToast("Слой декора (zIndex): " + std::to_string(m_decorations[m_selectedDecorationIndex].zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        }
        if (keyLayerDown && !m_keyLayerDownLastFrame) {
            m_decorations[m_selectedDecorationIndex].zIndex = std::clamp(m_decorations[m_selectedDecorationIndex].zIndex - 1, -10, 10);
            m_selectedDecorationIndex = DecorationRenderer::sortByZIndex(m_decorations, m_selectedDecorationIndex);
            m_isDirty = true;
            showToast("Слой декора (zIndex): " + std::to_string(m_decorations[m_selectedDecorationIndex].zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        }

        // Клавиша Delete / Backspace для удаления выделенного декора
        bool keyDel = (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        if (keyDel) {
            std::string dName = m_decorations[m_selectedDecorationIndex].type;
            m_decorations.erase(m_decorations.begin() + m_selectedDecorationIndex);
            m_selectedDecorationIndex = -1;
            m_isDraggingDecoration = false;
            m_isDirty = true;
            showToast("Декор удален: " + dName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
        }
    }

    // Стрелочки клавиатуры для ювелирного сдвига выделенного эмиттера (1px / 8px со Shift)
    if (!isCtrl && m_toolbarUI.getCategory() == PaletteCategory::Particles &&
        m_selectedEmitterIndex >= 0 && m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
        float stepPx = isShift ? 8.0f : 1.0f;
        float stepTile = stepPx / m_grid->getCellSize();

        bool arrowMoved = false;
        int arrowKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        glm::vec2 arrowDeltas[4] = {
            glm::vec2(-stepTile, 0.0f),
            glm::vec2(+stepTile, 0.0f),
            glm::vec2(0.0f, -stepTile),
            glm::vec2(0.0f, +stepTile)
        };

        for (int k = 0; k < 4; ++k) {
            int key = arrowKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.20f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.03f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                m_emitters[m_selectedEmitterIndex].tilePos += arrowDeltas[k];
                arrowMoved = true;
            }
        }

        if (arrowMoved) {
            m_envParticleManager.setEmitters(m_emitters);
            m_isDirty = true;
        }

        // Хоткеи изменения слоя эмиттера: PageUp / ']' (Layer +1), PageDown / '[' (Layer -1)
        bool keyLayerUp = (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS);
        bool keyLayerDown = (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS);
        if (keyLayerUp && !m_keyLayerUpLastFrame) {
            m_emitters[m_selectedEmitterIndex].zIndex = std::clamp(m_emitters[m_selectedEmitterIndex].zIndex + 1, -10, 10);
            m_selectedEmitterIndex = EditorEmitterRenderer::sortByZIndex(m_emitters, m_selectedEmitterIndex);
            m_envParticleManager.setEmitters(m_emitters);
            m_isDirty = true;
            showToast("Слой эмиттера (zIndex): " + std::to_string(m_emitters[m_selectedEmitterIndex].zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        }
        if (keyLayerDown && !m_keyLayerDownLastFrame) {
            m_emitters[m_selectedEmitterIndex].zIndex = std::clamp(m_emitters[m_selectedEmitterIndex].zIndex - 1, -10, 10);
            m_selectedEmitterIndex = EditorEmitterRenderer::sortByZIndex(m_emitters, m_selectedEmitterIndex);
            m_envParticleManager.setEmitters(m_emitters);
            m_isDirty = true;
            showToast("Слой эмиттера (zIndex): " + std::to_string(m_emitters[m_selectedEmitterIndex].zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        }

        // Клавиша Delete / Backspace для удаления выделенного эмиттера
        bool keyDel = (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        if (keyDel) {
            std::string eName = m_emitters[m_selectedEmitterIndex].type;
            m_emitters.erase(m_emitters.begin() + m_selectedEmitterIndex);
            m_envParticleManager.setEmitters(m_emitters);
            m_selectedEmitterIndex = -1;
            m_isDraggingEmitter = false;
            m_isDirty = true;
            showToast("Эмиттер удален: " + eName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
        }
    }

    // Сдвиг всей карты (тайлы, спавны, базы, вагонетки, эмиттеры, декор): Ctrl + Стрелочки (← / → / ↑ / ↓)
    if (isCtrl) {
        int shiftKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        int shiftDx[4] = { -1, 1, 0, 0 };
        int shiftDy[4] = { 0, 0, -1, 1 };

        for (int k = 0; k < 4; ++k) {
            int key = shiftKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.25f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.08f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                shiftMap(shiftDx[k], shiftDy[k]);
                break;
            }
        }
    }

    // Панорамирование камеры:
    // 1) В режиме настройки камеры (m_isCameraConfigMode): зажатие ПКМ или СКМ или Пробел+ЛКМ
    // 2) В обычном режиме: зажатие СКМ или Пробел+ЛКМ
    bool middleDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    bool spaceDown  = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    bool rmbPanCamera = m_isCameraConfigMode && rightDown;
    bool panActive  = middleDown || (spaceDown && leftDown) || rmbPanCamera;

    if (panActive) {
        if (m_isPanning) {
            glm::vec2 mouseDelta = mousePos - m_prevMousePos;
            m_cameraPan += mouseDelta;
            if (m_isCameraConfigMode) {
                syncCameraSettingsFromCurrentView();
            }
        }
        m_isPanning = true;
        m_prevMousePos = mousePos;
        m_suppressPlacementUntilRelease = true;
    } else {
        m_isPanning = false;
        m_prevMousePos = mousePos;
    }

    // В режиме настройки камеры: плавное перемещение стрелочками клавиатуры (← / → / ↑ / ↓)
    if (m_isCameraConfigMode && !isCtrl) {
        int camArrowKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        glm::vec2 camDeltas[4] = {
            glm::vec2(-1.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, -1.0f),
            glm::vec2(0.0f, 1.0f)
        };
        float stepTiles = (isShift ? 2.5f : 0.5f);

        for (int k = 0; k < 4; ++k) {
            int key = camArrowKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.18f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.03f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                m_cameraSettings.targetTile += camDeltas[k] * stepTiles;
                m_cameraSettings.zoom = m_zoom;
                glm::vec2 screenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
                m_cameraPan = screenCenter - getWorldCamCenter() * m_zoom;
            }
        }
    }

    // Прокрутка страниц декораций колёсиком мыши над нижней панелью инструментов
    if (m_toolbarUI.isBottomDockHovered(mousePos.x, mousePos.y) && m_toolbarUI.getCategory() == PaletteCategory::Decorations) {
        float scrollY = InputManager::getScrollY();
        if (std::abs(scrollY) > 0.01f) {
            if (scrollY < 0.0f) {
                m_toolbarUI.nextDecorPage();
            } else {
                m_toolbarUI.prevDecorPage();
            }
            updateButtonLayout();
        }
    }

    // Зум камеры колёсиком мыши (зум в точку под курсором)
    if (!panActive && !m_toolbarUI.isHovered(mousePos.x, mousePos.y)) {
        float scrollY = InputManager::getScrollY();
        if (std::abs(scrollY) > 0.01f) {
            float zoomFactor = (scrollY > 0.0f) ? 1.12f : (1.0f / 1.12f);
            float oldZoom = m_zoom;
            float newZoom = std::clamp(m_zoom * zoomFactor, 0.4f, 2.5f);
            if (newZoom != oldZoom) {
                glm::vec2 worldPosUnderCursor = screenToWorld(mousePos);
                m_zoom = newZoom;
                m_cameraPan = mousePos - worldPosUnderCursor * m_zoom;
                if (m_isCameraConfigMode) {
                    syncCameraSettingsFromCurrentView();
                }
            }
        }
    }

    // Сброс камеры (клавиша F)
    bool keyF = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    if (keyF && !m_keyFPressedLastFrame) {
        m_zoom = 1.0f;
        glm::vec2 screenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
        glm::vec2 defCenter = getDefaultWorldCenter();
        m_cameraPan = screenCenter - defCenter * m_zoom;
        if (m_isCameraConfigMode) {
            syncCameraSettingsFromCurrentView();
        }
        showToast("Камера центрирована (1.0x)", glm::vec4(0.3f, 0.85f, 1.0f, 1.0f));
    }
    m_keyFPressedLastFrame = keyF;

    bool clickedUI = false;

    // 0. Плавающая панель настроек эмиттера партиклов (Inspector)
    if (m_selectedEmitterIndex >= 0 && m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
        glm::vec2 emWorld = m_grid ? (m_grid->getOffset() + m_emitters[m_selectedEmitterIndex].tilePos * m_grid->getCellSize()) : glm::vec2(0.0f);
        glm::vec2 emScreen = worldToScreen(emWorld);

        bool handled = m_emitterInspector.handleInput(
            mousePos, leftDown, m_isLeftMouseDown,
            m_emitters[m_selectedEmitterIndex], m_selectedEmitterIndex, emScreen,
            static_cast<float>(m_width), static_cast<float>(m_height),
            getTopBarHeight(), getBottomDockHeight(),
            /* onChanged */ [this]() {
                m_selectedEmitterIndex = EditorEmitterRenderer::sortByZIndex(m_emitters, m_selectedEmitterIndex);
                m_envParticleManager.setEmitters(m_emitters);
                m_envParticleManager.triggerBurstAt(m_selectedEmitterIndex);
                m_isDirty = true;
            },
            /* onDelete */ [this]() {
                m_emitters.erase(m_emitters.begin() + m_selectedEmitterIndex);
                m_selectedEmitterIndex = -1;
                m_isDraggingEmitter = false;
                m_emitterInspector.resetPosition();
                m_envParticleManager.setEmitters(m_emitters);
                m_isDirty = true;
                showToast("Эмиттер удален", glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
            },
            /* onClose */ [this]() {
                m_selectedEmitterIndex = -1;
                m_isDraggingEmitter = false;
            },
            /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                showToast(msg, color);
            }
        );

        if (handled) {
            clickedUI = true;
            m_isDraggingEmitter = false;
            m_isLeftMouseDown = leftDown;
            m_isRightMouseDown = rightDown;
            return;
        }
    }

    // 0.1. Плавающая панель настроек декора (Decoration Inspector)
    if (m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
        const auto& dec = m_decorations[m_selectedDecorationIndex];
        glm::vec2 decActualWorld = m_grid ? (m_grid->getOffset() + dec.tilePos * m_grid->getCellSize()) : dec.worldPos;
        glm::vec2 decScreen = worldToScreen(decActualWorld);

        bool handled = m_decorInspector.handleInput(
            mousePos, leftDown, m_isLeftMouseDown,
            m_decorations[m_selectedDecorationIndex], m_selectedDecorationIndex, decScreen,
            static_cast<float>(m_width), static_cast<float>(m_height),
            getTopBarHeight(), getBottomDockHeight(),
            /* onChanged */ [this]() {
                m_selectedDecorationIndex = DecorationRenderer::sortByZIndex(m_decorations, m_selectedDecorationIndex);
                m_isDirty = true;
            },
            /* onDelete */ [this]() {
                std::string dName = m_decorations[m_selectedDecorationIndex].type;
                m_decorations.erase(m_decorations.begin() + m_selectedDecorationIndex);
                m_selectedDecorationIndex = -1;
                m_isDraggingDecoration = false;
                m_isDirty = true;
                showToast("Декор удален: " + dName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
            },
            /* onClose */ [this]() {
                m_selectedDecorationIndex = -1;
                m_isDraggingDecoration = false;
            },
            /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                showToast(msg, color);
            }
        );

        if (handled) {
            clickedUI = true;
            m_isDraggingDecoration = false;
            m_isLeftMouseDown = leftDown;
            m_isRightMouseDown = rightDown;
            return;
        }
    }

    // 0.2. Плавающая панель инспектора спавнера и базы (Entity Inspector)
    if (m_selectedEntityIndex >= 0) {
        if (m_selectedEntityType == EditorEntityType::Spawner && m_selectedEntityIndex < static_cast<int>(m_spawners.size())) {
            glm::vec2 entityWorld = m_grid ? (m_grid->gridToPixel(m_spawners[m_selectedEntityIndex].pos.x, m_spawners[m_selectedEntityIndex].pos.y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entityScreen = worldToScreen(entityWorld);

            bool handled = m_entityInspector.handleInput(
                mousePos, leftDown, m_isLeftMouseDown,
                m_spawners[m_selectedEntityIndex], m_selectedEntityIndex, entityScreen,
                static_cast<float>(m_width), static_cast<float>(m_height),
                getTopBarHeight(), getBottomDockHeight(),
                /* onChanged */ [this]() {
                    m_isDirty = true;
                    recalculatePaths();
                },
                /* onDelete */ [this]() {
                    m_spawners.erase(m_spawners.begin() + m_selectedEntityIndex);
                    m_selectedEntityIndex = -1;
                    m_entityInspector.resetPosition();
                    m_isDirty = true;
                    recalculatePaths();
                    showToast("Спавнер удален", glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
                },
                /* onClose */ [this]() {
                    m_selectedEntityIndex = -1;
                },
                /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                    showToast(msg, color);
                }
            );

            if (handled) {
                clickedUI = true;
                m_isLeftMouseDown = leftDown;
                m_isRightMouseDown = rightDown;
                return;
            }
        } else if (m_selectedEntityType == EditorEntityType::Base && m_selectedEntityIndex < static_cast<int>(m_bases.size())) {
            glm::vec2 entityWorld = m_grid ? (m_grid->gridToPixel(m_bases[m_selectedEntityIndex].x, m_bases[m_selectedEntityIndex].y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entityScreen = worldToScreen(entityWorld);

            bool handled = m_entityInspector.handleInput(
                mousePos, leftDown, m_isLeftMouseDown,
                m_bases[m_selectedEntityIndex], m_selectedEntityIndex, entityScreen,
                static_cast<float>(m_width), static_cast<float>(m_height),
                getTopBarHeight(), getBottomDockHeight(),
                /* onChanged */ [this]() {
                    m_isDirty = true;
                    recalculatePaths();
                },
                /* onDelete */ [this]() {
                    m_bases.erase(m_bases.begin() + m_selectedEntityIndex);
                    m_selectedEntityIndex = -1;
                    m_entityInspector.resetPosition();
                    m_isDirty = true;
                    recalculatePaths();
                    showToast("База удалена", glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
                },
                /* onClose */ [this]() {
                    m_selectedEntityIndex = -1;
                },
                /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                    showToast(msg, color);
                }
            );

            if (handled) {
                clickedUI = true;
                m_isLeftMouseDown = leftDown;
                m_isRightMouseDown = rightDown;
                return;
            }
        } else {
            m_selectedEntityIndex = -1;
        }
    }

    // 1. Проверка клика по кнопкам тулбара и палитры через EditorToolbarUI
    ::EditorAction action = m_toolbarUI.handleInput(mousePos.x, mousePos.y, leftDown, m_isLeftMouseDown, isShift, buildEditorContext(), isAlt);
    if (action.type != EditorActionType::None) {
        clickedUI = true;
        m_suppressPlacementUntilRelease = true;
        switch (action.type) {
            case EditorActionType::SetBrush:
                m_currentBrush = action.brush;
                if (m_currentBrush == EditorBrush::Base && m_selectedId < 0) {
                    m_selectedId = 0;
                }
                if (m_currentBrush == EditorBrush::Spawner || m_currentBrush == EditorBrush::Base ||
                    m_currentBrush == EditorBrush::Rail || m_currentBrush == EditorBrush::RailStart ||
                    m_currentBrush == EditorBrush::RailEnd) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                        m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                        updateButtonLayout();
                    }
                } else if (m_currentBrush == EditorBrush::EmitterSteamJet ||
                           m_currentBrush == EditorBrush::EmitterWaterDrip ||
                           m_currentBrush == EditorBrush::EmitterSparks ||
                           m_currentBrush == EditorBrush::EmitterSmoke ||
                           m_currentBrush == EditorBrush::EmitterFog) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::Particles) {
                        m_toolbarUI.setCategory(PaletteCategory::Particles);
                        updateButtonLayout();
                    }
                } else if (m_currentBrush >= EditorBrush::DecorBush && m_currentBrush <= EditorBrush::DecorOreDebris) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
                        m_toolbarUI.setCategory(PaletteCategory::Decorations);
                        updateButtonLayout();
                    }
                } else if (m_currentBrush != EditorBrush::Eraser) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::Tiles) {
                        m_toolbarUI.setCategory(PaletteCategory::Tiles);
                        updateButtonLayout();
                    }
                }
                if (m_currentBrush == EditorBrush::NoBuild || (m_currentBrush >= EditorBrush::Ground && m_currentBrush <= EditorBrush::RailEnd)) {
                    m_selectedDecorationIndex = -1;
                    m_selectedEntityIndex = -1;
                    m_selectedEmitterIndex = -1;
                    m_isDraggingDecoration = false;
                    m_gizmoController.endResize();
                }
                break;
            case EditorActionType::SwitchPaletteCategory:
                updateButtonLayout();
                if (m_toolbarUI.getCategory() == PaletteCategory::Tiles) {
                    showToast("Палитра: Тайлы");
                } else if (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects) {
                    showToast("Палитра: Спец-объекты");
                } else if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
                    showToast("Палитра: Партиклы");
                } else {
                    showToast("Палитра: Декор");
                }
                break;
            case EditorActionType::SaveMap:
                saveMap();
                break;
            case EditorActionType::TestMap:
                testMap();
                return;
            case EditorActionType::ClearMap:
                clearMap();
                break;
            case EditorActionType::Exit:
                openExitModal();
                m_isLeftMouseDown = leftDown;
                return;
            case EditorActionType::CycleSelectedId:
                cycleSelectedId(action.intParam);
                break;
            case EditorActionType::CyclePreset:
                cycleMapSizePreset();
                break;
            case EditorActionType::ResizeMap:
                resizeMap(m_gridWidth + action.intParam, m_gridHeight + action.intParam2, action.isAltDown);
                break;
            case EditorActionType::OpenRenameModal:
                m_mapsBrowserModal.openRename(m_currentLevelFileName, m_currentLevelDisplayName);
                m_suppressClickUntilRelease = true;
                break;
            case EditorActionType::NewMap: {
                std::string newF = LevelManager::createNewLevel("custom_map");
                loadLevelByName(newF);
                m_suppressClickUntilRelease = true;
                m_isLeftMouseDown = true;
                m_statusMessage = "Created map: " + m_currentLevelDisplayName;
                m_statusColor = glm::vec3(0.2f, 1.0f, 0.4f);
                m_statusTimer = 3.5f;
                break;
            }
            case EditorActionType::OpenMapsModal:
                m_mapsBrowserModal.open(m_currentLevelFileName, false);
                m_suppressClickUntilRelease = true;
                break;
            case EditorActionType::ToggleCampaign:
                m_isCampaign = !m_isCampaign;
                LevelManager::setLevelCampaign(m_currentLevelFileName, m_isCampaign);
                m_statusMessage = m_isCampaign ? "Map type: CAMPAIGN" : "Map type: TEST";
                m_statusColor = m_isCampaign ? glm::vec3(0.3f, 1.0f, 0.4f) : glm::vec3(1.0f, 0.7f, 0.2f);
                m_statusTimer = 2.5f;
                updateButtonLayout();
                break;
            case EditorActionType::ToggleWavesModal:
                m_waveModal.toggle(m_waves);
                break;
            case EditorActionType::ToggleMinecartModal:
                m_minecartModal.toggle();
                break;
            case EditorActionType::ToggleBackground:
                if (m_background == "default") {
                    m_background = "pipes_canal";
                    showToast("Фон: Дюкеры Канала");
                } else if (m_background == "pipes_canal") {
                    m_background = "mine_entrance";
                    showToast("Фон: Вход в шахту");
                } else {
                    m_background = "default";
                    showToast("Фон: Стандартный");
                }
                if (m_grid) {
                    m_grid->setHasSidePockets(m_hasSidePockets || m_background == "mine_entrance");
                    recalculatePaths();
                }
                m_isDirty = true;
                updateButtonLayout();
                break;
            case EditorActionType::ToggleCameraMode:
                m_isCameraConfigMode = !m_isCameraConfigMode;
                if (m_isCameraConfigMode) {
                    if (!m_cameraSettings.isCustom) {
                        syncCameraSettingsFromCurrentView();
                    }
                    showToast("Режим камеры: сдвиньте/зуммируйте и нажмите [Зафиксировать камеру]", glm::vec4(0.35f, 0.85f, 1.0f, 1.0f));
                } else {
                    showToast("Режим камеры выключен", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
                }
                updateGridDimensions();
                updateButtonLayout();
                break;
            case EditorActionType::LockCamera: {
                syncCameraSettingsFromCurrentView();
                showToast("Стартовая камера сохранена", glm::vec4(0.35f, 1.0f, 0.55f, 1.0f));
                updateButtonLayout();
                break;
            }
            case EditorActionType::ResetCamera:
                m_cameraSettings.isCustom = false;
                m_isCameraConfigMode = false;
                m_isDirty = true;
                showToast("Камера сброшена на Авто", glm::vec4(1.0f, 0.8f, 0.35f, 1.0f));
                updateGridDimensions();
                updateButtonLayout();
                break;
            case EditorActionType::ToggleHelpModal:
                m_helpModal.toggle();
                m_isLeftMouseDown = true;
                m_suppressPlacementUntilRelease = true;
                return;
            case EditorActionType::ToggleHudPreview:
                cycleHudPreviewMode();
                break;
            case EditorActionType::OpenLevelSettingsModal:
                m_levelSettingsModal.open(m_startingMoney, m_startingHealth, m_allowedTowers, m_towerMaxTiers, m_enemyBountyRules, static_cast<int>(m_waves.size()), m_hasSidePockets, m_enemyHpRules);
                m_suppressClickUntilRelease = true;
                break;
            case EditorActionType::ModifyStartingMoney:
            case EditorActionType::ModifyStartingHealth:
                break;
            case EditorActionType::ToggleDecorGridSnap:
                m_decorGridSnap = !m_decorGridSnap;
                updateButtonLayout();
                showToast(m_decorGridSnap ? "Привязка к сетке: ВКЛ [G]" : "Привязка к сетке: ВЫКЛ [G]", glm::vec4(0.4f, 0.9f, 0.5f, 1.0f));
                break;
            default:
                break;
        }
    }

    // 1.9. Обновление курсора мыши при наведении на интерактивные ручки трансформации (Gizmo)
    const DecorationConfig* selDecor = (m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) ? &m_decorations[m_selectedDecorationIndex] : nullptr;
    float gizmoCellSize = m_grid ? m_grid->getCellSize() : 64.0f;
    glm::vec2 worldPosForHover = screenToWorld(mousePos);
    m_gizmoController.updateHoverCursor(
        window,
        selDecor,
        worldPosForHover,
        gizmoCellSize,
        m_toolbarUI.getCategory() == PaletteCategory::Decorations,
        m_toolbarUI.isHovered(mousePos.x, mousePos.y)
    );

    // 2. Если кликаем по рабочей области сетки
    if (!clickedUI && !panActive && !m_isCameraConfigMode && !m_toolbarUI.isHovered(mousePos.x, mousePos.y) && m_grid) {
        if (!m_isLeftMouseDown && !m_isRightMouseDown && (leftDown || rightDown)) {
            m_currentStrokeAction.changes.clear();
        }

        glm::vec2 worldMouse = screenToWorld(mousePos);
        glm::ivec2 gridPos = m_grid->pixelToGrid(worldMouse);
        bool inBounds = (gridPos.x >= 0 && gridPos.x < m_gridWidth && gridPos.y >= 0 && gridPos.y < m_gridHeight);

        bool keyPPressed = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
        bool keyPTriggered = keyPPressed && !m_keyPPressedLastFrame;
        m_keyPPressedLastFrame = keyPPressed;

        // 2.1. ПИПЕТКА: Alt + клик ЛКМ по ячейке или клавиша P
        if ((isAlt && leftDown && !m_isLeftMouseDown) || keyPTriggered) {
            executePipette(gridPos.x, gridPos.y, worldMouse);
        }
        // 2.2. ПРЯМОУГОЛЬНАЯ ЗАЛИВКА: Shift + зажатый ЛКМ (только для тайлов и спец-объектов)
        else if (isShift && !isAlt && m_toolbarUI.getCategory() != PaletteCategory::Particles && m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
            if (leftDown) {
                if (!m_isLeftMouseDown && inBounds) {
                    m_isBoxFilling = true;
                    m_boxStartCell = gridPos;
                    m_boxCurrentCell = gridPos;
                } else if (m_isBoxFilling && inBounds) {
                    m_boxCurrentCell = gridPos;
                }
            }
        }
        // 2.3. СВОБОДНАЯ РАБОТА С ПАРТИКЛАМИ НА ВКЛАДКЕ [3] ПАРТИКЛЫ (ВНЕ СЕТКИ, DRAG, СЛОЁНЫЙ ЛАСТИК)
        else if (m_toolbarUI.getCategory() == PaletteCategory::Particles && !m_suppressPlacementUntilRelease) {
            float clickRadius = std::max(20.0f, m_grid->getCellSize() * 0.45f);

            // А. Перетаскивание (Drag) выделенного эмиттера зажатой мышью
            if (leftDown && m_isDraggingEmitter && m_selectedEmitterIndex >= 0 &&
                m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
                glm::vec2 targetWorld = worldMouse + m_emitterDragOffset;
                m_emitters[m_selectedEmitterIndex].tilePos = (targetWorld - m_grid->getOffset()) / m_grid->getCellSize();
                m_envParticleManager.setEmitters(m_emitters);
                m_isDirty = true;
            }
            // Б. Одиночный клик ЛКМ
            else if (leftDown && !m_isLeftMouseDown && m_currentBrush != EditorBrush::NoBuild) {
                // Слоёный ластик партиклов: удаляет ТОЛЬКО эмиттер под курсором, не трогая тайлы сетки!
                if (m_currentBrush == EditorBrush::Eraser) {
                    eraseEmitterNear(worldMouse, clickRadius);
                } else {
                    // Проверяем, кликнули ли по существующему эмиттеру для выделения / драга
                    int nearestIdx = findNearestEmitterIndex(worldMouse, clickRadius);
                    if (nearestIdx >= 0) {
                        m_selectedEmitterIndex = nearestIdx;
                        m_envParticleManager.triggerBurstAt(nearestIdx);
                        m_isDraggingEmitter = true;
                        glm::vec2 emWorld = m_grid->getOffset() + m_emitters[nearestIdx].tilePos * m_grid->getCellSize();
                        m_emitterDragOffset = emWorld - worldMouse;
                        showToast("Выделен эмиттер #" + std::to_string(nearestIdx + 1) + " [" + m_emitters[nearestIdx].type + "]", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                    } else if (m_currentBrush != EditorBrush::None) {
                        // Свободное размещение нового эмиттера ровно в точке курсора worldMouse
                        std::string eType = "steam_jet";
                        std::string eName = "Свищ пара";
                        float pMin = 5.0f, pMax = 12.0f, bDur = 2.2f, spd = 180.0f, pScl = 1.0f;

                        if (m_currentBrush == EditorBrush::EmitterWaterDrip) {
                            eType = "water_drip"; eName = "Капель"; pMin = 2.0f; pMax = 5.0f; bDur = 1.2f; spd = 120.0f; pScl = 1.0f;
                        } else if (m_currentBrush == EditorBrush::EmitterSparks) {
                            eType = "sparks"; eName = "Искры"; pMin = 4.0f; pMax = 10.0f; bDur = 1.5f; spd = 220.0f; pScl = 1.0f;
                        } else if (m_currentBrush == EditorBrush::EmitterSmoke) {
                            eType = "smoke"; eName = "Дым"; pMin = 3.0f; pMax = 8.0f; bDur = 2.8f; spd = 140.0f; pScl = 1.2f;
                        } else if (m_currentBrush == EditorBrush::EmitterFog) {
                            eType = "fog"; eName = "Туман"; pMin = 0.5f; pMax = 1.0f; bDur = 10.0f; spd = 35.0f; pScl = 1.6f;
                        }

                        ParticleEmitterConfig cfg;
                        cfg.id = static_cast<int>(m_emitters.size()) + 1;
                        cfg.type = eType;
                        if (eType == "fog") cfg.loopContinuous = true;
                        cfg.tilePos = (worldMouse - m_grid->getOffset()) / m_grid->getCellSize();
                        cfg.angleDeg = m_defaultEmitterAngle;
                        cfg.speed = spd;
                        cfg.particleScale = pScl;
                        cfg.periodMin = pMin;
                        cfg.periodMax = pMax;
                        cfg.burstDuration = bDur;

                        m_emitters.push_back(cfg);
                        m_selectedEmitterIndex = EditorEmitterRenderer::sortByZIndex(m_emitters, static_cast<int>(m_emitters.size()) - 1);
                        m_envParticleManager.setEmitters(m_emitters);
                        m_envParticleManager.triggerBurstAt(m_selectedEmitterIndex);
                        m_isDraggingEmitter = false;
                        m_emitterDragOffset = glm::vec2(0.0f);
                        m_isDirty = true;
                        showToast("Установлен эмиттер: " + eName, glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                    } else {
                        m_selectedEmitterIndex = -1;
                    }
                }
            }
            // В. Клик ПКМ на вкладке партиклов: поворот струи на 45° или слоёный ластик
            else if (rightDown && !m_isRightMouseDown) {
                int nearestIdx = findNearestEmitterIndex(worldMouse, clickRadius);
                if (nearestIdx >= 0) {
                    if (m_currentBrush == EditorBrush::Eraser) {
                        eraseEmitterNear(worldMouse, clickRadius);
                    } else {
                        m_emitters[nearestIdx].angleDeg = std::fmod(m_emitters[nearestIdx].angleDeg + 45.0f, 360.0f);
                        if (m_emitters[nearestIdx].angleDeg < 0.0f) m_emitters[nearestIdx].angleDeg += 360.0f;
                        m_envParticleManager.setEmitters(m_emitters);
                        m_selectedEmitterIndex = nearestIdx;
                        m_envParticleManager.triggerBurstAt(nearestIdx);
                        showToast("Угол струи: " + std::to_string(static_cast<int>(m_emitters[nearestIdx].angleDeg)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                        m_isDirty = true;
                    }
                } else {
                    m_selectedEmitterIndex = -1;
                    if (m_currentBrush != EditorBrush::None) {
                        m_currentBrush = EditorBrush::None;
                        showToast("Кисть сброшена (нейтральный курсор)");
                    }
                }
            }
        }
        // 2.4. СВОБОДНАЯ РАБОТА С ДЕКОРОМ НА ВКЛАДКЕ [4] ДЕКОР (ВНЕ СЕТКИ, DRAG & DROP, СЛОЁНЫЙ ЛАСТИК)
        else if (m_toolbarUI.getCategory() == PaletteCategory::Decorations && !m_suppressPlacementUntilRelease) {
            float clickRadius = std::max(22.0f, m_grid->getCellSize() * 0.45f);

            auto applyDecorSnap = [&](glm::vec2 targetWorld, bool isGrassField) -> glm::vec2 {
                if (!m_grid || m_grid->getCellSize() <= 0.001f) return targetWorld;
                bool shouldSnap = m_decorGridSnap || isCtrl || isShift || (isGrassField && !isAlt);
                if (!shouldSnap) return targetWorld;
                glm::vec2 rawTile = (targetWorld - m_grid->getOffset()) / m_grid->getCellSize();
                glm::vec2 snappedTile = isShift ? (glm::round(rawTile * 2.0f) * 0.5f) : (glm::floor(rawTile) + glm::vec2(0.5f));
                return m_grid->getOffset() + snappedTile * m_grid->getCellSize();
            };

            // А. Растягивание интерактивными ручками (Gizmo Handles) выделенного декора
            if (leftDown && m_gizmoController.isResizing() && m_selectedDecorationIndex >= 0 &&
                m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
                float cellSize = m_grid ? m_grid->getCellSize() : 64.0f;
                if (m_gizmoController.updateResize(m_decorations[m_selectedDecorationIndex], worldMouse, cellSize)) {
                    m_isDirty = true;
                }
            }
            // Б. Перетаскивание (Drag & Drop) выделенного декора зажатой мышью
            else if (leftDown && m_isDraggingDecoration && m_selectedDecorationIndex >= 0 &&
                m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
                glm::vec2 targetWorld = worldMouse + m_decorationDragOffset;
                bool isGrass = (m_decorations[m_selectedDecorationIndex].type == "grass_field");
                targetWorld = applyDecorSnap(targetWorld, isGrass);
                if (m_grid && m_grid->getCellSize() > 0.001f) {
                    m_decorations[m_selectedDecorationIndex].tilePos = (targetWorld - m_grid->getOffset()) / m_grid->getCellSize();
                    m_decorations[m_selectedDecorationIndex].worldPos = targetWorld;
                } else {
                    m_decorations[m_selectedDecorationIndex].worldPos = targetWorld;
                }
                m_isDirty = true;
            }
            // В. Одиночный клик ЛКМ
            else if (leftDown && !m_isLeftMouseDown) {
                // Слоёный ластик декора: удаляет ТОЛЬКО декор под курсором, не трогая тайлы сетки!
                if (m_currentBrush == EditorBrush::Eraser) {
                    eraseDecorationNear(worldMouse, clickRadius);
                } else {
                    // Сначала проверяем клик по интерактивным ручкам (Gizmo Handles) уже выделенного декора
                    bool hitGizmo = false;
                    if (m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
                        float cellSize = m_grid ? m_grid->getCellSize() : 64.0f;
                        hitGizmo = m_gizmoController.tryStartResize(m_decorations[m_selectedDecorationIndex], worldMouse, cellSize);
                    }

                    if (!hitGizmo) {
                        // Проверяем, кликнули ли по существующему декору для выделения / драга
                        int nearestIdx = findNearestDecorationIndex(worldMouse, clickRadius);
                        if (nearestIdx >= 0) {
                            m_selectedDecorationIndex = nearestIdx;
                            m_isDraggingDecoration = true;
                            glm::vec2 decActualWorld = m_grid ? (m_grid->getOffset() + m_decorations[nearestIdx].tilePos * m_grid->getCellSize()) : m_decorations[nearestIdx].worldPos;
                            m_decorationDragOffset = decActualWorld - worldMouse;
                            showToast("Выделен декор #" + std::to_string(m_decorations[nearestIdx].id) + " [" + m_decorations[nearestIdx].type + "]", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                        } else if (m_currentBrush != EditorBrush::None) {
                            // Размещение нового декора (с привязкой к сетке если активна)
                            std::string dType = "bush";
                            std::string dName = "Куст";
                            bool isDestructible = true;

                            if (m_currentBrush == EditorBrush::DecorGrass) {
                                dType = "grass_tuft"; dName = "Пучок травы";
                            } else if (m_currentBrush == EditorBrush::DecorGrassField) {
                                dType = "grass_field"; dName = "Поле травы";
                            } else if (m_currentBrush == EditorBrush::DecorFlower) {
                                dType = "flower"; dName = "Цветы";
                            } else if (m_currentBrush == EditorBrush::DecorStone) {
                                dType = "stone"; dName = "Камень";
                            } else if (m_currentBrush == EditorBrush::DecorHelmet) {
                                dType = "helmet"; dName = "Каска";
                            } else if (m_currentBrush == EditorBrush::DecorPickaxe) {
                                dType = "pickaxe"; dName = "Кирка";
                            } else if (m_currentBrush == EditorBrush::DecorPuddle) {
                                dType = "puddle"; dName = "Лужа";
                            } else if (m_currentBrush == EditorBrush::DecorCrack) {
                                dType = "crack"; dName = "Трещина";
                            } else if (m_currentBrush == EditorBrush::DecorFog) {
                                dType = "fog"; dName = "Туман"; isDestructible = false;
                            } else if (m_currentBrush == EditorBrush::DecorWoodenCrate) {
                                dType = "wooden_crate"; dName = "Деревянный ящик";
                            } else if (m_currentBrush == EditorBrush::DecorBrokenCart) {
                                dType = "broken_mercury_cart"; dName = "Разбитая вагонетка";
                            } else if (m_currentBrush == EditorBrush::DecorScrapDebris) {
                                dType = "scrap_debris"; dName = "Куча мусора";
                            } else if (m_currentBrush == EditorBrush::DecorDeadTree) {
                                dType = "dead_tree"; dName = "Сухое дерево";
                            } else if (m_currentBrush == EditorBrush::DecorDryGrass) {
                                dType = "dry_grass"; dName = "Сухая трава";
                            } else if (m_currentBrush == EditorBrush::DecorOreDebris) {
                                dType = "ore_debris"; dName = "Обломки руды";
                            }

                            DecorationConfig cfg;
                            cfg.id = static_cast<int>(m_decorations.size()) + 1;
                            cfg.type = dType;
                            cfg.scale = 1.0f;
                            cfg.scaleX = 1.0f;
                            cfg.scaleY = 1.0f;
                            cfg.rotation = m_defaultDecorationRotation;
                            cfg.destructible = isDestructible;
                            if (dType == "fog") {
                                cfg.scale = 1.4f;
                                cfg.opacity = 0.45f;
                                cfg.destructible = false;
                            }
                            bool isGrassField = (dType == "grass_field");
                            glm::vec2 placedWorld = applyDecorSnap(worldMouse, isGrassField);
                            if (m_grid && m_grid->getCellSize() > 0.001f) {
                                cfg.tilePos = (placedWorld - m_grid->getOffset()) / m_grid->getCellSize();
                                cfg.worldPos = placedWorld;
                            } else {
                                cfg.tilePos = glm::vec2(0.0f);
                                cfg.worldPos = placedWorld;
                            }
                            if (dType == "puddle" || dType == "crack") {
                                cfg.zIndex = -1;
                            } else {
                                cfg.zIndex = 0;
                            }

                            m_decorations.push_back(cfg);
                            m_selectedDecorationIndex = DecorationRenderer::sortByZIndex(m_decorations, static_cast<int>(m_decorations.size()) - 1);
                            m_isDraggingDecoration = false;
                            m_decorationDragOffset = glm::vec2(0.0f);
                            m_isDirty = true;
                            showToast("Установлен декор: " + dName, glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                        } else {
                            // Клик в пустоту нейтральным курсором снимает выделение
                            m_selectedDecorationIndex = -1;
                        }
                    }
                }
            }
            // В. Клик ПКМ на вкладке декора: поворот декора на 45° или слоёный ластик
            else if (rightDown && !m_isRightMouseDown) {
                int nearestIdx = findNearestDecorationIndex(worldMouse, clickRadius);
                if (nearestIdx >= 0) {
                    if (m_currentBrush == EditorBrush::Eraser) {
                        eraseDecorationNear(worldMouse, clickRadius);
                    } else {
                        m_decorations[nearestIdx].rotation = std::fmod(m_decorations[nearestIdx].rotation + 45.0f, 360.0f);
                        if (m_decorations[nearestIdx].rotation < 0.0f) m_decorations[nearestIdx].rotation += 360.0f;
                        m_selectedDecorationIndex = nearestIdx;
                        showToast("Угол декора: " + std::to_string(static_cast<int>(m_decorations[nearestIdx].rotation)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                        m_isDirty = true;
                    }
                } else {
                    // Клик ПКМ в пустоту: сброс выделения и кисти в нейтральный курсор
                    m_selectedDecorationIndex = -1;
                    if (m_currentBrush != EditorBrush::None) {
                        m_currentBrush = EditorBrush::None;
                        showToast("Кисть сброшена (нейтральный курсор)");
                    }
                }
            }
        }
        // 2.5. СТАНДАРТНАЯ РАБОТА С СЕТКОЙ (ТАЙЛЫ И СПЕЦ-ОБЪЕКТЫ)
        else if (!m_suppressPlacementUntilRelease) {
            m_isDraggingDecoration = false;
            m_gizmoController.endResize();
            bool canInteractTile = inBounds || m_hasSidePockets || (m_grid && m_grid->hasSidePockets());
            if (canInteractTile) {
                // Если мы в категории спец-объектов или нейтральном курсоре - проверяем выбор существующего спавнера/базы
                if (leftDown && !m_isLeftMouseDown) {
                    bool isSpecialCategory = (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects);
                    int clickedSpawner = -1;
                    for (int i = 0; i < static_cast<int>(m_spawners.size()); ++i) {
                        if (m_spawners[i].pos.x == gridPos.x && m_spawners[i].pos.y == gridPos.y) {
                            clickedSpawner = i;
                            break;
                        }
                    }
                    int clickedBase = -1;
                    for (int i = 0; i < static_cast<int>(m_bases.size()); ++i) {
                        if (m_bases[i].x == gridPos.x && m_bases[i].y == gridPos.y) {
                            clickedBase = i;
                            break;
                        }
                    }

                    if (clickedSpawner >= 0 && (isSpecialCategory || m_currentBrush == EditorBrush::None || m_currentBrush == EditorBrush::Spawner)) {
                        m_selectedEntityType = EditorEntityType::Spawner;
                        m_selectedEntityIndex = clickedSpawner;
                        m_selectedDecorationIndex = -1;
                        m_selectedEmitterIndex = -1;
                        showToast("Выбран спавнер S" + (m_spawners[clickedSpawner].targetBaseIndex == -1 ? ":Auto" : ">#" + std::to_string(m_spawners[clickedSpawner].targetBaseIndex)), glm::vec4(0.95f, 0.85f, 0.2f, 1.0f));
                        m_isLeftMouseDown = leftDown;
                        return;
                    } else if (clickedBase >= 0 && (isSpecialCategory || m_currentBrush == EditorBrush::None || m_currentBrush == EditorBrush::Base)) {
                        m_selectedEntityType = EditorEntityType::Base;
                        m_selectedEntityIndex = clickedBase;
                        m_selectedDecorationIndex = -1;
                        m_selectedEmitterIndex = -1;
                        showToast("Выбрана база B#" + std::to_string(m_bases[clickedBase].id), glm::vec4(0.3f, 0.8f, 1.0f, 1.0f));
                        m_isLeftMouseDown = leftDown;
                        return;
                    } else if (m_currentBrush == EditorBrush::None) {
                        m_selectedEntityIndex = -1;
                    }
                }

                bool isSinglePointBrush = (m_currentBrush == EditorBrush::Spawner || m_currentBrush == EditorBrush::Base ||
                                           m_currentBrush == EditorBrush::RailStart || m_currentBrush == EditorBrush::RailEnd);
                bool isNoBuildHere = false;
                if (inBounds) {
                    if (gridPos.y < (int)m_noBuildMask.size() && gridPos.x < (int)m_noBuildMask[gridPos.y].size()) {
                        isNoBuildHere = m_noBuildMask[gridPos.y][gridPos.x];
                    }
                } else if (m_grid) {
                    isNoBuildHere = m_grid->isNoBuild(gridPos.x, gridPos.y);
                }

                if (m_currentBrush == EditorBrush::NoBuild) {
                    if (leftDown) {
                        if (!m_isLeftMouseDown && isNoBuildHere) {
                            // Повторный клик ЛКМ по заблокированной клетке — снимает запрет
                            removeNoBuildAt(gridPos.x, gridPos.y);
                        } else {
                            applyBrush(gridPos.x, gridPos.y, EditorBrush::NoBuild);
                        }
                    } else if (rightDown) {
                        removeNoBuildAt(gridPos.x, gridPos.y);
                    }
                } else if (m_currentBrush == EditorBrush::Eraser && isShift) {
                    if (leftDown || rightDown) {
                        removeNoBuildAt(gridPos.x, gridPos.y);
                    }
                } else if (leftDown) {
                    if (m_currentBrush != EditorBrush::None && (!isSinglePointBrush || !m_isLeftMouseDown)) {
                        applyBrush(gridPos.x, gridPos.y, m_currentBrush);
                    }
                } else if (rightDown) {
                    bool isCellEmptyOrGround = false;
                    if (inBounds) {
                        isCellEmptyOrGround = (m_rawLayout[gridPos.y][gridPos.x] == 0);
                    } else if (m_grid) {
                        auto ct = m_grid->getCellType(gridPos.x, gridPos.y);
                        isCellEmptyOrGround = (ct == CellType::Empty || ct == CellType::Ground);
                    }
                    if (isCellEmptyOrGround && m_currentBrush != EditorBrush::None) {
                        // Клик ПКМ по пустой земле с активной кистью: сброс кисти в нейтральный курсор
                        if (!m_isRightMouseDown) {
                            m_currentBrush = EditorBrush::None;
                            showToast("Кисть сброшена (нейтральный курсор)");
                        }
                    } else {
                        eraseCell(gridPos.x, gridPos.y);
                    }
                }
            } else if (rightDown && !m_isRightMouseDown && m_currentBrush != EditorBrush::None) {
                // Клик ПКМ вне сетки: сброс активной кисти в нейтральный курсор
                m_currentBrush = EditorBrush::None;
                showToast("Кисть сброшена (нейтральный курсор)");
            }
        }
    }

    // Завершение прямоугольной заливки при отпускании ЛКМ
    if (!leftDown && m_isLeftMouseDown && m_isBoxFilling) {
        if (m_boxStartCell.x >= 0 && m_boxCurrentCell.x >= 0) {
            int minX = std::max(0, std::min(m_boxStartCell.x, m_boxCurrentCell.x));
            int maxX = std::min(m_gridWidth - 1, std::max(m_boxStartCell.x, m_boxCurrentCell.x));
            int minY = std::max(0, std::min(m_boxStartCell.y, m_boxCurrentCell.y));
            int maxY = std::min(m_gridHeight - 1, std::max(m_boxStartCell.y, m_boxCurrentCell.y));
            int count = 0;
            for (int gy = minY; gy <= maxY; ++gy) {
                for (int gx = minX; gx <= maxX; ++gx) {
                    applyBrush(gx, gy, m_currentBrush);
                    count++;
                }
            }
            recalculatePaths();
            m_isDirty = true;
            showToast("Заливка: " + std::to_string(count) + " ячеек", glm::vec4(0.3f, 0.85f, 0.4f, 1.0f));
        }
        m_isBoxFilling = false;
        m_boxStartCell = glm::ivec2(-1);
        m_boxCurrentCell = glm::ivec2(-1);
    }

    if (!leftDown) {
        m_isDraggingEmitter = false;
        m_isDraggingDecoration = false;
        m_gizmoController.endResize();
    }

    if (!leftDown && !rightDown) {
        m_suppressPlacementUntilRelease = false;
    }

    if (!leftDown && !rightDown && (m_isLeftMouseDown || m_isRightMouseDown)) {
        commitStroke();
    }

    m_isLeftMouseDown = leftDown;
    m_isRightMouseDown = rightDown;
    m_keyLayerUpLastFrame = (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS);
    m_keyLayerDownLastFrame = (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS);
}

void MapEditorState::update(float dt) {
    m_toolbarUI.update(dt);

    if (m_grid) {
        m_pathVisualizer.update(dt, m_grid->getCellSize());
        m_envParticleManager.update(dt, *m_grid);
    }

    if (m_waveModal.isOpen()) {
        m_waveModal.update(dt);
    }

    if (m_mapsBrowserModal.isOpen()) {
        m_mapsBrowserModal.update(dt);
    }

    if (m_levelSettingsModal.isOpen()) {
        m_levelSettingsModal.update(dt);
    }

    if (m_statusTimer > 0.0f) {
        m_statusTimer -= dt;
        if (m_statusTimer <= 0.0f) {
            m_statusMessage = "";
            m_statusColor = glm::vec3(0.9f, 0.9f, 0.9f);
        }
    }

    if (m_toastTimer > 0.0f) {
        m_toastTimer -= dt;
        if (m_toastTimer <= 0.0f) {
            m_toastTimer = 0.0f;
            m_toastMessage = "";
        }
    }
}

void MapEditorState::render() {
    glm::mat4 P_screen = glm::ortho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, -1.0f, 1.0f);

    // 0. Экранный фон окна за пределами игровой сцены
    m_renderer->setProjection(P_screen);
    if (m_textRenderer) m_textRenderer->updateProjection(P_screen);
    m_renderer->beginBatch();
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec3(0.08f, 0.09f, 0.12f));
    m_renderer->endBatch();

    // 1. Установка матрицы камеры мира (Zoom & Pan) — через EditorCamera
    glm::mat4 cameraProj = P_screen * m_camera.viewMatrix();

    m_renderer->setProjection(cameraProj);
    m_renderer->beginBatch();

    BackgroundViewRect bgView;
    bgView.min = m_camera.screenToWorld(glm::vec2(0.0f, 0.0f));
    bgView.max = m_camera.screenToWorld(glm::vec2(static_cast<float>(m_width), static_cast<float>(m_height)));
    bgView.valid = true;

    std::vector<glm::ivec2> baseCells;
    baseCells.reserve(m_bases.size());
    for (const auto& b : m_bases) {
        baseCells.emplace_back(b.x, b.y);
    }

    // 1.1. СЛОЙ 1: Подложка фона и базовые клетки матрицы / outerCells
    BackgroundRenderer::renderSubstrate(
        m_renderer.get(),
        m_whiteTexture,
        m_background,
        m_grid.get(),
        static_cast<float>(m_width),
        static_cast<float>(m_height),
        bgView
    );
    if (m_grid) {
        m_grid->drawBaseCells(m_renderer.get(), m_whiteTexture, glm::vec3(1.0f), m_background);
    }

    // 1.2. СЛОЙ 2: Окружение фона (стена шахты, нижний слой тоннеля, пустошь, трубы)
    BackgroundRenderer::renderEnvironment(
        m_renderer.get(),
        m_whiteTexture,
        m_background,
        m_grid.get(),
        static_cast<float>(m_width),
        static_cast<float>(m_height),
        bgView,
        &baseCells
    );

    // 2. Препятствия и редакторские оверлеи сетки
    if (m_grid) {
        m_grid->drawObstacleCells(m_renderer.get(), m_whiteTexture, glm::vec3(1.0f), m_background);
        m_grid->drawEditorGridLines(m_renderer.get(), m_whiteTexture);
        m_grid->drawOuterNoBuildOverlay(m_renderer.get(), m_whiteTexture);
    }

    // 2.1. СЛОЙ 3: Верхний слой фона (2.5D забор, козырек и перекрытие портала шахты)
    BackgroundRenderer::renderForeground(
        m_renderer.get(),
        m_whiteTexture,
        m_background,
        m_grid.get(),
        static_cast<float>(m_width),
        static_cast<float>(m_height),
        bgView,
        &baseCells
    );

    // 3. Стрелочки путей в реальном времени с цветом, соответствующим целевой базе!
    if (m_showPathArrows && m_grid && m_arrowTexture) {
        for (size_t i = 0; i < m_activePaths.size(); ++i) {
            if (!m_activePaths[i].empty() && i < m_spawners.size()) {
                int targetId = m_spawners[i].targetBaseIndex;
                glm::vec3 arrowColor = getIdColor(targetId);
                m_pathVisualizer.renderPathArrows(m_renderer.get(), m_arrowTexture, m_activePaths[i], *m_grid, arrowColor);
            }
        }
    }

    // 4. Отрисовка маркеров спавнеров и баз
    if (m_grid) {
        float cellSize = m_grid->getCellSize();

        // Базы
        for (size_t bi = 0; bi < m_bases.size(); ++bi) {
            const auto& b = m_bases[bi];
            glm::vec2 pPos = m_grid->gridToPixel(b.x, b.y);
            glm::vec3 bCol = getIdColor(b.id);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, bCol);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, bCol * 0.85f);

            // Рамка выделения, если база выбрана в инспекторе
            if (m_selectedEntityIndex == static_cast<int>(bi) && m_selectedEntityType == EditorEntityType::Base) {
                float selB = 2.5f;
                glm::vec4 selCol(1.0f, 1.0f, 1.0f, 0.95f);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(0.0f, cellSize - selB), glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(selB, cellSize), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(cellSize - selB, 0.0f), glm::vec2(selB, cellSize), 0.0f, selCol);
            }
        }

        // Спавнеры
        for (size_t i = 0; i < m_spawners.size(); ++i) {
            const auto& sp = m_spawners[i];
            glm::vec2 pPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
            bool isBlocked = (i >= m_activePaths.size() || m_activePaths[i].empty());

            glm::vec3 spColor;
            if (isBlocked) {
                spColor = glm::vec3(0.95f, 0.15f, 0.15f);
            } else {
                spColor = getIdColor(sp.targetBaseIndex);
            }

            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, spColor);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, spColor * 0.85f);

            // Рамка выделения, если спавнер выбран в инспекторе
            if (m_selectedEntityIndex == static_cast<int>(i) && m_selectedEntityType == EditorEntityType::Spawner) {
                float selB = 2.5f;
                glm::vec4 selCol(1.0f, 1.0f, 1.0f, 0.95f);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(0.0f, cellSize - selB), glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(selB, cellSize), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(cellSize - selB, 0.0f), glm::vec2(selB, cellSize), 0.0f, selCol);
            }
        }

        // Маркеры вагонетки (Депо/Старт и Тупик/Конец)
        if (!m_minecarts.empty()) {
            const auto& mc = m_minecarts[0];
            if (mc.hasStart()) {
                glm::vec2 pPos = m_grid->gridToPixel(mc.start.x, mc.start.y);
                glm::vec3 startCol = glm::vec3(0.95f, 0.70f, 0.15f); // Золотисто-помаранчевий
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, startCol);
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, startCol * 0.85f);
            }
            if (mc.hasEnd()) {
                glm::vec2 pPos = m_grid->gridToPixel(mc.end.x, mc.end.y);
                glm::vec3 endCol = glm::vec3(0.90f, 0.20f, 0.20f); // Червоний буферний
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, endCol);
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, endCol * 0.85f);
            }
        }

        // Підсвічування BFS-маршруту рейок (якщо валідний)
        if (m_isRailPathValid && !m_railPath.empty()) {
            float dotSize = std::max(4.0f, cellSize * 0.28f);
            float dotOff  = (cellSize - dotSize) * 0.5f;
            glm::vec3 pathCol = glm::vec3(0.15f, 0.90f, 0.80f); // бірюзовий
            for (const auto& pt : m_railPath) {
                glm::vec2 pPos = m_grid->gridToPixel(pt.x, pt.y);
                m_renderer->drawSprite(m_whiteTexture,
                    pPos + glm::vec2(dotOff),
                    glm::vec2(dotSize), 0.0f, pathCol);
            }
        }

        // 4.15. Отрисовка зон запрета строительства (No-Build / Blocked Cells)
        float borderW = std::max(1.5f, cellSize * 0.04f);
        glm::vec4 borderCol(0.95f, 0.22f, 0.22f, 0.85f);
        glm::vec4 bgCol(0.85f, 0.15f, 0.15f, 0.32f);
        glm::vec4 crossCol(1.0f, 0.25f, 0.25f, 0.75f);
        float barLen = cellSize * 0.68f;
        float barThick = std::max(2.0f, cellSize * 0.05f);

        for (int gy = 0; gy < m_gridHeight; ++gy) {
            for (int gx = 0; gx < m_gridWidth; ++gx) {
                bool blocked = (gy < (int)m_noBuildMask.size() && gx < (int)m_noBuildMask[gy].size()) ? m_noBuildMask[gy][gx] : false;
                if (!blocked) continue;

                glm::vec2 pPos = m_grid->gridToPixel(gx, gy);
                // Полупрозрачный красный фон
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(1.0f), glm::vec2(cellSize - 2.0f), 0.0f, bgCol);
                // Контурная рамка
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(cellSize, borderW), 0.0f, borderCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(0.0f, cellSize - borderW), glm::vec2(cellSize, borderW), 0.0f, borderCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(borderW, cellSize), 0.0f, borderCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(cellSize - borderW, 0.0f), glm::vec2(borderW, cellSize), 0.0f, borderCol);
                // Диагональный крест запрета
                glm::vec2 center = pPos + glm::vec2(cellSize * 0.5f);
                m_renderer->drawSpriteRGBA(m_whiteTexture, center - glm::vec2(barLen * 0.5f, barThick * 0.5f), glm::vec2(barLen, barThick), 45.0f, crossCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, center - glm::vec2(barLen * 0.5f, barThick * 0.5f), glm::vec2(barLen, barThick), -45.0f, crossCol);
            }
        }

        // Подсветка курсора при выбранном инструменте "Клетка запрета"
        if (m_currentBrush == EditorBrush::NoBuild && !m_toolbarUI.isHovered(m_mousePos.x, m_mousePos.y)) {
            glm::vec2 worldM = screenToWorld(m_mousePos);
            glm::ivec2 curTile = m_grid->pixelToGrid(worldM);
            bool inBoundsHover = (curTile.x >= 0 && curTile.x < m_gridWidth && curTile.y >= 0 && curTile.y < m_gridHeight);
            bool allowOuterHover = (m_hasSidePockets || (m_grid && m_grid->hasSidePockets()));
            if (inBoundsHover || allowOuterHover) {
                glm::vec2 pPos = m_grid->gridToPixel(curTile.x, curTile.y);
                float hBorder = std::max(2.5f, cellSize * 0.06f);
                glm::vec4 hBorderCol(1.0f, 0.45f, 0.45f, 0.95f);
                glm::vec4 hFillCol(1.0f, 0.20f, 0.20f, 0.40f);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(1.0f), glm::vec2(cellSize - 2.0f), 0.0f, hFillCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(cellSize, hBorder), 0.0f, hBorderCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(0.0f, cellSize - hBorder), glm::vec2(cellSize, hBorder), 0.0f, hBorderCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(hBorder, cellSize), 0.0f, hBorderCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(cellSize - hBorder, 0.0f), glm::vec2(hBorder, cellSize), 0.0f, hBorderCol);
            }
        }
    }

    // 4.2. Рамка предпросмотра прямоугольной заливки (Box Fill)
    if (m_isBoxFilling && m_grid && m_boxStartCell.x >= 0 && m_boxCurrentCell.x >= 0) {
        int minX = std::max(0, std::min(m_boxStartCell.x, m_boxCurrentCell.x));
        int maxX = std::min(m_gridWidth - 1, std::max(m_boxStartCell.x, m_boxCurrentCell.x));
        int minY = std::max(0, std::min(m_boxStartCell.y, m_boxCurrentCell.y));
        int maxY = std::min(m_gridHeight - 1, std::max(m_boxStartCell.y, m_boxCurrentCell.y));

        float cellSize = m_grid->getCellSize();
        glm::vec2 p1 = m_grid->gridToPixel(minX, minY);
        glm::vec2 p2 = m_grid->gridToPixel(maxX, maxY) + glm::vec2(cellSize);
        glm::vec2 boxPos = p1;
        glm::vec2 boxSize = p2 - p1;

        // Полупрозрачная подсветка области
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos, boxSize, 0.0f, glm::vec4(0.25f, 0.65f, 1.0f, 0.25f));

        // Контурная рамка
        glm::vec4 frameCol = glm::vec4(0.35f, 0.85f, 1.0f, 0.90f);
        float borderThick = 2.5f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos, glm::vec2(boxSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos + glm::vec2(0.0f, boxSize.y - borderThick), glm::vec2(boxSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos, glm::vec2(borderThick, boxSize.y), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos + glm::vec2(boxSize.x - borderThick, 0.0f), glm::vec2(borderThick, boxSize.y), 0.0f, frameCol);
    }

    // 4.3. ВИДОИСКАТЕЛЬ СТАРТОВОЙ КАМЕРЫ (Camera Viewport Frame)
    if (m_isCameraConfigMode || m_cameraSettings.isCustom || m_hudPreviewMode != HudPreviewMode::None) {
        float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
        glm::vec2 camCenter = getWorldCamCenter();

        float cs = m_grid ? m_grid->getCellSize() : 64.0f;
        glm::vec2 off = m_grid ? m_grid->getOffset() : glm::vec2(0.0f);
        float centerWorldX = off.x + m_cameraSettings.targetTile.x * cs;
        float centerWorldY = off.y + m_cameraSettings.targetTile.y * cs;
        float halfW = (m_cameraSettings.viewTilesW > 0.1f) ? (m_cameraSettings.viewTilesW * cs * 0.5f) : ((static_cast<float>(m_width) / camZoom) * 0.5f);
        float halfH = (m_cameraSettings.viewTilesH > 0.1f) ? (m_cameraSettings.viewTilesH * cs * 0.5f) : ((static_cast<float>(m_height) / camZoom) * 0.5f);

        float worldLeft   = centerWorldX - halfW;
        float worldRight  = centerWorldX + halfW;
        float worldTop    = centerWorldY - halfH;
        float worldBottom = centerWorldY + halfH;

        glm::vec2 framePos(worldLeft, worldTop);
        glm::vec2 frameSize(worldRight - worldLeft, worldBottom - worldTop);
        float frameW = frameSize.x;
        float frameH = frameSize.y;

        // Полупрозрачная белая подложка видоискателя
        float fillAlpha = m_isCameraConfigMode ? 0.08f : 0.04f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, frameSize, 0.0f, glm::vec4(1.0f, 1.0f, 1.0f, fillAlpha));

        // Контурная рамка видоискателя
        glm::vec4 frameCol = m_isCameraConfigMode ? glm::vec4(1.0f, 1.0f, 1.0f, 0.95f) : glm::vec4(0.85f, 0.92f, 1.0f, 0.65f);
        float borderThick = 2.5f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(frameSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(0.0f, frameSize.y - borderThick), glm::vec2(frameSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(borderThick, frameSize.y), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - borderThick, 0.0f), glm::vec2(borderThick, frameSize.y), 0.0f, frameCol);

        // Уголки видоискателя (Camera Viewport Brackets)
        float cornerLen = 28.0f;
        float cornerThick = 4.0f;
        glm::vec4 cornerCol = m_isCameraConfigMode ? glm::vec4(0.35f, 0.90f, 1.0f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.85f);
        // Top-Left
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);
        // Top-Right
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerLen, 0.0f), glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerThick, 0.0f), glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);
        // Bottom-Left
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(0.0f, frameSize.y - cornerThick), glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(0.0f, frameSize.y - cornerLen), glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);
        // Bottom-Right
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerLen, frameSize.y - cornerThick), glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerThick, frameSize.y - cornerLen), glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);

        // Прицел центра (Center Crosshair)
        float crossLen = 12.0f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, camCenter - glm::vec2(crossLen * 0.5f, 1.0f), glm::vec2(crossLen, 2.0f), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, camCenter - glm::vec2(1.0f, crossLen * 0.5f), glm::vec2(2.0f, crossLen), 0.0f, cornerCol);

        // Текстовая плашка сверху рамки
        std::string camTitle = m_isCameraConfigMode ? "CAMERA VIEWPORT (НАСТРОЙКА)" :
            (m_hudPreviewMode != HudPreviewMode::None ? "CAMERA VIEWPORT (БОЕВОЙ HUD)" : "CAMERA VIEWPORT (РУЧНАЯ)");
        float fScale = 0.55f;
        float tw = m_textRenderer ? m_textRenderer->CalculateTextWidth(camTitle, fScale) : 260.0f;
        float badgeW = tw + 20.0f;
        float badgeH = 24.0f;
        glm::vec2 badgePos(framePos.x + 10.0f, framePos.y - badgeH - 4.0f);
        m_renderer->drawSpriteRGBA(m_whiteTexture, badgePos, glm::vec2(badgeW, badgeH), 0.0f, glm::vec4(0.08f, 0.10f, 0.14f, 0.85f));
        m_renderer->drawSpriteRGBA(m_whiteTexture, badgePos, glm::vec2(3.0f, badgeH), 0.0f, cornerCol);
    }

    // 4.4. ПРЕДПРОСМОТР БОЕВОГО ИНТЕРФЕЙСА (HUD Preview Overlay в мировых координатах видоискателя)
    renderHudPreview();

    // 4.44. ФОНОВЫЕ ЧАСТИЦЫ ЭМИТТЕРОВ (zIndex < 0, под декорациями и объектами)
    if (m_grid) {
        m_envParticleManager.render(m_renderer.get(), m_whiteTexture, *m_grid, -100, -1);
    }

    // 4.45. ДЕКОРАТИВНЫЕ ПРОП-ОБЪЕКТЫ (ПОД ЭМИТТЕРАМИ/ПАРТИКЛАМИ, ПОВЕРХ ТАЙЛОВ)
    if (m_grid) {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, *m_grid, m_selectedDecorationIndex, false);
    } else {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, m_selectedDecorationIndex, 64.0f, false);
    }

    // Предпросмотр установки декора под курсором на вкладке [4] Декор (для обычных декораций)
    if (m_toolbarUI.getCategory() == PaletteCategory::Decorations && !m_isDraggingDecoration && !m_gizmoController.isResizing() &&
        m_currentBrush != EditorBrush::Eraser && m_currentBrush != EditorBrush::None && m_currentBrush != EditorBrush::DecorFog) {
        std::string previewType = "bush";
        if (m_currentBrush == EditorBrush::DecorGrass) previewType = "grass_tuft";
        else if (m_currentBrush == EditorBrush::DecorGrassField) previewType = "grass_field";
        else if (m_currentBrush == EditorBrush::DecorFlower) previewType = "flower";
        else if (m_currentBrush == EditorBrush::DecorStone) previewType = "stone";
        else if (m_currentBrush == EditorBrush::DecorHelmet) previewType = "helmet";
        else if (m_currentBrush == EditorBrush::DecorPickaxe) previewType = "pickaxe";
        else if (m_currentBrush == EditorBrush::DecorPuddle) previewType = "puddle";
        else if (m_currentBrush == EditorBrush::DecorCrack) previewType = "crack";
        else if (m_currentBrush == EditorBrush::DecorWoodenCrate) previewType = "wooden_crate";
        else if (m_currentBrush == EditorBrush::DecorBrokenCart) previewType = "broken_mercury_cart";
        else if (m_currentBrush == EditorBrush::DecorScrapDebris) previewType = "scrap_debris";
        else if (m_currentBrush == EditorBrush::DecorDeadTree) previewType = "dead_tree";
        else if (m_currentBrush == EditorBrush::DecorDryGrass) previewType = "dry_grass";
        else if (m_currentBrush == EditorBrush::DecorOreDebris) previewType = "ore_debris";

        glm::vec2 worldMouse = screenToWorld(m_mousePos);
        float previewCellSize = m_grid ? m_grid->getCellSize() : 64.0f;
        if (m_grid && previewCellSize > 0.001f) {
            bool isGrassField = (m_currentBrush == EditorBrush::DecorGrassField);
            if (m_decorGridSnap || isGrassField) {
                glm::vec2 rawTile = (worldMouse - m_grid->getOffset()) / previewCellSize;
                glm::vec2 snappedTile = glm::floor(rawTile) + glm::vec2(0.5f);
                worldMouse = m_grid->getOffset() + snappedTile * previewCellSize;
            }
        }
        DecorationRenderer::renderPreview(m_renderer.get(), m_whiteTexture, previewType, worldMouse, m_defaultDecorationRotation, 1.0f, 1.0f, 1.0f, 0.5f, previewCellSize);
    }

    // 4.5. РЕНДЕРИНГ МАРКЕРОВ ЭМИТТЕРОВ ЧАСТИЦ
    renderEmitters(m_renderer.get(), m_whiteTexture);

    // 4.6. ЖИВЫЕ ЧАСТИЦЫ ЭМИТТЕРОВ ОКРУЖЕНИЯ (zIndex >= 0, поверх объектов)
    m_envParticleManager.render(m_renderer.get(), m_whiteTexture, *m_grid, 0, 100);

    // 4.7. ДЕКОРАЦИИ ВЕРХНЕГО СЛОЯ (ТУМАН / ТЬМА) — поверх частиц, эмиттеров, предпросмотра
    if (m_grid) {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, *m_grid, m_selectedDecorationIndex, true);
    } else {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, m_selectedDecorationIndex, 64.0f, true);
    }

    // Предпросмотр установки тумана под курсором на вкладке [4] Декор (поверх частиц)
    if (m_toolbarUI.getCategory() == PaletteCategory::Decorations && !m_isDraggingDecoration && !m_gizmoController.isResizing() &&
        m_currentBrush == EditorBrush::DecorFog) {
        glm::vec2 worldMouse = screenToWorld(m_mousePos);
        float previewCellSize = m_grid ? m_grid->getCellSize() : 64.0f;
        if (m_grid && previewCellSize > 0.001f && m_decorGridSnap) {
            glm::vec2 rawTile = (worldMouse - m_grid->getOffset()) / previewCellSize;
            glm::vec2 snappedTile = glm::floor(rawTile) + glm::vec2(0.5f);
            worldMouse = m_grid->getOffset() + snappedTile * previewCellSize;
        }
        DecorationRenderer::renderPreview(m_renderer.get(), m_whiteTexture, "fog", worldMouse, m_defaultDecorationRotation, 1.0f, 1.0f, 1.0f, 0.5f, previewCellSize);
    }

    m_renderer->endBatch();

    // 5. Восстанавливаем проекцию экрана для элементов пользовательского интерфейса (UI) и экранного текста
    m_renderer->setProjection(P_screen);
    if (m_textRenderer) m_textRenderer->updateProjection(P_screen);

    // 5.0. Иконки скрытия в бою (перечеркнутый глаз) для баз и спавнеров
    if (m_whiteTexture && m_grid) {
        float cellSize = m_grid->getCellSize();
        float screenCellSize = cellSize * m_zoom;

        m_renderer->beginBatch();

        for (const auto& b : m_bases) {
            if (!b.visibleInGame) {
                glm::vec2 worldPos = m_grid->gridToPixel(b.x, b.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);

                if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                    screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                    continue;
                }

                float iconW = std::clamp(16.0f * m_zoom, 10.0f, 22.0f);
                float iconH = iconW * 0.70f;
                glm::vec2 iconPos(screenTopLeft.x + screenCellSize - iconW - 3.0f * m_zoom, screenTopLeft.y + 3.0f * m_zoom);

                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos, glm::vec2(iconW, iconH), 0.0f, glm::vec4(0.04f, 0.06f, 0.08f, 0.80f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.15f, iconH * 0.15f), glm::vec2(iconW * 0.70f, iconH * 0.70f), 0.0f, glm::vec4(0.85f, 0.90f, 0.95f, 0.75f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.35f, iconH * 0.28f), glm::vec2(iconW * 0.30f, iconH * 0.44f), 0.0f, glm::vec4(0.12f, 0.15f, 0.22f, 0.90f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.10f, iconH * 0.80f), glm::vec2(iconW * 0.95f, std::max(1.5f, 2.0f * m_zoom)), -35.0f, glm::vec4(1.0f, 0.32f, 0.32f, 0.95f));
            }
        }

        for (const auto& sp : m_spawners) {
            if (!sp.visibleInGame) {
                glm::vec2 worldPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);

                if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                    screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                    continue;
                }

                float iconW = std::clamp(16.0f * m_zoom, 10.0f, 22.0f);
                float iconH = iconW * 0.70f;
                glm::vec2 iconPos(screenTopLeft.x + screenCellSize - iconW - 3.0f * m_zoom, screenTopLeft.y + 3.0f * m_zoom);

                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos, glm::vec2(iconW, iconH), 0.0f, glm::vec4(0.04f, 0.06f, 0.08f, 0.80f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.15f, iconH * 0.15f), glm::vec2(iconW * 0.70f, iconH * 0.70f), 0.0f, glm::vec4(0.85f, 0.90f, 0.95f, 0.75f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.35f, iconH * 0.28f), glm::vec2(iconW * 0.30f, iconH * 0.44f), 0.0f, glm::vec4(0.12f, 0.15f, 0.22f, 0.90f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.10f, iconH * 0.80f), glm::vec2(iconW * 0.95f, std::max(1.5f, 2.0f * m_zoom)), -35.0f, glm::vec4(1.0f, 0.32f, 0.32f, 0.95f));
            }
        }

        m_renderer->endBatch();
    }

    // 5.1. ТЕКСТ НА КЛЕТКАХ СЕТКИ (в экранных координатах с учетом зума и панорамирования)
    if (m_textRenderer && m_grid) {
        float cellSize = m_grid->getCellSize();
        float screenCellSize = cellSize * m_zoom;

        // Базы B#ID
        for (const auto& b : m_bases) {
            glm::vec2 worldPos = m_grid->gridToPixel(b.x, b.y);
            glm::vec2 screenTopLeft = worldToScreen(worldPos);

            if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                continue;
            }

            std::string baseLabel = "B#" + std::to_string(b.id);
            float fontScale = std::clamp(0.70f * m_zoom, 0.25f, 0.75f);
            float tw = m_textRenderer->CalculateTextWidth(baseLabel, fontScale);
            float th = fontScale * 26.0f;
            float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
            float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
            m_textRenderer->RenderText(baseLabel, tx, ty, fontScale, glm::vec3(0.05f, 0.08f, 0.15f));
        }

        // Спавнеры S>#ID / STOP / S:Auto
        for (size_t i = 0; i < m_spawners.size(); ++i) {
            const auto& sp = m_spawners[i];
            glm::vec2 worldPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
            glm::vec2 screenTopLeft = worldToScreen(worldPos);

            if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                continue;
            }

            bool isBlocked = (i >= m_activePaths.size() || m_activePaths[i].empty());
            bool hasTargetBase = (sp.targetBaseIndex == -1);
            if (!hasTargetBase) {
                for (const auto& b : m_bases) {
                    if (b.id == sp.targetBaseIndex) { hasTargetBase = true; break; }
                }
            }

            std::string spText;
            if (isBlocked) {
                spText = "STOP";
            } else if (sp.targetBaseIndex == -1) {
                spText = "S:Auto";
            } else if (!hasTargetBase) {
                spText = "S>?" + std::to_string(sp.targetBaseIndex);
            } else {
                spText = "S>#" + std::to_string(sp.targetBaseIndex);
            }

            float fontScale = std::clamp(0.62f * m_zoom, 0.22f, 0.70f);
            float tw = m_textRenderer->CalculateTextWidth(spText, fontScale);
            float th = fontScale * 26.0f;
            float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
            float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
            m_textRenderer->RenderText(spText, tx, ty, fontScale, glm::vec3(0.05f, 0.08f, 0.12f));
        }

        // Маркеры вагонетки (START / END)
        if (!m_minecarts.empty()) {
            const auto& mc = m_minecarts[0];
            if (mc.hasStart()) {
                glm::vec2 worldPos = m_grid->gridToPixel(mc.start.x, mc.start.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);
                if (screenTopLeft.x + screenCellSize >= 0.0f && screenTopLeft.x <= m_width &&
                    screenTopLeft.y + screenCellSize >= 0.0f && screenTopLeft.y <= m_height) {
                    std::string label = "START";
                    float baseScale = std::clamp(cellSize / 64.0f * 0.52f, 0.35f, 0.65f);
                    float fontScale = std::clamp(baseScale * m_zoom, 0.20f, 0.65f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fontScale);
                    float th = fontScale * 26.0f;
                    float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
                    float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fontScale, glm::vec3(0.15f, 0.10f, 0.03f));
                }
            }
            if (mc.hasEnd()) {
                glm::vec2 worldPos = m_grid->gridToPixel(mc.end.x, mc.end.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);
                if (screenTopLeft.x + screenCellSize >= 0.0f && screenTopLeft.x <= m_width &&
                    screenTopLeft.y + screenCellSize >= 0.0f && screenTopLeft.y <= m_height) {
                    std::string label = "END";
                    float baseScale = std::clamp(cellSize / 64.0f * 0.56f, 0.35f, 0.68f);
                    float fontScale = std::clamp(baseScale * m_zoom, 0.20f, 0.68f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fontScale);
                    float th = fontScale * 26.0f;
                    float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
                    float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fontScale, glm::vec3(1.0f, 0.95f, 0.95f));
                }
            }
        }

        // Подписи эмиттеров частиц (ПАР, КАП, ИСК, ДЫМ и градус)
        for (const auto& em : m_emitters) {
            glm::vec2 worldPos = m_grid->getOffset() + em.tilePos * cellSize;
            glm::vec2 screenCenter = worldToScreen(worldPos);
            glm::vec2 screenTopLeft = screenCenter - glm::vec2(screenCellSize * 0.5f);
            if (screenTopLeft.x + screenCellSize >= 0.0f && screenTopLeft.x <= m_width &&
                screenTopLeft.y + screenCellSize >= 0.0f && screenTopLeft.y <= m_height) {
                std::string tag = "ПАР";
                if (em.type == "water_drip") tag = "КАП";
                else if (em.type == "sparks") tag = "ИСК";
                else if (em.type == "smoke") tag = "ДЫМ";
                else if (em.type == "fog") tag = "ТУМ";

                float baseScale = std::clamp(cellSize / 64.0f * 0.42f, 0.28f, 0.55f);
                float fontScale = std::clamp(baseScale * m_zoom, 0.18f, 0.55f);
                float tw = m_textRenderer->CalculateTextWidth(tag, fontScale);
                float th = fontScale * 26.0f;
                float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
                float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f - 4.0f * m_zoom;
                m_textRenderer->RenderText(tag, tx, ty, fontScale, glm::vec3(1.0f, 1.0f, 1.0f));

                std::string degStr = std::to_string(static_cast<int>(em.angleDeg)) + "°";
                float degScale = fontScale * 0.82f;
                float dtw = m_textRenderer->CalculateTextWidth(degStr, degScale);
                float dtx = screenTopLeft.x + (screenCellSize - dtw) * 0.5f;
                float dty = ty + th + 1.0f;
                m_textRenderer->RenderText(degStr, dtx, dty, degScale, glm::vec3(0.85f, 0.95f, 1.0f));
            }
        }

        // Подпись видоискателя камеры (Camera Viewport Badge)
        if (m_isCameraConfigMode || m_cameraSettings.isCustom || m_hudPreviewMode != HudPreviewMode::None) {
            float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
            glm::vec2 camCenter = getWorldCamCenter();
            glm::vec2 frameSize = getCameraViewportSize();
            glm::vec2 framePos = camCenter - frameSize * 0.5f;
            glm::vec2 badgePos(framePos.x + 10.0f, framePos.y - 24.0f - 4.0f);
            glm::vec2 screenBadgePos = worldToScreen(badgePos);

            std::string camTitle = m_isCameraConfigMode ? "CAMERA VIEWPORT (НАСТРОЙКА)" :
                (m_hudPreviewMode != HudPreviewMode::None ? "CAMERA VIEWPORT (БОЕВОЙ HUD)" : "CAMERA VIEWPORT (РУЧНАЯ)");
            float badgeFontScale = std::clamp(0.55f * m_zoom, 0.28f, 0.70f);
            m_textRenderer->RenderText(camTitle, screenBadgePos.x + 8.0f * m_zoom, screenBadgePos.y + 3.0f * m_zoom, badgeFontScale, glm::vec3(0.95f, 0.95f, 1.0f));
        }

        // Текстовые метки боевого интерфейса внутри видоискателя
        renderHudPreviewText();
    }

    // ВЕРХНИЙ ХЕДЕР-БАР И НИЖНИЙ ТУЛБАР (EditorToolbarUI)
    m_renderer->beginBatch();
    m_toolbarUI.render(m_renderer.get(), m_textRenderer, m_whiteTexture, buildEditorContext(), m_mousePos);
    m_renderer->endBatch();

    // 8. МОДАЛЬНОЕ ОКНО РЕДАКТОРА ВОЛН (если активно)
    if (m_waveModal.isOpen()) {
        m_renderer->beginBatch();
        m_waveModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                           m_width, m_height, getTopBarHeight(), getBottomDockHeight(),
                           m_mousePos, m_waves);
        m_renderer->endBatch();
    }

    // 9. МОДАЛЬНОЕ ОКНО СПИСКА КАРТ И ПЕРЕИМЕНОВАНИЯ (если активно)
    if (m_mapsBrowserModal.isOpen()) {
        m_renderer->beginBatch();
        m_mapsBrowserModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture, m_width, m_height, m_mousePos);
        m_renderer->endBatch();
    }

    // 11. МОДАЛЬНОЕ ОКНО ПОДТВЕРЖДЕНИЯ ВЫХОДА (если активно)
    if (m_exitModal.isOpen()) {
        m_renderer->beginBatch();
        m_exitModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                           static_cast<float>(m_width), static_cast<float>(m_height), m_mousePos);
        m_renderer->endBatch();
    }

    // 12. МОДАЛЬНЕ ВІКНО НАЛАШТУВАНЬ ВАГОНЕТКИ (якщо активне)
    if (m_minecartModal.isOpen()) {
        m_renderer->beginBatch();
        m_minecartModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                              m_width, m_height, m_mousePos,
                              m_minecarts, m_isRailPathValid, m_railPath.size());
        m_renderer->endBatch();
    }

    // 12.2. МОДАЛЬНОЕ ОКНО СПРАВКИ ПО УПРАВЛЕНИЮ (если активно)
    if (m_helpModal.isOpen()) {
        m_renderer->beginBatch();
        m_helpModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture, m_width, m_height);
        m_renderer->endBatch();
    }

    // 12.2b. МОДАЛЬНОЕ ОКНО НАСТРОЕК УРОВНЯ И ЭКОНОМИКИ (если активно)
    if (m_levelSettingsModal.isOpen()) {
        m_renderer->beginBatch();
        m_levelSettingsModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture, m_width, m_height, m_mousePos);
        m_renderer->endBatch();
    }

    // 12.3. ВСПЛЫВАЮЩАЯ ПАНЕЛЬ НАСТРОЕК ВЫБРАННОГО ЭМИТТЕРА ПАРТИКЛОВ
    if (m_selectedEmitterIndex >= 0 && m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
        glm::vec2 emWorld = m_grid ? (m_grid->getOffset() + m_emitters[m_selectedEmitterIndex].tilePos * m_grid->getCellSize()) : glm::vec2(0.0f);
        glm::vec2 emScreen = worldToScreen(emWorld);

        m_renderer->beginBatch();
        m_emitterInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                  m_emitters[m_selectedEmitterIndex], m_selectedEmitterIndex, emScreen,
                                  static_cast<float>(m_width), static_cast<float>(m_height),
                                  getTopBarHeight(), getBottomDockHeight(), m_mousePos);
        m_renderer->endBatch();
    }

    // 12.4. ВСПЛЫВАЮЩАЯ ПАНЕЛЬ НАСТРОЕК ВЫБРАННОГО ДЕКОРА
    if (m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
        const auto& dec = m_decorations[m_selectedDecorationIndex];
        glm::vec2 decActualWorld = m_grid ? (m_grid->getOffset() + dec.tilePos * m_grid->getCellSize()) : dec.worldPos;
        glm::vec2 decScreen = worldToScreen(decActualWorld);

        m_renderer->beginBatch();
        m_decorInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                dec, m_selectedDecorationIndex, decScreen,
                                static_cast<float>(m_width), static_cast<float>(m_height),
                                getTopBarHeight(), getBottomDockHeight(), m_mousePos);
        m_renderer->endBatch();
    }

    // 12.5. ВСПЛЫВАЮЩАЯ ПАНЕЛЬ НАСТРОЕК ВЫБРАННОГО СПАВНЕРА ИЛИ БАЗЫ (Entity Inspector)
    if (m_selectedEntityIndex >= 0) {
        if (m_selectedEntityType == EditorEntityType::Spawner && m_selectedEntityIndex < static_cast<int>(m_spawners.size())) {
            glm::vec2 entWorld = m_grid ? (m_grid->gridToPixel(m_spawners[m_selectedEntityIndex].pos.x, m_spawners[m_selectedEntityIndex].pos.y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entScreen = worldToScreen(entWorld);

            m_renderer->beginBatch();
            m_entityInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                     m_spawners[m_selectedEntityIndex], m_selectedEntityIndex, entScreen,
                                     static_cast<float>(m_width), static_cast<float>(m_height),
                                     getTopBarHeight(), getBottomDockHeight(), m_mousePos);
            m_renderer->endBatch();
        } else if (m_selectedEntityType == EditorEntityType::Base && m_selectedEntityIndex < static_cast<int>(m_bases.size())) {
            glm::vec2 entWorld = m_grid ? (m_grid->gridToPixel(m_bases[m_selectedEntityIndex].x, m_bases[m_selectedEntityIndex].y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entScreen = worldToScreen(entWorld);

            m_renderer->beginBatch();
            m_entityInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                     m_bases[m_selectedEntityIndex], m_selectedEntityIndex, entScreen,
                                     static_cast<float>(m_width), static_cast<float>(m_height),
                                     getTopBarHeight(), getBottomDockHeight(), m_mousePos);
            m_renderer->endBatch();
        }
    }

    // 12.5. Инфо-плашка тайла под курсором
    renderTileInfoBox();

    // 13. Всплывающее тост-уведомление (Toast Feedback)
    if (m_toastTimer > 0.0f && !m_toastMessage.empty()) {
        m_renderer->beginBatch();

        float scale = GetUIScale(m_width, m_height);
        float fontScale = std::clamp(0.52f * scale, 0.38f, 0.65f);
        float textW = m_textRenderer ? m_textRenderer->CalculateTextWidth(m_toastMessage, fontScale) : 180.0f;

        float padX = 22.0f * scale;
        float toastW = textW + padX * 2.0f;
        float toastH = 38.0f * scale;
        float toastX = (static_cast<float>(m_width) - toastW) * 0.5f;

        // Плавная прозрачность: появление в начале (первые 0.25с) и затухание в конце (последние 0.4с)
        float alpha = 1.0f;
        if (m_toastTimer < 0.4f) {
            alpha = m_toastTimer / 0.4f;
        } else if (m_toastTimer > 2.55f) {
            alpha = (2.8f - m_toastTimer) / 0.25f;
        }
        alpha = std::clamp(alpha, 0.0f, 1.0f);

        // Плавный выезд сверху
        float targetY = getTopBarHeight() + 12.0f * scale;
        float toastY = targetY - (1.0f - alpha) * 8.0f;

        // Тень
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(toastX + 3.0f, toastY + 4.0f),
            glm::vec2(toastW, toastH),
            0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.40f * alpha));

        // Темная подложка
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(toastX, toastY),
            glm::vec2(toastW, toastH),
            0.0f, glm::vec4(0.10f, 0.12f, 0.16f, 0.95f * alpha));

        // Рамка цвета тоста
        glm::vec4 borderCol = glm::vec4(glm::vec3(m_toastColor), m_toastColor.a * alpha);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX, toastY), glm::vec2(toastW, 1.5f), 0.0f, borderCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX, toastY + toastH - 1.5f), glm::vec2(toastW, 1.5f), 0.0f, borderCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX, toastY), glm::vec2(1.5f, toastH), 0.0f, borderCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX + toastW - 1.5f, toastY), glm::vec2(1.5f, toastH), 0.0f, borderCol);

        // Акцентная цветная полоса слева
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(toastX + 3.0f, toastY + 3.0f),
            glm::vec2(5.0f, toastH - 6.0f),
            0.0f, borderCol);

        m_renderer->flush();

        if (m_textRenderer) {
            float tx = toastX + padX;
            float ty = toastY + (toastH - fontScale * 28.0f) * 0.5f + 1.0f;
            glm::vec3 txtCol = glm::vec3(1.0f) * alpha;
            m_textRenderer->RenderText(m_toastMessage, tx, ty, fontScale, txtCol);
        }

        m_renderer->endBatch();
    }
}

glm::vec2 MapEditorState::getDefaultWorldCenter() const {
    if (m_grid) {
        return m_grid->getOffset() + glm::vec2(m_gridWidth, m_gridHeight) * m_grid->getCellSize() * 0.5f;
    }
    return glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
}

glm::vec2 MapEditorState::getWorldCamCenter() const {
    if (!m_grid) return getDefaultWorldCenter();
    glm::vec2 targetTile = m_cameraSettings.targetTile;
    if (targetTile.x < -100.0f || targetTile.y < -100.0f) {
        targetTile = glm::vec2(static_cast<float>(m_gridWidth) * 0.5f,
                               static_cast<float>(m_gridHeight) * 0.5f);
    }
    return m_grid->getOffset() + targetTile * m_grid->getCellSize();
}

glm::vec2 MapEditorState::getCameraViewportSize() const {
    if (!m_grid) {
        float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
        return glm::vec2(static_cast<float>(m_width) / camZoom, static_cast<float>(m_height) / camZoom);
    }
    float cs = m_grid->getCellSize();
    if (m_cameraSettings.viewTilesW > 0.1f && m_cameraSettings.viewTilesH > 0.1f) {
        return glm::vec2(m_cameraSettings.viewTilesW * cs, m_cameraSettings.viewTilesH * cs);
    }
    float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
    return glm::vec2(static_cast<float>(m_width) / camZoom, static_cast<float>(m_height) / camZoom);
}

void MapEditorState::syncCameraSettingsFromCurrentView() {
    glm::vec2 screenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
    glm::vec2 worldCenter = screenToWorld(screenCenter);
    float camZoom = std::clamp(m_zoom, 0.4f, 2.5f);
    m_cameraSettings.targetTile = worldPixelToTile(worldCenter);
    m_cameraSettings.zoom = camZoom;
    if (m_grid && m_grid->getCellSize() > 0.001f) {
        float cs = m_grid->getCellSize();
        m_cameraSettings.viewTilesW = (static_cast<float>(m_width) / camZoom) / cs;
        m_cameraSettings.viewTilesH = (static_cast<float>(m_height) / camZoom) / cs;
    }
    m_cameraSettings.isCustom = true;
    m_isDirty = true;
}

glm::vec2 MapEditorState::worldPixelToTile(glm::vec2 worldPos) const {
    if (!m_grid) return glm::vec2(0.0f);
    float cellSize = m_grid->getCellSize();
    if (cellSize <= 0.001f) return glm::vec2(0.0f);
    return (worldPos - m_grid->getOffset()) / cellSize;
}

glm::vec2 MapEditorState::tileToWorldPixel(glm::vec2 tilePos) const {
    if (!m_grid) return tilePos;
    return m_grid->getOffset() + tilePos * m_grid->getCellSize();
}

void MapEditorState::resize(int width, int height) {
    glm::vec2 oldScreenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
    glm::vec2 worldFocus = screenToWorld(oldScreenCenter);

    m_width = width;
    m_height = height;
    updateGridDimensions();
    updateButtonLayout();

    // Сохраняем фокус камеры на том же мировом объекте/точке при изменении размера окна
    glm::vec2 newScreenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
    m_cameraPan = newScreenCenter - worldFocus * m_zoom;
}















void MapEditorState::openExitModal() {
    if (!m_isDirty) {
        returnToOrigin();
        return;
    }
    m_exitModal.open(true);
    m_suppressPlacementUntilRelease = true;
    m_suppressClickUntilRelease = true;
}

void MapEditorState::closeExitModal() {
    m_exitModal.close();
    m_suppressPlacementUntilRelease = true;
    m_keyEscPressedLastFrame = true;
}

void MapEditorState::renderHudPreview() {
    EditorHudPreviewRenderer::renderWorldOverlay(
        m_renderer.get(),
        m_whiteTexture,
        m_hudPreviewMode,
        m_cameraSettings,
        getWorldCamCenter(),
        getCameraViewportSize()
    );
}

void MapEditorState::renderHudPreviewText() {
    EditorHudPreviewRenderer::renderScreenText(
        m_textRenderer,
        m_hudPreviewMode,
        m_cameraSettings,
        getWorldCamCenter(),
        getCameraViewportSize(),
        m_camera
    );
}

void MapEditorState::renderEmitters(SpriteRenderer* renderer, const Texture2D* texture) {
    EditorEmitterRenderer::render(renderer, texture, m_grid.get(), m_emitters, m_selectedEmitterIndex);
}

void MapEditorState::executePipette(int gx, int gy, glm::vec2 worldMouse) {
    if (!m_grid) return;
    bool inBounds = (gx >= 0 && gx < m_gridWidth && gy >= 0 && gy < m_gridHeight);
    bool canInteract = inBounds || m_hasSidePockets || m_grid->hasSidePockets();
    if (!canInteract) return;

    EditorBrush pickedBrush = EditorBrush::Ground;
    std::string tileName = (m_background == "pipes_canal" || m_background == "channel") ? "Асфальт" : "Грунт";
    bool foundObj = false;

    // 1. Спавнеры
    for (const auto& sp : m_spawners) {
        if (sp.pos.x == gx && sp.pos.y == gy) {
            pickedBrush = EditorBrush::Spawner;
            tileName = (sp.targetBaseIndex == -1) ? "Спавнер :Auto" : ("Спавнер #" + std::to_string(sp.targetBaseIndex));
            m_selectedId = sp.targetBaseIndex;
            foundObj = true;
            break;
        }
    }

    // 2. Базы
    if (!foundObj) {
        for (const auto& b : m_bases) {
            if (b.x == gx && b.y == gy) {
                pickedBrush = EditorBrush::Base;
                tileName = "База #" + std::to_string(b.id);
                m_selectedId = b.id;
                foundObj = true;
                break;
            }
        }
    }

    // 3. Маркеры вагонетки
    if (!foundObj && !m_minecarts.empty()) {
        const auto& mc = m_minecarts[0];
        if (mc.hasStart() && mc.start.x == gx && mc.start.y == gy) {
            pickedBrush = EditorBrush::RailStart;
            tileName = "Депо";
            foundObj = true;
        } else if (mc.hasEnd() && mc.end.x == gx && mc.end.y == gy) {
            pickedBrush = EditorBrush::RailEnd;
            tileName = "Тупик";
            foundObj = true;
        }
    }

    // 4. Тип тайла клетки (даже под настилом и за пределами базовой матрицы)
    if (!foundObj) {
        CellType ct = m_grid->getCellType(gx, gy);
        switch (ct) {
            case CellType::Ground:
                pickedBrush = EditorBrush::Ground;
                tileName = (m_background == "pipes_canal" || m_background == "channel") ? "Асфальт" : "Грунт";
                break;
            case CellType::Path:
                pickedBrush = EditorBrush::Path;
                tileName = "Дорога";
                break;
            case CellType::Platform:
                pickedBrush = EditorBrush::Platform;
                tileName = (m_background == "pipes_canal" || m_background == "channel") ? "Трава/Обочина" : ((m_background == "mine_entrance" || m_background == "mine") ? "Сланец" : "Платформа");
                break;
            case CellType::Scenery:
                pickedBrush = EditorBrush::Wall;
                tileName = (m_background == "mine_entrance" || m_background == "mine") ? "Ящики" : "Стена";
                break;
            case CellType::Chasm:
                pickedBrush = EditorBrush::Chasm;
                tileName = "Шурф";
                break;
            case CellType::Rail:
                pickedBrush = EditorBrush::Rail;
                tileName = "Рельсы";
                break;
            case CellType::Empty:
                pickedBrush = EditorBrush::Eraser;
                tileName = "Пусто (Ластик)";
                break;
            default:
                pickedBrush = EditorBrush::Ground;
                tileName = (m_background == "pipes_canal" || m_background == "channel") ? "Асфальт" : "Грунт";
                break;
        }
    }

    // 5. Декорации (если кликнули вблизи декора)
    int nearDec = findNearestDecorationIndex(worldMouse, 28.0f);
    if (nearDec >= 0 && nearDec < static_cast<int>(m_decorations.size())) {
        const std::string& dt = m_decorations[nearDec].type;
        if (dt == "bush") { pickedBrush = EditorBrush::DecorBush; tileName = "Куст"; }
        else if (dt == "grass_tuft") { pickedBrush = EditorBrush::DecorGrass; tileName = "Пучок травы"; }
        else if (dt == "grass_field") { pickedBrush = EditorBrush::DecorGrassField; tileName = "Поле травы"; }
        else if (dt == "flower") { pickedBrush = EditorBrush::DecorFlower; tileName = "Цветы"; }
        else if (dt == "stone") { pickedBrush = EditorBrush::DecorStone; tileName = "Камень"; }
        else if (dt == "helmet") { pickedBrush = EditorBrush::DecorHelmet; tileName = "Каска"; }
        else if (dt == "pickaxe") { pickedBrush = EditorBrush::DecorPickaxe; tileName = "Кирка"; }
        else if (dt == "puddle") { pickedBrush = EditorBrush::DecorPuddle; tileName = "Лужа"; }
        else if (dt == "crack") { pickedBrush = EditorBrush::DecorCrack; tileName = "Трещина"; }
        else if (dt == "fog") { pickedBrush = EditorBrush::DecorFog; tileName = "Туман"; }
        else if (dt == "wooden_crate") { pickedBrush = EditorBrush::DecorWoodenCrate; tileName = "Ящик"; }
        else if (dt == "broken_mercury_cart") { pickedBrush = EditorBrush::DecorBrokenCart; tileName = "Вагонетка"; }
        else if (dt == "scrap_debris") { pickedBrush = EditorBrush::DecorScrapDebris; tileName = "Мусор/доски"; }
        else if (dt == "dead_tree") { pickedBrush = EditorBrush::DecorDeadTree; tileName = "Сухое дерево"; }
        else if (dt == "dry_grass") { pickedBrush = EditorBrush::DecorDryGrass; tileName = "Сухая трава"; }
        else if (dt == "ore_debris") { pickedBrush = EditorBrush::DecorOreDebris; tileName = "Обломки руды"; }
        foundObj = true;
    }

    m_currentBrush = pickedBrush;
    if (pickedBrush == EditorBrush::Spawner || pickedBrush == EditorBrush::Base ||
        pickedBrush == EditorBrush::Rail || pickedBrush == EditorBrush::RailStart ||
        pickedBrush == EditorBrush::RailEnd) {
        m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
    } else if (pickedBrush == EditorBrush::EmitterSteamJet || pickedBrush == EditorBrush::EmitterWaterDrip ||
               pickedBrush == EditorBrush::EmitterSparks || pickedBrush == EditorBrush::EmitterSmoke ||
               pickedBrush == EditorBrush::EmitterFog) {
        m_toolbarUI.setCategory(PaletteCategory::Particles);
    } else if (pickedBrush >= EditorBrush::DecorBush && pickedBrush <= EditorBrush::DecorOreDebris) {
        m_toolbarUI.setCategory(PaletteCategory::Decorations);
        if (pickedBrush >= EditorBrush::DecorWoodenCrate) {
            m_toolbarUI.setDecorPage(1);
        } else {
            m_toolbarUI.setDecorPage(0);
        }
    } else {
        m_toolbarUI.setCategory(PaletteCategory::Tiles);
    }
    updateButtonLayout();

    showToast("Выбран тайл: " + tileName, glm::vec4(0.3f, 0.8f, 1.0f, 1.0f));
    m_suppressPlacementUntilRelease = true;
}

void MapEditorState::renderTileInfoBox() {
    EditorTileInfoOverlay::Params p;
    p.mousePos = m_mousePos;
    p.screenWidth = m_width;
    p.screenHeight = m_height;
    p.bottomDockHeight = getBottomDockHeight();
    p.gridWidth = m_gridWidth;
    p.gridHeight = m_gridHeight;
    p.hasSidePockets = m_hasSidePockets;
    p.background = m_background;
    p.isSuppressed = m_toolbarUI.isHovered(m_mousePos.x, m_mousePos.y) ||
                     m_waveModal.isOpen() || m_mapsBrowserModal.isOpen() || m_exitModal.isOpen() ||
                     m_helpModal.isOpen() || m_levelSettingsModal.isOpen() || m_minecartModal.isOpen();

    EditorTileInfoOverlay::render(
        m_renderer.get(),
        m_textRenderer,
        m_whiteTexture,
        m_camera,
        m_grid.get(),
        m_spawners,
        m_bases,
        m_minecarts,
        p
    );
}
