#include "GameWorld.h"
#include "../entities/Enemy.h"
#include "../ui/BuildPanel.h"
#include "../ui/UICommon.h"
#include "../audio/AudioManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <unordered_map>

GameWorld::GameWorld() {
}

bool GameWorld::loadLevel(const std::string& levelPath, int windowWidth, int windowHeight) {
    LevelMapData loadedData = LevelManager::loadLevelMap(levelPath);
    this->levelData = loadedData;
    currentLevelPath = levelPath;
    currentBackground = loadedData.background.empty() ? "default" : loadedData.background;
    cameraSettings = loadedData.camera;
    playerStats.reset(loadedData.startingMoney, loadedData.startingHealth);

    grid = std::make_unique<Grid>(
        levelData.gridWidth,
        levelData.gridHeight,
        levelData.cellSize,
        glm::vec2(levelData.offsetX, levelData.offsetY)
    );
    // Авто-камера (fitToScreen) должна рассчитываться ТОЛЬКО если isCustom == false
    if (!cameraSettings.isCustom || levelData.cellSize <= 0.001f) {
        float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
        float topMargin = TimeControlUI::getTopMargin(windowWidth, windowHeight);
        grid->updateCellSize(windowWidth, windowHeight, bottomBarHeight, topMargin);
    }
    grid->setHasSidePockets(levelData.hasSidePockets || currentBackground == "mine_entrance");

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
        if (spawner.visibleInGame) {
            grid->setCellType(spawner.pos.x, spawner.pos.y, CellType::Spawner);
        }
    }

    for (const auto& base : bases) {
        if (base.visibleInGame) {
            grid->setCellType(base.x, base.y, CellType::Base);
        }
    }

    for (const auto& oc : levelData.outerCells) {
        CellType ct = CellType::Ground;
        if (oc.type == 1) ct = CellType::Path;
        else if (oc.type == 2) ct = CellType::Platform;
        else if (oc.type == 3) ct = CellType::Scenery;
        else if (oc.type == 4) ct = CellType::Chasm;
        else if (oc.type == 5) ct = CellType::Rail;
        grid->setCellType(oc.x, oc.y, ct);
        if (oc.noBuild) {
            grid->setNoBuildAt(oc.x, oc.y, true);
        }
    }

    grid->saveOriginalGrid();

    // apply no-build mask for canBuildAt checks in gameplay
    grid->setNoBuildMask(levelData.noBuildMask);

    pathfinder = std::make_unique<Pathfinder>(levelData.gridWidth, levelData.gridHeight);
    waveManager = std::make_unique<WaveManager>();
    waveManager->loadWaves(levelData.waves);
    waveManager->setBountyRules(levelData.enemyBountyRules);
    entityManager = std::make_unique<EntityManager>();

    recalculateAllPaths();
    minecartManager.init(levelData, grid->getCellSize(), grid->getOffset());
    emitters = levelData.emitters;
    std::stable_sort(emitters.begin(), emitters.end(), [](const ParticleEmitterConfig& a, const ParticleEmitterConfig& b) {
        return a.zIndex < b.zIndex;
    });
    envParticleManager.setEmitters(emitters);
    decorations = levelData.decorations;
    std::stable_sort(decorations.begin(), decorations.end(), [](const DecorationConfig& a, const DecorationConfig& b) {
        return a.zIndex < b.zIndex;
    });
    if (grid) {
        for (auto& dec : decorations) {
            dec.worldPos = grid->getOffset() + dec.tilePos * grid->getCellSize();
        }
    }
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
    if (!pathfinder || !grid || !entityManager || bases.empty()) {
        return;
    }

    bool filterByCell = (blockedCell.x >= 0 && blockedCell.y >= 0);
    std::unordered_map<int, DijkstraMap> dmapCache;

    for (auto& enemy : entityManager->getEnemies()) {
        if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;

        // Если задана заблокированная клетка, проверяем пересечение с оставшимся путем
        if (filterByCell && !enemy->isPathIntersecting(blockedCell)) {
            continue; // Путь врага не затронут постройкой башни!
        }

        int targetId = enemy->getTargetBaseIndex();
        bool hasExactBase = false;
        if (targetId >= 0) {
            for (const auto& b : bases) {
                if (b.id == targetId) {
                    hasExactBase = true;
                    break;
                }
            }
        }

        int cacheKey = -1;
        if (hasExactBase) {
            cacheKey = targetId;
        }

        auto it = dmapCache.find(cacheKey);
        if (it == dmapCache.end()) {
            std::vector<glm::ivec2> targetPositions = PathService::getTargetBasePositions(bases, targetId);
            auto inserted = dmapCache.emplace(cacheKey, pathfinder->generateDijkstraMap(*grid, targetPositions));
            it = inserted.first;
        }

        enemy->recalculatePath(it->second, *pathfinder, *grid, bases);
    }
}

