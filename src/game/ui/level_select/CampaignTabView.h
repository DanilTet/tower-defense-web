#pragma once
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../../states/PauseState.h"

struct GLFWwindow;
class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;

struct CampaignMissionCardUI {
    UIButton cardBtn;
    std::string missionId;
    std::string filename;
    std::string displayName;
    std::string description;
    int order = 1;
    bool isUnlocked = false;
    bool isCompleted = false;

    UIButton playBtn;
    UIButton btnUp;
    UIButton btnDown;
    UIButton btnEdit;
    UIButton btnRemove;
};

class CampaignTabView {
public:
    CampaignTabView(GameStateManager& stateManager,
                    std::shared_ptr<SpriteRenderer> renderer,
                    TextRenderer* textRenderer,
                    const Texture2D* whiteTexture);
    ~CampaignTabView() = default;

    void init(int screenWidth, int screenHeight, float tabY, float tabH, float bottomY, float navBtnH, bool isDevMode);
    bool processInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt);
    void update(float dt);
    void renderGeometry(float uiScale, bool isDevMode);
    void renderText(float uiScale, bool isDevMode);

    bool isModalOpen() const { return m_isAddMissionModalOpen; }
    void resetPage() { m_currentPage = 0; }

private:
    GameStateManager& m_stateManager;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    const Texture2D* m_whiteTexture;

    int m_width = 0;
    int m_height = 0;
    float m_tabY = 0.0f;
    float m_tabH = 0.0f;
    float m_bottomY = 0.0f;
    float m_navBtnH = 0.0f;
    bool m_isDevMode = false;

    std::vector<CampaignMissionCardUI> m_cards;

    // Пагинация
    int m_currentPage = 0;
    int m_totalPages = 1;
    UIButton m_btnPrevPage;
    UIButton m_btnNextPage;

    // Кнопки управления автора в Dev Mode (правый верхний угол)
    UIButton m_btnDevAddMission;
    UIButton m_btnDevUnlockAll;
    UIButton m_btnDevResetProgress;

    // Модальное окно добавления карты в кампанию (Dev Mode)
    bool m_isAddMissionModalOpen = false;
    std::vector<std::string> m_availableFilesToAdd;
    int m_addMissionScrollOffset = 0;

    void initLayout();
    void openAddMissionModal();
    bool processAddMissionModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt);
    void renderAddMissionModal(float uiScale);

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const;
    void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const Texture2D* tex = nullptr);
    void drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const Texture2D* tex = nullptr);
};

