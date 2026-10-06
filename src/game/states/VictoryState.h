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

class VictoryState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    std::shared_ptr<Texture2D> m_uiTexture;

    bool m_mousePressedLastFrame = false;
    bool m_isEditorTest = false;
    std::string m_levelPath = "";
    GameplayOrigin m_origin = GameplayOrigin::Campaign;
    glm::vec2 m_windowPos;
    glm::vec2 m_windowSize;
    float m_uiScale = 1.0f;

    UIButton m_btnNext;
    UIButton m_btnMenu;

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    void updateLayout();

public:
    VictoryState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, const std::string& levelPath = "", bool isEditorTest = false, GameplayOrigin origin = GameplayOrigin::Campaign);
    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};