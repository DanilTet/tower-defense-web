#include "EditorMapsBrowserModal.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../../resources/ResourceManager.h"
#include "../ui/UICommon.h"
#include "../core/InputManager.h"
#include "../core/LocalizationManager.h"
#include "../core/CampaignManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void EditorMapsBrowserModal::open(const std::string& currentLevelFileName, bool isInitialLaunch) {
    m_isOpen = true;
    m_isRenameModalOpen = false;
    m_isInitialModalLaunch = isInitialLaunch;
    m_mapsScrollOffset = 0;
    if (!currentLevelFileName.empty()) {
        m_currentLevelFileName = currentLevelFileName;
    }
    m_wasLeftDown = true;
    m_keyStates.clear();
}

void EditorMapsBrowserModal::close() {
    m_isOpen = false;
    m_isRenameModalOpen = false;
    m_isInitialModalLaunch = false;
    m_keyStates.clear();
}

void EditorMapsBrowserModal::toggle(const std::string& currentLevelFileName) {
    if (m_isOpen || m_isRenameModalOpen) {
        close();
    } else {
        open(currentLevelFileName, false);
    }
}

void EditorMapsBrowserModal::refreshFiles() {
    m_mapsScrollOffset = 0;
}

void EditorMapsBrowserModal::openRename(const std::string& targetFileName, const std::string& currentDisplayName) {
    m_renameTargetFileName = targetFileName.empty() ? m_currentLevelFileName : targetFileName;
    m_renameInputText = currentDisplayName;
    m_isRenameModalOpen = true;
    m_cursorBlinkTimer = 0.0f;
    m_wasLeftDown = true;
    m_keyStates.clear();
}

void EditorMapsBrowserModal::closeRename() {
    m_isRenameModalOpen = false;
    m_keyStates.clear();
}

void EditorMapsBrowserModal::update(float dt) {
    m_cursorBlinkTimer += dt;
    if (m_cursorBlinkTimer >= 1.0f) {
        m_cursorBlinkTimer -= 1.0f;
    }
}

bool EditorMapsBrowserModal::checkKeyRepeat(GLFWwindow* window, int key, float dt) {
    if (!window) return false;
    bool down = (glfwGetKey(window, key) == GLFW_PRESS);
    auto& ks = m_keyStates[key];
    if (down) {
        if (!ks.isDown) {
            ks.isDown = true;
            ks.holdTimer = 0.0f;
            ks.repeatTimer = 0.0f;
            return true;
        } else {
            ks.holdTimer += dt;
            if (ks.holdTimer >= 0.30f) {
                ks.repeatTimer += dt;
                if (ks.repeatTimer >= 0.06f) {
                    ks.repeatTimer = 0.0f;
                    return true;
                }
            }
        }
    } else {
        ks.isDown = false;
        ks.holdTimer = 0.0f;
        ks.repeatTimer = 0.0f;
    }
    return false;
}

void EditorMapsBrowserModal::confirmRename(const std::function<void(const std::string&, const std::string&, const std::string&)>& onRenameLevel) {
    if (m_renameInputText.empty()) {
        m_isRenameModalOpen = false;
        return;
    }

    std::string targetOld = m_renameTargetFileName;
    LevelManager::setLevelDisplayName(m_renameTargetFileName, m_renameInputText);

    std::string safeBase = InputManager::transliterateToAscii(m_renameInputText);
    std::string newFileName = LevelManager::sanitizeLevelFileName(safeBase);
    if (newFileName != m_renameTargetFileName) {
        bool ok = LevelManager::renameLevel(m_renameTargetFileName, newFileName);
        if (ok) {
            if (m_renameTargetFileName == m_currentLevelFileName) {
                m_currentLevelFileName = newFileName;
            }
        }
    }
    m_isRenameModalOpen = false;
    if (onRenameLevel) {
        onRenameLevel(targetOld, newFileName, m_renameInputText);
    }
}

std::vector<LevelInfo> EditorMapsBrowserModal::getFilteredModalLevels() const {
    auto all = LevelManager::getAvailableLevels();
    bool dev = CampaignManager::isDevMode();
    if (dev) {
        return all;
    }
    // Если Dev Mode выключен - показываем только кастомные уровни
    std::vector<LevelInfo> filtered;
    for (const auto& lvl : all) {
        if (!CampaignManager::isFileInCampaign(lvl.filename)) {
            filtered.push_back(lvl);
        }
    }
    return filtered;
}

