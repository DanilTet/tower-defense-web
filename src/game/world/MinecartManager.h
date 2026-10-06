#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"

class SpriteRenderer;
class Texture2D;
class Enemy;

enum class CartState {
    Inactive,
    Cooldown,
    Warning,
    Moving
};

enum class CartParticleType {
    Spark,
    Smoke
};

struct CartParticle {
    glm::vec2 pos;
    glm::vec2 vel;
    float life;
    float maxLife;
    float size;
    float endSize;
    glm::vec4 color;
    glm::vec4 endColor;
    CartParticleType type;
};

struct FloatingTextEffect {
    std::string text;
    glm::vec2 pos;
    glm::vec2 vel;
    float life;
    float maxLife;
};

class MinecartManager {
private:
    CartState m_state = CartState::Inactive;

    std::vector<glm::ivec2> m_railGridCells;
    std::vector<glm::vec2> m_waypoints;
    size_t m_currentWaypointIndex = 0;

    glm::vec2 m_currentPos = glm::vec2(0.0f);
    float m_currentAngle = 0.0f; // radians

    float m_interval = 25.0f;
    float m_intervalTimer = 0.0f;

    float m_warningTime = 3.0f;
    float m_warningTimer = 0.0f;

    float m_speed = 8.0f; // cells per second
    float m_cellSize = 64.0f;
    glm::vec2 m_gridOffset = glm::vec2(0.0f);

    float m_pulseTimer = 0.0f;

    // Системы частиц и всплывающего текста
    std::vector<CartParticle> m_particles;
    std::vector<FloatingTextEffect> m_floatingTexts;
    float m_particleSpawnTimer = 0.0f;
    float m_popupCooldown = 0.0f;
    int m_killCounter = 0;

    void rebuildWaypoints();
    void spawnKillText(glm::vec2 pos);
    void spawnMovementParticles(float dt);
    void updateParticles(float dt);

    template <typename TEnemyPtr>
    void checkCollisions(std::vector<TEnemyPtr>& enemies);

    template <typename TEnemyPtr>
    void updateInternal(float dt, std::vector<TEnemyPtr>& enemies);

    void renderSemaphore(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture);
    void renderMovingCart(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture);
    void renderParticles(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture);
    void renderFloatingTexts(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture);

public:
    MinecartManager() = default;
    ~MinecartManager() = default;

    void init(const LevelMapData& levelData, float cellSize, glm::vec2 gridOffset);
    void updateCellSize(float cellSize, glm::vec2 gridOffset);

    void update(float dt, std::vector<std::unique_ptr<Enemy>>& enemies);
    void update(float dt, std::vector<std::shared_ptr<Enemy>>& enemies);

    void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture);

    bool isWarning() const { return m_state == CartState::Warning; }
    CartState getState() const { return m_state; }
    glm::vec2 getCurrentPos() const { return m_currentPos; }
    float getCurrentAngle() const { return m_currentAngle; }
};
