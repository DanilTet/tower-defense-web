#include "CampaignTabView.h"
#include "../../states/GameStateManager.h"
#include "../../states/GameplayState.h"
#include "../../states/MapEditorState.h"
#include "../../core/CampaignManager.h"
#include "../../core/LevelManager.h"
#include "../../core/LocalizationManager.h"
#include "../../core/SettingsManager.h"
#include "../../../renderer/SpriteRenderer.h"
#include "../../../renderer/TextRenderer.h"
#include "../../../textures/Texture2D.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

CampaignTabView::CampaignTabView(GameStateManager& stateManager,
                                 std::shared_ptr<SpriteRenderer> renderer,
                                 TextRenderer* textRenderer,
                                 const Texture2D* whiteTexture)
    : m_stateManager(stateManager),
      m_renderer(renderer),
      m_textRenderer(textRenderer),
      m_whiteTexture(whiteTexture)
{
}

void CampaignTabView::init(int screenWidth, int screenHeight, float tabY, float tabH, float bottomY, float navBtnH, bool isDevMode) {
    m_width = screenWidth;
    m_height = screenHeight;
    m_tabY = tabY;
    m_tabH = tabH;
    m_bottomY = bottomY;
    m_navBtnH = navBtnH;
    m_isDevMode = isDevMode;
    initLayout();
}

void CampaignTabView::initLayout() {
    m_cards.clear();

    const auto& missions = CampaignManager::getMissions();
    const int itemsPerPage = 3;
    m_totalPages = std::max(1, static_cast<int>((missions.size() + itemsPerPage - 1) / itemsPerPage));
    if (m_currentPage >= m_totalPages) m_currentPage = m_totalPages - 1;
    if (m_currentPage < 0) m_currentPage = 0;

    int startIndex = m_currentPage * itemsPerPage;
    int endIndex = std::min(startIndex + itemsPerPage, static_cast<int>(missions.size()));

    float uiScale = SettingsManager::getUIScaleMultiplier();
    float startTabsX = std::clamp(28.0f * uiScale, 16.0f, 40.0f);
    float tabW = std::clamp(160.0f * uiScale, 110.0f, 220.0f);
    float tabGap = std::clamp(8.0f * uiScale, 6.0f, 14.0f);

    float fDevBadge = std::clamp(0.44f * uiScale, 0.32f, 0.55f);
    std::string badgeText = m_isDevMode ? LOC("CAMPAIGN_DEV_MODE_ON") : "[ DEV MODE: ВЫКЛ ]";
    float bW = m_textRenderer ? (m_textRenderer->CalculateTextWidth(badgeText, fDevBadge) + 18.0f) : (140.0f * uiScale);
    float bX = startTabsX + tabW + tabGap;

    // Пагинация
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

    // Кнопки управления автора в Dev Mode (правая верхняя часть экрана)
    if (m_isDevMode) {
        float devBtnH = m_tabH;
        float addMissionBtnW = std::clamp(145.0f * uiScale, 110.0f, 185.0f);
        float unlockBtnW = std::clamp(115.0f * uiScale, 85.0f, 145.0f);
        float btnDevGap = std::clamp(6.0f * uiScale, 4.0f, 10.0f);

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
        m_btnDevResetProgress.pos = glm::vec2(rightX - unlockBtnW, m_tabY);
        m_btnDevResetProgress.text = "СБРОСИТЬ";

        m_btnDevUnlockAll.size = glm::vec2(unlockBtnW, devBtnH);
        m_btnDevUnlockAll.pos = glm::vec2(m_btnDevResetProgress.pos.x - unlockBtnW - btnDevGap, m_tabY);
        m_btnDevUnlockAll.text = "ОТКРЫТЬ ВСЕ";

        m_btnDevAddMission.size = glm::vec2(addMissionBtnW, devBtnH);
        m_btnDevAddMission.pos = glm::vec2(m_btnDevUnlockAll.pos.x - addMissionBtnW - btnDevGap, m_tabY);
        m_btnDevAddMission.text = "+ В КАМПАНИЮ";
    }

    float topY = m_tabY + m_tabH + std::clamp(16.0f * uiScale, 10.0f, 26.0f);
    float bottomAreaH = m_navBtnH + 28.0f;
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

        float playX = cardX + cardW - playBtnW - 14.0f * uiScale;
        float playY = y + (cardH - playBtnH) * 0.5f;
        card.playBtn.pos = glm::vec2(playX, playY);
        card.playBtn.size = glm::vec2(playBtnW, playBtnH);
        card.playBtn.text = card.isUnlocked ? LOC("LEVEL_BTN_PLAY") : LOC("CAMPAIGN_STATUS_LOCKED");
        card.playBtn.state = 0;

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

        m_cards.push_back(card);
    }
}

