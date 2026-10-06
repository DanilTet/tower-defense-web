#include "MapEditorState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "LevelSelectState.h"
#include "GameplayState.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../resources/ResourceManager.h"
#include "../textures/Texture2D.h"
#include "../world/PathService.h"
#include "../ui/UICommon.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <algorithm>
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
        m_isMapsModalOpen = true;
        m_isInitialModalLaunch = true;
    }
    std::cout << "[MapEditor] Ctor: levelToLoad='" << levelToLoad << "', currentLevel='" << m_currentLevelFileName
              << "', m_isMapsModalOpen=" << (m_isMapsModalOpen ? "true" : "false")
              << ", m_isInitialModalLaunch=" << (m_isInitialModalLaunch ? "true" : "false")
              << ", m_suppressClick=" << (m_suppressClickUntilRelease ? "true" : "false") << std::endl;
}

void MapEditorState::returnToOrigin() {
    if (m_origin == EditorOrigin::Campaign) {
        std::cout << "[MapEditor] Returning to Campaign LevelSelect..." << std::endl;
        m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Campaign));
    } else if (m_origin == EditorOrigin::Custom) {
        std::cout << "[MapEditor] Returning to Custom LevelSelect..." << std::endl;
        m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Custom));
    } else {
        std::cout << "[MapEditor] Returning to MainMenu..." << std::endl;
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
    }
}

glm::vec3 MapEditorState::getIdColor(int id) const {
    switch (id) {
        case -1: return glm::vec3(1.0f, 0.85f, 0.25f); // Золотистый (Auto/Nearest)
        case 0:  return glm::vec3(0.20f, 0.85f, 1.0f); // Бирюзовый / Циан
        case 1:  return glm::vec3(1.0f, 0.55f, 0.15f); // Оранжевый / Янтарный
        case 2:  return glm::vec3(0.25f, 1.0f, 0.40f); // Лаймовый / Зеленый
        case 3:  return glm::vec3(1.0f, 0.35f, 0.85f); // Маджента / Розовый
        case 4:  return glm::vec3(0.95f, 0.30f, 0.30f); // Кораллово-красный
        default: return glm::vec3(0.70f, 0.70f, 0.95f); // Лавандовый
    }
}

void MapEditorState::init() {
    std::cout << "[MapEditor] init(): resolution=" << m_width << "x" << m_height
              << ", m_isMapsModalOpen=" << (m_isMapsModalOpen ? "true" : "false")
              << ", m_isInitialModalLaunch=" << (m_isInitialModalLaunch ? "true" : "false")
              << ", m_suppressClick=" << (m_suppressClickUntilRelease ? "true" : "false") << std::endl;
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png");

    auto wrapNoDelete = [](const std::string& name) {
        Texture2D* tex = ResourceManager::getTexture(name);
        return std::shared_ptr<Texture2D>(tex, [](Texture2D*) {});
    };

    m_uiTexture = wrapNoDelete("uiBaseTexture");
    m_arrowTexture = wrapNoDelete("arrowTexture");
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});

    loadInitialMap();
    updateButtonLayout();
}

void MapEditorState::cleanup() {
    std::cout << "[MapEditor] Cleanup completed" << std::endl;
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
    std::cout << "[MapEditor] loadLevelByName: requesting '" << fileName << "' -> file='" << m_currentLevelFileName << "'" << std::endl;
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
        m_minecarts = data.minecarts;
        m_waves = data.waves;
        m_isCampaign = data.isCampaign;
        m_tags = data.tags;
        if (!data.name.empty()) {
            m_currentLevelDisplayName = data.name;
        }
        m_suppressPlacementUntilRelease = true;
        std::cout << "[MapEditor] Successfully loaded: " << m_currentLevelFileName << " (\"" << m_currentLevelDisplayName << "\", " << m_gridWidth << "x" << m_gridHeight << ")" << std::endl;
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
        m_spawners.push_back(sp);
        m_rawLayout.assign(12, std::vector<int>(20, 0));
        m_minecarts.clear();
        m_waves.clear();
        m_isCampaign = false;
        m_tags = { "Тест" };
        m_currentLevelDisplayName = "MY MAP";
        std::cout << "[MapEditor] In-memory blank template 20x12 initialized (no disk write)" << std::endl;
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
    m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);

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

    for (const auto& sp : m_spawners) {
        if (sp.pos.x >= 0 && sp.pos.x < m_gridWidth && sp.pos.y >= 0 && sp.pos.y < m_gridHeight) {
            m_grid->setCellType(sp.pos.x, sp.pos.y, CellType::Spawner);
        }
    }

    for (const auto& b : m_bases) {
        if (b.x >= 0 && b.x < m_gridWidth && b.y >= 0 && b.y < m_gridHeight) {
            m_grid->setCellType(b.x, b.y, CellType::Base);
        }
    }

    m_grid->saveOriginalGrid();
    m_pathfinder = std::make_unique<Pathfinder>(m_gridWidth, m_gridHeight);
    recalculatePaths();

    m_isDirty = false;

    updateButtonLayout();
}

void MapEditorState::loadInitialMap() {
    loadLevelByName(m_currentLevelFileName);
}

void MapEditorState::resizeMap(int newW, int newH) {
    newW = std::clamp(newW, 1, 250);
    newH = std::clamp(newH, 1, 250);
    if (newW == m_gridWidth && newH == m_gridHeight) return;

    // Сохраняем существующие тайлы
    std::vector<std::vector<int>> newLayout(newH, std::vector<int>(newW, 0));
    for (int y = 0; y < std::min(m_gridHeight, newH); ++y) {
        for (int x = 0; x < std::min(m_gridWidth, newW); ++x) {
            newLayout[y][x] = m_rawLayout[y][x];
        }
    }
    m_rawLayout = newLayout;
    m_gridWidth = newW;
    m_gridHeight = newH;

    // Удаляем спавнеры и базы, вышедшие за границы новой сетки
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [newW, newH](const SpawnerData& s) { return s.pos.x >= newW || s.pos.y >= newH; }),
        m_spawners.end()
    );

    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [newW, newH](const BaseData& b) { return b.x >= newW || b.y >= newH; }),
        m_bases.end()
    );

    for (auto& mc : m_minecarts) {
        if (mc.start.x >= newW || mc.start.y >= newH) mc.start = glm::ivec2(-1, -1);
        if (mc.end.x >= newW || mc.end.y >= newH) mc.end = glm::ivec2(-1, -1);
    }

    m_grid = std::make_unique<Grid>(m_gridWidth, m_gridHeight, m_cellSize);
    m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);

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
    m_pathfinder = std::make_unique<Pathfinder>(m_gridWidth, m_gridHeight);
    recalculatePaths();

    m_statusMessage = "Grid resized to " + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight);
    m_statusColor = glm::vec3(0.3f, 0.9f, 1.0f);
    m_statusTimer = 2.5f;

    updateButtonLayout();
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
    if (step > 0) {
        m_selectedId = (m_selectedId >= 5) ? -1 : (m_selectedId + 1);
    } else if (step < 0) {
        m_selectedId = (m_selectedId <= -1) ? 5 : (m_selectedId - 1);
    }

    std::string idStr = (m_selectedId == -1) ? "Auto" : ("#" + std::to_string(m_selectedId));
    m_statusMessage = "Selected Active ID: " + idStr + (m_selectedId == -1 ? " (Nearest Base)" : "");
    m_statusColor = getIdColor(m_selectedId);
    m_statusTimer = 2.5f;

    updateButtonLayout();
}

float MapEditorState::getTopBarHeight() const {
    float scale = GetUIScale(m_width, m_height);
    return std::clamp(42.0f * scale, 36.0f, 56.0f);
}

float MapEditorState::getBottomDockHeight() const {
    float scale = GetUIScale(m_width, m_height);
    return std::clamp(54.0f * scale, 46.0f, 74.0f);
}

void MapEditorState::updateButtonLayout() {
    m_bottomButtons.clear();
    m_topButtons.clear();

    auto getTextW = [this](const std::string& str, float sc) -> float {
        if (m_textRenderer) return m_textRenderer->CalculateTextWidth(str, sc);
        return static_cast<float>(str.length()) * 8.5f * sc;
    };

    // 1. КНОПКИ ВЕРХНЕГО ХЕДЕРА
    float topBarH = getTopBarHeight();
    float topScale = GetUIScale(m_width, m_height);
    float topH = topBarH - 8.0f;
    float topY = 4.0f;
    float topPadding = std::clamp(5.0f * topScale, 3.0f, 8.0f);
    float topFontScale = std::clamp(0.48f * topScale, 0.38f, 0.62f);
    m_topFontScale = topFontScale;

    // Слева: заголовок "MAP EDITOR" (динамически рассчитываем X с учетом ширины надписи)
    float titleFontScale = std::clamp(0.80f * topScale, 0.60f, 0.95f);
    float titleW = getTextW("MAP EDITOR", titleFontScale);
    float leftX = 15.0f + titleW + std::clamp(18.0f * topScale, 14.0f, 26.0f);

    // btnCurrentMap: Отображение текущего имени карты
    float nameW = getTextW(m_currentLevelDisplayName, topFontScale);
    float mapBtnW = std::clamp(nameW + 20.0f, 100.0f, 200.0f);
    EditorButton btnCurrentMap;
    btnCurrentMap.pos = glm::vec2(leftX, topY);
    btnCurrentMap.size = glm::vec2(mapBtnW, topH);
    btnCurrentMap.label = m_currentLevelDisplayName;
    btnCurrentMap.isAction = true;
    btnCurrentMap.actionId = 16; // Открыть список карт
    m_topButtons.push_back(btnCurrentMap);
    leftX += mapBtnW + topPadding;

    // btnRename
    std::string renLabel = LOC("LEVEL_BTN_RENAME");
    float renW = std::max(52.0f, getTextW(renLabel, topFontScale) + 16.0f);
    EditorButton btnRename;
    btnRename.pos = glm::vec2(leftX, topY);
    btnRename.size = glm::vec2(renW, topH);
    btnRename.label = renLabel;
    btnRename.isAction = true;
    btnRename.actionId = 14; // Переименовать
    m_topButtons.push_back(btnRename);
    leftX += renW + topPadding;

    // btnType
    std::string typeLabel = m_isCampaign ? LOC("EDITOR_TYPE_CAMPAIGN") : LOC("EDITOR_TYPE_TEST");
    float typeW = std::max(80.0f, getTextW(typeLabel, topFontScale) + 18.0f);
    EditorButton btnType;
    btnType.pos = glm::vec2(leftX, topY);
    btnType.size = glm::vec2(typeW, topH);
    btnType.label = typeLabel;
    btnType.isAction = true;
    btnType.actionId = 17; // Переключить Кампания/Тест
    m_topButtons.push_back(btnType);
    leftX += typeW + topPadding;

    // btnNewMap
    std::string newLabel = LOC("EDITOR_NEW_MAP");
    float newW = std::max(70.0f, getTextW(newLabel, topFontScale) + 18.0f);
    EditorButton btnNewMap;
    btnNewMap.pos = glm::vec2(leftX, topY);
    btnNewMap.size = glm::vec2(newW, topH);
    btnNewMap.label = newLabel;
    btnNewMap.isAction = true;
    btnNewMap.actionId = 15; // Создать новую карту
    m_topButtons.push_back(btnNewMap);
    leftX += newW + topPadding;

    // btnMaps
    std::string mapsLabel = LOC("EDITOR_MAPS_LIST");
    float mapsW = std::max(60.0f, getTextW(mapsLabel, topFontScale) + 18.0f);
    EditorButton btnMaps;
    btnMaps.pos = glm::vec2(leftX, topY);
    btnMaps.size = glm::vec2(mapsW, topH);
    btnMaps.label = mapsLabel;
    btnMaps.isAction = true;
    btnMaps.actionId = 16; // Список карт
    m_topButtons.push_back(btnMaps);
    leftX += mapsW + topPadding;

    m_topBarLeftEndX = leftX;

    // Справа: Выбор размера карты (динамически рассчитываем от правого края с запасом 12px)
    float presetW = std::clamp(80.0f * topScale, 70.0f, 100.0f);
    float decIncW = std::clamp(30.0f * topScale, 26.0f, 38.0f);
    float rightGroupW = presetW + topPadding + (decIncW + topPadding) * 4.0f;
    float currentTopX = static_cast<float>(m_width) - rightGroupW - 12.0f;

    EditorButton btnPreset;
    btnPreset.pos = glm::vec2(currentTopX, topY);
    btnPreset.size = glm::vec2(presetW, topH);
    btnPreset.label = std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight);
    btnPreset.isAction = true;
    btnPreset.actionId = 8; // Preset cycle
    m_topButtons.push_back(btnPreset);
    currentTopX += presetW + topPadding;

    EditorButton btnWDec;
    btnWDec.pos = glm::vec2(currentTopX, topY);
    btnWDec.size = glm::vec2(decIncW, topH);
    btnWDec.label = "W-";
    btnWDec.isAction = true;
    btnWDec.actionId = 9; // W-
    m_topButtons.push_back(btnWDec);
    currentTopX += decIncW + topPadding;

    EditorButton btnWInc;
    btnWInc.pos = glm::vec2(currentTopX, topY);
    btnWInc.size = glm::vec2(decIncW, topH);
    btnWInc.label = "W+";
    btnWInc.isAction = true;
    btnWInc.actionId = 10; // W+
    m_topButtons.push_back(btnWInc);
    currentTopX += decIncW + topPadding;

    EditorButton btnHDec;
    btnHDec.pos = glm::vec2(currentTopX, topY);
    btnHDec.size = glm::vec2(decIncW, topH);
    btnHDec.label = "H-";
    btnHDec.isAction = true;
    btnHDec.actionId = 11; // H-
    m_topButtons.push_back(btnHDec);
    currentTopX += decIncW + topPadding;

    EditorButton btnHInc;
    btnHInc.pos = glm::vec2(currentTopX, topY);
    btnHInc.size = glm::vec2(decIncW, topH);
    btnHInc.label = "H+";
    btnHInc.isAction = true;
    btnHInc.actionId = 12; // H+
    m_topButtons.push_back(btnHInc);

    // 2. КНОПКИ НИЖНЕГО ТУЛБАРА
    float scale = GetUIScale(m_width, m_height);
    float dockH = getBottomDockHeight();
    float btnH = dockH - 12.0f;
    float bottomY = static_cast<float>(m_height) - dockH + 6.0f;
    float startX = 10.0f;
    float padding = std::clamp(4.0f * scale, 2.0f, 6.0f);

    float bottomFontScale = std::clamp(0.46f * scale, 0.36f, 0.60f);
    m_bottomFontScale = bottomFontScale;

    struct BottomItem {
        std::string label;
        bool isAction;
        EditorBrush brush;
        int actionId;
        float width;
    };

    std::vector<BottomItem> items;
    // Кисти (1-8, Депо, Тупик, Ластик 0)
    items.push_back({ "1:" + LOC("EDITOR_GROUND"),     false, EditorBrush::Ground,    0, 0.0f });
    items.push_back({ "2:" + LOC("EDITOR_WALL"),       false, EditorBrush::Wall,      0, 0.0f });
    items.push_back({ "3:" + LOC("EDITOR_PLATFORM"),   false, EditorBrush::Platform,  0, 0.0f });
    items.push_back({ "4:" + LOC("EDITOR_PATH"),       false, EditorBrush::Path,      0, 0.0f });
    items.push_back({ "5:" + LOC("EDITOR_SPAWNER"),    false, EditorBrush::Spawner,   0, 0.0f });
    items.push_back({ "6:" + LOC("EDITOR_BASE"),       false, EditorBrush::Base,      0, 0.0f });
    items.push_back({ "7:" + LOC("EDITOR_CHASM"),      false, EditorBrush::Chasm,     0, 0.0f });
    items.push_back({ "8:" + LOC("EDITOR_RAIL"),       false, EditorBrush::Rail,      0, 0.0f });
    items.push_back({ LOC("EDITOR_RAIL_START"),        false, EditorBrush::RailStart, 0, 0.0f });
    items.push_back({ LOC("EDITOR_RAIL_END"),          false, EditorBrush::RailEnd,   0, 0.0f });
    items.push_back({ "0:" + LOC("EDITOR_ERASER"),     false, EditorBrush::Eraser,    0, 0.0f });

    // ID блок
    items.push_back({ "[-]", true, EditorBrush::Wall, 5, 0.0f });
    std::string idText = (m_selectedId == -1) ? "ID:Auto" : ("ID:#" + std::to_string(m_selectedId));
    items.push_back({ idText, true, EditorBrush::Wall, 6, 0.0f });
    items.push_back({ "[+]", true, EditorBrush::Wall, 7, 0.0f });

    // Действия
    items.push_back({ LOC("EDITOR_WAVES"),          true, EditorBrush::Wall, 13, 0.0f });
    items.push_back({ LOC("EDITOR_MINECART_BTN"),   true, EditorBrush::Wall, 18, 0.0f });
    items.push_back({ LOC("EDITOR_SAVE"),  true, EditorBrush::Wall, 1,  0.0f });
    items.push_back({ LOC("EDITOR_TEST"),  true, EditorBrush::Wall, 2,  0.0f });
    items.push_back({ LOC("EDITOR_CLEAR"), true, EditorBrush::Wall, 3,  0.0f });
    items.push_back({ LOC("EDITOR_EXIT"),  true, EditorBrush::Wall, 4,  0.0f });

    // Вычисляем ширину для каждого элемента с запасом по краям
    float totalWidth = startX;
    for (size_t i = 0; i < items.size(); ++i) {
        auto& it = items[i];
        float tw = getTextW(it.label, bottomFontScale);
        if (it.actionId == 5 || it.actionId == 7) {
            it.width = 28.0f;
        } else if (it.actionId == 6) {
            it.width = std::max(68.0f, tw + 18.0f);
        } else {
            it.width = std::max(56.0f, tw + 18.0f);
        }
        totalWidth += it.width + padding;
        if (i == 10 || i == 13) totalWidth += 6.0f; // Разделители после Ластика и после ID блока
    }

    // Если всё вместе шире экрана (например, на маленьком разрешении 1024x768), пропорционально уменьшаем
    float maxAvailW = static_cast<float>(m_width) - 20.0f;
    if (totalWidth > maxAvailW && totalWidth > 0.0f) {
        float factor = maxAvailW / totalWidth;
        bottomFontScale = std::max(0.36f, bottomFontScale * factor);
        m_bottomFontScale = bottomFontScale;
        padding = std::max(2.0f, padding * factor);
        for (auto& it : items) {
            float tw = getTextW(it.label, bottomFontScale);
            if (it.actionId == 5 || it.actionId == 7) {
                it.width = std::max(22.0f, 28.0f * factor);
            } else if (it.actionId == 6) {
                it.width = std::max(50.0f, tw + 10.0f);
            } else {
                it.width = std::max(44.0f, tw + 10.0f);
            }
        }
    }

    // Размещаем кнопки
    float currentX = startX;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i == 11 || i == 14) {
            currentX += 6.0f; // Визуальный разделитель
        }
        const auto& it = items[i];
        EditorButton btn;
        btn.pos = glm::vec2(currentX, bottomY);
        btn.size = glm::vec2(it.width, btnH);
        btn.label = it.label;
        btn.isAction = it.isAction;
        btn.brush = it.brush;
        btn.actionId = it.actionId;
        m_bottomButtons.push_back(btn);
        currentX += it.width + padding;
    }
}

bool MapEditorState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void MapEditorState::recalculatePaths() {
    if (!m_grid || !m_pathfinder) return;

    m_activePaths = PathService::calculateAllPaths(*m_grid, *m_pathfinder, m_spawners, m_bases);

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
    // Перераховуємо маршрут рейок після будь-якої зміни сітки
    recalculateRailPath();
}

