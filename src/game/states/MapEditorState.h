#pragma once
#include "IGameState.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"
#include "../world/Grid.h"
#include "../world/Pathfinder.h"
#include "../ui/PathRenderer.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
class GameStateManager;

enum class EditorBrush {
    Ground = 0,    // 0: Земля (можно строить и ходить)
    Wall = 1,      // 3: Стена/Вода (нельзя строить, нельзя ходить)
    Platform = 2,  // 2: Платформа (можно строить, нельзя ходить)
    Path = 3,      // 1: Дорога (нельзя строить, можно ходить)
    Spawner = 4,   // Точка спавна врагов
    Base = 5,      // База игрока
    Eraser = 6,    // Ластик: стирает тайлы, спавнеры и базы до Земли
    Chasm = 7,     // 4: Шурф / Обрыв (нельзя строить, нельзя ходить)
    Rail = 8,      // 5: Рельсы (нельзя строить, можно ходить)
    RailStart = 9, // Маркер Депо / Старт вагонетки
    RailEnd = 10   // Маркер Тупик / Финиш вагонетки
};

struct EditorButton {
    glm::vec2 pos;
    glm::vec2 size;
    std::string label;
    EditorBrush brush = EditorBrush::Wall;
    bool isAction = false;
    int actionId = 0; // 1: Save, 2: Test, 3: Clear, 4: Exit, 5: ID-, 6: ID Cycle, 7: ID+, 8: Preset, 9: W-, 10: W+, 11: H-, 12: H+
};

