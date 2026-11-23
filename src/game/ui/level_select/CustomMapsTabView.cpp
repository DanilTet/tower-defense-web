#include "CustomMapsTabView.h"
#include "../../states/GameStateManager.h"
#include "../../states/GameplayState.h"
#include "../../states/MapEditorState.h"
#include "../../core/InputManager.h"
#include "../../core/LevelManager.h"
#include "../../core/LocalizationManager.h"
#include "../../core/SettingsManager.h"
#include "../../../renderer/SpriteRenderer.h"
#include "../../../renderer/TextRenderer.h"
#include "../../../textures/Texture2D.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <iostream>

static bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto toLowerUtf8 = [](const std::string& str) {
        std::string res;
        for (size_t i = 0; i < str.length(); ) {
            unsigned char c = str[i];
            if (c < 0x80) {
                res += static_cast<char>(std::tolower(c));
                i++;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < str.length()) {
                unsigned char c2 = str[i + 1];
                if (c == 0xD0 && c2 >= 0x90 && c2 <= 0x9F) {
                    res += static_cast<char>(0xD0);
                    res += static_cast<char>(c2 + 0x20);
                } else if (c == 0xD0 && c2 >= 0xA0 && c2 <= 0xAF) {
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(c2 - 0x20);
                } else if (c == 0xD0 && c2 == 0x81) {
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x91);
                } else if (c == 0xD0 && c2 == 0x84) {
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x94);
                } else if (c == 0xD0 && c2 == 0x86) {
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x96);
                } else if (c == 0xD0 && c2 == 0x87) {
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x97);
                } else if (c == 0xD2 && c2 == 0x90) {
                    res += static_cast<char>(0xD2);
                    res += static_cast<char>(0x91);
                } else {
                    res += static_cast<char>(c);
                    res += static_cast<char>(c2);
                }
                i += 2;
            } else {
                res += static_cast<char>(c);
                i++;
            }
        }
        return res;
    };
    std::string hLower = toLowerUtf8(haystack);
    std::string nLower = toLowerUtf8(needle);
    return hLower.find(nLower) != std::string::npos;
}

CustomMapsTabView::CustomMapsTabView(GameStateManager& stateManager,
                                     std::shared_ptr<SpriteRenderer> renderer,
                                     TextRenderer* textRenderer,
                                     const Texture2D* whiteTexture)
    : m_stateManager(stateManager),
      m_renderer(renderer),
      m_textRenderer(textRenderer),
      m_whiteTexture(whiteTexture)
{
}

void CustomMapsTabView::init(int screenWidth, int screenHeight, float tabY, float tabH, float bottomY, float navBtnH) {
    m_width = screenWidth;
    m_height = screenHeight;
    m_tabY = tabY;
    m_tabH = tabH;
    m_bottomY = bottomY;
    m_navBtnH = navBtnH;
    initLayout();
}

void CustomMapsTabView::cancelSearch() {
    m_isSearchActive = false;
    m_searchQuery.clear();
    initLayout();
}