// ---------------------------------------------------------------------------
// BFS-валідація маршруту рейок: від m_minecarts[0].start до .end
// ---------------------------------------------------------------------------
void MapEditorState::recalculateRailPath() {
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

    if (!found) return;

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
    if (gridX < 0 || gridX >= m_gridWidth || gridY < 0 || gridY >= m_gridHeight) return;

    // Режим ластика: очищает клетку до чистой Земли
    if (brush == EditorBrush::Eraser) {
        eraseCell(gridX, gridY);
        return;
    }

    // Особый случай: клик по УЖЕ существующему спавнеру
    if (brush == EditorBrush::Spawner) {
        for (auto& sp : m_spawners) {
            if (sp.pos.x == gridX && sp.pos.y == gridY) {
                if (sp.targetBaseIndex == m_selectedId) {
                    sp.targetBaseIndex = (sp.targetBaseIndex >= 5) ? -1 : (sp.targetBaseIndex + 1);
                } else {
                    sp.targetBaseIndex = m_selectedId;
                }
                std::string idStr = (sp.targetBaseIndex == -1) ? "Auto" : ("#" + std::to_string(sp.targetBaseIndex));
                m_statusMessage = "Spawner at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") target Base ID set to: " + idStr;
                m_statusColor = getIdColor(sp.targetBaseIndex);
                m_statusTimer = 2.5f;
                recalculatePaths();
                return;
            }
        }
    }

    // Особый случай: клик по УЖЕ существующей базе
    if (brush == EditorBrush::Base) {
        int baseTargetId = (m_selectedId < 0) ? 0 : m_selectedId;
        for (auto& b : m_bases) {
            if (b.x == gridX && b.y == gridY) {
                if (b.id == baseTargetId) {
                    b.id = (b.id >= 5) ? 0 : (b.id + 1);
                } else {
                    b.id = baseTargetId;
                }
                m_statusMessage = "Base at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") ID set to: #" + std::to_string(b.id);
                m_statusColor = getIdColor(b.id);
                m_statusTimer = 2.5f;
                recalculatePaths();
                return;
            }
        }
    }

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

    switch (brush) {
        case EditorBrush::Ground:
            m_rawLayout[gridY][gridX] = 0;
            m_grid->setCellType(gridX, gridY, CellType::Ground);
            break;
        case EditorBrush::Wall:
            m_rawLayout[gridY][gridX] = 3;
            m_grid->setCellType(gridX, gridY, CellType::Scenery);
            break;
        case EditorBrush::Platform:
            m_rawLayout[gridY][gridX] = 2;
            m_grid->setCellType(gridX, gridY, CellType::Platform);
            break;
        case EditorBrush::Path:
            m_rawLayout[gridY][gridX] = 1;
            m_grid->setCellType(gridX, gridY, CellType::Path);
            break;
        case EditorBrush::Chasm:
            m_rawLayout[gridY][gridX] = 4;
            m_grid->setCellType(gridX, gridY, CellType::Chasm);
            break;
        case EditorBrush::Rail:
            m_rawLayout[gridY][gridX] = 5;
            m_grid->setCellType(gridX, gridY, CellType::Rail);
            break;
        case EditorBrush::RailStart:
            if (m_minecarts.empty()) m_minecarts.push_back(MinecartData{});
            m_minecarts[0].start = glm::ivec2(gridX, gridY);
            m_rawLayout[gridY][gridX] = 5;
            m_grid->setCellType(gridX, gridY, CellType::Rail);
            m_statusMessage = "Minecart Depot [Start] set at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
            m_statusColor = glm::vec3(1.0f, 0.75f, 0.2f);
            m_statusTimer = 2.0f;
            break;
        case EditorBrush::RailEnd:
            if (m_minecarts.empty()) m_minecarts.push_back(MinecartData{});
            m_minecarts[0].end = glm::ivec2(gridX, gridY);
            m_rawLayout[gridY][gridX] = 5;
            m_grid->setCellType(gridX, gridY, CellType::Rail);
            m_statusMessage = "Minecart Buffer [End] set at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
            m_statusColor = glm::vec3(1.0f, 0.35f, 0.35f);
            m_statusTimer = 2.0f;
            break;
        case EditorBrush::Spawner:
            m_rawLayout[gridY][gridX] = 0;
            m_grid->setCellType(gridX, gridY, CellType::Spawner);
            m_spawners.push_back({ glm::ivec2(gridX, gridY), m_selectedId });
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
                m_rawLayout[gridY][gridX] = 0;
                m_grid->setCellType(gridX, gridY, CellType::Base);
                m_bases.push_back(BaseData(gridX, gridY, baseId));
                m_statusMessage = "Placed Base with ID: #" + std::to_string(baseId);
                m_statusColor = getIdColor(baseId);
                m_statusTimer = 2.0f;
            }
            break;
        default:
            break;
    }

    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_isDirty = true;
}

void MapEditorState::eraseCell(int gridX, int gridY) {
    if (gridX < 0 || gridX >= m_gridWidth || gridY < 0 || gridY >= m_gridHeight) return;

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

    m_rawLayout[gridY][gridX] = 0;
    m_grid->setCellType(gridX, gridY, CellType::Ground);
    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_isDirty = true;
}

void MapEditorState::saveMap() {
    LevelMapData data;
    data.name = m_currentLevelDisplayName;
    data.isCampaign = m_isCampaign;
    data.tags = m_tags;
    data.gridWidth = m_gridWidth;
    data.gridHeight = m_gridHeight;
    data.cellSize = m_grid->getCellSize();
    data.offsetX = m_grid->getOffset().x;
    data.offsetY = m_grid->getOffset().y;
    data.spawners = m_spawners;
    data.bases = m_bases;
    data.layout = m_rawLayout;

    std::vector<MinecartData> activeCarts;
    for (const auto& mc : m_minecarts) {
        if (mc.hasStart() || mc.hasEnd()) {
            activeCarts.push_back(mc);
        }
    }
    data.minecarts = activeCarts;
    data.waves = m_waves;

    bool ok = LevelManager::saveLevel(m_currentLevelFileName, data);
    std::cout << "[MapEditor] saveMap: file=" << m_currentLevelFileName << ", name=" << m_currentLevelDisplayName << ", size=" << m_gridWidth << "x" << m_gridHeight << ", result=" << (ok ? "SUCCESS" : "FAIL") << std::endl;

    if (ok) {
        m_isDirty = false;
        m_statusMessage = "Map saved: " + m_currentLevelDisplayName + " (" + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight) + ")!";
        m_statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
    } else {
        m_statusMessage = "Error saving map file!";
        m_statusColor = glm::vec3(1.0f, 0.3f, 0.3f);
    }
    m_statusTimer = 3.5f;
}

void MapEditorState::clearMap() {
    m_spawners.clear();
    m_bases.clear();
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
    m_statusMessage = "Map cleared to Ground.";
    m_statusColor = glm::vec3(0.9f, 0.9f, 0.3f);
    m_statusTimer = 2.0f;
}

void MapEditorState::testMap() {
    if (m_spawners.empty() || m_bases.empty()) {
        m_statusMessage = "Cannot test: Place at least 1 Spawner and 1 Base!";
        m_statusColor = glm::vec3(1.0f, 0.3f, 0.3f);
        m_statusTimer = 4.0f;
        return;
    }

    if (m_hasInvalidSpawner) {
        m_statusMessage = "Cannot test: One or more Spawners have no path to a Base!";
        m_statusColor = glm::vec3(1.0f, 0.2f, 0.2f);
        m_statusTimer = 4.0f;
        return;
    }

    saveMap();
    std::string testPath = "res/levels/" + m_currentLevelFileName;
    std::cout << "[MapEditor] Launching test gameplay with " << testPath << std::endl;
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

    if (m_diagInputFrames < 6) {
        std::cout << "[MapEditor] processInput frame " << m_diagInputFrames
                  << ": leftDown=" << leftDown << ", rightDown=" << rightDown
                  << ", m_isLeftMouseDown=" << m_isLeftMouseDown
                  << ", m_suppressClick=" << m_suppressClickUntilRelease
                  << ", m_isMapsModalOpen=" << (m_isMapsModalOpen ? "true" : "false")
                  << ", mousePos=(" << mousePos.x << "," << mousePos.y << ")" << std::endl;
        m_diagInputFrames++;
    }

    // Подавляем клики от предыдущего состояния до полного отпускания кнопок мыши
    if (m_suppressClickUntilRelease) {
        if (!leftDown && !rightDown) {
            std::cout << "[MapEditor] Mouse released -> m_suppressClickUntilRelease cleared" << std::endl;
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
    if (m_isExitModalOpen) {
        bool handled = processExitModalInput(window, mousePos, leftDown, dt);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Если открыто модальное окно переименования
    if (m_isRenameModalOpen) {
        bool handled = processRenameModalInput(window, mousePos, leftDown, dt);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Если открыто модальное окно списка карт
    if (m_isMapsModalOpen) {
        bool handled = processMapsModalInput(window, mousePos, leftDown, dt);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Если открыто модальное окно настроек вагонетки
    if (m_isMinecartSettingsOpen) {
        bool handled = processMinecartSettingsInput(window, mousePos, leftDown);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Горячая клавиша Waves (W)
    bool keyW = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
    if (keyW && !m_keyWPressedLastFrame) {
        m_isWaveEditorOpen = !m_isWaveEditorOpen;
        if (m_isWaveEditorOpen) {
            m_statusMessage = "Wave Editor: Configure waves, sub-waves and pacing.";
            m_statusColor = glm::vec3(0.3f, 0.85f, 1.0f);
            m_statusTimer = 2.0f;
        }
    }
    m_keyWPressedLastFrame = keyW;

    // Горячая клавиша Escape
    bool keyEsc = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    if (keyEsc && !m_keyEscPressedLastFrame) {
        if (m_isExitModalOpen) {
            closeExitModal();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_isRenameModalOpen) {
            m_isRenameModalOpen = false;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_isMapsModalOpen) {
            std::cout << "[MapEditor] Maps modal closed by global ESC, m_isInitialModalLaunch=" << m_isInitialModalLaunch << std::endl;
            m_isMapsModalOpen = false;
            m_keyEscPressedLastFrame = keyEsc;
            if (m_isInitialModalLaunch) {
                returnToOrigin();
            }
            return;
        }
        if (m_isWaveEditorOpen) {
            if (m_openDropdownPartIdx >= 0) {
                m_openDropdownPartIdx = -1;
            } else if (m_focusedField != FocusedField::None) {
                commitFocusedInput();
            } else {
                m_isWaveEditorOpen = false;
            }
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_isMinecartSettingsOpen) {
            m_isMinecartSettingsOpen = false;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        openExitModal();
        m_keyEscPressedLastFrame = keyEsc;
        return;
    }
    m_keyEscPressedLastFrame = keyEsc;

    // Если открыт редактор волн, перехватываем ввод
    if (m_isWaveEditorOpen) {
        bool handled = processWaveEditorInput(window, mousePos, leftDown, dt);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Горячие клавиши 1-8 + 0/E для Ластика
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) m_currentBrush = EditorBrush::Ground;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) m_currentBrush = EditorBrush::Wall;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) m_currentBrush = EditorBrush::Platform;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) m_currentBrush = EditorBrush::Path;
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) m_currentBrush = EditorBrush::Spawner;
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) m_currentBrush = EditorBrush::Base;
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) m_currentBrush = EditorBrush::Chasm;
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) m_currentBrush = EditorBrush::Rail;
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_currentBrush = EditorBrush::Eraser;

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

    // Горячая клавиша Save (S)
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

    // Горячая клавиша Clear (C)
    bool keyC = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
    if (keyC && !m_keyCPressedLastFrame) {
        clearMap();
    }
    m_keyCPressedLastFrame = keyC;

    bool clickedUI = false;

    // 1. Проверка клика по кнопкам верхнего хедера
    if (leftDown && !m_isLeftMouseDown) {
        for (const auto& btn : m_topButtons) {
            if (isPointInRect(mousePos, btn.pos, btn.size)) {
                clickedUI = true;
                bool isShift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
                int step = isShift ? 5 : 1;
                if (btn.actionId == 8) cycleMapSizePreset();
                else if (btn.actionId == 9) resizeMap(m_gridWidth - step, m_gridHeight);
                else if (btn.actionId == 10) resizeMap(m_gridWidth + step, m_gridHeight);
                else if (btn.actionId == 11) resizeMap(m_gridWidth, m_gridHeight - step);
                else if (btn.actionId == 12) resizeMap(m_gridWidth, m_gridHeight + step);
                else if (btn.actionId == 14) openRenameModal();
                else if (btn.actionId == 15) {
                    std::string newF = LevelManager::createNewLevel("custom_map");
                    loadLevelByName(newF);
                    m_suppressPlacementUntilRelease = true;
                    m_suppressClickUntilRelease = true;
                    m_isLeftMouseDown = true;
                    m_statusMessage = "Created map: " + m_currentLevelDisplayName;
                    m_statusColor = glm::vec3(0.2f, 1.0f, 0.4f);
                    m_statusTimer = 3.5f;
                }
                else if (btn.actionId == 16) {
                    m_isMapsModalOpen = true;
                    m_mapsScrollOffset = 0;
                    m_suppressPlacementUntilRelease = true;
                    m_suppressClickUntilRelease = true;
                }
                else if (btn.actionId == 17) {
                    m_isCampaign = !m_isCampaign;
                    LevelManager::setLevelCampaign(m_currentLevelFileName, m_isCampaign);
                    m_statusMessage = m_isCampaign ? "Map type: CAMPAIGN" : "Map type: TEST";
                    m_statusColor = m_isCampaign ? glm::vec3(0.3f, 1.0f, 0.4f) : glm::vec3(1.0f, 0.7f, 0.2f);
                    m_statusTimer = 2.5f;
                    updateButtonLayout();
                }
                m_suppressPlacementUntilRelease = true;
                break;
            }
        }
    }

    // 2. Проверка клика по кнопкам нижнего тулбара
    if (!clickedUI && leftDown && !m_isLeftMouseDown) {
        for (const auto& btn : m_bottomButtons) {
            if (isPointInRect(mousePos, btn.pos, btn.size)) {
                clickedUI = true;
                m_suppressPlacementUntilRelease = true;
                if (!btn.isAction) {
                    m_currentBrush = btn.brush;
                } else {
                    if (btn.actionId == 1) saveMap();
                    else if (btn.actionId == 2) { testMap(); return; }
                    else if (btn.actionId == 3) clearMap();
                    else if (btn.actionId == 4) {
                        openExitModal();
                        m_isLeftMouseDown = leftDown;
                        return;
                    }
                    else if (btn.actionId == 5) cycleSelectedId(-1);
                    else if (btn.actionId == 6) cycleSelectedId(+1);
                    else if (btn.actionId == 7) cycleSelectedId(+1);
                    else if (btn.actionId == 13) {
                        m_isWaveEditorOpen = !m_isWaveEditorOpen;
                    }
                    else if (btn.actionId == 18) {
                        m_isMinecartSettingsOpen = !m_isMinecartSettingsOpen;
                    }
                }
                break;
            }
        }
    }

    // 3. Если кликаем по рабочей области сетки
    if (m_suppressPlacementUntilRelease) {
        if (!leftDown && !rightDown) {
            m_suppressPlacementUntilRelease = false;
        }
    } else {
        float topBarH = getTopBarHeight();
        float dockH = getBottomDockHeight();
        if (!clickedUI && mousePos.y >= topBarH && mousePos.y < static_cast<float>(m_height) - dockH) {
            if (m_grid) {
                glm::ivec2 gridPos = m_grid->pixelToGrid(mousePos);
                if (gridPos.x >= 0 && gridPos.x < m_gridWidth && gridPos.y >= 0 && gridPos.y < m_gridHeight) {
                    bool isSinglePointBrush = (m_currentBrush == EditorBrush::Spawner || m_currentBrush == EditorBrush::Base ||
                                               m_currentBrush == EditorBrush::RailStart || m_currentBrush == EditorBrush::RailEnd);
                    if (leftDown) {
                        if (!isSinglePointBrush || !m_isLeftMouseDown) {
                            applyBrush(gridPos.x, gridPos.y, m_currentBrush);
                        }
                    } else if (rightDown) {
                        eraseCell(gridPos.x, gridPos.y);
                    }
                }
            }
        }
    }

    m_isLeftMouseDown = leftDown;
    m_isRightMouseDown = rightDown;
}

void MapEditorState::update(float dt) {
    if (m_grid) {
        m_pathVisualizer.update(dt, m_grid->getCellSize());
    }

    if (m_isWaveEditorOpen) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= 1.0f) {
            m_cursorBlinkTimer -= 1.0f;
        }
    }

    if (m_statusTimer > 0.0f) {
        m_statusTimer -= dt;
        if (m_statusTimer <= 0.0f) {
            m_statusMessage = "";
            m_statusColor = glm::vec3(0.9f, 0.9f, 0.9f);
        }
    }
}

