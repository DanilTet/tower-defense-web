#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
class Grid;
class EditorCamera;

class EditorTileInfoOverlay {
public:
    struct Params {
        glm::vec2 mousePos{0.0f};
        int screenWidth = 1280;
        int screenHeight = 720;
        float bottomDockHeight = 120.0f;
        int gridWidth = 20;
        int gridHeight = 12;
        bool hasSidePockets = false;
        std::string background = "default";
        bool isSuppressed = false; // true if toolbar is hovered or any modal dialog is open
    };

    static void render(
        SpriteRenderer* renderer,
        TextRenderer* textRenderer,
        const Texture2D* whiteTexture,
        const EditorCamera& camera,
        const Grid* grid,
        const std::vector<SpawnerData>& spawners,
        const std::vector<BaseData>& bases,
        const std::vector<MinecartData>& minecarts,
        const Params& params
    );
};

