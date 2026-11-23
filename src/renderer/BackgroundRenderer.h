#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class SpriteRenderer;
class Texture2D;
class Grid;

struct BackgroundViewRect {
    glm::vec2 min = glm::vec2(0.0f);
    glm::vec2 max = glm::vec2(0.0f);
    bool valid = false;
};

class BackgroundRenderer {
public:
    // СЛОЙ 1: Базовая подложка под фон (заливка земли/травы и горизонтальные пласты породы/склона)
    static void renderSubstrate(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect = BackgroundViewRect()
    );

    // СЛОЙ 2: Фоновое окружение (деревянный настил, здание копра, рельсы, трубы)
    static void renderEnvironment(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect = BackgroundViewRect(),
        const std::vector<glm::ivec2>* baseCells = nullptr
    );

    // СЛОЙ 3: Верхнее окружение переднего плана (2.5D забор поверх клеток земли, козырек копра)
    static void renderForeground(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect = BackgroundViewRect(),
        const std::vector<glm::ivec2>* baseCells = nullptr
    );

    // Полная отрисовка обоих слоев (обратная совместимость)
    static void render(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect = BackgroundViewRect(),
        const std::vector<glm::ivec2>* baseCells = nullptr
    );

private:
    struct MinePortalGeom {
        float mouthX;
        float coverStartX;
        float endX;
        float centerY;
        float openingH;
        float topY;
        float botY;
    };

    static void computeCulledBounds(
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect,
        float paddingCells,
        float& outMinX,
        float& outMinY,
        float& outMaxX,
        float& outMaxY
    );

    static bool isRectVisible(
        float x,
        float y,
        float w,
        float h,
        float cullMinX,
        float cullMinY,
        float cullMaxX,
        float cullMaxY
    );

    static std::vector<MinePortalGeom> collectMinePortals(
        const Grid* grid,
        const std::vector<glm::ivec2>* baseCells
    );

    static void renderPipesCanalSubstrate(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect
    );

    static void renderPipesCanalEnvironment(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect
    );

    static void renderMineEntranceSubstrate(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect
    );

    static void renderMineEntranceEnvironment(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect,
        const std::vector<glm::ivec2>* baseCells
    );

    static void renderMineEntranceForeground(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect,
        const std::vector<glm::ivec2>* baseCells
    );

    static void renderPipe(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        float startX,
        float endX,
        float pipeY,
        float pipeThickness,
        float cellSize,
        float gridOffsetX
    );
};