void MapEditorState::render() {
    m_renderer->beginBatch();

    // 1. Темный космический фон рабочей зоны
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec3(0.09f, 0.10f, 0.13f));

    // 2. Игровая сетка (отцентрирована строго между верхним и нижним барами)
    if (m_grid) {
        m_grid->draw(m_renderer.get(), m_whiteTexture, glm::vec3(1.0f));
    }

    // 3. Стрелочки путей в реальном времени с цветом, соответствующим целевой базе!
    if (m_grid && m_arrowTexture) {
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
        for (const auto& b : m_bases) {
            glm::vec2 pPos = m_grid->gridToPixel(b.x, b.y);
            glm::vec3 bCol = getIdColor(b.id);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, bCol);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, bCol * 0.85f);
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
    }

    // 5. ВЕРХНИЙ ХЕДЕР-БАР (Глухой темный фон, текст никогда не наезжает на сетку!)
    float topBarH = getTopBarHeight();
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, 0.0f), glm::vec2(m_width, topBarH), 0.0f, glm::vec3(0.07f, 0.08f, 0.10f));
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, topBarH - 2.0f), glm::vec2(m_width, 2.0f), 0.0f, glm::vec3(0.25f, 0.27f, 0.32f));

    // Отрисовка кнопок верхнего хедера
    for (const auto& btn : m_topButtons) {
        glm::vec3 btnBg = glm::vec3(0.16f, 0.18f, 0.23f);
        glm::vec3 borderColor = glm::vec3(0.32f, 0.36f, 0.44f);
        if (btn.actionId == 8) {
            borderColor = glm::vec3(0.3f, 0.8f, 1.0f);
        } else if (btn.actionId == 14) { // Rename
            borderColor = glm::vec3(1.0f, 0.85f, 0.3f);
            btnBg = glm::vec3(0.22f, 0.20f, 0.12f);
        } else if (btn.actionId == 15) { // +New
            borderColor = glm::vec3(0.3f, 0.9f, 0.4f);
            btnBg = glm::vec3(0.12f, 0.22f, 0.14f);
        } else if (btn.actionId == 16) { // Maps / MapName
            borderColor = glm::vec3(0.4f, 0.7f, 1.0f);
            btnBg = glm::vec3(0.14f, 0.20f, 0.28f);
        }
        m_renderer->drawSprite(m_whiteTexture, btn.pos, btn.size, 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, btn.pos + glm::vec2(2.0f), btn.size - glm::vec2(4.0f), 0.0f, btnBg);
    }

    // 6. НИЖНЯЯ ПАНЕЛЬ ТУЛБАРА (Dock)
    float dockH = getBottomDockHeight();
    float dockY = static_cast<float>(m_height) - dockH;
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, dockY), glm::vec2(m_width, dockH), 0.0f, glm::vec3(0.07f, 0.08f, 0.10f));
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, dockY), glm::vec2(m_width, 2.0f), 0.0f, glm::vec3(0.25f, 0.27f, 0.32f));

    // Отрисовка кнопок нижнего тулбара
    for (const auto& btn : m_bottomButtons) {
        glm::vec3 btnBg = glm::vec3(0.18f, 0.20f, 0.25f);
        glm::vec3 borderColor = glm::vec3(0.35f, 0.38f, 0.45f);

        if (!btn.isAction && btn.brush == m_currentBrush) {
            btnBg = glm::vec3(0.25f, 0.35f, 0.50f);
            borderColor = glm::vec3(1.0f, 0.85f, 0.2f); // Золотая окантовка выбранной кисти
            if (btn.brush == EditorBrush::Eraser) {
                btnBg = glm::vec3(0.45f, 0.18f, 0.18f);
                borderColor = glm::vec3(1.0f, 0.35f, 0.35f);
            } else if (btn.brush == EditorBrush::Chasm) {
                btnBg = glm::vec3(0.12f, 0.12f, 0.18f);
                borderColor = glm::vec3(0.70f, 0.45f, 0.95f);
            } else if (btn.brush == EditorBrush::Rail) {
                btnBg = glm::vec3(0.24f, 0.25f, 0.30f);
                borderColor = glm::vec3(1.0f, 0.85f, 0.2f);
            } else if (btn.brush == EditorBrush::RailStart) {
                btnBg = glm::vec3(0.38f, 0.26f, 0.12f);
                borderColor = glm::vec3(1.0f, 0.75f, 0.2f);
            } else if (btn.brush == EditorBrush::RailEnd) {
                btnBg = glm::vec3(0.40f, 0.15f, 0.15f);
                borderColor = glm::vec3(1.0f, 0.40f, 0.40f);
            }
        } else if (!btn.isAction && btn.brush == EditorBrush::Eraser) {
            btnBg = glm::vec3(0.28f, 0.15f, 0.15f);
            borderColor = glm::vec3(0.6f, 0.25f, 0.25f);
        } else if (!btn.isAction && btn.brush == EditorBrush::Chasm) {
            btnBg = glm::vec3(0.10f, 0.10f, 0.14f);
            borderColor = glm::vec3(0.35f, 0.30f, 0.45f);
        } else if (!btn.isAction && btn.brush == EditorBrush::Rail) {
            btnBg = glm::vec3(0.16f, 0.16f, 0.20f);
            borderColor = glm::vec3(0.35f, 0.38f, 0.44f);
        } else if (!btn.isAction && btn.brush == EditorBrush::RailStart) {
            btnBg = glm::vec3(0.22f, 0.16f, 0.10f);
            borderColor = glm::vec3(0.55f, 0.40f, 0.20f);
        } else if (!btn.isAction && btn.brush == EditorBrush::RailEnd) {
            btnBg = glm::vec3(0.24f, 0.12f, 0.12f);
            borderColor = glm::vec3(0.55f, 0.25f, 0.25f);
        } else if (btn.isAction) {
            if (btn.actionId == 2) { // Test
                btnBg = glm::vec3(0.15f, 0.35f, 0.20f);
                borderColor = glm::vec3(0.25f, 0.75f, 0.35f);
            } else if (btn.actionId == 1) { // Save
                btnBg = glm::vec3(0.15f, 0.25f, 0.38f);
                borderColor = glm::vec3(0.3f, 0.6f, 0.9f);
            } else if (btn.actionId == 4) { // Exit
                btnBg = glm::vec3(0.35f, 0.15f, 0.15f);
                borderColor = glm::vec3(0.7f, 0.3f, 0.3f);
            } else if (btn.actionId == 6) { // ID Display
                btnBg = getIdColor(m_selectedId) * 0.3f;
                borderColor = getIdColor(m_selectedId);
            } else if (btn.actionId == 13) { // Waves
                if (m_isWaveEditorOpen) {
                    btnBg = glm::vec3(0.20f, 0.40f, 0.65f);
                    borderColor = glm::vec3(0.35f, 0.85f, 1.0f);
                } else {
                    btnBg = glm::vec3(0.18f, 0.28f, 0.45f);
                    borderColor = glm::vec3(0.30f, 0.50f, 0.80f);
                }
            } else if (btn.actionId == 18) { // Minecart Settings
                if (m_isMinecartSettingsOpen) {
                    btnBg = glm::vec3(0.45f, 0.32f, 0.10f);
                    borderColor = glm::vec3(1.0f, 0.80f, 0.20f);
                } else {
                    btnBg = glm::vec3(0.26f, 0.20f, 0.09f);
                    borderColor = glm::vec3(0.70f, 0.55f, 0.20f);
                }
            }
        }

        m_renderer->drawSprite(m_whiteTexture, btn.pos, btn.size, 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, btn.pos + glm::vec2(2.0f), btn.size - glm::vec2(4.0f), 0.0f, btnBg);
    }

    m_renderer->flush();

    // 7. ТЕКСТ (Хедер, кнопки, клетки)
    if (m_textRenderer) {
        float scale = GetUIScale(m_width, m_height);
        // Текст верхнего хедера
        float titleFontScale = std::clamp(0.80f * scale, 0.60f, 0.95f);
        float titleY = (topBarH - titleFontScale * 28.0f) * 0.5f;
        m_textRenderer->RenderText("MAP EDITOR", 15.0f, titleY, titleFontScale, glm::vec3(1.0f, 0.85f, 0.2f));

        // Текст на кнопках верхнего бара (каноническое центрирование по td-ui)
        for (const auto& btn : m_topButtons) {
            glm::vec3 textColor = (btn.actionId == 8) ? glm::vec3(0.4f, 0.9f, 1.0f) :
                                  (btn.actionId == 14) ? glm::vec3(1.0f, 0.9f, 0.35f) :
                                  (btn.actionId == 15) ? glm::vec3(0.4f, 1.0f, 0.5f) :
                                  (btn.actionId == 16) ? glm::vec3(0.7f, 0.9f, 1.0f) : glm::vec3(0.9f);
            float tw = m_textRenderer->CalculateTextWidth(btn.label, m_topFontScale);
            float tx = btn.pos.x + (btn.size.x - tw) * 0.5f;
            float ty = btn.pos.y + (btn.size.y - m_topFontScale * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText(btn.label, tx, ty, m_topFontScale, textColor);
        }

        // Статус / предупреждения строго между кнопками слева и размерными кнопками справа
        float rightControlsX = m_topButtons.empty() ? static_cast<float>(m_width) - 260.0f : m_topButtons.back().pos.x - 170.0f;
        float statusStartX = m_topBarLeftEndX + 16.0f;
        float maxStatusWidth = (rightControlsX - 16.0f) - statusStartX;

        if (maxStatusWidth > 60.0f) {
            std::string statusText = "";
            glm::vec3 curColor = m_statusColor;

            if (m_hasInvalidSpawner) {
                statusText = "[!] SPAWNER BLOCKED (NO PATH)";
                curColor = glm::vec3(1.0f, 0.25f, 0.25f);
            } else if (m_missingBaseWarning) {
                statusText = "[i] Base #" + std::to_string(m_missingBaseId) + " missing (using nearest)";
                curColor = glm::vec3(1.0f, 0.75f, 0.2f);
            } else if (!m_minecarts.empty() && m_minecarts[0].hasStart() && m_minecarts[0].hasEnd()) {
                // Обидва маркери рейок задані — показуємо стан маршруту
                if (m_isRailPathValid) {
                    statusText = "[+] РЕЙКИ: ОК (" + std::to_string(static_cast<int>(m_railPath.size())) + " кл.)";
                    curColor = glm::vec3(0.25f, 1.0f, 0.55f);
                } else {
                    statusText = "[!] РЕЙКИ: РОЗІРВАНІ";
                    curColor = glm::vec3(1.0f, 0.28f, 0.28f);
                }
            } else if (m_statusTimer > 0.0f && !m_statusMessage.empty()) {
                statusText = m_statusMessage;
                curColor = m_statusColor;
            }

            if (!statusText.empty()) {
                float sScale = std::clamp(0.48f * scale, 0.36f, 0.55f);
                float sw = m_textRenderer->CalculateTextWidth(statusText, sScale);
                if (sw > maxStatusWidth) {
                    sScale = std::max(0.32f, sScale * (maxStatusWidth / sw));
                }
                float statusY = (topBarH - sScale * 28.0f) * 0.5f + 1.0f;
                m_textRenderer->RenderText(statusText, statusStartX, statusY, sScale, curColor);
            }
        }

        // Текст на кнопках нижнего бара (центрирован по горизонтали и вертикали по формуле td-ui)
        for (const auto& btn : m_bottomButtons) {
            glm::vec3 textColor = glm::vec3(0.95f);
            if (!btn.isAction && btn.brush == m_currentBrush) {
                textColor = glm::vec3(1.0f, 0.9f, 0.3f);
            } else if (!btn.isAction && btn.brush == EditorBrush::RailStart) {
                textColor = glm::vec3(0.95f, 0.80f, 0.45f);
            } else if (!btn.isAction && btn.brush == EditorBrush::RailEnd) {
                textColor = glm::vec3(0.95f, 0.55f, 0.55f);
            } else if (btn.actionId == 6) {
                textColor = getIdColor(m_selectedId);
            } else if (btn.actionId == 13) {
                textColor = m_isWaveEditorOpen ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(0.6f, 0.85f, 1.0f);
            } else if (btn.actionId == 18) {
                textColor = m_isMinecartSettingsOpen ? glm::vec3(1.0f, 0.90f, 0.25f) : glm::vec3(0.90f, 0.72f, 0.30f);
            }
            float tw = m_textRenderer->CalculateTextWidth(btn.label, m_bottomFontScale);
            float tx = btn.pos.x + (btn.size.x - tw) * 0.5f;
            float ty = btn.pos.y + (btn.size.y - m_bottomFontScale * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText(btn.label, tx, ty, m_bottomFontScale, textColor);
        }

        // Текст на клетках спавнеров и баз
        if (m_grid) {
            float cellSize = m_grid->getCellSize();

            for (const auto& b : m_bases) {
                glm::vec2 pPos = m_grid->gridToPixel(b.x, b.y);
                std::string baseLabel = "B#" + std::to_string(b.id);
                m_textRenderer->RenderText(baseLabel, pPos.x + cellSize * 0.10f, pPos.y + cellSize * 0.25f, 0.70f, glm::vec3(0.05f, 0.08f, 0.15f));
            }

            for (size_t i = 0; i < m_spawners.size(); ++i) {
                const auto& sp = m_spawners[i];
                glm::vec2 pPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
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
                m_textRenderer->RenderText(spText, pPos.x + cellSize * 0.06f, pPos.y + cellSize * 0.25f, 0.62f, glm::vec3(0.05f, 0.08f, 0.12f));
            }

            // Текст на маркерах вагонетки (Депо/Старт и Тупик/Конец)
            if (!m_minecarts.empty()) {
                const auto& mc = m_minecarts[0];
                if (mc.hasStart()) {
                    glm::vec2 pPos = m_grid->gridToPixel(mc.start.x, mc.start.y);
                    std::string label = "START";
                    float fScale = std::clamp(cellSize / 64.0f * 0.52f, 0.35f, 0.65f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fScale);
                    float tx = pPos.x + (cellSize - tw) * 0.5f;
                    float ty = pPos.y + (cellSize - fScale * 28.0f) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fScale, glm::vec3(0.15f, 0.10f, 0.03f));
                }
                if (mc.hasEnd()) {
                    glm::vec2 pPos = m_grid->gridToPixel(mc.end.x, mc.end.y);
                    std::string label = "END";
                    float fScale = std::clamp(cellSize / 64.0f * 0.56f, 0.35f, 0.68f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fScale);
                    float tx = pPos.x + (cellSize - tw) * 0.5f;
                    float ty = pPos.y + (cellSize - fScale * 28.0f) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fScale, glm::vec3(1.0f, 0.95f, 0.95f));
                }
            }
        }
    }

    m_renderer->endBatch();

    // 8. МОДАЛЬНОЕ ОКНО РЕДАКТОРА ВОЛН (если активно)
    if (m_isWaveEditorOpen) {
        m_renderer->beginBatch();
        renderWaveEditor();
        m_renderer->endBatch();
    }

    // 9. МОДАЛЬНОЕ ОКНО СПИСКА КАРТ (если активно)
    if (m_isMapsModalOpen) {
        if (m_diagRenderFrames < 6) {
            std::cout << "[MapEditor] render(): m_isMapsModalOpen=true, dispatching renderMapsModal() frame " << m_diagRenderFrames << std::endl;
        }
        m_renderer->beginBatch();
        renderMapsModal();
        m_renderer->endBatch();
    }

    // 10. МОДАЛЬНОЕ ОКНО ПЕРЕИМЕНОВАНИЯ КАРТЫ (если активно)
    if (m_isRenameModalOpen) {
        m_renderer->beginBatch();
        renderRenameModal();
        m_renderer->endBatch();
    }

    // 11. МОДАЛЬНОЕ ОКНО ПОДТВЕРЖДЕНИЯ ВЫХОДА (если активно)
    if (m_isExitModalOpen) {
        m_renderer->beginBatch();
        renderExitModal();
        m_renderer->endBatch();
    }

    // 12. МОДАЛЬНЕ ВІКНО НАЛАШТУВАНЬ ВАГОНЕТКИ (якщо активне)
    if (m_isMinecartSettingsOpen) {
        m_renderer->beginBatch();
        renderMinecartSettingsModal();
        m_renderer->endBatch();
    }
}

void MapEditorState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_grid) {
        m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);
    }
    updateButtonLayout();
}

void MapEditorState::commitFocusedInput() {
    if (m_focusedField == FocusedField::None || m_focusedPartIdx < 0) {
        m_focusedField = FocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }
    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(m_waves.size())) {
        m_focusedField = FocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }

    WaveConfig& wave = m_waves[m_selectedWaveIdx];
    if (m_focusedPartIdx >= static_cast<int>(wave.parts.size())) {
        m_focusedField = FocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }

    WavePart& part = wave.parts[m_focusedPartIdx];

    if (!m_inputText.empty()) {
        try {
            if (m_focusedField == FocusedField::Count) {
                int val = std::stoi(m_inputText);
                part.count = std::clamp(val, 1, 999);
            } else if (m_focusedField == FocusedField::Interval) {
                float val = std::stof(m_inputText);
                part.spawnInterwal = std::clamp(val, 0.05f, 30.0f);
            } else if (m_focusedField == FocusedField::Delay) {
                float val = std::stof(m_inputText);
                part.delayAfter = std::clamp(val, 0.0f, 120.0f);
            }
        } catch (...) {
            // Keep previous valid value
        }
    }

    m_focusedField = FocusedField::None;
    m_focusedPartIdx = -1;
    m_inputText.clear();
    m_fieldJustFocused = false;
}

void MapEditorState::startEditingField(int partIdx, FocusedField field) {
    commitFocusedInput();
    m_openDropdownPartIdx = -1;
    m_focusedPartIdx = partIdx;
    m_focusedField = field;
    m_fieldJustFocused = true;
    m_cursorBlinkTimer = 0.0f;

    auto formatFloat1 = [](float val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", val);
        return std::string(buf);
    };

    if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(m_waves.size())) {
        WaveConfig& wave = m_waves[m_selectedWaveIdx];
        if (partIdx >= 0 && partIdx < static_cast<int>(wave.parts.size())) {
            WavePart& part = wave.parts[partIdx];
            if (field == FocusedField::Count) {
                m_inputText = std::to_string(part.count);
            } else if (field == FocusedField::Interval) {
                m_inputText = formatFloat1(part.spawnInterwal);
            } else if (field == FocusedField::Delay) {
                m_inputText = formatFloat1(part.delayAfter);
            }
        }
    }
}

void MapEditorState::cycleNextInputField() {
    if (m_focusedField == FocusedField::None || m_focusedPartIdx < 0) return;
    int curPart = m_focusedPartIdx;
    FocusedField curField = m_focusedField;
    commitFocusedInput();

    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(m_waves.size())) return;
    WaveConfig& wave = m_waves[m_selectedWaveIdx];
    if (wave.parts.empty()) return;

    if (curField == FocusedField::Count) {
        startEditingField(curPart, FocusedField::Interval);
    } else if (curField == FocusedField::Interval) {
        startEditingField(curPart, FocusedField::Delay);
    } else if (curField == FocusedField::Delay) {
        int nextPart = (curPart + 1) % static_cast<int>(wave.parts.size());
        startEditingField(nextPart, FocusedField::Count);
    }
}

