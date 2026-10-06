#pragma once
#include "IGameState.h"
#include <memory>
#include <glm/glm.hpp>
#include "../ui/VolumeSliderWidget.h"

class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;

class SettingsState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    std::shared_ptr<Texture2D> m_whiteTexture;
    std::shared_ptr<Texture2D> m_uiTexture;

    glm::vec2 m_windowPos;
    glm::vec2 m_windowSize;
    float m_headerHeight = 40.0f;
    bool m_isDragging = false;
    glm::vec2 m_dragOffset{ 0.0f, 0.0f };
    float m_uiScale = 1.0f;

    // Relative Y coordinates inside window
    float m_volRelY = 0.0f;
    float m_langRelY = 0.0f;
    float m_scaleLabelRelY = 0.0f;
    float m_scaleBtnsRelY = 0.0f;
    float m_closeRelY = 0.0f;

    bool m_mousePressedLastFrame = false;
    glm::vec2 m_currentMousePos{ 0.0f, 0.0f };

    VolumeSliderWidget m_volumeWidget;

    glm::vec2 m_langBtnRuPos{ 0.0f, 0.0f };
    glm::vec2 m_langBtnUaPos{ 0.0f, 0.0f };
    glm::vec2 m_langBtnEnPos{ 0.0f, 0.0f };
    glm::vec2 m_langBtnSize{ 80.0f, 34.0f };

    struct UIScalePresetBtn {
        int percent = 100;
        glm::vec2 pos{ 0.0f, 0.0f };
        glm::vec2 size{ 64.0f, 32.0f };
    };
    std::vector<UIScalePresetBtn> m_uiScaleBtns;

    glm::vec2 m_closeBtnPos{ 0.0f, 0.0f };
    glm::vec2 m_closeBtnSize{ 180.0f, 42.0f };
    int m_closeBtnState = 0; // 0=Idle, 1=Hover, 2=Pressed

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    void updateLayout();
    void updatePositions();

public:
    SettingsState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer);

    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};

