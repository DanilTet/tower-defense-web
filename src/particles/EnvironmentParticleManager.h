#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../game/core/LevelManager.h"

class SpriteRenderer;
class Texture2D;
class Grid;

struct EnvParticle {
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 vel = glm::vec2(0.0f);
    glm::vec4 startColor = glm::vec4(1.0f);
    glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    float startSize = 8.0f;
    float endSize = 24.0f;
    float lifeRemaining = 0.0f;
    float lifeTime = 1.0f;
    float gravity = 0.0f;
    float drag = 0.0f;
    float rotation = 0.0f;
    float angularVel = 0.0f;
    bool active = false;
    int zIndex = 0;
};

struct RuntimeEmitterState {
    ParticleEmitterConfig config;
    float timer = 0.0f;          // Таймер до следующего выброса (cooldown)
    float burstRemaining = 0.0f;// Сколько секунд ещё длится текущий залп
    float spawnAccumulator = 0.0f;
    bool isBursting = false;
};

class EnvironmentParticleManager {
public:
    EnvironmentParticleManager(size_t maxParticles = 1500);

    void setEmitters(const std::vector<ParticleEmitterConfig>& configs);
    std::vector<ParticleEmitterConfig> getEmitterConfigs() const;

    void addOrUpdateEmitter(const ParticleEmitterConfig& cfg);
    bool removeEmitterAt(int gridX, int gridY);
    bool rotateEmitterAt(int gridX, int gridY, float deltaDeg = 45.0f);
    const ParticleEmitterConfig* findEmitterAt(int gridX, int gridY) const;
    ParticleEmitterConfig* findEmitterAt(int gridX, int gridY);

    void update(float dt, const Grid& grid);
    void render(SpriteRenderer* renderer, const Texture2D* texture, const Grid& grid, int minZ = -100, int maxZ = 100);

    void clear();
    void clearParticles();
    void triggerBurstAt(size_t index);

    // Эффекты сноса декораций при строительстве башен
    void spawnDemolitionLeaves(glm::vec2 worldCenter, float scaleFactor = 1.0f);
    void spawnDemolitionSparksGravel(glm::vec2 worldCenter, float scaleFactor = 1.0f);
    void spawnDemolitionSplash(glm::vec2 worldCenter, float scaleFactor = 1.0f);

private:
    void emitParticle(const EnvParticle& p);
    void triggerEmitterBurst(RuntimeEmitterState& emitter);

    std::vector<RuntimeEmitterState> m_emitters;
    std::vector<EnvParticle> m_pool;
    size_t m_poolIndex = 0;
};

