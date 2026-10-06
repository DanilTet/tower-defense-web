#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include "../core/SettingsManager.h"

// все опрные точки
enum class UIAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Center
};

// четенькая функция расчета масштаба интерфейса с учетом разрешения и пользовательской настройки
inline float GetUIScale(int screenWidth, int screenHeight) {
    float scaleX = static_cast<float>(screenWidth) / 1280.0f;
    float scaleY = static_cast<float>(screenHeight) / 720.0f;

    // базовый масштаб от разрешения
    float s = std::min(scaleX, scaleY);
    float baseScale = std::clamp(s, 0.75f, 1.25f);
    return baseScale * SettingsManager::getUIScaleMultiplier();
}

// функция которая считает левый верхний угол (X, Y) для отрисовки спрайта или текста
inline glm::vec2 CalculateAnchorPosition(UIAnchor anchor, glm::vec2 offset, glm::vec2 elementSize, int screenWidth, int screenHeight) {
    glm::vec2 finalPos(0.0f);

    switch (anchor) {
    case UIAnchor::TopLeft:
        finalPos = glm::vec2(0.0f, 0.0f) + offset;
        break;

    case UIAnchor::TopCenter:
        finalPos.x = (static_cast<float>(screenWidth) / 2.0f) - (elementSize.x / 2.0f) + offset.x;
        finalPos.y = 0.0f + offset.y;
        break;

    case UIAnchor::TopRight:
        finalPos.x = static_cast<float>(screenWidth) - elementSize.x - offset.x;
        finalPos.y = 0.0f + offset.y;
        break;

    case UIAnchor::BottomLeft:
        finalPos.x = 0.0f + offset.x;
        finalPos.y = static_cast<float>(screenHeight) - elementSize.y - offset.y;
        break;

    case UIAnchor::BottomCenter:
        finalPos.x = (static_cast<float>(screenWidth) / 2.0f) - (elementSize.x / 2.0f) + offset.x;
        finalPos.y = static_cast<float>(screenHeight) - elementSize.y - offset.y;
        break;

    case UIAnchor::BottomRight:
        finalPos.x = static_cast<float>(screenWidth) - elementSize.x - offset.x;
        finalPos.y = static_cast<float>(screenHeight) - elementSize.y - offset.y;
        break;

    case UIAnchor::Center:
        finalPos.x = (static_cast<float>(screenWidth) / 2.0f) - (elementSize.x / 2.0f) + offset.x;
        finalPos.y = (static_cast<float>(screenHeight) / 2.0f) - (elementSize.y / 2.0f) + offset.y;
        break;
    }

    return finalPos;
}

struct UIRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

struct TimeControlUI {
    // Единый верхний правый HUD: Статистика (HP базы, Деньги, Волна) + Управление временем (||, 1x, 2x, 4x)
    static constexpr float PANEL_W        = 284.0f;
    static constexpr float STATS_ROW_H    = 24.0f;
    static constexpr float ROW_GAP        = 5.0f;
    static constexpr float BTN_H          = 28.0f;
    static constexpr float BTN_GAP        = 4.0f;
    static constexpr float PANEL_PAD_X    = 8.0f;
    static constexpr float PANEL_PAD_Y    = 6.0f;
    // Полная высота панели: 6 + 24 + 5 + 28 + 6 = 69.0f
    static constexpr float PANEL_H        = PANEL_PAD_Y + STATS_ROW_H + ROW_GAP + BTN_H + PANEL_PAD_Y;
    static constexpr float MARGIN_RIGHT   = 14.0f; // отступ от правого края окна
    static constexpr float TOP_Y          = 10.0f; // отступ от верха окна

    static UIRect getPanelRect(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        float pw = PANEL_W * scale;
        float ph = PANEL_H * scale;
        float px = static_cast<float>(screenWidth) - pw - MARGIN_RIGHT * scale;
        float py = TOP_Y * scale;
        return { px, py, pw, ph };
    }

    static float getTopMargin(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect p = getPanelRect(screenWidth, screenHeight);
        return p.y + p.height + 8.0f * scale;
    }

    static float getButtonWidth(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect panel = getPanelRect(screenWidth, screenHeight);
        float innerW = panel.width - PANEL_PAD_X * 2.0f * scale;
        return (innerW - BTN_GAP * 3.0f * scale) / 4.0f;
    }

    static float getButtonY(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect panel = getPanelRect(screenWidth, screenHeight);
        return panel.y + (PANEL_PAD_Y + STATS_ROW_H + ROW_GAP) * scale;
    }

    static UIRect getPauseButtonRect(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect panel = getPanelRect(screenWidth, screenHeight);
        float bw = getButtonWidth(screenWidth, screenHeight);
        float bh = BTN_H * scale;
        float bx = panel.x + PANEL_PAD_X * scale;
        float by = getButtonY(screenWidth, screenHeight);
        return { bx, by, bw, bh };
    }

    static UIRect getSpeed1xButtonRect(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect p0 = getPauseButtonRect(screenWidth, screenHeight);
        return { p0.x + p0.width + BTN_GAP * scale, p0.y, p0.width, p0.height };
    }

    static UIRect getSpeed2xButtonRect(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect p0 = getPauseButtonRect(screenWidth, screenHeight);
        return { p0.x + (p0.width + BTN_GAP * scale) * 2.0f, p0.y, p0.width, p0.height };
    }

    static UIRect getSpeed4xButtonRect(int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect p0 = getPauseButtonRect(screenWidth, screenHeight);
        return { p0.x + (p0.width + BTN_GAP * scale) * 3.0f, p0.y, p0.width, p0.height };
    }

    // Хелперы для 3 бейджей статистики в верхней строке: HP, Деньги, Волна
    static UIRect getStatsBadgeRect(int index, int screenWidth, int screenHeight) {
        float scale = GetUIScale(screenWidth, screenHeight);
        UIRect panel = getPanelRect(screenWidth, screenHeight);
        float innerW = panel.width - PANEL_PAD_X * 2.0f * scale;
        float gap = 4.0f * scale;
        // Пропорции: HP 31%, Деньги 38% (для больших сумм), Волна 31%
        float b0w = std::floor((innerW - gap * 2.0f) * 0.31f);
        float b1w = std::floor((innerW - gap * 2.0f) * 0.38f);
        float b2w = (innerW - gap * 2.0f) - b0w - b1w;
        float by = panel.y + PANEL_PAD_Y * scale;
        float bh = STATS_ROW_H * scale;

        if (index == 0) return { panel.x + PANEL_PAD_X * scale, by, b0w, bh };
        if (index == 1) return { panel.x + PANEL_PAD_X * scale + b0w + gap, by, b1w, bh };
        return { panel.x + PANEL_PAD_X * scale + b0w + gap + b1w + gap, by, b2w, bh };
    }
};

