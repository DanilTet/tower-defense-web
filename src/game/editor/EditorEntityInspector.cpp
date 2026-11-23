#include "EditorEntityInspector.h"
#include "EditorToolbarUI.h"
#include "../ui/UICommon.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../../resources/ResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

bool EditorEntityInspector::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

EntityInspectorLayout EditorEntityInspector::getLayout(EditorEntityType type, int entityIndex,
                                                       glm::vec2 entityScreenPos,
                                                       float screenW, float screenH,
                                                       float topBarH, float bottomDockH) const {
    EntityInspectorLayout l;

    float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));
    float w = 270.0f * s;
    float h = 186.0f * s;
    l.panelSize = glm::vec2(w, h);
    l.headerH = 32.0f * s;
    l.fontScale = std::clamp(0.40f * s, 0.32f, 0.60f);

    float minX = 12.0f * s;
    float maxX = std::max(minX, screenW - w - 12.0f * s);
    float minY = topBarH + 8.0f * s;
    float maxY = std::max(minY, screenH - bottomDockH - h - 8.0f * s);

    float px = 0.0f;
    float py = 0.0f;

    if (m_panelPos.x >= 0.0f && m_panelPos.y >= 0.0f && m_lastEntityIdx == entityIndex && m_lastEntityType == type) {
        px = std::clamp(m_panelPos.x, minX, maxX);
        py = std::clamp(m_panelPos.y, minY, maxY);
    } else {
        px = entityScreenPos.x + 50.0f * s;
        if (px > maxX) px = entityScreenPos.x - w - 50.0f * s;
        py = entityScreenPos.y - h * 0.5f;

        px = std::clamp(px, minX, maxX);
        py = std::clamp(py, minY, maxY);

        m_panelPos = glm::vec2(px, py);
        m_lastEntityIdx = entityIndex;
        m_lastEntityType = type;
    }
    l.panelPos = glm::vec2(px, py);

    float crossW = 24.0f * s;
    float crossH = 22.0f * s;
    l.btnCloseCrossPos = glm::vec2(px + w - crossW - 6.0f * s, py + (l.headerH - crossH) * 0.5f);
    l.btnCloseCrossSize = glm::vec2(crossW, crossH);

    float padX = 10.0f * s;
    float startY = py + l.headerH + 8.0f * s;
    float rowH = 26.0f * s;
    float rowGap = 7.0f * s;
    float rowW = w - padX * 2.0f;

    // Ряд 1: ID сущности [-] [ ID: ... ] [+]
    float y1 = startY;
    float btnStepW = 32.0f * s;
    l.btnIdDecPos = glm::vec2(px + padX, y1);
    l.btnIdDecSize = glm::vec2(btnStepW, rowH);
    l.idValuePos = glm::vec2(px + padX + btnStepW + 4.0f * s, y1);
    l.idValueSize = glm::vec2(rowW - btnStepW * 2.0f - 8.0f * s, rowH);
    l.btnIdIncPos = glm::vec2(px + padX + rowW - btnStepW, y1);
    l.btnIdIncSize = glm::vec2(btnStepW, rowH);

    // Ряд 2: В игре: СКРЫТ / ВИДИМ
    float y2 = y1 + rowH + rowGap;
    l.btnVisibilityPos = glm::vec2(px + padX, y2);
    l.btnVisibilitySize = glm::vec2(rowW, rowH);

    // Ряд 3: Плавный спавн / растворение: ВКЛ / ВЫКЛ
    float y3 = y2 + rowH + rowGap;
    l.btnFadePos = glm::vec2(px + padX, y3);
    l.btnFadeSize = glm::vec2(rowW, rowH);

    // Ряд 4: Кнопки [ Удалить объект ] и [ Закрыть ]
    float y4 = y3 + rowH + rowGap;
    float actBtnW = (rowW - 8.0f * s) * 0.5f;
    l.btnDeletePos = glm::vec2(px + padX, y4);
    l.btnDeleteSize = glm::vec2(actBtnW, rowH);
    l.btnClosePos = glm::vec2(px + padX + actBtnW + 8.0f * s, y4);
    l.btnCloseSize = glm::vec2(actBtnW, rowH);

    return l;
}