bool MapEditorState::processWaveEditorInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isWaveEditorOpen) return false;

    bool isShiftDown = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    float topBarH = getTopBarHeight();
    float dockH = getBottomDockHeight();
    float panelW = std::clamp(640.0f, 540.0f, static_cast<float>(m_width) - 180.0f);
    float panelX = static_cast<float>(m_width) - panelW;
    float panelY = topBarH;
    float panelH = static_cast<float>(m_height) - dockH - topBarH;

    // 1. Обработка ввода символов с клавиатуры
    if (m_focusedField != FocusedField::None && m_focusedPartIdx >= 0) {
        auto checkKeyInput = [&](int key) -> bool {
            bool down = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& ks = m_keyStates[key];
            if (down) {
                if (!ks.isDown) {
                    ks.isDown = true;
                    ks.holdTimer = 0.0f;
                    ks.repeatTimer = 0.0f;
                    return true;
                } else {
                    ks.holdTimer += dt;
                    if (ks.holdTimer >= 0.38f) {
                        ks.repeatTimer += dt;
                        if (ks.repeatTimer >= 0.05f) {
                            ks.repeatTimer = 0.0f;
                            return true;
                        }
                    }
                }
            } else {
                ks.isDown = false;
                ks.holdTimer = 0.0f;
                ks.repeatTimer = 0.0f;
            }
            return false;
        };

        if (checkKeyInput(GLFW_KEY_BACKSPACE)) {
            if (m_fieldJustFocused) {
                m_inputText.clear();
                m_fieldJustFocused = false;
            } else if (!m_inputText.empty()) {
                m_inputText.pop_back();
            }
            m_cursorBlinkTimer = 0.0f;
        }
        else if (checkKeyInput(GLFW_KEY_ENTER) || checkKeyInput(GLFW_KEY_KP_ENTER)) {
            commitFocusedInput();
        }
        else if (checkKeyInput(GLFW_KEY_TAB)) {
            cycleNextInputField();
        }
        else if (checkKeyInput(GLFW_KEY_ESCAPE)) {
            commitFocusedInput();
        }

        for (int k = 0; k <= 9; ++k) {
            if (checkKeyInput(GLFW_KEY_0 + k) || checkKeyInput(GLFW_KEY_KP_0 + k)) {
                char ch = '0' + k;
                if (m_fieldJustFocused) {
                    m_inputText.clear();
                    m_fieldJustFocused = false;
                }
                if (m_focusedField == FocusedField::Count) {
                    if (m_inputText.length() < 3) {
                        m_inputText += ch;
                        m_cursorBlinkTimer = 0.0f;
                    }
                } else {
                    if (m_inputText.length() < 5) {
                        m_inputText += ch;
                        m_cursorBlinkTimer = 0.0f;
                    }
                }
            }
        }

        if (m_focusedField != FocusedField::Count) {
            if (checkKeyInput(GLFW_KEY_PERIOD) || checkKeyInput(GLFW_KEY_COMMA) || checkKeyInput(GLFW_KEY_KP_DECIMAL)) {
                if (m_fieldJustFocused) {
                    m_inputText.clear();
                    m_fieldJustFocused = false;
                }
                if (m_inputText.find('.') == std::string::npos && m_inputText.length() < 5) {
                    if (m_inputText.empty()) m_inputText += "0.";
                    else m_inputText += '.';
                    m_cursorBlinkTimer = 0.0f;
                }
            }
        }
    }

    // 2. Обработка кликов мыши
    if (leftDown && !m_isLeftMouseDown) {
        float headerH = 36.0f;
        float footerH = 32.0f;

        float colLeftW = 145.0f;
        float colLeftX = panelX + 8.0f;
        float colLeftY = panelY + headerH + 6.0f;
        float colLeftH = panelH - headerH - footerH - 12.0f;

        float colRightX = colLeftX + colLeftW + 8.0f;
        float colRightW = panelW - colLeftW - 24.0f;
        float colRightY = colLeftY;

        float cardsStartY = colRightY + 34.0f;
        float cardH = 92.0f;
        float cardGap = 8.0f;
        int maxVisibleParts = static_cast<int>((colLeftH - 38.0f) / (cardH + cardGap));
        if (maxVisibleParts < 1) maxVisibleParts = 1;

        // ПЕРВОЕ: Проверяем клик по выпадающему списку типов, если он открыт
        if (m_openDropdownPartIdx >= 0 && m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(m_waves.size())) {
            WaveConfig& curWave = m_waves[m_selectedWaveIdx];
            int visIdx = m_openDropdownPartIdx - m_wavePartsScrollOffset;
            if (visIdx >= 0 && visIdx < maxVisibleParts && m_openDropdownPartIdx < static_cast<int>(curWave.parts.size())) {
                glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(visIdx) * (cardH + cardGap));
                glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                glm::vec2 typeBtnSize(115.0f, 26.0f);

                float dropX = cardPos.x + 38.0f;
                float dropW = 120.0f;
                float itemH = 28.0f;
                float totalDropH = itemH * 3.0f;
                float dropY = cardPos.y + 36.0f;
                if (dropY + totalDropH > panelY + panelH - footerH) {
                    dropY = cardPos.y + 8.0f - totalDropH - 2.0f;
                }

                if (isPointInRect(mousePos, glm::vec2(dropX, dropY), glm::vec2(dropW, itemH))) {
                    curWave.parts[m_openDropdownPartIdx].type = "Basic";
                    m_openDropdownPartIdx = -1;
                    return true;
                }
                if (isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH), glm::vec2(dropW, itemH))) {
                    curWave.parts[m_openDropdownPartIdx].type = "Fast";
                    m_openDropdownPartIdx = -1;
                    return true;
                }
                if (isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH * 2.0f), glm::vec2(dropW, itemH))) {
                    curWave.parts[m_openDropdownPartIdx].type = "Tank";
                    m_openDropdownPartIdx = -1;
                    return true;
                }
                // Клик по той же кнопке закрывает меню
                if (isPointInRect(mousePos, typeBtnPos, typeBtnSize)) {
                    m_openDropdownPartIdx = -1;
                    return true;
                }
            }
            // Клик в любое другое место просто закрывает меню (модальное поведение)
            m_openDropdownPartIdx = -1;
            return true;
        }

        // Если кликнули за пределами боковой панели
        if (mousePos.x < panelX || mousePos.y < panelY || mousePos.y > panelY + panelH) {
            commitFocusedInput();
            return false; // Клик передается в сетку/тулбар
        }

        // Кнопка [X Close] в заголовке
        glm::vec2 closeBtnPos(panelX + panelW - 74.0f, panelY + 5.0f);
        glm::vec2 closeBtnSize(68.0f, 26.0f);
        if (isPointInRect(mousePos, closeBtnPos, closeBtnSize)) {
            commitFocusedInput();
            m_isWaveEditorOpen = false;
            return true;
        }

        // Кнопка [+ Add Wave]
        glm::vec2 addWaveBtnPos(colLeftX + 4.0f, colLeftY + 4.0f);
        glm::vec2 addWaveBtnSize(colLeftW - 8.0f, 26.0f);
        if (isPointInRect(mousePos, addWaveBtnPos, addWaveBtnSize)) {
            commitFocusedInput();
            WaveConfig newWave;
            WavePart p;
            p.type = "Basic";
            p.count = 10;
            p.spawnInterwal = 0.8f;
            p.delayAfter = 2.0f;
            newWave.parts.push_back(p);
            m_waves.push_back(newWave);
            m_selectedWaveIdx = static_cast<int>(m_waves.size()) - 1;
            m_wavePartsScrollOffset = 0;
            return true;
        }

        // Список волн
        float waveItemY = colLeftY + 34.0f;
        float waveItemH = 30.0f;
        int maxVisibleWaves = static_cast<int>((colLeftH - 38.0f) / (waveItemH + 4.0f));

        for (size_t i = 0; i < m_waves.size() && static_cast<int>(i) < maxVisibleWaves; ++i) {
            float itemY = waveItemY + static_cast<float>(i) * (waveItemH + 4.0f);
            glm::vec2 itemPos(colLeftX + 4.0f, itemY);
            glm::vec2 itemSize(colLeftW - 36.0f, waveItemH);
            glm::vec2 delPos(colLeftX + colLeftW - 30.0f, itemY);
            glm::vec2 delSize(24.0f, waveItemH);

            if (isPointInRect(mousePos, itemPos, itemSize)) {
                commitFocusedInput();
                m_selectedWaveIdx = static_cast<int>(i);
                m_wavePartsScrollOffset = 0;
                return true;
            }

            if (isPointInRect(mousePos, delPos, delSize)) {
                commitFocusedInput();
                m_waves.erase(m_waves.begin() + i);
                if (m_waves.empty()) {
                    WaveConfig defW;
                    defW.parts.push_back({ "Basic", 10, 0.8f, 2.0f });
                    m_waves.push_back(defW);
                }
                m_selectedWaveIdx = std::clamp(m_selectedWaveIdx, 0, static_cast<int>(m_waves.size()) - 1);
                m_wavePartsScrollOffset = 0;
                return true;
            }
        }

        // Правая колонка: подволны
        if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(m_waves.size())) {
            WaveConfig& curWave = m_waves[m_selectedWaveIdx];

            // [+ Add Batch]
            glm::vec2 addPartBtnPos(colRightX + colRightW - 110.0f, colRightY + 2.0f);
            glm::vec2 addPartBtnSize(110.0f, 26.0f);
            if (isPointInRect(mousePos, addPartBtnPos, addPartBtnSize)) {
                commitFocusedInput();
                WavePart p;
                p.type = "Basic";
                p.count = 10;
                p.spawnInterwal = 0.8f;
                p.delayAfter = 2.0f;
                curWave.parts.push_back(p);
                return true;
            }

            // Scroll buttons [^] and [v]
            glm::vec2 scrollUpPos(colRightX + colRightW - 176.0f, colRightY + 2.0f);
            glm::vec2 scrollDownPos(colRightX + colRightW - 144.0f, colRightY + 2.0f);
            glm::vec2 scrollBtnSize(28.0f, 26.0f);

            if (isPointInRect(mousePos, scrollUpPos, scrollBtnSize)) {
                commitFocusedInput();
                if (m_wavePartsScrollOffset > 0) m_wavePartsScrollOffset--;
                return true;
            }
            if (isPointInRect(mousePos, scrollDownPos, scrollBtnSize)) {
                commitFocusedInput();
                m_wavePartsScrollOffset++;
                return true;
            }

            int maxOffset = std::max(0, static_cast<int>(curWave.parts.size()) - maxVisibleParts);
            m_wavePartsScrollOffset = std::clamp(m_wavePartsScrollOffset, 0, maxOffset);

            for (int v = 0; v < maxVisibleParts; ++v) {
                int pIdx = m_wavePartsScrollOffset + v;
                if (pIdx >= static_cast<int>(curWave.parts.size())) break;

                WavePart& part = curWave.parts[pIdx];
                glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(v) * (cardH + cardGap));
                glm::vec2 cardSize(colRightW, cardH);

                // Ряд 1: Кнопка выпадающего списка типа
                glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                glm::vec2 typeBtnSize(115.0f, 26.0f);
                if (isPointInRect(mousePos, typeBtnPos, typeBtnSize)) {
                    commitFocusedInput();
                    m_openDropdownPartIdx = (m_openDropdownPartIdx == pIdx) ? -1 : pIdx;
                    return true;
                }

                // Ряд 1: Кнопка удаления пачки
                glm::vec2 delPartBtnPos(cardPos.x + cardSize.x - 34.0f, cardPos.y + 8.0f);
                glm::vec2 delPartBtnSize(26.0f, 26.0f);
                if (isPointInRect(mousePos, delPartBtnPos, delPartBtnSize)) {
                    commitFocusedInput();
                    curWave.parts.erase(curWave.parts.begin() + pIdx);
                    if (m_wavePartsScrollOffset > 0 && m_wavePartsScrollOffset + maxVisibleParts > static_cast<int>(curWave.parts.size())) {
                        m_wavePartsScrollOffset--;
                    }
                    return true;
                }

                // Ряд 2: Count, Rate, Pause
                float row2Y = cardPos.y + 50.0f;
                float btnH = 26.0f;
                float nudgeW = 20.0f;

                // Count
                glm::vec2 cntDecPos(cardPos.x + 60.0f, row2Y);
                glm::vec2 cntBoxPos(cardPos.x + 82.0f, row2Y);
                glm::vec2 cntBoxSize(44.0f, btnH);
                glm::vec2 cntIncPos(cardPos.x + 128.0f, row2Y);

                if (isPointInRect(mousePos, cntDecPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput();
                    int delta = isShiftDown ? 5 : 1;
                    part.count = std::max(1, part.count - delta);
                    return true;
                }
                if (isPointInRect(mousePos, cntBoxPos, cntBoxSize)) {
                    startEditingField(pIdx, FocusedField::Count);
                    return true;
                }
                if (isPointInRect(mousePos, cntIncPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput();
                    int delta = isShiftDown ? 5 : 1;
                    part.count = std::min(999, part.count + delta);
                    return true;
                }

                // Rate
                glm::vec2 rateDecPos(cardPos.x + 204.0f, row2Y);
                glm::vec2 rateBoxPos(cardPos.x + 226.0f, row2Y);
                glm::vec2 rateBoxSize(48.0f, btnH);
                glm::vec2 rateIncPos(cardPos.x + 276.0f, row2Y);

                if (isPointInRect(mousePos, rateDecPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput();
                    part.spawnInterwal = std::max(0.05f, std::round((part.spawnInterwal - 0.1f) * 10.0f) / 10.0f);
                    return true;
                }
                if (isPointInRect(mousePos, rateBoxPos, rateBoxSize)) {
                    startEditingField(pIdx, FocusedField::Interval);
                    return true;
                }
                if (isPointInRect(mousePos, rateIncPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput();
                    part.spawnInterwal = std::min(30.0f, std::round((part.spawnInterwal + 0.1f) * 10.0f) / 10.0f);
                    return true;
                }

                // Pause
                glm::vec2 pauseDecPos(cardPos.x + 358.0f, row2Y);
                glm::vec2 pauseBoxPos(cardPos.x + 380.0f, row2Y);
                glm::vec2 pauseBoxSize(48.0f, btnH);
                glm::vec2 pauseIncPos(cardPos.x + 430.0f, row2Y);

                if (isPointInRect(mousePos, pauseDecPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput();
                    part.delayAfter = std::max(0.0f, std::round((part.delayAfter - 0.5f) * 10.0f) / 10.0f);
                    return true;
                }
                if (isPointInRect(mousePos, pauseBoxPos, pauseBoxSize)) {
                    startEditingField(pIdx, FocusedField::Delay);
                    return true;
                }
                if (isPointInRect(mousePos, pauseIncPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput();
                    part.delayAfter = std::min(120.0f, std::round((part.delayAfter + 0.5f) * 10.0f) / 10.0f);
                    return true;
                }
            }
        }
        commitFocusedInput();
        return true;
    }

    return true;
}

void MapEditorState::renderWaveEditor() {
    if (!m_isWaveEditorOpen) return;

    auto formatFloat1 = [](float val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", val);
        return std::string(buf);
    };

    float topBarH = getTopBarHeight();
    float dockH = getBottomDockHeight();
    float panelW = std::clamp(640.0f, 540.0f, static_cast<float>(m_width) - 180.0f);
    float panelX = static_cast<float>(m_width) - panelW;
    float panelY = topBarH;
    float panelH = static_cast<float>(m_height) - dockH - topBarH;

    float headerH = 36.0f;
    float footerH = 32.0f;

    // 1. БОКОВАЯ ПАНЕЛЬ ПОВЕРХ КАРТЫ (Карта слева остается полностью видимой!)
    // Левая неоновая полоса-разделитель
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(panelX - 2.0f, panelY), glm::vec2(2.0f, panelH), 0.0f, glm::vec3(0.25f, 0.65f, 0.95f));
    // Тело панели
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(panelX, panelY), glm::vec2(panelW, panelH), 0.0f, glm::vec3(0.09f, 0.10f, 0.14f));

    // 2. ХЕДЕР БОКОВОЙ ПАНЕЛИ
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(panelX, panelY), glm::vec2(panelW, headerH), 0.0f, glm::vec3(0.13f, 0.15f, 0.20f));
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(panelX, panelY + headerH - 1.0f), glm::vec2(panelW, 1.0f), 0.0f, glm::vec3(0.25f, 0.60f, 0.90f));

    // Кнопка [X Close] в хедере
    glm::vec2 closeBtnPos(panelX + panelW - 74.0f, panelY + 5.0f);
    glm::vec2 closeBtnSize(68.0f, 26.0f);
    bool closeHov = isPointInRect(m_mousePos, closeBtnPos, closeBtnSize);
    m_renderer->drawSprite(m_whiteTexture, closeBtnPos, closeBtnSize, 0.0f, closeHov ? glm::vec3(0.85f, 0.30f, 0.30f) : glm::vec3(0.65f, 0.20f, 0.20f));
    m_renderer->drawSprite(m_whiteTexture, closeBtnPos + glm::vec2(1.0f), closeBtnSize - glm::vec2(2.0f), 0.0f, closeHov ? glm::vec3(0.48f, 0.16f, 0.16f) : glm::vec3(0.35f, 0.12f, 0.12f));

    // 3. ФУТЕР БОКОВОЙ ПАНЕЛИ
    float footerY = panelY + panelH - footerH;
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(panelX, footerY), glm::vec2(panelW, footerH), 0.0f, glm::vec3(0.11f, 0.12f, 0.16f));
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(panelX, footerY), glm::vec2(panelW, 1.0f), 0.0f, glm::vec3(0.22f, 0.25f, 0.32f));

    // 4. ЛЕВАЯ КОЛОНКА (Список волн)
    float colLeftW = 145.0f;
    float colLeftX = panelX + 8.0f;
    float colLeftY = panelY + headerH + 6.0f;
    float colLeftH = panelH - headerH - footerH - 12.0f;

    m_renderer->drawSprite(m_whiteTexture, glm::vec2(colLeftX, colLeftY), glm::vec2(colLeftW, colLeftH), 0.0f, glm::vec3(0.20f, 0.23f, 0.29f));
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(colLeftX + 1.0f, colLeftY + 1.0f), glm::vec2(colLeftW - 2.0f, colLeftH - 2.0f), 0.0f, glm::vec3(0.07f, 0.08f, 0.11f));

    // Кнопка [+ Add Wave]
    glm::vec2 addWaveBtnPos(colLeftX + 4.0f, colLeftY + 4.0f);
    glm::vec2 addWaveBtnSize(colLeftW - 8.0f, 26.0f);
    bool addWaveHov = isPointInRect(m_mousePos, addWaveBtnPos, addWaveBtnSize);
    m_renderer->drawSprite(m_whiteTexture, addWaveBtnPos, addWaveBtnSize, 0.0f, addWaveHov ? glm::vec3(0.35f, 0.85f, 0.50f) : glm::vec3(0.25f, 0.70f, 0.40f));
    m_renderer->drawSprite(m_whiteTexture, addWaveBtnPos + glm::vec2(1.0f), addWaveBtnSize - glm::vec2(2.0f), 0.0f, addWaveHov ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.32f, 0.18f));

    // Список волн
    float waveItemY = colLeftY + 34.0f;
    float waveItemH = 30.0f;
    int maxVisibleWaves = static_cast<int>((colLeftH - 38.0f) / (waveItemH + 4.0f));

    for (size_t i = 0; i < m_waves.size() && static_cast<int>(i) < maxVisibleWaves; ++i) {
        float itemY = waveItemY + static_cast<float>(i) * (waveItemH + 4.0f);
        glm::vec2 itemPos(colLeftX + 4.0f, itemY);
        glm::vec2 itemSize(colLeftW - 36.0f, waveItemH);
        glm::vec2 delPos(colLeftX + colLeftW - 30.0f, itemY);
        glm::vec2 delSize(24.0f, waveItemH);

        bool isSel = (static_cast<int>(i) == m_selectedWaveIdx);
        bool waveHov = isPointInRect(m_mousePos, itemPos, itemSize);
        bool delHov = isPointInRect(m_mousePos, delPos, delSize);

        glm::vec3 waveBorder = isSel ? glm::vec3(0.35f, 0.85f, 1.0f) : (waveHov ? glm::vec3(0.35f, 0.42f, 0.55f) : glm::vec3(0.25f, 0.28f, 0.35f));
        glm::vec3 waveBg = isSel ? glm::vec3(0.20f, 0.38f, 0.60f) : (waveHov ? glm::vec3(0.16f, 0.19f, 0.25f) : glm::vec3(0.12f, 0.14f, 0.18f));

        m_renderer->drawSprite(m_whiteTexture, itemPos, itemSize, 0.0f, waveBorder);
        m_renderer->drawSprite(m_whiteTexture, itemPos + glm::vec2(1.0f), itemSize - glm::vec2(2.0f), 0.0f, waveBg);

        m_renderer->drawSprite(m_whiteTexture, delPos, delSize, 0.0f, delHov ? glm::vec3(0.75f, 0.25f, 0.25f) : glm::vec3(0.55f, 0.20f, 0.20f));
        m_renderer->drawSprite(m_whiteTexture, delPos + glm::vec2(1.0f), delSize - glm::vec2(2.0f), 0.0f, delHov ? glm::vec3(0.40f, 0.15f, 0.15f) : glm::vec3(0.28f, 0.12f, 0.12f));
    }

    // 5. ПРАВАЯ КОЛОНКА (Sub-waves / Batches)
    float colRightX = colLeftX + colLeftW + 8.0f;
    float colRightW = panelW - colLeftW - 24.0f;
    float colRightY = colLeftY;

    // Кнопка [+ Add Batch]
    glm::vec2 addPartBtnPos(colRightX + colRightW - 110.0f, colRightY + 2.0f);
    glm::vec2 addPartBtnSize(110.0f, 26.0f);
    bool addPartHov = isPointInRect(m_mousePos, addPartBtnPos, addPartBtnSize);
    m_renderer->drawSprite(m_whiteTexture, addPartBtnPos, addPartBtnSize, 0.0f, addPartHov ? glm::vec3(0.35f, 0.85f, 0.50f) : glm::vec3(0.25f, 0.70f, 0.40f));
    m_renderer->drawSprite(m_whiteTexture, addPartBtnPos + glm::vec2(1.0f), addPartBtnSize - glm::vec2(2.0f), 0.0f, addPartHov ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.32f, 0.18f));

    // Скролл-кнопки
    glm::vec2 scrollUpPos(colRightX + colRightW - 176.0f, colRightY + 2.0f);
    glm::vec2 scrollDownPos(colRightX + colRightW - 144.0f, colRightY + 2.0f);
    glm::vec2 scrollBtnSize(28.0f, 26.0f);
    bool upHov = isPointInRect(m_mousePos, scrollUpPos, scrollBtnSize);
    bool downHov = isPointInRect(m_mousePos, scrollDownPos, scrollBtnSize);

    m_renderer->drawSprite(m_whiteTexture, scrollUpPos, scrollBtnSize, 0.0f, upHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.35f, 0.38f, 0.46f));
    m_renderer->drawSprite(m_whiteTexture, scrollUpPos + glm::vec2(1.0f), scrollBtnSize - glm::vec2(2.0f), 0.0f, upHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.16f, 0.18f, 0.23f));
    m_renderer->drawSprite(m_whiteTexture, scrollDownPos, scrollBtnSize, 0.0f, downHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.35f, 0.38f, 0.46f));
    m_renderer->drawSprite(m_whiteTexture, scrollDownPos + glm::vec2(1.0f), scrollBtnSize - glm::vec2(2.0f), 0.0f, downHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.16f, 0.18f, 0.23f));

    // Cards
    float cardsStartY = colRightY + 34.0f;
    float cardH = 92.0f;
    float cardGap = 8.0f;
    int maxVisibleParts = static_cast<int>((colLeftH - 38.0f) / (cardH + cardGap));
    if (maxVisibleParts < 1) maxVisibleParts = 1;

    int totalMobsInWave = 0;
    if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(m_waves.size())) {
        const auto& curWave = m_waves[m_selectedWaveIdx];
        for (const auto& p : curWave.parts) totalMobsInWave += p.count;

        if (curWave.parts.empty()) {
            glm::vec2 emptyPos(colRightX, cardsStartY);
            glm::vec2 emptySize(colRightW, 90.0f);
            m_renderer->drawSprite(m_whiteTexture, emptyPos, emptySize, 0.0f, glm::vec3(0.22f, 0.25f, 0.30f));
            m_renderer->drawSprite(m_whiteTexture, emptyPos + glm::vec2(1.0f), emptySize - glm::vec2(2.0f), 0.0f, glm::vec3(0.10f, 0.11f, 0.14f));
        } else {
            for (int v = 0; v < maxVisibleParts; ++v) {
                int pIdx = m_wavePartsScrollOffset + v;
                if (pIdx >= static_cast<int>(curWave.parts.size())) break;
                const auto& part = curWave.parts[pIdx];

                glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(v) * (cardH + cardGap));
                glm::vec2 cardSize(colRightW, cardH);

                // Тело карточки
                m_renderer->drawSprite(m_whiteTexture, cardPos, cardSize, 0.0f, glm::vec3(0.24f, 0.28f, 0.36f));
                m_renderer->drawSprite(m_whiteTexture, cardPos + glm::vec2(1.0f), cardSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.12f, 0.14f, 0.18f));

                // Ряд 1: Кнопка выпадающего списка типа
                glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                glm::vec2 typeBtnSize(115.0f, 26.0f);
                bool typeHov = isPointInRect(m_mousePos, typeBtnPos, typeBtnSize);
                bool isDropOpen = (m_openDropdownPartIdx == pIdx);

                glm::vec3 typeBg(0.15f, 0.28f, 0.45f);
                glm::vec3 typeBorder(0.30f, 0.60f, 0.95f);
                if (part.type == "Fast") {
                    typeBg = isDropOpen ? glm::vec3(0.48f, 0.40f, 0.16f) : (typeHov ? glm::vec3(0.44f, 0.36f, 0.14f) : glm::vec3(0.38f, 0.32f, 0.12f));
                    typeBorder = glm::vec3(0.95f, 0.80f, 0.20f);
                } else if (part.type == "Tank") {
                    typeBg = isDropOpen ? glm::vec3(0.52f, 0.20f, 0.20f) : (typeHov ? glm::vec3(0.48f, 0.18f, 0.18f) : glm::vec3(0.42f, 0.16f, 0.16f));
                    typeBorder = glm::vec3(0.95f, 0.32f, 0.32f);
                } else {
                    typeBg = isDropOpen ? glm::vec3(0.22f, 0.38f, 0.58f) : (typeHov ? glm::vec3(0.20f, 0.34f, 0.52f) : glm::vec3(0.15f, 0.28f, 0.45f));
                    typeBorder = isDropOpen ? glm::vec3(0.40f, 0.90f, 1.0f) : (typeHov ? glm::vec3(0.40f, 0.75f, 1.0f) : glm::vec3(0.30f, 0.60f, 0.95f));
                }
                m_renderer->drawSprite(m_whiteTexture, typeBtnPos, typeBtnSize, 0.0f, typeBorder);
                m_renderer->drawSprite(m_whiteTexture, typeBtnPos + glm::vec2(1.0f), typeBtnSize - glm::vec2(2.0f), 0.0f, typeBg);

                // Ряд 1: Кнопка удаления пачки [X]
                glm::vec2 delPartBtnPos(cardPos.x + cardSize.x - 34.0f, cardPos.y + 8.0f);
                glm::vec2 delPartBtnSize(26.0f, 26.0f);
                bool delPartHov = isPointInRect(m_mousePos, delPartBtnPos, delPartBtnSize);
                m_renderer->drawSprite(m_whiteTexture, delPartBtnPos, delPartBtnSize, 0.0f, delPartHov ? glm::vec3(0.80f, 0.25f, 0.25f) : glm::vec3(0.60f, 0.20f, 0.20f));
                m_renderer->drawSprite(m_whiteTexture, delPartBtnPos + glm::vec2(1.0f), delPartBtnSize - glm::vec2(2.0f), 0.0f, delPartHov ? glm::vec3(0.45f, 0.16f, 0.16f) : glm::vec3(0.32f, 0.12f, 0.12f));

                // Ряд 2: Контролы параметров
                float row2Y = cardPos.y + 50.0f;
                float btnH = 26.0f;
                float nudgeW = 20.0f;

                // --- Count ---
                glm::vec2 cntDecPos(cardPos.x + 60.0f, row2Y);
                glm::vec2 cntBoxPos(cardPos.x + 82.0f, row2Y);
                glm::vec2 cntBoxSize(44.0f, btnH);
                glm::vec2 cntIncPos(cardPos.x + 128.0f, row2Y);

                bool cDecHov = isPointInRect(m_mousePos, cntDecPos, glm::vec2(nudgeW, btnH));
                m_renderer->drawSprite(m_whiteTexture, cntDecPos, glm::vec2(nudgeW, btnH), 0.0f, cDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                m_renderer->drawSprite(m_whiteTexture, cntDecPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, cDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isCntFocused = (m_focusedPartIdx == pIdx && m_focusedField == FocusedField::Count);
                bool cBoxHov = isPointInRect(m_mousePos, cntBoxPos, cntBoxSize);
                glm::vec3 cntBorder = isCntFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (cBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 cntBoxBg = isCntFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (cBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                m_renderer->drawSprite(m_whiteTexture, cntBoxPos, cntBoxSize, 0.0f, cntBorder);
                m_renderer->drawSprite(m_whiteTexture, cntBoxPos + glm::vec2(1.0f), cntBoxSize - glm::vec2(2.0f), 0.0f, cntBoxBg);

                bool cIncHov = isPointInRect(m_mousePos, cntIncPos, glm::vec2(nudgeW, btnH));
                m_renderer->drawSprite(m_whiteTexture, cntIncPos, glm::vec2(nudgeW, btnH), 0.0f, cIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                m_renderer->drawSprite(m_whiteTexture, cntIncPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, cIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                // --- Rate ---
                glm::vec2 rateDecPos(cardPos.x + 204.0f, row2Y);
                glm::vec2 rateBoxPos(cardPos.x + 226.0f, row2Y);
                glm::vec2 rateBoxSize(48.0f, btnH);
                glm::vec2 rateIncPos(cardPos.x + 276.0f, row2Y);

                bool rDecHov = isPointInRect(m_mousePos, rateDecPos, glm::vec2(nudgeW, btnH));
                m_renderer->drawSprite(m_whiteTexture, rateDecPos, glm::vec2(nudgeW, btnH), 0.0f, rDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                m_renderer->drawSprite(m_whiteTexture, rateDecPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, rDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isRateFocused = (m_focusedPartIdx == pIdx && m_focusedField == FocusedField::Interval);
                bool rBoxHov = isPointInRect(m_mousePos, rateBoxPos, rateBoxSize);
                glm::vec3 rateBorder = isRateFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (rBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 rateBoxBg = isRateFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (rBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                m_renderer->drawSprite(m_whiteTexture, rateBoxPos, rateBoxSize, 0.0f, rateBorder);
                m_renderer->drawSprite(m_whiteTexture, rateBoxPos + glm::vec2(1.0f), rateBoxSize - glm::vec2(2.0f), 0.0f, rateBoxBg);

                bool rIncHov = isPointInRect(m_mousePos, rateIncPos, glm::vec2(nudgeW, btnH));
                m_renderer->drawSprite(m_whiteTexture, rateIncPos, glm::vec2(nudgeW, btnH), 0.0f, rIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                m_renderer->drawSprite(m_whiteTexture, rateIncPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, rIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                // --- Pause ---
                glm::vec2 pauseDecPos(cardPos.x + 358.0f, row2Y);
                glm::vec2 pauseBoxPos(cardPos.x + 380.0f, row2Y);
                glm::vec2 pauseBoxSize(48.0f, btnH);
                glm::vec2 pauseIncPos(cardPos.x + 430.0f, row2Y);

                bool pDecHov = isPointInRect(m_mousePos, pauseDecPos, glm::vec2(nudgeW, btnH));
                m_renderer->drawSprite(m_whiteTexture, pauseDecPos, glm::vec2(nudgeW, btnH), 0.0f, pDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                m_renderer->drawSprite(m_whiteTexture, pauseDecPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, pDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isPauseFocused = (m_focusedPartIdx == pIdx && m_focusedField == FocusedField::Delay);
                bool pBoxHov = isPointInRect(m_mousePos, pauseBoxPos, pauseBoxSize);
                glm::vec3 pauseBorder = isPauseFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (pBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 pauseBoxBg = isPauseFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (pBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                m_renderer->drawSprite(m_whiteTexture, pauseBoxPos, pauseBoxSize, 0.0f, pauseBorder);
                m_renderer->drawSprite(m_whiteTexture, pauseBoxPos + glm::vec2(1.0f), pauseBoxSize - glm::vec2(2.0f), 0.0f, pauseBoxBg);

                bool pIncHov = isPointInRect(m_mousePos, pauseIncPos, glm::vec2(nudgeW, btnH));
                m_renderer->drawSprite(m_whiteTexture, pauseIncPos, glm::vec2(nudgeW, btnH), 0.0f, pIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                m_renderer->drawSprite(m_whiteTexture, pauseIncPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, pIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));
            }
        }
    }

    // Сбрасываем накопленные спрайты перед текстом карточек
    m_renderer->flush();

    // 6. ТЕКСТ В ПАНЕЛИ И КАРТОЧКАХ (без выпадающего списка!)
    if (m_textRenderer) {
        // Хедер
        m_textRenderer->RenderText("WAVE CONFIGURATOR", panelX + 16.0f, panelY + 10.0f, 0.88f, glm::vec3(0.35f, 0.85f, 1.0f));
        m_textRenderer->RenderText("[X] Close", closeBtnPos.x + 8.0f, closeBtnPos.y + 6.0f, 0.58f, glm::vec3(1.0f, 0.85f, 0.85f));

        // [+ Add Wave]
        m_textRenderer->RenderText("+ Add Wave", addWaveBtnPos.x + 22.0f, addWaveBtnPos.y + 6.0f, 0.58f, glm::vec3(0.9f, 1.0f, 0.9f));

        // Waves items
        for (size_t i = 0; i < m_waves.size() && static_cast<int>(i) < maxVisibleWaves; ++i) {
            float itemY = waveItemY + static_cast<float>(i) * (waveItemH + 4.0f);
            glm::vec2 itemPos(colLeftX + 4.0f, itemY);
            glm::vec2 delPos(colLeftX + colLeftW - 30.0f, itemY);

            bool isSel = (static_cast<int>(i) == m_selectedWaveIdx);
            glm::vec3 textColor = isSel ? glm::vec3(1.0f, 0.95f, 0.4f) : glm::vec3(0.9f);
            std::string label = "Wave #" + std::to_string(i + 1) + " (" + std::to_string(m_waves[i].parts.size()) + ")";
            m_textRenderer->RenderText(label, itemPos.x + 6.0f, itemPos.y + 8.0f, 0.56f, textColor);

            // [x]
            m_textRenderer->RenderText("x", delPos.x + 8.0f, delPos.y + 7.0f, 0.56f, glm::vec3(1.0f, 0.65f, 0.65f));
        }

        // Right column header
        if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(m_waves.size())) {
            const auto& curWave = m_waves[m_selectedWaveIdx];
            std::string rTitle = "Wave #" + std::to_string(m_selectedWaveIdx + 1) + " Batches (" + std::to_string(curWave.parts.size()) + ")";
            m_textRenderer->RenderText(rTitle, colRightX, colRightY + 6.0f, 0.70f, glm::vec3(0.95f, 0.95f, 0.95f));

            m_textRenderer->RenderText("^", scrollUpPos.x + 10.0f, scrollUpPos.y + 5.0f, 0.60f, glm::vec3(0.85f));
            m_textRenderer->RenderText("v", scrollDownPos.x + 10.0f, scrollDownPos.y + 5.0f, 0.60f, glm::vec3(0.85f));
            m_textRenderer->RenderText("+ Add Batch", addPartBtnPos.x + 14.0f, addPartBtnPos.y + 6.0f, 0.58f, glm::vec3(0.9f, 1.0f, 0.9f));

            if (curWave.parts.empty()) {
                m_textRenderer->RenderText("No sub-waves configured.", colRightX + 15.0f, cardsStartY + 30.0f, 0.64f, glm::vec3(0.7f, 0.7f, 0.7f));
                m_textRenderer->RenderText("Click '+ Add Batch' to add enemies!", colRightX + 15.0f, cardsStartY + 55.0f, 0.58f, glm::vec3(0.4f, 0.8f, 1.0f));
            } else {
                bool showBlinkCursor = (m_cursorBlinkTimer < 0.53f);

                for (int v = 0; v < maxVisibleParts; ++v) {
                    int pIdx = m_wavePartsScrollOffset + v;
                    if (pIdx >= static_cast<int>(curWave.parts.size())) break;
                    const auto& part = curWave.parts[pIdx];
                    glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(v) * (cardH + cardGap));

                    // Ряд 1: Текст
                    std::string badge = "#" + std::to_string(pIdx + 1);
                    m_textRenderer->RenderText(badge, cardPos.x + 10.0f, cardPos.y + 12.0f, 0.65f, glm::vec3(0.35f, 0.85f, 1.0f));

                    // Кнопка типа
                    glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                    std::string typeLabel = part.type + (m_openDropdownPartIdx == pIdx ? "  ^" : "  v");
                    glm::vec3 typeTxtCol = (part.type == "Basic") ? glm::vec3(0.4f, 0.9f, 1.0f) : (part.type == "Fast" ? glm::vec3(1.0f, 0.9f, 0.3f) : glm::vec3(1.0f, 0.45f, 0.45f));
                    m_textRenderer->RenderText(typeLabel, typeBtnPos.x + 10.0f, typeBtnPos.y + 6.0f, 0.58f, typeTxtCol);

                    m_textRenderer->RenderText(std::to_string(part.count) + " mobs", cardPos.x + 165.0f, cardPos.y + 14.0f, 0.52f, glm::vec3(0.75f, 0.78f, 0.85f));
                    m_textRenderer->RenderText("x", cardPos.x + colRightW - 26.0f, cardPos.y + 13.0f, 0.55f, glm::vec3(1.0f, 0.70f, 0.70f));

                    // Ряд 2: Текст параметров
                    float row2Y = cardPos.y + 50.0f;

                    // Count
                    m_textRenderer->RenderText("Count:", cardPos.x + 10.0f, row2Y + 6.0f, 0.52f, glm::vec3(0.85f));
                    m_textRenderer->RenderText("-", cardPos.x + 66.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    bool isCntFocused = (m_focusedPartIdx == pIdx && m_focusedField == FocusedField::Count);
                    std::string cntStr = isCntFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : std::to_string(part.count);
                    glm::vec3 cntCol = isCntFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(1.0f, 0.95f, 0.4f);
                    m_textRenderer->RenderText(cntStr, cardPos.x + 88.0f, row2Y + 6.0f, 0.54f, cntCol);
                    m_textRenderer->RenderText("+", cardPos.x + 133.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    // Rate
                    m_textRenderer->RenderText("Rate:", cardPos.x + 162.0f, row2Y + 6.0f, 0.52f, glm::vec3(0.85f));
                    m_textRenderer->RenderText("-", cardPos.x + 210.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    bool isRateFocused = (m_focusedPartIdx == pIdx && m_focusedField == FocusedField::Interval);
                    std::string rateStr = isRateFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : (formatFloat1(part.spawnInterwal) + "s");
                    glm::vec3 rateCol = isRateFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(0.4f, 1.0f, 0.6f);
                    m_textRenderer->RenderText(rateStr, cardPos.x + 232.0f, row2Y + 6.0f, 0.54f, rateCol);
                    m_textRenderer->RenderText("+", cardPos.x + 281.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    // Pause
                    m_textRenderer->RenderText("Pause:", cardPos.x + 310.0f, row2Y + 6.0f, 0.52f, glm::vec3(0.85f));
                    m_textRenderer->RenderText("-", cardPos.x + 364.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    bool isPauseFocused = (m_focusedPartIdx == pIdx && m_focusedField == FocusedField::Delay);
                    std::string pauseStr = isPauseFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : (formatFloat1(part.delayAfter) + "s");
                    glm::vec3 pauseCol = isPauseFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(1.0f, 0.8f, 0.4f);
                    m_textRenderer->RenderText(pauseStr, cardPos.x + 386.0f, row2Y + 6.0f, 0.54f, pauseCol);
                    m_textRenderer->RenderText("+", cardPos.x + 435.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));
                }
            }
        }

        // Footer info text
        std::string summary = "Waves: " + std::to_string(m_waves.size()) + " | Wave #" + std::to_string(m_selectedWaveIdx + 1) + ": " + std::to_string(totalMobsInWave) + " mobs | [Tab] Next field";
        m_textRenderer->RenderText(summary, panelX + 14.0f, panelY + panelH - 22.0f, 0.52f, glm::vec3(0.70f, 0.75f, 0.85f));
    }

    // 7. ОТДЕЛЬНЫЙ СЛОЙ ВЫПАДАЮЩЕГО СПИСКА (DROPDOWN POPUP OVERLAY)
    // Рисуется строго ПОСЛЕ отрисовки карточек и их текста, наглухо перекрывая любые нижележащие элементы!
    if (m_openDropdownPartIdx >= 0 && m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(m_waves.size())) {
        WaveConfig& curWave = m_waves[m_selectedWaveIdx];
        int visIdx = m_openDropdownPartIdx - m_wavePartsScrollOffset;
        if (visIdx >= 0 && visIdx < maxVisibleParts && m_openDropdownPartIdx < static_cast<int>(curWave.parts.size())) {
            glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(visIdx) * (cardH + cardGap));
            float dropX = cardPos.x + 38.0f;
            float dropW = 120.0f;
            float itemH = 28.0f;
            float totalDropH = itemH * 3.0f;
            float dropY = cardPos.y + 36.0f;
            if (dropY + totalDropH > panelY + panelH - footerH) {
                dropY = cardPos.y + 8.0f - totalDropH - 2.0f;
            }

            // Тень выпадающего окна
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX - 3.0f, dropY - 2.0f), glm::vec2(dropW + 6.0f, totalDropH + 7.0f), 0.0f, glm::vec3(0.02f, 0.03f, 0.05f));

            // Неоновая окантовка окна
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX - 1.0f, dropY - 1.0f), glm::vec2(dropW + 2.0f, totalDropH + 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

            // Глухой непрозрачный фон списка (полностью скрывает всё под собой)
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX, dropY), glm::vec2(dropW, totalDropH), 0.0f, glm::vec3(0.10f, 0.12f, 0.17f));

            // Фоновые плашки элементов с подсветкой при наведении
            bool hov0 = isPointInRect(m_mousePos, glm::vec2(dropX, dropY), glm::vec2(dropW, itemH));
            glm::vec3 bg0 = hov0 ? glm::vec3(0.20f, 0.35f, 0.55f) : (curWave.parts[m_openDropdownPartIdx].type == "Basic" ? glm::vec3(0.14f, 0.22f, 0.32f) : glm::vec3(0.11f, 0.13f, 0.18f));
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX + 1.0f, dropY + 1.0f), glm::vec2(dropW - 2.0f, itemH - 2.0f), 0.0f, bg0);

            bool hov1 = isPointInRect(m_mousePos, glm::vec2(dropX, dropY + itemH), glm::vec2(dropW, itemH));
            glm::vec3 bg1 = hov1 ? glm::vec3(0.38f, 0.35f, 0.15f) : (curWave.parts[m_openDropdownPartIdx].type == "Fast" ? glm::vec3(0.25f, 0.22f, 0.12f) : glm::vec3(0.11f, 0.13f, 0.18f));
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX + 1.0f, dropY + itemH + 1.0f), glm::vec2(dropW - 2.0f, itemH - 2.0f), 0.0f, bg1);

            bool hov2 = isPointInRect(m_mousePos, glm::vec2(dropX, dropY + itemH * 2.0f), glm::vec2(dropW, itemH));
            glm::vec3 bg2 = hov2 ? glm::vec3(0.42f, 0.20f, 0.20f) : (curWave.parts[m_openDropdownPartIdx].type == "Tank" ? glm::vec3(0.28f, 0.14f, 0.14f) : glm::vec3(0.11f, 0.13f, 0.18f));
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX + 1.0f, dropY + itemH * 2.0f + 1.0f), glm::vec2(dropW - 2.0f, itemH - 2.0f), 0.0f, bg2);

            // Тонкие линии-разделители пунктов
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX + 4.0f, dropY + itemH), glm::vec2(dropW - 8.0f, 1.0f), 0.0f, glm::vec3(0.25f, 0.30f, 0.40f));
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(dropX + 4.0f, dropY + itemH * 2.0f), glm::vec2(dropW - 8.0f, 1.0f), 0.0f, glm::vec3(0.25f, 0.30f, 0.40f));

            // ФЛАШИМ СПРАЙТЫ ВЫПАДАЮЩЕГО ОКНА НА ВИДЕОКАРТУ! Это перекрывает любой нижележащий текст!
            m_renderer->flush();

            // Текст пунктов выпадающего списка
            if (m_textRenderer) {
                m_textRenderer->RenderText("Basic", dropX + 12.0f, dropY + 7.0f, 0.58f, glm::vec3(0.4f, 0.9f, 1.0f));
                m_textRenderer->RenderText("Fast",  dropX + 12.0f, dropY + itemH + 7.0f, 0.58f, glm::vec3(1.0f, 0.9f, 0.3f));
                m_textRenderer->RenderText("Tank",  dropX + 12.0f, dropY + itemH * 2.0f + 7.0f, 0.58f, glm::vec3(1.0f, 0.45f, 0.45f));
            }
        }
    }
}

void MapEditorState::openRenameModal(const std::string& targetFileName) {
    m_renameTargetFileName = targetFileName.empty() ? m_currentLevelFileName : targetFileName;
    m_renameInputText = m_currentLevelDisplayName;
    m_isRenameModalOpen = true;
    m_cursorBlinkTimer = 0.0f;
    m_suppressPlacementUntilRelease = true;
    m_suppressClickUntilRelease = true;
}

void MapEditorState::confirmRename() {
    if (m_renameInputText.empty()) {
        m_isRenameModalOpen = false;
        m_suppressPlacementUntilRelease = true;
        return;
    }

    m_currentLevelDisplayName = m_renameInputText;
    LevelManager::setLevelDisplayName(m_renameTargetFileName, m_renameInputText);

    std::string safeBase = InputManager::transliterateToAscii(m_renameInputText);
    std::string newFileName = LevelManager::sanitizeLevelFileName(safeBase);
    if (newFileName != m_renameTargetFileName) {
        bool ok = LevelManager::renameLevel(m_renameTargetFileName, newFileName);
        if (ok) {
            if (m_renameTargetFileName == m_currentLevelFileName) {
                m_currentLevelFileName = newFileName;
            }
        }
    }
    m_statusMessage = "Renamed: " + m_currentLevelDisplayName;
    m_statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
    m_statusTimer = 3.5f;
    m_isRenameModalOpen = false;
    m_suppressPlacementUntilRelease = true;
    updateButtonLayout();
}

MapEditorState::RenameModalLayout MapEditorState::getRenameModalLayout() const {
    RenameModalLayout layout;
    float scale = GetUIScale(m_width, m_height);

    layout.fTitle = std::clamp(0.68f * scale, 0.48f, 0.86f);
    layout.fSub = std::clamp(0.48f * scale, 0.35f, 0.62f);
    layout.fInput = std::clamp(0.58f * scale, 0.42f, 0.72f);
    layout.fBtn = std::clamp(0.48f * scale, 0.36f, 0.62f);
    layout.fCross = std::clamp(0.55f * scale, 0.40f, 0.75f);

    layout.modalSize.x = std::clamp(480.0f * scale, 360.0f, static_cast<float>(m_width) - 40.0f);
    layout.modalSize.y = std::clamp(230.0f * scale, 180.0f, static_cast<float>(m_height) - 40.0f);
    layout.modalPos = glm::vec2((static_cast<float>(m_width) - layout.modalSize.x) * 0.5f,
                                (static_cast<float>(m_height) - layout.modalSize.y) * 0.5f);

    layout.headerH = std::clamp(40.0f * scale, 30.0f, 54.0f);

    float crossSize = std::clamp(26.0f * scale, 20.0f, 34.0f);
    layout.btnCloseCrossSize = glm::vec2(crossSize, crossSize);
    layout.btnCloseCrossPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - crossSize - std::clamp(8.0f * scale, 6.0f, 12.0f),
                                        layout.modalPos.y + (layout.headerH - crossSize) * 0.5f);

    layout.subY = layout.modalPos.y + layout.headerH + std::clamp(10.0f * scale, 7.0f, 14.0f);

    float boxMarginX = std::clamp(24.0f * scale, 16.0f, 34.0f);
    float boxH = std::clamp(38.0f * scale, 28.0f, 48.0f);
    float boxY = layout.subY + layout.fSub * 28.0f + std::clamp(8.0f * scale, 5.0f, 12.0f);
    layout.boxPos = glm::vec2(layout.modalPos.x + boxMarginX, boxY);
    layout.boxSize = glm::vec2(layout.modalSize.x - 2.0f * boxMarginX, boxH);

    float btnH = std::clamp(40.0f * scale, 30.0f, 50.0f);
    float btnMarginBottom = std::clamp(16.0f * scale, 10.0f, 22.0f);
    float btnY = layout.modalPos.y + layout.modalSize.y - btnH - btnMarginBottom;
    float btnGap = std::clamp(14.0f * scale, 10.0f, 20.0f);
    float availBtnW = layout.modalSize.x - 2.0f * boxMarginX - btnGap;
    float btnW = availBtnW * 0.5f;

    layout.btnSavePos = glm::vec2(layout.modalPos.x + boxMarginX, btnY);
    layout.btnSaveSize = glm::vec2(btnW, btnH);
    layout.btnCancelPos = glm::vec2(layout.btnSavePos.x + btnW + btnGap, btnY);
    layout.btnCancelSize = glm::vec2(btnW, btnH);

    return layout;
}

