#include "BackgroundRenderer.h"
#include "SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "../game/world/Grid.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

void BackgroundRenderer::computeCulledBounds(
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    float paddingCells,
    float& outMinX,
    float& outMinY,
    float& outMaxX,
    float& outMaxY
) {
    float cellSize = 64.0f;
    if (grid != nullptr) {
        cellSize = grid->getCellSize();
    }
    float pad = cellSize * paddingCells;

    if (viewRect.valid) {
        outMinX = std::min(viewRect.min.x, viewRect.max.x) - pad;
        outMaxX = std::max(viewRect.min.x, viewRect.max.x) + pad;
        outMinY = std::min(viewRect.min.y, viewRect.max.y) - pad;
        outMaxY = std::max(viewRect.min.y, viewRect.max.y) + pad;
    } else {
        glm::vec2 gridOffset(0.0f);
        float gridW = screenWidth;
        float gridH = screenHeight;
        if (grid != nullptr) {
            gridOffset = grid->getOffset();
            gridW = static_cast<float>(grid->getWidth()) * cellSize;
            gridH = static_cast<float>(grid->getHeight()) * cellSize;
        }
        float marginX = std::max(1600.0f, screenWidth * 2.5f);
        float marginY = std::max(1200.0f, screenHeight * 2.0f);
        outMinX = gridOffset.x - marginX;
        outMaxX = gridOffset.x + gridW + marginX;
        outMinY = gridOffset.y - marginY;
        outMaxY = gridOffset.y + gridH + marginY;
    }
}

bool BackgroundRenderer::isRectVisible(
    float x,
    float y,
    float w,
    float h,
    float cullMinX,
    float cullMinY,
    float cullMaxX,
    float cullMaxY
) {
    if (x + w < cullMinX) {
        return false;
    }
    if (x > cullMaxX) {
        return false;
    }
    if (y + h < cullMinY) {
        return false;
    }
    if (y > cullMaxY) {
        return false;
    }
    return true;
}

std::vector<BackgroundRenderer::MinePortalGeom> BackgroundRenderer::collectMinePortals(
    const Grid* grid,
    const std::vector<glm::ivec2>* baseCells
) {
    std::vector<MinePortalGeom> portals;
    if (grid == nullptr) {
        return portals;
    }

    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    float gridW = static_cast<float>(grid->getWidth()) * cellSize;
    float gridH = static_cast<float>(grid->getHeight()) * cellSize;
    float yardX = gridOffset.x;
    float yardY = gridOffset.y;
    float bldgX = yardX + gridW;

    if (baseCells != nullptr && !baseCells->empty()) {
        std::vector<glm::ivec2> sorted = *baseCells;
        std::sort(sorted.begin(), sorted.end(), [](const glm::ivec2& a, const glm::ivec2& b) {
            if (a.x != b.x) {
                return a.x < b.x;
            }
            return a.y < b.y;
        });

        size_t i = 0;
        while (i < sorted.size()) {
            int bx = sorted[i].x;
            int minY = sorted[i].y;
            int maxY = sorted[i].y;
            size_t j = i + 1;
            while (j < sorted.size() && sorted[j].x == bx && sorted[j].y <= maxY + 1) {
                if (sorted[j].y > maxY) {
                    maxY = sorted[j].y;
                }
                ++j;
            }
            i = j;

            float basePixelX = yardX + static_cast<float>(bx) * cellSize;
            float groupTopY = yardY + static_cast<float>(minY) * cellSize;
            float groupBotY = yardY + static_cast<float>(maxY + 1) * cellSize;

            MinePortalGeom p;
            p.centerY = (groupTopY + groupBotY) * 0.5f;
            p.openingH = std::max(cellSize * 2.1f, (groupBotY - groupTopY) + cellSize * 1.1f);
            p.topY = p.centerY - p.openingH * 0.5f;
            p.botY = p.centerY + p.openingH * 0.5f;
            p.mouthX = std::min(bldgX, basePixelX - cellSize * 0.35f);
            p.coverStartX = std::min(bldgX + cellSize * 0.85f, basePixelX + cellSize * 0.15f);
            p.endX = std::max(bldgX + cellSize * 3.6f, basePixelX + cellSize * 1.6f);
            portals.push_back(p);
        }
    } else {
        MinePortalGeom p;
        p.centerY = yardY + gridH * 0.5f;
        p.openingH = cellSize * 2.1f;
        p.topY = p.centerY - p.openingH * 0.5f;
        p.botY = p.centerY + p.openingH * 0.5f;
        p.mouthX = bldgX;
        p.coverStartX = bldgX + cellSize * 0.85f;
        p.endX = bldgX + cellSize * 3.6f;
        portals.push_back(p);
    }

    return portals;
}

void BackgroundRenderer::renderSubstrate(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect
) {
    if (!renderer || !whiteTexture) return;

    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        renderPipesCanalSubstrate(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect);
    } else if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        renderMineEntranceSubstrate(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect);
    } else if (isCinnabarMine(backgroundType)) {
        renderCinnabarMineSubstrate(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect);
    } else {
        float cullMinX = 0.0f;
        float cullMinY = 0.0f;
        float cullMaxX = 0.0f;
        float cullMaxY = 0.0f;
        computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 1.5f, cullMinX, cullMinY, cullMaxX, cullMaxY);
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, cullMinY),
            glm::vec2(cullMaxX - cullMinX, cullMaxY - cullMinY),
            0.0f,
            glm::vec3(0.12f, 0.13f, 0.16f)
        );
    }
}

void BackgroundRenderer::renderEnvironment(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    const std::vector<glm::ivec2>* baseCells
) {
    if (!renderer || !whiteTexture) return;

    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        renderPipesCanalEnvironment(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect);
    } else if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        renderMineEntranceEnvironment(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect, baseCells);
    } else if (isCinnabarMine(backgroundType)) {
        renderCinnabarMineEnvironment(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect, baseCells);
    }
}

void BackgroundRenderer::renderForeground(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    const std::vector<glm::ivec2>* baseCells
) {
    if (!renderer || !whiteTexture) return;

    if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        renderMineEntranceForeground(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect, baseCells);
    } else if (isCinnabarMine(backgroundType)) {
        renderCinnabarMineForeground(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect, baseCells);
    }
}

void BackgroundRenderer::render(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    const std::vector<glm::ivec2>* baseCells
) {
    renderSubstrate(renderer, whiteTexture, backgroundType, grid, screenWidth, screenHeight, viewRect);
    renderEnvironment(renderer, whiteTexture, backgroundType, grid, screenWidth, screenHeight, viewRect, baseCells);
}

void BackgroundRenderer::renderPipesCanalSubstrate(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect
) {
    glm::vec2 gridOffset(0.0f);
    float cellSize = 64.0f;
    if (grid != nullptr) {
        gridOffset = grid->getOffset();
        cellSize = grid->getCellSize();
    }

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 1.5f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    float totalW = cullMaxX - cullMinX;
    float totalH = cullMaxY - cullMinY;

    // 1. Базовый сочно-зеленый склон холма / трава
    renderer->drawSprite(
        whiteTexture,
        glm::vec2(cullMinX, cullMinY),
        glm::vec2(totalW, totalH),
        0.0f,
        glm::vec3(0.18f, 0.40f, 0.16f)
    );

    // Плавные горизонтальные градиентные полосы травяного склона (привязаны к gridOffset.y)
    float slopeBandH = std::max(36.0f, cellSize * 0.75f);
    int bandStart = static_cast<int>(std::floor((cullMinY - gridOffset.y) / slopeBandH));
    int bandEnd = static_cast<int>(std::ceil((cullMaxY - gridOffset.y) / slopeBandH));
    for (int i = bandStart; i <= bandEnd; ++i) {
        float bY = gridOffset.y + static_cast<float>(i) * slopeBandH;
        int modIdx = ((i % 12) + 12) % 12;
        float factor = static_cast<float>(modIdx) / 12.0f;
        float wave = std::sin(factor * 3.14159f * 2.0f) * 0.03f;
        glm::vec3 bandCol = glm::vec3(
            0.17f + 0.04f * factor + wave,
            0.37f + 0.06f * factor + wave * 1.5f,
            0.15f + 0.03f * factor + wave * 0.8f
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, bY),
            glm::vec2(totalW, slopeBandH + 1.0f),
            0.0f,
            bandCol
        );
    }
}

void BackgroundRenderer::renderPipesCanalEnvironment(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect
) {
    if (!grid) return;

    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    float gridH = static_cast<float>(grid->getHeight()) * cellSize;

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 4.0f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    // 2. Монументальные трубы дюкеров вдоль верхней и нижней границ поля
    float pipeThickness = 1.15f * cellSize;
    float pipeGap = 0.15f * cellSize;

    float topPipeY = gridOffset.y - pipeThickness - pipeGap;
    float botPipeY = gridOffset.y + gridH + pipeGap;

    if (isRectVisible(cullMinX, topPipeY - pipeThickness * 0.5f, cullMaxX - cullMinX, pipeThickness * 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
        renderPipe(renderer, whiteTexture, cullMinX, cullMaxX, topPipeY, pipeThickness, cellSize, gridOffset.x);
    }
    if (isRectVisible(cullMinX, botPipeY - pipeThickness * 0.5f, cullMaxX - cullMinX, pipeThickness * 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
        renderPipe(renderer, whiteTexture, cullMinX, cullMaxX, botPipeY, pipeThickness, cellSize, gridOffset.x);
    }
}

void BackgroundRenderer::renderPipe(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    float startX,
    float endX,
    float pipeY,
    float pipeThickness,
    float cellSize,
    float gridOffsetX
) {
    float totalW = endX - startX;

    // 1. Мягкая тень от трубы на траву (свет падает сверху)
    float shadowY = pipeY + pipeThickness;
    float shadowThick1 = std::max(16.0f, pipeThickness * 0.22f);
    float shadowThick2 = std::max(12.0f, pipeThickness * 0.16f);
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(startX, shadowY),
        glm::vec2(totalW, shadowThick1),
        0.0f,
        glm::vec4(0.04f, 0.10f, 0.03f, 0.55f)
    );
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(startX, shadowY + shadowThick1 * 0.5f),
        glm::vec2(totalW, shadowThick2),
        0.0f,
        glm::vec4(0.06f, 0.14f, 0.05f, 0.28f)
    );

    // 2. Опорные бетонные блоки (компенсаторы):
    // Привязаны СТРОГО к целым клеткам сетки (каждые 4 клетки), чтобы декорации и эмиттеры на трубах
    // никогда не съезжали относительно геометрии трубы при изменении размеров сетки
    float cubeSpacing = 4.0f * cellSize;
    float cubeW = pipeThickness * 1.30f; // Ширина больше диаметра трубы
    float cubeH = pipeThickness * 1.35f; // Высота больше диаметра трубы
    float cubeY = pipeY - (cubeH - pipeThickness) * 0.5f;

    // Привязываем сетку компенсаторов к gridOffsetX
    float firstCubeX = gridOffsetX - std::floor((gridOffsetX - startX) / cubeSpacing) * cubeSpacing;

    // Рисуем заднюю тень и основания кубов
    for (float cx = firstCubeX; cx <= endX; cx += cubeSpacing) {
        // Тень от массивного бетонного блока на траву
        float shadowH = std::max(4.0f, 0.20f * cellSize);
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(cx - 4.0f, cubeY + cubeH),
            glm::vec2(cubeW + 8.0f, shadowH),
            0.0f,
            glm::vec4(0.03f, 0.08f, 0.02f, 0.65f)
        );

        // Внешний контур бетонного куба
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx, cubeY),
            glm::vec2(cubeW, cubeH),
            0.0f,
            glm::vec3(0.20f, 0.22f, 0.24f)
        );

        // Основной объем бетона
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + 2.0f),
            glm::vec2(cubeW - 4.0f, cubeH - 4.0f),
            0.0f,
            glm::vec3(0.48f, 0.50f, 0.53f)
        );

        // Верхняя грань (светлая фаска)
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + 2.0f),
            glm::vec2(cubeW - 4.0f, 5.0f),
            0.0f,
            glm::vec3(0.72f, 0.74f, 0.77f)
        );

        // Левая грань (освещенная фаска)
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + 7.0f),
            glm::vec2(4.0f, cubeH - 12.0f),
            0.0f,
            glm::vec3(0.60f, 0.62f, 0.65f)
        );

        // Нижняя теневая грань
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + cubeH - 8.0f),
            glm::vec2(cubeW - 4.0f, 6.0f),
            0.0f,
            glm::vec3(0.30f, 0.32f, 0.34f)
        );

        // Правая теневая фаска
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + cubeW - 6.0f, cubeY + 7.0f),
            glm::vec2(4.0f, cubeH - 15.0f),
            0.0f,
            glm::vec3(0.38f, 0.40f, 0.42f)
        );

        // Вертикальный компенсаторный шов в центре
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + cubeW * 0.5f - 1.0f, cubeY + 4.0f),
            glm::vec2(2.0f, cubeH - 10.0f),
            0.0f,
            glm::vec3(0.22f, 0.24f, 0.26f)
        );
    }

    // 3. Объемный горизонтальный цилиндр трубы (проходит сквозь компенсаторы)
    struct PipeBand {
        float relY;
        float relH;
        glm::vec3 color;
    };

    static const PipeBand bands[] = {
        { 0.00f, 0.07f, glm::vec3(0.16f, 0.14f, 0.13f) }, // Верхний темный контур
        { 0.07f, 0.11f, glm::vec3(0.36f, 0.32f, 0.28f) }, // Затененная сталь
        { 0.18f, 0.14f, glm::vec3(0.56f, 0.52f, 0.48f) }, // Освещенный металл
        { 0.32f, 0.20f, glm::vec3(0.88f, 0.90f, 0.94f) }, // Мощный цилиндрический блик
        { 0.52f, 0.18f, glm::vec3(0.48f, 0.32f, 0.24f) }, // Темно-ржавый металл
        { 0.70f, 0.18f, glm::vec3(0.28f, 0.18f, 0.14f) }, // Глубокая ржавчина
        { 0.88f, 0.12f, glm::vec3(0.12f, 0.09f, 0.08f) }  // Нижний контур тени
    };

    for (const auto& b : bands) {
        float bY = pipeY + b.relY * pipeThickness;
        float bH = b.relH * pipeThickness + 0.5f;
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(startX, bY),
            glm::vec2(totalW, bH),
            0.0f,
            b.color
        );
    }

    // 4. Манжеты / сальники герметизации на входе и выходе трубы из куба
    float collarExtra = pipeThickness * 0.12f;
    float collarH = pipeThickness + collarExtra;
    float collarY = pipeY - collarExtra * 0.5f;
    float collarW = pipeThickness * 0.10f;
    for (float cx = firstCubeX; cx <= endX; cx += cubeSpacing) {
        // Левая манжета (вход трубы в блок)
        renderer->drawSprite(whiteTexture, glm::vec2(cx - collarW * 0.5f, collarY), glm::vec2(collarW, collarH), 0.0f, glm::vec3(0.25f, 0.27f, 0.30f));
        renderer->drawSprite(whiteTexture, glm::vec2(cx - collarW * 0.5f + 2.0f, collarY + 3.0f), glm::vec2(std::max(2.0f, collarW - 4.0f), collarH - 6.0f), 0.0f, glm::vec3(0.75f, 0.78f, 0.82f));

        // Правая манжета (выход трубы из блока)
        renderer->drawSprite(whiteTexture, glm::vec2(cx + cubeW - collarW * 0.5f, collarY), glm::vec2(collarW, collarH), 0.0f, glm::vec3(0.25f, 0.27f, 0.30f));
        renderer->drawSprite(whiteTexture, glm::vec2(cx + cubeW - collarW * 0.5f + 2.0f, collarY + 3.0f), glm::vec2(std::max(2.0f, collarW - 4.0f), collarH - 6.0f), 0.0f, glm::vec3(0.75f, 0.78f, 0.82f));
    }

    // 5. Поперечные стальные кольца/швы (фланцы) через каждые 2 клетки сетки
    float ringSpacing = 2.0f * cellSize;
    float ringW = pipeThickness * 0.14f;
    float ringExtra = pipeThickness * 0.12f;
    float ringH = pipeThickness + ringExtra;
    float ringY = pipeY - ringExtra * 0.5f;

    float firstRingX = gridOffsetX - std::floor((gridOffsetX - startX) / ringSpacing) * ringSpacing;
    for (float rx = firstRingX; rx <= endX; rx += ringSpacing) {
        // Темный шов слева
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(rx - 2.0f, ringY),
            glm::vec2(2.0f, ringH),
            0.0f,
            glm::vec3(0.10f, 0.08f, 0.08f)
        );

        // Рельефное кольцо / фланец
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(rx, ringY),
            glm::vec2(ringW, ringH),
            0.0f,
            glm::vec3(0.42f, 0.38f, 0.36f)
        );

        // Блик по центру кольца
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(rx + 1.5f, pipeY + 0.32f * pipeThickness),
            glm::vec2(ringW - 3.0f, 0.20f * pipeThickness),
            0.0f,
            glm::vec3(0.92f, 0.94f, 0.96f)
        );

        // Двойные шляпки болтов сверху и снизу кольца
        renderer->drawSprite(whiteTexture, glm::vec2(rx + 2.0f, ringY + 2.0f), glm::vec2(3.0f, 3.0f), 0.0f, glm::vec3(0.85f, 0.85f, 0.85f));
        renderer->drawSprite(whiteTexture, glm::vec2(rx + 2.0f, ringY + ringH - 5.0f), glm::vec2(3.0f, 3.0f), 0.0f, glm::vec3(0.85f, 0.85f, 0.85f));
    }
}

