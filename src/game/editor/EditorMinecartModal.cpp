#include "EditorMinecartModal.h"
#include "../ui/UICommon.h"
#include "../core/LocalizationManager.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

void EditorMinecartModal::open() {
    m_isOpen = true;
}

void EditorMinecartModal::open(LevelMapData& mapData) {
    m_boundMapData = &mapData;
    m_isOpen = true;
}

void EditorMinecartModal::close() {
    m_isOpen = false;
}

void EditorMinecartModal::toggle() {
    m_isOpen = !m_isOpen;
}

bool EditorMinecartModal::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void EditorMinecartModal::updateLayout(int screenWidth, int screenHeight) {
    float scale = GetUIScale(screenWidth, screenHeight);
    float mW = std::clamp(500.0f * scale, 370.0f, 600.0f);
    float mH = std::clamp(390.0f * scale, 310.0f, 460.0f);
    m_layout.modalPos = glm::vec2((screenWidth - mW) * 0.5f, (screenHeight - mH) * 0.5f);
    m_layout.modalSize = glm::vec2(mW, mH);
    m_layout.headerH = std::clamp(38.0f * scale, 32.0f, 44.0f);
    m_layout.pad = std::clamp(16.0f * scale, 12.0f, 20.0f);

    float crossSz = std::clamp(24.0f * scale, 20.0f, 28.0f);
    m_layout.btnCloseCrossPos = glm::vec2(m_layout.modalPos.x + mW - crossSz - 8.0f,
                                          m_layout.modalPos.y + (m_layout.headerH - crossSz) * 0.5f);
    m_layout.btnCloseCrossSize = glm::vec2(crossSz);

    float lineH = std::clamp(22.0f * scale, 17.0f, 26.0f);
    float curY = m_layout.modalPos.y + m_layout.headerH + m_layout.pad;
    m_layout.statusY = curY; curY += lineH;
    m_layout.depotY = curY; curY += lineH;
    m_layout.endY = curY; curY += lineH + m_layout.pad * 0.6f;

    float btnH = std::clamp(28.0f * scale, 24.0f, 34.0f);
    float smallBtnW = std::clamp(50.0f * scale, 40.0f, 60.0f);
    float rowGap = std::clamp(8.0f * scale, 5.0f, 10.0f);
    float rowTotalH = btnH + rowGap;

    float btnIncX = m_layout.modalPos.x + mW - m_layout.pad - smallBtnW;
    float btnDecX = btnIncX - smallBtnW - 6.0f;

    // Row 1: Trip Interval
    m_layout.row1Y = curY;
    m_layout.btnIntDecPos = glm::vec2(btnDecX, curY);
    m_layout.btnIntDecSize = glm::vec2(smallBtnW, btnH);
    m_layout.btnIntIncPos = glm::vec2(btnIncX, curY);
    m_layout.btnIntIncSize = glm::vec2(smallBtnW, btnH);
    curY += rowTotalH;

    // Row 2: Warning Time
    m_layout.row2Y = curY;
    m_layout.btnWarnDecPos = glm::vec2(btnDecX, curY);
    m_layout.btnWarnDecSize = glm::vec2(smallBtnW, btnH);
    m_layout.btnWarnIncPos = glm::vec2(btnIncX, curY);
    m_layout.btnWarnIncSize = glm::vec2(smallBtnW, btnH);
    curY += rowTotalH;

    // Row 3: Speed
    m_layout.row3Y = curY;
    m_layout.btnSpeedDecPos = glm::vec2(btnDecX, curY);
    m_layout.btnSpeedDecSize = glm::vec2(smallBtnW, btnH);
    m_layout.btnSpeedIncPos = glm::vec2(btnIncX, curY);
    m_layout.btnSpeedIncSize = glm::vec2(smallBtnW, btnH);

    // Нижній блок кнопок
    float bottomBtnH = std::clamp(32.0f * scale, 26.0f, 38.0f);
    float bottomY = m_layout.modalPos.y + mH - m_layout.pad - bottomBtnH;

    float clearW = std::clamp(170.0f * scale, 130.0f, 210.0f);
    m_layout.btnClearRoutePos = glm::vec2(m_layout.modalPos.x + m_layout.pad, bottomY);
    m_layout.btnClearRouteSize = glm::vec2(clearW, bottomBtnH);

    float closeW = std::clamp(96.0f * scale, 76.0f, 120.0f);
    m_layout.btnClosePos = glm::vec2(m_layout.modalPos.x + mW - m_layout.pad - closeW, bottomY);
    m_layout.btnCloseSize = glm::vec2(closeW, bottomBtnH);

    m_layout.fTitle = std::clamp(0.60f * scale, 0.48f, 0.72f);
    m_layout.fLabel = std::clamp(0.44f * scale, 0.35f, 0.52f);
    m_layout.fValue = std::clamp(0.46f * scale, 0.36f, 0.54f);
    m_layout.fBtn = std::clamp(0.42f * scale, 0.34f, 0.50f);
    m_layout.fCross = std::clamp(0.50f * scale, 0.40f, 0.60f);
}

