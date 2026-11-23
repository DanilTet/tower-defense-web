#include "EditorTileInfoOverlay.h"
#include "EditorCamera.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../textures/Texture2D.h"
#include "../world/Grid.h"
#include "../ui/UICommon.h"
#include <algorithm>
#include <cmath>

void EditorTileInfoOverlay::render(
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    const Texture2D* whiteTexture,
    const EditorCamera& camera,
    const Grid* grid,
    const std::vector<SpawnerData>& spawners,
    const std::vector<BaseData>& bases,
    const std::vector<MinecartData>& minecarts,
    const Params& params
) {
    if (!renderer || !textRenderer || !whiteTexture || !grid) return;
    if (params.isSuppressed) return;

    glm::vec2 worldM = camera.screenToWorld(params.mousePos);
    glm::ivec2 curTile = grid->pixelToGrid(worldM);

    bool inBounds = (curTile.x >= 0 && curTile.x < params.gridWidth && curTile.y >= 0 && curTile.y < params.gridHeight);
    bool allowOuter = (params.hasSidePockets || grid->hasSidePockets());
    bool inSidePockets = allowOuter && (curTile.x >= -30 && curTile.x < params.gridWidth + 30 && curTile.y >= -20 && curTile.y < params.gridHeight + 20);
    if (!inBounds && !inSidePockets) return;

    // 1. Координаты клетки: [X: ..., Y: ...]
    std::string coordStr = "[X: " + std::to_string(curTile.x) + ", Y: " + std::to_string(curTile.y) + "]";

    // 2. Текущий тип тайла: Тип: Грунт / Ящики / Дорога / Пусто и т.д.
    CellType ct = grid->getCellType(curTile.x, curTile.y);
    std::string typeName;
    switch (ct) {
        case CellType::Ground:
            typeName = (params.background == "pipes_canal" || params.background == "channel") ? "Асфальт" : "Грунт";
            break;
        case CellType::Path:
            typeName = "Дорога";
            break;
        case CellType::Platform:
            typeName = (params.background == "pipes_canal" || params.background == "channel") ? "Трава/Обочина" : ((params.background == "mine_entrance" || params.background == "mine") ? "Сланец" : "Платформа");
            break;
        case CellType::Scenery:
            typeName = (params.background == "mine_entrance" || params.background == "mine") ? "Ящики" : "Стена";
            break;
        case CellType::Chasm:
            typeName = "Шурф";
            break;
        case CellType::Rail:
            typeName = "Рельсы";
            break;
        case CellType::Spawner:
            typeName = "Спавнер";
            break;
        case CellType::Base:
            typeName = "База";
            break;
        case CellType::Empty:
            typeName = "Пусто";
            break;
        default:
            typeName = "Грунт";
            break;
    }
    std::string typeStr = "Тип: " + typeName;

    // 3. Статус запрета стройки: Запрет: Да / Нет
    bool isBlocked = grid->isNoBuild(curTile.x, curTile.y);
    std::string forbidStr = std::string("Запрет: ") + (isBlocked ? "Да" : "Нет");

    // 4. Наличие объекта: Объект: Спавнер #0 / База #0 / Нет
    std::string objName = "Нет";
    for (const auto& sp : spawners) {
        if (sp.pos.x == curTile.x && sp.pos.y == curTile.y) {
            objName = (sp.targetBaseIndex == -1) ? "Спавнер :Auto" : ("Спавнер #" + std::to_string(sp.targetBaseIndex));
            break;
        }
    }
    if (objName == "Нет") {
        for (const auto& b : bases) {
            if (b.x == curTile.x && b.y == curTile.y) {
                objName = "База #" + std::to_string(b.id);
                break;
            }
        }
    }
    if (objName == "Нет" && !minecarts.empty()) {
        const auto& mc = minecarts[0];
        if (mc.hasStart() && mc.start.x == curTile.x && mc.start.y == curTile.y) {
            objName = "Вагонетка (Депо)";
        } else if (mc.hasEnd() && mc.end.x == curTile.x && mc.end.y == curTile.y) {
            objName = "Вагонетка (Тупик)";
        }
    }
    std::string objStr = "Объект: " + objName;

    // Компактная плашка в углу экрана (слева снизу над нижним доком)
    float scale = GetUIScale(params.screenWidth, params.screenHeight);
    float boxW = 215.0f * scale;
    float boxH = 92.0f * scale;
    float pad = 12.0f * scale;
    float boxX = 14.0f * scale;
    float boxY = static_cast<float>(params.screenHeight) - params.bottomDockHeight - boxH - 12.0f * scale;

    renderer->beginBatch();

    // Тень под плашкой
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(boxX + 2.0f, boxY + 3.0f), glm::vec2(boxW, boxH), 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
    // Темное стекло подложки
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(boxX, boxY), glm::vec2(boxW, boxH), 0.0f, glm::vec4(0.08f, 0.10f, 0.14f, 0.90f));

    // Тонкая окантовка плашки
    float borderThick = 1.0f;
    glm::vec4 borderCol(0.24f, 0.30f, 0.40f, 0.70f);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(boxX, boxY), glm::vec2(boxW, borderThick), 0.0f, borderCol);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(boxX, boxY + boxH - borderThick), glm::vec2(boxW, borderThick), 0.0f, borderCol);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(boxX, boxY), glm::vec2(borderThick, boxH), 0.0f, borderCol);
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(boxX + boxW - borderThick, boxY), glm::vec2(borderThick, boxH), 0.0f, borderCol);

    // Цветная акцентная полоска слева
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(boxX + 1.0f, boxY + 1.0f), glm::vec2(3.5f * scale, boxH - 2.0f), 0.0f, glm::vec4(0.25f, 0.75f, 1.0f, 0.90f));

    renderer->endBatch();

    // Текстовые строки инфо-плашки
    float fontScale = std::clamp(0.40f * scale, 0.28f, 0.52f);
    float lineHeight = 19.5f * scale;
    float tx = boxX + pad + 3.0f * scale;
    float ty = boxY + 8.0f * scale;

    // 1. Координаты
    textRenderer->RenderText(coordStr, tx, ty, fontScale * 1.06f, glm::vec3(0.35f, 0.85f, 1.0f));
    // 2. Тип
    textRenderer->RenderText(typeStr, tx, ty + lineHeight, fontScale, glm::vec3(0.92f, 0.92f, 0.95f));
    // 3. Запрет стройки
    glm::vec3 forbidColor = isBlocked ? glm::vec3(1.0f, 0.35f, 0.35f) : glm::vec3(0.45f, 0.85f, 0.45f);
    textRenderer->RenderText(forbidStr, tx, ty + lineHeight * 2.0f, fontScale, forbidColor);
    // 4. Объект
    glm::vec3 objColor = (objName != "Нет") ? glm::vec3(1.0f, 0.85f, 0.30f) : glm::vec3(0.65f, 0.68f, 0.72f);
    textRenderer->RenderText(objStr, tx, ty + lineHeight * 3.0f, fontScale, objColor);
}