void BackgroundRenderer::renderMineEntranceSubstrate(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect
) {
    if (!renderer || !whiteTexture) return;

    glm::vec2 gridOffset(0.0f);
    float cellSize = 64.0f;
    if (grid != nullptr) {
        gridOffset = grid->getOffset();
        cellSize = grid->getCellSize();
    }

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 1.5f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    float totalW = cullMaxX - cullMinX;
    float totalH = cullMaxY - cullMinY;
    float yardY = gridOffset.y;

    // 1. БАЗОВАЯ ЗЕМЛЯ ОКРУЖЕНИЯ: теплый приглушенный уличный грунт / сухая глина
    renderer->drawSprite(
        whiteTexture,
        glm::vec2(cullMinX, cullMinY),
        glm::vec2(totalW, totalH),
        0.0f,
        glm::vec3(0.35f, 0.25f, 0.17f)
    );

    // 2. Широкие мягкие пласты теплого грунта (минимальный контраст, привязаны к yardY)
    float bandH = std::max(96.0f, cellSize * 2.2f);
    int bandStart = static_cast<int>(std::floor((cullMinY - yardY) / bandH));
    int bandEnd = static_cast<int>(std::ceil((cullMaxY - yardY) / bandH));
    for (int i = bandStart; i <= bandEnd; ++i) {
        float bY = yardY + static_cast<float>(i) * bandH;
        int modIdx = ((i % 8) + 8) % 8;
        float factor = static_cast<float>(modIdx) / 8.0f;
        float wave = std::sin(factor * 3.14159f * 2.0f) * 0.008f;
        glm::vec3 bandCol = glm::vec3(
            0.335f + 0.018f * factor + wave,
            0.240f + 0.014f * factor + wave * 0.85f,
            0.165f + 0.010f * factor + wave * 0.70f
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, bY),
            glm::vec2(totalW, bandH + 1.0f),
            0.0f,
            bandCol
        );
    }
}