bool MapEditorState::processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isRenameModalOpen) return false;

    RenameModalLayout l = getRenameModalLayout();

    auto checkKeyInput = [&](int key) -> bool {
        bool down = (glfwGetKey(window, key) == GLFW_PRESS);
        auto& ks = m_keyStates[key];
        if (down) {
            if (!ks.isDown) {
                ks.isDown = true;
                ks.holdTimer = 0.0f;
                ks.repeatTimer = 0.0f;
                return true;
            } else {
                ks.holdTimer += dt;
                if (ks.holdTimer >= 0.38f) {
                    ks.repeatTimer += dt;
                    if (ks.repeatTimer >= 0.05f) {
                        ks.repeatTimer = 0.0f;
                        return true;
                    }
                }
            }
        } else {
            ks.isDown = false;
            ks.holdTimer = 0.0f;
            ks.repeatTimer = 0.0f;
        }
        return false;
    };

    if (checkKeyInput(GLFW_KEY_ESCAPE)) {
        m_isRenameModalOpen = false;
        m_suppressPlacementUntilRelease = true;
        return true;
    }
    if (checkKeyInput(GLFW_KEY_ENTER) || checkKeyInput(GLFW_KEY_KP_ENTER)) {
        confirmRename();
        m_suppressPlacementUntilRelease = true;
        return true;
    }
    if (checkKeyInput(GLFW_KEY_BACKSPACE)) {
        InputManager::popUtf8(m_renameInputText);
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    std::string typed = InputManager::getFrameText();
    if (!typed.empty() && m_renameInputText.length() < 60) {
        m_renameInputText += typed;
        m_cursorBlinkTimer = 0.0f;
    }

    if (leftDown && !m_isLeftMouseDown) {
        if (isPointInRect(mousePos, l.btnSavePos, l.btnSaveSize)) {
            confirmRename();
            m_suppressPlacementUntilRelease = true;
            return true;
        }
        if (isPointInRect(mousePos, l.btnCancelPos, l.btnCancelSize)) {
            m_isRenameModalOpen = false;
            m_suppressPlacementUntilRelease = true;
            return true;
        }
        if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
            m_isRenameModalOpen = false;
            m_suppressPlacementUntilRelease = true;
            return true;
        }
        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            m_isRenameModalOpen = false;
            m_suppressPlacementUntilRelease = true;
            return true;
        }
    }

    return true;
}

std::vector<LevelInfo> MapEditorState::getFilteredModalLevels() const {
    auto all = LevelManager::getAvailableLevels();
    bool dev = CampaignManager::isDevMode();
    if (dev) {
        return all;
    }
    // Если Dev Mode выключен - показываем только кастомные уровни
    std::vector<LevelInfo> filtered;
    for (const auto& lvl : all) {
        if (!CampaignManager::isFileInCampaign(lvl.filename)) {
            filtered.push_back(lvl);
        }
    }
    return filtered;
}

