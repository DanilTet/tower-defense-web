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
class CampaignTabView;
class CustomMapsTabView;

class LevelSelectState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    const Texture2D* m_whiteTexture;

    bool m_mousePressedLastFrame = false;
    bool m_suppressClickUntilRelease = true;
    glm::vec2 m_mousePos{ 0.0f, 0.0f };

    // Вкладки
    LevelTab m_activeTab = LevelTab::Campaign;
    UIButton m_tabCampaign;
    UIButton m_tabCustom;

    // Режим разработчика (Ctrl + Shift + D)
    static LevelTab s_lastActiveTab;
    bool m_isDevMode = false;
    bool m_ctrlShiftDWasDown = false;
    UIButton m_btnDevModeToggle;

    // Нижняя навигация
    UIButton m_btnBack;

    // Изолированные вкладки выбора уровней
    std::unique_ptr<CampaignTabView> m_campaignTab;
    std::unique_ptr<CustomMapsTabView> m_customTab;

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const;
    void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const Texture2D* tex = nullptr);
    void drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const Texture2D* tex = nullptr);

public:
    static LevelTab getLastActiveTab() { return s_lastActiveTab; }
    LevelSelectState(GameStateManager& stateManager, int width, int height,
                     std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer,
                     LevelTab initialTab = LevelTab::Campaign);
    ~LevelSelectState() override;

    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};