void BackgroundRenderer::renderMineEntranceEnvironment(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    const std::vector<glm::ivec2>* baseCells
) {
    if (!renderer || !whiteTexture || !grid) return;

    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    int gridW_tiles = grid->getWidth();
    int gridH_tiles = grid->getHeight();
    float gridW = static_cast<float>(gridW_tiles) * cellSize;
    float gridH = static_cast<float>(gridH_tiles) * cellSize;

    float yardX = gridOffset.x;
    float yardY = gridOffset.y;
    float yardW = gridW;
    float yardH = gridH;
    float bldgX = yardX + yardW;

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 1.5f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    auto hash2D = [](int x, int y, uint32_t seed) -> uint32_t {
        uint32_t h = seed ^ (static_cast<uint32_t>(x) * 374761393u) ^ (static_cast<uint32_t>(y) * 668265263u);
        h = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    };

    auto rand01 = [](uint32_t val) -> float {
        return static_cast<float>(val & 0xFFFFu) / 65535.0f;
    };

    const auto& outerCells = grid->getOuterCells();
    auto isOuterTile = [&](int tx, int ty) -> bool {
        return outerCells.find({tx, ty}) != outerCells.end();
    };

    float wastelandMaxX = std::min(cullMaxX, bldgX + cellSize * 0.25f);

    // =========================================================================
    // 1. УЛИЧНЫЙ ГРУНТ: КРУПНЫЕ ПРИГЛУШЕННЫЕ ПЯТНА ГЛИНЫ, ЗЕМЛИ И ЖУХЛОЙ ТРАВЫ
    // =========================================================================
    if (wastelandMaxX > cullMinX) {
        float patchStep = cellSize * 4.0f;
        int ixStart = static_cast<int>(std::floor((cullMinX - yardX) / patchStep)) - 1;
        int ixEnd = static_cast<int>(std::ceil((wastelandMaxX - yardX) / patchStep));
        int iyStart = static_cast<int>(std::floor((cullMinY - yardY) / patchStep)) - 1;
        int iyEnd = static_cast<int>(std::ceil((cullMaxY - yardY) / patchStep));

        for (int iy = iyStart; iy <= iyEnd; ++iy) {
            for (int ix = ixStart; ix <= ixEnd; ++ix) {
                uint32_t h = hash2D(ix, iy, 908209u);
                if ((h % 100u) >= 45u) {
                    continue;
                }

                float rShiftX = (rand01(hash2D(ix, iy, 1103515245u)) - 0.5f) * patchStep * 0.45f;
                float rShiftY = (rand01(hash2D(ix, iy, 123456789u)) - 0.5f) * patchStep * 0.45f;
                float px = yardX + static_cast<float>(ix) * patchStep + rShiftX;
                float py = yardY + static_cast<float>(iy) * patchStep + rShiftY;
                float pW = patchStep * (0.75f + 0.40f * rand01(h >> 3));
                float pH = patchStep * (0.65f + 0.40f * rand01(h >> 7));

                float centerPx = px + pW * 0.5f;
                float centerPy = py + pH * 0.5f;
                int tx = static_cast<int>(std::floor((centerPx - yardX) / cellSize));
                int ty = static_cast<int>(std::floor((centerPy - yardY) / cellSize));

                if (centerPx >= yardX - cellSize * 0.2f && centerPx <= bldgX && centerPy >= yardY && centerPy <= yardY + yardH) {
                    continue;
                }
                if (centerPx >= bldgX) {
                    continue;
                }
                if (isOuterTile(tx, ty)) {
                    continue;
                }
                if (!isRectVisible(px, py, pW, pH, cullMinX, cullMinY, wastelandMaxX, cullMaxY)) {
                    continue;
                }

                float rType = rand01(h >> 12);
                glm::vec4 col(0.0f);
                if (rType < 0.34f) {
                    // Выгоревшая пыльная глина
                    col = glm::vec4(0.44f, 0.33f, 0.22f, 0.17f);
                } else if (rType < 0.64f) {
                    // Утоптанный темный суглинок / сырая земля
                    col = glm::vec4(0.24f, 0.17f, 0.11f, 0.19f);
                } else if (rType < 0.84f) {
                    // Тусклая сухая пожухлая трава
                    col = glm::vec4(0.33f, 0.30f, 0.17f, 0.18f);
                } else {
                    // Спокойная пыльная охра
                    col = glm::vec4(0.40f, 0.23f, 0.15f, 0.15f);
                }

                renderer->drawSpriteRGBA(whiteTexture, glm::vec2(px, py), glm::vec2(pW, pH), 0.0f, col);
            }
        }
    }

    // =========================================================================
    // 2. РЕДКИЕ ДЕТАЛИ УЛИЦЫ: ТРЕЩИНЫ, ПЛОСКИЕ КАМНИ, ПОЖУХЛАЯ ТРАВА
    // =========================================================================
    if (wastelandMaxX > cullMinX) {
        float featureStep = cellSize * 3.0f;
        int fxStart = static_cast<int>(std::floor((cullMinX - yardX) / featureStep)) - 1;
        int fxEnd = static_cast<int>(std::ceil((wastelandMaxX - yardX) / featureStep));
        int fyStart = static_cast<int>(std::floor((cullMinY - yardY) / featureStep)) - 1;
        int fyEnd = static_cast<int>(std::ceil((cullMaxY - yardY) / featureStep));

        for (int iy = fyStart; iy <= fyEnd; ++iy) {
            for (int ix = fxStart; ix <= fxEnd; ++ix) {
                uint32_t hDet = hash2D(ix, iy, 521471u);
                uint32_t detType = hDet % 100u;
                if (detType >= 32u) {
                    continue;
                }

                float fx = yardX + (static_cast<float>(ix) + 0.18f + 0.60f * rand01(hDet)) * featureStep;
                float fy = yardY + (static_cast<float>(iy) + 0.18f + 0.60f * rand01(hDet >> 5)) * featureStep;

                if (fx >= bldgX - cellSize * 0.35f) {
                    continue;
                }
                if (fx >= yardX - cellSize * 0.25f && fx <= bldgX && fy >= yardY - cellSize * 0.25f && fy <= yardY + yardH + cellSize * 0.25f) {
                    continue;
                }

                int tx = static_cast<int>(std::floor((fx - yardX) / cellSize));
                int ty = static_cast<int>(std::floor((fy - yardY) / cellSize));
                if (isOuterTile(tx, ty)) {
                    continue;
                }

                float featExtent = cellSize * 0.9f;
                if (!isRectVisible(fx - featExtent * 0.5f, fy - featExtent * 0.5f, featExtent, featExtent, cullMinX, cullMinY, wastelandMaxX, cullMaxY)) {
                    continue;
                }

                if (detType < 13u) {
                    // Трещина в сухой вытоптанной глине
                    float segLen = cellSize * (0.24f + 0.16f * rand01(hDet >> 9));
                    float thickness = std::max(2.0f, cellSize * 0.032f);
                    bool horizontal = ((hDet >> 13) % 2u) == 0u;
                    glm::vec4 crackCol(0.18f, 0.12f, 0.08f, 0.48f);
                    glm::vec4 rimCol(0.48f, 0.38f, 0.26f, 0.20f);

                    if (horizontal) {
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx, fy), glm::vec2(segLen, thickness), 0.0f, crackCol);
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx + segLen * 0.85f, fy + thickness * 0.8f), glm::vec2(segLen * 0.65f, thickness * 0.85f), 0.0f, crackCol);
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx + 2.0f, fy - 1.5f), glm::vec2(segLen * 0.8f, 1.5f), 0.0f, rimCol);
                    } else {
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx, fy), glm::vec2(thickness, segLen), 0.0f, crackCol);
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx + thickness * 0.8f, fy + segLen * 0.85f), glm::vec2(thickness * 0.85f, segLen * 0.65f), 0.0f, crackCol);
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx - 1.5f, fy + 2.0f), glm::vec2(1.5f, segLen * 0.8f), 0.0f, rimCol);
                    }
                } else if (detType < 23u) {
                    // Крупный плоский пыльный камень / обломок породы
                    float pw = cellSize * (0.28f + 0.18f * rand01(hDet >> 8));
                    float ph = pw * (0.55f + 0.20f * rand01(hDet >> 12));
                    glm::vec3 slabCol(0.37f, 0.33f, 0.28f);
                    if ((detType % 2u) == 0u) {
                        slabCol = glm::vec3(0.32f, 0.29f, 0.26f);
                    }
                    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx - 1.5f, fy + ph * 0.65f), glm::vec2(pw + 3.0f, ph * 0.45f), 0.0f, glm::vec4(0.12f, 0.08f, 0.05f, 0.28f));
                    renderer->drawSprite(whiteTexture, glm::vec2(fx, fy), glm::vec2(pw, ph), 0.0f, slabCol);
                    renderer->drawSprite(whiteTexture, glm::vec2(fx, fy), glm::vec2(pw, std::max(2.0f, ph * 0.22f)), 0.0f, slabCol + glm::vec3(0.08f, 0.07f, 0.06f));
                } else if (detType < 28u) {
                    // Редкие тусклые пятна сухой пожухлой травы
                    float gw = cellSize * (0.28f + 0.16f * rand01(hDet >> 7));
                    float gh = gw * 0.50f;
                    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx, fy + gh * 0.25f), glm::vec2(gw, gh * 0.75f), 0.0f, glm::vec4(0.22f, 0.17f, 0.10f, 0.30f));
                    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx + gw * 0.12f, fy), glm::vec2(gw * 0.74f, gh * 0.68f), 0.0f, glm::vec4(0.38f, 0.34f, 0.19f, 0.46f));
                    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx + gw * 0.28f, fy + gh * 0.12f), glm::vec2(gw * 0.42f, std::max(2.5f, gh * 0.35f)), 0.0f, glm::vec4(0.47f, 0.42f, 0.24f, 0.40f));
                } else {
                    // Редкое минеральное вкрапление вдали от границы двора
                    float distToYardX = std::min(std::abs(fx - yardX), std::abs(fx - bldgX));
                    float distToYardY = std::min(std::abs(fy - yardY), std::abs(fy - (yardY + yardH)));
                    float distToYard = std::min(distToYardX, distToYardY);
                    if (distToYard > cellSize * 0.7f) {
                        float mw = cellSize * (0.24f + 0.16f * rand01(hDet >> 10));
                        float mh = mw * 0.52f;
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx - 2.0f, fy - 1.5f), glm::vec2(mw + 4.0f, mh + 3.0f), 0.0f, glm::vec4(0.16f, 0.12f, 0.10f, 0.36f));
                        if ((hDet & 1u) == 0u) {
                            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx, fy), glm::vec2(mw, mh), 0.0f, glm::vec4(0.44f, 0.50f, 0.50f, 0.38f));
                        } else {
                            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx, fy), glm::vec2(mw, mh), 0.0f, glm::vec4(0.42f, 0.18f, 0.13f, 0.36f));
                        }
                    }
                }
            }
        }
    }

    // =========================================================================
    // 3. ЕСТЕСТВЕННЫЙ ПЕРЕХОД ПО ОТКРЫТОМУ КОНТУРУ ИГРОВОЙ ЗОНЫ И ОБОЧИНА
    // =========================================================================
    auto isPlayableOutdoorCell = [&](int tx, int ty) -> bool {
        if (tx >= gridW_tiles) {
            return false;
        }
        if (tx >= 0 && tx < gridW_tiles && ty >= 0 && ty < gridH_tiles) {
            return grid->getCellType(tx, ty) != CellType::Empty;
        }
        auto it = outerCells.find({tx, ty});
        if (it != outerCells.end() && it->second != CellType::Empty) {
            return true;
        }
        return false;
    };

    auto drawEdgeStrip = [&](int dir, float cellX, float cellY, float extA, float extB, float inward, float outward, const glm::vec4& col) {
        float thickness = inward + outward;
        float tanLen = cellSize + extA + extB;
        if (dir == 0) {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX - outward, cellY - extA), glm::vec2(thickness, tanLen), 0.0f, col);
        } else if (dir == 1) {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX + cellSize - inward, cellY - extA), glm::vec2(thickness, tanLen), 0.0f, col);
        } else if (dir == 2) {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX - extA, cellY - outward), glm::vec2(tanLen, thickness), 0.0f, col);
        } else {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX - extA, cellY + cellSize - inward), glm::vec2(tanLen, thickness), 0.0f, col);
        }
    };

    auto drawEdgeRect = [&](int dir, float cellX, float cellY, float tanPos, float tanLen, float outStart, float crossThick, const glm::vec4& col) {
        if (dir == 0) {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX - outStart - crossThick, cellY + tanPos), glm::vec2(crossThick, tanLen), 0.0f, col);
        } else if (dir == 1) {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX + cellSize + outStart, cellY + tanPos), glm::vec2(crossThick, tanLen), 0.0f, col);
        } else if (dir == 2) {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX + tanPos, cellY - outStart - crossThick), glm::vec2(tanLen, crossThick), 0.0f, col);
        } else {
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cellX + tanPos, cellY + cellSize + outStart), glm::vec2(tanLen, crossThick), 0.0f, col);
        }
    };

    auto drawOpenEdge = [&](int tx, int ty, int dir, bool extNeg, bool extPos) {
        float cellX = yardX + static_cast<float>(tx) * cellSize;
        float cellY = yardY + static_cast<float>(ty) * cellSize;
        float extA = 0.0f;
        if (extNeg) {
            extA = cellSize * 0.14f;
        }
        float extB = 0.0f;
        if (extPos) {
            extB = cellSize * 0.14f;
        }

        // Многослойная мягкая обочина / насыпь из 3 полупрозрачных полос
        drawEdgeStrip(dir, cellX, cellY, extA, extB, cellSize * 0.08f, cellSize * 0.14f, glm::vec4(0.41f, 0.27f, 0.17f, 0.56f));
        drawEdgeStrip(dir, cellX, cellY, extA, extB, -cellSize * 0.05f, cellSize * 0.29f, glm::vec4(0.37f, 0.255f, 0.168f, 0.34f));
        drawEdgeStrip(dir, cellX, cellY, extA, extB, -cellSize * 0.18f, cellSize * 0.46f, glm::vec4(0.335f, 0.235f, 0.158f, 0.17f));

        // Детерминированные пятна грязи, пыли и мелкого щебня для разбивки прямой линии
        uint32_t h = hash2D(tx, ty, 614203u + static_cast<uint32_t>(dir) * 9187u);
        if ((h % 100u) < 72u) {
            float outStart = (-0.08f + 0.22f * rand01(h >> 4)) * cellSize;
            float tanLen = cellSize * (0.36f + 0.34f * rand01(h >> 8));
            float crossThick = cellSize * (0.16f + 0.16f * rand01(h >> 12));
            float tanPos = (cellSize - tanLen) * rand01(h >> 2);

            glm::vec4 blotchCol(0.39f, 0.28f, 0.18f, 0.28f);
            if ((h % 3u) == 0u) {
                blotchCol = glm::vec4(0.28f, 0.19f, 0.12f, 0.34f);
            } else if ((h % 3u) == 1u) {
                blotchCol = glm::vec4(0.45f, 0.31f, 0.19f, 0.32f);
            }
            drawEdgeRect(dir, cellX, cellY, tanPos, tanLen, outStart, crossThick, blotchCol);
        }

        if (((h >> 3) % 100u) < 32u) {
            float outStart2 = (0.04f + 0.20f * rand01(h >> 5)) * cellSize;
            float tanLen2 = cellSize * (0.24f + 0.20f * rand01(h >> 6));
            float crossThick2 = cellSize * (0.12f + 0.12f * rand01(h >> 10));
            float tanPos2 = (cellSize - tanLen2) * (1.0f - rand01(h >> 2));
            drawEdgeRect(dir, cellX, cellY, tanPos2, tanLen2, outStart2, crossThick2, glm::vec4(0.31f, 0.21f, 0.14f, 0.26f));
        }

        if (((h >> 7) % 100u) < 42u) {
            float stoneSize = std::max(3.5f, cellSize * (0.045f + 0.035f * rand01(h >> 11)));
            float stoneOut = (0.04f + 0.20f * rand01(h >> 14)) * cellSize;
            float stoneTan = cellSize * (0.15f + 0.65f * rand01(h >> 9));
            drawEdgeRect(dir, cellX, cellY, stoneTan, stoneSize * 1.25f, stoneOut, stoneSize, glm::vec4(0.33f, 0.29f, 0.25f, 0.62f));
            if ((h & 1u) == 0u) {
                float stoneSize2 = std::max(3.0f, stoneSize * 0.78f);
                drawEdgeRect(dir, cellX, cellY, stoneTan + stoneSize * 1.6f, stoneSize2 * 1.15f, stoneOut + stoneSize * 0.45f, stoneSize2, glm::vec4(0.24f, 0.18f, 0.13f, 0.52f));
            }
        }
    };

    auto processPlayableCellEdges = [&](int tx, int ty) {
        float cx = yardX + static_cast<float>(tx) * cellSize;
        float cy = yardY + static_cast<float>(ty) * cellSize;
        if (!isRectVisible(cx - cellSize * 0.5f, cy - cellSize * 0.5f, cellSize * 2.0f, cellSize * 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
            return;
        }

        bool openLeft = !isPlayableOutdoorCell(tx - 1, ty);
        bool openRight = (tx + 1 < gridW_tiles) && !isPlayableOutdoorCell(tx + 1, ty);
        bool openTop = !isPlayableOutdoorCell(tx, ty - 1);
        bool openBot = !isPlayableOutdoorCell(tx, ty + 1);

        if (openLeft) {
            drawOpenEdge(tx, ty, 0, openTop, openBot);
        }
        if (openRight) {
            drawOpenEdge(tx, ty, 1, openTop, openBot);
        }
        if (openTop) {
            drawOpenEdge(tx, ty, 2, openLeft, openRight);
        }
        if (openBot) {
            drawOpenEdge(tx, ty, 3, openLeft, openRight);
        }
    };

    int visMinX = static_cast<int>(std::floor((cullMinX - yardX) / cellSize)) - 1;
    int visMaxX = std::min(gridW_tiles - 1, static_cast<int>(std::ceil((cullMaxX - yardX) / cellSize)) + 1);
    int visMinY = static_cast<int>(std::floor((cullMinY - yardY) / cellSize)) - 1;
    int visMaxY = static_cast<int>(std::ceil((cullMaxY - yardY) / cellSize)) + 1;

    int mainStartY = std::max(0, visMinY);
    int mainEndY = std::min(gridH_tiles - 1, visMaxY);
    int mainStartX = std::max(0, visMinX);
    int mainEndX = std::min(gridW_tiles - 1, visMaxX);

    for (int ty = mainStartY; ty <= mainEndY; ++ty) {
        for (int tx = mainStartX; tx <= mainEndX; ++tx) {
            if (grid->getCellType(tx, ty) != CellType::Empty) {
                processPlayableCellEdges(tx, ty);
            }
        }
    }

    for (const auto& [pos, type] : outerCells) {
        if (type == CellType::Empty) {
            continue;
        }
        if (pos.first >= gridW_tiles) {
            continue;
        }
        if (pos.first < visMinX || pos.first > visMaxX || pos.second < visMinY || pos.second > visMaxY) {
            continue;
        }
        processPlayableCellEdges(pos.first, pos.second);
    }

    // Собираем порталы заранее, чтобы не рисовать пятна сырости и передние ребра жесткости поверх проемов
    std::vector<MinePortalGeom> portals = collectMinePortals(grid, baseCells);
    std::sort(portals.begin(), portals.end(), [](const MinePortalGeom& a, const MinePortalGeom& b) {
        return a.topY < b.topY;
    });

    // =========================================================================
    // 4. ГЛУХОЙ ПРОМЫШЛЕННЫЙ БЕТОННЫЙ МОНОЛИТ ШАХТЫ СПРАВА (БЕЗ ОКОН)
    // =========================================================================
    float castShadowW = std::max(44.0f, cellSize * 0.80f);
    if (cullMaxX >= bldgX - castShadowW) {
        float wallTopY = cullMinY;
        float wallBotY = cullMaxY;
        float wallH = wallBotY - wallTopY;
        float wallRightX = std::max(bldgX, cullMaxX);
        float wallVisW = wallRightX - bldgX;

        // 4.1. Падающая и контактная тень от бетонного массива на рабочий двор влево
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(bldgX - castShadowW, wallTopY),
            glm::vec2(castShadowW, wallH),
            0.0f,
            glm::vec4(0.07f, 0.06f, 0.05f, 0.22f)
        );
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(bldgX - castShadowW * 0.42f, wallTopY),
            glm::vec2(castShadowW * 0.42f, wallH),
            0.0f,
            glm::vec4(0.05f, 0.045f, 0.04f, 0.28f)
        );

        float contactW = std::max(10.0f, cellSize * 0.16f);
        float contactTop = std::max(wallTopY, yardY);
        float contactBot = std::min(wallBotY, yardY + yardH);
        if (contactBot > contactTop) {
            float curY = contactTop;
            for (const auto& p : portals) {
                if (p.mouthX <= bldgX + 2.0f && p.topY > curY) {
                    float segEnd = std::min(contactBot, p.topY);
                    if (segEnd > curY) {
                        renderer->drawSpriteRGBA(
                            whiteTexture,
                            glm::vec2(bldgX - contactW, curY),
                            glm::vec2(contactW, segEnd - curY),
                            0.0f,
                            glm::vec4(0.03f, 0.025f, 0.02f, 0.42f)
                        );
                    }
                }
                if (p.mouthX <= bldgX + 2.0f && p.botY > curY) {
                    curY = p.botY;
                }
            }
            if (contactBot > curY) {
                renderer->drawSpriteRGBA(
                    whiteTexture,
                    glm::vec2(bldgX - contactW, curY),
                    glm::vec2(contactW, contactBot - curY),
                    0.0f,
                    glm::vec4(0.03f, 0.025f, 0.02f, 0.42f)
                );
            }
        }

        if (wallVisW > 0.0f) {
            // 4.2. Монолитный массив бетона
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(bldgX, wallTopY),
                glm::vec2(wallVisW, wallH),
                0.0f,
                glm::vec3(0.30f, 0.31f, 0.33f)
            );

            // 4.3. Крупные бетонные плиты с едва заметными швами (только видимые)
            float slabW = cellSize * 4.0f;
            float slabH = cellSize * 2.4f;
            int rowStart = static_cast<int>(std::floor((wallTopY - yardY) / slabH)) - 1;
            int rowEnd = static_cast<int>(std::ceil((wallBotY - yardY) / slabH));
            int colStart = std::max(0, static_cast<int>(std::floor((cullMinX - bldgX) / slabW)) - 1);
            int colEnd = static_cast<int>(std::ceil((wallRightX - bldgX) / slabW));

            for (int r = rowStart; r <= rowEnd; ++r) {
                float sy = yardY + static_cast<float>(r) * slabH;
                float xShift = 0.0f;
                if (((r % 2) + 2) % 2 == 1) {
                    xShift = slabW * 0.5f;
                }

                for (int c = colStart - 1; c <= colEnd; ++c) {
                    float rawX = bldgX + static_cast<float>(c) * slabW + xShift;
                    float sx = std::max(bldgX, rawX);
                    float sRight = std::min(wallRightX, rawX + slabW);
                    float sw = sRight - sx;
                    if (sw <= 0.0f) {
                        continue;
                    }
                    if (!isRectVisible(sx, sy, sw, slabH, cullMinX, cullMinY, wallRightX, cullMaxY)) {
                        continue;
                    }

                    uint32_t sh = hash2D(c, r, 731921u);
                    float tone = (rand01(sh) - 0.5f) * 0.028f;
                    glm::vec3 slabCol(0.30f + tone, 0.31f + tone, 0.33f + tone);
                    renderer->drawSprite(whiteTexture, glm::vec2(sx, sy), glm::vec2(sw, slabH), 0.0f, slabCol);

                    // Едва заметный горизонтальный шов между плитами
                    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(sx, sy), glm::vec2(sw, 2.0f), 0.0f, glm::vec4(0.16f, 0.17f, 0.18f, 0.42f));
                    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(sx, sy + 2.0f), glm::vec2(sw, 1.5f), 0.0f, glm::vec4(0.44f, 0.45f, 0.47f, 0.22f));

                    // Едва заметный вертикальный шов
                    if (rawX >= bldgX) {
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(rawX, sy), glm::vec2(2.0f, slabH), 0.0f, glm::vec4(0.16f, 0.17f, 0.18f, 0.42f));
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(rawX + 2.0f, sy), glm::vec2(1.5f, slabH), 0.0f, glm::vec4(0.44f, 0.45f, 0.47f, 0.20f));
                    }
                }
            }

            // 4.4. Передний край парапета по границе bldgX (прерывается на устьях порталов)
            float parapetW = std::max(10.0f, cellSize * 0.18f);
            if (isRectVisible(bldgX, wallTopY, parapetW, wallH, cullMinX, cullMinY, wallRightX, cullMaxY)) {
                float curY = wallTopY;
                for (const auto& p : portals) {
                    if (p.mouthX <= bldgX + parapetW && p.topY > curY) {
                        float segEnd = std::min(wallBotY, p.topY);
                        if (segEnd > curY) {
                            renderer->drawSprite(whiteTexture, glm::vec2(bldgX, curY), glm::vec2(parapetW, segEnd - curY), 0.0f, glm::vec3(0.35f, 0.36f, 0.38f));
                            renderer->drawSprite(whiteTexture, glm::vec2(bldgX, curY), glm::vec2(2.5f, segEnd - curY), 0.0f, glm::vec3(0.50f, 0.52f, 0.55f));
                        }
                    }
                    if (p.mouthX <= bldgX + parapetW && p.botY > curY) {
                        curY = p.botY;
                    }
                }
                if (wallBotY > curY) {
                    renderer->drawSprite(whiteTexture, glm::vec2(bldgX, curY), glm::vec2(parapetW, wallBotY - curY), 0.0f, glm::vec3(0.35f, 0.36f, 0.38f));
                    renderer->drawSprite(whiteTexture, glm::vec2(bldgX, curY), glm::vec2(2.5f, wallBotY - curY), 0.0f, glm::vec3(0.50f, 0.52f, 0.55f));
                }
            }

            // 4.5. Массивные вертикальные ребра жесткости (несущие контрфорсы)
            float ribSpacing = cellSize * 4.0f;
            float ribW = std::max(28.0f, cellSize * 0.52f);
            float ribShadowW = std::max(18.0f, cellSize * 0.35f);
            int ribColStart = std::max(0, static_cast<int>(std::floor((cullMinX - bldgX - ribShadowW - ribW) / ribSpacing)));
            int ribColEnd = static_cast<int>(std::ceil((wallRightX - bldgX) / ribSpacing));

            auto drawVerticalRibSegment = [&](float rx, float segTop, float segBot) {
                float segH = segBot - segTop;
                if (segH <= 0.0f) {
                    return;
                }
                renderer->drawSprite(whiteTexture, glm::vec2(rx, segTop), glm::vec2(ribW, segH), 0.0f, glm::vec3(0.37f, 0.38f, 0.40f));
                renderer->drawSprite(whiteTexture, glm::vec2(rx, segTop), glm::vec2(3.0f, segH), 0.0f, glm::vec3(0.50f, 0.52f, 0.55f));
                renderer->drawSprite(whiteTexture, glm::vec2(rx + ribW - 4.0f, segTop), glm::vec2(4.0f, segH), 0.0f, glm::vec3(0.19f, 0.20f, 0.22f));
                renderer->drawSpriteRGBA(whiteTexture, glm::vec2(rx + ribW, segTop), glm::vec2(ribShadowW, segH), 0.0f, glm::vec4(0.06f, 0.07f, 0.08f, 0.36f));
            };

            for (int col = ribColStart; col <= ribColEnd; ++col) {
                float rx = bldgX + static_cast<float>(col) * ribSpacing;
                if (col == 0) {
                    rx = bldgX + parapetW;
                }
                if (!isRectVisible(rx, wallTopY, ribW + ribShadowW, wallH, cullMinX, cullMinY, wallRightX, cullMaxY)) {
                    continue;
                }

                float curY = wallTopY;
                for (const auto& p : portals) {
                    bool hitsPortalX = (rx + ribW >= p.mouthX - 4.0f && rx <= p.endX + 4.0f);
                    if (hitsPortalX) {
                        float cutoutTop = p.topY - cellSize * 0.38f;
                        float cutoutBot = p.botY + cellSize * 0.38f;
                        if (cutoutTop > curY) {
                            drawVerticalRibSegment(rx, curY, std::min(wallBotY, cutoutTop));
                        }
                        if (cutoutBot > curY) {
                            curY = cutoutBot;
                        }
                    }
                }
                if (wallBotY > curY) {
                    drawVerticalRibSegment(rx, curY, wallBotY);
                }
            }

            // 4.6. Легкие потеки и пятна сырости / ржавчины на глухом бетоне (без окон)
            float weatherStepX = cellSize * 3.2f;
            float weatherStepY = cellSize * 3.0f;
            float weatherOriginX = bldgX + parapetW;
            int wxColStart = std::max(0, static_cast<int>(std::floor((cullMinX - weatherOriginX) / weatherStepX)) - 1);
            int wxColEnd = static_cast<int>(std::ceil((wallRightX - weatherOriginX) / weatherStepX));
            int wyRowStart = static_cast<int>(std::floor((wallTopY - yardY) / weatherStepY)) - 1;
            int wyRowEnd = static_cast<int>(std::ceil((wallBotY - yardY) / weatherStepY));

            for (int wyRow = wyRowStart; wyRow <= wyRowEnd; ++wyRow) {
                for (int wxCol = wxColStart; wxCol <= wxColEnd; ++wxCol) {
                    uint32_t wh = hash2D(wxCol, wyRow, 0x50B137u);
                    uint32_t wType = wh % 100u;
                    if (wType >= 45u) {
                        continue;
                    }

                    float wx = weatherOriginX + (static_cast<float>(wxCol) + 0.15f + 0.65f * rand01(wh >> 3)) * weatherStepX;
                    float wy = yardY + (static_cast<float>(wyRow) + 0.15f + 0.65f * rand01(wh >> 7)) * weatherStepY;

                    if (wType < 26u) {
                        float patchW = cellSize * (0.9f + 0.9f * rand01(wh >> 11));
                        float patchH = cellSize * (0.6f + 0.7f * rand01(wh >> 15));
                        if (!isRectVisible(wx, wy, patchW, patchH, cullMinX, cullMinY, wallRightX, cullMaxY)) {
                            continue;
                        }

                        bool overlapsPortal = false;
                        for (const auto& p : portals) {
                            float pad = cellSize * 0.6f;
                            bool overlapX = (wx + patchW >= p.mouthX - pad && wx <= p.endX + pad);
                            bool overlapY = (wy + patchH >= p.topY - pad && wy <= p.botY + pad);
                            if (overlapX && overlapY) {
                                overlapsPortal = true;
                                break;
                            }
                        }
                        if (overlapsPortal) {
                            continue;
                        }

                        glm::vec4 dampCol(0.16f, 0.17f, 0.18f, 0.16f);
                        if ((wh & 1u) == 1u) {
                            dampCol = glm::vec4(0.22f, 0.21f, 0.19f, 0.13f);
                        }
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(wx, wy), glm::vec2(patchW, patchH), 0.0f, dampCol);
                    } else {
                        float streakW = 3.0f + static_cast<float>((wh >> 12) % 5u);
                        float streakH = cellSize * (0.8f + 1.2f * rand01(wh >> 16));
                        if (!isRectVisible(wx, wy, streakW, streakH, cullMinX, cullMinY, wallRightX, cullMaxY)) {
                            continue;
                        }

                        bool overlapsPortal = false;
                        for (const auto& p : portals) {
                            float pad = cellSize * 0.6f;
                            bool overlapX = (wx + streakW >= p.mouthX - pad && wx <= p.endX + pad);
                            bool overlapY = (wy + streakH >= p.topY - pad && wy <= p.botY + pad);
                            if (overlapX && overlapY) {
                                overlapsPortal = true;
                                break;
                            }
                        }
                        if (overlapsPortal) {
                            continue;
                        }

                        glm::vec4 streakCol(0.14f, 0.15f, 0.16f, 0.24f);
                        if ((wh & 1u) == 0u) {
                            streakCol = glm::vec4(0.32f, 0.19f, 0.12f, 0.22f);
                        }
                        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(wx, wy), glm::vec2(streakW, streakH), 0.0f, streakCol);
                        renderer->drawSpriteRGBA(
                            whiteTexture,
                            glm::vec2(wx + 0.5f, wy + streakH * 0.45f),
                            glm::vec2(std::max(1.5f, streakW * 0.55f), streakH * 0.55f),
                            0.0f,
                            glm::vec4(streakCol.r, streakCol.g, streakCol.b, streakCol.a * 0.55f)
                        );
                    }
                }
            }
        }
    }

    // =========================================================================
    // 5. НИЖНИЙ СЛОЙ ПОРТАЛА ВХОДА В ШАХТУ (ПОД ВРАГАМИ)
    // =========================================================================
    for (const auto& p : portals) {
        float visPad = cellSize * 1.2f;
        if (!isRectVisible(p.mouthX - visPad, p.topY - visPad, (p.endX - p.mouthX) + visPad * 2.0f, p.openingH + visPad * 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
            continue;
        }

        float tunnelW = p.endX - p.mouthX;

        // 5.1. Бетонный подъездной пандус / порог перед проемом
        float apronX = p.mouthX - cellSize * 0.55f;
        float apronW = cellSize * 0.70f;
        float apronPadY = cellSize * 0.15f;
        renderer->drawSprite(whiteTexture, glm::vec2(apronX, p.topY - apronPadY), glm::vec2(apronW, p.openingH + apronPadY * 2.0f), 0.0f, glm::vec3(0.25f, 0.26f, 0.27f));
        renderer->drawSprite(whiteTexture, glm::vec2(apronX, p.topY - apronPadY), glm::vec2(2.5f, p.openingH + apronPadY * 2.0f), 0.0f, glm::vec3(0.38f, 0.40f, 0.42f));

        // 5.2. Глубокий темный тоннель с плавным уходом пола в кромешную тьму
        renderer->drawSprite(whiteTexture, glm::vec2(p.mouthX, p.topY), glm::vec2(tunnelW, p.openingH), 0.0f, glm::vec3(0.14f, 0.145f, 0.155f));
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(p.mouthX + tunnelW * 0.12f, p.topY), glm::vec2(tunnelW * 0.88f, p.openingH), 0.0f, glm::vec4(0.05f, 0.052f, 0.058f, 0.62f));
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(p.mouthX + tunnelW * 0.26f, p.topY), glm::vec2(tunnelW * 0.74f, p.openingH), 0.0f, glm::vec4(0.02f, 0.02f, 0.025f, 0.80f));
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(p.mouthX + tunnelW * 0.42f, p.topY), glm::vec2(tunnelW * 0.58f, p.openingH), 0.0f, glm::vec4(0.01f, 0.01f, 0.012f, 0.96f));

        // Внутренние теневые откосы стен тоннеля сверху и снизу
        float wallEdgeH = std::max(6.0f, cellSize * 0.14f);
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(p.mouthX, p.topY), glm::vec2(tunnelW, wallEdgeH), 0.0f, glm::vec4(0.01f, 0.01f, 0.015f, 0.72f));
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(p.mouthX, p.botY - wallEdgeH), glm::vec2(tunnelW, wallEdgeH), 0.0f, glm::vec4(0.01f, 0.01f, 0.015f, 0.72f));

        // 5.3. Шахтные рельсы и шпалы, уходящие в темноту ствола
        float railGauge = std::max(22.0f, cellSize * 0.42f);
        float railTopY = p.centerY - railGauge * 0.5f;
        float railStartX = p.mouthX - cellSize * 0.45f;
        float railEndX = std::min(p.endX - cellSize * 0.25f, p.coverStartX + cellSize * 0.75f);
        float railLen = std::max(cellSize, railEndX - railStartX);

        float sleeperSpacing = std::max(16.0f, cellSize * 0.32f);
        float sleeperW = std::max(4.5f, cellSize * 0.08f);
        for (float sx = railStartX; sx < railStartX + railLen; sx += sleeperSpacing) {
            float t = std::clamp((sx - railStartX) / railLen, 0.0f, 1.0f);
            float fade = 1.0f - t * 0.92f;
            renderer->drawSpriteRGBA(
                whiteTexture,
                glm::vec2(sx, railTopY - 4.0f),
                glm::vec2(sleeperW, railGauge + 8.0f),
                0.0f,
                glm::vec4(0.18f, 0.15f, 0.12f, fade * 0.85f)
            );
        }

        const int railSegments = 5;
        float segLen = railLen / static_cast<float>(railSegments);
        float railThickness = std::max(3.0f, cellSize * 0.055f);
        for (int s = 0; s < railSegments; ++s) {
            float sx = railStartX + static_cast<float>(s) * segLen;
            float t = (static_cast<float>(s) + 0.5f) / static_cast<float>(railSegments);
            float fade = std::max(0.05f, 1.0f - t * 0.92f);
            glm::vec4 railCol(0.55f, 0.58f, 0.62f, fade * 0.90f);
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(sx, railTopY), glm::vec2(segLen + 0.5f, railThickness), 0.0f, railCol);
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(sx, railTopY + railGauge), glm::vec2(segLen + 0.5f, railThickness), 0.0f, railCol);
        }

        // 5.4. Капитальные бетонные боковые устои портала (фланкируют тоннель сверху и снизу)
        float pierH = std::max(22.0f, cellSize * 0.42f);
        float pierW = tunnelW + cellSize * 0.25f;
        float pierX = p.mouthX - cellSize * 0.15f;
        renderer->drawSprite(whiteTexture, glm::vec2(pierX, p.topY - pierH), glm::vec2(pierW, pierH), 0.0f, glm::vec3(0.36f, 0.37f, 0.39f));
        renderer->drawSprite(whiteTexture, glm::vec2(pierX, p.topY - pierH), glm::vec2(pierW, 2.5f), 0.0f, glm::vec3(0.52f, 0.54f, 0.57f));
        renderer->drawSprite(whiteTexture, glm::vec2(pierX, p.topY - 3.0f), glm::vec2(pierW, 3.0f), 0.0f, glm::vec3(0.18f, 0.19f, 0.21f));

        renderer->drawSprite(whiteTexture, glm::vec2(pierX, p.botY), glm::vec2(pierW, pierH), 0.0f, glm::vec3(0.36f, 0.37f, 0.39f));
        renderer->drawSprite(whiteTexture, glm::vec2(pierX, p.botY), glm::vec2(pierW, 3.0f), 0.0f, glm::vec3(0.48f, 0.50f, 0.53f));
        renderer->drawSprite(whiteTexture, glm::vec2(pierX, p.botY + pierH - 3.0f), glm::vec2(pierW, 3.0f), 0.0f, glm::vec3(0.18f, 0.19f, 0.21f));
    }
}

