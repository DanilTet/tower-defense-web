#include "EditorExitModal.h"
#include "../ui/UICommon.h"
#include "../core/LocalizationManager.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../../resources/ResourceManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

bool EditorExitModal::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void EditorExitModal::open(bool isDirty) {
    if (!isDirty) {
        m_isOpen = false;
        return;
    }
    m_isOpen = true;
    m_suppressClickUntilRelease = true;
    m_escReleased = false;
    std::cout << "[EditorExitModal] open: exit confirmation dialog opened" << std::endl;
}

void EditorExitModal::close() {
    m_isOpen = false;
    m_suppressClickUntilRelease = false;
    m_escReleased = false;
    std::cout << "[EditorExitModal] close: exit dialog closed" << std::endl;
}

ExitModalLayout EditorExitModal::layoutExitModal(float screenW, float screenH, TextRenderer* textRenderer) const {
    ExitModalLayout layout;
    float scale = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));

    layout.fTitle = std::clamp(0.68f * scale, 0.48f, 0.88f);
    layout.fQuestion = std::clamp(0.66f * scale, 0.46f, 0.84f);
    layout.fSub = std::clamp(0.48f * scale, 0.35f, 0.64f);
    layout.fBtn = std::clamp(0.46f * scale, 0.34f, 0.60f);
    layout.fCross = std::clamp(0.55f * scale, 0.40f, 0.75f);

    std::string saveExitStr = LOC("EDITOR_EXIT_SAVE_AND_EXIT");
    std::string discardStr = LOC("EDITOR_EXIT_DISCARD");
    std::string cancelStr = LOC("EDITOR_EXIT_CANCEL");

    float w1 = textRenderer ? textRenderer->CalculateTextWidth(saveExitStr, layout.fBtn) : 130.0f;
    float w2 = textRenderer ? textRenderer->CalculateTextWidth(discardStr, layout.fBtn) : 140.0f;
    float w3 = textRenderer ? textRenderer->CalculateTextWidth(cancelStr, layout.fBtn) : 60.0f;

    float btnPad = std::clamp(14.0f * scale, 8.0f, 20.0f);
    float req1 = w1 + btnPad * 2.0f;
    float req2 = w2 + btnPad * 2.0f;
    float req3 = w3 + btnPad * 2.0f;
    float totalReq = req1 + req2 + req3;

    float sideMargin = std::clamp(18.0f * scale, 12.0f, 26.0f);
    float btnGap = std::clamp(12.0f * scale, 8.0f, 16.0f);

    float desiredW = std::max(540.0f * scale, totalReq + 2.0f * sideMargin + 2.0f * btnGap);
    layout.modalSize.x = std::clamp(desiredW, 300.0f, std::max(300.0f, screenW - 40.0f));
    layout.modalSize.y = std::clamp(220.0f * scale, 150.0f, std::max(150.0f, screenH - 40.0f));

    layout.modalPos = glm::vec2((screenW - layout.modalSize.x) * 0.5f,
                                (screenH - layout.modalSize.y) * 0.5f);

    layout.headerH = std::clamp(40.0f * scale, 30.0f, 56.0f);

    float crossSize = std::clamp(26.0f * scale, 20.0f, 36.0f);
    layout.btnCloseCrossSize = glm::vec2(crossSize, crossSize);
    layout.btnCloseCrossPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - crossSize - std::clamp(8.0f * scale, 6.0f, 12.0f),
                                        layout.modalPos.y + (layout.headerH - crossSize) * 0.5f);

    float btnH = std::clamp(42.0f * scale, 32.0f, 54.0f);
    float btnMarginBottom = std::clamp(18.0f * scale, 12.0f, 24.0f);
    float btnY = layout.modalPos.y + layout.modalSize.y - btnH - btnMarginBottom;

    float availW = layout.modalSize.x - 2.0f * sideMargin - 2.0f * btnGap;
    float btn1W, btn2W, btn3W;
    if (availW >= totalReq) {
        float extra = availW - totalReq;
        btn1W = req1 + extra * (req1 / totalReq);
        btn2W = req2 + extra * (req2 / totalReq);
        btn3W = req3 + extra * (req3 / totalReq);
    } else {
        float ratio = availW / std::max(1.0f, totalReq);
        btn1W = req1 * ratio;
        btn2W = req2 * ratio;
        btn3W = req3 * ratio;
    }

    layout.btnSaveExitPos = glm::vec2(layout.modalPos.x + sideMargin, btnY);
    layout.btnSaveExitSize = glm::vec2(btn1W, btnH);

    layout.btnDiscardPos = glm::vec2(layout.btnSaveExitPos.x + btn1W + btnGap, btnY);
    layout.btnDiscardSize = glm::vec2(btn2W, btnH);

    layout.btnCancelPos = glm::vec2(layout.btnDiscardPos.x + btn2W + btnGap, btnY);
    layout.btnCancelSize = glm::vec2(btn3W, btnH);

    float contentTop = layout.modalPos.y + layout.headerH;
    float contentH = btnY - contentTop;
    layout.questionY = contentTop + contentH * 0.28f;
    layout.subY = contentTop + contentH * 0.62f;

    return layout;
}

