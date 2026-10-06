#pragma once
#include "IGameState.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>

class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;
class GameplayState;

#include "../ui/VolumeSliderWidget.h"

// кнопка
struct UIButton {
    glm::vec2 pos;
    glm::vec2 size;
    std::string text;
    int state; // 0 = Idle, 1 = Hover, 2 = Pressed
};

class PauseState : public IGameState {
private:
    GameStateManager& m_stateManager;
    GameplayState* m_gameplayState; // указатель на стейт игры
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    // Ui текстурка
    std::shared_ptr<Texture2D> m_uiTexture;
    std::shared_ptr<Texture2D> m_whiteTexture;
    // переменные мыши и клавиатуры
    bool m_mousePressedLastFrame;
    bool m_escPressedLastFrame = false;
    glm::vec2 m_currentMousePos;
    // переменные окна
    glm::vec2 m_windowPos;
    glm::vec2 m_windowSize;
    float m_headerHeight;
    bool m_isDragging;
    glm::vec2 m_dragOffset;
    float m_uiScale = 1.0f;

    // Относительные Y-координаты элементов внутри окна
    float m_resumeRelY = 0.0f;
    float m_saveRelY = 0.0f;
    float m_volRelY = 0.0f;
    float m_exitRelY = 0.0f;

    // кнопки
    UIButton m_btnResume;
    UIButton m_btnSave;
    UIButton m_btnExit;

    VolumeSliderWidget m_volumeWidget;

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    void updateLayout();
    void updateButtonPositions();

public:
    PauseState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, GameplayState* gameplayState);

    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};