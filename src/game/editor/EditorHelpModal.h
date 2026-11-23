#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GLFWwindow;

class EditorHelpModal {
public:
    EditorHelpModal() = default;

    void open() { m_isOpen = true; m_justOpened = true; m_wasLeftDown = true; }
    void close() { m_isOpen = false; m_justOpened = false; }
    void toggle() { if (m_isOpen) close(); else open(); }
    bool isOpen() const { return m_isOpen; }

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                     bool leftReleased, bool isEscJustPressed);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                int screenWidth, int screenHeight);

private:
    bool m_isOpen = false;
    bool m_wasLeftDown = false;
    bool m_justOpened = false;
};

