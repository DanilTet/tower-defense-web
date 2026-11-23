#include "EditorGizmoController.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

EditorGizmoController::~EditorGizmoController() {
    destroyCursors();
}

void EditorGizmoController::initCursors() {
#ifndef __EMSCRIPTEN__
    if (!m_cursorResizeNWSE) {
        m_cursorResizeNWSE = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    }
    if (!m_cursorResizeEW) {
        m_cursorResizeEW = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    }
    if (!m_cursorResizeNS) {
        m_cursorResizeNS = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    }
#endif
}

void EditorGizmoController::destroyCursors() {
#ifndef __EMSCRIPTEN__
    if (m_cursorResizeNWSE) {
        glfwDestroyCursor(m_cursorResizeNWSE);
        m_cursorResizeNWSE = nullptr;
    }
    if (m_cursorResizeEW) {
        glfwDestroyCursor(m_cursorResizeEW);
        m_cursorResizeEW = nullptr;
    }
    if (m_cursorResizeNS) {
        glfwDestroyCursor(m_cursorResizeNS);
        m_cursorResizeNS = nullptr;
    }
#endif
}

void EditorGizmoController::updateHoverCursor(
    GLFWwindow* window,
    const DecorationConfig* selectedDecor,
    glm::vec2 worldMouse,
    float cellSize,
    bool isPaletteDecorations,
    bool isToolbarHovered
) {
    m_hoverHandle = GizmoHandleType::None;
    if (isPaletteDecorations && selectedDecor && !isToolbarHovered) {
        m_hoverHandle = DecorationRenderer::hitTestGizmo(*selectedDecor, worldMouse, 16.0f, cellSize);
    }

    GizmoHandleType curHandle = m_isResizing ? m_activeHandle : m_hoverHandle;
#ifndef __EMSCRIPTEN__
    if (window) {
        if (curHandle == GizmoHandleType::Corner && m_cursorResizeNWSE) {
            glfwSetCursor(window, m_cursorResizeNWSE);
        } else if (curHandle == GizmoHandleType::ScaleX && m_cursorResizeEW) {
            glfwSetCursor(window, m_cursorResizeEW);
        } else if (curHandle == GizmoHandleType::ScaleY && m_cursorResizeNS) {
            glfwSetCursor(window, m_cursorResizeNS);
        } else {
            glfwSetCursor(window, nullptr);
        }
    }
#else
    (void)window;
    (void)curHandle;
#endif
}

bool EditorGizmoController::tryStartResize(const DecorationConfig& d, glm::vec2 worldMouse, float cellSize) {
    auto handleType = DecorationRenderer::hitTestGizmo(d, worldMouse, 16.0f, cellSize);
    if (handleType == GizmoHandleType::None) return false;

    m_isResizing = true;
    m_activeHandle = handleType;
    m_resizeStartScaleX = d.scaleX;
    m_resizeStartScaleY = d.scaleY;
    m_resizeStartScale = d.scale;
    m_resizeStartWorldMouse = worldMouse;

    float rad = glm::radians(d.rotation);
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);
    glm::vec2 deltaStart = worldMouse - d.worldPos;
    m_resizeStartLocalMouse = glm::vec2(
        deltaStart.x * cosR + deltaStart.y * sinR,
        -deltaStart.x * sinR + deltaStart.y * cosR
    );
    return true;
}

bool EditorGizmoController::updateResize(DecorationConfig& d, glm::vec2 worldMouse, float cellSize) {
    if (!m_isResizing || m_activeHandle == GizmoHandleType::None) return false;

    float rad = glm::radians(d.rotation);
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);
    glm::vec2 deltaCur = worldMouse - d.worldPos;
    glm::vec2 curLocal(
        deltaCur.x * cosR + deltaCur.y * sinR,
        -deltaCur.x * sinR + deltaCur.y * cosR
    );

    float baseDim = (d.type == "fog") ? 130.0f : ((d.type == "grass_field") ? 64.0f : ((d.type == "bush") ? 56.0f : (d.type == "puddle" ? 52.0f : 42.0f)));
    float cellScaleFactor = (cellSize > 0.001f) ? (cellSize / 64.0f) : 1.0f;
    float baseHalfW = 0.5f * baseDim * (d.scale * cellScaleFactor) * m_resizeStartScaleX;
    float baseHalfH = 0.5f * baseDim * (d.scale * cellScaleFactor) * m_resizeStartScaleY;

    bool changed = false;
    if (m_activeHandle == GizmoHandleType::ScaleX) {
        float dx = curLocal.x - m_resizeStartLocalMouse.x;
        if (baseHalfW > 1.0f) {
            float ratio = (baseHalfW + dx) / baseHalfW;
            float newScaleX = std::clamp(m_resizeStartScaleX * ratio, 0.2f, 4.0f);
            d.scaleX = std::round(newScaleX * 100.0f) / 100.0f;
            changed = true;
        }
    } else if (m_activeHandle == GizmoHandleType::ScaleY) {
        float dy = -(curLocal.y - m_resizeStartLocalMouse.y); // вверх = -Y
        if (baseHalfH > 1.0f) {
            float ratio = (baseHalfH + dy) / baseHalfH;
            float newScaleY = std::clamp(m_resizeStartScaleY * ratio, 0.2f, 4.0f);
            d.scaleY = std::round(newScaleY * 100.0f) / 100.0f;
            changed = true;
        }
    } else if (m_activeHandle == GizmoHandleType::Corner) {
        float dx = curLocal.x - m_resizeStartLocalMouse.x;
        float dy = curLocal.y - m_resizeStartLocalMouse.y;
        float ratioX = (baseHalfW > 1.0f) ? ((baseHalfW + dx) / baseHalfW) : 1.0f;
        float ratioY = (baseHalfH > 1.0f) ? ((baseHalfH + dy) / baseHalfH) : 1.0f;
        float ratio = (ratioX + ratioY) * 0.5f;
        float newScaleX = std::clamp(m_resizeStartScaleX * ratio, 0.2f, 4.0f);
        float newScaleY = std::clamp(m_resizeStartScaleY * ratio, 0.2f, 4.0f);
        d.scaleX = std::round(newScaleX * 100.0f) / 100.0f;
        d.scaleY = std::round(newScaleY * 100.0f) / 100.0f;
        changed = true;
    }
    return changed;
}

void EditorGizmoController::endResize() {
    m_isResizing = false;
    m_activeHandle = GizmoHandleType::None;
}

