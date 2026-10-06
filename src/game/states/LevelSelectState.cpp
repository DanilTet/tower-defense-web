#include "LevelSelectState.h"
#include "GameStateManager.h"
#include "GameplayState.h"
#include "MainMenuState.h"
#include "MapEditorState.h"
#include "../core/InputManager.h"
#include "../core/LocalizationManager.h"
#include "../core/SettingsManager.h"
#include "../core/CampaignManager.h"
#include "../ui/UICommon.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../textures/Texture2D.h"
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

LevelTab LevelSelectState::s_lastActiveTab = LevelTab::Campaign;

LevelSelectState::LevelSelectState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, LevelTab initialTab)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_activeTab(initialTab)
{
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
    m_mousePressedLastFrame = true;
    CampaignManager::init();
    m_isDevMode = CampaignManager::isDevMode();
    s_lastActiveTab = m_activeTab;
}

void LevelSelectState::init() {
    m_campaignCards.clear();
    m_customCards.clear();
    m_filterChips.clear();

    m_isDevMode = CampaignManager::isDevMode();
    float uiScale = SettingsManager::getUIScaleMultiplier();

    // Размеры и позиции вкладок
    float tabH = std::clamp(34.0f * uiScale, 26.0f, 44.0f);
    float tabY = std::clamp(14.0f + 20.0f * uiScale, 14.0f, 50.0f);
    float tabW = std::clamp(160.0f * uiScale, 110.0f, 220.0f);
    float tabGap = std::clamp(8.0f * uiScale, 6.0f, 14.0f);

    float startTabsX = std::clamp(28.0f * uiScale, 16.0f, 40.0f);

    m_tabCampaign.pos = glm::vec2(startTabsX, tabY);
    m_tabCampaign.size = glm::vec2(tabW, tabH);
    m_tabCampaign.text = LOC("LEVEL_TAB_CAMPAIGN");

    m_tabCustom.pos = glm::vec2(startTabsX + tabW + tabGap, tabY);
    m_tabCustom.size = glm::vec2(tabW, tabH);
    m_tabCustom.text = LOC("LEVEL_TAB_CUSTOM");

    // Бейдж-кнопка переключения Dev Mode
    float fDevBadge = std::clamp(0.44f * uiScale, 0.32f, 0.55f);
    std::string badgeText = m_isDevMode ? LOC("CAMPAIGN_DEV_MODE_ON") : "[ DEV MODE: ВЫКЛ ]";
    float bW = m_textRenderer ? (m_textRenderer->CalculateTextWidth(badgeText, fDevBadge) + 18.0f) : (140.0f * uiScale);
    float bX = m_tabCustom.pos.x + m_tabCustom.size.x + tabGap;
    m_btnDevModeToggle.pos = glm::vec2(bX, tabY);
    m_btnDevModeToggle.size = glm::vec2(bW, tabH);
    m_btnDevModeToggle.text = badgeText;

    // Нижняя навигация
    float navBtnH = std::clamp(38.0f * uiScale, 26.0f, 52.0f);
    float bottomY = static_cast<float>(m_height) - navBtnH - 14.0f;

    float backBtnW = std::clamp(160.0f * uiScale, 110.0f, 220.0f);
    m_btnBack.size = glm::vec2(backBtnW, navBtnH);
    m_btnBack.pos = glm::vec2(startTabsX, bottomY);
    m_btnBack.text = LOC("LEVEL_BTN_BACK");

    float createBtnW = std::clamp(190.0f * uiScale, 130.0f, 260.0f);
    m_btnEditor.size = glm::vec2(createBtnW, navBtnH);
    m_btnEditor.pos = glm::vec2(static_cast<float>(m_width) - startTabsX - createBtnW, bottomY);
    m_btnEditor.text = LOC("LEVEL_BTN_CREATE");

    // Пагинация
    float pageBtnW = std::clamp(100.0f * uiScale, 70.0f, 140.0f);
    float pageBtnH = std::clamp(34.0f * uiScale, 24.0f, 46.0f);
    float centerX = m_width * 0.5f;
    float pageGap = std::clamp(40.0f * uiScale, 20.0f, 60.0f);

    m_btnPrevPage.size = glm::vec2(pageBtnW, pageBtnH);
    m_btnPrevPage.pos = glm::vec2(centerX - pageBtnW - pageGap, bottomY + (navBtnH - pageBtnH) * 0.5f);
    m_btnPrevPage.text = LOC("LEVEL_BTN_PREV");

    m_btnNextPage.size = glm::vec2(pageBtnW, pageBtnH);
    m_btnNextPage.pos = glm::vec2(centerX + pageGap, bottomY + (navBtnH - pageBtnH) * 0.5f);
    m_btnNextPage.text = LOC("LEVEL_BTN_NEXT");

    // Кнопки управления автора в Dev Mode (правая верхняя часть экрана)
    if (m_isDevMode && m_activeTab == LevelTab::Campaign) {
        float devBtnH = tabH;
        float addMissionBtnW = std::clamp(145.0f * uiScale, 110.0f, 185.0f);
        float unlockBtnW = std::clamp(115.0f * uiScale, 85.0f, 145.0f);
        float btnDevGap = std::clamp(6.0f * uiScale, 4.0f, 10.0f);

        // Проверяем, чтобы правый блок кнопок не наезжал на кнопку DEV MODE
        float totalDevWidth = addMissionBtnW + unlockBtnW * 2.0f + btnDevGap * 2.0f;
        float minStartX = bX + bW + tabGap;
        float desiredStartX = static_cast<float>(m_width) - startTabsX - totalDevWidth;
        if (desiredStartX < minStartX) {
            float availW = (static_cast<float>(m_width) - startTabsX) - minStartX;
            if (availW > 0.0f) {
                float factor = std::clamp(availW / totalDevWidth, 0.65f, 1.0f);
                addMissionBtnW *= factor;
                unlockBtnW *= factor;
                btnDevGap = std::max(3.0f, btnDevGap * factor);
            }
        }

        float rightX = static_cast<float>(m_width) - startTabsX;

        m_btnDevResetProgress.size = glm::vec2(unlockBtnW, devBtnH);
        m_btnDevResetProgress.pos = glm::vec2(rightX - unlockBtnW, tabY);
        m_btnDevResetProgress.text = "СБРОСИТЬ";

        m_btnDevUnlockAll.size = glm::vec2(unlockBtnW, devBtnH);
        m_btnDevUnlockAll.pos = glm::vec2(m_btnDevResetProgress.pos.x - unlockBtnW - btnDevGap, tabY);
        m_btnDevUnlockAll.text = "ОТКРЫТЬ ВСЕ";

        m_btnDevAddMission.size = glm::vec2(addMissionBtnW, devBtnH);
        m_btnDevAddMission.pos = glm::vec2(m_btnDevUnlockAll.pos.x - addMissionBtnW - btnDevGap, tabY);
        m_btnDevAddMission.text = "+ В КАМПАНИЮ";
    }

    if (m_activeTab == LevelTab::Campaign) {
        initCampaignLayout(uiScale, tabY, tabH, bottomY, navBtnH);
    } else {
        initCustomLayout(uiScale, tabY, tabH, bottomY, navBtnH);
    }
}

