#include "Projectile.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "world/Grid.h"
#include "../../particles/ParticleSystem.h"
#include <cmath>

// конструктор задает спящего бизнесмена
Projectile::Projectile()
    : m_pos(0.0f), m_angle(0.0f), m_speed(0.0f), m_damage(0), m_radius(5.0f),
    m_destroyed(true), m_lifeTime(0.0f), m_targetId(-1), m_splashRadius(0.0f),
    m_turnSpeed(540.0f), m_originPoint(0.0f), m_searchRadius(0.0f), m_isActive(false) {
}

//  вызывается башней и тут не создается новая память
void Projectile::init(glm::vec2 startPos, float startAngle, float speed, int damage, int targetId, float splashRadius, float searchRadius) {
    m_pos = startPos;
    m_angle = startAngle;
    m_speed = speed;
    m_damage = damage;
    m_radius = 5.0f;
    m_destroyed = false;
    m_lifeTime = 3.0f;
    m_targetId = targetId;
    m_splashRadius = splashRadius;
    m_trailTimer = 0.0f;
    m_turnSpeed = 540.0f;
    m_originPoint = startPos;
    m_searchRadius = searchRadius;
    m_isActive = true; // и вот тут спящий бизнесмен просыпается

    float rad = glm::radians(m_angle);
    m_direction = glm::vec2(cos(rad), sin(rad));
}

bool Projectile::tryHitEnemy(Enemy* enemy, const glm::vec2& prevPos, const glm::vec2& travelVec, float travelDistSq, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid, ParticleSystem& particleSystem) {
    if (!enemy || enemy->isDead() || enemy->isReachedEnd() || enemy->isFalling()) {
        return false;
    }

    CircleCollider enemyCol = enemy->getCollider(grid);
    float combinedRadius = m_radius + enemyCol.radius;

    // Swept CCD: расстояние от центра врага до отрезка пути [prevPos, m_pos]
    float t = 0.0f;
    if (travelDistSq > 0.0001f) {
        t = glm::clamp(glm::dot(enemyCol.center - prevPos, travelVec) / travelDistSq, 0.0f, 1.0f);
    }
    glm::vec2 closestPoint = prevPos + travelVec * t;
    glm::vec2 diff = enemyCol.center - closestPoint;
    float distSq = glm::dot(diff, diff);

    if (distSq > combinedRadius * combinedRadius) {
        return false;
    }

    m_pos = closestPoint;

    if (m_hasImpactParticle) {
        ParticleEmitterProps impact = m_impactProps;
        impact.position = m_pos;
        particleSystem.emit(impact, impact.spawnCount, false);
    }

    if (m_splashRadius > 0.0f) {
        float splashPixels = m_splashRadius * grid.getCellSize();
        CircleCollider explosion = { m_pos, splashPixels };

        for (const auto& otherEnemy : enemies) {
            if (!otherEnemy || otherEnemy->isDead() || otherEnemy->isReachedEnd() || otherEnemy->isFalling()) {
                continue;
            }
            if (explosion.intersects(otherEnemy->getCollider(grid))) {
                otherEnemy->takeDamage(m_damage);
                if (m_slowDuration > 0.0f && m_slowPercent > 0.0f) {
                    otherEnemy->applySlow(m_slowDuration, m_slowPercent);
                }
                if (m_poisonDuration > 0.0f && m_poisonDamagePerTick > 0) {
                    otherEnemy->applyPoison(m_poisonDuration, m_poisonInterval, m_poisonDamagePerTick);
                }
            }
        }
    }
    else {
        enemy->takeDamage(m_damage);
        if (m_slowDuration > 0.0f && m_slowPercent > 0.0f) {
            enemy->applySlow(m_slowDuration, m_slowPercent);
        }
        if (m_poisonDuration > 0.0f && m_poisonDamagePerTick > 0) {
            enemy->applyPoison(m_poisonDuration, m_poisonInterval, m_poisonDamagePerTick);
        }
    }

    m_destroyed = true;
    return true;
}