MapEditorState::MapsModalLayout MapEditorState::getMapsModalLayout() const {
    MapsModalLayout layout;
    float scale = GetUIScale(m_width, m_height);

    layout.fTitle = std::clamp(0.68f * scale, 0.48f, 0.88f);
    layout.fSub = std::clamp(0.50f * scale, 0.38f, 0.65f);
    layout.fNew = std::clamp(0.50f * scale, 0.36f, 0.64f);
    layout.fCardName = std::clamp(0.54f * scale, 0.38f, 0.70f);
    layout.fCardSub = std::clamp(0.42f * scale, 0.30f, 0.54f);
    layout.fActionBtn = std::clamp(0.48f * scale, 0.35f, 0.62f);
    layout.fClose = std::clamp(0.52f * scale, 0.38f, 0.68f);
    layout.fCross = std::clamp(0.55f * scale, 0.40f, 0.75f);

    layout.modalSize.x = std::clamp(640.0f * scale, 480.0f, static_cast<float>(m_width) - 40.0f);
    layout.modalSize.y = std::clamp(520.0f * scale, 380.0f, static_cast<float>(m_height) - 40.0f);
    layout.modalPos = glm::vec2((static_cast<float>(m_width) - layout.modalSize.x) * 0.5f,
                                (static_cast<float>(m_height) - layout.modalSize.y) * 0.5f);

    layout.headerH = std::clamp(42.0f * scale, 32.0f, 56.0f);

    float crossSize = std::clamp(26.0f * scale, 20.0f, 34.0f);
    layout.btnCloseCrossSize = glm::vec2(crossSize, crossSize);
    layout.btnCloseCrossPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - crossSize - std::clamp(8.0f * scale, 6.0f, 12.0f),
                                        layout.modalPos.y + (layout.headerH - crossSize) * 0.5f);

    float subMarginTop = std::clamp(10.0f * scale, 8.0f, 16.0f);
    float subY = layout.modalPos.y + layout.headerH + subMarginTop;

    float btnNewW = std::clamp(160.0f * scale, 120.0f, 210.0f);
    float btnNewH = std::clamp(32.0f * scale, 24.0f, 42.0f);
    layout.btnNewSize = glm::vec2(btnNewW, btnNewH);
    layout.btnNewPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - btnNewW - std::clamp(20.0f * scale, 14.0f, 28.0f), subY);

    float scrollBtnW = std::clamp(30.0f * scale, 24.0f, 38.0f);
    float scrollBtnH = btnNewH;
    layout.btnScrollDownSize = glm::vec2(scrollBtnW, scrollBtnH);
    layout.btnScrollDownPos = glm::vec2(layout.btnNewPos.x - scrollBtnW - std::clamp(8.0f * scale, 4.0f, 12.0f), subY);
    layout.btnScrollUpSize = glm::vec2(scrollBtnW, scrollBtnH);
    layout.btnScrollUpPos = glm::vec2(layout.btnScrollDownPos.x - scrollBtnW - std::clamp(4.0f * scale, 2.0f, 6.0f), subY);

    float btnBottomH = std::clamp(36.0f * scale, 26.0f, 46.0f);
    float btnBottomMargin = std::clamp(14.0f * scale, 10.0f, 20.0f);
    float bottomY = layout.modalPos.y + layout.modalSize.y - btnBottomH - btnBottomMargin;

    float btnCloseW = std::clamp(130.0f * scale, 95.0f, 170.0f);
    layout.btnCloseSize = glm::vec2(btnCloseW, btnBottomH);
    layout.btnClosePos = glm::vec2(layout.modalPos.x + (layout.modalSize.x - btnCloseW) * 0.5f, bottomY);

    float navBtnW = std::clamp(80.0f * scale, 56.0f, 104.0f);
    float navGap = std::clamp(14.0f * scale, 8.0f, 20.0f);
    layout.btnPrevPageSize = glm::vec2(navBtnW, btnBottomH);
    layout.btnPrevPagePos = glm::vec2(layout.btnClosePos.x - navGap - navBtnW, bottomY);
    layout.btnNextPageSize = glm::vec2(navBtnW, btnBottomH);
    layout.btnNextPagePos = glm::vec2(layout.btnClosePos.x + btnCloseW + navGap, bottomY);

    layout.listStartY = subY + btnNewH + std::clamp(10.0f * scale, 6.0f, 16.0f);
    float listEndY = bottomY - std::clamp(10.0f * scale, 6.0f, 16.0f);
    float availListH = listEndY - layout.listStartY;

    layout.itemH = std::clamp(48.0f * scale, 36.0f, 64.0f);
    layout.itemGap = std::clamp(6.0f * scale, 4.0f, 10.0f);

    layout.maxVisible = std::max(1, static_cast<int>((availListH + layout.itemGap) / (layout.itemH + layout.itemGap)));

    auto levels = getFilteredModalLevels();
    layout.totalLevels = static_cast<int>(levels.size());
    layout.hasPagination = (layout.totalLevels > layout.maxVisible);
    layout.maxOffset = std::max(0, layout.totalLevels - layout.maxVisible);
    layout.currentOffset = std::clamp(m_mapsScrollOffset, 0, layout.maxOffset);

    float cardMarginX = std::clamp(18.0f * scale, 12.0f, 26.0f);
    float scrollbarSpace = layout.hasPagination ? std::clamp(14.0f * scale, 10.0f, 20.0f) : 0.0f;
    float cardW = layout.modalSize.x - 2.0f * cardMarginX - scrollbarSpace;

    float btnLoadW = std::clamp(64.0f * scale, 46.0f, 84.0f);
    float btnRenW = std::clamp(72.0f * scale, 52.0f, 96.0f);
    float btnDelW = std::clamp(50.0f * scale, 36.0f, 68.0f);
    float btnActH = std::clamp(28.0f * scale, 22.0f, 38.0f);
    float btnActGap = std::clamp(6.0f * scale, 4.0f, 10.0f);
    float rightMargin = std::clamp(10.0f * scale, 6.0f, 14.0f);

    for (int i = 0; i < layout.maxVisible && (i + layout.currentOffset) < layout.totalLevels; ++i) {
        int idx = i + layout.currentOffset;
        const auto& lvl = levels[idx];

        MapCardLayout card;
        card.levelIdx = idx;
        card.levelInfo = lvl;
        card.hasDel = !lvl.isBuiltIn;
        card.cardPos = glm::vec2(layout.modalPos.x + cardMarginX, layout.listStartY + i * (layout.itemH + layout.itemGap));
        card.cardSize = glm::vec2(cardW, layout.itemH);

        float actY = card.cardPos.y + (layout.itemH - btnActH) * 0.5f;

        if (card.hasDel) {
            card.btnDelSize = glm::vec2(btnDelW, btnActH);
            card.btnDelPos = glm::vec2(card.cardPos.x + card.cardSize.x - rightMargin - btnDelW, actY);

            card.btnRenSize = glm::vec2(btnRenW, btnActH);
            card.btnRenPos = glm::vec2(card.btnDelPos.x - btnActGap - btnRenW, actY);

            card.btnLoadSize = glm::vec2(btnLoadW, btnActH);
            card.btnLoadPos = glm::vec2(card.btnRenPos.x - btnActGap - btnLoadW, actY);
        } else {
            card.btnDelSize = glm::vec2(0.0f);
            card.btnDelPos = glm::vec2(-1000.0f);

            card.btnRenSize = glm::vec2(btnRenW, btnActH);
            card.btnRenPos = glm::vec2(card.cardPos.x + card.cardSize.x - rightMargin - btnDelW - btnActGap - btnRenW, actY);

            card.btnLoadSize = glm::vec2(btnLoadW, btnActH);
            card.btnLoadPos = glm::vec2(card.btnRenPos.x - btnActGap - btnLoadW, actY);
        }

        layout.visibleCards.push_back(card);
    }

    return layout;
}

bool MapEditorState::processMapsModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isMapsModalOpen) return false;

    MapsModalLayout l = getMapsModalLayout();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !m_keyEscPressedLastFrame) {
        std::cout << "[MapEditor] Maps modal closed by ESC (processMapsModalInput), m_isInitialModalLaunch=" << m_isInitialModalLaunch << std::endl;
        m_isMapsModalOpen = false;
        m_suppressPlacementUntilRelease = true;
        if (m_isInitialModalLaunch) {
            returnToOrigin();
        }
        return true;
    }

    auto checkKey = [&](int key) -> bool {
        bool down = (glfwGetKey(window, key) == GLFW_PRESS);
        auto& ks = m_keyStates[key];
        if (down) {
            if (!ks.isDown) {
                ks.isDown = true;
                ks.holdTimer = 0.0f;
                ks.repeatTimer = 0.0f;
                return true;
            } else {
                ks.holdTimer += dt;
                if (ks.holdTimer >= 0.30f) {
                    ks.repeatTimer += dt;
                    if (ks.repeatTimer >= 0.08f) {
                        ks.repeatTimer = 0.0f;
                        return true;
                    }
                }
            }
        } else {
            ks.isDown = false;
        }
        return false;
    };

    if (l.hasPagination) {
        if (checkKey(GLFW_KEY_UP)) {
            if (m_mapsScrollOffset > 0) m_mapsScrollOffset--;
            return true;
        }
        if (checkKey(GLFW_KEY_DOWN)) {
            if (m_mapsScrollOffset < l.maxOffset) m_mapsScrollOffset++;
            return true;
        }
        if (checkKey(GLFW_KEY_PAGE_UP)) {
            m_mapsScrollOffset = std::max(0, m_mapsScrollOffset - l.maxVisible);
            return true;
        }
        if (checkKey(GLFW_KEY_PAGE_DOWN)) {
            m_mapsScrollOffset = std::min(l.maxOffset, m_mapsScrollOffset + l.maxVisible);
            return true;
        }
    }

    if (leftDown && !m_isLeftMouseDown) {
        if (isPointInRect(mousePos, l.btnNewPos, l.btnNewSize)) {
            std::cout << "[MapEditor] Maps modal: clicked + New Map" << std::endl;
            std::string newF = LevelManager::createNewLevel("custom_map");
            loadLevelByName(newF);
            m_isMapsModalOpen = false;
            m_isInitialModalLaunch = false;
            m_suppressPlacementUntilRelease = true;
            m_suppressClickUntilRelease = true;
            m_isLeftMouseDown = true;
            m_statusMessage = "Created and loaded: " + m_currentLevelDisplayName;
            m_statusColor = glm::vec3(0.2f, 1.0f, 0.4f);
            m_statusTimer = 3.5f;
            return true;
        }

        if (isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize) ||
            isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
            std::cout << "[MapEditor] Maps modal closed by Close/Cross button, m_isInitialModalLaunch=" << m_isInitialModalLaunch << std::endl;
            m_isMapsModalOpen = false;
            m_suppressPlacementUntilRelease = true;
            if (m_isInitialModalLaunch) {
                returnToOrigin();
            }
            return true;
        }

        if (l.hasPagination) {
            if (isPointInRect(mousePos, l.btnScrollUpPos, l.btnScrollUpSize)) {
                if (m_mapsScrollOffset > 0) m_mapsScrollOffset--;
                return true;
            }
            if (isPointInRect(mousePos, l.btnScrollDownPos, l.btnScrollDownSize)) {
                if (m_mapsScrollOffset < l.maxOffset) m_mapsScrollOffset++;
                return true;
            }
            if (isPointInRect(mousePos, l.btnPrevPagePos, l.btnPrevPageSize)) {
                m_mapsScrollOffset = std::max(0, m_mapsScrollOffset - l.maxVisible);
                return true;
            }
            if (isPointInRect(mousePos, l.btnNextPagePos, l.btnNextPageSize)) {
                m_mapsScrollOffset = std::min(l.maxOffset, m_mapsScrollOffset + l.maxVisible);
                return true;
            }
        }

        auto levels = getFilteredModalLevels();
        for (const auto& card : l.visibleCards) {
            const auto& lvl = card.levelInfo;

            if (isPointInRect(mousePos, card.btnLoadPos, card.btnLoadSize)) {
                std::cout << "[MapEditor] Maps modal: clicked Load map (" << lvl.filename << ")" << std::endl;
                loadLevelByName(lvl.filename);
                m_isMapsModalOpen = false;
                m_isInitialModalLaunch = false;
                m_suppressPlacementUntilRelease = true;
                m_statusMessage = "Loaded map: " + m_currentLevelDisplayName;
                m_statusColor = glm::vec3(0.3f, 0.9f, 1.0f);
                m_statusTimer = 3.0f;
                return true;
            }

            if (isPointInRect(mousePos, card.btnRenPos, card.btnRenSize)) {
                openRenameModal(lvl.filename);
                m_suppressPlacementUntilRelease = true;
                return true;
            }

            if (card.hasDel && isPointInRect(mousePos, card.btnDelPos, card.btnDelSize)) {
                LevelManager::deleteLevel(lvl.filename);
                if (lvl.filename == m_currentLevelFileName) {
                    loadLevelByName("level_editor.json");
                }
                int newTotal = static_cast<int>(getFilteredModalLevels().size());
                int newMaxOffset = std::max(0, newTotal - l.maxVisible);
                m_mapsScrollOffset = std::clamp(m_mapsScrollOffset, 0, newMaxOffset);
                m_suppressPlacementUntilRelease = true;
                return true;
            }
        }

        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            std::cout << "[MapEditor] Maps modal closed by OUTSIDE CLICK at (" << mousePos.x << "," << mousePos.y 
                      << ") modalPos=(" << l.modalPos.x << "," << l.modalPos.y << ") modalSize=(" << l.modalSize.x << "," << l.modalSize.y 
                      << "), m_isInitialModalLaunch=" << m_isInitialModalLaunch << std::endl;
            m_isMapsModalOpen = false;
            m_suppressPlacementUntilRelease = true;
            if (m_isInitialModalLaunch) {
                returnToOrigin();
            }
            return true;
        }
    }

    return true;
}