void BackgroundRenderer::renderMineEntranceForeground(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    const std::vector<glm::ivec2>* baseCells
) {
    if (!renderer || !whiteTexture || !grid) return;

    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    int gridW_tiles = grid->getWidth();
    int gridH_tiles = grid->getHeight();
    float gridW = static_cast<float>(gridW_tiles) * cellSize;
    float gridH = static_cast<float>(gridH_tiles) * cellSize;

    float yardX = gridOffset.x;
    float yardY = gridOffset.y;
    float yardW = gridW;
    float yardH = gridH;

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 1.5f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    // =========================================================================
    // 1. ОПТИМИЗИРОВАННЫЙ 2.5D ЗАБОР (С ОТСЕЧЕНИЕМ ПО КАМЕРЕ И КОНТАКТНОЙ ТЕНЬЮ)
    // =========================================================================
    auto drawFence25D = [&](float groundY, bool isTopFence) {
        float fenceH = cellSize * 0.46f;
        float postSpacing = cellSize * 2.0f;
        float postW = std::max(11.0f, cellSize * 0.20f);
        float postH = fenceH + std::max(4.0f, cellSize * 0.07f);
        float postCapH = std::max(4.0f, cellSize * 0.08f);

        float fenceStartX = yardX;
        float fenceEndX = yardX + yardW;
        float facadeTopY = groundY - fenceH;
        float softH = std::max(16.0f, cellSize * 0.28f);
        float contactH = std::max(5.0f, cellSize * 0.08f);
        float fullTopY = groundY - postH - postCapH;
        float fullBotY = groundY + softH;

        if (fullBotY < cullMinY || fullTopY > cullMaxY) {
            return;
        }
        if (fenceEndX + 12.0f < cullMinX || fenceStartX - 12.0f > cullMaxX) {
            return;
        }

        float visStartX = std::max(fenceStartX, cullMinX);
        float visEndX = std::min(fenceEndX, cullMaxX);
        float visLen = visEndX - visStartX;
        if (visLen <= 0.0f) {
            return;
        }

        // Контактная и мягкая падающая тень у основания забора
        if (!isTopFence) {
            float backShadowY = yardY + yardH - cellSize * 0.10f;
            renderer->drawSpriteRGBA(
                whiteTexture,
                glm::vec2(visStartX, backShadowY),
                glm::vec2(visLen, std::max(6.0f, cellSize * 0.14f)),
                0.0f,
                glm::vec4(0.06f, 0.05f, 0.035f, 0.20f)
            );
        }
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(visStartX, groundY),
            glm::vec2(visLen, softH),
            0.0f,
            glm::vec4(0.06f, 0.05f, 0.035f, 0.22f)
        );
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(visStartX, groundY - contactH * 0.35f),
            glm::vec2(visLen, contactH),
            0.0f,
            glm::vec4(0.05f, 0.04f, 0.03f, 0.42f)
        );

        // Горизонтальные лаги (ригели)
        float railH = std::max(5.0f, cellSize * 0.085f);
        float topRailY = facadeTopY + fenceH * 0.22f;
        float botRailY = facadeTopY + fenceH * 0.72f;

        auto drawHorizontalRail = [&](float ry) {
            renderer->drawSprite(whiteTexture, glm::vec2(visStartX, ry), glm::vec2(visLen, railH), 0.0f, glm::vec3(0.25f, 0.27f, 0.29f));
            renderer->drawSprite(whiteTexture, glm::vec2(visStartX, ry), glm::vec2(visLen, 1.5f), 0.0f, glm::vec3(0.55f, 0.58f, 0.62f));
            renderer->drawSprite(whiteTexture, glm::vec2(visStartX, ry + railH - 1.5f), glm::vec2(visLen, 1.5f), 0.0f, glm::vec3(0.12f, 0.13f, 0.14f));
        };
        drawHorizontalRail(topRailY);
        drawHorizontalRail(botRailY);

        // Вертикальные секции профлиста с редкими ребрами жесткости
        int numIntervals = static_cast<int>(std::ceil(yardW / postSpacing));
        int iStart = std::max(0, static_cast<int>(std::floor((cullMinX - fenceStartX) / postSpacing)));
        int iEnd = std::min(numIntervals - 1, static_cast<int>(std::ceil((cullMaxX - fenceStartX) / postSpacing)));

        float ribStep = std::max(14.0f, cellSize * 0.28f);
        for (int i = iStart; i <= iEnd; ++i) {
            float secX = fenceStartX + static_cast<float>(i) * postSpacing + postW;
            float nextPostX = fenceStartX + static_cast<float>(i + 1) * postSpacing;
            if (nextPostX > fenceEndX) {
                nextPostX = fenceEndX;
            }
            float secW = nextPostX - secX;
            if (secW <= 0.0f) {
                continue;
            }

            float panelTop = topRailY - 2.0f;
            float panelH = (botRailY + railH + 2.0f) - panelTop;
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(secX, panelTop), glm::vec2(secW, panelH), 0.0f, glm::vec4(0.18f, 0.20f, 0.22f, 0.34f));

            for (float mx = secX + ribStep * 0.5f; mx < secX + secW; mx += ribStep) {
                renderer->drawSpriteRGBA(whiteTexture, glm::vec2(mx, panelTop), glm::vec2(1.8f, panelH), 0.0f, glm::vec4(0.40f, 0.43f, 0.47f, 0.52f));
            }
        }

        // Вертикальные опорные железобетонные столбы (только видимые)
        int pStart = std::max(0, static_cast<int>(std::floor((cullMinX - fenceStartX - postW) / postSpacing)));
        int pEnd = std::min(numIntervals, static_cast<int>(std::ceil((cullMaxX - fenceStartX) / postSpacing)));

        for (int p = pStart; p <= pEnd; ++p) {
            float postX = fenceStartX + static_cast<float>(p) * postSpacing;
            if (postX > fenceEndX) {
                postX = fenceEndX;
            }
            float py = groundY - postH;
            renderer->drawSprite(whiteTexture, glm::vec2(postX, py), glm::vec2(postW, postH), 0.0f, glm::vec3(0.44f, 0.46f, 0.48f));
            renderer->drawSprite(whiteTexture, glm::vec2(postX, py), glm::vec2(2.5f, postH), 0.0f, glm::vec3(0.65f, 0.67f, 0.70f));
            renderer->drawSprite(whiteTexture, glm::vec2(postX + postW - 3.0f, py), glm::vec2(3.0f, postH), 0.0f, glm::vec3(0.22f, 0.23f, 0.24f));

            float capW = postW + 4.0f;
            float capX = postX - 2.0f;
            float capY = py - postCapH + 1.0f;
            renderer->drawSprite(whiteTexture, glm::vec2(capX, capY), glm::vec2(capW, postCapH), 0.0f, glm::vec3(0.50f, 0.52f, 0.54f));
            renderer->drawSprite(whiteTexture, glm::vec2(capX, capY), glm::vec2(capW, 1.5f), 0.0f, glm::vec3(0.72f, 0.75f, 0.78f));

            if (postX >= fenceEndX) {
                break;
            }
        }
    };

    drawFence25D(yardY, true);
    drawFence25D(yardY + yardH + cellSize * 0.46f, false);

    // =========================================================================
    // 2. ВЕРХНИЙ СЛОЙ ПОРТАЛА (ПОВЕРХ ВРАГОВ): КОЗЫРЕК И БЕТОННОЕ ПЕРЕКРЫТИЕ
    // =========================================================================
    std::vector<MinePortalGeom> portals = collectMinePortals(grid, baseCells);
    for (const auto& p : portals) {
        float canopyPadY = std::max(22.0f, cellSize * 0.42f);
        float roofTopY = p.topY - canopyPadY;
        float roofBotY = p.botY + canopyPadY;
        float roofH = roofBotY - roofTopY;

        if (!isRectVisible(p.mouthX - cellSize, roofTopY - cellSize * 0.5f, (p.endX - p.mouthX) + cellSize * 1.5f, roofH + cellSize, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
            continue;
        }

        // 2.1. Затемняющая завеса устья тоннеля (враг темнеет, заходя под козырек)
        float throatStartX = p.mouthX + cellSize * 0.12f;
        float throatW = std::max(0.0f, p.coverStartX - throatStartX);
        if (throatW > 2.0f) {
            float stripW = throatW / 3.0f;
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(throatStartX, p.topY), glm::vec2(stripW + 0.5f, p.openingH), 0.0f, glm::vec4(0.015f, 0.015f, 0.02f, 0.22f));
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(throatStartX + stripW, p.topY), glm::vec2(stripW + 0.5f, p.openingH), 0.0f, glm::vec4(0.015f, 0.015f, 0.02f, 0.48f));
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(throatStartX + stripW * 2.0f, p.topY), glm::vec2(stripW + 0.5f, p.openingH), 0.0f, glm::vec4(0.01f, 0.01f, 0.015f, 0.78f));
        }

        // 2.2. Передние обрамляющие боковые перемычки над и под проемом у входа
        float frontPierX = p.mouthX - cellSize * 0.15f;
        float frontPierW = std::max(0.0f, p.coverStartX - frontPierX);
        if (frontPierW > 1.0f) {
            renderer->drawSprite(whiteTexture, glm::vec2(frontPierX, roofTopY), glm::vec2(frontPierW, canopyPadY), 0.0f, glm::vec3(0.36f, 0.37f, 0.39f));
            renderer->drawSprite(whiteTexture, glm::vec2(frontPierX, roofTopY), glm::vec2(frontPierW, 2.5f), 0.0f, glm::vec3(0.54f, 0.56f, 0.59f));
            renderer->drawSprite(whiteTexture, glm::vec2(frontPierX, p.topY - 3.0f), glm::vec2(frontPierW, 3.0f), 0.0f, glm::vec3(0.18f, 0.19f, 0.20f));

            renderer->drawSprite(whiteTexture, glm::vec2(frontPierX, p.botY), glm::vec2(frontPierW, canopyPadY), 0.0f, glm::vec3(0.36f, 0.37f, 0.39f));
            renderer->drawSprite(whiteTexture, glm::vec2(frontPierX, p.botY), glm::vec2(frontPierW, 2.5f), 0.0f, glm::vec3(0.50f, 0.52f, 0.55f));
            renderer->drawSprite(whiteTexture, glm::vec2(frontPierX, roofBotY - 3.0f), glm::vec2(frontPierW, 3.0f), 0.0f, glm::vec3(0.18f, 0.19f, 0.20f));
        }

        // 2.3. Массивный непрозрачный бетонный козырек и перекрытие портала (ПОЛНОСТЬЮ СКРЫВАЕТ ВРАГОВ И БАЗУ)
        float coverW = p.endX - p.coverStartX;
        float fasciaW = std::min(coverW * 0.35f, std::max(24.0f, cellSize * 0.42f));

        // Глубокая падающая тень от нависающего козырька влево на проем
        float canopyShadowW = std::max(14.0f, cellSize * 0.28f);
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(p.coverStartX - canopyShadowW, p.topY),
            glm::vec2(canopyShadowW, p.openingH),
            0.0f,
            glm::vec4(0.01f, 0.01f, 0.015f, 0.65f)
        );

        // Передняя несущая балка-козырек (фасция портала)
        renderer->drawSprite(whiteTexture, glm::vec2(p.coverStartX, roofTopY), glm::vec2(fasciaW, roofH), 0.0f, glm::vec3(0.38f, 0.40f, 0.42f));
        renderer->drawSprite(whiteTexture, glm::vec2(p.coverStartX, roofTopY), glm::vec2(3.0f, roofH), 0.0f, glm::vec3(0.56f, 0.58f, 0.61f));
        renderer->drawSprite(whiteTexture, glm::vec2(p.coverStartX + fasciaW - 3.0f, roofTopY), glm::vec2(3.0f, roofH), 0.0f, glm::vec3(0.18f, 0.19f, 0.21f));

        // Сигнальная черно-желтая предупреждающая полоса по кромке козырька над проемом
        float stripeW = std::max(8.0f, fasciaW * 0.40f);
        float stripeX = p.coverStartX + fasciaW * 0.24f;
        float stripeStep = std::max(14.0f, cellSize * 0.26f);
        for (float sy = p.topY + 4.0f; sy < p.botY - 4.0f; sy += stripeStep) {
            float rem = (p.botY - 4.0f) - sy;
            float yellowH = std::min(stripeStep * 0.54f, rem);
            if (yellowH > 1.0f) {
                renderer->drawSprite(whiteTexture, glm::vec2(stripeX, sy), glm::vec2(stripeW, yellowH), 0.0f, glm::vec3(0.82f, 0.66f, 0.14f));
            }
            float blackStart = sy + yellowH;
            float blackH = std::min(stripeStep * 0.46f, (p.botY - 4.0f) - blackStart);
            if (blackH > 1.0f) {
                renderer->drawSprite(whiteTexture, glm::vec2(stripeX, blackStart), glm::vec2(stripeW, blackH), 0.0f, glm::vec3(0.11f, 0.11f, 0.12f));
            }
        }

        // Непрозрачная железобетонная плита крыши портала (перекрывает тоннель и клетку Base)
        float deckX = p.coverStartX + fasciaW;
        float deckW = std::max(0.0f, p.endX - deckX);
        if (deckW > 0.0f) {
            renderer->drawSprite(whiteTexture, glm::vec2(deckX, roofTopY), glm::vec2(deckW, roofH), 0.0f, glm::vec3(0.32f, 0.33f, 0.35f));

            // Поперечные ребра жесткости сверху и снизу крыши портала
            float beamH = canopyPadY;
            renderer->drawSprite(whiteTexture, glm::vec2(deckX, roofTopY), glm::vec2(deckW, beamH), 0.0f, glm::vec3(0.39f, 0.41f, 0.43f));
            renderer->drawSprite(whiteTexture, glm::vec2(deckX, roofTopY), glm::vec2(deckW, 2.5f), 0.0f, glm::vec3(0.55f, 0.57f, 0.60f));
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(deckX, p.topY), glm::vec2(deckW, 8.0f), 0.0f, glm::vec4(0.05f, 0.06f, 0.07f, 0.45f));

            renderer->drawSprite(whiteTexture, glm::vec2(deckX, p.botY), glm::vec2(deckW, beamH), 0.0f, glm::vec3(0.39f, 0.41f, 0.43f));
            renderer->drawSprite(whiteTexture, glm::vec2(deckX, p.botY), glm::vec2(deckW, 2.5f), 0.0f, glm::vec3(0.52f, 0.54f, 0.57f));
            renderer->drawSprite(whiteTexture, glm::vec2(deckX, roofBotY - 3.0f), glm::vec2(deckW, 3.0f), 0.0f, glm::vec3(0.18f, 0.19f, 0.21f));

            // Продольный деформационный шов посередине плиты перекрытия
            renderer->drawSprite(whiteTexture, glm::vec2(deckX, p.centerY - 1.5f), glm::vec2(deckW, 3.0f), 0.0f, glm::vec3(0.20f, 0.21f, 0.23f));
            renderer->drawSprite(whiteTexture, glm::vec2(deckX, p.centerY + 1.5f), glm::vec2(deckW, 1.5f), 0.0f, glm::vec3(0.43f, 0.45f, 0.47f));

            // Вентиляционный короб / технический люк на крыше портала
            if (deckW > cellSize * 1.1f) {
                float ventW = std::min(deckW * 0.45f, cellSize * 0.90f);
                float ventH = std::min(p.openingH * 0.42f, cellSize * 0.72f);
                float ventX = deckX + cellSize * 0.45f;
                float ventY = p.centerY - ventH * 0.5f;
                renderer->drawSpriteRGBA(whiteTexture, glm::vec2(ventX + 4.0f, ventY + 4.0f), glm::vec2(ventW, ventH), 0.0f, glm::vec4(0.05f, 0.06f, 0.07f, 0.45f));
                renderer->drawSprite(whiteTexture, glm::vec2(ventX, ventY), glm::vec2(ventW, ventH), 0.0f, glm::vec3(0.26f, 0.27f, 0.29f));
                renderer->drawSprite(whiteTexture, glm::vec2(ventX, ventY), glm::vec2(ventW, 2.0f), 0.0f, glm::vec3(0.48f, 0.50f, 0.53f));
                for (int louver = 0; louver < 3; ++louver) {
                    float ly = ventY + ventH * (0.24f + static_cast<float>(louver) * 0.24f);
                    renderer->drawSprite(whiteTexture, glm::vec2(ventX + 5.0f, ly), glm::vec2(ventW - 10.0f, 3.0f), 0.0f, glm::vec3(0.11f, 0.12f, 0.13f));
                }
            }
        }

        // 2.4. Взрывозащищенный рудничный светильник на кромке козырька
        float lampX = p.coverStartX - 5.0f;
        float lampY = p.topY + cellSize * 0.18f;
        renderer->drawSprite(whiteTexture, glm::vec2(lampX - 4.0f, lampY), glm::vec2(10.0f, 12.0f), 0.0f, glm::vec3(0.24f, 0.26f, 0.28f));
        renderer->drawSprite(whiteTexture, glm::vec2(lampX - 5.0f, lampY + 3.0f), glm::vec2(6.0f, 6.0f), 0.0f, glm::vec3(1.0f, 0.80f, 0.34f));
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(lampX - 42.0f, lampY - 18.0f), glm::vec2(48.0f, 52.0f), 0.0f, glm::vec4(1.0f, 0.78f, 0.30f, 0.11f));
    }
}

