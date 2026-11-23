#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include "../game/core/LevelManager.h"

class SpriteRenderer;
class Texture2D;
class Grid;

enum class GizmoHandleType {
    None = 0,
    Corner,  // Proportional scale (bottom-right circle/ball)
    ScaleX,  // Width scale (right arrow)
    ScaleY   // Height scale (top arrow)
};

struct GizmoInfo {
    float boxW = 0.0f;
    float boxH = 0.0f;
    glm::vec2 cornerLocal = glm::vec2(0.0f);
    glm::vec2 xHandleLocal = glm::vec2(0.0f);
    glm::vec2 yHandleLocal = glm::vec2(0.0f);
    glm::vec2 cornerWorld = glm::vec2(0.0f);
    glm::vec2 xHandleWorld = glm::vec2(0.0f);
    glm::vec2 yHandleWorld = glm::vec2(0.0f);
};

class DecorationRenderer {
public:
    static GizmoInfo getGizmoInfo(const DecorationConfig& d, float cellSize = 64.0f);
    static GizmoHandleType hitTestGizmo(const DecorationConfig& d, glm::vec2 testWorldPos, float hitRadius = 16.0f, float cellSize = 64.0f);

    static void renderDecoration(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const DecorationConfig& decor,
        bool isSelected = false,
        float alpha = 1.0f,
        float cellSize = 64.0f
    );

    static void renderDecoration(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        glm::vec2 renderPos,
        const DecorationConfig& decor,
        bool isSelected = false,
        float alpha = 1.0f,
        float cellSize = 64.0f
    );

    static void renderDecorations(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::vector<DecorationConfig>& decorations,
        int selectedIndex = -1,
        float cellSize = 64.0f,
        bool onlyFog = false
    );

    static void renderDecorations(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::vector<DecorationConfig>& decorations,
        glm::vec2 gridOffset,
        float cellSize,
        int selectedIndex = -1,
        bool onlyFog = false
    );

    static void renderDecorations(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::vector<DecorationConfig>& decorations,
        const Grid& grid,
        int selectedIndex = -1,
        bool onlyFog = false
    );

    static int sortByZIndex(std::vector<DecorationConfig>& decorations, int selectedIndex = -1);

    static void renderPreview(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::string& type,
        glm::vec2 worldPos,
        float rotation = 0.0f,
        float scale = 1.0f,
        float scaleX = 1.0f,
        float scaleY = 1.0f,
        float opacity = 0.5f,
        float cellSize = 64.0f
    );
};