void MapEditorState::renderRenameModal() {
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.78f));

    RenameModalLayout l = getRenameModalLayout();

    // Modal panel & border
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    // Header
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    // Close cross [X]
    bool hovCross = isPointInRect(m_mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f, hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.35f, 0.38f, 0.45f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f, hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.22f, 0.28f));

    // Input box
    m_renderer->drawSprite(m_whiteTexture, l.boxPos, l.boxSize, 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.boxPos + glm::vec2(1.0f), l.boxSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.08f, 0.10f, 0.14f));

    // Save button
    bool hovSave = isPointInRect(m_mousePos, l.btnSavePos, l.btnSaveSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnSavePos, l.btnSaveSize, 0.0f, hovSave ? glm::vec3(0.3f, 0.9f, 0.45f) : glm::vec3(0.2f, 0.7f, 0.35f));
    m_renderer->drawSprite(m_whiteTexture, l.btnSavePos + glm::vec2(1.0f), l.btnSaveSize - glm::vec2(2.0f), 0.0f, hovSave ? glm::vec3(0.18f, 0.38f, 0.22f) : glm::vec3(0.14f, 0.30f, 0.18f));

    // Cancel button
    bool hovCancel = isPointInRect(m_mousePos, l.btnCancelPos, l.btnCancelSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCancelPos, l.btnCancelSize, 0.0f, hovCancel ? glm::vec3(0.6f, 0.25f, 0.25f) : glm::vec3(0.45f, 0.20f, 0.20f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCancelPos + glm::vec2(1.0f), l.btnCancelSize - glm::vec2(2.0f), 0.0f, hovCancel ? glm::vec3(0.28f, 0.14f, 0.14f) : glm::vec3(0.22f, 0.11f, 0.11f));

    m_renderer->flush();

    if (m_textRenderer) {
        // Title
        std::string titleStr = LOC("RENAME_TITLE");
        float titleW = m_textRenderer->CalculateTextWidth(titleStr, l.fTitle);
        float titleX = l.modalPos.x + (l.modalSize.x - titleW) * 0.5f;
        float titleY = l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(titleStr, titleX, titleY, l.fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

        // Cross text
        float crossW = m_textRenderer->CalculateTextWidth("x", l.fCross);
        m_textRenderer->RenderText("x", l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f,
                                  l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 2.0f,
                                  l.fCross, glm::vec3(0.9f));

        // Subtitle
        std::string sub = "File: " + m_renameTargetFileName;
        m_textRenderer->RenderText(sub, l.boxPos.x + 2.0f, l.subY, l.fSub, glm::vec3(0.70f, 0.75f, 0.85f));

        // Input text
        bool showCursor = (m_cursorBlinkTimer < 0.5f);
        std::string displayText = m_renameInputText + (showCursor ? "|" : "");
        float inputY = l.boxPos.y + (l.boxSize.y - l.fInput * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(displayText, l.boxPos.x + 10.0f, inputY, l.fInput, glm::vec3(0.40f, 0.95f, 1.0f));

        // Save button text
        std::string saveStr = LOC("RENAME_SAVE") + " (Enter)";
        float sTxtW = m_textRenderer->CalculateTextWidth(saveStr, l.fBtn);
        float sTxtX = l.btnSavePos.x + (l.btnSaveSize.x - sTxtW) * 0.5f;
        float sTxtY = l.btnSavePos.y + (l.btnSaveSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(saveStr, sTxtX, sTxtY, l.fBtn, glm::vec3(0.95f));

        // Cancel button text
        std::string cancelStr = LOC("RENAME_CANCEL") + " (Esc)";
        float cTxtW = m_textRenderer->CalculateTextWidth(cancelStr, l.fBtn);
        float cTxtX = l.btnCancelPos.x + (l.btnCancelSize.x - cTxtW) * 0.5f;
        float cTxtY = l.btnCancelPos.y + (l.btnCancelSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(cancelStr, cTxtX, cTxtY, l.fBtn, glm::vec3(0.95f));
    }
}

void MapEditorState::renderMapsModal() {
    MapsModalLayout l = getMapsModalLayout();

    if (m_diagRenderFrames < 6) {
        std::cout << "[MapEditor] renderMapsModal() drawing frame " << m_diagRenderFrames
                  << ": modalPos=(" << l.modalPos.x << "," << l.modalPos.y
                  << "), modalSize=(" << l.modalSize.x << "," << l.modalSize.y
                  << "), totalLevels=" << l.totalLevels << ", visibleCards=" << l.visibleCards.size() << std::endl;
        m_diagRenderFrames++;
    }

    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.78f));

    // Modal background & borders
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));

    // Header
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));

    // Close cross [X] in header
    bool hovCross = isPointInRect(m_mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f, hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.35f, 0.38f, 0.45f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f, hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.22f, 0.28f));

    // "+ New Map" button
    bool hovNew = isPointInRect(m_mousePos, l.btnNewPos, l.btnNewSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnNewPos, l.btnNewSize, 0.0f, hovNew ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.80f, 0.40f));
    m_renderer->drawSprite(m_whiteTexture, l.btnNewPos + glm::vec2(1.0f), l.btnNewSize - glm::vec2(2.0f), 0.0f, hovNew ? glm::vec3(0.16f, 0.36f, 0.20f) : glm::vec3(0.12f, 0.28f, 0.16f));

    // Quick scroll buttons in subheader if pagination
    if (l.hasPagination) {
        bool canUp = (m_mapsScrollOffset > 0);
        bool canDown = (m_mapsScrollOffset < l.maxOffset);
        bool hovUp = isPointInRect(m_mousePos, l.btnScrollUpPos, l.btnScrollUpSize);
        bool hovDown = isPointInRect(m_mousePos, l.btnScrollDownPos, l.btnScrollDownSize);

        glm::vec3 upBorder = canUp ? (hovUp ? glm::vec3(0.50f, 0.85f, 1.0f) : glm::vec3(0.35f, 0.55f, 0.75f)) : glm::vec3(0.22f, 0.25f, 0.30f);
        glm::vec3 upBg = canUp ? (hovUp ? glm::vec3(0.22f, 0.32f, 0.44f) : glm::vec3(0.16f, 0.22f, 0.30f)) : glm::vec3(0.12f, 0.14f, 0.18f);
        m_renderer->drawSprite(m_whiteTexture, l.btnScrollUpPos, l.btnScrollUpSize, 0.0f, upBorder);
        m_renderer->drawSprite(m_whiteTexture, l.btnScrollUpPos + glm::vec2(1.0f), l.btnScrollUpSize - glm::vec2(2.0f), 0.0f, upBg);

        glm::vec3 downBorder = canDown ? (hovDown ? glm::vec3(0.50f, 0.85f, 1.0f) : glm::vec3(0.35f, 0.55f, 0.75f)) : glm::vec3(0.22f, 0.25f, 0.30f);
        glm::vec3 downBg = canDown ? (hovDown ? glm::vec3(0.22f, 0.32f, 0.44f) : glm::vec3(0.16f, 0.22f, 0.30f)) : glm::vec3(0.12f, 0.14f, 0.18f);
        m_renderer->drawSprite(m_whiteTexture, l.btnScrollDownPos, l.btnScrollDownSize, 0.0f, downBorder);
        m_renderer->drawSprite(m_whiteTexture, l.btnScrollDownPos + glm::vec2(1.0f), l.btnScrollDownSize - glm::vec2(2.0f), 0.0f, downBg);
    }

    // Render cards
    for (const auto& card : l.visibleCards) {
        const auto& lvl = card.levelInfo;

        bool isCurrent = (lvl.filename == m_currentLevelFileName);
        glm::vec3 cardBg = isCurrent ? glm::vec3(0.18f, 0.24f, 0.35f) : glm::vec3(0.15f, 0.16f, 0.21f);
        glm::vec3 cardBorder = isCurrent ? glm::vec3(0.40f, 0.85f, 1.0f) : glm::vec3(0.28f, 0.30f, 0.38f);

        m_renderer->drawSprite(m_whiteTexture, card.cardPos, card.cardSize, 0.0f, cardBorder);
        m_renderer->drawSprite(m_whiteTexture, card.cardPos + glm::vec2(1.0f), card.cardSize - glm::vec2(2.0f), 0.0f, cardBg);

        // [Load] button
        bool hovLoad = isPointInRect(m_mousePos, card.btnLoadPos, card.btnLoadSize);
        m_renderer->drawSprite(m_whiteTexture, card.btnLoadPos, card.btnLoadSize, 0.0f, hovLoad ? glm::vec3(0.40f, 0.85f, 1.0f) : glm::vec3(0.25f, 0.55f, 0.80f));
        m_renderer->drawSprite(m_whiteTexture, card.btnLoadPos + glm::vec2(1.0f), card.btnLoadSize - glm::vec2(2.0f), 0.0f, hovLoad ? glm::vec3(0.16f, 0.30f, 0.45f) : glm::vec3(0.12f, 0.22f, 0.35f));

        // [Rename] button
        bool hovRen = isPointInRect(m_mousePos, card.btnRenPos, card.btnRenSize);
        m_renderer->drawSprite(m_whiteTexture, card.btnRenPos, card.btnRenSize, 0.0f, hovRen ? glm::vec3(1.0f, 0.85f, 0.35f) : glm::vec3(0.75f, 0.65f, 0.25f));
        m_renderer->drawSprite(m_whiteTexture, card.btnRenPos + glm::vec2(1.0f), card.btnRenSize - glm::vec2(2.0f), 0.0f, hovRen ? glm::vec3(0.38f, 0.30f, 0.14f) : glm::vec3(0.28f, 0.22f, 0.10f));

        // [Del] button
        if (card.hasDel) {
            bool hovDel = isPointInRect(m_mousePos, card.btnDelPos, card.btnDelSize);
            m_renderer->drawSprite(m_whiteTexture, card.btnDelPos, card.btnDelSize, 0.0f, hovDel ? glm::vec3(0.95f, 0.30f, 0.30f) : glm::vec3(0.70f, 0.20f, 0.20f));
            m_renderer->drawSprite(m_whiteTexture, card.btnDelPos + glm::vec2(1.0f), card.btnDelSize - glm::vec2(2.0f), 0.0f, hovDel ? glm::vec3(0.40f, 0.15f, 0.15f) : glm::vec3(0.28f, 0.10f, 0.10f));
        }
    }

    // Scrollbar track and thumb on right side
    if (l.hasPagination && !l.visibleCards.empty()) {
        float trackX = l.modalPos.x + l.modalSize.x - std::clamp(16.0f * GetUIScale(m_width, m_height), 12.0f, 20.0f);
        float trackY = l.listStartY;
        float trackH = static_cast<float>(l.visibleCards.size()) * (l.itemH + l.itemGap) - l.itemGap;
        float trackW = std::clamp(6.0f * GetUIScale(m_width, m_height), 4.0f, 8.0f);

        m_renderer->drawSprite(m_whiteTexture, glm::vec2(trackX, trackY), glm::vec2(trackW, trackH), 0.0f, glm::vec3(0.18f, 0.20f, 0.25f));

        float thumbH = std::max(20.0f, trackH * (static_cast<float>(l.maxVisible) / static_cast<float>(l.totalLevels)));
        float thumbRatio = (l.maxOffset > 0) ? (static_cast<float>(m_mapsScrollOffset) / static_cast<float>(l.maxOffset)) : 0.0f;
        float thumbY = trackY + (trackH - thumbH) * thumbRatio;
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(trackX, thumbY), glm::vec2(trackW, thumbH), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    }

    // Bottom [Close] button
    bool hovClose = isPointInRect(m_mousePos, l.btnClosePos, l.btnCloseSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnClosePos, l.btnCloseSize, 0.0f, hovClose ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.35f, 0.38f, 0.45f));
    m_renderer->drawSprite(m_whiteTexture, l.btnClosePos + glm::vec2(1.0f), l.btnCloseSize - glm::vec2(2.0f), 0.0f, hovClose ? glm::vec3(0.22f, 0.25f, 0.30f) : glm::vec3(0.16f, 0.18f, 0.22f));

    // Bottom Prev / Next buttons if pagination
    if (l.hasPagination) {
        bool canPrev = (m_mapsScrollOffset > 0);
        bool canNext = (m_mapsScrollOffset < l.maxOffset);
        bool hovPrev = isPointInRect(m_mousePos, l.btnPrevPagePos, l.btnPrevPageSize);
        bool hovNext = isPointInRect(m_mousePos, l.btnNextPagePos, l.btnNextPageSize);

        glm::vec3 prevBorder = canPrev ? (hovPrev ? glm::vec3(0.40f, 0.80f, 1.0f) : glm::vec3(0.30f, 0.55f, 0.85f)) : glm::vec3(0.24f, 0.26f, 0.32f);
        glm::vec3 prevBg = canPrev ? (hovPrev ? glm::vec3(0.20f, 0.32f, 0.46f) : glm::vec3(0.14f, 0.22f, 0.32f)) : glm::vec3(0.12f, 0.13f, 0.16f);
        m_renderer->drawSprite(m_whiteTexture, l.btnPrevPagePos, l.btnPrevPageSize, 0.0f, prevBorder);
        m_renderer->drawSprite(m_whiteTexture, l.btnPrevPagePos + glm::vec2(1.0f), l.btnPrevPageSize - glm::vec2(2.0f), 0.0f, prevBg);

        glm::vec3 nextBorder = canNext ? (hovNext ? glm::vec3(0.40f, 0.80f, 1.0f) : glm::vec3(0.30f, 0.55f, 0.85f)) : glm::vec3(0.24f, 0.26f, 0.32f);
        glm::vec3 nextBg = canNext ? (hovNext ? glm::vec3(0.20f, 0.32f, 0.46f) : glm::vec3(0.14f, 0.22f, 0.32f)) : glm::vec3(0.12f, 0.13f, 0.16f);
        m_renderer->drawSprite(m_whiteTexture, l.btnNextPagePos, l.btnNextPageSize, 0.0f, nextBorder);
        m_renderer->drawSprite(m_whiteTexture, l.btnNextPagePos + glm::vec2(1.0f), l.btnNextPageSize - glm::vec2(2.0f), 0.0f, nextBg);
    }

    m_renderer->flush();

    if (m_textRenderer) {
        // Title
        std::string titleStr = "LEVELS LIST";
        float titleW = m_textRenderer->CalculateTextWidth(titleStr, l.fTitle);
        float titleX = l.modalPos.x + (l.modalSize.x - titleW) * 0.5f;
        float titleY = l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(titleStr, titleX, titleY, l.fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

        // Cross text
        float crossW = m_textRenderer->CalculateTextWidth("x", l.fCross);
        m_textRenderer->RenderText("x", l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f,
                                  l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 2.0f,
                                  l.fCross, glm::vec3(0.9f));

        // Subtitle
        float subY = l.modalPos.y + l.headerH + std::clamp(10.0f * GetUIScale(m_width, m_height), 8.0f, 16.0f);
        m_textRenderer->RenderText("Select, rename, or create maps:", l.modalPos.x + 22.0f, subY + 4.0f, l.fSub, glm::vec3(0.70f, 0.75f, 0.85f));

        // "+ New Map" text
        float newTxtW = m_textRenderer->CalculateTextWidth("+ New Map", l.fNew);
        float newTxtX = l.btnNewPos.x + (l.btnNewSize.x - newTxtW) * 0.5f;
        float newTxtY = l.btnNewPos.y + (l.btnNewSize.y - l.fNew * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText("+ New Map", newTxtX, newTxtY, l.fNew, glm::vec3(0.95f));

        // Subheader scroll arrows [^] and [v]
        if (l.hasPagination) {
            float upW = m_textRenderer->CalculateTextWidth("^", l.fNew);
            m_textRenderer->RenderText("^", l.btnScrollUpPos.x + (l.btnScrollUpSize.x - upW) * 0.5f,
                                      l.btnScrollUpPos.y + (l.btnScrollUpSize.y - l.fNew * 28.0f) * 0.5f + 2.0f,
                                      l.fNew, (m_mapsScrollOffset > 0) ? glm::vec3(0.95f) : glm::vec3(0.45f));

            float downW = m_textRenderer->CalculateTextWidth("v", l.fNew);
            m_textRenderer->RenderText("v", l.btnScrollDownPos.x + (l.btnScrollDownSize.x - downW) * 0.5f,
                                      l.btnScrollDownPos.y + (l.btnScrollDownSize.y - l.fNew * 28.0f) * 0.5f + 2.0f,
                                      l.fNew, (m_mapsScrollOffset < l.maxOffset) ? glm::vec3(0.95f) : glm::vec3(0.45f));
        }

        // Render card text
        for (const auto& card : l.visibleCards) {
            const auto& lvl = card.levelInfo;

            bool isCurrent = (lvl.filename == m_currentLevelFileName);
            std::string label = lvl.name;
            if (isCurrent) label += "  [ACTIVE]";
            glm::vec3 nameCol = isCurrent ? glm::vec3(0.40f, 1.0f, 0.60f) : (lvl.isBuiltIn ? glm::vec3(1.0f, 0.85f, 0.3f) : glm::vec3(0.92f, 0.94f, 0.98f));

            float textLeftX = card.cardPos.x + std::clamp(14.0f * GetUIScale(m_width, m_height), 10.0f, 18.0f);
            float totalH = (l.fCardName + l.fCardSub) * 28.0f + 2.0f;
            float nameY = card.cardPos.y + (card.cardSize.y - totalH) * 0.5f + 1.0f;
            float subCardY = nameY + l.fCardName * 28.0f + 1.0f;

            // Ensure name doesn't overlap action buttons
            float maxLabelW = card.btnLoadPos.x - textLeftX - 10.0f;
            std::string displayLabel = label;
            if (m_textRenderer->CalculateTextWidth(displayLabel, l.fCardName) > maxLabelW) {
                while (!displayLabel.empty() && m_textRenderer->CalculateTextWidth(displayLabel + "...", l.fCardName) > maxLabelW) {
                    InputManager::popUtf8(displayLabel);
                }
                displayLabel += "...";
            }
            m_textRenderer->RenderText(displayLabel, textLeftX, nameY, l.fCardName, nameCol);

            std::string sub = lvl.filename + (lvl.isBuiltIn ? " (Campaign)" : " (Custom)");
            m_textRenderer->RenderText(sub, textLeftX, subCardY, l.fCardSub, glm::vec3(0.60f, 0.65f, 0.75f));

            // [Load] text
            float loadW = m_textRenderer->CalculateTextWidth("Load", l.fActionBtn);
            float loadX = card.btnLoadPos.x + (card.btnLoadSize.x - loadW) * 0.5f;
            float loadY = card.btnLoadPos.y + (card.btnLoadSize.y - l.fActionBtn * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText("Load", loadX, loadY, l.fActionBtn, glm::vec3(0.95f));

            // [Rename] text
            float renW = m_textRenderer->CalculateTextWidth("Rename", l.fActionBtn);
            float renX = card.btnRenPos.x + (card.btnRenSize.x - renW) * 0.5f;
            float renY = card.btnRenPos.y + (card.btnRenSize.y - l.fActionBtn * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText("Rename", renX, renY, l.fActionBtn, glm::vec3(0.95f));

            // [Del] text
            if (card.hasDel) {
                float delW = m_textRenderer->CalculateTextWidth("Del", l.fActionBtn);
                float delX = card.btnDelPos.x + (card.btnDelSize.x - delW) * 0.5f;
                float delY = card.btnDelPos.y + (card.btnDelSize.y - l.fActionBtn * 28.0f) * 0.5f + 2.0f;
                m_textRenderer->RenderText("Del", delX, delY, l.fActionBtn, glm::vec3(0.95f));
            }
        }

        // [Close] text
        float closeW = m_textRenderer->CalculateTextWidth("Close", l.fClose);
        float closeX = l.btnClosePos.x + (l.btnCloseSize.x - closeW) * 0.5f;
        float closeY = l.btnClosePos.y + (l.btnCloseSize.y - l.fClose * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText("Close", closeX, closeY, l.fClose, glm::vec3(0.95f));

        // [ < Prev ] and [ Next > ] texts
        if (l.hasPagination) {
            float pW = m_textRenderer->CalculateTextWidth("< Prev", l.fClose);
            float pX = l.btnPrevPagePos.x + (l.btnPrevPageSize.x - pW) * 0.5f;
            float pY = l.btnPrevPagePos.y + (l.btnPrevPageSize.y - l.fClose * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText("< Prev", pX, pY, l.fClose, (m_mapsScrollOffset > 0) ? glm::vec3(0.95f) : glm::vec3(0.45f));

            float nW = m_textRenderer->CalculateTextWidth("Next >", l.fClose);
            float nX = l.btnNextPagePos.x + (l.btnNextPageSize.x - nW) * 0.5f;
            float nY = l.btnNextPagePos.y + (l.btnNextPageSize.y - l.fClose * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText("Next >", nX, nY, l.fClose, (m_mapsScrollOffset < l.maxOffset) ? glm::vec3(0.95f) : glm::vec3(0.45f));

            // Page/item count text on the left
            int startItem = l.currentOffset + 1;
            int endItem = std::min(l.totalLevels, l.currentOffset + l.maxVisible);
            std::string countStr = std::to_string(startItem) + "-" + std::to_string(endItem) + " / " + std::to_string(l.totalLevels);
            float cFont = std::clamp(0.44f * GetUIScale(m_width, m_height), 0.32f, 0.56f);
            float cY = l.btnClosePos.y + (l.btnCloseSize.y - cFont * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText(countStr, l.modalPos.x + std::clamp(20.0f * GetUIScale(m_width, m_height), 14.0f, 26.0f), cY, cFont, glm::vec3(0.65f, 0.70f, 0.80f));
        }
    }
}

void MapEditorState::openExitModal() {
    if (!m_isDirty) {
        std::cout << "[MapEditor] openExitModal: Map is not dirty (unmodified), exiting directly to origin." << std::endl;
        returnToOrigin();
        return;
    }
    std::cout << "[MapEditor] openExitModal: exit confirmation dialog opened" << std::endl;
    m_isExitModalOpen = true;
    m_suppressPlacementUntilRelease = true;
    m_suppressClickUntilRelease = true;
    m_exitModalEscReleased = false;
}

void MapEditorState::closeExitModal() {
    std::cout << "[MapEditor] closeExitModal: exit dialog closed (staying in editor)" << std::endl;
    m_isExitModalOpen = false;
    m_suppressPlacementUntilRelease = true;
    m_exitModalEscReleased = false;
    m_keyEscPressedLastFrame = true;
}

MapEditorState::ExitModalLayout MapEditorState::getExitModalLayout() const {
    ExitModalLayout layout;
    float scale = GetUIScale(m_width, m_height);

    layout.fTitle = std::clamp(0.68f * scale, 0.48f, 0.88f);
    layout.fQuestion = std::clamp(0.66f * scale, 0.46f, 0.84f);
    layout.fSub = std::clamp(0.48f * scale, 0.35f, 0.64f);
    layout.fBtn = std::clamp(0.46f * scale, 0.34f, 0.60f);
    layout.fCross = std::clamp(0.55f * scale, 0.40f, 0.75f);

    std::string saveExitStr = LOC("EDITOR_EXIT_SAVE_AND_EXIT");
    std::string discardStr = LOC("EDITOR_EXIT_DISCARD");
    std::string cancelStr = LOC("EDITOR_EXIT_CANCEL");

    float w1 = m_textRenderer ? m_textRenderer->CalculateTextWidth(saveExitStr, layout.fBtn) : 130.0f;
    float w2 = m_textRenderer ? m_textRenderer->CalculateTextWidth(discardStr, layout.fBtn) : 140.0f;
    float w3 = m_textRenderer ? m_textRenderer->CalculateTextWidth(cancelStr, layout.fBtn) : 60.0f;

    float btnPad = std::clamp(14.0f * scale, 8.0f, 20.0f);
    float req1 = w1 + btnPad * 2.0f;
    float req2 = w2 + btnPad * 2.0f;
    float req3 = w3 + btnPad * 2.0f;
    float totalReq = req1 + req2 + req3;

    float sideMargin = std::clamp(18.0f * scale, 12.0f, 26.0f);
    float btnGap = std::clamp(12.0f * scale, 8.0f, 16.0f);

    float desiredW = std::max(540.0f * scale, totalReq + 2.0f * sideMargin + 2.0f * btnGap);
    layout.modalSize.x = std::clamp(desiredW, 380.0f, static_cast<float>(m_width) - 40.0f);
    layout.modalSize.y = std::clamp(220.0f * scale, 170.0f, static_cast<float>(m_height) - 40.0f);

    layout.modalPos = glm::vec2((static_cast<float>(m_width) - layout.modalSize.x) * 0.5f,
                                (static_cast<float>(m_height) - layout.modalSize.y) * 0.5f);

    layout.headerH = std::clamp(40.0f * scale, 30.0f, 56.0f);

    float crossSize = std::clamp(26.0f * scale, 20.0f, 36.0f);
    layout.btnCloseCrossSize = glm::vec2(crossSize, crossSize);
    layout.btnCloseCrossPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - crossSize - std::clamp(8.0f * scale, 6.0f, 12.0f),
                                        layout.modalPos.y + (layout.headerH - crossSize) * 0.5f);

    float btnH = std::clamp(42.0f * scale, 32.0f, 54.0f);
    float btnMarginBottom = std::clamp(18.0f * scale, 12.0f, 24.0f);
    float btnY = layout.modalPos.y + layout.modalSize.y - btnH - btnMarginBottom;

    float availW = layout.modalSize.x - 2.0f * sideMargin - 2.0f * btnGap;
    float btn1W, btn2W, btn3W;
    if (availW >= totalReq) {
        float extra = availW - totalReq;
        btn1W = req1 + extra * (req1 / totalReq);
        btn2W = req2 + extra * (req2 / totalReq);
        btn3W = req3 + extra * (req3 / totalReq);
    } else {
        float ratio = availW / std::max(1.0f, totalReq);
        btn1W = req1 * ratio;
        btn2W = req2 * ratio;
        btn3W = req3 * ratio;
    }

    layout.btnSaveExitPos = glm::vec2(layout.modalPos.x + sideMargin, btnY);
    layout.btnSaveExitSize = glm::vec2(btn1W, btnH);

    layout.btnDiscardPos = glm::vec2(layout.btnSaveExitPos.x + btn1W + btnGap, btnY);
    layout.btnDiscardSize = glm::vec2(btn2W, btnH);

    layout.btnCancelPos = glm::vec2(layout.btnDiscardPos.x + btn2W + btnGap, btnY);
    layout.btnCancelSize = glm::vec2(btn3W, btnH);

    float contentTop = layout.modalPos.y + layout.headerH;
    float contentH = btnY - contentTop;
    layout.questionY = contentTop + contentH * 0.28f;
    layout.subY = contentTop + contentH * 0.62f;

    return layout;
}

bool MapEditorState::processExitModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isExitModalOpen) return false;

    // 1. Подавление клика мыши, которым открыли модалку
    if (m_suppressPlacementUntilRelease) {
        if (!leftDown) {
            m_suppressPlacementUntilRelease = false;
        }
    }

    // 2. Горячая клавиша Escape:
    // При открытии m_exitModalEscReleased = false.
    // Когда пользователь отпустит Escape (если открыл окно по Escape), взводится m_exitModalEscReleased = true.
    // При следующем нажатии Escape — подтверждаем выход без сохранения!
    bool keyEsc = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    if (!keyEsc) {
        m_exitModalEscReleased = true;
        m_keyEscPressedLastFrame = false;
    } else if (m_exitModalEscReleased) {
        std::cout << "[MapEditor] Exit Modal: ESC pressed -> exiting without saving" << std::endl;
        returnToOrigin();
        return true;
    }

    // 3. Горячая клавиша Enter — сохранить и выйти
    bool keyEnter = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS);
    if (keyEnter) {
        std::cout << "[MapEditor] Exit Modal: Enter pressed -> saving and exiting" << std::endl;
        saveMap();
        returnToOrigin();
        return true;
    }

    ExitModalLayout l = getExitModalLayout();

    // 4. Клики мыши обрабатываются только после отпускания кнопки мыши после открытия
    if (!m_suppressPlacementUntilRelease && leftDown && !m_isLeftMouseDown) {
        // Кнопка [ Сохранить и выйти ]
        if (isPointInRect(mousePos, l.btnSaveExitPos, l.btnSaveExitSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [Save and Exit] -> saving and exiting" << std::endl;
            saveMap();
            returnToOrigin();
            return true;
        }

        // Кнопка [ Выйти без сохранения ]
        if (isPointInRect(mousePos, l.btnDiscardPos, l.btnDiscardSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [Discard and Exit] -> exiting without saving" << std::endl;
            returnToOrigin();
            return true;
        }

        // Кнопка [ Отмена ]
        if (isPointInRect(mousePos, l.btnCancelPos, l.btnCancelSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [Cancel]" << std::endl;
            closeExitModal();
            return true;
        }

        // Крестик [X]
        if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [X] close cross" << std::endl;
            closeExitModal();
            return true;
        }

        // Клик вне модального окна -> закрываем (Отмена)
        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked outside modal -> cancelling" << std::endl;
            closeExitModal();
            return true;
        }
    }

    return true;
}

void MapEditorState::renderExitModal() {
    // Полупрозрачный фон-затемнение
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.78f));

    ExitModalLayout l = getExitModalLayout();

    // Основная подложка и 2px обводка
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.18f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));

    // Шапка
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.25f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));

    // Кнопка-крестик [X]
    bool hovCross = isPointInRect(m_mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f, hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.35f, 0.38f, 0.45f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f, hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.22f, 0.28f));

    // Кнопки действий
    // 1. [ Сохранить и выйти ]
    bool hovSaveExit = isPointInRect(m_mousePos, l.btnSaveExitPos, l.btnSaveExitSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnSaveExitPos, l.btnSaveExitSize, 0.0f, hovSaveExit ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.75f, 0.38f));
    m_renderer->drawSprite(m_whiteTexture, l.btnSaveExitPos + glm::vec2(2.0f), l.btnSaveExitSize - glm::vec2(4.0f), 0.0f, hovSaveExit ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.30f, 0.18f));

    // 2. [ Выйти без сохранения ]
    bool hovDiscard = isPointInRect(m_mousePos, l.btnDiscardPos, l.btnDiscardSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnDiscardPos, l.btnDiscardSize, 0.0f, hovDiscard ? glm::vec3(0.95f, 0.35f, 0.35f) : glm::vec3(0.75f, 0.25f, 0.25f));
    m_renderer->drawSprite(m_whiteTexture, l.btnDiscardPos + glm::vec2(2.0f), l.btnDiscardSize - glm::vec2(4.0f), 0.0f, hovDiscard ? glm::vec3(0.40f, 0.16f, 0.16f) : glm::vec3(0.28f, 0.12f, 0.12f));

    // 3. [ Отмена ]
    bool hovCancel = isPointInRect(m_mousePos, l.btnCancelPos, l.btnCancelSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCancelPos, l.btnCancelSize, 0.0f, hovCancel ? glm::vec3(0.60f, 0.65f, 0.75f) : glm::vec3(0.40f, 0.44f, 0.52f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCancelPos + glm::vec2(2.0f), l.btnCancelSize - glm::vec2(4.0f), 0.0f, hovCancel ? glm::vec3(0.25f, 0.28f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.25f));

    m_renderer->flush();

    // Тексты в модальном окне
    if (m_textRenderer) {
        // Заголовок в шапке
        std::string titleStr = LOC("EDITOR_EXIT_TITLE");
        float tW = m_textRenderer->CalculateTextWidth(titleStr, l.fTitle);
        m_textRenderer->RenderText(titleStr, l.modalPos.x + (l.modalSize.x - tW) * 0.5f,
                                  l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 2.0f,
                                  l.fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

        // Крестик [X]
        float crossW = m_textRenderer->CalculateTextWidth("x", l.fCross);
        m_textRenderer->RenderText("x", l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f,
                                  l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 2.0f,
                                  l.fCross, glm::vec3(0.9f));

        // Основной вопрос
        std::string questionStr = LOC("EDITOR_EXIT_QUESTION");
        float qW = m_textRenderer->CalculateTextWidth(questionStr, l.fQuestion);
        m_textRenderer->RenderText(questionStr, l.modalPos.x + (l.modalSize.x - qW) * 0.5f, l.questionY, l.fQuestion, glm::vec3(1.0f, 1.0f, 1.0f));

        // Поясняющий подтекст
        std::string subStr = LOC("EDITOR_EXIT_SUB");
        float sW = m_textRenderer->CalculateTextWidth(subStr, l.fSub);
        m_textRenderer->RenderText(subStr, l.modalPos.x + (l.modalSize.x - sW) * 0.5f, l.subY, l.fSub, glm::vec3(0.85f, 0.65f, 0.65f));

        // Текст кнопки 1: Сохранить и выйти
        std::string saveExitStr = LOC("EDITOR_EXIT_SAVE_AND_EXIT");
        float seW = m_textRenderer->CalculateTextWidth(saveExitStr, l.fBtn);
        float seX = l.btnSaveExitPos.x + (l.btnSaveExitSize.x - seW) * 0.5f;
        float seY = l.btnSaveExitPos.y + (l.btnSaveExitSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(saveExitStr, seX, seY, l.fBtn, glm::vec3(0.95f, 1.0f, 0.95f));

        // Текст кнопки 2: Выйти без сохранения
        std::string discardStr = LOC("EDITOR_EXIT_DISCARD");
        float dW = m_textRenderer->CalculateTextWidth(discardStr, l.fBtn);
        float dX = l.btnDiscardPos.x + (l.btnDiscardSize.x - dW) * 0.5f;
        float dY = l.btnDiscardPos.y + (l.btnDiscardSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(discardStr, dX, dY, l.fBtn, glm::vec3(1.0f, 0.90f, 0.90f));

        // Текст кнопки 3: Отмена
        std::string cancelStr = LOC("EDITOR_EXIT_CANCEL");
        float cW = m_textRenderer->CalculateTextWidth(cancelStr, l.fBtn);
        float cX = l.btnCancelPos.x + (l.btnCancelSize.x - cW) * 0.5f;
        float cY = l.btnCancelPos.y + (l.btnCancelSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(cancelStr, cX, cY, l.fBtn, glm::vec3(0.90f, 0.92f, 0.96f));
    }
}

// =============================================================================
// МОДАЛЬНЕ ВІКНО: НАЛАШТУВАННЯ ВАГОНЕТКИ (Крок 4)
// =============================================================================

MapEditorState::MinecartModalLayout MapEditorState::getMinecartModalLayout() const {
    MinecartModalLayout l;
    float scale = GetUIScale(m_width, m_height);
    float mW = std::clamp(500.0f * scale, 370.0f, 600.0f);
    float mH = std::clamp(390.0f * scale, 310.0f, 460.0f);
    l.modalPos = glm::vec2((m_width - mW) * 0.5f, (m_height - mH) * 0.5f);
    l.modalSize = glm::vec2(mW, mH);
    l.headerH = std::clamp(38.0f * scale, 32.0f, 44.0f);
    l.pad = std::clamp(16.0f * scale, 12.0f, 20.0f);

    float crossSz = std::clamp(24.0f * scale, 20.0f, 28.0f);
    l.btnCloseCrossPos = glm::vec2(l.modalPos.x + mW - crossSz - 8.0f, l.modalPos.y + (l.headerH - crossSz) * 0.5f);
    l.btnCloseCrossSize = glm::vec2(crossSz);

    float lineH = std::clamp(22.0f * scale, 17.0f, 26.0f);
    float curY = l.modalPos.y + l.headerH + l.pad;
    l.statusY = curY; curY += lineH;
    l.depotY = curY; curY += lineH;
    l.endY = curY; curY += lineH + l.pad * 0.6f;

    float btnH = std::clamp(28.0f * scale, 24.0f, 34.0f);
    float smallBtnW = std::clamp(50.0f * scale, 40.0f, 60.0f);
    float rowGap = std::clamp(8.0f * scale, 5.0f, 10.0f);
    float rowTotalH = btnH + rowGap;

    float btnIncX = l.modalPos.x + mW - l.pad - smallBtnW;
    float btnDecX = btnIncX - smallBtnW - 6.0f;

    // Row 1: Trip Interval
    l.row1Y = curY;
    l.btnIntDecPos = glm::vec2(btnDecX, curY);
    l.btnIntDecSize = glm::vec2(smallBtnW, btnH);
    l.btnIntIncPos = glm::vec2(btnIncX, curY);
    l.btnIntIncSize = glm::vec2(smallBtnW, btnH);
    curY += rowTotalH;

    // Row 2: Warning Time
    l.row2Y = curY;
    l.btnWarnDecPos = glm::vec2(btnDecX, curY);
    l.btnWarnDecSize = glm::vec2(smallBtnW, btnH);
    l.btnWarnIncPos = glm::vec2(btnIncX, curY);
    l.btnWarnIncSize = glm::vec2(smallBtnW, btnH);
    curY += rowTotalH;

    // Row 3: Speed
    l.row3Y = curY;
    l.btnSpeedDecPos = glm::vec2(btnDecX, curY);
    l.btnSpeedDecSize = glm::vec2(smallBtnW, btnH);
    l.btnSpeedIncPos = glm::vec2(btnIncX, curY);
    l.btnSpeedIncSize = glm::vec2(smallBtnW, btnH);

    // Нижній блок кнопок
    float bottomBtnH = std::clamp(32.0f * scale, 26.0f, 38.0f);
    float bottomY = l.modalPos.y + mH - l.pad - bottomBtnH;

    float clearW = std::clamp(170.0f * scale, 130.0f, 210.0f);
    l.btnClearRoutePos = glm::vec2(l.modalPos.x + l.pad, bottomY);
    l.btnClearRouteSize = glm::vec2(clearW, bottomBtnH);

    float closeW = std::clamp(96.0f * scale, 76.0f, 120.0f);
    l.btnClosePos = glm::vec2(l.modalPos.x + mW - l.pad - closeW, bottomY);
    l.btnCloseSize = glm::vec2(closeW, bottomBtnH);

    l.fTitle = std::clamp(0.60f * scale, 0.48f, 0.72f);
    l.fLabel = std::clamp(0.44f * scale, 0.35f, 0.52f);
    l.fValue = std::clamp(0.46f * scale, 0.36f, 0.54f);
    l.fBtn = std::clamp(0.42f * scale, 0.34f, 0.50f);
    l.fCross = std::clamp(0.50f * scale, 0.40f, 0.60f);

    return l;
}

bool MapEditorState::processMinecartSettingsInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown) {
    if (!m_isMinecartSettingsOpen) return false;

    // Закриття по Escape
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_isMinecartSettingsOpen = false;
        m_keyEscPressedLastFrame = true;
        return true;
    }

    MinecartModalLayout l = getMinecartModalLayout();

    if (leftDown && !m_isLeftMouseDown) {
        // Клік повз вікно -> закрити
        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            m_isMinecartSettingsOpen = false;
            return true;
        }

        // Хрестик [X]
        if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
            m_isMinecartSettingsOpen = false;
            return true;
        }

        // Кнопка [Закрити]
        if (isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize)) {
            m_isMinecartSettingsOpen = false;
            return true;
        }

        // Кнопка [Очистити маршрут]
        if (isPointInRect(mousePos, l.btnClearRoutePos, l.btnClearRouteSize)) {
            if (m_minecarts.empty()) {
                m_minecarts.push_back(MinecartData{});
            }
            m_minecarts[0].start = glm::ivec2(-1, -1);
            m_minecarts[0].end = glm::ivec2(-1, -1);
            m_isDirty = true;
            recalculateRailPath();
            return true;
        }

        // Налаштування числових параметрів
        if (!m_minecarts.empty()) {
            auto& mc = m_minecarts[0];

            // Інтервал рейсу [-5s] / [+5s] (5.0 - 120.0s)
            if (isPointInRect(mousePos, l.btnIntDecPos, l.btnIntDecSize)) {
                mc.interval = std::clamp(mc.interval - 5.0f, 5.0f, 120.0f);
                m_isDirty = true;
                return true;
            }
            if (isPointInRect(mousePos, l.btnIntIncPos, l.btnIntIncSize)) {
                mc.interval = std::clamp(mc.interval + 5.0f, 5.0f, 120.0f);
                m_isDirty = true;
                return true;
            }

            // Час попередження [-0.5s] / [+0.5s] (1.0 - 10.0s)
            if (isPointInRect(mousePos, l.btnWarnDecPos, l.btnWarnDecSize)) {
                mc.warningTime = std::clamp(mc.warningTime - 0.5f, 1.0f, 10.0f);
                m_isDirty = true;
                return true;
            }
            if (isPointInRect(mousePos, l.btnWarnIncPos, l.btnWarnIncSize)) {
                mc.warningTime = std::clamp(mc.warningTime + 0.5f, 1.0f, 10.0f);
                m_isDirty = true;
                return true;
            }

            // Швидкість [-1.0] / [+1.0] (2.0 - 25.0 кл/с)
            if (isPointInRect(mousePos, l.btnSpeedDecPos, l.btnSpeedDecSize)) {
                mc.speed = std::clamp(mc.speed - 1.0f, 2.0f, 25.0f);
                m_isDirty = true;
                return true;
            }
            if (isPointInRect(mousePos, l.btnSpeedIncPos, l.btnSpeedIncSize)) {
                mc.speed = std::clamp(mc.speed + 1.0f, 2.0f, 25.0f);
                m_isDirty = true;
                return true;
            }
        }
    }

    // Поглинаємо всі кліки, поки модалка активна (не малюємо по сітці)
    return true;
}

