#include "HealthBarRenderer.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../textures/Texture2D.h"
#include <algorithm>

void HealthBarRenderer::draw(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> texture,
    const glm::vec2& entityPos,
    float entityWidth,
    int currentHp,
    int maxHp,
    float yOffset,
    float barHeight)
{
    if (!renderer || !texture || maxHp <= 0) return;

    float hpPercent = std::clamp(static_cast<float>(currentHp) / static_cast<float>(maxHp), 0.0f, 1.0f);

    glm::vec2 barPos = entityPos + glm::vec2(0.0f, yOffset);

    // Фоновая красная полоска
    renderer->drawSprite(texture, barPos, glm::vec2(entityWidth, barHeight), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));

    // Зеленая полоска текущего здоровья
    if (hpPercent > 0.0f) {
        renderer->drawSprite(texture, barPos, glm::vec2(entityWidth * hpPercent, barHeight), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

