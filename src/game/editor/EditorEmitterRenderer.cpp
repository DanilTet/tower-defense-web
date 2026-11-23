#include "EditorEmitterRenderer.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "../world/Grid.h"
#include <algorithm>
#include <cmath>

int EditorEmitterRenderer::sortByZIndex(std::vector<ParticleEmitterConfig>& emitters, int selectedIndex) {
    if (emitters.size() <= 1) {
        return selectedIndex;
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(emitters.size())) {
        int idx = selectedIndex;
        while (idx > 0 && emitters[idx].zIndex < emitters[idx - 1].zIndex) {
            std::swap(emitters[idx], emitters[idx - 1]);
            --idx;
        }
        while (idx + 1 < static_cast<int>(emitters.size()) && emitters[idx].zIndex > emitters[idx + 1].zIndex) {
            std::swap(emitters[idx], emitters[idx + 1]);
            ++idx;
        }
        return idx;
    }

    std::stable_sort(emitters.begin(), emitters.end(), [](const ParticleEmitterConfig& a, const ParticleEmitterConfig& b) {
        return a.zIndex < b.zIndex;
    });
    return selectedIndex;
}

void EditorEmitterRenderer::render(
    SpriteRenderer* renderer,
    const Texture2D* texture,
    const Grid* grid,
    const std::vector<ParticleEmitterConfig>& emitters,
    int selectedEmitterIndex
) {
    if (!renderer || !texture || !grid || emitters.empty()) return;

    float cellSize = grid->getCellSize();

    for (size_t i = 0; i < emitters.size(); ++i) {
        const auto& em = emitters[i];
        bool isSelected = (static_cast<int>(i) == selectedEmitterIndex);
        glm::vec2 center = grid->getOffset() + em.tilePos * cellSize;

        // Цветовая схема по пресету
        glm::vec4 bgCol(0.18f, 0.40f, 0.60f, 0.85f);
        glm::vec4 borderCol(0.65f, 0.90f, 1.0f, 1.0f);
        glm::vec4 arrowCol(0.85f, 0.96f, 1.0f, 1.0f);

        if (em.type == "water_drip") {
            bgCol = glm::vec4(0.12f, 0.28f, 0.55f, 0.85f);
            borderCol = glm::vec4(0.40f, 0.75f, 1.0f, 1.0f);
            arrowCol = glm::vec4(0.60f, 0.88f, 1.0f, 1.0f);
        } else if (em.type == "sparks") {
            bgCol = glm::vec4(0.55f, 0.30f, 0.10f, 0.85f);
            borderCol = glm::vec4(1.0f, 0.85f, 0.25f, 1.0f);
            arrowCol = glm::vec4(1.0f, 0.95f, 0.50f, 1.0f);
        } else if (em.type == "smoke") {
            bgCol = glm::vec4(0.24f, 0.24f, 0.28f, 0.85f);
            borderCol = glm::vec4(0.85f, 0.85f, 0.90f, 1.0f);
            arrowCol = glm::vec4(0.95f, 0.95f, 1.0f, 1.0f);
        } else if (em.type == "fog") {
            bgCol = glm::vec4(0.20f, 0.28f, 0.25f, 0.85f);
            borderCol = glm::vec4(0.65f, 0.85f, 0.75f, 1.0f);
            arrowCol = glm::vec4(0.80f, 0.95f, 0.85f, 1.0f);
        }

        if (isSelected) {
            borderCol = glm::vec4(1.0f, 0.92f, 0.25f, 1.0f);
            arrowCol = glm::vec4(1.0f, 1.0f, 0.60f, 1.0f);
        }

        // Базовый круглый/квадратный значок в точке эмиттера
        float badgeSize = std::max(18.0f, cellSize * 0.44f);
        glm::vec2 badgeTopLeft = center - glm::vec2(badgeSize * 0.5f);

        // Внешнее свечение при выделении
        if (isSelected) {
            float haloSize = badgeSize + 8.0f;
            glm::vec2 haloTopLeft = center - glm::vec2(haloSize * 0.5f);
            float haloT = 2.0f;
            glm::vec4 haloCol(1.0f, 0.85f, 0.20f, 0.90f);
            renderer->drawSpriteRGBA(texture, haloTopLeft, glm::vec2(haloSize, haloT), 0.0f, haloCol);
            renderer->drawSpriteRGBA(texture, haloTopLeft + glm::vec2(0.0f, haloSize - haloT), glm::vec2(haloSize, haloT), 0.0f, haloCol);
            renderer->drawSpriteRGBA(texture, haloTopLeft, glm::vec2(haloT, haloSize), 0.0f, haloCol);
            renderer->drawSpriteRGBA(texture, haloTopLeft + glm::vec2(haloSize - haloT, 0.0f), glm::vec2(haloT, haloSize), 0.0f, haloCol);
        }

        // Тень значка
        renderer->drawSpriteRGBA(texture, badgeTopLeft + glm::vec2(1.5f), glm::vec2(badgeSize), 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
        // Тело значка
        renderer->drawSpriteRGBA(texture, badgeTopLeft, glm::vec2(badgeSize), 0.0f, bgCol);
        // Кайма
        float borderT = isSelected ? 3.0f : 2.0f;
        renderer->drawSpriteRGBA(texture, badgeTopLeft, glm::vec2(badgeSize, borderT), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, badgeTopLeft + glm::vec2(0.0f, badgeSize - borderT), glm::vec2(badgeSize, borderT), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, badgeTopLeft, glm::vec2(borderT, badgeSize), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, badgeTopLeft + glm::vec2(badgeSize - borderT, 0.0f), glm::vec2(borderT, badgeSize), 0.0f, borderCol);

        // Стрелка направления струи angleDeg
        float rad = glm::radians(em.angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        glm::vec2 perp(-dir.y, dir.x);

        float arrowLen = badgeSize * 0.75f;
        glm::vec2 tip = center + dir * arrowLen;

        // Линия стрелки (сегментами)
        for (float t = 0.2f; t <= 1.0f; t += 0.2f) {
            glm::vec2 pt = center + dir * (arrowLen * t);
            renderer->drawSpriteRGBA(texture, pt - glm::vec2(1.5f), glm::vec2(3.0f), 0.0f, arrowCol);
        }
        // Наконечник стрелки
        renderer->drawSpriteRGBA(texture, tip - glm::vec2(2.5f), glm::vec2(5.0f), 0.0f, borderCol);
        glm::vec2 wing1 = tip - dir * 5.0f + perp * 4.0f;
        glm::vec2 wing2 = tip - dir * 5.0f - perp * 4.0f;
        renderer->drawSpriteRGBA(texture, wing1 - glm::vec2(1.5f), glm::vec2(3.0f), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, wing2 - glm::vec2(1.5f), glm::vec2(3.0f), 0.0f, borderCol);
    }
}

