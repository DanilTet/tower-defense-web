#pragma once
#include "IGameState.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "PauseState.h"
#include "../core/LevelManager.h"
#include "../core/CampaignManager.h"

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

struct CustomCardUI {
    UIButton cardBtn;
    std::string filename;
    std::string displayName;
    std::string fullPath;
    std::vector<std::string> tags;

    UIButton playBtn;
    UIButton editBtn;
    UIButton renameBtn;
    UIButton deleteBtn;
};

struct TagChip {
    std::string tag;
    glm::vec2 pos;
    glm::vec2 size;
};

class LevelSelectState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    std::shared_ptr<Texture2D> m_uiTexture;
    std::shared_ptr<Texture2D> m_whiteTexture;

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
    UIButton m_btnDevAddMission;
    UIButton m_btnDevUnlockAll;
    UIButton m_btnDevResetProgress;

    // Списки карточек
    std::vector<CampaignMissionCardUI> m_campaignCards;
    std::vector<CustomCardUI> m_customCards;

    // Нижняя навигация
    UIButton m_btnBack;
    UIButton m_btnEditor; // Для вкладки кастомных карт

    // Пагинация
    int m_currentPage = 0;
    int m_totalPages = 1;
    UIButton m_btnPrevPage;
    UIButton m_btnNextPage;

    // Поиск (для вкладки кастомных карт)
    std::string m_searchQuery = "";
    bool m_isSearchActive = false;
    glm::vec2 m_searchBoxPos{ 0.0f, 0.0f };
    glm::vec2 m_searchBoxSize{ 220.0f, 32.0f };
    std::vector<TagChip> m_filterChips;

    // Модальное окно переименования кастомной карты
    bool m_isRenameModalOpen = false;
    std::string m_renameInputText = "";
    std::string m_renameTargetFileName = "";
    float m_cursorBlinkTimer = 0.0f;

    // Модальное окно подтверждения удаления кастомной карты
    bool m_isDeleteModalOpen = false;
    std::string m_deleteTargetFileName = "";

    // Модальное окно добавления карты в кампанию (Dev Mode)
    bool m_isAddMissionModalOpen = false;
    std::vector<std::string> m_availableFilesToAdd;
    int m_addMissionScrollOffset = 0;

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const std::shared_ptr<Texture2D>& tex = nullptr);
    void drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const std::shared_ptr<Texture2D>& tex = nullptr);

    void initCampaignLayout(float uiScale, float tabY, float tabH, float bottomY, float navBtnH);
    void initCustomLayout(float uiScale, float tabY, float tabH, float bottomY, float navBtnH);

    void openRenameModal(const std::string& targetFileName);
    void confirmRename();
    bool processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);
    void renderRenameModal(float uiScale);

    void openDeleteModal(const std::string& targetFileName);
    void confirmDelete();
    bool processDeleteModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);
    void renderDeleteModal(float uiScale);

    void openAddMissionModal();
    bool processAddMissionModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);
    void renderAddMissionModal(float uiScale);

public:
    static LevelTab getLastActiveTab() { return s_lastActiveTab; }
    LevelSelectState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, LevelTab initialTab = LevelTab::Campaign);
    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};