void LevelSelectState::initCampaignLayout(float uiScale, float tabY, float tabH, float bottomY, float navBtnH) {
    const auto& missions = CampaignManager::getMissions();

    const int itemsPerPage = 3;
    m_totalPages = std::max(1, static_cast<int>((missions.size() + itemsPerPage - 1) / itemsPerPage));
    if (m_currentPage >= m_totalPages) m_currentPage = m_totalPages - 1;
    if (m_currentPage < 0) m_currentPage = 0;

    int startIndex = m_currentPage * itemsPerPage;
    int endIndex = std::min(startIndex + itemsPerPage, static_cast<int>(missions.size()));

    float topY = tabY + tabH + std::clamp(16.0f * uiScale, 10.0f, 26.0f);
    float bottomAreaH = navBtnH + 28.0f;
    float availH = static_cast<float>(m_height) - topY - bottomAreaH;

    float cardGapY = std::clamp(16.0f * uiScale, 10.0f, 26.0f);
    float maxCardH = (availH - 2.0f * cardGapY) / 3.0f;
    float cardH = std::clamp(130.0f * uiScale, 90.0f, std::max(90.0f, maxCardH));
    float cardW = std::clamp(720.0f * uiScale, 400.0f, static_cast<float>(m_width) - 60.0f);
    float cardX = (m_width - cardW) * 0.5f;

    float playBtnW = std::clamp(130.0f * uiScale, 95.0f, 180.0f);
    float playBtnH = std::clamp(38.0f * uiScale, 28.0f, 50.0f);

    float devBtnW = std::clamp(64.0f * uiScale, 48.0f, 85.0f);
    float devBtnH = std::clamp(28.0f * uiScale, 22.0f, 36.0f);

    for (int i = startIndex; i < endIndex; ++i) {
        const auto& mission = missions[i];
        int cardIdx = i - startIndex;
        float y = topY + cardIdx * (cardH + cardGapY);

        CampaignMissionCardUI card;
        card.missionId = mission.id;
        card.filename = mission.file;
        card.displayName = mission.name;
        card.description = mission.description;
        card.order = mission.order;
        card.isUnlocked = CampaignManager::isMissionUnlocked(i, m_isDevMode);
        card.isCompleted = CampaignManager::isMissionCompleted(mission.file) || CampaignManager::isMissionCompleted(mission.id);

        card.cardBtn.pos = glm::vec2(cardX, y);
        card.cardBtn.size = glm::vec2(cardW, cardH);
        card.cardBtn.state = 0;

        // Кнопка В бой / Играть
        float playX = cardX + cardW - playBtnW - 14.0f * uiScale;
        float playY = y + (cardH - playBtnH) * 0.5f;
        card.playBtn.pos = glm::vec2(playX, playY);
        card.playBtn.size = glm::vec2(playBtnW, playBtnH);
        card.playBtn.text = card.isUnlocked ? LOC("LEVEL_BTN_PLAY") : LOC("CAMPAIGN_STATUS_LOCKED");
        card.playBtn.state = 0;

        // В Dev Mode кнопки порядка, редактора и удаления
        if (m_isDevMode) {
            float btnY = y + cardH - devBtnH - 8.0f * uiScale;
            float curX = cardX + 16.0f * uiScale;

            card.btnUp.pos = glm::vec2(curX, btnY);
            card.btnUp.size = glm::vec2(devBtnW, devBtnH);
            card.btnUp.text = LOC("CAMPAIGN_BTN_UP");
            card.btnUp.state = 0;
            curX += devBtnW + 6.0f;

            card.btnDown.pos = glm::vec2(curX, btnY);
            card.btnDown.size = glm::vec2(devBtnW, devBtnH);
            card.btnDown.text = LOC("CAMPAIGN_BTN_DOWN");
            card.btnDown.state = 0;
            curX += devBtnW + 6.0f;

            float editBtnW = std::clamp(86.0f * uiScale, 65.0f, 110.0f);
            card.btnEdit.pos = glm::vec2(curX, btnY);
            card.btnEdit.size = glm::vec2(editBtnW, devBtnH);
            card.btnEdit.text = LOC("CAMPAIGN_BTN_EDIT");
            card.btnEdit.state = 0;
            curX += editBtnW + 6.0f;

            float removeBtnW = std::clamp(96.0f * uiScale, 70.0f, 120.0f);
            card.btnRemove.pos = glm::vec2(curX, btnY);
            card.btnRemove.size = glm::vec2(removeBtnW, devBtnH);
            card.btnRemove.text = LOC("CAMPAIGN_BTN_REMOVE");
            card.btnRemove.state = 0;
        }

        m_campaignCards.push_back(card);
    }
}

