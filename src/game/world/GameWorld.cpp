#include "GameWorld.h"
#include "../entities/Enemy.h"
#include "../ui/BuildPanel.h"
#include "../ui/UICommon.h"
#include <iostream>

GameWorld::GameWorld() {
}

bool GameWorld::loadLevel(const std::string& levelPath, int windowWidth, int windowHeight) {
    LevelMapData levelData = LevelManager::loadLevelMap(levelPath);
    currentLevelPath = levelPath;

    grid = std::make_unique<Grid>(
        levelData.gridWidth,
        levelData.gridHeight,
        levelData.cellSize,
        glm::vec2(levelData.offsetX, levelData.offsetY)
    );
    float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
    float topMargin = TimeControlUI::getTopMargin(windowWidth, windowHeight);
    grid->updateCellSize(windowWidth, windowHeight, bottomBarHeight, topMargin);

    if (!levelData.layout.empty()) {
        for (int y = 0; y < levelData.gridHeight; ++y) {
            for (int x = 0; x < levelData.gridWidth; ++x) {
                if (levelData.layout[y][x] == 1) {
                    grid->setCellType(x, y, CellType::Path);
                }
                else if (levelData.layout[y][x] == 2) {
                    grid->setCellType(x, y, CellType::Platform);
                }
                else if (levelData.layout[y][x] == 3) {
                    grid->setCellType(x, y, CellType::Scenery);
                }
                else if (levelData.layout[y][x] == 4) {
                    grid->setCellType(x, y, CellType::Chasm);
                }
                else if (levelData.layout[y][x] == 5) {
                    grid->setCellType(x, y, CellType::Rail);
                }
            }
        }
    }

    spawners = levelData.spawners;
    bases = levelData.bases;

    for (const auto& spawner : spawners) {
        grid->setCellType(spawner.pos.x, spawner.pos.y, CellType::Spawner);
    }

    for (const auto& base : bases) {
        grid->setCellType(base.x, base.y, CellType::Base);
    }

    grid->saveOriginalGrid();

    pathfinder = std::make_unique<Pathfinder>(levelData.gridWidth, levelData.gridHeight);
    waveManager = std::make_unique<WaveManager>();
    waveManager->loadLevel(currentLevelPath);
    entityManager = std::make_unique<EntityManager>();

    recalculateAllPaths();
    minecartManager.init(levelData, grid->getCellSize(), grid->getOffset());
    return true;
}

#include "PathService.h"

void GameWorld::recalculateAllPaths() {
    paths = PathService::calculateAllPaths(*grid, *pathfinder, spawners, bases);
    if (!paths.empty()) {
        levelPath = paths[0];
    }
}

void GameWorld::notifyEnemiesPathChanged(glm::ivec2 blockedCell) {
    bool filterByCell = (blockedCell.x >= 0 && blockedCell.y >= 0);
    for (auto& enemy : entityManager->getEnemies()) {
        if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;

        // Если задана заблокированная клетка, проверяем пересечение с оставшимся путем
        if (filterByCell && !enemy->isPathIntersecting(blockedCell)) {
            continue; // Путь врага не затронут постройкой башни!
        }

        enemy->recalculatePath(pathfinder.get(), *grid, bases);
    }
}

void GameWorld::update(float dt) {
    if (waveManager) {
        auto requests = waveManager->update(dt, paths.size());
        for (const auto& req : requests) {
            spawnEnemy(req.type, req.spawnerIndex);
        }
    }
    entityManager->update(dt, *grid);
    minecartManager.update(dt, entityManager->getEnemies());
}

void GameWorld::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture) {
    minecartManager.render(renderer, whiteTexture);
}

void GameWorld::resize(int windowWidth, int windowHeight) {
    if (grid) {
        Grid oldGrid = *grid;
        float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
        float topMargin = TimeControlUI::getTopMargin(windowWidth, windowHeight);
        grid->updateCellSize(windowWidth, windowHeight, bottomBarHeight, topMargin);
        minecartManager.updateCellSize(grid->getCellSize(), grid->getOffset());

        for (const auto& enemy : entityManager->getEnemies()) {
            if (enemy) {
                enemy->recalculatePosition(oldGrid, *grid);
            }
        }

        for (auto& proj : entityManager->getProjectilePool()) {
            proj.recalculatePosition(oldGrid, *grid);
        }
    }
}

void GameWorld::spawnEnemy(const std::string& type, int spawnerIndex) {
    if (spawnerIndex >= paths.size() || paths[spawnerIndex].empty()) return;

    int targetIdx = spawners[spawnerIndex].targetBaseIndex;
    auto newEnemy = std::make_unique<Enemy>(paths[spawnerIndex], *grid, type, targetIdx);
    entityManager->addEnemy(std::move(newEnemy));
}
