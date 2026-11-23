#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include "../core/LevelManager.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GLFWwindow;

struct MapCardLayout {
    glm::vec2 cardPos{0.0f};
    glm::vec2 cardSize{0.0f};
    glm::vec2 btnLoadPos{0.0f};
    glm::vec2 btnLoadSize{0.0f};
    glm::vec2 btnRenPos{0.0f};
    glm::vec2 btnRenSize{0.0f};
    glm::vec2 btnDelPos{0.0f};
    glm::vec2 btnDelSize{0.0f};
    bool hasDel = false;
    int levelIdx = 0;
    LevelInfo levelInfo;
};

struct MapsModalLayout {
    glm::vec2 modalPos{0.0f};
    glm::vec2 modalSize{0.0f};
    float headerH = 40.0f;
    glm::vec2 btnCloseCrossPos{0.0f};
    glm::vec2 btnCloseCrossSize{0.0f};
    glm::vec2 btnNewPos{0.0f};
    glm::vec2 btnNewSize{0.0f};
    glm::vec2 btnClosePos{0.0f};
    glm::vec2 btnCloseSize{0.0f};
    glm::vec2 btnScrollUpPos{0.0f};
    glm::vec2 btnScrollUpSize{0.0f};
    glm::vec2 btnScrollDownPos{0.0f};
    glm::vec2 btnScrollDownSize{0.0f};
    glm::vec2 btnPrevPagePos{0.0f};
    glm::vec2 btnPrevPageSize{0.0f};
    glm::vec2 btnNextPagePos{0.0f};
    glm::vec2 btnNextPageSize{0.0f};
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

struct RenameModalLayout {
    glm::vec2 modalPos{0.0f};
    glm::vec2 modalSize{0.0f};
    float headerH = 40.0f;
    glm::vec2 btnCloseCrossPos{0.0f};
    glm::vec2 btnCloseCrossSize{0.0f};
    glm::vec2 boxPos{0.0f};
    glm::vec2 boxSize{0.0f};
    glm::vec2 btnSavePos{0.0f};
    glm::vec2 btnSaveSize{0.0f};
    glm::vec2 btnCancelPos{0.0f};
    glm::vec2 btnCancelSize{0.0f};
    float fTitle = 0.68f;
    float fSub = 0.48f;
    float fInput = 0.58f;
    float fBtn = 0.48f;
    float fCross = 0.55f;
    float subY = 0.0f;
};

class EditorMapsBrowserModal {
public:
    EditorMapsBrowserModal() = default;

    void open(const std::string& currentLevelFileName = "", bool isInitialLaunch = false);
    void close();
    void toggle(const std::string& currentLevelFileName = "");
    void refreshFiles();

    void openRename(const std::string& targetFileName, const std::string& currentDisplayName);
    void closeRename();

    bool isOpen() const { return m_isOpen || m_isRenameModalOpen; }
    bool isMapsOpen() const { return m_isOpen; }
    bool isRenameOpen() const { return m_isRenameModalOpen; }

    void update(float dt);

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt,
                     int screenWidth, int screenHeight,
                     const std::function<void(const std::string&)>& onLoadLevel,
                     const std::function<void()>& onNewLevel,
                     const std::function<void(const std::string&, const std::string&, const std::string&)>& onRenameLevel,
                     const std::function<void()>& onReturnToOrigin);

    bool handleInput(float mouseX, float mouseY, bool mousePressed,
                     LevelMapData& currentMap,
                     std::function<void(const std::string&)> onLoadMap,
                     std::function<void()> onNewMap);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                int screenWidth, int screenHeight, glm::vec2 mousePos);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                float screenWidth, float screenHeight);

    std::vector<LevelInfo> getFilteredModalLevels() const;
    MapsModalLayout getMapsModalLayout(int screenWidth, int screenHeight) const;
    RenameModalLayout getRenameModalLayout(int screenWidth, int screenHeight) const;

    void setCurrentLevelFileName(const std::string& fileName) { m_currentLevelFileName = fileName; }
    const std::string& getCurrentLevelFileName() const { return m_currentLevelFileName; }

private:
    bool m_isOpen = false;
    bool m_isRenameModalOpen = false;
    bool m_isInitialModalLaunch = false;
    bool m_wasLeftDown = false;
    int m_mapsScrollOffset = 0;

    std::string m_currentLevelFileName = "";
    std::string m_renameTargetFileName = "";
    std::string m_renameInputText = "";
    float m_cursorBlinkTimer = 0.0f;

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    bool checkKeyRepeat(GLFWwindow* window, int key, float dt);
    void confirmRename(const std::function<void(const std::string&, const std::string&, const std::string&)>& onRenameLevel);

    void renderMapsModal(SpriteRenderer* renderer, TextRenderer* textRenderer,
                         const Texture2D* whiteTexture,
                         int screenWidth, int screenHeight, glm::vec2 mousePos);

    void renderRenameModal(SpriteRenderer* renderer, TextRenderer* textRenderer,
                           const Texture2D* whiteTexture,
                           int screenWidth, int screenHeight, glm::vec2 mousePos);
};

