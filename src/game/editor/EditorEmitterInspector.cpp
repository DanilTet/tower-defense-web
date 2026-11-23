#include "EditorEmitterInspector.h"
#include "../ui/UICommon.h"
#include "../core/InputManager.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../../resources/ResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

bool EditorEmitterInspector::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

EmitterInspectorLayout EditorEmitterInspector::getLayout(const ParticleEmitterConfig& config, int emitterIndex,
                                                         glm::vec2 emitterScreenPos,
                                                         float screenW, float screenH,
                                                         float topBarH, float bottomDockH) const {
    EmitterInspectorLayout l;

    float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));
    float w = 286.0f * s;
    float h = 356.0f * s;
    l.panelSize = glm::vec2(w, h);
    l.headerH = 32.0f * s;
    l.fontScale = std::clamp(0.40f * s, 0.32f, 0.60f);

    float minX = 12.0f * s;
    float maxX = std::max(minX, screenW - w - 12.0f * s);
    float minY = topBarH + 8.0f * s;
    float maxY = std::max(minY, screenH - bottomDockH - h - 8.0f * s);

    float px = 0.0f;
    float py = 0.0f;

    if (m_panelPos.x >= 0.0f && m_panelPos.y >= 0.0f && m_lastEmitterIdx == emitterIndex) {
        px = std::clamp(m_panelPos.x, minX, maxX);
        py = std::clamp(m_panelPos.y, minY, maxY);
    } else {
        float rad = glm::radians(config.angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));

        if (dir.x > 0.3f) {
            px = emitterScreenPos.x - w - 50.0f * s;
        } else {
            px = emitterScreenPos.x + 60.0f * s;
        }

        if (dir.y < -0.3f) {
            py = emitterScreenPos.y + 40.0f * s;
        } else {
            py = emitterScreenPos.y - h * 0.5f;
        }

        if (px < minX) px = emitterScreenPos.x + 60.0f * s;
        if (px > maxX) px = emitterScreenPos.x - w - 50.0f * s;
        px = std::clamp(px, minX, maxX);
        py = std::clamp(py, minY, maxY);

        m_panelPos = glm::vec2(px, py);
        m_lastEmitterIdx = emitterIndex;
    }
    l.panelPos = glm::vec2(px, py);

    float crossW = 24.0f * s;
    float crossH = 22.0f * s;
    l.btnCloseCrossPos = glm::vec2(px + w - crossW - 6.0f * s, py + (l.headerH - crossH) * 0.5f);
    l.btnCloseCrossSize = glm::vec2(crossW, crossH);

    float padX = 10.0f * s;
    float rowW = w - padX * 2.0f;
    float rowH = 26.0f * s;
    float rowGap = 5.0f * s;
    float startY = py + l.headerH + 7.0f * s;

    // Ряд 0: РЕЖИМ
    float y0 = startY;
    l.btnModePos = glm::vec2(px + padX, y0);
    l.btnModeSize = glm::vec2(rowW, rowH);

    // Ряд 1: ТИП
    float y1 = y0 + rowH + rowGap;
    float prevNextW = 26.0f * s;
    l.btnTypePrevPos = glm::vec2(px + padX, y1);
    l.btnTypePrevSize = glm::vec2(prevNextW, rowH);
    l.btnTypePos = glm::vec2(px + padX + prevNextW + 4.0f * s, y1);
    l.btnTypeSize = glm::vec2(rowW - (prevNextW + 4.0f * s) * 2.0f, rowH);
    l.btnTypeNextPos = glm::vec2(px + padX + rowW - prevNextW, y1);
    l.btnTypeNextSize = glm::vec2(prevNextW, rowH);

    // Ряд 1.5: СЛОЙ (zIndex) [-1] [ СЛОЙ: N ] [+1]
    float yLayer = y1 + rowH + rowGap;
    float btnLayerW = 28.0f * s;
    l.btnLayerDecPos = glm::vec2(px + padX, yLayer);
    l.btnLayerDecSize = glm::vec2(btnLayerW, rowH);
    l.layerValuePos = glm::vec2(px + padX + btnLayerW + 4.0f * s, yLayer);
    l.layerValueSize = glm::vec2(rowW - (btnLayerW + 4.0f * s) * 2.0f, rowH);
    l.btnLayerIncPos = glm::vec2(px + padX + rowW - btnLayerW, yLayer);
    l.btnLayerIncSize = glm::vec2(btnLayerW, rowH);

    // Ряд 2: УГОЛ
    float y2 = yLayer + rowH + rowGap;
    float btnAngleDecW = 36.0f * s;
    l.btnAngleDecPos = glm::vec2(px + padX, y2);
    l.btnAngleDecSize = glm::vec2(btnAngleDecW, rowH);
    l.btnAnglePos = glm::vec2(px + padX + btnAngleDecW + 4.0f * s, y2);
    l.btnAngleSize = glm::vec2(rowW - (btnAngleDecW + 4.0f * s) * 2.0f, rowH);
    l.btnAngleIncPos = glm::vec2(px + padX + rowW - btnAngleDecW, y2);
    l.btnAngleIncSize = glm::vec2(btnAngleDecW, rowH);

    // Ряд 3: СИЛА
    float y3 = y2 + rowH + rowGap;
    float btnStepW = 28.0f * s;
    l.btnSpeedDecPos = glm::vec2(px + padX, y3);
    l.btnSpeedDecSize = glm::vec2(btnStepW, rowH);
    l.speedValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y3);
    l.btnSpeedIncPos = glm::vec2(px + padX + rowW - btnStepW, y3);
    l.btnSpeedIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 4: РАЗМЕР
    float y4 = y3 + rowH + rowGap;
    l.btnScaleDecPos = glm::vec2(px + padX, y4);
    l.btnScaleDecSize = glm::vec2(btnStepW, rowH);
    l.scaleValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y4);
    l.btnScaleIncPos = glm::vec2(px + padX + rowW - btnStepW, y4);
    l.btnScaleIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 5: ЧАСТОТА
    float y5 = y4 + rowH + rowGap;
    l.btnPeriodDecPos = glm::vec2(px + padX, y5);
    l.btnPeriodDecSize = glm::vec2(btnStepW, rowH);
    l.periodValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y5);
    l.btnPeriodIncPos = glm::vec2(px + padX + rowW - btnStepW, y5);
    l.btnPeriodIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 6: ДЛИТЕЛЬНОСТЬ
    float y6 = y5 + rowH + rowGap;
    l.btnBurstDecPos = glm::vec2(px + padX, y6);
    l.btnBurstDecSize = glm::vec2(btnStepW, rowH);
    l.burstValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y6);
    l.btnBurstIncPos = glm::vec2(px + padX + rowW - btnStepW, y6);
    l.btnBurstIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 7: Кнопки удалить / закрыть
    float y7 = y6 + rowH + 8.0f * s;
    float botBtnW = (rowW - 6.0f * s) * 0.5f;
    l.btnDeletePos = glm::vec2(px + padX, y7);
    l.btnDeleteSize = glm::vec2(botBtnW, 26.0f * s);
    l.btnClosePos = glm::vec2(px + padX + botBtnW + 6.0f * s, y7);
    l.btnCloseSize = glm::vec2(botBtnW, 26.0f * s);

    return l;
}

