#include "EditorDecorInspector.h"
#include "../ui/UICommon.h"
#include "../core/InputManager.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../../resources/ResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

bool EditorDecorInspector::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

glm::vec2 EditorDecorInspector::clampPanelPosition(glm::vec2 pos, glm::vec2 panelSize,
                                                   float screenW, float screenH,
                                                   float topBarH, float bottomDockH, float uiScale) {
    float marginX = 12.0f * uiScale;
    float marginY = 8.0f * uiScale;
    float minX = marginX;
    float maxX = std::max(minX, screenW - panelSize.x - marginX);
    float minY = topBarH + marginY;
    float maxY = std::max(minY, screenH - bottomDockH - panelSize.y - marginY);
    return glm::vec2(std::clamp(pos.x, minX, maxX), std::clamp(pos.y, minY, maxY));
}

DecorInspectorLayout EditorDecorInspector::getLayout(const DecorationConfig& config, int decorIndex,
                                                     glm::vec2 decorScreenPos,
                                                     float screenW, float screenH,
                                                     float topBarH, float bottomDockH) const {
    DecorInspectorLayout l;

    bool isPuddle = (config.type == "puddle" || config.type == "fog");
    l.isPuddle = isPuddle;

    float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));
    float w = 286.0f * s;
    float h = 336.0f * s;
    if (isPuddle) {
        h = 368.0f * s;
    }
    l.panelSize = glm::vec2(w, h);
    l.headerH = 32.0f * s;
    l.fontScale = std::clamp(0.40f * s, 0.32f, 0.60f);

    float px = 0.0f;
    float py = 0.0f;

    if (m_hasCustomPosition) {
        glm::vec2 clamped = clampPanelPosition(m_customPosition, l.panelSize,
                                               screenW, screenH, topBarH, bottomDockH, s);
        px = clamped.x;
        py = clamped.y;
        m_panelPos = clamped;
        m_lastDecorIdx = decorIndex;
    } else if (m_panelPos.x >= 0.0f && m_panelPos.y >= 0.0f && m_lastDecorIdx == decorIndex) {
        glm::vec2 clamped = clampPanelPosition(m_panelPos, l.panelSize,
                                               screenW, screenH, topBarH, bottomDockH, s);
        px = clamped.x;
        py = clamped.y;
    } else {
        float minX = 12.0f * s;
        float maxX = std::max(minX, screenW - w - 12.0f * s);
        px = decorScreenPos.x + 60.0f * s;
        if (px > maxX) {
            px = decorScreenPos.x - w - 60.0f * s;
        }
        py = decorScreenPos.y - h * 0.5f;

        glm::vec2 clamped = clampPanelPosition(glm::vec2(px, py), l.panelSize,
                                               screenW, screenH, topBarH, bottomDockH, s);
        px = clamped.x;
        py = clamped.y;
        m_panelPos = clamped;
        m_lastDecorIdx = decorIndex;
    }
    l.panelPos = glm::vec2(px, py);

    float crossW = 24.0f * s;
    float crossH = 22.0f * s;
    l.btnCloseCrossPos = glm::vec2(px + w - crossW - 6.0f * s, py + (l.headerH - crossH) * 0.5f);
    l.btnCloseCrossSize = glm::vec2(crossW, crossH);

    float padX = 10.0f * s;
    float startY = py + l.headerH + 8.0f * s;
    float rowH = 24.0f * s;
    float rowGap = 7.0f * s;
    float rowW = w - padX * 2.0f;

    // Ряд 1: Тип [<] [ Название ] [>]
    float y1 = startY;
    float arrowW = 28.0f * s;
    float typeCenterW = rowW - arrowW * 2.0f - 8.0f * s;
    l.btnTypePrevPos = glm::vec2(px + padX, y1);
    l.btnTypePrevSize = glm::vec2(arrowW, rowH);
    l.btnTypePos = glm::vec2(px + padX + arrowW + 4.0f * s, y1);
    l.btnTypeSize = glm::vec2(typeCenterW, rowH);
    l.btnTypeNextPos = glm::vec2(px + padX + arrowW + typeCenterW + 8.0f * s, y1);
    l.btnTypeNextSize = glm::vec2(arrowW, rowH);

    // Ряд 2: Слой (zIndex) [-1] [ СЛОЙ: N ] [+1]
    float yLayer = y1 + rowH + rowGap;
    float btnStepW = 32.0f * s;
    float valW = rowW - btnStepW * 2.0f - 8.0f * s;
    l.btnLayerDecPos = glm::vec2(px + padX, yLayer);
    l.btnLayerDecSize = glm::vec2(btnStepW, rowH);
    l.layerValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, yLayer);
    l.layerValueSize = glm::vec2(valW, rowH);
    l.btnLayerIncPos = glm::vec2(px + padX + btnStepW + valW + 8.0f * s, yLayer);
    l.btnLayerIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 3: Общий масштаб [-] [ 1.0x ] [+]
    float y2 = yLayer + rowH + rowGap;
    l.btnScaleDecPos = glm::vec2(px + padX, y2);
    l.btnScaleDecSize = glm::vec2(btnStepW, rowH);
    l.scaleValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y2);
    l.btnScaleIncPos = glm::vec2(px + padX + btnStepW + valW + 8.0f * s, y2);
    l.btnScaleIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 4: Масштаб по ширине X [-] [ ШИРИНА X: 1.0x ] [+]
    float y3 = y2 + rowH + rowGap;
    l.btnScaleXDecPos = glm::vec2(px + padX, y3);
    l.btnScaleXDecSize = glm::vec2(btnStepW, rowH);
    l.scaleXValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y3);
    l.btnScaleXIncPos = glm::vec2(px + padX + btnStepW + valW + 8.0f * s, y3);
    l.btnScaleXIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 5: Масштаб по высоте Y [-] [ ВЫСОТА Y: 1.0x ] [+]
    float y4 = y3 + rowH + rowGap;
    l.btnScaleYDecPos = glm::vec2(px + padX, y4);
    l.btnScaleYDecSize = glm::vec2(btnStepW, rowH);
    l.scaleYValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y4);
    l.btnScaleYIncPos = glm::vec2(px + padX + btnStepW + valW + 8.0f * s, y4);
    l.btnScaleYIncSize = glm::vec2(btnStepW, rowH);

    float curY = y4;

    if (isPuddle) {
        // Ряд 5.5: Темнота / Opacity
        curY += rowH + rowGap;
        l.btnOpacityDecPos = glm::vec2(px + padX, curY);
        l.btnOpacityDecSize = glm::vec2(btnStepW, rowH);
        l.opacityValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, curY);
        l.btnOpacityIncPos = glm::vec2(px + padX + btnStepW + valW + 8.0f * s, curY);
        l.btnOpacityIncSize = glm::vec2(btnStepW, rowH);
    }

    // Ряд 6: Угол поворота [-45°] [ 0° ] [+45°]
    curY += rowH + rowGap;
    float btnAngleStepW = 38.0f * s;
    float angleCenterW = rowW - btnAngleStepW * 2.0f - 8.0f * s;
    l.btnAngleDecPos = glm::vec2(px + padX, curY);
    l.btnAngleDecSize = glm::vec2(btnAngleStepW, rowH);
    l.btnAnglePos = glm::vec2(px + padX + btnAngleStepW + 4.0f * s, curY);
    l.btnAngleSize = glm::vec2(angleCenterW, rowH);
    l.btnAngleIncPos = glm::vec2(px + padX + btnAngleStepW + angleCenterW + 8.0f * s, curY);
    l.btnAngleIncSize = glm::vec2(btnAngleStepW, rowH);

    // Ряд 7: Сносится башней
    curY += rowH + rowGap;
    l.btnDestructiblePos = glm::vec2(px + padX, curY);
    l.btnDestructibleSize = glm::vec2(rowW, rowH);

    // Ряд 8: Кнопки Удалить / Закрыть
    curY += rowH + 10.0f * s;
    float botBtnW = (rowW - 6.0f * s) * 0.5f;
    l.btnDeletePos = glm::vec2(px + padX, curY);
    l.btnDeleteSize = glm::vec2(botBtnW, 26.0f * s);
    l.btnClosePos = glm::vec2(px + padX + botBtnW + 6.0f * s, curY);
    l.btnCloseSize = glm::vec2(botBtnW, 26.0f * s);

    return l;
}

