#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "../core/LevelManager.h"
#include "../core/WaveData.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GLFWwindow;

enum class WaveFocusedField {
    None,
    Count,
    Interval,
    Delay
};

struct WaveModalLayout {
    float scale = 1.0f;
    glm::vec2 modalPos{0.0f};
    glm::vec2 modalSize{0.0f};
    float headerH = 40.0f;
    float footerH = 32.0f;

    // Header close button
    glm::vec2 btnClosePos{0.0f};
    glm::vec2 btnCloseSize{0.0f};

    // Left column (waves list)
    glm::vec2 colLeftPos{0.0f};
    glm::vec2 colLeftSize{0.0f};
    glm::vec2 btnAddWavePos{0.0f};
    glm::vec2 btnAddWaveSize{0.0f};
    glm::vec2 btnDuplicateWavePos{0.0f};
    glm::vec2 btnDuplicateWaveSize{0.0f};
    float waveListStartY = 0.0f;
    float waveItemH = 36.0f;
    float waveItemGap = 4.0f;
    int maxVisibleWaves = 8;
    glm::vec2 btnWaveScrollUpPos{0.0f};
    glm::vec2 btnWaveScrollDownPos{0.0f};
    glm::vec2 btnWaveScrollSize{0.0f};

    // Right column (selected wave configurator)
    glm::vec2 colRightPos{0.0f};
    glm::vec2 colRightSize{0.0f};
    float rHeaderH = 36.0f;
    glm::vec2 btnAddPartPos{0.0f};
    glm::vec2 btnAddPartSize{0.0f};

    // Batches list
    float cardsStartY = 0.0f;
    float cardH = 88.0f;
    float cardGap = 8.0f;
    int maxVisibleParts = 4;
};

class EditorWaveModal {
public:
    EditorWaveModal() = default;

    void open();
    void open(std::vector<WaveConfig>& waves);
    void open(LevelMapData& mapData);

    void close();

    void toggle();
    void toggle(std::vector<WaveConfig>& waves);
    void toggle(LevelMapData& mapData);

    bool isOpen() const { return m_isOpen; }

    void update(float dt);

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt,
                     int screenWidth, int screenHeight, float topBarHeight, float bottomDockHeight,
                     std::vector<WaveConfig>& waves, bool& isDirty);

    bool handleInput(float mouseX, float mouseY, bool mousePressed, LevelMapData& mapData, int key = -1, int action = -1);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                int screenWidth, int screenHeight, float topBarHeight, float bottomDockHeight,
                glm::vec2 mousePos, const std::vector<WaveConfig>& waves);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenWidth, float screenHeight, const LevelMapData& mapData);

    void commitFocusedInput(std::vector<WaveConfig>& waves);
    void startEditingField(int partIdx, WaveFocusedField field, const std::vector<WaveConfig>& waves);
    void cycleNextInputField(std::vector<WaveConfig>& waves);

    int getSelectedWaveIdx() const { return m_selectedWaveIdx; }
    void setSelectedWaveIdx(int idx) { m_selectedWaveIdx = idx; }

    const WaveModalLayout& getLayout() const { return m_layout; }
    void updateLayout(int screenWidth, int screenHeight);

    static std::string getSpawnerLabel(int spawnerId);
    static glm::vec3 getSpawnerColor(int spawnerId);

private:
    bool m_isOpen = false;
    bool m_wasLeftDown = false;
    LevelMapData* m_boundMapData = nullptr;

    int m_selectedWaveIdx = 0;
    int m_wavePartsScrollOffset = 0;
    int m_wavesScrollOffset = 0;

    WaveFocusedField m_focusedField = WaveFocusedField::None;
    int m_focusedPartIdx = -1;
    std::string m_inputText = "";
    bool m_fieldJustFocused = false;
    float m_cursorBlinkTimer = 0.0f;
    int m_openDropdownPartIdx = -1; // -1 = closed

    WaveModalLayout m_layout;

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    static std::string formatFloat1(float val);
};

