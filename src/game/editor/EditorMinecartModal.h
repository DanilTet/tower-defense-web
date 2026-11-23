#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "../core/LevelManager.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GLFWwindow;

struct MinecartModalLayout {
    glm::vec2 modalPos{0.0f};
    glm::vec2 modalSize{0.0f};
    float headerH = 40.0f;
    glm::vec2 btnCloseCrossPos{0.0f};
    glm::vec2 btnCloseCrossSize{0.0f};
    float pad = 16.0f;
    float statusY = 0.0f;
    float depotY = 0.0f;
    float endY = 0.0f;
    float row1Y = 0.0f;
    glm::vec2 btnIntDecPos{0.0f};
    glm::vec2 btnIntDecSize{0.0f};
    glm::vec2 btnIntIncPos{0.0f};
    glm::vec2 btnIntIncSize{0.0f};
    float row2Y = 0.0f;
    glm::vec2 btnWarnDecPos{0.0f};
    glm::vec2 btnWarnDecSize{0.0f};
    glm::vec2 btnWarnIncPos{0.0f};
    glm::vec2 btnWarnIncSize{0.0f};
    float row3Y = 0.0f;
    glm::vec2 btnSpeedDecPos{0.0f};
    glm::vec2 btnSpeedDecSize{0.0f};
    glm::vec2 btnSpeedIncPos{0.0f};
    glm::vec2 btnSpeedIncSize{0.0f};
    glm::vec2 btnClearRoutePos{0.0f};
    glm::vec2 btnClearRouteSize{0.0f};
    glm::vec2 btnClosePos{0.0f};
    glm::vec2 btnCloseSize{0.0f};
    float fTitle = 0.60f;
    float fLabel = 0.44f;
    float fValue = 0.46f;
    float fBtn = 0.42f;
    float fCross = 0.50f;
};

class EditorMinecartModal {
public:
    EditorMinecartModal() = default;

    void open();
    void open(LevelMapData& mapData);
    void close();
    void toggle();
    bool isOpen() const { return m_isOpen; }

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                     int screenWidth, int screenHeight,
                     std::vector<MinecartData>& minecarts, bool& isDirty,
                     const std::function<void()>& onRouteCleared = nullptr);

    bool handleInput(float mouseX, float mouseY, bool mousePressed, LevelMapData& mapData);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                int screenWidth, int screenHeight,
                glm::vec2 mousePos,
                const std::vector<MinecartData>& minecarts,
                bool isRailPathValid,
                size_t railPathSize);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenWidth, float screenHeight);

    const MinecartModalLayout& getLayout() const { return m_layout; }
    void updateLayout(int screenWidth, int screenHeight);

private:
    bool m_isOpen = false;
    bool m_wasLeftDown = false;
    LevelMapData* m_boundMapData = nullptr;
    MinecartModalLayout m_layout;

    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
};