bool EditorDecorInspector::handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                                       DecorationConfig& config, int decorIndex, glm::vec2 decorScreenPos,
                                       float screenW, float screenH, float topBarH, float bottomDockH,
                                       const std::function<void()>& onChanged,
                                       const std::function<void()>& onDelete,
                                       const std::function<void()>& onClose,
                                       const std::function<void(const std::string&, const glm::vec4&)>& showToast) {
    DecorInspectorLayout l = getLayout(config, decorIndex, decorScreenPos, screenW, screenH, topBarH, bottomDockH);

    if (m_isDragging) {
        if (!leftDown) {
            m_isDragging = false;
        } else {
            float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));
            glm::vec2 newPos = clampPanelPosition(mousePos + m_dragOffset, l.panelSize,
                                                  screenW, screenH, topBarH, bottomDockH, s);
            m_panelPos = newPos;
            m_customPosition = newPos;
            m_hasCustomPosition = true;
        }
        return true;
    }

    if (!isPointInRect(mousePos, l.panelPos, l.panelSize)) {
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
            if (showToast) showToast("Слой декора (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
            return true;
        } else if (isPointInRect(mousePos, l.btnScaleDecPos, l.btnScaleDecSize) ||
                   isPointInRect(mousePos, l.scaleValuePos, l.btnScaleIncPos - l.scaleValuePos + l.btnScaleIncSize) ||
                   isPointInRect(mousePos, l.btnScaleIncPos, l.btnScaleIncSize)) {
            float delta = (scrollY > 0.0f) ? 0.1f : -0.1f;
            config.scale = std::clamp(std::round((config.scale + delta) * 10.0f) / 10.0f, 0.3f, 3.0f);
            if (onChanged) onChanged();
            return true;
        } else if (isPointInRect(mousePos, l.btnScaleXDecPos, l.btnScaleXDecSize) ||
                   isPointInRect(mousePos, l.scaleXValuePos, l.btnScaleXIncPos - l.scaleXValuePos + l.btnScaleXIncSize) ||
                   isPointInRect(mousePos, l.btnScaleXIncPos, l.btnScaleXIncSize)) {
            float delta = (scrollY > 0.0f) ? 0.1f : -0.1f;
            config.scaleX = std::clamp(std::round((config.scaleX + delta) * 10.0f) / 10.0f, 0.3f, 3.0f);
            if (onChanged) onChanged();
            return true;
        } else if (isPointInRect(mousePos, l.btnScaleYDecPos, l.btnScaleYDecSize) ||
                   isPointInRect(mousePos, l.scaleYValuePos, l.btnScaleYIncPos - l.scaleYValuePos + l.btnScaleYIncSize) ||
                   isPointInRect(mousePos, l.btnScaleYIncPos, l.btnScaleYIncSize)) {
            float delta = (scrollY > 0.0f) ? 0.1f : -0.1f;
            config.scaleY = std::clamp(std::round((config.scaleY + delta) * 10.0f) / 10.0f, 0.3f, 3.0f);
            if (onChanged) onChanged();
            return true;
        } else if (isPointInRect(mousePos, l.btnAngleDecPos, l.btnAngleDecSize) ||
                   isPointInRect(mousePos, l.btnAnglePos, l.btnAngleSize) ||
                   isPointInRect(mousePos, l.btnAngleIncPos, l.btnAngleIncSize)) {
            float delta = (scrollY > 0.0f) ? 15.0f : -15.0f;
            config.rotation = std::fmod(config.rotation + delta + 360.0f, 360.0f);
            if (onChanged) onChanged();
            return true;
        }
    }

    if (!leftDown || wasLeftDown) {
        return true;
    }

    if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize) ||
        isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize)) {
        m_isDragging = false;
        if (onClose) {
            onClose();
        }
        return true;
    }

    glm::vec2 headerSz(l.panelSize.x, l.headerH);
    if (isPointInRect(mousePos, l.panelPos, headerSz)) {
        m_isDragging = true;
        m_dragOffset = l.panelPos - mousePos;
        return true;
    }

    if (isPointInRect(mousePos, l.btnDeletePos, l.btnDeleteSize)) {
        m_isDragging = false;
        m_lastDecorIdx = -1;
        std::string dName = config.type;
        if (onDelete) {
            onDelete();
        }
        if (showToast) {
            showToast("Декор удален: " + dName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
        }
        return true;
    }

    static const std::vector<std::string> s_decorTypes = {
        "bush", "grass_tuft", "grass_field", "flower", "stone", "helmet", "pickaxe", "puddle", "crack", "fog",
        "wooden_crate", "broken_mercury_cart", "scrap_debris"
    };

    auto cycleType = [&](int dir) {
        int idx = 0;
        for (size_t i = 0; i < s_decorTypes.size(); ++i) {
            if (s_decorTypes[i] == config.type) {
                idx = static_cast<int>(i);
                break;
            }
        }
        idx = (idx + dir + static_cast<int>(s_decorTypes.size())) % static_cast<int>(s_decorTypes.size());
        config.type = s_decorTypes[idx];
        if (onChanged) onChanged();
    };

    if (isPointInRect(mousePos, l.btnTypePrevPos, l.btnTypePrevSize)) {
        cycleType(-1);
        return true;
    }
    if (isPointInRect(mousePos, l.btnTypeNextPos, l.btnTypeNextSize) || isPointInRect(mousePos, l.btnTypePos, l.btnTypeSize)) {
        cycleType(+1);
        return true;
    }

    // Слой (zIndex)
    if (isPointInRect(mousePos, l.btnLayerDecPos, l.btnLayerDecSize)) {
        config.zIndex = std::clamp(config.zIndex - 1, -10, 10);
        if (onChanged) onChanged();
        if (showToast) showToast("Слой декора (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        return true;
    }
    if (isPointInRect(mousePos, l.btnLayerIncPos, l.btnLayerIncSize)) {
        config.zIndex = std::clamp(config.zIndex + 1, -10, 10);
        if (onChanged) onChanged();
        if (showToast) showToast("Слой декора (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        return true;
    }
    if (isPointInRect(mousePos, l.layerValuePos, l.layerValueSize)) {
        config.zIndex = (config.zIndex != 0) ? 0 : 1;
        if (onChanged) onChanged();
        if (showToast) showToast("Слой декора (zIndex): " + std::to_string(config.zIndex), glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        return true;
    }

    // Общий масштаб
    if (isPointInRect(mousePos, l.btnScaleDecPos, l.btnScaleDecSize)) {
        config.scale = std::max(0.3f, std::round((config.scale - 0.1f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnScaleIncPos, l.btnScaleIncSize)) {
        config.scale = std::min(3.0f, std::round((config.scale + 0.1f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.scaleValuePos, l.btnScaleIncPos - l.scaleValuePos + l.btnScaleIncSize)) {
        config.scale = 1.0f;
        if (onChanged) onChanged();
        return true;
    }

    // Ширина X
    if (isPointInRect(mousePos, l.btnScaleXDecPos, l.btnScaleXDecSize)) {
        config.scaleX = std::max(0.3f, std::round((config.scaleX - 0.1f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnScaleXIncPos, l.btnScaleXIncSize)) {
        config.scaleX = std::min(3.0f, std::round((config.scaleX + 0.1f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.scaleXValuePos, l.btnScaleXIncPos - l.scaleXValuePos + l.btnScaleXIncSize)) {
        config.scaleX = 1.0f;
        if (onChanged) onChanged();
        return true;
    }

    // Высота Y
    if (isPointInRect(mousePos, l.btnScaleYDecPos, l.btnScaleYDecSize)) {
        config.scaleY = std::max(0.3f, std::round((config.scaleY - 0.1f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnScaleYIncPos, l.btnScaleYIncSize)) {
        config.scaleY = std::min(3.0f, std::round((config.scaleY + 0.1f) * 10.0f) / 10.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.scaleYValuePos, l.btnScaleYIncPos - l.scaleYValuePos + l.btnScaleYIncSize)) {
        config.scaleY = 1.0f;
        if (onChanged) onChanged();
        return true;
    }

    if (l.isPuddle) {
        // Темнота
        if (isPointInRect(mousePos, l.btnOpacityDecPos, l.btnOpacityDecSize)) {
            config.opacity = std::max(0.10f, std::round((config.opacity - 0.05f) * 100.0f) / 100.0f);
            if (onChanged) onChanged();
            return true;
        }
        if (isPointInRect(mousePos, l.btnOpacityIncPos, l.btnOpacityIncSize)) {
            config.opacity = std::min(1.00f, std::round((config.opacity + 0.05f) * 100.0f) / 100.0f);
            if (onChanged) onChanged();
            return true;
        }
    }

    // Угол
    if (isPointInRect(mousePos, l.btnAngleDecPos, l.btnAngleDecSize)) {
        config.rotation = std::fmod(config.rotation - 45.0f + 360.0f, 360.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnAngleIncPos, l.btnAngleIncSize)) {
        config.rotation = std::fmod(config.rotation + 45.0f, 360.0f);
        if (onChanged) onChanged();
        return true;
    }
    if (isPointInRect(mousePos, l.btnAnglePos, l.btnAngleSize)) {
        config.rotation = std::fmod(config.rotation + 45.0f, 360.0f);
        if (onChanged) onChanged();
        return true;
    }

    // Сносится башней
    if (isPointInRect(mousePos, l.btnDestructiblePos, l.btnDestructibleSize)) {
        config.destructible = !config.destructible;
        if (onChanged) onChanged();
        return true;
    }

    return true;
}

void EditorDecorInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                  const DecorationConfig& config, int decorIndex, glm::vec2 decorScreenPos,
                                  float screenW, float screenH, float topBarH, float bottomDockH,
                                  glm::vec2 mousePos) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    render(renderer, textRenderer, whiteTex, config, decorIndex, decorScreenPos, screenW, screenH, topBarH, bottomDockH, mousePos);
}

void EditorDecorInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                  const Texture2D* whiteTexture,
                                  const DecorationConfig& config, int decorIndex, glm::vec2 decorScreenPos,
                                  float screenW, float screenH, float topBarH, float bottomDockH,
                                  glm::vec2 mousePos) {
    if (!renderer || !whiteTexture) return;

    DecorInspectorLayout l = getLayout(config, decorIndex, decorScreenPos, screenW, screenH, topBarH, bottomDockH);
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

    // Шапка панели (с подсветкой при наведении/перетаскивании)
    glm::vec2 headerSize(l.panelSize.x, l.headerH);
    bool headerHovered = isHover(l.panelPos, headerSize) && !isHover(l.btnCloseCrossPos, l.btnCloseCrossSize);
    glm::vec4 headerBg(0.12f, 0.16f, 0.22f, 0.98f);
    if (m_isDragging) {
        headerBg = glm::vec4(0.18f, 0.25f, 0.36f, 0.98f);
    } else if (headerHovered) {
        headerBg = glm::vec4(0.15f, 0.21f, 0.30f, 0.98f);
    }
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, headerSize, 0.0f, headerBg);
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
        glm::vec4(0.20f, 0.12f, 0.12f, 0.8f),
        glm::vec4(0.60f, 0.15f, 0.15f, 1.0f),
        glm::vec4(0.80f, 0.30f, 0.30f, 0.9f)
    );

    glm::vec4 btnBaseBg(0.14f, 0.18f, 0.25f, 0.85f);
    glm::vec4 btnHovBg(0.22f, 0.30f, 0.42f, 1.0f);
    glm::vec4 btnBorder(0.30f, 0.42f, 0.58f, 0.8f);

    // Ряд 1: Тип
    drawBtn(l.btnTypePrevPos, l.btnTypePrevSize, btnBaseBg, btnHovBg, btnBorder);
    drawBtn(l.btnTypePos, l.btnTypeSize, glm::vec4(0.11f, 0.15f, 0.22f, 0.9f), glm::vec4(0.18f, 0.24f, 0.34f, 1.0f), btnBorder);
    drawBtn(l.btnTypeNextPos, l.btnTypeNextSize, btnBaseBg, btnHovBg, btnBorder);

    // Ряд 2: Слой (zIndex)
    float rowW = l.panelSize.x - 20.0f * s;
    float btnStepW = 32.0f * s;
    float valW = rowW - btnStepW * 2.0f - 8.0f * s;
    glm::vec2 valSz(valW, 24.0f * s);

    drawBtn(l.btnLayerDecPos, l.btnLayerDecSize, btnBaseBg, btnHovBg, btnBorder);
    drawBtn(l.layerValuePos, valSz, glm::vec4(0.10f, 0.14f, 0.20f, 0.9f), glm::vec4(0.14f, 0.20f, 0.28f, 1.0f), btnBorder);
    drawBtn(l.btnLayerIncPos, l.btnLayerIncSize, btnBaseBg, btnHovBg, btnBorder);

    // Ряд 3: Общий масштаб
    drawBtn(l.btnScaleDecPos, l.btnScaleDecSize, btnBaseBg, btnHovBg, btnBorder);
    drawBtn(l.scaleValuePos, valSz, glm::vec4(0.10f, 0.13f, 0.18f, 0.9f), glm::vec4(0.14f, 0.18f, 0.24f, 1.0f), btnBorder);
    drawBtn(l.btnScaleIncPos, l.btnScaleIncSize, btnBaseBg, btnHovBg, btnBorder);

    // Ряд 4: Масштаб по ширине X
    drawBtn(l.btnScaleXDecPos, l.btnScaleXDecSize, btnBaseBg, btnHovBg, btnBorder);
    drawBtn(l.scaleXValuePos, valSz, glm::vec4(0.10f, 0.13f, 0.18f, 0.9f), glm::vec4(0.14f, 0.18f, 0.24f, 1.0f), btnBorder);
    drawBtn(l.btnScaleXIncPos, l.btnScaleXIncSize, btnBaseBg, btnHovBg, btnBorder);

    // Ряд 5: Масштаб по высоте Y
    drawBtn(l.btnScaleYDecPos, l.btnScaleYDecSize, btnBaseBg, btnHovBg, btnBorder);
    drawBtn(l.scaleYValuePos, valSz, glm::vec4(0.10f, 0.13f, 0.18f, 0.9f), glm::vec4(0.14f, 0.18f, 0.24f, 1.0f), btnBorder);
    drawBtn(l.btnScaleYIncPos, l.btnScaleYIncSize, btnBaseBg, btnHovBg, btnBorder);

    if (l.isPuddle) {
        // Ряд 5.5: Темнота / Opacity
        drawBtn(l.btnOpacityDecPos, l.btnOpacityDecSize, btnBaseBg, btnHovBg, btnBorder);
        drawBtn(l.opacityValuePos, valSz, glm::vec4(0.10f, 0.13f, 0.18f, 0.9f), glm::vec4(0.14f, 0.18f, 0.24f, 1.0f), btnBorder);
        drawBtn(l.btnOpacityIncPos, l.btnOpacityIncSize, btnBaseBg, btnHovBg, btnBorder);
    }

    // Ряд 6: Угол
    drawBtn(l.btnAngleDecPos, l.btnAngleDecSize, btnBaseBg, btnHovBg, btnBorder);
    drawBtn(l.btnAnglePos, l.btnAngleSize, glm::vec4(0.12f, 0.16f, 0.22f, 0.9f), glm::vec4(0.18f, 0.24f, 0.32f, 1.0f), btnBorder);
    drawBtn(l.btnAngleIncPos, l.btnAngleIncSize, btnBaseBg, btnHovBg, btnBorder);

    // Ряд 7: Сносится башней
    glm::vec4 destrBg = config.destructible ? glm::vec4(0.14f, 0.28f, 0.18f, 0.9f) : glm::vec4(0.28f, 0.16f, 0.14f, 0.9f);
    glm::vec4 destrHov = config.destructible ? glm::vec4(0.20f, 0.40f, 0.25f, 1.0f) : glm::vec4(0.40f, 0.22f, 0.20f, 1.0f);
    glm::vec4 destrBdr = config.destructible ? glm::vec4(0.35f, 0.65f, 0.40f, 0.9f) : glm::vec4(0.65f, 0.35f, 0.30f, 0.9f);
    drawBtn(l.btnDestructiblePos, l.btnDestructibleSize, destrBg, destrHov, destrBdr);

    // Ряд 8: Удалить / Закрыть
    drawBtn(l.btnDeletePos, l.btnDeleteSize,
        glm::vec4(0.35f, 0.12f, 0.12f, 0.9f),
        glm::vec4(0.55f, 0.16f, 0.16f, 1.0f),
        glm::vec4(0.75f, 0.30f, 0.30f, 0.9f)
    );
    drawBtn(l.btnClosePos, l.btnCloseSize, btnBaseBg, btnHovBg, btnBorder);

    renderer->flush();

    if (!textRenderer) return;

    auto renderCentered = [textRenderer](const std::string& text, glm::vec2 pos, glm::vec2 sz, float scale, glm::vec3 col) {
        float tw = textRenderer->CalculateTextWidth(text, scale);
        float th = scale * 26.0f;
        float tx = pos.x + (sz.x - tw) * 0.5f;
        float ty = pos.y + (sz.y - th) * 0.5f + 1.0f;
        textRenderer->RenderText(text, tx, ty, scale, col);
    };

    // Заголовок
    std::string title = "ИНСПЕКТОР ДЕКОРА #" + std::to_string(config.id);
    textRenderer->RenderText(title, l.panelPos.x + 12.0f * s, l.panelPos.y + 7.0f * s, l.fontScale * 1.05f, glm::vec3(0.95f, 0.96f, 1.0f));

    // Крестик [X]
    renderCentered("X", l.btnCloseCrossPos, l.btnCloseCrossSize, l.fontScale, glm::vec3(1.0f, 0.8f, 0.8f));

    // Текст ряда 1: Тип
    std::string friendlyName = config.type;
    if (config.type == "bush") friendlyName = "Куст";
    else if (config.type == "grass_tuft") friendlyName = "Пучок травы";
    else if (config.type == "grass_field") friendlyName = "Поле травы";
    else if (config.type == "flower") friendlyName = "Цветы";
    else if (config.type == "stone") friendlyName = "Камень";
    else if (config.type == "helmet") friendlyName = "Каска";
    else if (config.type == "pickaxe") friendlyName = "Кирка";
    else if (config.type == "puddle") friendlyName = "Лужа";
    else if (config.type == "crack") friendlyName = "Трещина";
    else if (config.type == "fog") friendlyName = "Туман";
    else if (config.type == "wooden_crate") friendlyName = "Ящик";
    else if (config.type == "broken_mercury_cart") friendlyName = "Вагонетка";
    else if (config.type == "scrap_debris") friendlyName = "Куча мусора";

    renderCentered("<", l.btnTypePrevPos, l.btnTypePrevSize, l.fontScale * 1.1f, glm::vec3(0.8f, 0.9f, 1.0f));
    renderCentered("ТИП: " + friendlyName, l.btnTypePos, l.btnTypeSize, l.fontScale, glm::vec3(0.6f, 0.9f, 1.0f));
    renderCentered(">", l.btnTypeNextPos, l.btnTypeNextSize, l.fontScale * 1.1f, glm::vec3(0.8f, 0.9f, 1.0f));

    // Текст ряда 2: Слой (zIndex)
    std::string layerText = std::string("СЛОЙ: ") + (config.zIndex > 0 ? "+" : "") + std::to_string(config.zIndex);
    renderCentered("-1", l.btnLayerDecPos, l.btnLayerDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered(layerText, l.layerValuePos, valSz, l.fontScale, glm::vec3(0.60f, 0.90f, 1.0f));
    renderCentered("+1", l.btnLayerIncPos, l.btnLayerIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));

    // Текст ряда 3: Общий масштаб
    char scaleBuf[32];
    std::snprintf(scaleBuf, sizeof(scaleBuf), "РАЗМЕР: %.1fx", config.scale);
    renderCentered("-", l.btnScaleDecPos, l.btnScaleDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered(scaleBuf, l.scaleValuePos, valSz, l.fontScale, glm::vec3(1.0f, 0.95f, 0.75f));
    renderCentered("+", l.btnScaleIncPos, l.btnScaleIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));

    // Текст ряда 4: Ширина X
    char sxBuf[32];
    std::snprintf(sxBuf, sizeof(sxBuf), "ШИРИНА X: %.1fx", config.scaleX);
    renderCentered("-", l.btnScaleXDecPos, l.btnScaleXDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered(sxBuf, l.scaleXValuePos, valSz, l.fontScale, glm::vec3(0.75f, 0.95f, 1.0f));
    renderCentered("+", l.btnScaleXIncPos, l.btnScaleXIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));

    // Текст ряда 5: Высота Y
    char syBuf[32];
    std::snprintf(syBuf, sizeof(syBuf), "ВЫСОТА Y: %.1fx", config.scaleY);
    renderCentered("-", l.btnScaleYDecPos, l.btnScaleYDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
    renderCentered(syBuf, l.scaleYValuePos, valSz, l.fontScale, glm::vec3(0.75f, 0.95f, 1.0f));
    renderCentered("+", l.btnScaleYIncPos, l.btnScaleYIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));

    if (l.isPuddle) {
        // Текст ряда 5.5: Темнота (Opacity)
        char opBuf[32];
        std::snprintf(opBuf, sizeof(opBuf), "ТЕМНОТА: %d%%", static_cast<int>(std::round(config.opacity * 100.0f)));
        renderCentered("-", l.btnOpacityDecPos, l.btnOpacityDecSize, l.fontScale * 1.1f, glm::vec3(1.0f, 0.8f, 0.8f));
        renderCentered(opBuf, l.opacityValuePos, valSz, l.fontScale, glm::vec3(0.65f, 0.90f, 0.85f));
        renderCentered("+", l.btnOpacityIncPos, l.btnOpacityIncSize, l.fontScale * 1.1f, glm::vec3(0.8f, 1.0f, 0.8f));
    }

    // Текст ряда 6: Угол
    renderCentered("-45", l.btnAngleDecPos, l.btnAngleDecSize, l.fontScale * 0.9f, glm::vec3(0.8f, 0.9f, 1.0f));
    renderCentered("УГОЛ: " + std::to_string(static_cast<int>(std::round(config.rotation))) + "°", l.btnAnglePos, l.btnAngleSize, l.fontScale, glm::vec3(0.95f, 0.95f, 0.7f));
    renderCentered("+45", l.btnAngleIncPos, l.btnAngleIncSize, l.fontScale * 0.9f, glm::vec3(0.8f, 0.9f, 1.0f));

    // Текст ряда 7: Сносится башней
    std::string destrText = config.destructible ? "СНОСИТСЯ БАШНЕЙ: ДА" : "СНОСИТСЯ БАШНЕЙ: НЕТ";
    glm::vec3 destrCol = config.destructible ? glm::vec3(0.6f, 1.0f, 0.65f) : glm::vec3(1.0f, 0.65f, 0.6f);
    renderCentered(destrText, l.btnDestructiblePos, l.btnDestructibleSize, l.fontScale, destrCol);

    // Текст ряда 8: Удалить / Закрыть
    renderCentered("УДАЛИТЬ", l.btnDeletePos, l.btnDeleteSize, l.fontScale * 0.95f, glm::vec3(1.0f, 0.5f, 0.5f));
    renderCentered("ЗАКРЫТЬ", l.btnClosePos, l.btnCloseSize, l.fontScale * 0.95f, glm::vec3(0.9f, 0.9f, 1.0f));
}