void MapEditorState::renderMinecartSettingsModal() {
    if (!m_isMinecartSettingsOpen) return;

    MinecartModalLayout l = getMinecartModalLayout();

    // 1. Напівпрозорий фон-затемнення
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.76f));

    // 2. Основна панель та 2px золотисто-бурштинова обводка
    glm::vec3 borderCol = glm::vec3(0.95f, 0.75f, 0.20f);
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, borderCol);
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, borderCol);
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, borderCol);
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, borderCol);

    // 3. Шапка модального вікна
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.19f, 0.16f, 0.10f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, borderCol);

    // 4. Кнопка-хрестик [X]
    bool hovCross = isPointInRect(m_mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f,
                           hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.40f, 0.36f, 0.28f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f,
                           hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.18f, 0.14f));

    // 5. Кнопки параметрів ([-] та [+]) з hover-підсвічуванням
    auto drawParamBtn = [&](glm::vec2 pos, glm::vec2 size, bool isInc) {
        bool hov = isPointInRect(m_mousePos, pos, size);
        glm::vec3 bCol, bgCol;
        if (isInc) {
            bCol  = hov ? glm::vec3(0.40f, 0.90f, 0.45f) : glm::vec3(0.25f, 0.60f, 0.30f);
            bgCol = hov ? glm::vec3(0.18f, 0.38f, 0.20f) : glm::vec3(0.13f, 0.25f, 0.15f);
        } else {
            bCol  = hov ? glm::vec3(0.90f, 0.50f, 0.25f) : glm::vec3(0.60f, 0.35f, 0.18f);
            bgCol = hov ? glm::vec3(0.38f, 0.20f, 0.10f) : glm::vec3(0.24f, 0.14f, 0.08f);
        }
        m_renderer->drawSprite(m_whiteTexture, pos, size, 0.0f, bCol);
        m_renderer->drawSprite(m_whiteTexture, pos + glm::vec2(1.0f), size - glm::vec2(2.0f), 0.0f, bgCol);
    };

    drawParamBtn(l.btnIntDecPos, l.btnIntDecSize, false);
    drawParamBtn(l.btnIntIncPos, l.btnIntIncSize, true);
    drawParamBtn(l.btnWarnDecPos, l.btnWarnDecSize, false);
    drawParamBtn(l.btnWarnIncPos, l.btnWarnIncSize, true);
    drawParamBtn(l.btnSpeedDecPos, l.btnSpeedDecSize, false);
    drawParamBtn(l.btnSpeedIncPos, l.btnSpeedIncSize, true);

    // 6. Нижні кнопки: [Очистити маршрут] та [Закрити]
    bool hovClear = isPointInRect(m_mousePos, l.btnClearRoutePos, l.btnClearRouteSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnClearRoutePos, l.btnClearRouteSize, 0.0f,
                           hovClear ? glm::vec3(0.90f, 0.35f, 0.35f) : glm::vec3(0.65f, 0.25f, 0.25f));
    m_renderer->drawSprite(m_whiteTexture, l.btnClearRoutePos + glm::vec2(1.0f), l.btnClearRouteSize - glm::vec2(2.0f), 0.0f,
                           hovClear ? glm::vec3(0.40f, 0.15f, 0.15f) : glm::vec3(0.25f, 0.11f, 0.11f));

    bool hovClose = isPointInRect(m_mousePos, l.btnClosePos, l.btnCloseSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnClosePos, l.btnCloseSize, 0.0f,
                           hovClose ? glm::vec3(0.60f, 0.65f, 0.75f) : glm::vec3(0.38f, 0.42f, 0.50f));
    m_renderer->drawSprite(m_whiteTexture, l.btnClosePos + glm::vec2(1.0f), l.btnCloseSize - glm::vec2(2.0f), 0.0f,
                           hovClose ? glm::vec3(0.25f, 0.28f, 0.34f) : glm::vec3(0.17f, 0.19f, 0.24f));

    m_renderer->flush();

    // 7. Текстовий шар (канонічне вертикальне центрування за правилами td-ui)
    if (!m_textRenderer) return;

    // Заголовок у шапці
    std::string titleStr = LOC("EDITOR_MINECART_TITLE");
    float tW = m_textRenderer->CalculateTextWidth(titleStr, l.fTitle);
    float tX = l.modalPos.x + (l.modalSize.x - tW) * 0.5f;
    float tY = l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 1.0f;
    m_textRenderer->RenderText(titleStr, tX, tY, l.fTitle, glm::vec3(1.0f, 0.88f, 0.30f));

    // Хрестик [X]
    float crossW = m_textRenderer->CalculateTextWidth("x", l.fCross);
    float cX = l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f;
    float cY = l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 1.0f;
    m_textRenderer->RenderText("x", cX, cY, l.fCross, glm::vec3(0.92f));

    // Блок статусу колії та маркерів
    const MinecartData& mc = m_minecarts.empty() ? MinecartData{} : m_minecarts[0];
    bool hasRoute = mc.hasStart() && mc.hasEnd();

    std::string statusText;
    glm::vec3 statusColor;
    if (!hasRoute) {
        statusText = "Статус колії: маркери не встановлено";
        statusColor = glm::vec3(0.70f, 0.72f, 0.75f);
    } else if (m_isRailPathValid) {
        statusText = "[✓] " + LOC("EDITOR_MINECART_STATUS_OK") + " (" +
                     std::to_string(static_cast<int>(m_railPath.size())) + " кл.)";
        statusColor = glm::vec3(0.30f, 1.0f, 0.55f);
    } else {
        statusText = "[!] " + LOC("EDITOR_MINECART_STATUS_BROKEN");
        statusColor = glm::vec3(1.0f, 0.30f, 0.30f);
    }
    m_textRenderer->RenderText(statusText, l.modalPos.x + l.pad, l.statusY, l.fLabel, statusColor);

    // Координати Депо та Тупика
    std::string depotStr = LOC("EDITOR_RAIL_START") + ": " +
                           (mc.hasStart() ? ("(" + std::to_string(mc.start.x) + ", " + std::to_string(mc.start.y) + ")") : "---");
    m_textRenderer->RenderText(depotStr, l.modalPos.x + l.pad, l.depotY, l.fLabel, glm::vec3(0.95f, 0.78f, 0.30f));

    std::string endStr = LOC("EDITOR_RAIL_END") + ": " +
                         (mc.hasEnd() ? ("(" + std::to_string(mc.end.x) + ", " + std::to_string(mc.end.y) + ")") : "---");
    m_textRenderer->RenderText(endStr, l.modalPos.x + l.pad, l.endY, l.fLabel, glm::vec3(0.95f, 0.45f, 0.45f));

    // Рядки параметрів (назва зліва, значення перед кнопками, мітки всередині кнопок)
    auto renderParamRow = [&](float rowY, const std::string& label, const std::string& valStr,
                              glm::vec2 decPos, glm::vec2 decSz, const std::string& decLbl,
                              glm::vec2 incPos, glm::vec2 incSz, const std::string& incLbl) {
        // Назва параметра зліва
        float lblY = rowY + (decSz.y - l.fLabel * 28.0f) * 0.5f + 1.0f;
        m_textRenderer->RenderText(label, l.modalPos.x + l.pad, lblY, l.fLabel, glm::vec3(0.88f));

        // Числове значення (праворуч, перед кнопкою [-])
        float valW = m_textRenderer->CalculateTextWidth(valStr, l.fValue);
        float valX = decPos.x - valW - 12.0f;
        float valY = rowY + (decSz.y - l.fValue * 28.0f) * 0.5f + 1.0f;
        m_textRenderer->RenderText(valStr, valX, valY, l.fValue, glm::vec3(1.0f, 0.90f, 0.45f));

        // Напис кнопки [-]
        float dTW = m_textRenderer->CalculateTextWidth(decLbl, l.fBtn);
        float dTX = decPos.x + (decSz.x - dTW) * 0.5f;
        float dTY = decPos.y + (decSz.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
        m_textRenderer->RenderText(decLbl, dTX, dTY, l.fBtn, glm::vec3(1.0f, 0.75f, 0.50f));

        // Напис кнопки [+]
        float iTW = m_textRenderer->CalculateTextWidth(incLbl, l.fBtn);
        float iTX = incPos.x + (incSz.x - iTW) * 0.5f;
        float iTY = incPos.y + (incSz.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
        m_textRenderer->RenderText(incLbl, iTX, iTY, l.fBtn, glm::vec3(0.60f, 1.0f, 0.65f));
    };

    // 1. Інтервал рейсу
    std::string intValStr = std::to_string(static_cast<int>(std::round(mc.interval))) + " s";
    renderParamRow(l.row1Y, LOC("EDITOR_MINECART_INTERVAL"), intValStr,
                   l.btnIntDecPos, l.btnIntDecSize, "-5s",
                   l.btnIntIncPos, l.btnIntIncSize, "+5s");

    // 2. Час попередження
    char warnBuf[32];
    std::snprintf(warnBuf, sizeof(warnBuf), "%.1f s", mc.warningTime);
    renderParamRow(l.row2Y, LOC("EDITOR_MINECART_WARNING"), std::string(warnBuf),
                   l.btnWarnDecPos, l.btnWarnDecSize, "-0.5s",
                   l.btnWarnIncPos, l.btnWarnIncSize, "+0.5s");

    // 3. Швидкість вагонетки
    std::string spValStr = std::to_string(static_cast<int>(std::round(mc.speed))) + " кл/с";
    renderParamRow(l.row3Y, LOC("EDITOR_MINECART_SPEED"), spValStr,
                   l.btnSpeedDecPos, l.btnSpeedDecSize, "-1.0",
                   l.btnSpeedIncPos, l.btnSpeedIncSize, "+1.0");

    // Кнопка [Очистити маршрут]
    std::string clrStr = LOC("EDITOR_MINECART_CLEAR_ROUTE");
    float clrW = m_textRenderer->CalculateTextWidth(clrStr, l.fBtn);
    float clrX = l.btnClearRoutePos.x + (l.btnClearRouteSize.x - clrW) * 0.5f;
    float clrY = l.btnClearRoutePos.y + (l.btnClearRouteSize.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
    m_textRenderer->RenderText(clrStr, clrX, clrY, l.fBtn, glm::vec3(1.0f, 0.85f, 0.85f));

    // Кнопка [Закрити]
    std::string clsStr = LOC("EDITOR_MINECART_CLOSE");
    float clsW = m_textRenderer->CalculateTextWidth(clsStr, l.fBtn);
    float clsX = l.btnClosePos.x + (l.btnCloseSize.x - clsW) * 0.5f;
    float clsY = l.btnClosePos.y + (l.btnCloseSize.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
    m_textRenderer->RenderText(clsStr, clsX, clsY, l.fBtn, glm::vec3(0.92f, 0.94f, 0.98f));
}