bool BackgroundRenderer::isCinnabarMine(const std::string& backgroundType) {
    return (backgroundType == "cinnabar_mine" || backgroundType == "cinnabar");
}

bool BackgroundRenderer::usesSidePocketsByDefault(const std::string& backgroundType) {
    if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        return true;
    }
    if (isCinnabarMine(backgroundType)) {
        return true;
    }
    return false;
}

std::string BackgroundRenderer::nextBackgroundPreset(const std::string& currentBackground) {
    if (currentBackground == "default") {
        return "pipes_canal";
    }
    if (currentBackground == "pipes_canal" || currentBackground == "channel") {
        return "mine_entrance";
    }
    if (currentBackground == "mine_entrance" || currentBackground == "mine") {
        return "cinnabar_mine";
    }
    return "default";
}

std::string BackgroundRenderer::getBackgroundToast(const std::string& backgroundType) {
    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        return "Фон: Дюкеры Канала";
    }
    if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        return "Фон: Вход в шахту";
    }
    if (isCinnabarMine(backgroundType)) {
        return "Фон: Киноварный штрек";
    }
    return "Фон: Стандартный";
}

std::string BackgroundRenderer::getEditorToolbarBgLabel(const std::string& backgroundType, bool isCompact) {
    if (isCompact) {
        if (backgroundType == "mine_entrance" || backgroundType == "mine") {
            return "[ Шахта ]";
        }
        if (backgroundType == "pipes_canal" || backgroundType == "channel") {
            return "[ Канал ]";
        }
        if (isCinnabarMine(backgroundType)) {
            return "[ Штрек ]";
        }
        return "[ Фон ]";
    }

    if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        return "[ Фон: Шахта ]";
    }
    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        return "[ Фон: Канал ]";
    }
    if (isCinnabarMine(backgroundType)) {
        return "[ Фон: Киноварь ]";
    }
    return "[ Фон: Стандарт ]";
}