bool EditorEntityInspector::handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                                        SpawnerData& spawner, int entityIndex, glm::vec2 entityScreenPos,
                                        float screenW, float screenH, float topBarH, float bottomDockH,
                                        const std::function<void()>& onChanged,
                                        const std::function<void()>& onDelete,
                                        const std::function<void()>& onClose,
                                        const std::function<void(const std::string&, const glm::vec4&)>& showToast) {
    EntityInspectorLayout l = getLayout(EditorEntityType::Spawner, entityIndex, entityScreenPos, screenW, screenH, topBarH, bottomDockH);

    if (m_isDragging) {
        if (!leftDown) {
            m_isDragging = false;
        } else {
            float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));
            float minX = 12.0f * s;
            float maxX = std::max(minX, screenW - l.panelSize.x - 12.0f * s);
            float minY = topBarH + 8.0f * s;
            float maxY = std::max(minY, screenH - bottomDockH - l.panelSize.y - 8.0f * s);
            glm::vec2 newPos = mousePos + m_dragOffset;
            newPos.x = std::clamp(newPos.x, minX, maxX);
            newPos.y = std::clamp(newPos.y, minY, maxY);
            m_panelPos = newPos;
        }
        return true;
    }

    if (!isPointInRect(mousePos, l.panelPos, l.panelSize)) {
        return false;
    }

    if (!leftDown || wasLeftDown) {
        return true;
    }

    // Перетаскивание за заголовок
    glm::vec2 headerSz(l.panelSize.x - l.btnCloseCrossSize.x - 12.0f, l.headerH);
    if (isPointInRect(mousePos, l.panelPos, headerSz)) {
        m_isDragging = true;
        m_dragOffset = l.panelPos - mousePos;
        return true;
    }

    // Закрытие крестиком в шапке или кнопкой [ Закрыть ]
    if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize) ||
        isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize)) {
        m_isDragging = false;
        if (onClose) onClose();
        return true;
    }

    // Удаление
    if (isPointInRect(mousePos, l.btnDeletePos, l.btnDeleteSize)) {
        m_isDragging = false;
        if (onDelete) onDelete();
        return true;
    }

    // Смена ID: стрелочка [-]
    if (isPointInRect(mousePos, l.btnIdDecPos, l.btnIdDecSize)) {
        int curId = spawner.targetBaseIndex;
        if (curId > -1) {
            spawner.targetBaseIndex = curId - 1;
        } else {
            spawner.targetBaseIndex = -1;
        }
        spawner.groupId = spawner.targetBaseIndex;
        if (onChanged) onChanged();
        if (showToast) {
            std::string idText = (spawner.targetBaseIndex == -1) ? "Auto (Ближайшая)" : ("#" + std::to_string(spawner.targetBaseIndex));
            showToast("Спавнер: Целевая база " + idText, glm::vec4(0.95f, 0.85f, 0.2f, 1.0f));
        }
        return true;
    }

    // Смена ID: стрелочка [+]
    if (isPointInRect(mousePos, l.btnIdIncPos, l.btnIdIncSize)) {
        int curId = spawner.targetBaseIndex;
        if (curId < 9) {
            spawner.targetBaseIndex = curId + 1;
        }
        spawner.groupId = spawner.targetBaseIndex;
        if (onChanged) onChanged();
        if (showToast) {
            std::string idText = (spawner.targetBaseIndex == -1) ? "Auto (Ближайшая)" : ("#" + std::to_string(spawner.targetBaseIndex));
            showToast("Спавнер: Целевая база " + idText, glm::vec4(0.95f, 0.85f, 0.2f, 1.0f));
        }
        return true;
    }

    // Переключение visibleInGame
    if (isPointInRect(mousePos, l.btnVisibilityPos, l.btnVisibilitySize)) {
        spawner.visibleInGame = !spawner.visibleInGame;
        if (onChanged) onChanged();
        if (showToast) {
            showToast(spawner.visibleInGame ? "Спавнер: ВИДИМ в бою" : "Спавнер: СКРЫТ в бою (чистый асфальт)",
                      spawner.visibleInGame ? glm::vec4(0.3f, 0.9f, 0.4f, 1.0f) : glm::vec4(0.9f, 0.7f, 0.3f, 1.0f));
        }
        return true;
    }

    // Переключение enableFadeIn
    if (isPointInRect(mousePos, l.btnFadePos, l.btnFadeSize)) {
        spawner.enableFadeIn = !spawner.enableFadeIn;
        if (onChanged) onChanged();
        if (showToast) {
            showToast(spawner.enableFadeIn ? "Плавный спавн (Fade In): ВКЛ" : "Плавный спавн (Fade In): ВЫКЛ",
                      spawner.enableFadeIn ? glm::vec4(0.3f, 0.9f, 0.4f, 1.0f) : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        }
        return true;
    }

    return true;
}

