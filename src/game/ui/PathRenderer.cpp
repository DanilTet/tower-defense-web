#include "PathRenderer.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "world/Grid.h"
#include "core/ConfigManager.h"

void PathVisualizer::update(float dt, float cellSize) {
    m_pathAnimationTimer += dt * (cellSize * 0.5f);
}

void PathVisualizer::renderPathArrows(
    SpriteRenderer* renderer,
    const Texture2D* arrowTex,
    const std::vector<glm::ivec2>& levelPath,
    const Grid& gameGrid,
    glm::vec3 arrowColor) {
    // защита если текстурка не подгрузило или пустой путь
    if (!renderer || !arrowTex || levelPath.size() < 2) return;

    float cellSize = gameGrid.getCellSize(); // размер клетки
    float halfCell = cellSize / 2.0f; // половина клетки

    glm::vec2 arrowSize(cellSize * 0.20f, cellSize * 0.20f); // Размер стрелки — 25% от ширины клетки
    float arrowSpacing = cellSize * 0.50f; // Расстояние между стрелками — 60% от ширины клетки

    // Аддитивное свечение стрелок пути через батчер SpriteRenderer
    renderer->setAdditiveBlending(true);

    // пробегаемся по всем отрезкам пути
    for (size_t i = 0; i < levelPath.size() - 1; ++i) {
        // переводим узлы пути в пиксельные центры клеток
        glm::vec2 startPx = gameGrid.gridToPixel(levelPath[i].x, levelPath[i].y) + glm::vec2(halfCell);
        glm::vec2 endPx = gameGrid.gridToPixel(levelPath[i + 1].x, levelPath[i + 1].y) + glm::vec2(halfCell);

        glm::vec2 toEnd = endPx - startPx;
        float segmentLength = glm::length(toEnd);
        glm::vec2 direction = glm::normalize(toEnd);

        // вычисляем угол поворота стрелочки в зависимости от направления отрезка
        float angle = glm::degrees(atan2(direction.y, direction.x));
        // ефект ползанья
        // стартовое смещение для этого кадра
        float startOffset = fmod(m_pathAnimationTimer, arrowSpacing);

        // рисуем стрелочки вдоль всего текущего сегмента
        for (float d = startOffset; d < segmentLength; d += arrowSpacing) {
            glm::vec2 arrowPos = startPx + direction * d;

            // сдвигаем позицию, чтобы arrowPos была ровно по центру стрелочки
            glm::vec2 drawPos = arrowPos - (arrowSize / 2.0f);

            // отрисовка через спрайт-рендерер
            renderer->drawSprite(arrowTex, drawPos, arrowSize, angle, arrowColor);
        }
    }

    // Возврат обычного альфа-блендинга через батчер SpriteRenderer
    renderer->setAdditiveBlending(false);
}