std::string BackgroundRenderer::getEditorGroundName(const std::string& backgroundType) {
    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        return "Асфальт";
    }
    if (isCinnabarMine(backgroundType)) {
        return "Штрек";
    }
    return "Грунт";
}

std::string BackgroundRenderer::getEditorWallName(const std::string& backgroundType) {
    if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        return "Ящики";
    }
    if (isCinnabarMine(backgroundType)) {
        return "Киноварь";
    }
    return "Стена";
}

std::string BackgroundRenderer::getEditorPlatformName(const std::string& backgroundType) {
    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        return "Трава/Обочина";
    }
    if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        return "Сланец";
    }
    if (isCinnabarMine(backgroundType)) {
        return "Уступ";
    }
    return "Платформа";
}

// =============================================================================
// ТЕМА: "КИНОВАРНЫЙ ШТРЕК" (cinnabar_mine) — Подземный ртутный горизонт
// Формат трассы-коридора (сверху и снизу массив породы с крепью, киноварью,
// ретортой и водоотливом; слева и справа открытая выработка под трассу)
// =============================================================================
void BackgroundRenderer::renderCinnabarMineSubstrate(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect
) {
    if (!renderer || !whiteTexture) return;

    glm::vec2 gridOffset(0.0f);
    float cellSize = 64.0f;
    if (grid != nullptr) {
        gridOffset = grid->getOffset();
        cellSize = grid->getCellSize();
    }

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    float totalW = cullMaxX - cullMinX;
    float totalH = cullMaxY - cullMinY;

    // 1. Глубокий массив мокрого донецкого углистого сланца и кварцито-песчаника
    renderer->drawSprite(
        whiteTexture,
        glm::vec2(cullMinX, cullMinY),
        glm::vec2(totalW, totalH),
        0.0f,
        glm::vec3(0.072f, 0.078f, 0.098f)
    );

    // 2. Слоистые пласты породы с тонкими тектоническими швами (привязаны к gridOffset.y)
    float strataH = std::max(38.0f, cellSize * 0.85f);
    int bandStart = static_cast<int>(std::floor((cullMinY - gridOffset.y) / strataH));
    int bandEnd = static_cast<int>(std::ceil((cullMaxY - gridOffset.y) / strataH));
    for (int i = bandStart; i <= bandEnd; ++i) {
        float bY = gridOffset.y + static_cast<float>(i) * strataH;
        int modIdx = ((i % 10) + 10) % 10;
        float factor = static_cast<float>(modIdx) / 10.0f;
        float wave = std::sin(factor * 6.28318f) * 0.012f;
        // Легкий красновато-железистый подтон в отдельных прослоях песчаника
        float ironTint = (modIdx == 3 || modIdx == 7) ? 0.018f : 0.0f;
        glm::vec3 strataCol(
            0.078f + 0.022f * factor + wave + ironTint,
            0.082f + 0.020f * factor + wave * 0.8f,
            0.102f + 0.026f * factor + wave * 1.1f
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, bY),
            glm::vec2(totalW, strataH + 1.0f),
            0.0f,
            strataCol
        );
        // Тонкий темный шов напластования
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, bY),
            glm::vec2(totalW, std::max(1.5f, cellSize * 0.025f)),
            0.0f,
            glm::vec3(0.045f, 0.048f, 0.062f)
        );
    }

    // 3. Природные багрово-алые прожилки киновари (HgS) и капли самородной ртути в массиве породы
    float clusterW = std::max(160.0f, cellSize * 3.5f);
    float clusterH = std::max(130.0f, cellSize * 2.8f);
    int cxStart = static_cast<int>(std::floor((cullMinX - gridOffset.x) / clusterW)) - 1;
    int cxEnd = static_cast<int>(std::ceil((cullMaxX - gridOffset.x) / clusterW)) + 1;
    int cyStart = static_cast<int>(std::floor((cullMinY - gridOffset.y) / clusterH)) - 1;
    int cyEnd = static_cast<int>(std::ceil((cullMaxY - gridOffset.y) / clusterH)) + 1;

    for (int cy = cyStart; cy <= cyEnd; ++cy) {
        for (int cx = cxStart; cx <= cxEnd; ++cx) {
            uint32_t hash = static_cast<uint32_t>(cx * 73856093) ^ static_cast<uint32_t>(cy * 19349663);
            if ((hash % 10) >= 7) {
                continue; // Не в каждом секторе, чтобы порода выглядела естественно
            }

            float baseX = gridOffset.x + static_cast<float>(cx) * clusterW + static_cast<float>((hash % 45) + 15) * 0.01f * clusterW;
            float baseY = gridOffset.y + static_cast<float>(cy) * clusterH + static_cast<float>(((hash / 64) % 45) + 15) * 0.01f * clusterH;

            if (!isRectVisible(baseX - cellSize, baseY - cellSize, cellSize * 2.5f, cellSize * 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
                continue;
            }

            float angle = ((hash / 512) % 2 == 0) ? -26.0f : 31.0f;
            float veinLen = cellSize * (0.95f + static_cast<float>((hash / 1024) % 45) * 0.01f);
            float veinThick = std::max(3.0f, cellSize * 0.09f);

            // Темная расселина в сланце
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(baseX - 2.0f, baseY - 2.0f),
                glm::vec2(veinLen + 4.0f, veinThick + 4.0f),
                angle,
                glm::vec3(0.035f, 0.025f, 0.038f)
            );
            // Основное тело жилы киновари (густой карминно-красный)
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(baseX, baseY),
                glm::vec2(veinLen, veinThick),
                angle,
                glm::vec3(0.56f, 0.10f, 0.13f)
            );
            // Яркие кристаллические грани алой киновари
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(baseX + veinLen * 0.18f, baseY + 1.0f),
                glm::vec2(veinLen * 0.48f, std::max(2.0f, veinThick * 0.52f)),
                angle,
                glm::vec3(0.88f, 0.20f, 0.22f)
            );

            // Боковой отросток жилы
            float branchLen = veinLen * 0.45f;
            float branchAngle = (angle > 0.0f) ? -18.0f : 22.0f;
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(baseX + veinLen * 0.35f, baseY + veinThick * 0.6f),
                glm::vec2(branchLen, std::max(2.0f, veinThick * 0.65f)),
                branchAngle,
                glm::vec3(0.66f, 0.13f, 0.16f)
            );

            // Серебристые капельки выступившей жидкой ртути на сколе жилы
            float dropSz = std::max(2.5f, cellSize * 0.055f);
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(baseX + veinLen * 0.28f, baseY - dropSz * 0.4f),
                glm::vec2(dropSz, dropSz),
                0.0f,
                glm::vec3(0.84f, 0.89f, 0.95f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(baseX + veinLen * 0.68f, baseY + veinThick * 0.5f),
                glm::vec2(dropSz * 0.8f, dropSz * 0.8f),
                0.0f,
                glm::vec3(0.75f, 0.81f, 0.88f)
            );
        }
    }
}