void LevelSelectState::initCustomLayout(float uiScale, float tabY, float tabH, float bottomY, float navBtnH) {
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

    float topY = tabY + tabH + std::clamp(16.0f * uiScale, 10.0f, 22.0f);
    float bottomAreaH = navBtnH + 28.0f;
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
    m_searchBoxPos = glm::vec2(startX + totalW - searchW, tabY);
    m_searchBoxSize = glm::vec2(searchW, tabH);

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

        m_customCards.push_back(card);
    }
}

void LevelSelectState::cleanup() {}

bool LevelSelectState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void LevelSelectState::drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const std::shared_ptr<Texture2D>& tex) {
    if (m_renderer) {
        m_renderer->drawSprite(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

void LevelSelectState::drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const std::shared_ptr<Texture2D>& tex) {
    if (m_renderer) {
        m_renderer->drawSpriteRGBA(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

void LevelSelectState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_mousePos = glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY));

    bool leftDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    if (m_suppressClickUntilRelease) {
        if (!leftDown) {
            m_suppressClickUntilRelease = false;
            m_mousePressedLastFrame = false;
        } else {
            return;
        }
    }

    // Хоткей переключения Dev Mode: Ctrl + Shift + D
    bool ctrlDown = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
    bool shiftDown = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool dDown = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);

    if (ctrlDown && shiftDown && dDown) {
        if (!m_ctrlShiftDWasDown) {
            m_ctrlShiftDWasDown = true;
            CampaignManager::toggleDevMode();
            m_isDevMode = CampaignManager::isDevMode();
            std::cout << "[LevelSelect] Dev Mode toggled via shortcut: " << (m_isDevMode ? "ON" : "OFF") << std::endl;
            init();
            return;
        }
    } else {
        m_ctrlShiftDWasDown = false;
    }

    // Обработка модальных окон
    if (m_isRenameModalOpen) {
        bool handled = processRenameModalInput(window, m_mousePos, leftDown, dt);
        m_mousePressedLastFrame = leftDown;
        if (handled) return;
    }

    if (m_isDeleteModalOpen) {
        bool handled = processDeleteModalInput(window, m_mousePos, leftDown, dt);
        m_mousePressedLastFrame = leftDown;
        if (handled) return;
    }

    if (m_isAddMissionModalOpen) {
        bool handled = processAddMissionModalInput(window, m_mousePos, leftDown, dt);
        m_mousePressedLastFrame = leftDown;
        if (handled) return;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (m_isSearchActive) {
            m_isSearchActive = false;
            m_searchQuery.clear();
            init();
            return;
        }
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
        return;
    }

    // Ввод в поиск на вкладке кастомных карт
    if (m_isSearchActive) {
        if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
            auto& ks = m_keyStates[GLFW_KEY_BACKSPACE];
            if (!ks.isDown) {
                ks.isDown = true;
                ks.holdTimer = 0.0f;
                InputManager::popUtf8(m_searchQuery);
                init();
            }
        } else {
            m_keyStates[GLFW_KEY_BACKSPACE].isDown = false;
        }

        std::string typed = InputManager::getFrameText();
        if (!typed.empty() && m_searchQuery.length() < 30) {
            m_searchQuery += typed;
            init();
        }
    }

    // Обработка клика
    if (leftDown && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        // Переключение вкладок
        if (isPointInRect(m_mousePos, m_tabCampaign.pos, m_tabCampaign.size)) {
            m_activeTab = LevelTab::Campaign;
            s_lastActiveTab = m_activeTab;
            m_currentPage = 0;
            init();
            return;
        }
        if (isPointInRect(m_mousePos, m_tabCustom.pos, m_tabCustom.size)) {
            m_activeTab = LevelTab::Custom;
            s_lastActiveTab = m_activeTab;
            m_currentPage = 0;
            init();
            return;
        }

        // Клик по бейджу/кнопке переключения Dev Mode
        if (isPointInRect(m_mousePos, m_btnDevModeToggle.pos, m_btnDevModeToggle.size)) {
            CampaignManager::toggleDevMode();
            m_isDevMode = CampaignManager::isDevMode();
            std::cout << "[LevelSelect] Dev Mode toggled via click: " << (m_isDevMode ? "ON" : "OFF") << std::endl;
            init();
            return;
        }

        // Кнопка Назад
        if (isPointInRect(m_mousePos, m_btnBack.pos, m_btnBack.size)) {
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        // Пагинация
        if (m_totalPages > 1) {
            if (isPointInRect(m_mousePos, m_btnPrevPage.pos, m_btnPrevPage.size) && m_currentPage > 0) {
                m_currentPage--;
                init();
                return;
            }
            if (isPointInRect(m_mousePos, m_btnNextPage.pos, m_btnNextPage.size) && m_currentPage < m_totalPages - 1) {
                m_currentPage++;
                init();
                return;
            }
        }

        // Действия на вкладке Кампании
        if (m_activeTab == LevelTab::Campaign) {
            // Dev Mode кнопки в верхнем баре
            if (m_isDevMode) {
                if (isPointInRect(m_mousePos, m_btnDevAddMission.pos, m_btnDevAddMission.size)) {
                    openAddMissionModal();
                    return;
                }
                if (isPointInRect(m_mousePos, m_btnDevUnlockAll.pos, m_btnDevUnlockAll.size)) {
                    CampaignManager::unlockAllProgress();
                    init();
                    return;
                }
                if (isPointInRect(m_mousePos, m_btnDevResetProgress.pos, m_btnDevResetProgress.size)) {
                    CampaignManager::resetProgress();
                    init();
                    return;
                }
            }

            for (size_t idx = 0; idx < m_campaignCards.size(); ++idx) {
                auto& card = m_campaignCards[idx];
                int globalMissionIdx = m_currentPage * 3 + static_cast<int>(idx);

                // Клик по Играть
                if (isPointInRect(m_mousePos, card.playBtn.pos, card.playBtn.size) || isPointInRect(m_mousePos, card.cardBtn.pos, card.cardBtn.size)) {
                    if (card.isUnlocked || m_isDevMode) {
                        std::string fullPath = "res/levels/" + card.filename;
                        std::cout << "[LevelSelect] Launching campaign mission: " << card.displayName << " (" << fullPath << ")" << std::endl;
                        m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, fullPath, false, GameplayOrigin::Campaign));
                        return;
                    }
                }

                // Кнопки Dev Mode
                if (m_isDevMode) {
                    if (isPointInRect(m_mousePos, card.btnUp.pos, card.btnUp.size)) {
                        CampaignManager::moveMissionUp(globalMissionIdx);
                        init();
                        return;
                    }
                    if (isPointInRect(m_mousePos, card.btnDown.pos, card.btnDown.size)) {
                        CampaignManager::moveMissionDown(globalMissionIdx);
                        init();
                        return;
                    }
                    if (isPointInRect(m_mousePos, card.btnEdit.pos, card.btnEdit.size)) {
                        std::cout << "[LevelSelect] Dev editing campaign level: " << card.filename << std::endl;
                        m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.filename, EditorOrigin::Campaign));
                        return;
                    }
                    if (isPointInRect(m_mousePos, card.btnRemove.pos, card.btnRemove.size)) {
                        CampaignManager::removeMission(globalMissionIdx);
                        init();
                        return;
                    }
                }
            }
        }
        // Действия на вкладке Кастомных карт
        else {
            // Клик по поисковой строке
            if (isPointInRect(m_mousePos, m_searchBoxPos, m_searchBoxSize)) {
                m_isSearchActive = true;
                return;
            } else {
                m_isSearchActive = false;
            }

            // Кнопка + Создать карту
            if (isPointInRect(m_mousePos, m_btnEditor.pos, m_btnEditor.size)) {
                m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, "", EditorOrigin::Custom));
                return;
            }

            for (auto& card : m_customCards) {
                if (isPointInRect(m_mousePos, card.playBtn.pos, card.playBtn.size)) {
                    std::cout << "[LevelSelect] Launching custom level: " << card.displayName << " (" << card.fullPath << ")" << std::endl;
                    m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.fullPath, false, GameplayOrigin::Custom));
                    return;
                }

                if (isPointInRect(m_mousePos, card.editBtn.pos, card.editBtn.size)) {
                    std::cout << "[LevelSelect] Opening map in editor: " << card.filename << std::endl;
                    m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.filename, EditorOrigin::Custom));
                    return;
                }

                if (isPointInRect(m_mousePos, card.renameBtn.pos, card.renameBtn.size)) {
                    openRenameModal(card.filename);
                    return;
                }

                if (isPointInRect(m_mousePos, card.deleteBtn.pos, card.deleteBtn.size)) {
                    openDeleteModal(card.filename);
                    return;
                }

                if (isPointInRect(m_mousePos, card.cardBtn.pos, card.cardBtn.size)) {
                    m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.fullPath, false, GameplayOrigin::Custom));
                    return;
                }
            }
        }
    } else if (!leftDown) {
        m_mousePressedLastFrame = false;
    }
}

