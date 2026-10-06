#pragma once
#include "IGameState.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "PauseState.h"

class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;

class GameOverState : public IGameState {
private:
    // ссылка на главный менеджер
    GameStateManager& m_stateManager;
    int m_width, m_height; // размеры окна

    // дерьмицо для отрисовочки
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    std::shared_ptr<Texture2D> m_uiTexture;

    // путь к уровню
    std::string m_levelPath;

    bool m_mousePressedLastFrame = false; // от залипания клавиши
    bool m_isEditorTest = false;
    GameplayOrigin m_origin = GameplayOrigin::Campaign;

    // размеры окощка высплывающего
    glm::vec2 m_windowPos;
    glm::vec2 m_windowSize;
    float m_uiScale = 1.0f;

    // две кнопки
    UIButton m_btnRetry;
    UIButton m_btnMenu;

    // курсор внутри кнопки???
    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    void updateLayout();

public:
    // дефолт
    GameOverState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, std::string levelPath, bool isEditorTest = false, GameplayOrigin origin = GameplayOrigin::Campaign);
    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};