bool EditorMinecartModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                                      int screenWidth, int screenHeight,
                                      std::vector<MinecartData>& minecarts, bool& isDirty,
                                      const std::function<void()>& onRouteCleared) {
    if (!m_isOpen) {
        m_wasLeftDown = leftDown;
        return false;
    }

    // Закриття по Escape
    if (window && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_isOpen = false;
        m_wasLeftDown = leftDown;
        return true;
    }

    updateLayout(screenWidth, screenHeight);
    const auto& l = m_layout;

    bool isClick = leftDown && !m_wasLeftDown;
    m_wasLeftDown = leftDown;

    if (isClick) {
        // Клік повз вікно -> закрити
        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            m_isOpen = false;
            return true;
        }

        // Хрестик [X]
        if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
            m_isOpen = false;
            return true;
        }

        // Кнопка [Закрити]
        if (isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize)) {
            m_isOpen = false;
            return true;
        }

        // Кнопка [Очистити маршрут]
        if (isPointInRect(mousePos, l.btnClearRoutePos, l.btnClearRouteSize)) {
            if (minecarts.empty()) {
                minecarts.push_back(MinecartData{});
            }
            minecarts[0].start = glm::ivec2(-1, -1);
            minecarts[0].end = glm::ivec2(-1, -1);
            isDirty = true;
            if (onRouteCleared) {
                onRouteCleared();
            }
            return true;
        }

        // Налаштування числових параметрів
        if (!minecarts.empty()) {
            auto& mc = minecarts[0];

            // Інтервал рейсу [-5s] / [+5s] (5.0 - 120.0s)
            if (isPointInRect(mousePos, l.btnIntDecPos, l.btnIntDecSize)) {
                mc.interval = std::clamp(mc.interval - 5.0f, 5.0f, 120.0f);
                isDirty = true;
                return true;
            }
            if (isPointInRect(mousePos, l.btnIntIncPos, l.btnIntIncSize)) {
                mc.interval = std::clamp(mc.interval + 5.0f, 5.0f, 120.0f);
                isDirty = true;
                return true;
            }

            // Час попередження [-0.5s] / [+0.5s] (1.0 - 10.0s)
            if (isPointInRect(mousePos, l.btnWarnDecPos, l.btnWarnDecSize)) {
                mc.warningTime = std::clamp(mc.warningTime - 0.5f, 1.0f, 10.0f);
                isDirty = true;
                return true;
            }
            if (isPointInRect(mousePos, l.btnWarnIncPos, l.btnWarnIncSize)) {
                mc.warningTime = std::clamp(mc.warningTime + 0.5f, 1.0f, 10.0f);
                isDirty = true;
                return true;
            }

            // Швидкість [-1.0] / [+1.0] (2.0 - 25.0 кл/с)
            if (isPointInRect(mousePos, l.btnSpeedDecPos, l.btnSpeedDecSize)) {
                mc.speed = std::clamp(mc.speed - 1.0f, 2.0f, 25.0f);
                isDirty = true;
                return true;
            }
            if (isPointInRect(mousePos, l.btnSpeedIncPos, l.btnSpeedIncSize)) {
                mc.speed = std::clamp(mc.speed + 1.0f, 2.0f, 25.0f);
                isDirty = true;
                return true;
            }
        }
    }

    // Поглинаємо всі кліки, поки модалка активна (не малюємо по сітці)
    return true;
}