void Projectile::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid, ParticleSystem& particleSystem, Enemy* resolvedTarget, bool hasResolvedTarget, float timeScale) {
    if (!m_isActive || m_destroyed) return;

    m_radius = (m_baseSize * (grid.getCellSize() / 64.0f)) / 2.0f;

    m_lifeTime -= dt;
    if (m_lifeTime <= 0.0f) {
        m_destroyed = true;
        return;
    }

    Enemy* target = nullptr;
    if (m_targetId != -1) {
        if (hasResolvedTarget) {
            if (resolvedTarget != nullptr && !resolvedTarget->isDead() && !resolvedTarget->isReachedEnd() && !resolvedTarget->isFalling()) {
                target = resolvedTarget;
            }
        }
        else {
            for (const auto& enemy : enemies) {
                if (enemy && !enemy->isDead() && !enemy->isReachedEnd() && !enemy->isFalling() && enemy->getId() == m_targetId) {
                    target = enemy.get();
                    break;
                }
            }
        }
    }

    if (target) {
        glm::vec2 toTarget = target->getCollider(grid).center - m_pos;
        float targetAngle = glm::degrees(std::atan2(toTarget.y, toTarget.x));

        float angleDiff = targetAngle - m_angle;
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;

        float rotationStep = m_turnSpeed * dt;

        if (std::abs(angleDiff) <= rotationStep) {
            m_angle = targetAngle;
        }
        else if (angleDiff > 0.0f) {
            m_angle += rotationStep;
        }
        else {
            m_angle -= rotationStep;
        }

        while (m_angle > 180.0f) m_angle -= 360.0f;
        while (m_angle < -180.0f) m_angle += 360.0f;

        float rad = glm::radians(m_angle);
        m_direction = glm::vec2(std::cos(rad), std::sin(rad));
    }
    else {
        m_targetId = -1;
    }

    glm::vec2 prevPos = m_pos;
    m_pos += m_direction * m_speed * dt;

    if (m_hasTrailParticle) {
        m_trailTimer += dt;
        float effectiveTimeScale = 1.0f;
        if (timeScale > 1.0f) {
            effectiveTimeScale = timeScale;
        }
        float trailInterval = 0.045f * effectiveTimeScale;
        if (m_trailTimer >= trailInterval) {
            m_trailTimer = 0.0f;
            if (particleSystem.getActiveCount() < particleSystem.getTrailSoftCap()) {
                ParticleEmitterProps trail = m_trailProps;
                trail.position = m_pos;
                particleSystem.emit(trail, trail.spawnCount, true);
            }
        }
    }

    glm::vec2 travelVec = m_pos - prevPos;
    float travelDistSq = glm::dot(travelVec, travelVec);

    if (m_hitOnlyTarget && target != nullptr) {
        tryHitEnemy(target, prevPos, travelVec, travelDistSq, enemies, grid, particleSystem);
        return;
    }

    for (const auto& enemy : enemies) {
        if (m_hitOnlyTarget && m_targetId != -1 && (!enemy || enemy->getId() != m_targetId)) {
            continue;
        }
        if (tryHitEnemy(enemy.get(), prevPos, travelVec, travelDistSq, enemies, grid, particleSystem)) {
            break;
        }
    }
}

void Projectile::render(SpriteRenderer* renderer, const Texture2D* mainAtlas, const Grid& grid) {
    if (!m_isActive || m_destroyed) return;

    float scaleFactor = grid.getCellSize() / 64.0f;
    float visualSize = m_baseSize * scaleFactor;

    glm::vec2 size(visualSize, visualSize);
    glm::vec2 drawPos = m_pos - glm::vec2(visualSize * 0.5f, visualSize * 0.5f);

    if (mainAtlas != nullptr) {
        renderer->drawSprite(mainAtlas, drawPos, size, m_angle, glm::vec3(1.0f), m_cachedUV);
    }
    else {
        renderer->drawSprite(nullptr, drawPos, size, m_angle, glm::vec3(1.0f, 0.0f, 0.0f));
    }
}

void Projectile::recalculatePosition(const Grid& oldGrid, const Grid& newGrid) {
    if (!m_isActive || m_destroyed) return;

    // коэф изменения размера ячеек
    float ratio = newGrid.getCellSize() / oldGrid.getCellSize();

    // пересчитываем текущую позицию пули
    glm::vec2 localPos = (m_pos - oldGrid.getOffset()) / oldGrid.getCellSize();
    m_pos = (localPos * newGrid.getCellSize()) + newGrid.getOffset();

    // пересчитываем точку старта
    glm::vec2 localOrigin = (m_originPoint - oldGrid.getOffset()) / oldGrid.getCellSize();
    m_originPoint = (localOrigin * newGrid.getCellSize()) + newGrid.getOffset();

    // масштабируем пиксельную скорость полета под новую плотность пикселей на клетку
    m_speed *= ratio;
}