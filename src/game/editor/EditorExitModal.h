#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

class SpriteRenderer;
class TextRenderer;
class Texture2D;

enum class ExitModalAction {
    None,
    SaveAndExit,
    DiscardAndExit,
    Cancel
};

struct ExitModalLayout {
    glm::vec2 modalPos{0.0f};
    glm::vec2 modalSize{0.0f};
    float headerH = 40.0f;
    glm::vec2 btnCloseCrossPos{0.0f};
    glm::vec2 btnCloseCrossSize{0.0f};
    glm::vec2 btnSaveExitPos{0.0f};
    glm::vec2 btnSaveExitSize{0.0f};
    glm::vec2 btnDiscardPos{0.0f};
    glm::vec2 btnDiscardSize{0.0f};
    glm::vec2 btnCancelPos{0.0f};
    glm::vec2 btnCancelSize{0.0f};
    float fTitle = 0.68f;
    float fQuestion = 0.66f;
    float fSub = 0.48f;
    float fBtn = 0.46f;
    float fCross = 0.55f;
    float questionY = 0.0f;
    float subY = 0.0f;
};

class EditorExitModal {
public:
    EditorExitModal() = default;

    bool isOpen() const { return m_isOpen; }
    void open(bool isDirty = true);
    void close();

    ExitModalLayout layoutExitModal(float screenW, float screenH, TextRenderer* textRenderer = nullptr) const;

    ExitModalAction handleInput(glm::vec2 mousePos, bool mouseClicked, int key = 0, bool keyReleased = false);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenW, float screenH);
    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                float screenW, float screenH, glm::vec2 mousePos);

private:
    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);

    bool m_isOpen = false;
    bool m_suppressClickUntilRelease = false;
    bool m_escReleased = false;
    glm::vec2 m_mousePos{0.0f};
    float m_lastScreenW = 1280.0f;
    float m_lastScreenH = 720.0f;
};