RenameModalLayout EditorMapsBrowserModal::getRenameModalLayout(int screenWidth, int screenHeight) const {
    RenameModalLayout layout;
    float scale = GetUIScale(screenWidth, screenHeight);

    layout.fTitle = std::clamp(0.68f * scale, 0.48f, 0.86f);
    layout.fSub = std::clamp(0.48f * scale, 0.35f, 0.62f);
    layout.fInput = std::clamp(0.58f * scale, 0.42f, 0.72f);
    layout.fBtn = std::clamp(0.48f * scale, 0.36f, 0.62f);
    layout.fCross = std::clamp(0.55f * scale, 0.40f, 0.75f);

    layout.modalSize.x = std::clamp(480.0f * scale, 360.0f, static_cast<float>(screenWidth) - 40.0f);
    layout.modalSize.y = std::clamp(230.0f * scale, 180.0f, static_cast<float>(screenHeight) - 40.0f);
    layout.modalPos = glm::vec2((static_cast<float>(screenWidth) - layout.modalSize.x) * 0.5f,
                                (static_cast<float>(screenHeight) - layout.modalSize.y) * 0.5f);

    layout.headerH = std::clamp(40.0f * scale, 30.0f, 54.0f);

    float crossSize = std::clamp(26.0f * scale, 20.0f, 34.0f);
    layout.btnCloseCrossSize = glm::vec2(crossSize, crossSize);
    layout.btnCloseCrossPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - crossSize - std::clamp(8.0f * scale, 6.0f, 12.0f),
                                        layout.modalPos.y + (layout.headerH - crossSize) * 0.5f);

    layout.subY = layout.modalPos.y + layout.headerH + std::clamp(10.0f * scale, 7.0f, 14.0f);

    float boxMarginX = std::clamp(24.0f * scale, 16.0f, 34.0f);
    float boxH = std::clamp(38.0f * scale, 28.0f, 48.0f);
    float boxY = layout.subY + layout.fSub * 28.0f + std::clamp(8.0f * scale, 5.0f, 12.0f);
    layout.boxPos = glm::vec2(layout.modalPos.x + boxMarginX, boxY);
    layout.boxSize = glm::vec2(layout.modalSize.x - 2.0f * boxMarginX, boxH);

    float btnH = std::clamp(40.0f * scale, 30.0f, 50.0f);
    float btnMarginBottom = std::clamp(16.0f * scale, 10.0f, 22.0f);
    float btnY = layout.modalPos.y + layout.modalSize.y - btnH - btnMarginBottom;
    float btnGap = std::clamp(14.0f * scale, 10.0f, 20.0f);
    float availBtnW = layout.modalSize.x - 2.0f * boxMarginX - btnGap;
    float btnW = availBtnW * 0.5f;

    layout.btnSavePos = glm::vec2(layout.modalPos.x + boxMarginX, btnY);
    layout.btnSaveSize = glm::vec2(btnW, btnH);
    layout.btnCancelPos = glm::vec2(layout.btnSavePos.x + btnW + btnGap, btnY);
    layout.btnCancelSize = glm::vec2(btnW, btnH);

    return layout;
}

MapsModalLayout EditorMapsBrowserModal::getMapsModalLayout(int screenWidth, int screenHeight) const {
    MapsModalLayout layout;
    float scale = GetUIScale(screenWidth, screenHeight);

    layout.fTitle = std::clamp(0.68f * scale, 0.48f, 0.88f);
    layout.fSub = std::clamp(0.50f * scale, 0.38f, 0.65f);
    layout.fNew = std::clamp(0.50f * scale, 0.36f, 0.64f);
    layout.fCardName = std::clamp(0.54f * scale, 0.38f, 0.70f);
    layout.fCardSub = std::clamp(0.42f * scale, 0.30f, 0.54f);
    layout.fActionBtn = std::clamp(0.48f * scale, 0.35f, 0.62f);
    layout.fClose = std::clamp(0.52f * scale, 0.38f, 0.68f);
    layout.fCross = std::clamp(0.55f * scale, 0.40f, 0.75f);

    layout.modalSize.x = std::clamp(640.0f * scale, 480.0f, static_cast<float>(screenWidth) - 40.0f);
    layout.modalSize.y = std::clamp(520.0f * scale, 380.0f, static_cast<float>(screenHeight) - 40.0f);
    layout.modalPos = glm::vec2((static_cast<float>(screenWidth) - layout.modalSize.x) * 0.5f,
                                (static_cast<float>(screenHeight) - layout.modalSize.y) * 0.5f);

    layout.headerH = std::clamp(42.0f * scale, 32.0f, 56.0f);

    float crossSize = std::clamp(26.0f * scale, 20.0f, 34.0f);
    layout.btnCloseCrossSize = glm::vec2(crossSize, crossSize);
    layout.btnCloseCrossPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - crossSize - std::clamp(8.0f * scale, 6.0f, 12.0f),
                                        layout.modalPos.y + (layout.headerH - crossSize) * 0.5f);

    float subMarginTop = std::clamp(10.0f * scale, 8.0f, 16.0f);
    float subY = layout.modalPos.y + layout.headerH + subMarginTop;

    float btnNewW = std::clamp(160.0f * scale, 120.0f, 210.0f);
    float btnNewH = std::clamp(32.0f * scale, 24.0f, 42.0f);
    layout.btnNewSize = glm::vec2(btnNewW, btnNewH);
    layout.btnNewPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - btnNewW - std::clamp(20.0f * scale, 14.0f, 28.0f), subY);

    float scrollBtnW = std::clamp(30.0f * scale, 24.0f, 38.0f);
    float scrollBtnH = btnNewH;
    layout.btnScrollDownSize = glm::vec2(scrollBtnW, scrollBtnH);
    layout.btnScrollDownPos = glm::vec2(layout.btnNewPos.x - scrollBtnW - std::clamp(8.0f * scale, 4.0f, 12.0f), subY);
    layout.btnScrollUpSize = glm::vec2(scrollBtnW, scrollBtnH);
    layout.btnScrollUpPos = glm::vec2(layout.btnScrollDownPos.x - scrollBtnW - std::clamp(4.0f * scale, 2.0f, 6.0f), subY);

    float btnBottomH = std::clamp(36.0f * scale, 26.0f, 46.0f);
    float btnBottomMargin = std::clamp(14.0f * scale, 10.0f, 20.0f);
    float bottomY = layout.modalPos.y + layout.modalSize.y - btnBottomH - btnBottomMargin;

    float btnCloseW = std::clamp(130.0f * scale, 95.0f, 170.0f);
    layout.btnCloseSize = glm::vec2(btnCloseW, btnBottomH);
    layout.btnClosePos = glm::vec2(layout.modalPos.x + (layout.modalSize.x - btnCloseW) * 0.5f, bottomY);

    float navBtnW = std::clamp(80.0f * scale, 56.0f, 104.0f);
    float navGap = std::clamp(14.0f * scale, 8.0f, 20.0f);
    layout.btnPrevPageSize = glm::vec2(navBtnW, btnBottomH);
    layout.btnPrevPagePos = glm::vec2(layout.btnClosePos.x - navGap - navBtnW, bottomY);
    layout.btnNextPageSize = glm::vec2(navBtnW, btnBottomH);
    layout.btnNextPagePos = glm::vec2(layout.btnClosePos.x + btnCloseW + navGap, bottomY);

    layout.listStartY = subY + btnNewH + std::clamp(10.0f * scale, 6.0f, 16.0f);
    float listEndY = bottomY - std::clamp(10.0f * scale, 6.0f, 16.0f);
    float availListH = listEndY - layout.listStartY;

    layout.itemH = std::clamp(48.0f * scale, 36.0f, 64.0f);
    layout.itemGap = std::clamp(6.0f * scale, 4.0f, 10.0f);

    layout.maxVisible = std::max(1, static_cast<int>((availListH + layout.itemGap) / (layout.itemH + layout.itemGap)));

    auto levels = getFilteredModalLevels();
    layout.totalLevels = static_cast<int>(levels.size());
    layout.hasPagination = (layout.totalLevels > layout.maxVisible);
    layout.maxOffset = std::max(0, layout.totalLevels - layout.maxVisible);
    layout.currentOffset = std::clamp(m_mapsScrollOffset, 0, layout.maxOffset);

    float cardMarginX = std::clamp(18.0f * scale, 12.0f, 26.0f);
    float scrollbarSpace = layout.hasPagination ? std::clamp(14.0f * scale, 10.0f, 20.0f) : 0.0f;
    float cardW = layout.modalSize.x - 2.0f * cardMarginX - scrollbarSpace;

    float btnLoadW = std::clamp(64.0f * scale, 46.0f, 84.0f);
    float btnRenW = std::clamp(72.0f * scale, 52.0f, 96.0f);
    float btnDelW = std::clamp(50.0f * scale, 36.0f, 68.0f);
    float btnActH = std::clamp(28.0f * scale, 22.0f, 38.0f);
    float btnActGap = std::clamp(6.0f * scale, 4.0f, 10.0f);
    float rightMargin = std::clamp(10.0f * scale, 6.0f, 14.0f);

    for (int i = 0; i < layout.maxVisible && (i + layout.currentOffset) < layout.totalLevels; ++i) {
        int idx = i + layout.currentOffset;
        const auto& lvl = levels[idx];

        MapCardLayout card;
        card.levelIdx = idx;
        card.levelInfo = lvl;
        card.hasDel = !lvl.isBuiltIn;
        card.cardPos = glm::vec2(layout.modalPos.x + cardMarginX, layout.listStartY + i * (layout.itemH + layout.itemGap));
        card.cardSize = glm::vec2(cardW, layout.itemH);

        float actY = card.cardPos.y + (layout.itemH - btnActH) * 0.5f;

        if (card.hasDel) {
            card.btnDelSize = glm::vec2(btnDelW, btnActH);
            card.btnDelPos = glm::vec2(card.cardPos.x + card.cardSize.x - rightMargin - btnDelW, actY);

            card.btnRenSize = glm::vec2(btnRenW, btnActH);
            card.btnRenPos = glm::vec2(card.btnDelPos.x - btnActGap - btnRenW, actY);

            card.btnLoadSize = glm::vec2(btnLoadW, btnActH);
            card.btnLoadPos = glm::vec2(card.btnRenPos.x - btnActGap - btnLoadW, actY);
        } else {
            card.btnDelSize = glm::vec2(0.0f);
            card.btnDelPos = glm::vec2(-1000.0f);

            card.btnRenSize = glm::vec2(btnRenW, btnActH);
            card.btnRenPos = glm::vec2(card.cardPos.x + card.cardSize.x - rightMargin - btnDelW - btnActGap - btnRenW, actY);

            card.btnLoadSize = glm::vec2(btnLoadW, btnActH);
            card.btnLoadPos = glm::vec2(card.btnRenPos.x - btnActGap - btnLoadW, actY);
        }

        layout.visibleCards.push_back(card);
    }

    return layout;
}

