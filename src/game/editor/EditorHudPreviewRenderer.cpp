#include "EditorHudPreviewRenderer.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../textures/Texture2D.h"
#include "../ui/UICommon.h"
#include "../ui/BuildPanel.h"
#include <algorithm>
#include <cmath>

void EditorHudPreviewRenderer::renderWorldOverlay(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    HudPreviewMode mode,
    const LevelMapData::CameraSettings& camSettings,
    glm::vec2 camCenter,
    glm::vec2 frameSize
) {
    if (!renderer || !whiteTexture || mode == HudPreviewMode::None) return;

    float previewScale = 1.0f;
    if (mode == HudPreviewMode::Scale100) {
        previewScale = 1.0f;
    } else if (mode == HudPreviewMode::Scale125) {
        previewScale = 1.25f;
    } else if (mode == HudPreviewMode::Scale150) {
        previewScale = 1.50f;
    }

    // Параметры видоискателя стартовой камеры
    float camZoom = std::clamp(camSettings.zoom, 0.4f, 2.5f);
    float frameW = frameSize.x;
    float frameH = frameSize.y;
    glm::vec2 framePos = camCenter - glm::vec2(frameW * 0.5f, frameH * 0.5f);

    // Масштаб элементов HUD в мировых координатах видоискателя
    float s = previewScale / camZoom;

    // Вспомогательные функции для рисования пунктирных линий (контуров безопасных зон)
    auto drawDashedHLine = [&](float x1, float x2, float y, glm::vec4 col) {
        float dashLen = 8.0f * (1.0f / camZoom);
        float gapLen = 6.0f * (1.0f / camZoom);
        float lineThick = 2.0f * (1.0f / camZoom);
        float curX = x1;
        while (curX < x2) {
            float curLen = std::min(dashLen, x2 - curX);
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(curX, y - lineThick * 0.5f), glm::vec2(curLen, lineThick), 0.0f, col);
            curX += dashLen + gapLen;
        }
    };

    auto drawDashedVLine = [&](float x, float y1, float y2, glm::vec4 col) {
        float dashLen = 8.0f * (1.0f / camZoom);
        float gapLen = 6.0f * (1.0f / camZoom);
        float lineThick = 2.0f * (1.0f / camZoom);
        float curY = y1;
        while (curY < y2) {
            float curLen = std::min(dashLen, y2 - curY);
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(x - lineThick * 0.5f, curY), glm::vec2(lineThick, curLen), 0.0f, col);
            curY += dashLen + gapLen;
        }
    };

    auto drawDashedRect = [&](glm::vec2 pos, glm::vec2 size, glm::vec4 col) {
        drawDashedHLine(pos.x, pos.x + size.x, pos.y, col);
        drawDashedHLine(pos.x, pos.x + size.x, pos.y + size.y, col);
        drawDashedVLine(pos.x, pos.y, pos.y + size.y, col);
        drawDashedVLine(pos.x + size.x, pos.y, pos.y + size.y, col);
    };

    glm::vec4 warnColor(1.0f, 0.78f, 0.15f, 0.95f);

    // =========================================================================
    // 1. ВЕРХНИЙ ПРАВЫЙ БЛОК: Панель ресурсов, тайм-контроль, старт волны
    // =========================================================================
    float hudPanelW = TimeControlUI::PANEL_W * s;
    float hudPanelH = TimeControlUI::PANEL_H * s;
    float hudPanelX = framePos.x + frameW - hudPanelW - TimeControlUI::MARGIN_RIGHT * s;
    float hudPanelY = framePos.y + TimeControlUI::TOP_Y * s;

    // Полупрозрачный силуэт подложки верхнего правого HUD
    renderer->drawSpriteRGBA(whiteTexture,
        glm::vec2(hudPanelX, hudPanelY),
        glm::vec2(hudPanelW, hudPanelH),
        0.0f,
        glm::vec4(0.08f, 0.10f, 0.15f, 0.75f));

    // Верхняя акцентная полоска
    renderer->drawSpriteRGBA(whiteTexture,
        glm::vec2(hudPanelX, hudPanelY),
        glm::vec2(hudPanelW, 2.0f * s),
        0.0f,
        glm::vec4(0.40f, 0.75f, 1.0f, 0.9f));

    // Индикаторы статов (HP, Gold, Wave)
    float padX = TimeControlUI::PANEL_PAD_X * s;
    float padY = TimeControlUI::PANEL_PAD_Y * s;
    float statsInnerW = hudPanelW - padX * 2.0f;
    float badgeGap = 4.0f * s;
    float b0w = std::floor((statsInnerW - badgeGap * 2.0f) * 0.28f);
    float b1w = std::floor((statsInnerW - badgeGap * 2.0f) * 0.36f);
    float b2w = (statsInnerW - badgeGap * 2.0f) - b0w - b1w;
    float statsH = TimeControlUI::STATS_ROW_H * s;

    // HP
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(hudPanelX + padX, hudPanelY + padY), glm::vec2(b0w, statsH), 0.0f, glm::vec4(0.14f, 0.17f, 0.24f, 0.85f));
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(hudPanelX + padX, hudPanelY + padY), glm::vec2(3.0f * s, statsH), 0.0f, glm::vec4(0.92f, 0.28f, 0.28f, 1.0f));

    // Gold
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap, hudPanelY + padY), glm::vec2(b1w, statsH), 0.0f, glm::vec4(0.14f, 0.17f, 0.24f, 0.85f));
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap, hudPanelY + padY), glm::vec2(3.0f * s, statsH), 0.0f, glm::vec4(1.0f, 0.82f, 0.22f, 1.0f));

    // Wave
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap + b1w + badgeGap, hudPanelY + padY), glm::vec2(b2w, statsH), 0.0f, glm::vec4(0.14f, 0.17f, 0.24f, 0.85f));
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap + b1w + badgeGap, hudPanelY + padY), glm::vec2(3.0f * s, statsH), 0.0f, glm::vec4(0.35f, 0.75f, 1.0f, 1.0f));

    // Кнопки управления временем (||, 1x, 2x, 4x)
    float btnY = hudPanelY + padY + statsH + TimeControlUI::ROW_GAP * s;
    float timeBtnW = TimeControlUI::TIME_BTN_W * s;
    float btnH = TimeControlUI::BTN_H * s;
    float btnGap = TimeControlUI::BTN_GAP * s;
    for (int i = 0; i < 4; ++i) {
        float bx = hudPanelX + padX + i * (timeBtnW + btnGap);
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(bx, btnY), glm::vec2(timeBtnW, btnH), 0.0f, glm::vec4(0.15f, 0.18f, 0.26f, 0.85f));
    }

    // Кнопка вызова волны (Start / Call Wave)
    float waveBtnX = hudPanelX + padX + 4.0f * (timeBtnW + btnGap);
    float waveBtnW = std::max(0.0f, (hudPanelX + hudPanelW - padX) - waveBtnX);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(waveBtnX, btnY), glm::vec2(waveBtnW, btnH), 0.0f, glm::vec4(0.18f, 0.45f, 0.24f, 0.90f));

    // =========================================================================
    // 2. ВЕРХНЯЯ ЛЕВАЯ КНОПКА: [ В редактор / Меню ]
    // =========================================================================
    float topLeftBtnX = framePos.x + 20.0f * s;
    float topLeftBtnY = framePos.y + 20.0f * s;
    float topLeftBtnW = 150.0f * s;
    float topLeftBtnH = 40.0f * s;

    renderer->drawSpriteRGBA(whiteTexture,
        glm::vec2(topLeftBtnX, topLeftBtnY),
        glm::vec2(topLeftBtnW, topLeftBtnH),
        0.0f,
        glm::vec4(0.11f, 0.14f, 0.18f, 0.75f));
    renderer->drawSpriteRGBA(whiteTexture,
        glm::vec2(topLeftBtnX, topLeftBtnY),
        glm::vec2(4.0f * s, topLeftBtnH),
        0.0f,
        glm::vec4(1.0f, 0.80f, 0.20f, 0.9f));

    // =========================================================================
    // 3. НИЖНЯЯ ПАНЕЛЬ СЛОТОВ БАШЕН (BuildPanel)
    // =========================================================================
    size_t towerCount = 3;
    float basePanelW = (Buildpanel::DOCK_PADDING * 2.0f) + (towerCount * Buildpanel::CARD_WIDTH) + ((towerCount - 1) * Buildpanel::DOCK_GAP);
    float dockW = basePanelW * s;
    float dockH = Buildpanel::UI_PANEL_HEIGHT * s;
    float dockX = framePos.x + (frameW - dockW) * 0.5f;
    float bottomMargin = Buildpanel::DOCK_BOTTOM_MARGIN * s;
    float dockY = framePos.y + frameH - dockH - bottomMargin;

    // Плавающий остров дока (подложка под карточки)
    renderer->drawSpriteRGBA(whiteTexture,
        glm::vec2(dockX, dockY),
        glm::vec2(dockW, dockH),
        0.0f,
        glm::vec4(0.06f, 0.08f, 0.11f, 0.75f));

    // Тонкая кайма острова
    float islandBorderT = 1.5f * s;
    glm::vec4 islandBorder(0.22f, 0.28f, 0.38f, 0.65f);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(dockX, dockY), glm::vec2(dockW, islandBorderT), 0.0f, islandBorder);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(dockX, dockY + dockH - islandBorderT), glm::vec2(dockW, islandBorderT), 0.0f, islandBorder);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(dockX, dockY), glm::vec2(islandBorderT, dockH), 0.0f, islandBorder);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(dockX + dockW - islandBorderT, dockY), glm::vec2(islandBorderT, dockH), 0.0f, islandBorder);

    // Карточки башен внутри BuildPanel
    for (size_t i = 0; i < towerCount; ++i) {
        float cardX = dockX + (Buildpanel::DOCK_PADDING * s) + i * ((Buildpanel::CARD_WIDTH + Buildpanel::DOCK_GAP) * s);
        float cardY = dockY + (Buildpanel::DOCK_PADDING * s);
        float cardW = Buildpanel::CARD_WIDTH * s;
        float cardH = Buildpanel::CARD_HEIGHT * s;

        // Плашка карточки
        renderer->drawSpriteRGBA(whiteTexture,
            glm::vec2(cardX, cardY),
            glm::vec2(cardW, cardH),
            0.0f,
            glm::vec4(0.10f, 0.12f, 0.16f, 0.85f));

        // Тонкая кайма карточки
        float cardBorderT = 1.5f * s;
        glm::vec4 cardBorder(0.24f, 0.32f, 0.44f, 0.85f);
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cardX, cardY), glm::vec2(cardW, cardBorderT), 0.0f, cardBorder);
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cardX, cardY + cardH - cardBorderT), glm::vec2(cardW, cardBorderT), 0.0f, cardBorder);
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cardX, cardY), glm::vec2(cardBorderT, cardH), 0.0f, cardBorder);
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cardX + cardW - cardBorderT, cardY), glm::vec2(cardBorderT, cardH), 0.0f, cardBorder);

        // Хоткей карточки
        renderer->drawSpriteRGBA(whiteTexture,
            glm::vec2(cardX + 4.0f * s, cardY + 4.0f * s),
            glm::vec2(14.0f * s, 8.0f * s),
            0.0f,
            glm::vec4(0.85f, 0.70f, 0.25f, 0.70f));

        // Силуэт иконки башни
        float iconSize = Buildpanel::UI_ICON_SIZE * s;
        float iconX = cardX + (cardW - iconSize) * 0.5f;
        float iconY = cardY + 18.0f * s;
        renderer->drawSpriteRGBA(whiteTexture,
            glm::vec2(iconX, iconY),
            glm::vec2(iconSize, iconSize),
            0.0f,
            glm::vec4(0.18f, 0.22f, 0.30f, 0.85f));

        // Силуэт ценника башни
        renderer->drawSpriteRGBA(whiteTexture,
            glm::vec2(cardX + 10.0f * s, cardY + cardH - 16.0f * s),
            glm::vec2(cardW - 20.0f * s, 10.0f * s),
            0.0f,
            glm::vec4(0.25f, 0.75f, 0.35f, 0.85f));
    }

    // =========================================================================
    // 4. КОНТУРЫ БЕЗОПАСНЫХ ЗОН (SAFE AREA): Желтый пунктир вокруг реальных блоков
    // =========================================================================
    drawDashedRect(glm::vec2(hudPanelX, hudPanelY), glm::vec2(hudPanelW, hudPanelH), warnColor);
    drawDashedRect(glm::vec2(topLeftBtnX, topLeftBtnY), glm::vec2(topLeftBtnW, topLeftBtnH), warnColor);
    drawDashedRect(glm::vec2(dockX, dockY), glm::vec2(dockW, dockH), warnColor);
}

