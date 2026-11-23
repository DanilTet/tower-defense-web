#pragma once
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"

class SpriteRenderer;
class Texture2D;
class Grid;

class EditorEmitterRenderer {
public:
    static int sortByZIndex(std::vector<ParticleEmitterConfig>& emitters, int selectedIndex = -1);

    static void render(
        SpriteRenderer* renderer,
        const Texture2D* texture,
        const Grid* grid,
        const std::vector<ParticleEmitterConfig>& emitters,
        int selectedEmitterIndex
    );
};

