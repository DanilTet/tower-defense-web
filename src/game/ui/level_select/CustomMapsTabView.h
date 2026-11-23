#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "../../states/PauseState.h"

struct GLFWwindow;
class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;

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

class CustomMapsTabView {
public:
    CustomMapsTabView(GameStateManager& stateManager,
                      std::shared_ptr<SpriteRenderer> renderer,
                      TextRenderer* textRenderer,
                      const Texture2D* whiteTexture);
    ~CustomMapsTabView() = default;

    void init(int screenWidth, int screenHeight, float tabY, float tabH, float bottomY, float navBtnH);
    bool processInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt);
    void update(float dt);
    void renderGeometry(float uiScale);
    void renderText(float uiScale);

    bool isModalOpen() const { return m_isRenameModalOpen || m_isDeleteModalOpen; }
    bool isSearchActive() const { return m_isSearchActive; }
    void cancelSearch();
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

    std::vector<CustomCardUI> m_cards;

    // Пагинация
    int m_currentPage = 0;
    int m_totalPages = 1;
    UIButton m_btnPrevPage;
    UIButton m_btnNextPage;

    // Поиск
    std::string m_searchQuery = "";
    bool m_isSearchActive = false;
    glm::vec2 m_searchBoxPos{ 0.0f, 0.0f };
    glm::vec2 m_searchBoxSize{ 220.0f, 32.0f };
    std::vector<TagChip> m_filterChips;

    // Нижняя кнопка "+ Создать карту"
    UIButton m_btnEditor;

    // Модальное окно переименования
    bool m_isRenameModalOpen = false;
    std::string m_renameInputText = "";
    std::string m_renameTargetFileName = "";
    float m_cursorBlinkTimer = 0.0f;

    // Модальное окно удаления
    bool m_isDeleteModalOpen = false;
    std::string m_deleteTargetFileName = "";

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    void initLayout();

    void openRenameModal(const std::string& targetFileName);
    void confirmRename();
    bool processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt);
    void renderRenameModal(float uiScale);

    void openDeleteModal(const std::string& targetFileName);
    void confirmDelete();
    bool processDeleteModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt);
    void renderDeleteModal(float uiScale);

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const;
    void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const Texture2D* tex = nullptr);
    void drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const Texture2D* tex = nullptr);
};

