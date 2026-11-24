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
#include "../../particles/EnvironmentParticleManager.h"

class SpriteRenderer;
class Texture2D;

struct GameWorld {
    std::unique_ptr<Grid> grid;
    std::unique_ptr<Pathfinder> pathfinder;
    std::unique_ptr<WaveManager> waveManager;
    std::unique_ptr<EntityManager> entityManager;
    MinecartManager minecartManager;
    EnvironmentParticleManager envParticleManager;
    std::vector<ParticleEmitterConfig> emitters;
    std::vector<DecorationConfig> decorations;

    PlayerStats playerStats;

    std::vector<SpawnerData> spawners;
    std::vector<BaseData> bases;
    std::vector<std::vector<glm::ivec2>> paths;
    std::vector<glm::ivec2> levelPath;

    std::string currentLevelPath;
    std::string currentBackground = "default";
    LevelMapData::CameraSettings cameraSettings;
    LevelMapData levelData;

    glm::vec2 getWorldCamCenter() const;
    float getEffectiveZoom(int windowWidth, int windowHeight) const;
    glm::vec2 getCombatCameraPan(int windowWidth, int windowHeight) const;
    glm::mat4 getViewMatrix(int windowWidth, int windowHeight) const;
    glm::vec2 screenToWorld(glm::vec2 screenPos, int windowWidth, int windowHeight) const;

    GameWorld();
    ~GameWorld() = default;

    bool loadLevel(const std::string& levelPath, int windowWidth, int windowHeight);
    void recalculateAllPaths();
    void notifyEnemiesPathChanged(glm::ivec2 blockedCell = glm::ivec2(-1, -1));
    void update(float dt, float timeScale = 1.0f);
    void render(SpriteRenderer* renderer, const Texture2D* whiteTexture);
    void resize(int windowWidth, int windowHeight);
    void spawnEnemy(const std::string& type, int spawnerIndex = 0, int reward = 1, int rewardStep = 1, int waveIndex = -1);
    void demolishDecorationsInCell(int gridX, int gridY);

    // Ранний вызов волны (Фаза 4.2) и автопродолжение волн
    bool canCallEarlyWave() const;
    int getEarlyCallBonus() const;
    void triggerEarlyWave();
    void toggleAutoStartWaves();
    bool isAutoStartWaves() const;
};
