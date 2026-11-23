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
    struct CartInstance {
        CartState state = CartState::Inactive;
        std::vector<glm::ivec2> railGridCells;
        std::vector<glm::vec2> waypoints;
        size_t currentWaypointIndex = 0;

        glm::vec2 currentPos = glm::vec2(0.0f);
        float currentAngle = 0.0f; // radians

        float interval = 25.0f;
        float intervalTimer = 0.0f;

        float warningTime = 3.0f;
        float warningTimer = 0.0f;

        float speed = 8.0f; // cells per second
        float pulseTimer = 0.0f;
        float particleSpawnTimer = 0.0f;
    };

    std::vector<CartInstance> m_carts;
    float m_cellSize = 64.0f;
    glm::vec2 m_gridOffset = glm::vec2(0.0f);

    // Системы частиц и всплывающего текста
    std::vector<CartParticle> m_particles;
    std::vector<FloatingTextEffect> m_floatingTexts;
    float m_popupCooldown = 0.0f;
    int m_killCounter = 0;

    void rebuildWaypoints(CartInstance& cart);
    void spawnKillText(glm::vec2 pos);
    void spawnMovementParticles(CartInstance& cart, float dt);
    void updateParticles(float dt);

    template <typename TEnemyPtr>
    void checkCollisions(const CartInstance& cart, std::vector<TEnemyPtr>& enemies);

    template <typename TEnemyPtr>
    void updateInternal(float dt, std::vector<TEnemyPtr>& enemies);

    void renderSemaphore(const CartInstance& cart, SpriteRenderer* renderer, const Texture2D* whiteTexture);
    void renderMovingCart(const CartInstance& cart, SpriteRenderer* renderer, const Texture2D* whiteTexture);
    void renderParticles(SpriteRenderer* renderer, const Texture2D* whiteTexture);
    void renderFloatingTexts(SpriteRenderer* renderer, const Texture2D* whiteTexture);

public:
    MinecartManager() = default;
    ~MinecartManager() = default;

    void init(const LevelMapData& levelData, float cellSize, glm::vec2 gridOffset);
    void updateCellSize(float cellSize, glm::vec2 gridOffset);

    void update(float dt, std::vector<std::unique_ptr<Enemy>>& enemies);
    void update(float dt, std::vector<std::shared_ptr<Enemy>>& enemies);

    void render(SpriteRenderer* renderer, const Texture2D* whiteTexture);

    size_t getCartCount() const { return m_carts.size(); }

    bool isWarning() const {
        for (const auto& cart : m_carts) {
            if (cart.state == CartState::Warning) {
                return true;
            }
        }
        return false;
    }

    CartState getState() const {
        if (m_carts.empty()) {
            return CartState::Inactive;
        }
        return m_carts[0].state;
    }

    glm::vec2 getCurrentPos() const {
        if (m_carts.empty()) {
            return glm::vec2(0.0f);
        }
        return m_carts[0].currentPos;
    }

    float getCurrentAngle() const {
        if (m_carts.empty()) {
            return 0.0f;
        }
        return m_carts[0].currentAngle;
    }
};