void BackgroundRenderer::renderCinnabarMineEnvironment(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    const std::vector<glm::ivec2>* /*baseCells*/
) {
    if (!renderer || !whiteTexture || !grid) return;

    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    float gridW = static_cast<float>(grid->getWidth()) * cellSize;
    float gridH = static_cast<float>(grid->getHeight()) * cellSize;

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 4.0f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    float spanW = cullMaxX - cullMinX;
    if (spanW <= 0.0f) return;

    // Параметры бортов штрека сверху и снизу игровой трассы (как трубы на 1-й карте, но подземная выработка)
    float topWallH = cellSize * 1.45f;
    float topWallY = gridOffset.y - topWallH;
    float botWallY = gridOffset.y + gridH;
    float botWallH = cellSize * 1.30f;

    float postSpacing = 3.0f * cellSize;
    float firstPostX = gridOffset.x - std::floor((gridOffset.x - cullMinX) / postSpacing) * postSpacing;

    // =========================================================================
    // 1. ВЕРХНИЙ БОРТ ШТРЕКА: порода с жилой киновари, реторта, труба СВМ, ртутный трубопровод и крепь
    // =========================================================================
    if (isRectVisible(cullMinX, topWallY - cellSize * 1.8f, spanW, topWallH + cellSize * 2.2f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
        // Падающая тень от верхнего свода штрека на полотно трассы
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(cullMinX, gridOffset.y),
            glm::vec2(spanW, std::max(10.0f, cellSize * 0.22f)),
            0.0f,
            glm::vec4(0.02f, 0.02f, 0.04f, 0.42f)
        );
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(cullMinX, gridOffset.y + cellSize * 0.14f),
            glm::vec2(spanW, std::max(8.0f, cellSize * 0.18f)),
            0.0f,
            glm::vec4(0.02f, 0.02f, 0.04f, 0.18f)
        );

        // Тесаный уступ каменной стены верхнего борта
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, topWallY),
            glm::vec2(spanW, topWallH),
            0.0f,
            glm::vec3(0.118f, 0.126f, 0.155f)
        );
        // Нижняя каменная бровка у границы сетки
        float ledgeH = std::max(6.0f, cellSize * 0.12f);
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, gridOffset.y - ledgeH),
            glm::vec2(spanW, ledgeH),
            0.0f,
            glm::vec3(0.175f, 0.182f, 0.218f)
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, topWallY),
            glm::vec2(spanW, std::max(3.0f, cellSize * 0.05f)),
            0.0f,
            glm::vec3(0.065f, 0.070f, 0.088f)
        );

        // Богатый вскрытый пласт алой киновари вдоль верхнего борта выработки
        float seamStep = cellSize * 1.5f;
        float firstSeamX = gridOffset.x - std::floor((gridOffset.x - cullMinX) / seamStep) * seamStep;
        for (float sx = firstSeamX - seamStep; sx <= cullMaxX; sx += seamStep) {
            int sIdx = static_cast<int>(std::round((sx - gridOffset.x) / seamStep));
            uint32_t sHash = static_cast<uint32_t>(sIdx) * 2654435761u;
            float offY = static_cast<float>((sHash % 24) - 12) * 0.01f * cellSize;
            float seamY = topWallY + topWallH * 0.30f + offY;
            float segW = seamStep * 0.82f;
            float segH = std::max(5.0f, cellSize * 0.13f);
            float segAngle = ((sHash % 2) == 0) ? 7.0f : -8.0f;

            renderer->drawSprite(whiteTexture, glm::vec2(sx - 2.0f, seamY - 2.0f), glm::vec2(segW + 4.0f, segH + 4.0f), segAngle, glm::vec3(0.05f, 0.03f, 0.04f));
            renderer->drawSprite(whiteTexture, glm::vec2(sx, seamY), glm::vec2(segW, segH), segAngle, glm::vec3(0.64f, 0.11f, 0.14f));
            renderer->drawSprite(whiteTexture, glm::vec2(sx + segW * 0.15f, seamY + 1.0f), glm::vec2(segW * 0.52f, segH * 0.52f), segAngle, glm::vec3(0.92f, 0.22f, 0.24f));
        }

        // Желтая гофрированная вентиляционная труба местного проветривания (СВМ) вдоль верхнего борта
        float ventH = std::max(14.0f, cellSize * 0.34f);
        float ventY = topWallY + cellSize * 0.16f;
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cullMinX, ventY + ventH), glm::vec2(spanW, std::max(5.0f, cellSize * 0.10f)), 0.0f, glm::vec4(0.02f, 0.02f, 0.03f, 0.48f));
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, ventY), glm::vec2(spanW, ventH), 0.0f, glm::vec3(0.58f, 0.44f, 0.12f));
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, ventY + ventH * 0.12f), glm::vec2(spanW, ventH * 0.38f), 0.0f, glm::vec3(0.76f, 0.60f, 0.17f));
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, ventY + ventH * 0.72f), glm::vec2(spanW, ventH * 0.28f), 0.0f, glm::vec3(0.38f, 0.28f, 0.08f));

        // Стальная магистраль перегонки жидкой ртути ниже вентиляционного рукава
        float hgPipeH = std::max(8.0f, cellSize * 0.18f);
        float hgPipeY = ventY + ventH + cellSize * 0.18f;
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(cullMinX, hgPipeY + hgPipeH), glm::vec2(spanW, std::max(4.0f, cellSize * 0.07f)), 0.0f, glm::vec4(0.02f, 0.03f, 0.04f, 0.45f));
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, hgPipeY), glm::vec2(spanW, hgPipeH), 0.0f, glm::vec3(0.30f, 0.35f, 0.42f));
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, hgPipeY + hgPipeH * 0.18f), glm::vec2(spanW, hgPipeH * 0.30f), 0.0f, glm::vec3(0.62f, 0.69f, 0.78f));

        // СЮЖЕТНЫЙ УЗЕЛ УРОВНЯ: Ртутная реторта-конденсатор у материнского кармана киновари
        // Расположена над левой частью поля (вне строительной сетки), объясняя открытие Ртутной башни
        float retortW = cellSize * 3.2f;
        float retortH = cellSize * 1.50f;
        float retortX = gridOffset.x + std::min(cellSize * 1.5f, std::max(0.0f, gridW - retortW) * 0.25f);
        float retortY = topWallY - retortH * 0.62f;
        if (isRectVisible(retortX - cellSize * 0.5f, retortY - cellSize * 0.4f, retortW + cellSize, retortH + cellSize, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
            // Вырубленная ниша в скале с крупной друзой кристаллов киновари
            renderer->drawSprite(whiteTexture, glm::vec2(retortX - cellSize * 0.25f, retortY - cellSize * 0.18f), glm::vec2(retortW + cellSize * 0.5f, retortH + cellSize * 0.28f), 0.0f, glm::vec3(0.055f, 0.058f, 0.075f));
            renderer->drawSprite(whiteTexture, glm::vec2(retortX - cellSize * 0.18f, retortY - cellSize * 0.10f), glm::vec2(cellSize * 1.25f, cellSize * 0.85f), -12.0f, glm::vec3(0.62f, 0.10f, 0.14f));
            renderer->drawSprite(whiteTexture, glm::vec2(retortX - cellSize * 0.05f, retortY), glm::vec2(cellSize * 0.85f, cellSize * 0.48f), -8.0f, glm::vec3(0.92f, 0.22f, 0.24f));
            renderer->drawSprite(whiteTexture, glm::vec2(retortX + cellSize * 0.22f, retortY - cellSize * 0.08f), glm::vec2(cellSize * 0.45f, cellSize * 0.30f), 18.0f, glm::vec3(0.98f, 0.34f, 0.32f));

            // Бетонный фундамент реторты
            float padY = retortY + retortH - cellSize * 0.26f;
            renderer->drawSprite(whiteTexture, glm::vec2(retortX - cellSize * 0.10f, padY), glm::vec2(retortW + cellSize * 0.20f, cellSize * 0.32f), 0.0f, glm::vec3(0.22f, 0.23f, 0.26f));
            renderer->drawSprite(whiteTexture, glm::vec2(retortX - cellSize * 0.10f, padY), glm::vec2(retortW + cellSize * 0.20f, 3.0f), 0.0f, glm::vec3(0.36f, 0.38f, 0.42f));

            // Чугунный барабан обжиговой печи-реторты слева
            float drumX = retortX + cellSize * 0.12f;
            float drumY = retortY + cellSize * 0.18f;
            float drumW = cellSize * 1.15f;
            float drumH = cellSize * 1.02f;
            renderer->drawSprite(whiteTexture, glm::vec2(drumX, drumY), glm::vec2(drumW, drumH), 0.0f, glm::vec3(0.18f, 0.20f, 0.24f));
            renderer->drawSprite(whiteTexture, glm::vec2(drumX + 4.0f, drumY + 4.0f), glm::vec2(drumW - 8.0f, drumH - 8.0f), 0.0f, glm::vec3(0.27f, 0.30f, 0.35f));
            // Узкое топочное окошко с тлеющим жаром обжига руды
            renderer->drawSprite(whiteTexture, glm::vec2(drumX + drumW * 0.22f, drumY + drumH * 0.65f), glm::vec2(drumW * 0.56f, drumH * 0.18f), 0.0f, glm::vec3(0.10f, 0.06f, 0.05f));
            renderer->drawSprite(whiteTexture, glm::vec2(drumX + drumW * 0.26f, drumY + drumH * 0.68f), glm::vec2(drumW * 0.48f, drumH * 0.12f), 0.0f, glm::vec3(0.95f, 0.38f, 0.12f));
            // Круглый манометр давления паров ртути
            float gaugeSz = std::max(8.0f, cellSize * 0.20f);
            glm::vec2 gaugePos(drumX + drumW * 0.5f - gaugeSz * 0.5f, drumY + drumH * 0.20f);
            renderer->drawSprite(whiteTexture, gaugePos - glm::vec2(2.0f), glm::vec2(gaugeSz + 4.0f), 0.0f, glm::vec3(0.45f, 0.36f, 0.16f));
            renderer->drawSprite(whiteTexture, gaugePos, glm::vec2(gaugeSz), 0.0f, glm::vec3(0.88f, 0.90f, 0.86f));
            renderer->drawSprite(whiteTexture, gaugePos + glm::vec2(gaugeSz * 0.5f, gaugeSz * 0.2f), glm::vec2(gaugeSz * 0.35f, 2.0f), -35.0f, glm::vec3(0.85f, 0.15f, 0.15f));

            // Три вертикальные трубки воздушного конденсатора паров ртути
            float condStartX = drumX + drumW + cellSize * 0.14f;
            float condColW = std::max(6.0f, cellSize * 0.16f);
            float condGap = cellSize * 0.24f;
            for (int c = 0; c < 3; ++c) {
                float cxPos = condStartX + static_cast<float>(c) * condGap;
                renderer->drawSprite(whiteTexture, glm::vec2(cxPos, retortY + cellSize * 0.12f), glm::vec2(condColW, drumH + cellSize * 0.06f), 0.0f, glm::vec3(0.32f, 0.37f, 0.44f));
                renderer->drawSprite(whiteTexture, glm::vec2(cxPos + 2.0f, retortY + cellSize * 0.12f), glm::vec2(condColW * 0.35f, drumH + cellSize * 0.06f), 0.0f, glm::vec3(0.64f, 0.71f, 0.80f));
            }
            // Верхний перекидной коллектор конденсатора
            renderer->drawSprite(whiteTexture, glm::vec2(drumX + drumW * 0.75f, retortY + cellSize * 0.10f), glm::vec2(cellSize * 0.95f, std::max(6.0f, cellSize * 0.13f)), 0.0f, glm::vec3(0.38f, 0.43f, 0.50f));

            // Приемная ванна-отстойник с блестящей жидкой серебристой ртутью справа
            float vatX = condStartX + 3.0f * condGap + cellSize * 0.04f;
            float vatW = cellSize * 0.92f;
            float vatH = cellSize * 0.58f;
            float vatY = padY - vatH + cellSize * 0.04f;
            renderer->drawSprite(whiteTexture, glm::vec2(vatX, vatY), glm::vec2(vatW, vatH), 0.0f, glm::vec3(0.20f, 0.23f, 0.28f));
            // Зеркало жидкой ртути с ярким металлическим бликом
            renderer->drawSprite(whiteTexture, glm::vec2(vatX + 4.0f, vatY + 5.0f), glm::vec2(vatW - 8.0f, vatH * 0.55f), 0.0f, glm::vec3(0.76f, 0.83f, 0.91f));
            renderer->drawSprite(whiteTexture, glm::vec2(vatX + 8.0f, vatY + 8.0f), glm::vec2((vatW - 16.0f) * 0.65f, std::max(3.0f, vatH * 0.18f)), 0.0f, glm::vec3(0.94f, 0.98f, 1.0f));
            // Стояк от ванны к магистральному ртутному трубопроводу
            float riserX = vatX + vatW * 0.5f - hgPipeH * 0.45f;
            float riserH = std::max(4.0f, hgPipeY - (vatY + vatH));
            renderer->drawSprite(whiteTexture, glm::vec2(riserX, vatY + vatH), glm::vec2(hgPipeH * 0.9f, riserH + hgPipeH * 0.5f), 0.0f, glm::vec3(0.45f, 0.52f, 0.60f));
        }

        // Деревянные стойки шахтной крепи, хомуты труб и взрывозащищенные лампы ("коногонки" / РВП)
        float postW = std::max(10.0f, cellSize * 0.24f);
        for (float px = firstPostX; px <= cullMaxX; px += postSpacing) {
            int pIdx = static_cast<int>(std::round((px - gridOffset.x) / postSpacing));

            // Хомут вентиляционной трубы и фланец ртутной магистрали
            renderer->drawSprite(whiteTexture, glm::vec2(px + postW * 0.2f, ventY - 2.0f), glm::vec2(std::max(4.0f, postW * 0.45f), ventH + 4.0f), 0.0f, glm::vec3(0.26f, 0.28f, 0.32f));
            renderer->drawSprite(whiteTexture, glm::vec2(px + postW * 0.15f, hgPipeY - 2.0f), glm::vec2(std::max(5.0f, postW * 0.55f), hgPipeH + 4.0f), 0.0f, glm::vec3(0.46f, 0.52f, 0.60f));

            // Тень от массивной деревянной стойки крепи
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(px + postW, topWallY + cellSize * 0.08f), glm::vec2(std::max(5.0f, postW * 0.4f), topWallH - cellSize * 0.08f), 0.0f, glm::vec4(0.02f, 0.02f, 0.03f, 0.50f));
            // Ствол деревянной стойки крепи
            renderer->drawSprite(whiteTexture, glm::vec2(px, topWallY), glm::vec2(postW, topWallH), 0.0f, glm::vec3(0.28f, 0.18f, 0.10f));
            renderer->drawSprite(whiteTexture, glm::vec2(px + 2.0f, topWallY), glm::vec2(postW * 0.45f, topWallH), 0.0f, glm::vec3(0.39f, 0.25f, 0.14f));
            // Стальной башмак у основания стойки
            float shoeH = std::max(5.0f, cellSize * 0.10f);
            renderer->drawSprite(whiteTexture, glm::vec2(px - 2.0f, gridOffset.y - shoeH), glm::vec2(postW + 4.0f, shoeH), 0.0f, glm::vec3(0.24f, 0.26f, 0.30f));

            // На каждой четной стойке — шахтный светильник в защитной решетке с теплым янтарным светом
            if ((pIdx % 2) == 0) {
                float lampW = std::max(8.0f, cellSize * 0.16f);
                float lampH = std::max(10.0f, cellSize * 0.22f);
                float lampX = px + (postW - lampW) * 0.5f;
                float lampY = gridOffset.y - cellSize * 0.42f;

                renderer->drawSpriteRGBA(whiteTexture, glm::vec2(lampX - cellSize * 0.45f, lampY - cellSize * 0.25f), glm::vec2(lampW + cellSize * 0.90f, lampH + cellSize * 0.75f), 0.0f, glm::vec4(1.0f, 0.72f, 0.24f, 0.14f));
                renderer->drawSprite(whiteTexture, glm::vec2(lampX - 1.0f, lampY - 3.0f), glm::vec2(lampW + 2.0f, 4.0f), 0.0f, glm::vec3(0.20f, 0.22f, 0.25f));
                renderer->drawSprite(whiteTexture, glm::vec2(lampX, lampY), glm::vec2(lampW, lampH), 0.0f, glm::vec3(0.30f, 0.26f, 0.18f));
                renderer->drawSprite(whiteTexture, glm::vec2(lampX + 2.0f, lampY + 2.0f), glm::vec2(lampW - 4.0f, lampH - 4.0f), 0.0f, glm::vec3(1.0f, 0.82f, 0.36f));
            }

            // На каждой нечетной кратной 3 стойке — красный запорный вентиль ртутной трубы
            if (((pIdx % 2) != 0) && ((pIdx % 3) == 0)) {
                float wheelSz = std::max(8.0f, cellSize * 0.20f);
                renderer->drawSprite(whiteTexture, glm::vec2(px + postW + cellSize * 0.25f, hgPipeY - wheelSz * 0.35f), glm::vec2(wheelSz, wheelSz), 0.0f, glm::vec3(0.78f, 0.16f, 0.18f));
                renderer->drawSprite(whiteTexture, glm::vec2(px + postW + cellSize * 0.25f + wheelSz * 0.3f, hgPipeY - wheelSz * 0.35f + wheelSz * 0.3f), glm::vec2(wheelSz * 0.4f, wheelSz * 0.4f), 0.0f, glm::vec3(0.22f, 0.24f, 0.28f));
            }
        }
    }

    // =========================================================================
    // 2. НИЖНИЙ БОРТ ШТРЕКА: водоотливная канавка с осевшей ртутью, кабель и нижние стойки крепи
    // =========================================================================
    if (isRectVisible(cullMinX, botWallY - cellSize * 0.2f, spanW, botWallH + cellSize * 0.5f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
        // Каменная полка нижнего борта
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, botWallY),
            glm::vec2(spanW, botWallH),
            0.0f,
            glm::vec3(0.112f, 0.118f, 0.145f)
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cullMinX, botWallY),
            glm::vec2(spanW, std::max(5.0f, cellSize * 0.09f)),
            0.0f,
            glm::vec3(0.170f, 0.178f, 0.212f)
        );

        // Шахтная водоотливная канавка вдоль нижней бровки с блестящими линзами осевшей жидкой ртути
        float gutterY = botWallY + cellSize * 0.14f;
        float gutterH = std::max(10.0f, cellSize * 0.28f);
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, gutterY - 2.0f), glm::vec2(spanW, gutterH + 4.0f), 0.0f, glm::vec3(0.19f, 0.20f, 0.24f));
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, gutterY), glm::vec2(spanW, gutterH), 0.0f, glm::vec3(0.055f, 0.062f, 0.082f));

        float poolStep = cellSize * 2.2f;
        float firstPoolX = gridOffset.x - std::floor((gridOffset.x - cullMinX) / poolStep) * poolStep;
        for (float mx = firstPoolX - poolStep; mx <= cullMaxX; mx += poolStep) {
            int mIdx = static_cast<int>(std::round((mx - gridOffset.x) / poolStep));
            uint32_t mHash = static_cast<uint32_t>(mIdx) * 1103515245u + 12345u;
            float poolW = cellSize * (0.65f + static_cast<float>(mHash % 55) * 0.01f);
            float poolH = std::max(4.0f, gutterH * 0.52f);
            float poolX = mx + static_cast<float>((mHash / 64) % 35) * 0.01f * cellSize;
            float poolY = gutterY + (gutterH - poolH) * 0.5f;

            renderer->drawSprite(whiteTexture, glm::vec2(poolX, poolY), glm::vec2(poolW, poolH), 0.0f, glm::vec3(0.66f, 0.74f, 0.84f));
            renderer->drawSprite(whiteTexture, glm::vec2(poolX + poolW * 0.15f, poolY + 1.0f), glm::vec2(poolW * 0.50f, std::max(2.0f, poolH * 0.42f)), 0.0f, glm::vec3(0.90f, 0.95f, 1.0f));
        }

        // Ржавая труба сжатого воздуха и бронированный силовой кабель ниже канавки
        float airPipeY = gutterY + gutterH + cellSize * 0.16f;
        float airPipeH = std::max(7.0f, cellSize * 0.15f);
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, airPipeY), glm::vec2(spanW, airPipeH), 0.0f, glm::vec3(0.34f, 0.22f, 0.16f));
        renderer->drawSprite(whiteTexture, glm::vec2(cullMinX, airPipeY + 1.5f), glm::vec2(spanW, airPipeH * 0.35f), 0.0f, glm::vec3(0.48f, 0.31f, 0.22f));

        // Переходные деревянные мостки через канавку и нижние стойки крепи
        float postW = std::max(10.0f, cellSize * 0.24f);
        for (float px = firstPostX; px <= cullMaxX; px += postSpacing) {
            float bridgeW = postW * 1.8f;
            float bridgeX = px - (bridgeW - postW) * 0.5f;
            renderer->drawSprite(whiteTexture, glm::vec2(bridgeX, gutterY - 3.0f), glm::vec2(bridgeW, gutterH + 6.0f), 0.0f, glm::vec3(0.32f, 0.21f, 0.12f));
            renderer->drawSprite(whiteTexture, glm::vec2(bridgeX + 2.0f, gutterY - 1.0f), glm::vec2(bridgeW - 4.0f, gutterH + 2.0f), 0.0f, glm::vec3(0.43f, 0.28f, 0.16f));

            float lowPostY = gutterY + gutterH + cellSize * 0.05f;
            float lowPostH = botWallH - (lowPostY - botWallY);
            if (lowPostH > 2.0f) {
                renderer->drawSprite(whiteTexture, glm::vec2(px, lowPostY), glm::vec2(postW, lowPostH), 0.0f, glm::vec3(0.26f, 0.16f, 0.09f));
                renderer->drawSprite(whiteTexture, glm::vec2(px + 2.0f, lowPostY), glm::vec2(postW * 0.42f, lowPostH), 0.0f, glm::vec3(0.36f, 0.23f, 0.13f));
            }
        }
    }
}