void CustomMapsTabView::initLayout() {
    m_cards.clear();

    auto customLevels = LevelManager::getCustomLevels();

    std::vector<LevelInfo> filtered;
    for (const auto& lvl : customLevels) {
        if (!m_searchQuery.empty()) {
            bool mName = containsCaseInsensitive(lvl.name, m_searchQuery);
            bool mFile = containsCaseInsensitive(lvl.filename, m_searchQuery);
            if (!mName && !mFile) continue;
        }
        filtered.push_back(lvl);
    }

    const int itemsPerPage = 6;
    m_totalPages = std::max(1, static_cast<int>((filtered.size() + itemsPerPage - 1) / itemsPerPage));
    if (m_currentPage >= m_totalPages) m_currentPage = m_totalPages - 1;
    if (m_currentPage < 0) m_currentPage = 0;

    int startIndex = m_currentPage * itemsPerPage;
    int endIndex = std::min(startIndex + itemsPerPage, static_cast<int>(filtered.size()));

    float uiScale = SettingsManager::getUIScaleMultiplier();
    float startTabsX = std::clamp(28.0f * uiScale, 16.0f, 40.0f);

    // Кнопка + Создать карту (справа внизу)
    float createBtnW = std::clamp(190.0f * uiScale, 130.0f, 260.0f);
    m_btnEditor.size = glm::vec2(createBtnW, m_navBtnH);
    m_btnEditor.pos = glm::vec2(static_cast<float>(m_width) - startTabsX - createBtnW, m_bottomY);
    m_btnEditor.text = LOC("LEVEL_BTN_CREATE");

    // Пагинация (по центру внизу)
    float pageBtnW = std::clamp(100.0f * uiScale, 70.0f, 140.0f);
    float pageBtnH = std::clamp(34.0f * uiScale, 24.0f, 46.0f);
    float centerX = m_width * 0.5f;
    float pageGap = std::clamp(40.0f * uiScale, 20.0f, 60.0f);

    m_btnPrevPage.size = glm::vec2(pageBtnW, pageBtnH);
    m_btnPrevPage.pos = glm::vec2(centerX - pageBtnW - pageGap, m_bottomY + (m_navBtnH - pageBtnH) * 0.5f);
    m_btnPrevPage.text = LOC("LEVEL_BTN_PREV");

    m_btnNextPage.size = glm::vec2(pageBtnW, pageBtnH);
    m_btnNextPage.pos = glm::vec2(centerX + pageGap, m_bottomY + (m_navBtnH - pageBtnH) * 0.5f);
    m_btnNextPage.text = LOC("LEVEL_BTN_NEXT");

    float topY = m_tabY + m_tabH + std::clamp(16.0f * uiScale, 10.0f, 22.0f);
    float bottomAreaH = m_navBtnH + 28.0f;
    float availH = static_cast<float>(m_height) - topY - bottomAreaH;

    float gapX = std::clamp(20.0f * uiScale, 12.0f, 32.0f);
    float gapY = std::clamp(14.0f * uiScale, 8.0f, 22.0f);

    float maxAvailW = (static_cast<float>(m_width) - 60.0f - gapX) * 0.5f;
    float maxAvailH = (availH - 2.0f * gapY) / 3.0f;

    float cardWidth = std::clamp(380.0f * uiScale, 220.0f, maxAvailW);
    float cardHeight = std::clamp(140.0f * uiScale, 100.0f, std::max(100.0f, maxAvailH));

    float totalW = 2.0f * cardWidth + gapX;
    float startX = (m_width - totalW) * 0.5f;

    // Поле поиска (правый верхний угол)
    float searchW = std::clamp(220.0f * uiScale, 150.0f, 300.0f);
    m_searchBoxPos = glm::vec2(startX + totalW - searchW, m_tabY);
    m_searchBoxSize = glm::vec2(searchW, m_tabH);

    float smBtnH = std::clamp(26.0f * uiScale, 20.0f, 34.0f);
    float smBtnW = std::clamp(72.0f * uiScale, 54.0f, cardWidth * 0.26f);
    float playBtnW = std::clamp(86.0f * uiScale, 65.0f, cardWidth * 0.30f);

    for (int i = startIndex; i < endIndex; ++i) {
        const auto& lvl = filtered[i];
        int cardIdx = i - startIndex;
        int col = cardIdx % 2;
        int row = cardIdx / 2;

        float x = startX + col * (cardWidth + gapX);
        float y = topY + row * (cardHeight + gapY);

        CustomCardUI card;
        card.filename = lvl.filename;
        card.displayName = lvl.name;
        card.fullPath = lvl.fullPath;
        card.tags = lvl.tags;

        card.cardBtn.pos = glm::vec2(x, y);
        card.cardBtn.size = glm::vec2(cardWidth, cardHeight);
        card.cardBtn.state = 0;

        float btnY = y + cardHeight - smBtnH - 8.0f;
        float curX = x + 10.0f;

        card.renameBtn.pos = glm::vec2(curX, btnY);
        card.renameBtn.size = glm::vec2(smBtnW, smBtnH);
        card.renameBtn.text = LOC("LEVEL_BTN_RENAME");
        curX += smBtnW + 5.0f;

        card.editBtn.pos = glm::vec2(curX, btnY);
        card.editBtn.size = glm::vec2(smBtnW, smBtnH);
        card.editBtn.text = LOC("CAMPAIGN_BTN_EDIT");
        curX += smBtnW + 5.0f;

        card.deleteBtn.pos = glm::vec2(curX, btnY);
        card.deleteBtn.size = glm::vec2(smBtnW, smBtnH);
        card.deleteBtn.text = LOC("CUSTOM_BTN_DELETE");

        card.playBtn.pos = glm::vec2(x + cardWidth - playBtnW - 8.0f, btnY);
        card.playBtn.size = glm::vec2(playBtnW, smBtnH);
        card.playBtn.text = LOC("LEVEL_BTN_PLAY");

        m_cards.push_back(card);
    }
}