bool EditorEntityInspector::handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                                        BaseData& base, int entityIndex, glm::vec2 entityScreenPos,
                                        float screenW, float screenH, float topBarH, float bottomDockH,
                                        const std::function<void()>& onChanged,
                                        const std::function<void()>& onDelete,
                                        const std::function<void()>& onClose,
                                        const std::function<void(const std::string&, const glm::vec4&)>& showToast) {
    EntityInspectorLayout l = getLayout(EditorEntityType::Base, entityIndex, entityScreenPos, screenW, screenH, topBarH, bottomDockH);

    if (m_isDragging) {
        if (!leftDown) {
            m_isDragging = false;
        } else {
            float s = GetUIScale(static_cast<int>(screenW), static_cast<int>(screenH));
            float minX = 12.0f * s;
            float maxX = std::max(minX, screenW - l.panelSize.x - 12.0f * s);
            float minY = topBarH + 8.0f * s;
            float maxY = std::max(minY, screenH - bottomDockH - l.panelSize.y - 8.0f * s);
            glm::vec2 newPos = mousePos + m_dragOffset;
            newPos.x = std::clamp(newPos.x, minX, maxX);
            newPos.y = std::clamp(newPos.y, minY, maxY);
            m_panelPos = newPos;
        }
        return true;
    }

    if (!isPointInRect(mousePos, l.panelPos, l.panelSize)) {
        return false;
    }

    if (!leftDown || wasLeftDown) {
        return true;
    }

    // Перетаскивание за заголовок
    glm::vec2 headerSz(l.panelSize.x - l.btnCloseCrossSize.x - 12.0f, l.headerH);
    if (isPointInRect(mousePos, l.panelPos, headerSz)) {
        m_isDragging = true;
        m_dragOffset = l.panelPos - mousePos;
        return true;
    }

    // Закрытие
    if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize) ||
        isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize)) {
        m_isDragging = false;
        if (onClose) onClose();
        return true;
    }

    // Удаление
    if (isPointInRect(mousePos, l.btnDeletePos, l.btnDeleteSize)) {
        m_isDragging = false;
        if (onDelete) onDelete();
        return true;
    }

    // Смена ID: стрелочка [-]
    if (isPointInRect(mousePos, l.btnIdDecPos, l.btnIdDecSize)) {
        if (base.id > 0) {
            base.id--;
        }
        if (onChanged) onChanged();
        if (showToast) {
            showToast("База: ID #" + std::to_string(base.id), glm::vec4(0.3f, 0.8f, 1.0f, 1.0f));
        }
        return true;
    }

    // Смена ID: стрелочка [+]
    if (isPointInRect(mousePos, l.btnIdIncPos, l.btnIdIncSize)) {
        if (base.id < 9) {
            base.id++;
        }
        if (onChanged) onChanged();
        if (showToast) {
            showToast("База: ID #" + std::to_string(base.id), glm::vec4(0.3f, 0.8f, 1.0f, 1.0f));
        }
        return true;
    }

    // Переключение visibleInGame
    if (isPointInRect(mousePos, l.btnVisibilityPos, l.btnVisibilitySize)) {
        base.visibleInGame = !base.visibleInGame;
        if (onChanged) onChanged();
        if (showToast) {
            showToast(base.visibleInGame ? "База: ВИДИМА в бою" : "База: СКРЫТА в бою",
                      base.visibleInGame ? glm::vec4(0.3f, 0.9f, 0.4f, 1.0f) : glm::vec4(0.9f, 0.7f, 0.3f, 1.0f));
        }
        return true;
    }

    // Переключение enableFadeOut
    if (isPointInRect(mousePos, l.btnFadePos, l.btnFadeSize)) {
        base.enableFadeOut = !base.enableFadeOut;
        if (onChanged) onChanged();
        if (showToast) {
            showToast(base.enableFadeOut ? "Плавный уход (Fade Out): ВКЛ" : "Плавный уход (Fade Out): ВЫКЛ",
                      base.enableFadeOut ? glm::vec4(0.3f, 0.9f, 0.4f, 1.0f) : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        }
        return true;
    }

    return true;
}

void EditorEntityInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                   const SpawnerData& spawner, int entityIndex, glm::vec2 entityScreenPos,
                                   float screenW, float screenH, float topBarH, float bottomDockH,
                                   glm::vec2 mousePos) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    render(renderer, textRenderer, whiteTex, spawner, entityIndex, entityScreenPos, screenW, screenH, topBarH, bottomDockH, mousePos);
}

void EditorEntityInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                   const BaseData& base, int entityIndex, glm::vec2 entityScreenPos,
                                   float screenW, float screenH, float topBarH, float bottomDockH,
                                   glm::vec2 mousePos) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    render(renderer, textRenderer, whiteTex, base, entityIndex, entityScreenPos, screenW, screenH, topBarH, bottomDockH, mousePos);
}

void EditorEntityInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                   const Texture2D* whiteTexture,
                                   const SpawnerData& spawner, int entityIndex, glm::vec2 entityScreenPos,
                                   float screenW, float screenH, float topBarH, float bottomDockH,
                                   glm::vec2 mousePos) {
    if (!renderer || !whiteTexture) return;

    EntityInspectorLayout l = getLayout(EditorEntityType::Spawner, entityIndex, entityScreenPos, screenW, screenH, topBarH, bottomDockH);
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

    // Тень и тело панели
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(4.0f * s, 4.0f * s), l.panelSize, 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, l.panelSize, 0.0f, glm::vec4(0.08f, 0.10f, 0.14f, 0.95f));

    // Шапка
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(l.panelSize.x, l.headerH), 0.0f, glm::vec4(0.14f, 0.16f, 0.22f, 0.98f));

    // Кайма панели
    float panelBorderT = 1.5f * s;
    glm::vec4 panelBorder(0.28f, 0.40f, 0.58f, 0.90f);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(l.panelSize.x, panelBorderT), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(0.0f, l.panelSize.y - panelBorderT), glm::vec2(l.panelSize.x, panelBorderT), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(panelBorderT, l.panelSize.y), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(l.panelSize.x - panelBorderT, 0.0f), glm::vec2(panelBorderT, l.panelSize.y), 0.0f, panelBorder);

    // Разделитель
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.panelSize.x, 1.0f * s), 0.0f, glm::vec4(0.25f, 0.35f, 0.48f, 0.8f));

    // [X] кнопка закрытия
    drawBtn(l.btnCloseCrossPos, l.btnCloseCrossSize,
        glm::vec4(0.20f, 0.12f, 0.12f, 0.8f),
        glm::vec4(0.40f, 0.16f, 0.16f, 0.95f),
        glm::vec4(0.60f, 0.25f, 0.25f, 0.9f));

    // Ряд 1: ID кнопки [-] и [+]
    drawBtn(l.btnIdDecPos, l.btnIdDecSize,
        glm::vec4(0.15f, 0.18f, 0.25f, 0.85f), glm::vec4(0.24f, 0.30f, 0.42f, 0.95f), glm::vec4(0.35f, 0.45f, 0.60f, 0.8f));
    renderer->drawSpriteRGBA(whiteTexture, l.idValuePos, l.idValueSize, 0.0f, glm::vec4(0.10f, 0.12f, 0.16f, 0.90f));
    drawBtn(l.btnIdIncPos, l.btnIdIncSize,
        glm::vec4(0.15f, 0.18f, 0.25f, 0.85f), glm::vec4(0.24f, 0.30f, 0.42f, 0.95f), glm::vec4(0.35f, 0.45f, 0.60f, 0.8f));

    // Ряд 2: В игре: СКРЫТ / ВИДИМ
    if (spawner.visibleInGame) {
        drawBtn(l.btnVisibilityPos, l.btnVisibilitySize,
            glm::vec4(0.14f, 0.32f, 0.22f, 0.90f), glm::vec4(0.20f, 0.46f, 0.30f, 0.95f), glm::vec4(0.35f, 0.75f, 0.45f, 0.9f));
    } else {
        drawBtn(l.btnVisibilityPos, l.btnVisibilitySize,
            glm::vec4(0.24f, 0.18f, 0.12f, 0.90f), glm::vec4(0.35f, 0.25f, 0.16f, 0.95f), glm::vec4(0.65f, 0.45f, 0.25f, 0.9f));
    }

    // Ряд 3: Плавный спавн: ВКЛ / ВЫКЛ
    if (spawner.enableFadeIn) {
        drawBtn(l.btnFadePos, l.btnFadeSize,
            glm::vec4(0.12f, 0.25f, 0.35f, 0.90f), glm::vec4(0.18f, 0.36f, 0.50f, 0.95f), glm::vec4(0.30f, 0.65f, 0.85f, 0.9f));
    } else {
        drawBtn(l.btnFadePos, l.btnFadeSize,
            glm::vec4(0.18f, 0.18f, 0.20f, 0.85f), glm::vec4(0.25f, 0.25f, 0.28f, 0.95f), glm::vec4(0.40f, 0.40f, 0.45f, 0.8f));
    }

    // Ряд 4: [ Удалить ] и [ Закрыть ]
    drawBtn(l.btnDeletePos, l.btnDeleteSize,
        glm::vec4(0.32f, 0.12f, 0.12f, 0.85f), glm::vec4(0.48f, 0.16f, 0.16f, 0.95f), glm::vec4(0.75f, 0.25f, 0.25f, 0.9f));
    drawBtn(l.btnClosePos, l.btnCloseSize,
        glm::vec4(0.14f, 0.18f, 0.24f, 0.85f), glm::vec4(0.22f, 0.28f, 0.38f, 0.95f), glm::vec4(0.35f, 0.45f, 0.60f, 0.8f));

    renderer->flush();

    if (!textRenderer) return;

    auto renderCentered = [&](const std::string& text, glm::vec2 pos, glm::vec2 sz, float scale, glm::vec3 col) {
        float tw = textRenderer->CalculateTextWidth(text, scale);
        float th = scale * 26.0f;
        float tx = pos.x + (sz.x - tw) * 0.5f;
        float ty = pos.y + (sz.y - th) * 0.5f + 1.0f * s;
        textRenderer->RenderText(text, tx, ty, scale, col);
    };

    // Заголовок в шапке
    std::string title = "СПАВНЕР S" + (spawner.targetBaseIndex == -1 ? ":Auto" : ">#" + std::to_string(spawner.targetBaseIndex)) +
                        " (" + std::to_string(spawner.pos.x) + "," + std::to_string(spawner.pos.y) + ")";
    float titleScale = l.fontScale * 0.95f;
    textRenderer->RenderText(title, l.panelPos.x + 10.0f * s, l.panelPos.y + (l.headerH - titleScale * 26.0f) * 0.5f + 1.0f * s,
                             titleScale, glm::vec3(0.95f, 0.85f, 0.30f));

    // Крестик [X]
    renderCentered("x", l.btnCloseCrossPos, l.btnCloseCrossSize, l.fontScale, glm::vec3(0.95f, 0.5f, 0.5f));

    // Ряд 1: [-], [ ID: ... ], [+]
    renderCentered("-", l.btnIdDecPos, l.btnIdDecSize, l.fontScale * 1.1f, glm::vec3(0.85f, 0.90f, 1.0f));
    std::string idText = (spawner.targetBaseIndex == -1) ? "Цель: Auto" : ("Цель: База #" + std::to_string(spawner.targetBaseIndex));
    renderCentered(idText, l.idValuePos, l.idValueSize, l.fontScale * 0.92f, glm::vec3(0.95f, 0.85f, 0.30f));
    renderCentered("+", l.btnIdIncPos, l.btnIdIncSize, l.fontScale * 1.1f, glm::vec3(0.85f, 0.90f, 1.0f));

    // Ряд 2: [ В игре: СКРЫТ / ВИДИМ ]
    std::string visText = spawner.visibleInGame ? "В игре: ВИДИМ (Маркер ВКЛ)" : "В игре: СКРЫТ (Асфальт)";
    glm::vec3 visCol = spawner.visibleInGame ? glm::vec3(0.40f, 0.95f, 0.55f) : glm::vec3(0.95f, 0.70f, 0.35f);
    renderCentered(visText, l.btnVisibilityPos, l.btnVisibilitySize, l.fontScale * 0.90f, visCol);

    // Ряд 3: [ Плавный спавн: ВКЛ / ВЫКЛ ]
    std::string fadeText = spawner.enableFadeIn ? "Плавный спавн (Fade In): ВКЛ" : "Плавный спавн (Fade In): ВЫКЛ";
    glm::vec3 fadeCol = spawner.enableFadeIn ? glm::vec3(0.45f, 0.85f, 1.0f) : glm::vec3(0.70f, 0.72f, 0.78f);
    renderCentered(fadeText, l.btnFadePos, l.btnFadeSize, l.fontScale * 0.90f, fadeCol);

    // Ряд 4: [ Удалить ] и [ Закрыть ]
    renderCentered("Удалить", l.btnDeletePos, l.btnDeleteSize, l.fontScale * 0.90f, glm::vec3(1.0f, 0.45f, 0.45f));
    renderCentered("Закрыть", l.btnClosePos, l.btnCloseSize, l.fontScale * 0.90f, glm::vec3(0.85f, 0.90f, 0.95f));
}