bool EditorMapsBrowserModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt,
                                         int screenWidth, int screenHeight,
                                         const std::function<void(const std::string&)>& onLoadLevel,
                                         const std::function<void()>& onNewLevel,
                                         const std::function<void(const std::string&, const std::string&, const std::string&)>& onRenameLevel,
                                         const std::function<void()>& onReturnToOrigin) {
    if (!isOpen()) return false;

    // 1. Модальное окно переименования
    if (m_isRenameModalOpen) {
        RenameModalLayout l = getRenameModalLayout(screenWidth, screenHeight);

        if (checkKeyRepeat(window, GLFW_KEY_ESCAPE, dt)) {
            m_isRenameModalOpen = false;
            m_wasLeftDown = leftDown;
            return true;
        }
        if (checkKeyRepeat(window, GLFW_KEY_ENTER, dt) || checkKeyRepeat(window, GLFW_KEY_KP_ENTER, dt)) {
            confirmRename(onRenameLevel);
            m_wasLeftDown = leftDown;
            return true;
        }
        if (checkKeyRepeat(window, GLFW_KEY_BACKSPACE, dt)) {
            InputManager::popUtf8(m_renameInputText);
            m_cursorBlinkTimer = 0.0f;
            m_wasLeftDown = leftDown;
            return true;
        }

        std::string typed = InputManager::getFrameText();
        if (!typed.empty() && m_renameInputText.length() < 60) {
            m_renameInputText += typed;
            m_cursorBlinkTimer = 0.0f;
        }

        if (leftDown && !m_wasLeftDown) {
            if (isPointInRect(mousePos, l.btnSavePos, l.btnSaveSize)) {
                confirmRename(onRenameLevel);
                m_wasLeftDown = leftDown;
                return true;
            }
            if (isPointInRect(mousePos, l.btnCancelPos, l.btnCancelSize)) {
                m_isRenameModalOpen = false;
                m_wasLeftDown = leftDown;
                return true;
            }
            if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
                m_isRenameModalOpen = false;
                m_wasLeftDown = leftDown;
                return true;
            }
            if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
                m_isRenameModalOpen = false;
                m_wasLeftDown = leftDown;
                return true;
            }
        }

        m_wasLeftDown = leftDown;
        return true;
    }

    // 2. Модальное окно списка карт
    if (m_isOpen) {
        MapsModalLayout l = getMapsModalLayout(screenWidth, screenHeight);

        if (checkKeyRepeat(window, GLFW_KEY_ESCAPE, dt)) {
            m_isOpen = false;
            if (m_isInitialModalLaunch && onReturnToOrigin) {
                onReturnToOrigin();
            }
            m_wasLeftDown = leftDown;
            return true;
        }

        if (l.hasPagination) {
            if (checkKeyRepeat(window, GLFW_KEY_UP, dt)) {
                if (m_mapsScrollOffset > 0) m_mapsScrollOffset--;
                m_wasLeftDown = leftDown;
                return true;
            }
            if (checkKeyRepeat(window, GLFW_KEY_DOWN, dt)) {
                if (m_mapsScrollOffset < l.maxOffset) m_mapsScrollOffset++;
                m_wasLeftDown = leftDown;
                return true;
            }
            if (checkKeyRepeat(window, GLFW_KEY_PAGE_UP, dt)) {
                m_mapsScrollOffset = std::max(0, m_mapsScrollOffset - l.maxVisible);
                m_wasLeftDown = leftDown;
                return true;
            }
            if (checkKeyRepeat(window, GLFW_KEY_PAGE_DOWN, dt)) {
                m_mapsScrollOffset = std::min(l.maxOffset, m_mapsScrollOffset + l.maxVisible);
                m_wasLeftDown = leftDown;
                return true;
            }
        }

        if (leftDown && !m_wasLeftDown) {
            if (isPointInRect(mousePos, l.btnNewPos, l.btnNewSize)) {
                m_isOpen = false;
                m_isInitialModalLaunch = false;
                if (onNewLevel) onNewLevel();
                m_wasLeftDown = leftDown;
                return true;
            }

            if (isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize) ||
                isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
                m_isOpen = false;
                if (m_isInitialModalLaunch && onReturnToOrigin) {
                    onReturnToOrigin();
                }
                m_wasLeftDown = leftDown;
                return true;
            }

            if (l.hasPagination) {
                if (isPointInRect(mousePos, l.btnScrollUpPos, l.btnScrollUpSize)) {
                    if (m_mapsScrollOffset > 0) m_mapsScrollOffset--;
                    m_wasLeftDown = leftDown;
                    return true;
                }
                if (isPointInRect(mousePos, l.btnScrollDownPos, l.btnScrollDownSize)) {
                    if (m_mapsScrollOffset < l.maxOffset) m_mapsScrollOffset++;
                    m_wasLeftDown = leftDown;
                    return true;
                }
                if (isPointInRect(mousePos, l.btnPrevPagePos, l.btnPrevPageSize)) {
                    m_mapsScrollOffset = std::max(0, m_mapsScrollOffset - l.maxVisible);
                    m_wasLeftDown = leftDown;
                    return true;
                }
                if (isPointInRect(mousePos, l.btnNextPagePos, l.btnNextPageSize)) {
                    m_mapsScrollOffset = std::min(l.maxOffset, m_mapsScrollOffset + l.maxVisible);
                    m_wasLeftDown = leftDown;
                    return true;
                }
            }

            for (const auto& card : l.visibleCards) {
                const auto& lvl = card.levelInfo;

                if (isPointInRect(mousePos, card.btnLoadPos, card.btnLoadSize)) {
                    m_isOpen = false;
                    m_isInitialModalLaunch = false;
                    m_currentLevelFileName = lvl.filename;
                    if (onLoadLevel) onLoadLevel(lvl.filename);
                    m_wasLeftDown = leftDown;
                    return true;
                }

                if (isPointInRect(mousePos, card.btnRenPos, card.btnRenSize)) {
                    openRename(lvl.filename, lvl.name);
                    m_wasLeftDown = leftDown;
                    return true;
                }

                if (card.hasDel && isPointInRect(mousePos, card.btnDelPos, card.btnDelSize)) {
                    LevelManager::deleteLevel(lvl.filename);
                    if (lvl.filename == m_currentLevelFileName) {
                        m_currentLevelFileName = "level_editor.json";
                        if (onLoadLevel) onLoadLevel("level_editor.json");
                    }
                    int newTotal = static_cast<int>(getFilteredModalLevels().size());
                    int newMaxOffset = std::max(0, newTotal - l.maxVisible);
                    m_mapsScrollOffset = std::clamp(m_mapsScrollOffset, 0, newMaxOffset);
                    m_wasLeftDown = leftDown;
                    return true;
                }
            }

            if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
                m_isOpen = false;
                if (m_isInitialModalLaunch && onReturnToOrigin) {
                    onReturnToOrigin();
                }
                m_wasLeftDown = leftDown;
                return true;
            }
        }

        m_wasLeftDown = leftDown;
        return true;
    }

    m_wasLeftDown = leftDown;
    return false;
}