bool CustomMapsTabView::processInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt) {
    if (m_isRenameModalOpen) {
        return processRenameModalInput(window, mousePos, leftDown, leftPressed, dt);
    }

    if (m_isDeleteModalOpen) {
        return processDeleteModalInput(window, mousePos, leftDown, leftPressed, dt);
    }

    // Escape для снятия фокуса / очистки поиска
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (m_isSearchActive) {
            cancelSearch();
            return true;
        }
    }

    // Ввод текста в поисковую строку
    if (m_isSearchActive) {
        if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
            auto& ks = m_keyStates[GLFW_KEY_BACKSPACE];
            if (!ks.isDown) {
                ks.isDown = true;
                ks.holdTimer = 0.0f;
                InputManager::popUtf8(m_searchQuery);
                initLayout();
            }
        } else {
            m_keyStates[GLFW_KEY_BACKSPACE].isDown = false;
        }

        std::string typed = InputManager::getFrameText();
        if (!typed.empty() && m_searchQuery.length() < 30) {
            m_searchQuery += typed;
            initLayout();
        }
    }

    if (leftPressed) {
        // Клик по строке поиска
        if (isPointInRect(mousePos, m_searchBoxPos, m_searchBoxSize)) {
            m_isSearchActive = true;
            return true;
        } else {
            m_isSearchActive = false;
        }

        // Кнопка + Создать карту
        if (isPointInRect(mousePos, m_btnEditor.pos, m_btnEditor.size)) {
            m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, "", EditorOrigin::Custom));
            return true;
        }

        // Пагинация
        if (m_totalPages > 1) {
            if (isPointInRect(mousePos, m_btnPrevPage.pos, m_btnPrevPage.size) && m_currentPage > 0) {
                m_currentPage--;
                initLayout();
                return true;
            }
            if (isPointInRect(mousePos, m_btnNextPage.pos, m_btnNextPage.size) && m_currentPage < m_totalPages - 1) {
                m_currentPage++;
                initLayout();
                return true;
            }
        }

        // Клики по карточкам кастомных карт
        for (auto& card : m_cards) {
            if (isPointInRect(mousePos, card.playBtn.pos, card.playBtn.size)) {
                std::cout << "[LevelSelect] Launching custom level: " << card.displayName << " (" << card.fullPath << ")" << std::endl;
                m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.fullPath, false, GameplayOrigin::Custom));
                return true;
            }

            if (isPointInRect(mousePos, card.editBtn.pos, card.editBtn.size)) {
                std::cout << "[LevelSelect] Opening map in editor: " << card.filename << std::endl;
                m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.filename, EditorOrigin::Custom));
                return true;
            }

            if (isPointInRect(mousePos, card.renameBtn.pos, card.renameBtn.size)) {
                openRenameModal(card.filename);
                return true;
            }

            if (isPointInRect(mousePos, card.deleteBtn.pos, card.deleteBtn.size)) {
                openDeleteModal(card.filename);
                return true;
            }

            if (isPointInRect(mousePos, card.cardBtn.pos, card.cardBtn.size)) {
                m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.fullPath, false, GameplayOrigin::Custom));
                return true;
            }
        }
    }

    return false;
}

void CustomMapsTabView::update(float dt) {
    if (m_isRenameModalOpen || m_isSearchActive) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= 1.0f) {
            m_cursorBlinkTimer -= 1.0f;
        }
    }
}

