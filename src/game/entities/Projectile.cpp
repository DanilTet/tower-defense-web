#include "Projectile.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "world/Grid.h"
#include "core/ConfigManager.h"
#include "../../particles/ParticleSystem.h"
#include "../../resources/ResourceManager.h"

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
    m_turnSpeed = 540.0f;
    m_originPoint = startPos;
    m_searchRadius = searchRadius;
    m_isActive = true; // и вот тут спящий бизнесмен просыпается

    float rad = glm::radians(m_angle);
    m_direction = glm::vec2(cos(rad), sin(rad));
}

void Projectile::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid, ParticleSystem& particleSystem) {    // пошел ты спящий бизнесмен
    if (!m_isActive || m_destroyed) return;

    // уменьшаем время жизни
    m_lifeTime -= dt;
    if (m_lifeTime <= 0.0f) {
        m_destroyed = true;
        return;
    }

    Enemy* target = nullptr; // враг

    // ищем цель по айдишнику
    if(m_targetId != -1) {
        for (const auto& enemy : enemies) {
            if (enemy && !enemy->isDead() && !enemy->isReachedEnd() && !enemy->isFalling() && enemy->getId() == m_targetId) {
                target = enemy.get();
                break;
            }
        }
    }

    // рулим пулей
    if (target) {
        glm::vec2 toTarget = target->getCollider(grid).center - m_pos;
        float targetAngle = glm::degrees(atan2(toTarget.y, toTarget.x));

        // считаем разницу углов
        float angleDiff = targetAngle - m_angle;
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;

        // лимит поворота за кадр
        float rotationStep = m_turnSpeed * dt;

        if (abs(angleDiff) <= rotationStep) {
            m_angle = targetAngle; // защелкиваемся
        }
        else {
            if (angleDiff > 0) {
                m_angle += rotationStep;
            }
            else {
                m_angle -= rotationStep;
            } // плавно поворачиваем
        }

        // держим угол в границах
        while (m_angle > 180.0f) m_angle -= 360.0f;
        while (m_angle < -180.0f) m_angle += 360.0f;

        // обновляем направление полета
        float rad = glm::radians(m_angle);
        m_direction = glm::vec2(cos(rad), sin(rad));
    }
    else {
        // если враг умер или потерян то сбрасуем айдишник
        m_targetId = -1;
    }
    // сохраняем предыдущую позицию для непрерывной проверки коллизий (CCD)
    glm::vec2 prevPos = m_pos;

    // двигаем пулю
    m_pos += m_direction * m_speed * dt;

    // добавляем треил
    if (!m_trailParticle.empty()) {
        ParticleEmitterProps trail = ConfigManager::getParticleProps(m_trailParticle);
        trail.position = m_pos;
        particleSystem.emit(trail, trail.spawnCount);
    }

    glm::vec2 travelVec = m_pos - prevPos;
    float travelDistSq = glm::dot(travelVec, travelVec);

    // проверка на столкновение
    for (const auto& enemy : enemies) {
        if (!enemy || enemy->isDead() || enemy->isReachedEnd() || enemy->isFalling()) continue;

        // если включен аркадный режим, пропускаем всех врагов, кроме текущей цели
        if (m_hitOnlyTarget && m_targetId != -1 && enemy->getId() != m_targetId) {
            continue;
        }

        CircleCollider enemyCol = enemy->getCollider(grid);
        float combinedRadius = m_radius + enemyCol.radius;

        // Swept CCD: расстояние от центра врага до отрезка пути [prevPos, m_pos]
        float t = (travelDistSq > 0.0001f) 
            ? glm::clamp(glm::dot(enemyCol.center - prevPos, travelVec) / travelDistSq, 0.0f, 1.0f) 
            : 0.0f;
        glm::vec2 closestPoint = prevPos + travelVec * t;
        float distSq = glm::dot(enemyCol.center - closestPoint, enemyCol.center - closestPoint);

        if (distSq <= combinedRadius * combinedRadius) {
            m_pos = closestPoint; // точная точка контакта

            // спавн партиклов взрыва или попадания
            if (!m_impactParticle.empty()) {
                ParticleEmitterProps impact = ConfigManager::getParticleProps(m_impactParticle);
                impact.position = m_pos; // там где было столкновение
                particleSystem.emit(impact, impact.spawnCount);
            }

            if (m_splashRadius > 0.0f) {
                float splashPixels = m_splashRadius * grid.getCellSize();
                CircleCollider explosion = { m_pos, splashPixels };

                for (const auto& otherEnemy : enemies) {
                    if (!otherEnemy || otherEnemy->isDead() || otherEnemy->isReachedEnd() || otherEnemy->isFalling()) continue;
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
            break;
        }
    }


}

void Projectile::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> mainAtlas, const Grid& grid) {
    if (!m_isActive || m_destroyed) return;

    float scaleFactor = grid.getCellSize() / 64.0f;
    m_radius = (m_baseSize * scaleFactor) / 2.0f;

    float paddingCompensation = 3.5f;
    float visualSize = m_baseSize * scaleFactor * paddingCompensation;

    // центрируем увеличенный визуальный квад относительно физической точки m_pos
    glm::vec2 size(visualSize, visualSize);
    glm::vec2 drawPos = m_pos - glm::vec2(visualSize / 2.0f, visualSize / 2.0f);

    // берем текстурку
    std::string regionName = m_textureId;
    if (regionName == "bulletBasic")   regionName = "bullet_basic";
    if (regionName == "bulletSniper")  regionName = "bullet_sniper";
    if (regionName == "bulletCannon")  regionName = "bullet_cannon";
    if (regionName == "bulletMinigun") regionName = "bullet_minigun";

    // рисуем
    SpriteUV uv = ConfigManager::getUV("main_atlas", regionName);

    if (mainAtlas != nullptr) {
        // рисуем пулю
        renderer->drawSprite(mainAtlas, drawPos, size, m_angle, glm::vec3(1.0f), uv);
    }
    else {
        // рисуем красный квадрат-заглушку
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