void BackgroundRenderer::renderCinnabarMineForeground(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect,
    const std::vector<glm::ivec2>* /*baseCells*/
) {
    if (!renderer || !whiteTexture || !grid) return;

    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    float gridH = static_cast<float>(grid->getHeight()) * cellSize;

    float cullMinX = 0.0f;
    float cullMinY = 0.0f;
    float cullMaxX = 0.0f;
    float cullMaxY = 0.0f;
    computeCulledBounds(grid, screenWidth, screenHeight, viewRect, 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY);

    float spanW = cullMaxX - cullMinX;
    if (spanW <= 0.0f) return;

    float postSpacing = 3.0f * cellSize;
    float firstPostX = gridOffset.x - std::floor((gridOffset.x - cullMinX) / postSpacing) * postSpacing;
    float capW = std::max(14.0f, cellSize * 0.32f);
    float capH = std::max(6.0f, cellSize * 0.13f);
    float postW = std::max(10.0f, cellSize * 0.24f);

    // Аккуратные выступающие оголовки верхняков крепи и мягкие конусы теплого света шахтных ламп
    for (float px = firstPostX; px <= cullMaxX; px += postSpacing) {
        int pIdx = static_cast<int>(std::round((px - gridOffset.x) / postSpacing));
        float capX = px - (capW - postW) * 0.5f;

        // Верхний оголовок балки-верхняка на границе свода
        if (isRectVisible(capX, gridOffset.y - capH, capW, capH * 2.0f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
            renderer->drawSprite(whiteTexture, glm::vec2(capX, gridOffset.y - capH * 0.65f), glm::vec2(capW, capH), 0.0f, glm::vec3(0.22f, 0.14f, 0.08f));
            renderer->drawSprite(whiteTexture, glm::vec2(capX + 2.0f, gridOffset.y - capH * 0.65f), glm::vec2(capW - 4.0f, capH * 0.45f), 0.0f, glm::vec3(0.36f, 0.23f, 0.13f));
        }

        // Мягкое теплое пятно света от четных светильников на край полотна штрека
        if ((pIdx % 2) == 0) {
            float glowW = cellSize * 1.9f;
            float glowH = cellSize * 0.85f;
            float glowX = px + postW * 0.5f - glowW * 0.5f;
            float glowY = gridOffset.y - cellSize * 0.12f;
            if (isRectVisible(glowX, glowY, glowW, glowH, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
                renderer->drawSpriteRGBA(
                    whiteTexture,
                    glm::vec2(glowX, glowY),
                    glm::vec2(glowW, glowH),
                    0.0f,
                    glm::vec4(1.0f, 0.76f, 0.28f, 0.055f)
                );
            }
        }

        // Нижний торец балки крепи у нижней границы штрека
        float botCapY = gridOffset.y + gridH - capH * 0.35f;
        if (isRectVisible(capX, botCapY, capW, capH * 1.5f, cullMinX, cullMinY, cullMaxX, cullMaxY)) {
            renderer->drawSprite(whiteTexture, glm::vec2(capX, botCapY), glm::vec2(capW, capH), 0.0f, glm::vec3(0.20f, 0.13f, 0.07f));
            renderer->drawSprite(whiteTexture, glm::vec2(capX + 2.0f, botCapY), glm::vec2(capW - 4.0f, capH * 0.40f), 0.0f, glm::vec3(0.33f, 0.21f, 0.12f));
        }
    }
}


