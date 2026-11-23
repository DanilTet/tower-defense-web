#pragma once
#include <glm/glm.hpp>
#include <algorithm>

class Grid;
struct LevelMapData;

// ============================================================
//  EditorCamera
//  Инкапсулирует зум / панорамирование редактора карт и
//  преобразования координат world <-> screen.
// ============================================================
class EditorCamera {
public:
    // Диапазон масштаба
    static constexpr float kZoomMin = 0.4f;
    static constexpr float kZoomMax = 2.5f;

    // -------- Состояние камеры --------
    float        zoom       = 1.0f;
    glm::vec2    pan        = glm::vec2(0.0f); // смещение world→screen
    bool         isPanning  = false;
    glm::vec2    prevMouse  = glm::vec2(0.0f);

    // -------- Преобразования координат --------
    glm::vec2 screenToWorld(glm::vec2 screen) const {
        return (screen - pan) / zoom;
    }
    glm::vec2 worldToScreen(glm::vec2 world) const {
        return world * zoom + pan;
    }

    // -------- Навигация --------

    /// Обновление панорамирования при движении мыши (middle-mouse drag).
    void updatePan(glm::vec2 mousePos) {
        if (isPanning) {
            pan += mousePos - prevMouse;
        }
        prevMouse = mousePos;
    }

    /// Масштабирование с фокусом на позиции курсора.
    bool applyZoom(float factor, glm::vec2 mousePos) {
        float newZoom = std::clamp(zoom * factor, kZoomMin, kZoomMax);
        if (newZoom == zoom) return false;
        glm::vec2 worldUnderCursor = screenToWorld(mousePos);
        zoom = newZoom;
        pan  = mousePos - worldUnderCursor * zoom;
        return true;
    }

    /// Центрировать камеру на мировой точке.
    void centerOn(glm::vec2 worldCenter, glm::vec2 screenSize, float newZoom = -1.0f) {
        if (newZoom > 0.0f)
            zoom = std::clamp(newZoom, kZoomMin, kZoomMax);
        pan = screenSize * 0.5f - worldCenter * zoom;
    }

    // -------- Матрица для рендера --------
    //  использование: glm::mat4 cam = camera.viewMatrix();
    //                 glm::mat4 proj = P_screen * cam;
    glm::mat4 viewMatrix() const;
};