void GameWorld::update(float dt, float timeScale) {
    if (waveManager) {
        auto requests = waveManager->update(dt, spawners);
        for (const auto& req : requests) {
            spawnEnemy(req.type, req.spawnerIndex, req.reward, req.rewardStep, req.waveIndex);
        }
    }
    entityManager->update(dt, *grid, timeScale);
    minecartManager.update(dt, entityManager->getEnemies());
    if (waveManager && entityManager) {
        waveManager->onEnemiesUpdated(entityManager->getEnemies().empty());
    }
    if (grid) {
        envParticleManager.update(dt, *grid);
    }
}

void GameWorld::render(SpriteRenderer* renderer, const Texture2D* whiteTexture) {
    minecartManager.render(renderer, whiteTexture);
    if (grid) {
        envParticleManager.render(renderer, whiteTexture, *grid, 0, 100);
    }
}

void GameWorld::resize(int windowWidth, int windowHeight) {
    if (grid) {
        Grid oldGrid = *grid;
        if (!cameraSettings.isCustom) {
            float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
            float topMargin = TimeControlUI::getTopMargin(windowWidth, windowHeight);
            grid->updateCellSize(windowWidth, windowHeight, bottomBarHeight, topMargin);
        }
        grid->setHasSidePockets(levelData.hasSidePockets || currentBackground == "mine_entrance");
        minecartManager.updateCellSize(grid->getCellSize(), grid->getOffset());

        for (const auto& enemy : entityManager->getEnemies()) {
            if (enemy) {
                enemy->recalculatePosition(oldGrid, *grid);
            }
        }

        for (auto& proj : entityManager->getProjectilePool()) {
            proj.recalculatePosition(oldGrid, *grid);
        }

        for (auto& dec : decorations) {
            dec.worldPos = grid->getOffset() + dec.tilePos * grid->getCellSize();
        }
    }
}

void GameWorld::spawnEnemy(const std::string& type, int spawnerIndex, int reward, int rewardStep, int waveIndex) {
    if (spawnerIndex >= paths.size() || paths[spawnerIndex].empty()) return;

    // Награда ДОЛЖНА БРАТЬСЯ СТРОГО из enemyBountyRules текущей карты (LevelMapData) через getEnemyBountyForWave!
    int currentWaveIndex = 0;
    if (waveIndex >= 0) {
        currentWaveIndex = waveIndex;
    } else if (waveManager) {
        currentWaveIndex = waveManager->getCurrentWaveIndex();
    }
    auto bounty = getEnemyBountyForWave(levelData.enemyBountyRules, type, currentWaveIndex);
    int finalReward = bounty.first;
    int finalRewardStep = bounty.second;

    int targetIdx = spawners[spawnerIndex].targetBaseIndex;
    bool fadeIn = spawners[spawnerIndex].enableFadeIn;
    bool fadeOut = true;
    if (!paths[spawnerIndex].empty()) {
        glm::ivec2 basePos = paths[spawnerIndex].back();
        for (const auto& b : bases) {
            if (b.x == basePos.x && b.y == basePos.y) {
                fadeOut = b.enableFadeOut;
                break;
            }
        }
    }

    auto newEnemy = std::make_unique<Enemy>(paths[spawnerIndex], *grid, type, targetIdx, fadeIn, fadeOut);
    newEnemy->setWaveIndex(currentWaveIndex);
    newEnemy->setReward(finalReward);
    newEnemy->setRewardStep(finalRewardStep);

    int customHp = getEnemyHpForWave(levelData.enemyHpRules, type, currentWaveIndex);
    if (customHp > 0) {
        newEnemy->setMaxHealth(customHp);
        newEnemy->setHealth(customHp);
    }

    entityManager->addEnemy(std::move(newEnemy));
}

bool GameWorld::canCallEarlyWave() const {
    return waveManager ? waveManager->canCallEarlyWave() : false;
}

int GameWorld::getEarlyCallBonus() const {
    return waveManager ? waveManager->getEarlyCallBonus() : 0;
}

void GameWorld::triggerEarlyWave() {
    if (waveManager && waveManager->canCallEarlyWave()) {
        waveManager->triggerEarlyWave(&playerStats);
    }
}

