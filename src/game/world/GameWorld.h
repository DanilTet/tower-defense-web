#pragma once
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"
#include "../core/WaveManager.h"
#include "../gameplay/PlayerStats.h"
#include "../gameplay/EntityManager.h"
#include "Grid.h"
#include "Pathfinder.h"
#include "MinecartManager.h"

class SpriteRenderer;
class Texture2D;

struct GameWorld {
    std::unique_ptr<Grid> grid;
    std::unique_ptr<Pathfinder> pathfinder;
    std::unique_ptr<WaveManager> waveManager;
    std::unique_ptr<EntityManager> entityManager;
    MinecartManager minecartManager;

    PlayerStats playerStats;

    std::vector<SpawnerData> spawners;
    std::vector<BaseData> bases;
    std::vector<std::vector<glm::ivec2>> paths;
    std::vector<glm::ivec2> levelPath;

    std::string currentLevelPath;

    GameWorld();
    ~GameWorld() = default;

    bool loadLevel(const std::string& levelPath, int windowWidth, int windowHeight);
    void recalculateAllPaths();
    void notifyEnemiesPathChanged(glm::ivec2 blockedCell = glm::ivec2(-1, -1));
    void update(float dt);
    void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture);
    void resize(int windowWidth, int windowHeight);
    void spawnEnemy(const std::string& type, int spawnerIndex = 0);
};
