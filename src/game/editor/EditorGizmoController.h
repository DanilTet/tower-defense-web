#pragma once
#include <glm/glm.hpp>
#include "../renderer/DecorationRenderer.h"
#include "../core/LevelManager.h"

struct GLFWcursor;
struct GLFWwindow;

class EditorGizmoController {
private:
    bool m_isResizing = false;
    GizmoHandleType m_activeHandle = GizmoHandleType::None;
    GizmoHandleType m_hoverHandle = GizmoHandleType::None;

    glm::vec2 m_resizeStartLocalMouse{0.0f};
    glm::vec2 m_resizeStartWorldMouse{0.0f};
    float m_resizeStartScaleX = 1.0f;
    float m_resizeStartScaleY = 1.0f;
    float m_resizeStartScale = 1.0f;

    GLFWcursor* m_cursorResizeNWSE = nullptr;
    GLFWcursor* m_cursorResizeEW = nullptr;
    GLFWcursor* m_cursorResizeNS = nullptr;

public:
    EditorGizmoController() = default;
    ~EditorGizmoController();

    void initCursors();
    void destroyCursors();

    void updateHoverCursor(
        GLFWwindow* window,
        const DecorationConfig* selectedDecor,
        glm::vec2 worldMouse,
        float cellSize,
        bool isPaletteDecorations,
        bool isToolbarHovered
    );

    bool tryStartResize(const DecorationConfig& decor, glm::vec2 worldMouse, float cellSize);
    bool updateResize(DecorationConfig& decor, glm::vec2 worldMouse, float cellSize);
    void endResize();

    bool isResizing() const { return m_isResizing; }
    GizmoHandleType getHoverHandle() const { return m_hoverHandle; }
    GizmoHandleType getActiveHandle() const { return m_activeHandle; }
};

