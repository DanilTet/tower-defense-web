#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "EditorToolbarUI.h"
#include "../core/LevelManager.h"
#include "EditorCamera.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;

class EditorHudPreviewRenderer {
public:
    static void renderWorldOverlay(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        HudPreviewMode mode,
        const LevelMapData::CameraSettings& camSettings,
        glm::vec2 camCenter,
        glm::vec2 frameSize
    );

    static void renderScreenText(
        TextRenderer* textRenderer,
        HudPreviewMode mode,
        const LevelMapData::CameraSettings& camSettings,
        glm::vec2 camCenter,
        glm::vec2 frameSize,
        const EditorCamera& camera
    );
};