bool CampaignTabView::processInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt) {
    if (m_isAddMissionModalOpen) {
        return processAddMissionModalInput(window, mousePos, leftDown, leftPressed, dt);
    }

    if (leftPressed) {
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

        // Кнопки Dev Mode
        if (m_isDevMode) {
            if (isPointInRect(mousePos, m_btnDevAddMission.pos, m_btnDevAddMission.size)) {
                openAddMissionModal();
                return true;
            }
            if (isPointInRect(mousePos, m_btnDevUnlockAll.pos, m_btnDevUnlockAll.size)) {
                CampaignManager::unlockAllProgress();
                initLayout();
                return true;
            }
            if (isPointInRect(mousePos, m_btnDevResetProgress.pos, m_btnDevResetProgress.size)) {
                CampaignManager::resetProgress();
                initLayout();
                return true;
            }
        }

        // Клики по карточкам
        for (size_t idx = 0; idx < m_cards.size(); ++idx) {
            auto& card = m_cards[idx];
            int globalMissionIdx = m_currentPage * 3 + static_cast<int>(idx);

            // Клик по Играть / по карточке
            if (isPointInRect(mousePos, card.playBtn.pos, card.playBtn.size) ||
                isPointInRect(mousePos, card.cardBtn.pos, card.cardBtn.size)) {
                if (card.isUnlocked || m_isDevMode) {
                    std::string fullPath = "res/levels/" + card.filename;
                    std::cout << "[LevelSelect] Launching campaign mission: " << card.displayName << " (" << fullPath << ")" << std::endl;
                    m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, fullPath, false, GameplayOrigin::Campaign));
                    return true;
                }
            }

            // Кнопки Dev Mode на карточке
            if (m_isDevMode) {
                if (isPointInRect(mousePos, card.btnUp.pos, card.btnUp.size)) {
                    CampaignManager::moveMissionUp(globalMissionIdx);
                    initLayout();
                    return true;
                }
                if (isPointInRect(mousePos, card.btnDown.pos, card.btnDown.size)) {
                    CampaignManager::moveMissionDown(globalMissionIdx);
                    initLayout();
                    return true;
                }
                if (isPointInRect(mousePos, card.btnEdit.pos, card.btnEdit.size)) {
                    std::cout << "[LevelSelect] Dev editing campaign level: " << card.filename << std::endl;
                    m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.filename, EditorOrigin::Campaign));
                    return true;
                }
                if (isPointInRect(mousePos, card.btnRemove.pos, card.btnRemove.size)) {
                    CampaignManager::removeMission(globalMissionIdx);
                    initLayout();
                    return true;
                }
            }
        }
    }

    return false;
}

void CampaignTabView::update(float dt) {
    (void)dt;
}

void CampaignTabView::renderGeometry(float uiScale, bool isDevMode) {
    // 1. Dev Mode кнопки
    if (isDevMode) {
        drawQuad(m_btnDevAddMission.pos, m_btnDevAddMission.size, glm::vec3(0.18f, 0.46f, 0.28f), m_whiteTexture);
        drawQuad(m_btnDevUnlockAll.pos, m_btnDevUnlockAll.size, glm::vec3(0.25f, 0.35f, 0.50f), m_whiteTexture);
        drawQuad(m_btnDevResetProgress.pos, m_btnDevResetProgress.size, glm::vec3(0.50f, 0.22f, 0.22f), m_whiteTexture);
    }

    // 2. Карточки
    for (const auto& card : m_cards) {
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

        if (isDevMode) {
            drawQuad(card.btnUp.pos, card.btnUp.size, glm::vec3(0.22f, 0.32f, 0.44f), m_whiteTexture);
            drawQuad(card.btnDown.pos, card.btnDown.size, glm::vec3(0.22f, 0.32f, 0.44f), m_whiteTexture);
            drawQuad(card.btnEdit.pos, card.btnEdit.size, glm::vec3(0.28f, 0.44f, 0.30f), m_whiteTexture);
            drawQuad(card.btnRemove.pos, card.btnRemove.size, glm::vec3(0.52f, 0.24f, 0.24f), m_whiteTexture);
        }
    }

    // 3. Пагинация
    if (m_totalPages > 1) {
        if (m_currentPage > 0) drawQuad(m_btnPrevPage.pos, m_btnPrevPage.size, glm::vec3(0.20f, 0.24f, 0.30f), m_whiteTexture);
        if (m_currentPage < m_totalPages - 1) drawQuad(m_btnNextPage.pos, m_btnNextPage.size, glm::vec3(0.20f, 0.24f, 0.30f), m_whiteTexture);
    }
}