void EditorHudPreviewRenderer::renderScreenText(
    TextRenderer* textRenderer,
    HudPreviewMode mode,
    const LevelMapData::CameraSettings& camSettings,
    glm::vec2 camCenter,
    glm::vec2 frameSize,
    const EditorCamera& camera
) {
    if (!textRenderer || mode == HudPreviewMode::None) return;

    float previewScale = 1.0f;
    std::string scaleText = "100%";
    if (mode == HudPreviewMode::Scale100) {
        previewScale = 1.0f;
        scaleText = "100%";
    } else if (mode == HudPreviewMode::Scale125) {
        previewScale = 1.25f;
        scaleText = "125%";
    } else if (mode == HudPreviewMode::Scale150) {
        previewScale = 1.50f;
        scaleText = "150%";
    }

    float camZoom = std::clamp(camSettings.zoom, 0.4f, 2.5f);
    float frameW = frameSize.x;
    float frameH = frameSize.y;
    glm::vec2 framePos = camCenter - glm::vec2(frameW * 0.5f, frameH * 0.5f);
    float s = previewScale / camZoom;

    // Мировые координаты элементов
    float hudPanelW = TimeControlUI::PANEL_W * s;
    float hudPanelH = TimeControlUI::PANEL_H * s;
    float hudPanelX = framePos.x + frameW - hudPanelW - TimeControlUI::MARGIN_RIGHT * s;
    float hudPanelY = framePos.y + TimeControlUI::TOP_Y * s;

    float padX = TimeControlUI::PANEL_PAD_X * s;
    float padY = TimeControlUI::PANEL_PAD_Y * s;
    float statsH = TimeControlUI::STATS_ROW_H * s;
    float btnY = hudPanelY + padY + statsH + TimeControlUI::ROW_GAP * s;
    float timeBtnW = TimeControlUI::TIME_BTN_W * s;
    float btnGap = TimeControlUI::BTN_GAP * s;
    float waveBtnX = hudPanelX + padX + 4.0f * (timeBtnW + btnGap);

    float topLeftBtnX = framePos.x + 20.0f * s;
    float topLeftBtnY = framePos.y + 20.0f * s;

    size_t towerCount = 3;
    float basePanelW = (Buildpanel::DOCK_PADDING * 2.0f) + (towerCount * Buildpanel::CARD_WIDTH) + ((towerCount - 1) * Buildpanel::DOCK_GAP);
    float dockW = basePanelW * s;
    float dockH = Buildpanel::UI_PANEL_HEIGHT * s;
    float dockX = framePos.x + (frameW - dockW) * 0.5f;
    float bottomMargin = Buildpanel::DOCK_BOTTOM_MARGIN * s;
    float dockY = framePos.y + frameH - dockH - bottomMargin;

    // Перевод мировых координат в экранные с учетом зума редактора
    glm::vec2 sTopLeft = camera.worldToScreen(glm::vec2(topLeftBtnX, topLeftBtnY));
    glm::vec2 sHudPanel = camera.worldToScreen(glm::vec2(hudPanelX, hudPanelY));
    glm::vec2 sWaveBtn = camera.worldToScreen(glm::vec2(waveBtnX, btnY));
    glm::vec2 sDock = camera.worldToScreen(glm::vec2(dockX, dockY));

    float fontScale = std::clamp(0.40f * previewScale * (camera.zoom / camZoom), 0.22f, 0.70f);

    // Метка верхней левой кнопки
    textRenderer->RenderText("[ В РЕДАКТОР ]", sTopLeft.x + 16.0f * s * camera.zoom, sTopLeft.y + 12.0f * s * camera.zoom, fontScale, glm::vec3(0.95f, 0.95f, 1.0f));

    // Метка верхнего правого HUD
    std::string topLabel = "БОЕВОЙ HUD (" + scaleText + ")";
    float tlw = textRenderer->CalculateTextWidth(topLabel, fontScale);
    float screenHudPanelW = hudPanelW * camera.zoom;
    textRenderer->RenderText(topLabel, sHudPanel.x + (screenHudPanelW - tlw) * 0.5f, sHudPanel.y + hudPanelH * camera.zoom + 6.0f * s * camera.zoom, fontScale, glm::vec3(1.0f, 0.82f, 0.20f));

    // Метка кнопки старта волны
    float miniFont = std::clamp(0.32f * previewScale * (camera.zoom / camZoom), 0.18f, 0.55f);
    textRenderer->RenderText("СТАРТ", sWaveBtn.x + 8.0f * s * camera.zoom, sWaveBtn.y + 8.0f * s * camera.zoom, miniFont, glm::vec3(0.95f, 1.0f, 0.95f));

    // Метка нижнего центрального дока строительства
    std::string dockLabel = "ДОК БАШЕН (" + scaleText + ")";
    float dlw = textRenderer->CalculateTextWidth(dockLabel, fontScale);
    float screenDockW = dockW * camera.zoom;
    textRenderer->RenderText(dockLabel, sDock.x + (screenDockW - dlw) * 0.5f, sDock.y - (fontScale * 26.0f) - 4.0f * s * camera.zoom, fontScale, glm::vec3(1.0f, 0.82f, 0.20f));
}