bool EditorMinecartModal::handleInput(float mouseX, float mouseY, bool mousePressed, LevelMapData& mapData) {
    bool dummyDirty = false;
    return handleInput(nullptr, glm::vec2(mouseX, mouseY), mousePressed, 1920, 1080, mapData.minecarts, dummyDirty);
}

void EditorMinecartModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                 const Texture2D* whiteTexture,
                                 int screenWidth, int screenHeight,
                                 glm::vec2 mousePos,
                                 const std::vector<MinecartData>& minecarts,
                                 bool isRailPathValid,
                                 size_t railPathSize) {
    if (!m_isOpen || !renderer) return;

    const Texture2D* tex = whiteTexture;
    if (!tex) return;

    updateLayout(screenWidth, screenHeight);
    const auto& l = m_layout;

    // 1. Напівпрозорий фон-затемнення
    renderer->drawSpriteRGBA(tex, glm::vec2(0.0f), glm::vec2(screenWidth, screenHeight), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.76f));

    // 2. Основна панель та 2px золотисто-бурштинова обводка
    glm::vec3 borderCol = glm::vec3(0.95f, 0.75f, 0.20f);
    renderer->drawSprite(tex, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    renderer->drawSprite(tex, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, borderCol);
    renderer->drawSprite(tex, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, borderCol);
    renderer->drawSprite(tex, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, borderCol);
    renderer->drawSprite(tex, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, borderCol);

    // 3. Шапка модального вікна
    renderer->drawSprite(tex, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.19f, 0.16f, 0.10f));
    renderer->drawSprite(tex, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, borderCol);

    // 4. Кнопка-хрестик [X]
    bool hovCross = isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    renderer->drawSprite(tex, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f,
                         hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.40f, 0.36f, 0.28f));
    renderer->drawSprite(tex, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f,
                         hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.18f, 0.14f));

    // 5. Кнопки параметрів ([-] та [+]) з hover-підсвічуванням
    auto drawParamBtn = [&](glm::vec2 pos, glm::vec2 size, bool isInc) {
        bool hov = isPointInRect(mousePos, pos, size);
        glm::vec3 bCol, bgCol;
        if (isInc) {
            bCol  = hov ? glm::vec3(0.40f, 0.90f, 0.45f) : glm::vec3(0.25f, 0.60f, 0.30f);
            bgCol = hov ? glm::vec3(0.18f, 0.38f, 0.20f) : glm::vec3(0.13f, 0.25f, 0.15f);
        } else {
            bCol  = hov ? glm::vec3(0.90f, 0.50f, 0.25f) : glm::vec3(0.60f, 0.35f, 0.18f);
            bgCol = hov ? glm::vec3(0.38f, 0.20f, 0.10f) : glm::vec3(0.24f, 0.14f, 0.08f);
        }
        renderer->drawSprite(tex, pos, size, 0.0f, bCol);
        renderer->drawSprite(tex, pos + glm::vec2(1.0f), size - glm::vec2(2.0f), 0.0f, bgCol);
    };

    drawParamBtn(l.btnIntDecPos, l.btnIntDecSize, false);
    drawParamBtn(l.btnIntIncPos, l.btnIntIncSize, true);
    drawParamBtn(l.btnWarnDecPos, l.btnWarnDecSize, false);
    drawParamBtn(l.btnWarnIncPos, l.btnWarnIncSize, true);
    drawParamBtn(l.btnSpeedDecPos, l.btnSpeedDecSize, false);
    drawParamBtn(l.btnSpeedIncPos, l.btnSpeedIncSize, true);

    // 6. Нижні кнопки: [Очистити маршрут] та [Закрити]
    bool hovClear = isPointInRect(mousePos, l.btnClearRoutePos, l.btnClearRouteSize);
    renderer->drawSprite(tex, l.btnClearRoutePos, l.btnClearRouteSize, 0.0f,
                         hovClear ? glm::vec3(0.90f, 0.35f, 0.35f) : glm::vec3(0.65f, 0.25f, 0.25f));
    renderer->drawSprite(tex, l.btnClearRoutePos + glm::vec2(1.0f), l.btnClearRouteSize - glm::vec2(2.0f), 0.0f,
                         hovClear ? glm::vec3(0.40f, 0.15f, 0.15f) : glm::vec3(0.25f, 0.11f, 0.11f));

    bool hovClose = isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize);
    renderer->drawSprite(tex, l.btnClosePos, l.btnCloseSize, 0.0f,
                         hovClose ? glm::vec3(0.60f, 0.65f, 0.75f) : glm::vec3(0.38f, 0.42f, 0.50f));
    renderer->drawSprite(tex, l.btnClosePos + glm::vec2(1.0f), l.btnCloseSize - glm::vec2(2.0f), 0.0f,
                         hovClose ? glm::vec3(0.25f, 0.28f, 0.34f) : glm::vec3(0.17f, 0.19f, 0.24f));

    renderer->flush();

    // 7. Текстовий шар
    if (!textRenderer) return;

    // Заголовок у шапці
    std::string titleStr = LOC("EDITOR_MINECART_TITLE");
    float tW = textRenderer->CalculateTextWidth(titleStr, l.fTitle);
    float tX = l.modalPos.x + (l.modalSize.x - tW) * 0.5f;
    float tY = l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 1.0f;
    textRenderer->RenderText(titleStr, tX, tY, l.fTitle, glm::vec3(1.0f, 0.88f, 0.30f));

    // Хрестик [X]
    float crossW = textRenderer->CalculateTextWidth("x", l.fCross);
    float cX = l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f;
    float cY = l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 1.0f;
    textRenderer->RenderText("x", cX, cY, l.fCross, glm::vec3(0.92f));

    // Блок статусу колії та маркерів
    const MinecartData& mc = minecarts.empty() ? MinecartData{} : minecarts[0];
    bool hasRoute = mc.hasStart() && mc.hasEnd();

    std::string statusText;
    glm::vec3 statusColor;
    if (!hasRoute) {
        statusText = "Статус колії: маркери не встановлено";
        statusColor = glm::vec3(0.70f, 0.72f, 0.75f);
    } else if (isRailPathValid) {
        statusText = "[✓] " + LOC("EDITOR_MINECART_STATUS_OK") + " (" +
                     std::to_string(static_cast<int>(railPathSize)) + " кл.)";
        statusColor = glm::vec3(0.30f, 1.0f, 0.55f);
    } else {
        statusText = "[!] " + LOC("EDITOR_MINECART_STATUS_BROKEN");
        statusColor = glm::vec3(1.0f, 0.30f, 0.30f);
    }
    textRenderer->RenderText(statusText, l.modalPos.x + l.pad, l.statusY, l.fLabel, statusColor);

    // Координати Депо та Тупика
    std::string depotStr = LOC("EDITOR_RAIL_START") + ": " +
                           (mc.hasStart() ? ("(" + std::to_string(mc.start.x) + ", " + std::to_string(mc.start.y) + ")") : "---");
    textRenderer->RenderText(depotStr, l.modalPos.x + l.pad, l.depotY, l.fLabel, glm::vec3(0.95f, 0.78f, 0.30f));

    std::string endStr = LOC("EDITOR_RAIL_END") + ": " +
                         (mc.hasEnd() ? ("(" + std::to_string(mc.end.x) + ", " + std::to_string(mc.end.y) + ")") : "---");
    textRenderer->RenderText(endStr, l.modalPos.x + l.pad, l.endY, l.fLabel, glm::vec3(0.95f, 0.45f, 0.45f));

    // Рядки параметрів
    auto renderParamRow = [&](float rowY, const std::string& label, const std::string& valStr,
                              glm::vec2 decPos, glm::vec2 decSz, const std::string& decLbl,
                              glm::vec2 incPos, glm::vec2 incSz, const std::string& incLbl) {
        float lblY = rowY + (decSz.y - l.fLabel * 28.0f) * 0.5f + 1.0f;
        textRenderer->RenderText(label, l.modalPos.x + l.pad, lblY, l.fLabel, glm::vec3(0.88f));

        float valW = textRenderer->CalculateTextWidth(valStr, l.fValue);
        float valX = decPos.x - valW - 12.0f;
        float valY = rowY + (decSz.y - l.fValue * 28.0f) * 0.5f + 1.0f;
        textRenderer->RenderText(valStr, valX, valY, l.fValue, glm::vec3(1.0f, 0.90f, 0.45f));

        float dTW = textRenderer->CalculateTextWidth(decLbl, l.fBtn);
        float dTX = decPos.x + (decSz.x - dTW) * 0.5f;
        float dTY = decPos.y + (decSz.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
        textRenderer->RenderText(decLbl, dTX, dTY, l.fBtn, glm::vec3(1.0f, 0.75f, 0.50f));

        float iTW = textRenderer->CalculateTextWidth(incLbl, l.fBtn);
        float iTX = incPos.x + (incSz.x - iTW) * 0.5f;
        float iTY = incPos.y + (incSz.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
        textRenderer->RenderText(incLbl, iTX, iTY, l.fBtn, glm::vec3(0.60f, 1.0f, 0.65f));
    };

    // 1. Інтервал рейсу
    std::string intValStr = std::to_string(static_cast<int>(std::round(mc.interval))) + " s";
    renderParamRow(l.row1Y, LOC("EDITOR_MINECART_INTERVAL"), intValStr,
                   l.btnIntDecPos, l.btnIntDecSize, "-5s",
                   l.btnIntIncPos, l.btnIntIncSize, "+5s");

    // 2. Час попередження
    char warnBuf[32];
    std::snprintf(warnBuf, sizeof(warnBuf), "%.1f s", mc.warningTime);
    renderParamRow(l.row2Y, LOC("EDITOR_MINECART_WARNING"), std::string(warnBuf),
                   l.btnWarnDecPos, l.btnWarnDecSize, "-0.5s",
                   l.btnWarnIncPos, l.btnWarnIncSize, "+0.5s");

    // 3. Швидкість вагонетки
    std::string spValStr = std::to_string(static_cast<int>(std::round(mc.speed))) + " кл/с";
    renderParamRow(l.row3Y, LOC("EDITOR_MINECART_SPEED"), spValStr,
                   l.btnSpeedDecPos, l.btnSpeedDecSize, "-1.0",
                   l.btnSpeedIncPos, l.btnSpeedIncSize, "+1.0");

    // Кнопка [Очистити маршрут]
    std::string clrStr = LOC("EDITOR_MINECART_CLEAR_ROUTE");
    float clrW = textRenderer->CalculateTextWidth(clrStr, l.fBtn);
    float clrX = l.btnClearRoutePos.x + (l.btnClearRouteSize.x - clrW) * 0.5f;
    float clrY = l.btnClearRoutePos.y + (l.btnClearRouteSize.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
    textRenderer->RenderText(clrStr, clrX, clrY, l.fBtn, glm::vec3(1.0f, 0.85f, 0.85f));

    // Кнопка [Закрити]
    std::string clsStr = LOC("EDITOR_MINECART_CLOSE");
    float clsW = textRenderer->CalculateTextWidth(clsStr, l.fBtn);
    float clsX = l.btnClosePos.x + (l.btnCloseSize.x - clsW) * 0.5f;
    float clsY = l.btnClosePos.y + (l.btnCloseSize.y - l.fBtn * 28.0f) * 0.5f + 1.0f;
    textRenderer->RenderText(clsStr, clsX, clsY, l.fBtn, glm::vec3(0.92f, 0.94f, 0.98f));
}

void EditorMinecartModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenWidth, float screenHeight) {
    static std::vector<MinecartData> emptyCarts;
    const auto& carts = m_boundMapData ? m_boundMapData->minecarts : emptyCarts;
    render(renderer, textRenderer, nullptr, static_cast<int>(screenWidth), static_cast<int>(screenHeight),
           glm::vec2(-1000.0f), carts, false, 0);
}

