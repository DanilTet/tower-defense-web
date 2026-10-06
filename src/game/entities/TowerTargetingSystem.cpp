#include "TowerTargetingSystem.h"
#include "Enemy.h"
#include "world/Grid.h"
#include <cmath>

Enemy* TowerTargetingSystem::selectTarget(
    const glm::vec2& towerCenter,
    float rangePixels,
    TargetMode mode,
    const std::vector<std::unique_ptr<Enemy>>& enemies,
    const Grid& grid)
{
    CircleCollider towerCollider = { towerCenter, rangePixels };

    Enemy* bestTarget = nullptr;
    float maxTraveled = -1.0f;
    float minTraveled = 999999.0f;
    float minDistance = 999999.0f;
    int minHealth = 999999;

    for (const auto& enemy : enemies) {
        if (!enemy || enemy->isReachedEnd() || enemy->isDead() || enemy->isFalling()) continue;

        if (towerCollider.intersects(enemy->getCollider(grid))) {
            switch (mode) {
            case TargetMode::First:
                if (enemy->getDistanceTraveled() > maxTraveled) {
                    maxTraveled = enemy->getDistanceTraveled();
                    bestTarget = enemy.get();
                }
                break;

            case TargetMode::Last:
                if (enemy->getDistanceTraveled() < minTraveled) {
                    minTraveled = enemy->getDistanceTraveled();
                    bestTarget = enemy.get();
                }
                break;

            case TargetMode::Close: {
                float dist = glm::distance(towerCenter, enemy->getCollider(grid).center);
                if (dist < minDistance) {
                    minDistance = dist;
                    bestTarget = enemy.get();
                }
                break;
            }

            case TargetMode::Weak:
                if (enemy->getHealth() < minHealth) {
                    minHealth = enemy->getHealth();
                    bestTarget = enemy.get();
                }
                break;
            }
        }
    }

    return bestTarget;
}

bool TowerTargetingSystem::updateAim(
    float& currentAngle,
    const glm::vec2& towerCenter,
    const glm::vec2& targetCenter,
    float rotationSpeed,
    float dt)
{
    glm::vec2 dir = targetCenter - towerCenter;
    float targetAngle = glm::degrees(atan2(dir.y, dir.x));

    float angleDiff = targetAngle - currentAngle;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;

    if (std::abs(angleDiff) <= rotationSpeed * dt) {
        currentAngle = targetAngle;
        return true;
    }
    else {
        float direction = (angleDiff > 0.0f) ? 1.0f : -1.0f;
        float rotationStep = direction * rotationSpeed * dt;
        currentAngle += rotationStep;

        if (currentAngle > 180.0f) currentAngle -= 360.0f;
        if (currentAngle < -180.0f) currentAngle += 360.0f;

        return false;
    }
}