void CustomMapsTabView::renderGeometry(float uiScale) {
    // 1. Поисковая строка
    drawQuad(m_searchBoxPos, m_searchBoxSize,
             m_isSearchActive ? glm::vec3(0.20f, 0.24f, 0.32f) : glm::vec3(0.12f, 0.14f, 0.18f), m_whiteTexture);

    // 2. Карточки кастомных карт
    for (const auto& card : m_cards) {
        drawQuad(card.cardBtn.pos, card.cardBtn.size, glm::vec3(0.14f, 0.16f, 0.22f), m_whiteTexture);
        drawQuad(card.cardBtn.pos, glm::vec2(5.0f * uiScale, card.cardBtn.size.y), glm::vec3(0.45f, 0.55f, 0.85f), m_whiteTexture);

        drawQuad(card.renameBtn.pos, card.renameBtn.size, glm::vec3(0.20f, 0.24f, 0.32f), m_whiteTexture);
        drawQuad(card.editBtn.pos, card.editBtn.size, glm::vec3(0.20f, 0.36f, 0.28f), m_whiteTexture);
        drawQuad(card.deleteBtn.pos, card.deleteBtn.size, glm::vec3(0.45f, 0.20f, 0.20f), m_whiteTexture);
        drawQuad(card.playBtn.pos, card.playBtn.size, glm::vec3(0.18f, 0.48f, 0.78f), m_whiteTexture);
    }

    // 3. Кнопка + Создать карту
    drawQuad(m_btnEditor.pos, m_btnEditor.size, glm::vec3(0.18f, 0.52f, 0.28f), m_whiteTexture);

    // 4. Пагинация
    if (m_totalPages > 1) {
        if (m_currentPage > 0) drawQuad(m_btnPrevPage.pos, m_btnPrevPage.size, glm::vec3(0.20f, 0.24f, 0.30f), m_whiteTexture);
        if (m_currentPage < m_totalPages - 1) drawQuad(m_btnNextPage.pos, m_btnNextPage.size, glm::vec3(0.20f, 0.24f, 0.30f), m_whiteTexture);
    }
}