glm::vec2 GameWorld::getWorldCamCenter() const {
    if (!grid) return glm::vec2(0.0f);
    glm::vec2 targetTile = cameraSettings.targetTile;
    if (targetTile.x < -100.0f || targetTile.y < -100.0f) {
        targetTile = glm::vec2(static_cast<float>(grid->getWidth()) * 0.5f,
                               static_cast<float>(grid->getHeight()) * 0.5f);
    }
    return grid->getOffset() + targetTile * grid->getCellSize();
}

float GameWorld::getEffectiveZoom(int windowWidth, int windowHeight) const {
    if (!cameraSettings.isCustom || !grid) return 1.0f;
    float cs = grid->getCellSize();
    if (cs <= 0.001f) return 1.0f;

    float viewW = (cameraSettings.viewTilesW > 0.1f) ? (cameraSettings.viewTilesW * cs) : 0.0f;
    float viewH = (cameraSettings.viewTilesH > 0.1f) ? (cameraSettings.viewTilesH * cs) : 0.0f;

    if (viewW > 1.0f && viewH > 1.0f) {
        float scaleX = static_cast<float>(windowWidth) / viewW;
        float scaleY = static_cast<float>(windowHeight) / viewH;
        return std::min(scaleX, scaleY);
    }
    return (cameraSettings.zoom > 0.001f) ? cameraSettings.zoom : 1.0f;
}

glm::vec2 GameWorld::getCombatCameraPan(int windowWidth, int windowHeight) const {
    if (!cameraSettings.isCustom) return glm::vec2(0.0f);
    glm::vec2 screenCenter(static_cast<float>(windowWidth) * 0.5f, static_cast<float>(windowHeight) * 0.5f);
    glm::vec2 worldCamCenter = getWorldCamCenter();
    float z = getEffectiveZoom(windowWidth, windowHeight);
    return screenCenter - worldCamCenter * z;
}

glm::mat4 GameWorld::getViewMatrix(int windowWidth, int windowHeight) const {
    if (!cameraSettings.isCustom) return glm::mat4(1.0f);
    glm::vec2 screenCenter(static_cast<float>(windowWidth) * 0.5f, static_cast<float>(windowHeight) * 0.5f);
    glm::vec2 worldCamCenter = getWorldCamCenter();
    float z = getEffectiveZoom(windowWidth, windowHeight);

    // view = translate(screenCenter) * scale(zoom) * translate(-worldCamCenter)
    return glm::translate(glm::mat4(1.0f), glm::vec3(screenCenter, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(z, z, 1.0f))
         * glm::translate(glm::mat4(1.0f), glm::vec3(-worldCamCenter, 0.0f));
}

glm::vec2 GameWorld::screenToWorld(glm::vec2 screenPos, int windowWidth, int windowHeight) const {
    if (!cameraSettings.isCustom) return screenPos;
    glm::vec2 screenCenter(static_cast<float>(windowWidth) * 0.5f, static_cast<float>(windowHeight) * 0.5f);
    glm::vec2 worldCamCenter = getWorldCamCenter();
    float z = getEffectiveZoom(windowWidth, windowHeight);
    return (screenPos - screenCenter) / z + worldCamCenter;
}

void GameWorld::demolishDecorationsInCell(int gridX, int gridY) {
    if (!grid || decorations.empty()) return;

    glm::vec2 cellMin = grid->gridToPixel(gridX, gridY);
    float cellSize = grid->getCellSize();
    glm::vec2 cellMax = cellMin + glm::vec2(cellSize);

    auto it = decorations.begin();
    while (it != decorations.end()) {
        const auto& dec = *it;
        bool inCell = (dec.worldPos.x >= cellMin.x && dec.worldPos.x <= cellMax.x &&
                       dec.worldPos.y >= cellMin.y && dec.worldPos.y <= cellMax.y);
        if (inCell && dec.destructible) {
            float s = dec.scale;
            if (dec.type == "bush" || dec.type == "grass_tuft" || dec.type == "flower") {
                envParticleManager.spawnDemolitionLeaves(dec.worldPos, s);
            } else if (dec.type == "stone" || dec.type == "helmet" || dec.type == "pickaxe" || dec.type == "wooden_crate" || dec.type == "broken_mercury_cart" || dec.type == "scrap_debris") {
                envParticleManager.spawnDemolitionSparksGravel(dec.worldPos, s);
                AudioManager::playSound("res/sounds/build.wav");
            } else if (dec.type == "puddle") {
                envParticleManager.spawnDemolitionSplash(dec.worldPos, s);
            } else { // "crack" and others
                envParticleManager.spawnDemolitionSparksGravel(dec.worldPos, s);
            }
            it = decorations.erase(it);
        } else {
            ++it;
        }
    }
}