void CampaignTabView::renderText(float uiScale, bool isDevMode) {
    // 1. Dev Mode текст кнопок или подсказка
    if (isDevMode) {
        auto renderDevBtnText = [&](const UIButton& btn) {
            float fBtn = std::clamp(0.44f * uiScale, 0.32f, 0.58f);
            float bw = m_textRenderer->CalculateTextWidth(btn.text, fBtn);
            m_textRenderer->RenderText(btn.text, btn.pos.x + (btn.size.x - bw) * 0.5f,
                                       btn.pos.y + (btn.size.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f));
        };
        renderDevBtnText(m_btnDevAddMission);
        renderDevBtnText(m_btnDevUnlockAll);
        renderDevBtnText(m_btnDevResetProgress);
    } else {
        float fHint = std::clamp(0.40f * uiScale, 0.30f, 0.52f);
        std::string hint = LOC("CAMPAIGN_DEV_HINT");
        float rightX = static_cast<float>(m_width) - 36.0f * uiScale - m_textRenderer->CalculateTextWidth(hint, fHint);
        m_textRenderer->RenderText(hint, rightX, m_tabY + (m_tabH - fHint * 28.0f) * 0.5f + 2.0f, fHint, glm::vec3(0.50f, 0.52f, 0.58f));
    }

    // 2. Текст карточек
    float fTitle = std::clamp(0.58f * uiScale, 0.42f, 0.76f);
    float fSub = std::clamp(0.44f * uiScale, 0.32f, 0.56f);
    float fPlay = std::clamp(0.54f * uiScale, 0.40f, 0.72f);
    float fSmall = std::clamp(0.42f * uiScale, 0.30f, 0.54f);

    for (const auto& card : m_cards) {
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

        if (isDevMode) {
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

    // 3. Текст пагинации
    if (m_totalPages > 1) {
        float fNav = std::clamp(0.54f * uiScale, 0.40f, 0.70f);
        auto renderNavText = [&](const UIButton& btn) {
            float tw = m_textRenderer->CalculateTextWidth(btn.text, fNav);
            m_textRenderer->RenderText(btn.text, btn.pos.x + (btn.size.x - tw) * 0.5f,
                                       btn.pos.y + (btn.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(1.0f));
        };
        if (m_currentPage > 0) renderNavText(m_btnPrevPage);
        if (m_currentPage < m_totalPages - 1) renderNavText(m_btnNextPage);

        std::string pageStr = std::to_string(m_currentPage + 1) + " / " + std::to_string(m_totalPages);
        float pw = m_textRenderer->CalculateTextWidth(pageStr, fNav);
        m_textRenderer->RenderText(pageStr, (m_width - pw) * 0.5f,
                                   m_btnPrevPage.pos.y + (m_btnPrevPage.size.y - fNav * 28.0f) * 0.5f + 2.0f, fNav, glm::vec3(0.80f));
    }

    // 4. Модальное окно добавления миссии
    if (m_isAddMissionModalOpen) {
        renderAddMissionModal(uiScale);
    }
}

void CampaignTabView::openAddMissionModal() {
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

bool CampaignTabView::processAddMissionModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, bool leftPressed, float dt) {
    (void)leftDown;
    (void)dt;
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

    if (leftPressed) {
        for (size_t i = 0; i < m_availableFilesToAdd.size(); ++i) {
            float y = startY + i * (itemH + 6.0f);
            if (y + itemH > mPos.y + mH - 50.0f * uiScale) break;

            if (isPointInRect(mousePos, glm::vec2(mPos.x + 20.0f * uiScale, y), glm::vec2(mW - 40.0f * uiScale, itemH))) {
                const std::string& file = m_availableFilesToAdd[i];
                CampaignManager::addMission(file, file, "Пользовательская миссия кампании");
                m_isAddMissionModalOpen = false;
                initLayout();
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

void CampaignTabView::renderAddMissionModal(float uiScale) {
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

bool CampaignTabView::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void CampaignTabView::drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec3 color, const Texture2D* tex) {
    if (m_renderer) {
        m_renderer->drawSprite(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

void CampaignTabView::drawQuadRGBA(glm::vec2 pos, glm::vec2 size, glm::vec4 color, const Texture2D* tex) {
    if (m_renderer) {
        m_renderer->drawSpriteRGBA(tex ? tex : m_whiteTexture, pos, size, 0.0f, color);
    }
}