bool EditorMapsBrowserModal::handleInput(float mouseX, float mouseY, bool mousePressed,
                                         LevelMapData& currentMap,
                                         std::function<void(const std::string&)> onLoadMap,
                                         std::function<void()> onNewMap) {
    return handleInput(nullptr, glm::vec2(mouseX, mouseY), mousePressed, 0.016f, 1280, 720,
                       onLoadMap, onNewMap, nullptr, nullptr);
}

void EditorMapsBrowserModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                    const Texture2D* whiteTexture,
                                    int screenWidth, int screenHeight, glm::vec2 mousePos) {
    if (m_isOpen) {
        renderMapsModal(renderer, textRenderer, whiteTexture, screenWidth, screenHeight, mousePos);
    }
    if (m_isRenameModalOpen) {
        renderRenameModal(renderer, textRenderer, whiteTexture, screenWidth, screenHeight, mousePos);
    }
}

void EditorMapsBrowserModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                    float screenWidth, float screenHeight) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    render(renderer, textRenderer, whiteTex, static_cast<int>(screenWidth), static_cast<int>(screenHeight), glm::vec2(0.0f));
}

void EditorMapsBrowserModal::renderRenameModal(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                               const Texture2D* whiteTexture,
                                               int screenWidth, int screenHeight, glm::vec2 mousePos) {
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(0.0f), glm::vec2(screenWidth, screenHeight), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.78f));

    RenameModalLayout l = getRenameModalLayout(screenWidth, screenHeight);

    // Modal panel & border
    renderer->drawSprite(whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    // Header
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    // Close cross [X]
    bool hovCross = isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    renderer->drawSprite(whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f, hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.35f, 0.38f, 0.45f));
    renderer->drawSprite(whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f, hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.22f, 0.28f));

    // Input box
    renderer->drawSprite(whiteTexture, l.boxPos, l.boxSize, 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    renderer->drawSprite(whiteTexture, l.boxPos + glm::vec2(1.0f), l.boxSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.08f, 0.10f, 0.14f));

    // Save button
    bool hovSave = isPointInRect(mousePos, l.btnSavePos, l.btnSaveSize);
    renderer->drawSprite(whiteTexture, l.btnSavePos, l.btnSaveSize, 0.0f, hovSave ? glm::vec3(0.3f, 0.9f, 0.45f) : glm::vec3(0.2f, 0.7f, 0.35f));
    renderer->drawSprite(whiteTexture, l.btnSavePos + glm::vec2(1.0f), l.btnSaveSize - glm::vec2(2.0f), 0.0f, hovSave ? glm::vec3(0.18f, 0.38f, 0.22f) : glm::vec3(0.14f, 0.30f, 0.18f));

    // Cancel button
    bool hovCancel = isPointInRect(mousePos, l.btnCancelPos, l.btnCancelSize);
    renderer->drawSprite(whiteTexture, l.btnCancelPos, l.btnCancelSize, 0.0f, hovCancel ? glm::vec3(0.6f, 0.25f, 0.25f) : glm::vec3(0.45f, 0.20f, 0.20f));
    renderer->drawSprite(whiteTexture, l.btnCancelPos + glm::vec2(1.0f), l.btnCancelSize - glm::vec2(2.0f), 0.0f, hovCancel ? glm::vec3(0.28f, 0.14f, 0.14f) : glm::vec3(0.22f, 0.11f, 0.11f));

    renderer->flush();

    if (textRenderer) {
        // Title
        std::string titleStr = LOC("RENAME_TITLE");
        float titleW = textRenderer->CalculateTextWidth(titleStr, l.fTitle);
        float titleX = l.modalPos.x + (l.modalSize.x - titleW) * 0.5f;
        float titleY = l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(titleStr, titleX, titleY, l.fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

        // Cross text
        float crossW = textRenderer->CalculateTextWidth("x", l.fCross);
        textRenderer->RenderText("x", l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f,
                                  l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 2.0f,
                                  l.fCross, glm::vec3(0.9f));

        // Subtitle
        std::string sub = "File: " + m_renameTargetFileName;
        textRenderer->RenderText(sub, l.boxPos.x + 2.0f, l.subY, l.fSub, glm::vec3(0.70f, 0.75f, 0.85f));

        // Input text
        bool showCursor = (m_cursorBlinkTimer < 0.5f);
        std::string displayText = m_renameInputText + (showCursor ? "|" : "");
        float inputY = l.boxPos.y + (l.boxSize.y - l.fInput * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(displayText, l.boxPos.x + 10.0f, inputY, l.fInput, glm::vec3(0.40f, 0.95f, 1.0f));

        // Save button text
        std::string saveStr = LOC("RENAME_SAVE") + " (Enter)";
        float sTxtW = textRenderer->CalculateTextWidth(saveStr, l.fBtn);
        float sTxtX = l.btnSavePos.x + (l.btnSaveSize.x - sTxtW) * 0.5f;
        float sTxtY = l.btnSavePos.y + (l.btnSaveSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(saveStr, sTxtX, sTxtY, l.fBtn, glm::vec3(0.95f));

        // Cancel button text
        std::string cancelStr = LOC("RENAME_CANCEL") + " (Esc)";
        float cTxtW = textRenderer->CalculateTextWidth(cancelStr, l.fBtn);
        float cTxtX = l.btnCancelPos.x + (l.btnCancelSize.x - cTxtW) * 0.5f;
        float cTxtY = l.btnCancelPos.y + (l.btnCancelSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(cancelStr, cTxtX, cTxtY, l.fBtn, glm::vec3(0.95f));
    }
}

void EditorMapsBrowserModal::renderMapsModal(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                             const Texture2D* whiteTexture,
                                             int screenWidth, int screenHeight, glm::vec2 mousePos) {
    MapsModalLayout l = getMapsModalLayout(screenWidth, screenHeight);

    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(0.0f), glm::vec2(screenWidth, screenHeight), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.78f));

    // Modal background & borders
    renderer->drawSprite(whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));

    // Header
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));

    // Close cross [X] in header
    bool hovCross = isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    renderer->drawSprite(whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f, hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.35f, 0.38f, 0.45f));
    renderer->drawSprite(whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f, hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.22f, 0.28f));

    // "+ New Map" button
    bool hovNew = isPointInRect(mousePos, l.btnNewPos, l.btnNewSize);
    renderer->drawSprite(whiteTexture, l.btnNewPos, l.btnNewSize, 0.0f, hovNew ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.80f, 0.40f));
    renderer->drawSprite(whiteTexture, l.btnNewPos + glm::vec2(1.0f), l.btnNewSize - glm::vec2(2.0f), 0.0f, hovNew ? glm::vec3(0.16f, 0.36f, 0.20f) : glm::vec3(0.12f, 0.28f, 0.16f));

    // Quick scroll buttons in subheader if pagination
    if (l.hasPagination) {
        bool canUp = (m_mapsScrollOffset > 0);
        bool canDown = (m_mapsScrollOffset < l.maxOffset);
        bool hovUp = isPointInRect(mousePos, l.btnScrollUpPos, l.btnScrollUpSize);
        bool hovDown = isPointInRect(mousePos, l.btnScrollDownPos, l.btnScrollDownSize);

        glm::vec3 upBorder = canUp ? (hovUp ? glm::vec3(0.50f, 0.85f, 1.0f) : glm::vec3(0.35f, 0.55f, 0.75f)) : glm::vec3(0.22f, 0.25f, 0.30f);
        glm::vec3 upBg = canUp ? (hovUp ? glm::vec3(0.22f, 0.32f, 0.44f) : glm::vec3(0.16f, 0.22f, 0.30f)) : glm::vec3(0.12f, 0.14f, 0.18f);
        renderer->drawSprite(whiteTexture, l.btnScrollUpPos, l.btnScrollUpSize, 0.0f, upBorder);
        renderer->drawSprite(whiteTexture, l.btnScrollUpPos + glm::vec2(1.0f), l.btnScrollUpSize - glm::vec2(2.0f), 0.0f, upBg);

        glm::vec3 downBorder = canDown ? (hovDown ? glm::vec3(0.50f, 0.85f, 1.0f) : glm::vec3(0.35f, 0.55f, 0.75f)) : glm::vec3(0.22f, 0.25f, 0.30f);
        glm::vec3 downBg = canDown ? (hovDown ? glm::vec3(0.22f, 0.32f, 0.44f) : glm::vec3(0.16f, 0.22f, 0.30f)) : glm::vec3(0.12f, 0.14f, 0.18f);
        renderer->drawSprite(whiteTexture, l.btnScrollDownPos, l.btnScrollDownSize, 0.0f, downBorder);
        renderer->drawSprite(whiteTexture, l.btnScrollDownPos + glm::vec2(1.0f), l.btnScrollDownSize - glm::vec2(2.0f), 0.0f, downBg);
    }

    // Render cards
    for (const auto& card : l.visibleCards) {
        const auto& lvl = card.levelInfo;

        bool isCurrent = (lvl.filename == m_currentLevelFileName);
        glm::vec3 cardBg = isCurrent ? glm::vec3(0.18f, 0.24f, 0.35f) : glm::vec3(0.15f, 0.16f, 0.21f);
        glm::vec3 cardBorder = isCurrent ? glm::vec3(0.40f, 0.85f, 1.0f) : glm::vec3(0.28f, 0.30f, 0.38f);

        renderer->drawSprite(whiteTexture, card.cardPos, card.cardSize, 0.0f, cardBorder);
        renderer->drawSprite(whiteTexture, card.cardPos + glm::vec2(1.0f), card.cardSize - glm::vec2(2.0f), 0.0f, cardBg);

        // [Load] button
        bool hovLoad = isPointInRect(mousePos, card.btnLoadPos, card.btnLoadSize);
        renderer->drawSprite(whiteTexture, card.btnLoadPos, card.btnLoadSize, 0.0f, hovLoad ? glm::vec3(0.40f, 0.85f, 1.0f) : glm::vec3(0.25f, 0.55f, 0.80f));
        renderer->drawSprite(whiteTexture, card.btnLoadPos + glm::vec2(1.0f), card.btnLoadSize - glm::vec2(2.0f), 0.0f, hovLoad ? glm::vec3(0.16f, 0.30f, 0.45f) : glm::vec3(0.12f, 0.22f, 0.35f));

        // [Rename] button
        bool hovRen = isPointInRect(mousePos, card.btnRenPos, card.btnRenSize);
        renderer->drawSprite(whiteTexture, card.btnRenPos, card.btnRenSize, 0.0f, hovRen ? glm::vec3(1.0f, 0.85f, 0.35f) : glm::vec3(0.75f, 0.65f, 0.25f));
        renderer->drawSprite(whiteTexture, card.btnRenPos + glm::vec2(1.0f), card.btnRenSize - glm::vec2(2.0f), 0.0f, hovRen ? glm::vec3(0.38f, 0.30f, 0.14f) : glm::vec3(0.28f, 0.22f, 0.10f));

        // [Del] button
        if (card.hasDel) {
            bool hovDel = isPointInRect(mousePos, card.btnDelPos, card.btnDelSize);
            renderer->drawSprite(whiteTexture, card.btnDelPos, card.btnDelSize, 0.0f, hovDel ? glm::vec3(0.95f, 0.30f, 0.30f) : glm::vec3(0.70f, 0.20f, 0.20f));
            renderer->drawSprite(whiteTexture, card.btnDelPos + glm::vec2(1.0f), card.btnDelSize - glm::vec2(2.0f), 0.0f, hovDel ? glm::vec3(0.40f, 0.15f, 0.15f) : glm::vec3(0.28f, 0.10f, 0.10f));
        }
    }

    // Scrollbar track and thumb on right side
    if (l.hasPagination && !l.visibleCards.empty()) {
        float trackX = l.modalPos.x + l.modalSize.x - std::clamp(16.0f * GetUIScale(screenWidth, screenHeight), 12.0f, 20.0f);
        float trackY = l.listStartY;
        float trackH = static_cast<float>(l.visibleCards.size()) * (l.itemH + l.itemGap) - l.itemGap;
        float trackW = std::clamp(6.0f * GetUIScale(screenWidth, screenHeight), 4.0f, 8.0f);

        renderer->drawSprite(whiteTexture, glm::vec2(trackX, trackY), glm::vec2(trackW, trackH), 0.0f, glm::vec3(0.18f, 0.20f, 0.25f));

        float thumbH = std::max(20.0f, trackH * (static_cast<float>(l.maxVisible) / static_cast<float>(l.totalLevels)));
        float thumbRatio = (l.maxOffset > 0) ? (static_cast<float>(m_mapsScrollOffset) / static_cast<float>(l.maxOffset)) : 0.0f;
        float thumbY = trackY + (trackH - thumbH) * thumbRatio;
        renderer->drawSprite(whiteTexture, glm::vec2(trackX, thumbY), glm::vec2(trackW, thumbH), 0.0f, glm::vec3(0.35f, 0.70f, 1.0f));
    }

    // Bottom [Close] button
    bool hovClose = isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize);
    renderer->drawSprite(whiteTexture, l.btnClosePos, l.btnCloseSize, 0.0f, hovClose ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.35f, 0.38f, 0.45f));
    renderer->drawSprite(whiteTexture, l.btnClosePos + glm::vec2(1.0f), l.btnCloseSize - glm::vec2(2.0f), 0.0f, hovClose ? glm::vec3(0.22f, 0.25f, 0.30f) : glm::vec3(0.16f, 0.18f, 0.22f));

    // Bottom Prev / Next buttons if pagination
    if (l.hasPagination) {
        bool canPrev = (m_mapsScrollOffset > 0);
        bool canNext = (m_mapsScrollOffset < l.maxOffset);
        bool hovPrev = isPointInRect(mousePos, l.btnPrevPagePos, l.btnPrevPageSize);
        bool hovNext = isPointInRect(mousePos, l.btnNextPagePos, l.btnNextPageSize);

        glm::vec3 prevBorder = canPrev ? (hovPrev ? glm::vec3(0.40f, 0.80f, 1.0f) : glm::vec3(0.30f, 0.55f, 0.85f)) : glm::vec3(0.24f, 0.26f, 0.32f);
        glm::vec3 prevBg = canPrev ? (hovPrev ? glm::vec3(0.20f, 0.32f, 0.46f) : glm::vec3(0.14f, 0.22f, 0.32f)) : glm::vec3(0.12f, 0.13f, 0.16f);
        renderer->drawSprite(whiteTexture, l.btnPrevPagePos, l.btnPrevPageSize, 0.0f, prevBorder);
        renderer->drawSprite(whiteTexture, l.btnPrevPagePos + glm::vec2(1.0f), l.btnPrevPageSize - glm::vec2(2.0f), 0.0f, prevBg);

        glm::vec3 nextBorder = canNext ? (hovNext ? glm::vec3(0.40f, 0.80f, 1.0f) : glm::vec3(0.30f, 0.55f, 0.85f)) : glm::vec3(0.24f, 0.26f, 0.32f);
        glm::vec3 nextBg = canNext ? (hovNext ? glm::vec3(0.20f, 0.32f, 0.46f) : glm::vec3(0.14f, 0.22f, 0.32f)) : glm::vec3(0.12f, 0.13f, 0.16f);
        renderer->drawSprite(whiteTexture, l.btnNextPagePos, l.btnNextPageSize, 0.0f, nextBorder);
        renderer->drawSprite(whiteTexture, l.btnNextPagePos + glm::vec2(1.0f), l.btnNextPageSize - glm::vec2(2.0f), 0.0f, nextBg);
    }

    renderer->flush();

    if (textRenderer) {
        // Title
        std::string titleStr = "LEVELS LIST";
        float titleW = textRenderer->CalculateTextWidth(titleStr, l.fTitle);
        float titleX = l.modalPos.x + (l.modalSize.x - titleW) * 0.5f;
        float titleY = l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(titleStr, titleX, titleY, l.fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

        // Cross text
        float crossW = textRenderer->CalculateTextWidth("x", l.fCross);
        textRenderer->RenderText("x", l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f,
                                  l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 2.0f,
                                  l.fCross, glm::vec3(0.9f));

        // Subtitle
        float subY = l.modalPos.y + l.headerH + std::clamp(10.0f * GetUIScale(screenWidth, screenHeight), 8.0f, 16.0f);
        textRenderer->RenderText("Select, rename, or create maps:", l.modalPos.x + 22.0f, subY + 4.0f, l.fSub, glm::vec3(0.70f, 0.75f, 0.85f));

        // "+ New Map" text
        float newTxtW = textRenderer->CalculateTextWidth("+ New Map", l.fNew);
        float newTxtX = l.btnNewPos.x + (l.btnNewSize.x - newTxtW) * 0.5f;
        float newTxtY = l.btnNewPos.y + (l.btnNewSize.y - l.fNew * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText("+ New Map", newTxtX, newTxtY, l.fNew, glm::vec3(0.95f));

        // Subheader scroll arrows [^] and [v]
        if (l.hasPagination) {
            float upW = textRenderer->CalculateTextWidth("^", l.fNew);
            textRenderer->RenderText("^", l.btnScrollUpPos.x + (l.btnScrollUpSize.x - upW) * 0.5f,
                                      l.btnScrollUpPos.y + (l.btnScrollUpSize.y - l.fNew * 28.0f) * 0.5f + 2.0f,
                                      l.fNew, (m_mapsScrollOffset > 0) ? glm::vec3(0.95f) : glm::vec3(0.45f));

            float downW = textRenderer->CalculateTextWidth("v", l.fNew);
            textRenderer->RenderText("v", l.btnScrollDownPos.x + (l.btnScrollDownSize.x - downW) * 0.5f,
                                      l.btnScrollDownPos.y + (l.btnScrollDownSize.y - l.fNew * 28.0f) * 0.5f + 2.0f,
                                      l.fNew, (m_mapsScrollOffset < l.maxOffset) ? glm::vec3(0.95f) : glm::vec3(0.45f));
        }

        // Render card text
        for (const auto& card : l.visibleCards) {
            const auto& lvl = card.levelInfo;

            bool isCurrent = (lvl.filename == m_currentLevelFileName);
            std::string label = lvl.name;
            if (isCurrent) label += "  [ACTIVE]";
            glm::vec3 nameCol = isCurrent ? glm::vec3(0.40f, 1.0f, 0.60f) : (lvl.isBuiltIn ? glm::vec3(1.0f, 0.85f, 0.3f) : glm::vec3(0.92f, 0.94f, 0.98f));

            float textLeftX = card.cardPos.x + std::clamp(14.0f * GetUIScale(screenWidth, screenHeight), 10.0f, 18.0f);
            float totalH = (l.fCardName + l.fCardSub) * 28.0f + 2.0f;
            float nameY = card.cardPos.y + (card.cardSize.y - totalH) * 0.5f + 1.0f;
            float subCardY = nameY + l.fCardName * 28.0f + 1.0f;

            // Ensure name doesn't overlap action buttons
            float maxLabelW = card.btnLoadPos.x - textLeftX - 10.0f;
            std::string displayLabel = label;
            if (textRenderer->CalculateTextWidth(displayLabel, l.fCardName) > maxLabelW) {
                while (!displayLabel.empty() && textRenderer->CalculateTextWidth(displayLabel + "...", l.fCardName) > maxLabelW) {
                    InputManager::popUtf8(displayLabel);
                }
                displayLabel += "...";
            }
            textRenderer->RenderText(displayLabel, textLeftX, nameY, l.fCardName, nameCol);

            std::string sub = lvl.filename + (lvl.isBuiltIn ? " (Campaign)" : " (Custom)");
            textRenderer->RenderText(sub, textLeftX, subCardY, l.fCardSub, glm::vec3(0.60f, 0.65f, 0.75f));

            // [Load] text
            float loadW = textRenderer->CalculateTextWidth("Load", l.fActionBtn);
            float loadX = card.btnLoadPos.x + (card.btnLoadSize.x - loadW) * 0.5f;
            float loadY = card.btnLoadPos.y + (card.btnLoadSize.y - l.fActionBtn * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText("Load", loadX, loadY, l.fActionBtn, glm::vec3(0.95f));

            // [Rename] text
            float renW = textRenderer->CalculateTextWidth("Rename", l.fActionBtn);
            float renX = card.btnRenPos.x + (card.btnRenSize.x - renW) * 0.5f;
            float renY = card.btnRenPos.y + (card.btnRenSize.y - l.fActionBtn * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText("Rename", renX, renY, l.fActionBtn, glm::vec3(0.95f));

            // [Del] text
            if (card.hasDel) {
                float delW = textRenderer->CalculateTextWidth("Del", l.fActionBtn);
                float delX = card.btnDelPos.x + (card.btnDelSize.x - delW) * 0.5f;
                float delY = card.btnDelPos.y + (card.btnDelSize.y - l.fActionBtn * 28.0f) * 0.5f + 2.0f;
                textRenderer->RenderText("Del", delX, delY, l.fActionBtn, glm::vec3(0.95f));
            }
        }

        // [Close] text
        float closeW = textRenderer->CalculateTextWidth("Close", l.fClose);
        float closeX = l.btnClosePos.x + (l.btnCloseSize.x - closeW) * 0.5f;
        float closeY = l.btnClosePos.y + (l.btnCloseSize.y - l.fClose * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText("Close", closeX, closeY, l.fClose, glm::vec3(0.95f));

        // [ < Prev ] and [ Next > ] texts
        if (l.hasPagination) {
            float pW = textRenderer->CalculateTextWidth("< Prev", l.fClose);
            float pX = l.btnPrevPagePos.x + (l.btnPrevPageSize.x - pW) * 0.5f;
            float pY = l.btnPrevPagePos.y + (l.btnPrevPageSize.y - l.fClose * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText("< Prev", pX, pY, l.fClose, (m_mapsScrollOffset > 0) ? glm::vec3(0.95f) : glm::vec3(0.45f));

            float nW = textRenderer->CalculateTextWidth("Next >", l.fClose);
            float nX = l.btnNextPagePos.x + (l.btnNextPageSize.x - nW) * 0.5f;
            float nY = l.btnNextPagePos.y + (l.btnNextPageSize.y - l.fClose * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText("Next >", nX, nY, l.fClose, (m_mapsScrollOffset < l.maxOffset) ? glm::vec3(0.95f) : glm::vec3(0.45f));

            // Page/item count text on the left
            int startItem = l.currentOffset + 1;
            int endItem = std::min(l.totalLevels, l.currentOffset + l.maxVisible);
            std::string countStr = std::to_string(startItem) + "-" + std::to_string(endItem) + " / " + std::to_string(l.totalLevels);
            float cFont = std::clamp(0.44f * GetUIScale(screenWidth, screenHeight), 0.32f, 0.56f);
            float cY = l.btnClosePos.y + (l.btnCloseSize.y - cFont * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText(countStr, l.modalPos.x + std::clamp(20.0f * GetUIScale(screenWidth, screenHeight), 14.0f, 26.0f), cY, cFont, glm::vec3(0.65f, 0.70f, 0.80f));
        }
    }
}