void EditorEntityInspector::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                   const Texture2D* whiteTexture,
                                   const BaseData& base, int entityIndex, glm::vec2 entityScreenPos,
                                   float screenW, float screenH, float topBarH, float bottomDockH,
                                   glm::vec2 mousePos) {
    if (!renderer || !whiteTexture) return;

    EntityInspectorLayout l = getLayout(EditorEntityType::Base, entityIndex, entityScreenPos, screenW, screenH, topBarH, bottomDockH);
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

    // Тень и тело панели
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(4.0f * s, 4.0f * s), l.panelSize, 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, l.panelSize, 0.0f, glm::vec4(0.08f, 0.10f, 0.14f, 0.95f));

    // Шапка
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(l.panelSize.x, l.headerH), 0.0f, glm::vec4(0.12f, 0.17f, 0.24f, 0.98f));

    // Кайма панели
    float panelBorderT = 1.5f * s;
    glm::vec4 panelBorder(0.28f, 0.40f, 0.58f, 0.90f);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(l.panelSize.x, panelBorderT), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(0.0f, l.panelSize.y - panelBorderT), glm::vec2(l.panelSize.x, panelBorderT), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos, glm::vec2(panelBorderT, l.panelSize.y), 0.0f, panelBorder);
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(l.panelSize.x - panelBorderT, 0.0f), glm::vec2(panelBorderT, l.panelSize.y), 0.0f, panelBorder);

    // Разделитель
    renderer->drawSpriteRGBA(whiteTexture, l.panelPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.panelSize.x, 1.0f * s), 0.0f, glm::vec4(0.25f, 0.35f, 0.48f, 0.8f));

    // [X] кнопка закрытия
    drawBtn(l.btnCloseCrossPos, l.btnCloseCrossSize,
        glm::vec4(0.20f, 0.12f, 0.12f, 0.8f),
        glm::vec4(0.40f, 0.16f, 0.16f, 0.95f),
        glm::vec4(0.60f, 0.25f, 0.25f, 0.9f));

    // Ряд 1: ID кнопки [-] и [+]
    drawBtn(l.btnIdDecPos, l.btnIdDecSize,
        glm::vec4(0.15f, 0.18f, 0.25f, 0.85f), glm::vec4(0.24f, 0.30f, 0.42f, 0.95f), glm::vec4(0.35f, 0.45f, 0.60f, 0.8f));
    renderer->drawSpriteRGBA(whiteTexture, l.idValuePos, l.idValueSize, 0.0f, glm::vec4(0.10f, 0.12f, 0.16f, 0.90f));
    drawBtn(l.btnIdIncPos, l.btnIdIncSize,
        glm::vec4(0.15f, 0.18f, 0.25f, 0.85f), glm::vec4(0.24f, 0.30f, 0.42f, 0.95f), glm::vec4(0.35f, 0.45f, 0.60f, 0.8f));

    // Ряд 2: В игре: СКРЫТ / ВИДИМ
    if (base.visibleInGame) {
        drawBtn(l.btnVisibilityPos, l.btnVisibilitySize,
            glm::vec4(0.14f, 0.32f, 0.22f, 0.90f), glm::vec4(0.20f, 0.46f, 0.30f, 0.95f), glm::vec4(0.35f, 0.75f, 0.45f, 0.9f));
    } else {
        drawBtn(l.btnVisibilityPos, l.btnVisibilitySize,
            glm::vec4(0.24f, 0.18f, 0.12f, 0.90f), glm::vec4(0.35f, 0.25f, 0.16f, 0.95f), glm::vec4(0.65f, 0.45f, 0.25f, 0.9f));
    }

    // Ряд 3: Плавное растворение: ВКЛ / ВЫКЛ
    if (base.enableFadeOut) {
        drawBtn(l.btnFadePos, l.btnFadeSize,
            glm::vec4(0.12f, 0.25f, 0.35f, 0.90f), glm::vec4(0.18f, 0.36f, 0.50f, 0.95f), glm::vec4(0.30f, 0.65f, 0.85f, 0.9f));
    } else {
        drawBtn(l.btnFadePos, l.btnFadeSize,
            glm::vec4(0.18f, 0.18f, 0.20f, 0.85f), glm::vec4(0.25f, 0.25f, 0.28f, 0.95f), glm::vec4(0.40f, 0.40f, 0.45f, 0.8f));
    }

    // Ряд 4: [ Удалить ] и [ Закрыть ]
    drawBtn(l.btnDeletePos, l.btnDeleteSize,
        glm::vec4(0.32f, 0.12f, 0.12f, 0.85f), glm::vec4(0.48f, 0.16f, 0.16f, 0.95f), glm::vec4(0.75f, 0.25f, 0.25f, 0.9f));
    drawBtn(l.btnClosePos, l.btnCloseSize,
        glm::vec4(0.14f, 0.18f, 0.24f, 0.85f), glm::vec4(0.22f, 0.28f, 0.38f, 0.95f), glm::vec4(0.35f, 0.45f, 0.60f, 0.8f));

    renderer->flush();

    if (!textRenderer) return;

    auto renderCentered = [&](const std::string& text, glm::vec2 pos, glm::vec2 sz, float scale, glm::vec3 col) {
        float tw = textRenderer->CalculateTextWidth(text, scale);
        float th = scale * 26.0f;
        float tx = pos.x + (sz.x - tw) * 0.5f;
        float ty = pos.y + (sz.y - th) * 0.5f + 1.0f * s;
        textRenderer->RenderText(text, tx, ty, scale, col);
    };

    // Заголовок
    std::string title = "БАЗА B#" + std::to_string(base.id) +
                        " (" + std::to_string(base.x) + "," + std::to_string(base.y) + ")";
    float titleScale = l.fontScale * 0.95f;
    textRenderer->RenderText(title, l.panelPos.x + 10.0f * s, l.panelPos.y + (l.headerH - titleScale * 26.0f) * 0.5f + 1.0f * s,
                             titleScale, glm::vec3(0.35f, 0.80f, 1.0f));

    // Крестик [X]
    renderCentered("x", l.btnCloseCrossPos, l.btnCloseCrossSize, l.fontScale, glm::vec3(0.95f, 0.5f, 0.5f));

    // Ряд 1: [-], [ ID: ... ], [+]
    renderCentered("-", l.btnIdDecPos, l.btnIdDecSize, l.fontScale * 1.1f, glm::vec3(0.85f, 0.90f, 1.0f));
    std::string idText = "ID Базы: #" + std::to_string(base.id);
    renderCentered(idText, l.idValuePos, l.idValueSize, l.fontScale * 0.92f, glm::vec3(0.35f, 0.85f, 1.0f));
    renderCentered("+", l.btnIdIncPos, l.btnIdIncSize, l.fontScale * 1.1f, glm::vec3(0.85f, 0.90f, 1.0f));

    // Ряд 2: [ В игре: СКРЫТ / ВИДИМ ]
    std::string visText = base.visibleInGame ? "В игре: ВИДИМА (Маркер ВКЛ)" : "В игре: СКРЫТА (Асфальт)";
    glm::vec3 visCol = base.visibleInGame ? glm::vec3(0.40f, 0.95f, 0.55f) : glm::vec3(0.95f, 0.70f, 0.35f);
    renderCentered(visText, l.btnVisibilityPos, l.btnVisibilitySize, l.fontScale * 0.90f, visCol);

    // Ряд 3: [ Плавный уход: ВКЛ / ВЫКЛ ]
    std::string fadeText = base.enableFadeOut ? "Плавный уход (Fade Out): ВКЛ" : "Плавный уход (Fade Out): ВЫКЛ";
    glm::vec3 fadeCol = base.enableFadeOut ? glm::vec3(0.45f, 0.85f, 1.0f) : glm::vec3(0.70f, 0.72f, 0.78f);
    renderCentered(fadeText, l.btnFadePos, l.btnFadeSize, l.fontScale * 0.90f, fadeCol);

    // Ряд 4: [ Удалить ] и [ Закрыть ]
    renderCentered("Удалить", l.btnDeletePos, l.btnDeleteSize, l.fontScale * 0.90f, glm::vec3(1.0f, 0.45f, 0.45f));
    renderCentered("Закрыть", l.btnClosePos, l.btnCloseSize, l.fontScale * 0.90f, glm::vec3(0.85f, 0.90f, 0.95f));
}