void CustomMapsTabView::renderText(float uiScale) {
    float fTitle = std::clamp(0.54f * uiScale, 0.40f, 0.70f);
    float fSub = std::clamp(0.42f * uiScale, 0.30f, 0.52f);
    float fBtn = std::clamp(0.44f * uiScale, 0.32f, 0.56f);

    // 1. Поиск
    std::string sText = m_searchQuery.empty() ? LOC("LEVEL_SEARCH_HINT") : m_searchQuery;
    glm::vec3 sCol = m_searchQuery.empty() ? glm::vec3(0.50f) : glm::vec3(1.0f);
    m_textRenderer->RenderText(sText, m_searchBoxPos.x + 10.0f,
                               m_searchBoxPos.y + (m_searchBoxSize.y - fSub * 28.0f) * 0.5f + 2.0f, fSub, sCol);

    // 2. Сообщение, если карт нет
    if (m_cards.empty()) {
        std::string emptyMsg = "Кастомных карт пока нет. Нажмите '+ СОЗДАТЬ КАРТУ' справа внизу!";
        float ew = m_textRenderer->CalculateTextWidth(emptyMsg, fTitle);
        m_textRenderer->RenderText(emptyMsg, (m_width - ew) * 0.5f, m_height * 0.45f, fTitle, glm::vec3(0.60f, 0.65f, 0.72f));
    }

    // 3. Карточки
    for (const auto& card : m_cards) {
        float tx = card.cardBtn.pos.x + 16.0f * uiScale;
        float ty = card.cardBtn.pos.y + 14.0f * uiScale;

        m_textRenderer->RenderText(card.displayName, tx, ty, fTitle, glm::vec3(1.0f));
        m_textRenderer->RenderText(card.filename, tx, ty + fTitle * 28.0f + 4.0f, fSub, glm::vec3(0.55f, 0.58f, 0.65f));

        auto renderCustomBtnText = [&](const UIButton& b, glm::vec3 fg) {
            float bw = m_textRenderer->CalculateTextWidth(b.text, fBtn);
            m_textRenderer->RenderText(b.text, b.pos.x + (b.size.x - bw) * 0.5f,
                                       b.pos.y + (b.size.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, fg);
        };

        renderCustomBtnText(card.renameBtn, glm::vec3(0.9f));
        renderCustomBtnText(card.editBtn, glm::vec3(1.0f));
        renderCustomBtnText(card.deleteBtn, glm::vec3(1.0f));
        renderCustomBtnText(card.playBtn, glm::vec3(1.0f));
    }

    // 4. Кнопка + Создать карту
    float fNav = std::clamp(0.54f * uiScale, 0.40f, 0.70f);
    float tw = m_textRenderer->CalculateTextWidth(m_btnEditor.text, fNav);
    m_textRenderer->RenderText(m_btnEditor.text, m_btnEditor.pos.x + (m_btnEditor.size.x - tw) * 0.5f,
                               m_btnEditor.pos.y + (m_btnEditor.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(1.0f));

    // 5. Пагинация
    if (m_totalPages > 1) {
        auto renderNavText = [&](const UIButton& btn) {
            float bw = m_textRenderer->CalculateTextWidth(btn.text, fNav);
            m_textRenderer->RenderText(btn.text, btn.pos.x + (btn.size.x - bw) * 0.5f,
                                       btn.pos.y + (btn.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(1.0f));
        };
        if (m_currentPage > 0) renderNavText(m_btnPrevPage);
        if (m_currentPage < m_totalPages - 1) renderNavText(m_btnNextPage);

        std::string pageStr = std::to_string(m_currentPage + 1) + " / " + std::to_string(m_totalPages);
        float pw = m_textRenderer->CalculateTextWidth(pageStr, fNav);
        m_textRenderer->RenderText(pageStr, (m_width - pw) * 0.5f,
                                   m_btnPrevPage.pos.y + (m_btnPrevPage.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(0.80f));
    }

    // 6. Модальные окна
    if (m_isRenameModalOpen) {
        renderRenameModal(uiScale);
    }
    if (m_isDeleteModalOpen) {
        renderDeleteModal(uiScale);
    }
}

// === Модальное окно переименования ===
void CustomMapsTabView::openRenameModal(const std::string& targetFileName) {
    m_renameTargetFileName = targetFileName;
    m_renameInputText.clear();
    for (const auto& c : m_cards) {
        if (c.filename == targetFileName) {
            m_renameInputText = c.displayName;
            break;
        }
    }
    m_isRenameModalOpen = true;
}

void CustomMapsTabView::confirmRename() {
    if (!m_renameInputText.empty()) {
        LevelManager::setLevelDisplayName(m_renameTargetFileName, m_renameInputText);
    }
    m_isRenameModalOpen = false;
    initLayout();
}

bool CustomMapsTabView::processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt) {
    (void)leftDown;
    (void)dt;
    if (!m_isRenameModalOpen) return false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_isRenameModalOpen = false;
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) {
        confirmRename();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
        auto& ks = m_keyStates[GLFW_KEY_BACKSPACE];
        if (!ks.isDown) {
            ks.isDown = true;
            InputManager::popUtf8(m_renameInputText);
            m_cursorBlinkTimer = 0.0f;
            return true;
        }
    } else {
        m_keyStates[GLFW_KEY_BACKSPACE].isDown = false;
    }

    std::string typed = InputManager::getFrameText();
    if (!typed.empty() && m_renameInputText.length() < 50) {
        m_renameInputText += typed;
        m_cursorBlinkTimer = 0.0f;
    }

    float uiScale = SettingsManager::getUIScaleMultiplier();
    float mW = std::clamp(460.0f * uiScale, 320.0f, static_cast<float>(m_width) - 40.0f);
    float mH = std::clamp(200.0f * uiScale, 150.0f, static_cast<float>(m_height) - 40.0f);
    glm::vec2 mPos((m_width - mW) * 0.5f, (m_height - mH) * 0.5f);

    float btnH = std::clamp(36.0f * uiScale, 26.0f, 48.0f);
    float btnW = (mW - 60.0f * uiScale) * 0.48f;
    float btnY = mPos.y + mH - btnH - 18.0f * uiScale;

    glm::vec2 savePos(mPos.x + 24.0f * uiScale, btnY);
    glm::vec2 cancelPos(mPos.x + mW - 24.0f * uiScale - btnW, btnY);

    if (leftPressed) {
        if (isPointInRect(mousePos, savePos, glm::vec2(btnW, btnH))) {
            confirmRename();
            return true;
        }
        if (isPointInRect(mousePos, cancelPos, glm::vec2(btnW, btnH))) {
            m_isRenameModalOpen = false;
            return true;
        }
    }

    return true;
}

void CustomMapsTabView::renderRenameModal(float uiScale) {
    float mW = std::clamp(460.0f * uiScale, 320.0f, static_cast<float>(m_width) - 40.0f);
    float mH = std::clamp(200.0f * uiScale, 150.0f, static_cast<float>(m_height) - 40.0f);
    glm::vec2 mPos((m_width - mW) * 0.5f, (m_height - mH) * 0.5f);

    float boxW = mW - 48.0f * uiScale;
    float boxH = std::clamp(38.0f * uiScale, 28.0f, 48.0f);
    glm::vec2 boxPos(mPos.x + 24.0f * uiScale, mPos.y + 60.0f * uiScale);

    float btnH = std::clamp(36.0f * uiScale, 26.0f, 48.0f);
    float btnW = (mW - 60.0f * uiScale) * 0.48f;
    float btnY = mPos.y + mH - btnH - 18.0f * uiScale;

    glm::vec2 savePos(mPos.x + 24.0f * uiScale, btnY);
    glm::vec2 cancelPos(mPos.x + mW - 24.0f * uiScale - btnW, btnY);

    // 1. Quads
    m_renderer->beginBatch();
    drawQuadRGBA(glm::vec2(0.0f), glm::vec2(m_width, m_height), glm::vec4(0.0f, 0.0f, 0.0f, 0.70f), m_whiteTexture);
    drawQuad(mPos, glm::vec2(mW, mH), glm::vec3(0.16f, 0.18f, 0.24f), m_whiteTexture);
    drawQuad(mPos, glm::vec2(mW, 36.0f * uiScale), glm::vec3(0.20f, 0.24f, 0.32f), m_whiteTexture);
    drawQuad(boxPos, glm::vec2(boxW, boxH), glm::vec3(0.10f, 0.12f, 0.16f), m_whiteTexture);
    drawQuad(savePos, glm::vec2(btnW, btnH), glm::vec3(0.18f, 0.48f, 0.78f), m_whiteTexture);
    drawQuad(cancelPos, glm::vec2(btnW, btnH), glm::vec3(0.32f, 0.35f, 0.40f), m_whiteTexture);
    m_renderer->endBatch();

    // 2. Text
    float fTitle = std::clamp(0.56f * uiScale, 0.40f, 0.72f);
    float fInput = std::clamp(0.50f * uiScale, 0.36f, 0.65f);
    float fBtn = std::clamp(0.48f * uiScale, 0.34f, 0.60f);

    std::string title = LOC("RENAME_TITLE");
    m_textRenderer->RenderText(title, mPos.x + 20.0f * uiScale, mPos.y + 10.0f * uiScale, fTitle, glm::vec3(1.0f));

    std::string textToDraw = m_renameInputText + (m_cursorBlinkTimer < 0.5f ? "|" : "");
    m_textRenderer->RenderText(textToDraw, boxPos.x + 10.0f, boxPos.y + (boxH - fInput * 28.0f) * 0.5f + 2.0f, fInput, glm::vec3(1.0f));

    float sw = m_textRenderer->CalculateTextWidth("Сохранить", fBtn);
    m_textRenderer->RenderText("Сохранить", savePos.x + (btnW - sw) * 0.5f, savePos.y + (btnH - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f));

    float cw = m_textRenderer->CalculateTextWidth(LOC("CUSTOM_DELETE_CANCEL"), fBtn);
    m_textRenderer->RenderText(LOC("CUSTOM_DELETE_CANCEL"), cancelPos.x + (btnW - cw) * 0.5f, cancelPos.y + (btnH - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(0.9f));
}

// === Модальное окно подтверждения удаления ===
void CustomMapsTabView::openDeleteModal(const std::string& targetFileName) {
    m_deleteTargetFileName = targetFileName;
    m_isDeleteModalOpen = true;
}

void CustomMapsTabView::confirmDelete() {
    if (!m_deleteTargetFileName.empty()) {
        LevelManager::deleteLevel(m_deleteTargetFileName);
    }
    m_isDeleteModalOpen = false;
    initLayout();
}

bool CustomMapsTabView::processDeleteModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt) {
    (void)leftDown;
    (void)dt;
    if (!m_isDeleteModalOpen) return false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_isDeleteModalOpen = false;
        return true;
    }

    float uiScale = SettingsManager::getUIScaleMultiplier();
    float mW = std::clamp(420.0f * uiScale, 300.0f, static_cast<float>(m_width) - 40.0f);
    float mH = std::clamp(180.0f * uiScale, 130.0f, static_cast<float>(m_height) - 40.0f);
    glm::vec2 mPos((m_width - mW) * 0.5f, (m_height - mH) * 0.5f);

    float btnH = std::clamp(36.0f * uiScale, 26.0f, 48.0f);
    float btnW = (mW - 60.0f * uiScale) * 0.48f;
    float btnY = mPos.y + mH - btnH - 18.0f * uiScale;

    glm::vec2 delPos(mPos.x + 24.0f * uiScale, btnY);
    glm::vec2 cancelPos(mPos.x + mW - 24.0f * uiScale - btnW, btnY);

    if (leftPressed) {
        if (isPointInRect(mousePos, delPos, glm::vec2(btnW, btnH))) {
            confirmDelete();
            return true;
        }
        if (isPointInRect(mousePos, cancelPos, glm::vec2(btnW, btnH))) {
            m_isDeleteModalOpen = false;
            return true;
        }
    }

    return true;
}

void CustomMapsTabView::renderDeleteModal(float uiScale) {
    float mW = std::clamp(420.0f * uiScale, 300.0f, static_cast<float>(m_width) - 40.0f);
    float mH = std::clamp(180.0f * uiScale, 130.0f, static_cast<float>(m_height) - 40.0f);
    glm::vec2 mPos((m_width - mW) * 0.5f, (m_height - mH) * 0.5f);

    float btnH = std::clamp(36.0f * uiScale, 26.0f, 48.0f);
    float btnW = (mW - 60.0f * uiScale) * 0.48f;
    float btnY = mPos.y + mH - btnH - 18.0f * uiScale;

    glm::vec2 delPos(mPos.x + 24.0f * uiScale, btnY);
    glm::vec2 cancelPos(mPos.x + mW - 24.0f * uiScale - btnW, btnY);

    // 1. Quads
    m_renderer->beginBatch();
    drawQuadRGBA(glm::vec2(0.0f), glm::vec2(m_width, m_height), glm::vec4(0.0f, 0.0f, 0.0f, 0.70f), m_whiteTexture);
    drawQuad(mPos, glm::vec2(mW, mH), glm::vec3(0.18f, 0.16f, 0.20f), m_whiteTexture);
    drawQuad(delPos, glm::vec2(btnW, btnH), glm::vec3(0.65f, 0.20f, 0.20f), m_whiteTexture);
    drawQuad(cancelPos, glm::vec2(btnW, btnH), glm::vec3(0.32f, 0.35f, 0.40f), m_whiteTexture);
    m_renderer->endBatch();

    // 2. Text
    float fTitle = std::clamp(0.56f * uiScale, 0.40f, 0.72f);
    float fSub = std::clamp(0.46f * uiScale, 0.32f, 0.60f);
    float fBtn = std::clamp(0.48f * uiScale, 0.34f, 0.60f);

    std::string title = LOC("CUSTOM_DELETE_TITLE");
    m_textRenderer->RenderText(title, mPos.x + 24.0f * uiScale, mPos.y + 24.0f * uiScale, fTitle, glm::vec3(1.0f, 0.40f, 0.40f));

    std::string warn = "Удалить карту '" + m_deleteTargetFileName + "' безвозвратно?";
    m_textRenderer->RenderText(warn, mPos.x + 24.0f * uiScale, mPos.y + 64.0f * uiScale, fSub, glm::vec3(0.85f));

    float dw = m_textRenderer->CalculateTextWidth(LOC("CUSTOM_DELETE_CONFIRM"), fBtn);
    m_textRenderer->RenderText(LOC("CUSTOM_DELETE_CONFIRM"), delPos.x + (btnW - dw) * 0.5f, delPos.y + (btnH - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f));

    float cw = m_textRenderer->CalculateTextWidth(LOC("CUSTOM_DELETE_CANCEL"), fBtn);
    m_textRenderer->RenderText(LOC("CUSTOM_DELETE_CANCEL"), cancelPos.x + (btnW - cw) * 0.5f, cancelPos.y + (btnH - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(0.9f));
}

bool CustomMapsTabView::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void CustomMapsTabView::drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const Texture2D* tex) {
    if (m_renderer) {
        m_renderer->drawSprite(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

void CustomMapsTabView::drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const Texture2D* tex) {
    if (m_renderer) {
        m_renderer->drawSpriteRGBA(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