void LevelSelectState::update(float dt) {
    if (m_isRenameModalOpen || m_isSearchActive) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= 1.0f) {
            m_cursorBlinkTimer -= 1.0f;
        }
    }
}

void LevelSelectState::render() {
    if (!m_renderer || !m_textRenderer) return;

    float uiScale = SettingsManager::getUIScaleMultiplier();
    float fontTab = std::clamp(0.55f * uiScale, 0.40f, 0.72f);
    float fNav = std::clamp(0.54f * uiScale, 0.40f, 0.70f);

    // =========================================================================
    // PASS 1: ГЕОМЕТРИЯ (ВСЕ ФОНЫ, ПЛАШКИ И КНОПКИ РИСУЮТСЯ В БАТЧЕ)
    // =========================================================================
    m_renderer->beginBatch();

    // 1. Верхние вкладки
    auto drawTabBg = [&](const UIButton& tab, bool active) {
        glm::vec3 bg = active ? glm::vec3(0.20f, 0.42f, 0.65f) : glm::vec3(0.12f, 0.14f, 0.20f);
        drawQuad(tab.pos, tab.size, bg, m_whiteTexture);
        drawQuad(glm::vec2(tab.pos.x, tab.pos.y + tab.size.y - 3.0f), glm::vec2(tab.size.x, 3.0f),
                 active ? glm::vec3(0.35f, 0.75f, 1.0f) : glm::vec3(0.08f, 0.10f, 0.14f), m_whiteTexture);
    };
    drawTabBg(m_tabCampaign, m_activeTab == LevelTab::Campaign);
    drawTabBg(m_tabCustom, m_activeTab == LevelTab::Custom);

    // 2. Dev Mode плашка-кнопка и кнопки
    glm::vec3 devToggleBg = m_isDevMode ? glm::vec3(0.75f, 0.45f, 0.12f) : glm::vec3(0.16f, 0.18f, 0.24f);
    drawQuad(m_btnDevModeToggle.pos, m_btnDevModeToggle.size, devToggleBg, m_whiteTexture);

    if (m_isDevMode && m_activeTab == LevelTab::Campaign) {
        drawQuad(m_btnDevAddMission.pos, m_btnDevAddMission.size, glm::vec3(0.18f, 0.46f, 0.28f), m_whiteTexture);
        drawQuad(m_btnDevUnlockAll.pos, m_btnDevUnlockAll.size, glm::vec3(0.25f, 0.35f, 0.50f), m_whiteTexture);
        drawQuad(m_btnDevResetProgress.pos, m_btnDevResetProgress.size, glm::vec3(0.50f, 0.22f, 0.22f), m_whiteTexture);
    }

    // 3. Карточки
    if (m_activeTab == LevelTab::Campaign) {
        for (const auto& card : m_campaignCards) {
            glm::vec3 cardBg = card.isUnlocked ? glm::vec3(0.14f, 0.16f, 0.22f) : glm::vec3(0.10f, 0.11f, 0.14f);
            if (card.isCompleted) {
                cardBg = glm::vec3(0.13f, 0.18f, 0.20f);
            }
            drawQuad(card.cardBtn.pos, card.cardBtn.size, cardBg, m_whiteTexture);

            glm::vec3 accentColor = card.isCompleted ? glm::vec3(0.25f, 0.75f, 0.35f) :
                                   (card.isUnlocked ? glm::vec3(0.35f, 0.65f, 0.95f) : glm::vec3(0.30f, 0.32f, 0.38f));
            drawQuad(card.cardBtn.pos, glm::vec2(6.0f * uiScale, card.cardBtn.size.y), accentColor, m_whiteTexture);

            glm::vec3 playBg = card.isUnlocked ? glm::vec3(0.18f, 0.48f, 0.78f) : glm::vec3(0.20f, 0.22f, 0.26f);
            drawQuad(card.playBtn.pos, card.playBtn.size, playBg, m_whiteTexture);

            if (m_isDevMode) {
                drawQuad(card.btnUp.pos, card.btnUp.size, glm::vec3(0.22f, 0.32f, 0.44f), m_whiteTexture);
                drawQuad(card.btnDown.pos, card.btnDown.size, glm::vec3(0.22f, 0.32f, 0.44f), m_whiteTexture);
                drawQuad(card.btnEdit.pos, card.btnEdit.size, glm::vec3(0.28f, 0.44f, 0.30f), m_whiteTexture);
                drawQuad(card.btnRemove.pos, card.btnRemove.size, glm::vec3(0.52f, 0.24f, 0.24f), m_whiteTexture);
            }
        }
    } else {
        drawQuad(m_searchBoxPos, m_searchBoxSize,
                 m_isSearchActive ? glm::vec3(0.20f, 0.24f, 0.32f) : glm::vec3(0.12f, 0.14f, 0.18f), m_whiteTexture);

        for (const auto& card : m_customCards) {
            drawQuad(card.cardBtn.pos, card.cardBtn.size, glm::vec3(0.14f, 0.16f, 0.22f), m_whiteTexture);
            drawQuad(card.cardBtn.pos, glm::vec2(5.0f * uiScale, card.cardBtn.size.y), glm::vec3(0.45f, 0.55f, 0.85f), m_whiteTexture);

            drawQuad(card.renameBtn.pos, card.renameBtn.size, glm::vec3(0.20f, 0.24f, 0.32f), m_whiteTexture);
            drawQuad(card.editBtn.pos, card.editBtn.size, glm::vec3(0.20f, 0.36f, 0.28f), m_whiteTexture);
            drawQuad(card.deleteBtn.pos, card.deleteBtn.size, glm::vec3(0.45f, 0.20f, 0.20f), m_whiteTexture);
            drawQuad(card.playBtn.pos, card.playBtn.size, glm::vec3(0.18f, 0.48f, 0.78f), m_whiteTexture);
        }
    }

    // 4. Нижняя навигация
    drawQuad(m_btnBack.pos, m_btnBack.size, glm::vec3(0.22f, 0.25f, 0.32f), m_whiteTexture);
    if (m_activeTab == LevelTab::Custom) {
        drawQuad(m_btnEditor.pos, m_btnEditor.size, glm::vec3(0.18f, 0.52f, 0.28f), m_whiteTexture);
    }
    if (m_totalPages > 1) {
        if (m_currentPage > 0) drawQuad(m_btnPrevPage.pos, m_btnPrevPage.size, glm::vec3(0.20f, 0.24f, 0.30f), m_whiteTexture);
        if (m_currentPage < m_totalPages - 1) drawQuad(m_btnNextPage.pos, m_btnNextPage.size, glm::vec3(0.20f, 0.24f, 0.30f), m_whiteTexture);
    }

    // ЗАВЕРШАЕМ БАТЧ СПРАЙТОВ ДО ОТРИСОВКИ ТЕКСТА!
    m_renderer->endBatch();

    // =========================================================================
    // PASS 2: ТЕКСТ (ТЕКСТ ГАРАНТИРОВАННО РИСУЕТСЯ ПОВЕРХ ВСЕХ КНОПОК И ФОНОВ)
    // =========================================================================

    // 1. Текст верхних вкладок
    auto renderTabText = [&](const UIButton& tab, bool active) {
        float textW = m_textRenderer->CalculateTextWidth(tab.text, fontTab);
        float textX = tab.pos.x + (tab.size.x - textW) * 0.5f;
        float textY = tab.pos.y + (tab.size.y - fontTab * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(tab.text, textX, textY, fontTab, active ? glm::vec3(1.0f) : glm::vec3(0.70f));
    };
    renderTabText(m_tabCampaign, m_activeTab == LevelTab::Campaign);
    renderTabText(m_tabCustom, m_activeTab == LevelTab::Custom);

    // 2. Dev Mode текст
    float fDevBadge = std::clamp(0.48f * uiScale, 0.36f, 0.62f);
    float tw = m_textRenderer->CalculateTextWidth(m_btnDevModeToggle.text, fDevBadge);
    float tx = m_btnDevModeToggle.pos.x + (m_btnDevModeToggle.size.x - tw) * 0.5f;
    float ty = m_btnDevModeToggle.pos.y + (m_btnDevModeToggle.size.y - fDevBadge * 28.0f) * 0.5f + 2.0f;
    glm::vec3 devToggleFg = m_isDevMode ? glm::vec3(1.0f) : glm::vec3(0.70f, 0.72f, 0.78f);
    m_textRenderer->RenderText(m_btnDevModeToggle.text, tx, ty, fDevBadge, devToggleFg);

    if (m_isDevMode && m_activeTab == LevelTab::Campaign) {
        auto renderDevBtnText = [&](const UIButton& btn) {
            float fBtn = std::clamp(0.44f * uiScale, 0.32f, 0.58f);
            float bw = m_textRenderer->CalculateTextWidth(btn.text, fBtn);
            m_textRenderer->RenderText(btn.text, btn.pos.x + (btn.size.x - bw) * 0.5f,
                                       btn.pos.y + (btn.size.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f));
        };
        renderDevBtnText(m_btnDevAddMission);
        renderDevBtnText(m_btnDevUnlockAll);
        renderDevBtnText(m_btnDevResetProgress);
    } else if (!m_isDevMode) {
        float fHint = std::clamp(0.40f * uiScale, 0.30f, 0.52f);
        std::string hint = LOC("CAMPAIGN_DEV_HINT");
        float rightX = static_cast<float>(m_width) - 36.0f * uiScale - m_textRenderer->CalculateTextWidth(hint, fHint);
        m_textRenderer->RenderText(hint, rightX, m_tabCampaign.pos.y + (m_tabCampaign.size.y - fHint * 28.0f) * 0.5f + 2.0f, fHint, glm::vec3(0.50f, 0.52f, 0.58f));
    }

    // 3. Текст карточек
    if (m_activeTab == LevelTab::Campaign) {
        float fTitle = std::clamp(0.58f * uiScale, 0.42f, 0.76f);
        float fSub = std::clamp(0.44f * uiScale, 0.32f, 0.56f);
        float fPlay = std::clamp(0.54f * uiScale, 0.40f, 0.72f);
        float fSmall = std::clamp(0.42f * uiScale, 0.30f, 0.54f);

        for (const auto& card : m_campaignCards) {
            std::string missionPrefix = (card.order < 10 ? "0" : "") + std::to_string(card.order) + ". ";
            float tx = card.cardBtn.pos.x + 20.0f * uiScale;
            float ty = card.cardBtn.pos.y + 16.0f * uiScale;

            m_textRenderer->RenderText(missionPrefix + card.displayName, tx, ty, fTitle, card.isUnlocked ? glm::vec3(1.0f) : glm::vec3(0.60f));

            std::string desc = card.description.empty() ? ("Файл: " + card.filename) : card.description;
            m_textRenderer->RenderText(desc, tx, ty + fTitle * 28.0f + 6.0f, fSub, glm::vec3(0.65f, 0.68f, 0.75f));

            std::string statusText = card.isCompleted ? "[ ПРОЙДЕНО ★ ]" : (card.isUnlocked ? "[ ДОСТУПНО ]" : "[ ЗАБЛОКИРОВАНО ]");
            glm::vec3 statusColor = card.isCompleted ? glm::vec3(0.28f, 0.85f, 0.40f) :
                                   (card.isUnlocked ? glm::vec3(0.40f, 0.70f, 1.0f) : glm::vec3(0.55f, 0.55f, 0.60f));
            m_textRenderer->RenderText(statusText, tx, ty + fTitle * 28.0f + fSub * 28.0f + 10.0f, fSub, statusColor);

            float pw = m_textRenderer->CalculateTextWidth(card.playBtn.text, fPlay);
            m_textRenderer->RenderText(card.playBtn.text, card.playBtn.pos.x + (card.playBtn.size.x - pw) * 0.5f,
                                       card.playBtn.pos.y + (card.playBtn.size.y - fPlay * 28.0f) * 0.5f + 2.0f, fPlay,
                                       card.isUnlocked ? glm::vec3(1.0f) : glm::vec3(0.50f));

            if (m_isDevMode) {
                auto renderSmallText = [&](const UIButton& btn) {
                    float sw = m_textRenderer->CalculateTextWidth(btn.text, fSmall);
                    m_textRenderer->RenderText(btn.text, btn.pos.x + (btn.size.x - sw) * 0.5f,
                                               btn.pos.y + (btn.size.y - fSmall * 28.0f) * 0.5f + 2.0f, fSmall, glm::vec3(1.0f));
                };
                renderSmallText(card.btnUp);
                renderSmallText(card.btnDown);
                renderSmallText(card.btnEdit);
                renderSmallText(card.btnRemove);
            }
        }
    } else {
        float fTitle = std::clamp(0.54f * uiScale, 0.40f, 0.70f);
        float fSub = std::clamp(0.42f * uiScale, 0.30f, 0.52f);
        float fBtn = std::clamp(0.44f * uiScale, 0.32f, 0.56f);

        std::string sText = m_searchQuery.empty() ? LOC("LEVEL_SEARCH_HINT") : m_searchQuery;
        glm::vec3 sCol = m_searchQuery.empty() ? glm::vec3(0.50f) : glm::vec3(1.0f);
        m_textRenderer->RenderText(sText, m_searchBoxPos.x + 10.0f,
                                   m_searchBoxPos.y + (m_searchBoxSize.y - fSub * 28.0f) * 0.5f + 2.0f, fSub, sCol);

        if (m_customCards.empty()) {
            std::string emptyMsg = "Кастомных карт пока нет. Нажмите '+ СОЗДАТЬ КАРТУ' справа внизу!";
            float ew = m_textRenderer->CalculateTextWidth(emptyMsg, fTitle);
            m_textRenderer->RenderText(emptyMsg, (m_width - ew) * 0.5f, m_height * 0.45f, fTitle, glm::vec3(0.60f, 0.65f, 0.72f));
        }

        for (const auto& card : m_customCards) {
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
    }

    // 4. Текст нижней навигации
    auto renderNavText = [&](const UIButton& btn) {
        float tw = m_textRenderer->CalculateTextWidth(btn.text, fNav);
        m_textRenderer->RenderText(btn.text, btn.pos.x + (btn.size.x - tw) * 0.5f,
                                   btn.pos.y + (btn.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(1.0f));
    };

    renderNavText(m_btnBack);
    if (m_activeTab == LevelTab::Custom) {
        renderNavText(m_btnEditor);
    }

    if (m_totalPages > 1) {
        if (m_currentPage > 0) renderNavText(m_btnPrevPage);
        if (m_currentPage < m_totalPages - 1) renderNavText(m_btnNextPage);

        std::string pageStr = std::to_string(m_currentPage + 1) + " / " + std::to_string(m_totalPages);
        float pw = m_textRenderer->CalculateTextWidth(pageStr, fNav);
        m_textRenderer->RenderText(pageStr, (m_width - pw) * 0.5f,
                                   m_btnPrevPage.pos.y + (m_btnPrevPage.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(0.80f));
    }

    // 5. Модальные окна
    if (m_isRenameModalOpen) {
        renderRenameModal(uiScale);
    }
    if (m_isDeleteModalOpen) {
        renderDeleteModal(uiScale);
    }
    if (m_isAddMissionModalOpen) {
        renderAddMissionModal(uiScale);
    }
}

void LevelSelectState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    init();
}

// === Модальное окно переименования ===
void LevelSelectState::openRenameModal(const std::string& targetFileName) {
    m_renameTargetFileName = targetFileName;
    m_renameInputText.clear();
    for (const auto& c : m_customCards) {
        if (c.filename == targetFileName) {
            m_renameInputText = c.displayName;
            break;
        }
    }
    m_isRenameModalOpen = true;
}

void LevelSelectState::confirmRename() {
    if (!m_renameInputText.empty()) {
        LevelManager::setLevelDisplayName(m_renameTargetFileName, m_renameInputText);
    }
    m_isRenameModalOpen = false;
    init();
}

bool LevelSelectState::processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
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

    if (leftDown && !m_mousePressedLastFrame) {
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

void LevelSelectState::renderRenameModal(float uiScale) {
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
void LevelSelectState::openDeleteModal(const std::string& targetFileName) {
    m_deleteTargetFileName = targetFileName;
    m_isDeleteModalOpen = true;
}

void LevelSelectState::confirmDelete() {
    if (!m_deleteTargetFileName.empty()) {
        LevelManager::deleteLevel(m_deleteTargetFileName);
    }
    m_isDeleteModalOpen = false;
    init();
}

bool LevelSelectState::processDeleteModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
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

    if (leftDown && !m_mousePressedLastFrame) {
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

void LevelSelectState::renderDeleteModal(float uiScale) {
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

// === Модальное окно добавления миссии в кампанию (Dev Mode) ===
void LevelSelectState::openAddMissionModal() {
    m_availableFilesToAdd.clear();
    auto all = LevelManager::getAvailableLevels();
    for (const auto& lvl : all) {
        if (!CampaignManager::isFileInCampaign(lvl.filename)) {
            m_availableFilesToAdd.push_back(lvl.filename);
        }
    }
    m_addMissionScrollOffset = 0;
    m_isAddMissionModalOpen = true;
}

bool LevelSelectState::processAddMissionModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isAddMissionModalOpen) return false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_isAddMissionModalOpen = false;
        return true;
    }

    float uiScale = SettingsManager::getUIScaleMultiplier();
    float mW = std::clamp(500.0f * uiScale, 340.0f, static_cast<float>(m_width) - 40.0f);
    float mH = std::clamp(380.0f * uiScale, 260.0f, static_cast<float>(m_height) - 40.0f);
    glm::vec2 mPos((m_width - mW) * 0.5f, (m_height - mH) * 0.5f);

    float itemH = std::clamp(38.0f * uiScale, 28.0f, 48.0f);
    float startY = mPos.y + 60.0f * uiScale;

    if (leftDown && !m_mousePressedLastFrame) {
        for (size_t i = 0; i < m_availableFilesToAdd.size(); ++i) {
            float y = startY + i * (itemH + 6.0f);
            if (y + itemH > mPos.y + mH - 50.0f * uiScale) break;

            if (isPointInRect(mousePos, glm::vec2(mPos.x + 20.0f * uiScale, y), glm::vec2(mW - 40.0f * uiScale, itemH))) {
                const std::string& file = m_availableFilesToAdd[i];
                CampaignManager::addMission(file, file, "Пользовательская миссия кампании");
                m_isAddMissionModalOpen = false;
                init();
                return true;
            }
        }

        // Кнопка закрыть внизу
        float btnCloseW = std::clamp(140.0f * uiScale, 100.0f, 180.0f);
        float btnCloseH = std::clamp(34.0f * uiScale, 26.0f, 44.0f);
        glm::vec2 btnClosePos(mPos.x + (mW - btnCloseW) * 0.5f, mPos.y + mH - btnCloseH - 12.0f * uiScale);
        if (isPointInRect(mousePos, btnClosePos, glm::vec2(btnCloseW, btnCloseH))) {
            m_isAddMissionModalOpen = false;
            return true;
        }
    }

    return true;
}

void LevelSelectState::renderAddMissionModal(float uiScale) {
    float mW = std::clamp(500.0f * uiScale, 340.0f, static_cast<float>(m_width) - 40.0f);
    float mH = std::clamp(380.0f * uiScale, 260.0f, static_cast<float>(m_height) - 40.0f);
    glm::vec2 mPos((m_width - mW) * 0.5f, (m_height - mH) * 0.5f);

    float itemH = std::clamp(38.0f * uiScale, 28.0f, 48.0f);
    float startY = mPos.y + 54.0f * uiScale;

    float btnCloseW = std::clamp(140.0f * uiScale, 100.0f, 180.0f);
    float btnCloseH = std::clamp(34.0f * uiScale, 26.0f, 44.0f);
    glm::vec2 btnClosePos(mPos.x + (mW - btnCloseW) * 0.5f, mPos.y + mH - btnCloseH - 12.0f * uiScale);

    // 1. Quads
    m_renderer->beginBatch();
    drawQuadRGBA(glm::vec2(0.0f), glm::vec2(m_width, m_height), glm::vec4(0.0f, 0.0f, 0.0f, 0.75f), m_whiteTexture);
    drawQuad(mPos, glm::vec2(mW, mH), glm::vec3(0.16f, 0.18f, 0.24f), m_whiteTexture);
    drawQuad(mPos, glm::vec2(mW, 40.0f * uiScale), glm::vec3(0.20f, 0.25f, 0.35f), m_whiteTexture);

    for (size_t i = 0; i < m_availableFilesToAdd.size(); ++i) {
        float y = startY + i * (itemH + 6.0f);
        if (y + itemH > mPos.y + mH - 50.0f * uiScale) break;

        glm::vec2 itemPos(mPos.x + 20.0f * uiScale, y);
        glm::vec2 itemSize(mW - 40.0f * uiScale, itemH);
        drawQuad(itemPos, itemSize, glm::vec3(0.22f, 0.25f, 0.32f), m_whiteTexture);
    }
    drawQuad(btnClosePos, glm::vec2(btnCloseW, btnCloseH), glm::vec3(0.32f, 0.35f, 0.40f), m_whiteTexture);
    m_renderer->endBatch();

    // 2. Text
    float fTitle = std::clamp(0.56f * uiScale, 0.40f, 0.72f);
    float fItem = std::clamp(0.48f * uiScale, 0.34f, 0.62f);
    float fBtn = std::clamp(0.48f * uiScale, 0.34f, 0.60f);

    m_textRenderer->RenderText("ДОБАВИТЬ КАРТУ В КАМПАНИЮ", mPos.x + 20.0f * uiScale, mPos.y + 12.0f * uiScale, fTitle, glm::vec3(1.0f));

    if (m_availableFilesToAdd.empty()) {
        m_textRenderer->RenderText("Нет доступных свободных карт для добавления.", mPos.x + 24.0f * uiScale, startY + 20.0f, fItem, glm::vec3(0.70f));
    }

    for (size_t i = 0; i < m_availableFilesToAdd.size(); ++i) {
        float y = startY + i * (itemH + 6.0f);
        if (y + itemH > mPos.y + mH - 50.0f * uiScale) break;

        glm::vec2 itemPos(mPos.x + 20.0f * uiScale, y);
        m_textRenderer->RenderText("+ " + m_availableFilesToAdd[i], itemPos.x + 14.0f * uiScale, itemPos.y + (itemH - fItem * 28.0f) * 0.5f + 2.0f, fItem, glm::vec3(0.95f));
    }

    float cw = m_textRenderer->CalculateTextWidth(LOC("TAGS_CLOSE"), fBtn);
    m_textRenderer->RenderText(LOC("TAGS_CLOSE"), btnClosePos.x + (btnCloseW - cw) * 0.5f, btnClosePos.y + (btnCloseH - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f));
}