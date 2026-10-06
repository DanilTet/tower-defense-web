#pragma once
#include <memory>
#include <glm/glm.hpp>

class SpriteRenderer;
class Texture2D;

class HealthBarRenderer {
public:
    static void draw(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> texture,
        const glm::vec2& entityPos,
        float entityWidth,
        int currentHp,
        int maxHp,
        float yOffset = -10.0f,
        float barHeight = 6.0f
    );
};