class MapEditorState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width;
    int m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;

    std::shared_ptr<Texture2D> m_whiteTexture;
    std::shared_ptr<Texture2D> m_uiTexture;
    std::shared_ptr<Texture2D> m_arrowTexture;

    // Сетка и данные карты
    int m_gridWidth = 20;
    int m_gridHeight = 12;
    float m_cellSize = 64.0f;
    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<Pathfinder> m_pathfinder;
    PathVisualizer m_pathVisualizer;

    std::vector<SpawnerData> m_spawners;
    std::vector<BaseData> m_bases;
    std::vector<std::vector<int>> m_rawLayout; // 0: Ground, 1: Path, 2: Platform, 3: Scenery, 4: Chasm, 5: Rail
    std::vector<MinecartData> m_minecarts;
    std::vector<glm::ivec2> m_railPath;        // BFS-маршрут від start до end по рейках
    bool m_isRailPathValid = false;            // true якщо шлях знайдено
    std::vector<std::vector<glm::ivec2>> m_activePaths;
    bool m_hasInvalidSpawner = false;
    bool m_missingBaseWarning = false;
    int m_missingBaseId = -1;

    // Режим кисти и выбранный ID
    EditorBrush m_currentBrush = EditorBrush::Wall;
    int m_selectedId = -1; // -1 = Auto/Nearest, 0, 1, 2, 3, 4...

    // Данные и состояние редактора волн
    std::vector<WaveConfig> m_waves;
    int m_selectedWaveIdx = 0;
    int m_wavePartsScrollOffset = 0;
    bool m_isWaveEditorOpen = false;
    bool m_keyWPressedLastFrame = false;

    bool m_isLeftMouseDown = false;
    bool m_isRightMouseDown = false;
    bool m_keySPressedLastFrame = false;
    bool m_keyTPressedLastFrame = false;
    bool m_keyCPressedLastFrame = false;
    bool m_keyEscPressedLastFrame = false;
    bool m_keyLeftBracketLastFrame = false;
    bool m_keyRightBracketLastFrame = false;

    std::vector<EditorButton> m_bottomButtons;
    std::vector<EditorButton> m_topButtons;

    std::string m_editorSavePath = "res/levels/level_editor.json";
    std::string m_currentLevelFileName = "level_editor.json";
    std::string m_currentLevelDisplayName = "level_editor";
    bool m_isCampaign = false;
    std::vector<std::string> m_tags;
    bool m_suppressPlacementUntilRelease = true;
    bool m_suppressClickUntilRelease = true;

    // Состояния модальных окон управления картами
    bool m_isRenameModalOpen = false;
    std::string m_renameInputText = "";
    std::string m_renameTargetFileName = "";
    bool m_isMapsModalOpen = false;
    int m_mapsScrollOffset = 0;
    bool m_isExitModalOpen = false;
    bool m_exitModalEscReleased = false;

    // Флаг изменений и авто-открытия модального окна карт при запуске
    bool m_isDirty = false;
    bool m_isInitialModalLaunch = false;
    int m_diagInputFrames = 0;
    int m_diagRenderFrames = 0;

    std::string m_statusMessage = "";
    float m_statusTimer = 0.0f;
    glm::vec3 m_statusColor = glm::vec3(0.9f, 0.9f, 0.9f);
    float m_topBarLeftEndX = 540.0f;
    float m_topFontScale = 0.50f;
    float m_bottomFontScale = 0.48f;

    float getTopBarHeight() const;
    float getBottomDockHeight() const;

    void updateButtonLayout();
    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const;
    void recalculatePaths();
    void recalculateRailPath(); // BFS-валідація маршруту рейок від start до end
    void applyBrush(int gridX, int gridY, EditorBrush brush);
    void eraseCell(int gridX, int gridY);
    void saveMap();
    void loadInitialMap();
    void loadLevelByName(const std::string& fileName);
    void updateCurrentLevelDisplayName();
    void testMap();
    void clearMap();
    void cycleSelectedId(int step);
    void resizeMap(int newW, int newH);
    void cycleMapSizePreset();

    // Модальне вікно налаштувань вагонетки
    bool m_isMinecartSettingsOpen = false;
    void renderMinecartSettingsModal();
    bool processMinecartSettingsInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown);

    struct MinecartModalLayout {
        glm::vec2 modalPos;
        glm::vec2 modalSize;
        float headerH = 40.0f;
        glm::vec2 btnCloseCrossPos;
        glm::vec2 btnCloseCrossSize;
        float pad = 16.0f;
        float statusY = 0.0f;
        float depotY = 0.0f;
        float endY = 0.0f;
        float row1Y = 0.0f;
        glm::vec2 btnIntDecPos;
        glm::vec2 btnIntDecSize;
        glm::vec2 btnIntIncPos;
        glm::vec2 btnIntIncSize;
        float row2Y = 0.0f;
        glm::vec2 btnWarnDecPos;
        glm::vec2 btnWarnDecSize;
        glm::vec2 btnWarnIncPos;
        glm::vec2 btnWarnIncSize;
        float row3Y = 0.0f;
        glm::vec2 btnSpeedDecPos;
        glm::vec2 btnSpeedDecSize;
        glm::vec2 btnSpeedIncPos;
        glm::vec2 btnSpeedIncSize;
        glm::vec2 btnClearRoutePos;
        glm::vec2 btnClearRouteSize;
        glm::vec2 btnClosePos;
        glm::vec2 btnCloseSize;
        float fTitle = 0.60f;
        float fLabel = 0.44f;
        float fValue = 0.46f;
        float fBtn = 0.42f;
        float fCross = 0.50f;
    };
    MinecartModalLayout getMinecartModalLayout() const;

    struct ExitModalLayout {
        glm::vec2 modalPos;
        glm::vec2 modalSize;
        float headerH = 40.0f;
        glm::vec2 btnCloseCrossPos;
        glm::vec2 btnCloseCrossSize;
        glm::vec2 btnSaveExitPos;
        glm::vec2 btnSaveExitSize;
        glm::vec2 btnDiscardPos;
        glm::vec2 btnDiscardSize;
        glm::vec2 btnCancelPos;
        glm::vec2 btnCancelSize;
        float fTitle = 0.68f;
        float fQuestion = 0.66f;
        float fSub = 0.48f;
        float fBtn = 0.46f;
        float fCross = 0.55f;
        float questionY = 0.0f;
        float subY = 0.0f;
    };
    ExitModalLayout getExitModalLayout() const;

    struct MapCardLayout {
        glm::vec2 cardPos;
        glm::vec2 cardSize;
        glm::vec2 btnLoadPos;
        glm::vec2 btnLoadSize;
        glm::vec2 btnRenPos;
        glm::vec2 btnRenSize;
        glm::vec2 btnDelPos;
        glm::vec2 btnDelSize;
        bool hasDel = false;
        int levelIdx = 0;
        LevelInfo levelInfo;
    };

    std::vector<LevelInfo> getFilteredModalLevels() const;

    struct MapsModalLayout {
        glm::vec2 modalPos;
        glm::vec2 modalSize;
        float headerH = 40.0f;
        glm::vec2 btnCloseCrossPos;
        glm::vec2 btnCloseCrossSize;
        glm::vec2 btnNewPos;
        glm::vec2 btnNewSize;
        glm::vec2 btnClosePos;
        glm::vec2 btnCloseSize;
        glm::vec2 btnScrollUpPos;
        glm::vec2 btnScrollUpSize;
        glm::vec2 btnScrollDownPos;
        glm::vec2 btnScrollDownSize;
        glm::vec2 btnPrevPagePos;
        glm::vec2 btnPrevPageSize;
        glm::vec2 btnNextPagePos;
        glm::vec2 btnNextPageSize;
        bool hasPagination = false;
        int totalLevels = 0;
        int maxVisible = 6;
        int currentOffset = 0;
        int maxOffset = 0;
        float listStartY = 0.0f;
        float itemH = 48.0f;
        float itemGap = 6.0f;
        float fTitle = 0.68f;
        float fSub = 0.50f;
        float fNew = 0.50f;
        float fCardName = 0.54f;
        float fCardSub = 0.42f;
        float fActionBtn = 0.48f;
        float fClose = 0.52f;
        float fCross = 0.55f;
        std::vector<MapCardLayout> visibleCards;
    };
    MapsModalLayout getMapsModalLayout() const;

    struct RenameModalLayout {
        glm::vec2 modalPos;
        glm::vec2 modalSize;
        float headerH = 40.0f;
        glm::vec2 btnCloseCrossPos;
        glm::vec2 btnCloseCrossSize;
        glm::vec2 boxPos;
        glm::vec2 boxSize;
        glm::vec2 btnSavePos;
        glm::vec2 btnSaveSize;
        glm::vec2 btnCancelPos;
        glm::vec2 btnCancelSize;
        float fTitle = 0.68f;
        float fSub = 0.48f;
        float fInput = 0.58f;
        float fBtn = 0.48f;
        float fCross = 0.55f;
        float subY = 0.0f;
    };
    RenameModalLayout getRenameModalLayout() const;

    void openRenameModal(const std::string& targetFileName = "");
    void confirmRename();
    void renderRenameModal();
    void renderMapsModal();
    bool processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);
    bool processMapsModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);

    void openExitModal();
    void closeExitModal();
    void renderExitModal();
    bool processExitModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);

    enum class FocusedField {
        None,
        Count,
        Interval,
        Delay
    };

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    glm::vec2 m_mousePos = glm::vec2(0.0f);
    FocusedField m_focusedField = FocusedField::None;
    int m_focusedPartIdx = -1;
    std::string m_inputText = "";
    bool m_fieldJustFocused = false;
    float m_cursorBlinkTimer = 0.0f;
    int m_openDropdownPartIdx = -1; // -1 = закрыт

    void commitFocusedInput();
    void startEditingField(int partIdx, FocusedField field);
    void cycleNextInputField();

    void renderWaveEditor();
    bool processWaveEditorInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);
    EditorOrigin m_origin = EditorOrigin::MainMenu;
    void returnToOrigin();

public:
    MapEditorState(GameStateManager& stateManager, int width, int height,
                   std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer,
                   const std::string& levelToLoad = "",
                   EditorOrigin origin = EditorOrigin::MainMenu);
    ~MapEditorState() override = default;

    glm::vec3 getIdColor(int id) const;

    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};