ExitModalAction EditorExitModal::handleInput(glm::vec2 mousePos, bool mouseClicked, int key, bool keyReleased) {
    if (!m_isOpen) return ExitModalAction::None;
    m_mousePos = mousePos;

    if (m_suppressClickUntilRelease) {
        if (!mouseClicked) {
            m_suppressClickUntilRelease = false;
        }
    }

    if (key == GLFW_KEY_ESCAPE) {
        if (keyReleased) {
            m_escReleased = true;
        } else if (m_escReleased) {
            close();
            return ExitModalAction::DiscardAndExit;
        }
    } else if (!keyReleased && (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)) {
        close();
        return ExitModalAction::SaveAndExit;
    }

    if (!m_suppressClickUntilRelease && mouseClicked) {
        ExitModalLayout l = layoutExitModal(m_lastScreenW, m_lastScreenH, nullptr);

        if (isPointInRect(mousePos, l.btnSaveExitPos, l.btnSaveExitSize)) {
            close();
            return ExitModalAction::SaveAndExit;
        }
        if (isPointInRect(mousePos, l.btnDiscardPos, l.btnDiscardSize)) {
            close();
            return ExitModalAction::DiscardAndExit;
        }
        if (isPointInRect(mousePos, l.btnCancelPos, l.btnCancelSize)) {
            close();
            return ExitModalAction::Cancel;
        }
        if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
            close();
            return ExitModalAction::Cancel;
        }
        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            close();
            return ExitModalAction::Cancel;
        }
    }

    return ExitModalAction::None;
}

void EditorExitModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenW, float screenH) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    render(renderer, textRenderer, whiteTex, screenW, screenH, m_mousePos);
}

void EditorExitModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                             const Texture2D* whiteTexture,
                             float screenW, float screenH, glm::vec2 mousePos) {
    if (!m_isOpen || !renderer || !whiteTexture) return;

    m_lastScreenW = screenW;
    m_lastScreenH = screenH;
    m_mousePos = mousePos;

    // Полупрозрачный фон-затемнение
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(0.0f), glm::vec2(screenW, screenH), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.78f));

    ExitModalLayout l = layoutExitModal(screenW, screenH, textRenderer);

    // Основная подложка и 2px обводка
    renderer->drawSprite(whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.18f));
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));

    // Шапка
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.25f));
    renderer->drawSprite(whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));

    // Кнопка-крестик [X]
    bool hovCross = isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    renderer->drawSprite(whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f, hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.35f, 0.38f, 0.45f));
    renderer->drawSprite(whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f, hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.22f, 0.28f));

    // Кнопки действий
    // 1. [ Сохранить и выйти ]
    bool hovSaveExit = isPointInRect(mousePos, l.btnSaveExitPos, l.btnSaveExitSize);
    renderer->drawSprite(whiteTexture, l.btnSaveExitPos, l.btnSaveExitSize, 0.0f, hovSaveExit ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.75f, 0.38f));
    renderer->drawSprite(whiteTexture, l.btnSaveExitPos + glm::vec2(2.0f), l.btnSaveExitSize - glm::vec2(4.0f), 0.0f, hovSaveExit ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.30f, 0.18f));

    // 2. [ Выйти без сохранения ]
    bool hovDiscard = isPointInRect(mousePos, l.btnDiscardPos, l.btnDiscardSize);
    renderer->drawSprite(whiteTexture, l.btnDiscardPos, l.btnDiscardSize, 0.0f, hovDiscard ? glm::vec3(0.95f, 0.35f, 0.35f) : glm::vec3(0.75f, 0.25f, 0.25f));
    renderer->drawSprite(whiteTexture, l.btnDiscardPos + glm::vec2(2.0f), l.btnDiscardSize - glm::vec2(4.0f), 0.0f, hovDiscard ? glm::vec3(0.40f, 0.16f, 0.16f) : glm::vec3(0.28f, 0.12f, 0.12f));

    // 3. [ Отмена ]
    bool hovCancel = isPointInRect(mousePos, l.btnCancelPos, l.btnCancelSize);
    renderer->drawSprite(whiteTexture, l.btnCancelPos, l.btnCancelSize, 0.0f, hovCancel ? glm::vec3(0.60f, 0.65f, 0.75f) : glm::vec3(0.40f, 0.44f, 0.52f));
    renderer->drawSprite(whiteTexture, l.btnCancelPos + glm::vec2(2.0f), l.btnCancelSize - glm::vec2(4.0f), 0.0f, hovCancel ? glm::vec3(0.25f, 0.28f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.25f));

    renderer->flush();

    // Тексты в модальном окне
    if (textRenderer) {
        // Заголовок в шапке
        std::string titleStr = LOC("EDITOR_EXIT_TITLE");
        float tW = textRenderer->CalculateTextWidth(titleStr, l.fTitle);
        textRenderer->RenderText(titleStr, l.modalPos.x + (l.modalSize.x - tW) * 0.5f,
                                 l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 2.0f,
                                 l.fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

        // Крестик [X]
        float crossW = textRenderer->CalculateTextWidth("x", l.fCross);
        textRenderer->RenderText("x", l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f,
                                 l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 2.0f,
                                 l.fCross, glm::vec3(0.9f));

        // Основной вопрос
        std::string questionStr = LOC("EDITOR_EXIT_QUESTION");
        float qW = textRenderer->CalculateTextWidth(questionStr, l.fQuestion);
        textRenderer->RenderText(questionStr, l.modalPos.x + (l.modalSize.x - qW) * 0.5f, l.questionY, l.fQuestion, glm::vec3(1.0f, 1.0f, 1.0f));

        // Поясняющий подтекст
        std::string subStr = LOC("EDITOR_EXIT_SUB");
        float sW = textRenderer->CalculateTextWidth(subStr, l.fSub);
        textRenderer->RenderText(subStr, l.modalPos.x + (l.modalSize.x - sW) * 0.5f, l.subY, l.fSub, glm::vec3(0.85f, 0.65f, 0.65f));

        // Текст кнопки 1: Сохранить и выйти
        std::string saveExitStr = LOC("EDITOR_EXIT_SAVE_AND_EXIT");
        float seW = textRenderer->CalculateTextWidth(saveExitStr, l.fBtn);
        float seX = l.btnSaveExitPos.x + (l.btnSaveExitSize.x - seW) * 0.5f;
        float seY = l.btnSaveExitPos.y + (l.btnSaveExitSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(saveExitStr, seX, seY, l.fBtn, glm::vec3(0.95f, 1.0f, 0.95f));

        // Текст кнопки 2: Выйти без сохранения
        std::string discardStr = LOC("EDITOR_EXIT_DISCARD");
        float dW = textRenderer->CalculateTextWidth(discardStr, l.fBtn);
        float dX = l.btnDiscardPos.x + (l.btnDiscardSize.x - dW) * 0.5f;
        float dY = l.btnDiscardPos.y + (l.btnDiscardSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(discardStr, dX, dY, l.fBtn, glm::vec3(1.0f, 0.90f, 0.90f));

        // Текст кнопки 3: Отмена
        std::string cancelStr = LOC("EDITOR_EXIT_CANCEL");
        float cW = textRenderer->CalculateTextWidth(cancelStr, l.fBtn);
        float cX = l.btnCancelPos.x + (l.btnCancelSize.x - cW) * 0.5f;
        float cY = l.btnCancelPos.y + (l.btnCancelSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        textRenderer->RenderText(cancelStr, cX, cY, l.fBtn, glm::vec3(0.90f, 0.92f, 0.96f));
    }
}