bool EditorEmitterInspector::handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                                         ParticleEmitterConfig& config, int emitterIndex, glm::vec2 emitterScreenPos,
                                         float screenW, float screenH, float topBarH, float bottomDockH,
                                         const std::function<void()>& onChanged,
                                         const std::function<void()>& onDelete,
                                         const std::function<void()>& onClose,
                                         const std::function<void(const std::string&, const glm::vec4&)>& showToast) {
    EmitterInspectorLayout l = getLayout(config, emitterIndex, emitterScreenPos, screenW, screenH, topBarH, bottomDockH);

    // 0. Dragging окна за шапку
    if (m_isDragging) {
        if (!leftDown) {
            m_isDragging = false;
        } else {
            float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));
            float minX = 12.0f * s;
            float maxX = std::max(minX, screenW - l.panelSize.x - 12.0f * s);
            float minY = topBarH + 8.0f * s;
            float maxY = std::max(minY, screenH - bottomDockH - l.panelSize.y - 8.0f * s);

            glm::vec2 newPos = mousePos - m_dragOffset;
            m_panelPos.x = std::clamp(newPos.x, minX, maxX);
            m_panelPos.y = std::clamp(newPos.y, minY, maxY);
            return true;
        }
    }

    bool insidePanel = (mousePos.x >= l.panelPos.x && mousePos.x <= l.panelPos.x + l.panelSize.x &&
                        mousePos.y >= l.panelPos.y && mousePos.y <= l.panelPos.y + l.panelSize.y);

    if (!insidePanel) {
        return false;
    }

    float scrollY = InputManager::getScrollY();
    if (std::abs(scrollY) > 0.01f) {
        if (isPointInRect(mousePos, l.btnLayerDecPos, l.btnLayerDecSize) ||
            isPointInRect(mousePos, l.layerValuePos, l.layerValueSize) ||
            isPointInRect(mousePos, l.btnLayerIncPos, l.btnLayerIncSize)) {
            int delta = (scrollY > 0.0f) ? 1 : -1;
            config.zIndex = std::clamp(config.zIndex + delta, -10, 10);
            if (onChanged) onChanged();
            if (showToast) showToast("Слой эмиттера (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
            return true;
        } else if (isPointInRect(mousePos, l.btnAngleDecPos, l.btnAngleDecSize) ||
                   isPointInRect(mousePos, l.btnAnglePos, l.btnAngleSize) ||
                   isPointInRect(mousePos, l.btnAngleIncPos, l.btnAngleIncSize)) {
            float delta = (scrollY > 0.0f) ? 15.0f : -15.0f;
            config.angleDeg = std::fmod(config.angleDeg + delta + 360.0f, 360.0f);
            if (onChanged) onChanged();
            return true;
        }
    }

    // Если курсор над панелью, но клика не было - поглощаем курсор
    if (!leftDown || wasLeftDown) {
        return true;
    }

    // 1. Закрытие [ X ] или [ Закрыть ]
    if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize) ||
        isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize)) {
        m_isDragging = false;
        if (onClose) onClose();
        return true;
    }

    // 2. Начало перетаскивания за шапку
    bool insideHeader = (mousePos.x >= l.panelPos.x && mousePos.x <= l.panelPos.x + l.panelSize.x &&
                         mousePos.y >= l.panelPos.y && mousePos.y <= l.panelPos.y + l.headerH);
    if (insideHeader) {
        m_isDragging = true;
        m_dragOffset = mousePos - l.panelPos;
        return true;
    }

    // 3. Удаление [ Удалить ]
    if (isPointInRect(mousePos, l.btnDeletePos, l.btnDeleteSize)) {
        m_isDragging = false;
        if (onDelete) onDelete();
        return true;
    }

    // 4. Режим: [ ПЕРИОДИЧЕСКИЙ (ЗАЛПЫ) ] <--> [ ПОСТОЯННЫЙ (ЦИКЛ) ]
    if (isPointInRect(mousePos, l.btnModePos, l.btnModeSize)) {
        config.loopContinuous = !config.loopContinuous;
        if (onChanged) onChanged();
        if (showToast) {
            if (config.loopContinuous) {
                showToast("Режим: ПОСТОЯННЫЙ (ЦИКЛ)", glm::vec4(0.35f, 0.95f, 0.55f, 1.0f));
            } else {
                showToast("Режим: ПЕРИОДИЧЕСКИЙ (ЗАЛПЫ)", glm::vec4(0.4f, 0.8f, 1.0f, 1.0f));
            }
        }
        return true;
    }

    // 5. Тип: [ ◀ ] [ ТИП ] [ ▶ ]
    static const std::vector<std::string> s_types = { "steam_jet", "water_drip", "sparks", "smoke", "fog" };
    auto cycleType = [&](int dir) {
        int idx = 0;
        for (int i = 0; i < static_cast<int>(s_types.size()); ++i) {
            if (s_types[i] == config.type) { idx = i; break; }
        }
        idx = (idx + dir + static_cast<int>(s_types.size())) % static_cast<int>(s_types.size());
        config.type = s_types[idx];
        if (onChanged) onChanged();
    };

    if (isPointInRect(mousePos, l.btnTypePrevPos, l.btnTypePrevSize)) {
        cycleType(-1);
        return true;
    }
    if (isPointInRect(mousePos, l.btnTypeNextPos, l.btnTypeNextSize) || isPointInRect(mousePos, l.btnTypePos, l.btnTypeSize)) {
        cycleType(1);
        return true;
    }

    // 5.5. Слой (zIndex): [-1] [ СЛОЙ: N ] [+1]
    if (isPointInRect(mousePos, l.btnLayerDecPos, l.btnLayerDecSize)) {
        config.zIndex = std::clamp(config.zIndex - 1, -10, 10);
        if (onChanged) onChanged();
        if (showToast) showToast("Слой эмиттера (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        return true;
    }
    if (isPointInRect(mousePos, l.btnLayerIncPos, l.btnLayerIncSize)) {
        config.zIndex = std::clamp(config.zIndex + 1, -10, 10);
        if (onChanged) onChanged();
        if (showToast) showToast("Слой эмиттера (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        return true;
    }
    if (isPointInRect(mousePos, l.layerValuePos, l.layerValueSize)) {
        config.zIndex = (config.zIndex != 0) ? 0 : 1;
        if (onChanged) onChanged();
        if (showToast) showToast("Слой эмиттера (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        return true;
    }

    // 6. Угол: [ -45° ] [ УГОЛ ↻ ] [ +45° ]
    if (isPointInRect(mousePos, l.btnAngleDecPos, l.btnAngleDecSize)) {
        config.angleDeg = std::fmod(config.angleDeg - 45.0f + 360.0f, 360.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnAngleIncPos, l.btnAngleIncSize) || isPointInRect(mousePos, l.btnAnglePos, l.btnAngleSize)) {
        config.angleDeg = std::fmod(config.angleDeg + 45.0f, 360.0f);
        if (config.angleDeg < 0.0f) config.angleDeg += 360.0f;
        if (onChanged) onChanged();
        return true;
    }

    // 7. Скорость (Сила): [-] [+]
    if (isPointInRect(mousePos, l.btnSpeedDecPos, l.btnSpeedDecSize)) {
        config.speed = std::max(40.0f, config.speed - 20.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnSpeedIncPos, l.btnSpeedIncSize)) {
        config.speed = std::min(450.0f, config.speed + 20.0f);
        if (onChanged) onChanged();
        return true;
    }

    // 8. Размер частиц: [-] [+]
    if (isPointInRect(mousePos, l.btnScaleDecPos, l.btnScaleDecSize)) {
        config.particleScale = std::max(0.5f, std::round((config.particleScale - 0.2f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnScaleIncPos, l.btnScaleIncSize)) {
        config.particleScale = std::min(2.5f, std::round((config.particleScale + 0.2f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }

    // 9. Частота (период): [-] [+] (активно только в периодическом режиме)
    if (!config.loopContinuous) {
        if (isPointInRect(mousePos, l.btnPeriodDecPos, l.btnPeriodDecSize)) {
            config.periodMin = std::max(1.0f, config.periodMin - 1.0f);
            config.periodMax = std::max(config.periodMin + 1.0f, config.periodMax - 2.0f);
            if (onChanged) onChanged();
            return true;
        }
        if (isPointInRect(mousePos, l.btnPeriodIncPos, l.btnPeriodIncSize)) {
            config.periodMin = std::min(25.0f, config.periodMin + 1.0f);
            config.periodMax = std::min(40.0f, config.periodMax + 2.0f);
            if (onChanged) onChanged();
            return true;
        }
    }

    // 10. Длительность залпа: [-] [+]
    if (isPointInRect(mousePos, l.btnBurstDecPos, l.btnBurstDecSize)) {
        config.burstDuration = std::max(0.4f, std::round((config.burstDuration - 0.3f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnBurstIncPos, l.btnBurstIncSize)) {
        config.burstDuration = std::min(8.0f, std::round((config.burstDuration + 0.3f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }

    return true;
}

void EditorEmitterInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                    const ParticleEmitterConfig& config, int emitterIndex, glm::vec2 emitterScreenPos,
                                    float screenW, float screenH, float topBarH, float bottomDockH,
                                    glm::vec2 mousePos) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    render(renderer, textRenderer, whiteTex, config, emitterIndex, emitterScreenPos, screenW, screenH, topBarH, bottomDockH, mousePos);
}

void EditorEmitterInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                    const Texture2D* whiteTexture,
                                    const ParticleEmitterConfig& config, int emitterIndex, glm::vec2 emitterScreenPos,
                                    float screenW, float screenH, float topBarH, float bottomDockH,
                                    glm::vec2 mousePos) {
    if (!renderer || !whiteTexture) return;

    EmitterInspectorLayout l = getLayout(config, emitterIndex, emitterScreenPos, screenW, screenH, topBarH, bottomDockH);
    float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));

    auto isHover = [mousePos](glm::vec2 pos, glm::vec2 sz) {
        return mousePos.x >= pos.x && mousePos.x <= pos.x + sz.x &&
               mousePos.y >= pos.y && mousePos.y <= pos.y + sz.y;
    };

    auto drawBtn = [&](glm::vec2 pos, glm::vec2 sz, glm::vec4 baseBg, glm::vec4 hoverBg, glm::vec4 borderCol) {
        bool hov = isHover(pos, sz);
        glm::vec4 bg = hov ? hoverBg : baseBg;
        renderer->drawSpriteRGBA(whiteTexture, pos, sz, 0.0f, bg);
        float b = 1.0f * s;
        renderer->drawSpriteRGBA(whiteTexture, pos, glm::vec2(sz.x, b), 0.0f, borderCol);
        renderer->drawSpriteRGBA(whiteTexture, pos + glm::vec2(0.0f, sz.y - b), glm::vec2(sz.x, b), 0.0f, borderCol);
        renderer->drawSpriteRGBA(whiteTexture, pos, glm::vec2(b, sz.y), 0.0f, borderCol);
        renderer->drawSpriteRGBA(whiteTexture, pos + glm::vec2(sz.x - b, 0.0f), glm::vec2(b, sz.y), 0.0f, borderCol);
    };

    // 1. Тень и тело плавающей панели
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(4.0f * s, 4.0f * s), l.panelSize, 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, l.panelSize, 0.0f, glm::vec4(0.08f, 0.10f, 0.14f, 0.94f));

    // Шапка панели
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(l.panelSize.x, l.headerH), 0.0f, glm::vec4(0.12f, 0.16f, 0.22f, 0.98f));
    // Кайма панели
    float panelBorderT = 1.5f * s;
    glm::vec4 panelBorder(0.28f, 0.40f, 0.58f, 0.90f);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(l.panelSize.x, panelBorderT), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(0.0f, l.panelSize.y - panelBorderT), glm::vec2(l.panelSize.x, panelBorderT), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(panelBorderT, l.panelSize.y), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(l.panelSize.x - panelBorderT, 0.0f), glm::vec2(panelBorderT, l.panelSize.y), 0.0f, panelBorder);

    // Разделитель под шапкой
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.panelSize.x, 1.0f * s), 0.0f, glm::vec4(0.25f, 0.35f, 0.48f, 0.8f));

    // Кнопка [ X ] в шапке
    drawBtn(l.btnCloseCrossPos, l.btnCloseCrossSize,
            glm::vec4(0.24f, 0.12f, 0.14f, 0.85f), glm::vec4(0.70f, 0.20f, 0.25f, 0.95f), glm::vec4(0.85f, 0.35f, 0.40f, 0.9f));

    // Ряд 0: Режим
    glm::vec4 modeBaseBg = config.loopContinuous ? glm::vec4(0.12f, 0.28f, 0.18f, 0.90f) : glm::vec4(0.15f, 0.20f, 0.28f, 0.85f);
    glm::vec4 modeHovBg  = config.loopContinuous ? glm::vec4(0.18f, 0.38f, 0.24f, 0.98f) : glm::vec4(0.22f, 0.30f, 0.42f, 0.95f);
    glm::vec4 modeBorder = config.loopContinuous ? glm::vec4(0.35f, 0.75f, 0.45f, 0.95f) : glm::vec4(0.35f, 0.48f, 0.65f, 0.85f);
    drawBtn(l.btnModePos, l.btnModeSize, modeBaseBg, modeHovBg, modeBorder);

    // Ряд 1: Тип
    glm::vec4 btnBaseBg(0.14f, 0.18f, 0.24f, 0.85f);
    glm::vec4 btnHoverBg(0.22f, 0.28f, 0.38f, 0.95f);
    glm::vec4 btnBorder(0.32f, 0.42f, 0.55f, 0.85f);

    drawBtn(l.btnTypePrevPos, l.btnTypePrevSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.btnTypePos, l.btnTypeSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.btnTypeNextPos, l.btnTypeNextSize, btnBaseBg, btnHoverBg, btnBorder);

    // Ряд 1.5: Слой (zIndex)
    drawBtn(l.btnLayerDecPos, l.btnLayerDecSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.layerValuePos, l.layerValueSize,
            glm::vec4(0.10f, 0.14f, 0.20f, 0.85f), glm::vec4(0.14f, 0.20f, 0.28f, 0.95f), glm::vec4(0.30f, 0.45f, 0.60f, 0.85f));
    drawBtn(l.btnLayerIncPos, l.btnLayerIncSize, btnBaseBg, btnHoverBg, btnBorder);

    // Ряд 2: Угол
    drawBtn(l.btnAngleDecPos, l.btnAngleDecSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.btnAnglePos, l.btnAngleSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.btnAngleIncPos, l.btnAngleIncSize, btnBaseBg, btnHoverBg, btnBorder);

    // Ряд 3: Скорость
    glm::vec2 speedValSz(l.panelSize.x - (l.btnSpeedDecSize.x * 2.0f + 20.0f * s + 8.0f * s), l.btnSpeedDecSize.y);
    drawBtn(l.btnSpeedDecPos, l.btnSpeedDecSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.speedValuePos, speedValSz,
            glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.22f, 0.26f, 0.34f, 0.7f));
    drawBtn(l.btnSpeedIncPos, l.btnSpeedIncSize, btnBaseBg, btnHoverBg, btnBorder);

    // Ряд 4: Размер
    drawBtn(l.btnScaleDecPos, l.btnScaleDecSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.scaleValuePos, speedValSz,
            glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.22f, 0.26f, 0.34f, 0.7f));
    drawBtn(l.btnScaleIncPos, l.btnScaleIncSize, btnBaseBg, btnHoverBg, btnBorder);

    // Ряд 5: Частота
    if (config.loopContinuous) {
        glm::vec4 disabledBg(0.10f, 0.12f, 0.15f, 0.50f);
        glm::vec4 disabledBorder(0.18f, 0.22f, 0.28f, 0.40f);
        drawBtn(l.btnPeriodDecPos, l.btnPeriodDecSize, disabledBg, disabledBg, disabledBorder);
        drawBtn(l.periodValuePos, speedValSz, disabledBg, disabledBg, disabledBorder);
        drawBtn(l.btnPeriodIncPos, l.btnPeriodIncSize, disabledBg, disabledBg, disabledBorder);
    } else {
        drawBtn(l.btnPeriodDecPos, l.btnPeriodDecSize, btnBaseBg, btnHoverBg, btnBorder);
        drawBtn(l.periodValuePos, speedValSz,
                glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.22f, 0.26f, 0.34f, 0.7f));
        drawBtn(l.btnPeriodIncPos, l.btnPeriodIncSize, btnBaseBg, btnHoverBg, btnBorder);
    }

    // Ряд 6: Длительность
    drawBtn(l.btnBurstDecPos, l.btnBurstDecSize, btnBaseBg, btnHoverBg, btnBorder);
    drawBtn(l.burstValuePos, speedValSz,
            glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.11f, 0.13f, 0.18f, 0.85f), glm::vec4(0.22f, 0.26f, 0.34f, 0.7f));
    drawBtn(l.btnBurstIncPos, l.btnBurstIncSize, btnBaseBg, btnHoverBg, btnBorder);

    // Ряд 7: Удалить / Закрыть
    drawBtn(l.btnDeletePos, l.btnDeleteSize,
            glm::vec4(0.26f, 0.12f, 0.14f, 0.85f), glm::vec4(0.55f, 0.18f, 0.22f, 0.95f), glm::vec4(0.70f, 0.25f, 0.30f, 0.85f));
    drawBtn(l.btnClosePos, l.btnCloseSize,
            glm::vec4(0.16f, 0.20f, 0.26f, 0.85f), glm::vec4(0.24f, 0.30f, 0.40f, 0.95f), glm::vec4(0.35f, 0.45f, 0.58f, 0.85f));

    renderer->flush();

    if (!textRenderer) return;

    // Текст заголовка
    std::string title = "ЭМИТТЕР #" + std::to_string(emitterIndex + 1);
    textRenderer->RenderText(title, l.panelPos.x + 10.0f * s, l.panelPos.y + 7.0f * s, l.fontScale * 1.05f, glm::vec3(0.95f, 0.95f, 1.0f));

    // Символ [ X ]
    textRenderer->RenderText("X", l.btnCloseCrossPos.x + 7.0f * s, l.btnCloseCrossPos.y + 4.0f * s, l.fontScale, glm::vec3(1.0f, 0.65f, 0.65f));

    auto renderCentered = [textRenderer](const std::string& str, glm::vec2 pos, glm::vec2 sz, float scale, glm::vec3 col) {
        float tw = textRenderer->CalculateTextWidth(str, scale);
        float tx = pos.x + (sz.x - tw) * 0.5f;
        float ty = pos.y + (sz.y - 14.0f * scale) * 0.5f;
        textRenderer->RenderText(str, tx, ty, scale, col);
    };

    // Текст ряда 0: Режим
    if (config.loopContinuous) {
        renderCentered("РЕЖИМ: ПОСТОЯННЫЙ (ЦИКЛ)", l.btnModePos, l.btnModeSize, l.fontScale * 0.95f, glm::vec3(0.40f, 1.0f, 0.60f));
    } else {
        renderCentered("РЕЖИМ: ПЕРИОДИЧЕСКИЙ (ЗАЛПЫ)", l.btnModePos, l.btnModeSize, l.fontScale * 0.95f, glm::vec3(0.75f, 0.90f, 1.0f));
    }

    // Текст ряда 1: Тип
    std::string typeName = "ПАР";
    glm::vec3 typeCol(0.40f, 0.85f, 1.0f);
    if (config.type == "water_drip") { typeName = "КАПЕЛЬ"; typeCol = glm::vec3(0.35f, 0.65f, 1.0f); }
    else if (config.type == "sparks") { typeName = "ИСКРЫ"; typeCol = glm::vec3(1.0f, 0.85f, 0.35f); }
    else if (config.type == "smoke") { typeName = "ДЫМ"; typeCol = glm::vec3(0.85f, 0.85f, 0.90f); }
    else if (config.type == "fog") { typeName = "ТУМАН"; typeCol = glm::vec3(0.65f, 0.85f, 0.75f); }

    renderCentered("<", l.btnTypePrevPos, l.btnTypePrevSize, l.fontScale, glm::vec3(0.7f, 0.85f, 1.0f));
    renderCentered("ТИП: " + typeName, l.btnTypePos, l.btnTypeSize, l.fontScale, typeCol);
    renderCentered(">", l.btnTypeNextPos, l.btnTypeNextSize, l.fontScale, glm::vec3(0.7f, 0.85f, 1.0f));

    // Текст ряда 1.5: Слой (zIndex)
    std::string layerText = std::string("СЛОЙ: ") + (config.zIndex > 0 ? "+" : "") + std::to_string(config.zIndex);
    renderCentered("-1", l.btnLayerDecPos, l.btnLayerDecSize, l.fontScale, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered(layerText, l.layerValuePos, l.layerValueSize, l.fontScale, glm::vec3(0.60f, 0.90f, 1.0f));
    renderCentered("+1", l.btnLayerIncPos, l.btnLayerIncSize, l.fontScale, glm::vec3(0.8f, 1.0f, 0.8f));

    // Текст ряда 2: Угол
    renderCentered("-45", l.btnAngleDecPos, l.btnAngleDecSize, l.fontScale * 0.9f, glm::vec3(0.8f, 0.9f, 1.0f));
    renderCentered("УГОЛ: " + std::to_string(static_cast<int>(std::round(config.angleDeg))) + " град", l.btnAnglePos, l.btnAngleSize, l.fontScale, glm::vec3(0.95f, 0.95f, 0.7f));
    renderCentered("+45", l.btnAngleIncPos, l.btnAngleIncSize, l.fontScale * 0.9f, glm::vec3(0.8f, 0.9f, 1.0f));

    // Текст ряда 3: Скорость
    renderCentered("-", l.btnSpeedDecPos, l.btnSpeedDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered("СИЛА: " + std::to_string(static_cast<int>(std::round(config.speed))) + " px/s", l.speedValuePos, speedValSz, l.fontScale, glm::vec3(0.75f, 0.95f, 1.0f));
    renderCentered("+", l.btnSpeedIncPos, l.btnSpeedIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));

    // Текст ряда 4: Размер
    char scaleBuf[32];
    std::snprintf(scaleBuf, sizeof(scaleBuf), "РАЗМЕР: %.1fx", config.particleScale);
    renderCentered("-", l.btnScaleDecPos, l.btnScaleDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered(scaleBuf, l.scaleValuePos, speedValSz, l.fontScale, glm::vec3(1.0f, 0.95f, 0.75f));
    renderCentered("+", l.btnScaleIncPos, l.btnScaleIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));

    // Текст ряда 5: Частота
    if (config.loopContinuous) {
        renderCentered("-", l.btnPeriodDecPos, l.btnPeriodDecSize, l.fontScale * 1.1f, glm::vec3(0.4f, 0.45f, 0.5f));
        renderCentered("ЧАСТОТА: НЕПРЕРЫВНО", l.periodValuePos, speedValSz, l.fontScale * 0.92f, glm::vec3(0.5f, 0.7f, 0.55f));
        renderCentered("+", l.btnPeriodIncPos, l.btnPeriodIncSize, l.fontScale * 1.1f, glm::vec3(0.4f, 0.45f, 0.5f));
    } else {
        char freqBuf[32];
        std::snprintf(freqBuf, sizeof(freqBuf), "ЧАСТОТА: %.0f-%.0fs", config.periodMin, config.periodMax);
        renderCentered("-", l.btnPeriodDecPos, l.btnPeriodDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
        renderCentered(freqBuf, l.periodValuePos, speedValSz, l.fontScale, glm::vec3(0.85f, 0.85f, 1.0f));
        renderCentered("+", l.btnPeriodIncPos, l.btnPeriodIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));
    }

    // Текст ряда 6: Длительность
    char burstBuf[32];
    std::snprintf(burstBuf, sizeof(burstBuf), "ДЛИТ.: %.1fs", config.burstDuration);
    renderCentered("-", l.btnBurstDecPos, l.btnBurstDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered(burstBuf, l.burstValuePos, speedValSz, l.fontScale, glm::vec3(0.85f, 1.0f, 0.85f));
    renderCentered("+", l.btnBurstIncPos, l.btnBurstIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));

    // Текст ряда 7: Удалить / Закрыть
    renderCentered("УДАЛИТЬ", l.btnDeletePos, l.btnDeleteSize, l.fontScale * 0.95f, glm::vec3(1.0f, 0.5f, 0.5f));
    renderCentered("ЗАКРЫТЬ", l.btnClosePos, l.btnCloseSize, l.fontScale * 0.95f, glm::vec3(0.9f, 0.9f, 1.0f));
}

