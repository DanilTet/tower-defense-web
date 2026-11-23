#include "EntityManager.h"
#include "world/Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include <algorithm>
#include <cstdlib>
#include "../core/EventBus.h"
#include "../../particles/ParticleSystem.h"
#include "../../resources/ResourceManager.h"

EntityManager::EntityManager() {
    m_projectiles.reserve(512);
    m_enemies.reserve(256);
    m_towers.reserve(64);
    m_spawnedChildrenBuffer.reserve(32);
    m_enemyLookup.reserve(512);
    m_renderEnemyBuffer.reserve(256);
    m_particleSystem = std::make_unique<ParticleSystem>();
}

void EntityManager::rebuildEnemyLookup() {
    if (m_enemies.empty()) {
        m_enemyLookupValid = false;
        return;
    }

    int minId = 0;
    int maxId = 0;
    bool foundAny = false;

    for (const auto& enemy : m_enemies) {
        if (!enemy || enemy->isDead() || enemy->isReachedEnd() || enemy->isFalling()) {
            continue;
        }
        int id = enemy->getId();
        if (!foundAny) {
            minId = id;
            maxId = id;
            foundAny = true;
        }
        else {
            if (id < minId) minId = id;
            if (id > maxId) maxId = id;
        }
    }

    if (!foundAny || (maxId - minId) > 4096) {
        m_enemyLookupValid = false;
        return;
    }

    m_enemyLookupBaseId = minId;
    size_t span = static_cast<size_t>(maxId - minId + 1);
    m_enemyLookup.assign(span, nullptr);
    for (const auto& enemy : m_enemies) {
        if (!enemy || enemy->isDead() || enemy->isReachedEnd() || enemy->isFalling()) {
            continue;
        }
        int idx = enemy->getId() - minId;
        if (idx >= 0 && static_cast<size_t>(idx) < span) {
            m_enemyLookup[static_cast<size_t>(idx)] = enemy.get();
        }
    }
    m_enemyLookupValid = true;
}

Enemy* EntityManager::findAliveEnemyById(int id) const {
    if (!m_enemyLookupValid || id < m_enemyLookupBaseId) {
        return nullptr;
    }
    size_t idx = static_cast<size_t>(id - m_enemyLookupBaseId);
    if (idx >= m_enemyLookup.size()) {
        return nullptr;
    }
    Enemy* e = m_enemyLookup[idx];
    if (e != nullptr && (e->isDead() || e->isReachedEnd() || e->isFalling())) {
        return nullptr;
    }
    return e;
}

void EntityManager::update(float dt, Grid& gameGrid, float timeScale) {
    m_particleSystem->setTimeScale(timeScale);

    // пробегаемся по всему вектору активных башен на карте
    for (const auto& tower : m_towers) {
        if (tower) {
            tower->update(dt, m_enemies, *this, gameGrid, *m_particleSystem);
        }
    }

    // Подсчёт активных искр для адаптивного троттлинга шлейфа
    int activeSparkCount = 0;
    for (const auto& enemy : m_enemies) {
        if (enemy && enemy->isSpark() && !enemy->isDead() && !enemy->isReachedEnd() && !enemy->isFalling()) {
            activeSparkCount++;
        }
    }

    // Пробегаемся по всему вектору активных врагов на карте
    for (const auto& enemy : m_enemies) {
        if (enemy) {
            enemy->update(dt, gameGrid, m_particleSystem.get(), timeScale, activeSparkCount);
        }
    }

    // Строим O(1) таблицу врагов для быстрого наведения и столкновений пуль
    rebuildEnemyLookup();

    // Обновляем активные пули и удаляем уничтоженные через swap-and-pop
    size_t pIdx = 0;
    while (pIdx < m_projectiles.size()) {
        Projectile& proj = m_projectiles[pIdx];
        Enemy* resolvedTarget = nullptr;
        if (m_enemyLookupValid) {
            resolvedTarget = findAliveEnemyById(proj.getTargetId());
        }
        proj.update(dt, m_enemies, gameGrid, *m_particleSystem, resolvedTarget, m_enemyLookupValid, timeScale);

        if (proj.isDestroyed()) {
            if (pIdx + 1 < m_projectiles.size()) {
                m_projectiles[pIdx] = std::move(m_projectiles.back());
            }
            m_projectiles.pop_back();
        }
        else {
            ++pIdx;
        }
    }

    m_spawnedChildrenBuffer.clear();

    // Обработка погибших и дошедших до базы врагов с удалением через swap-and-pop
    size_t eIdx = 0;
    while (eIdx < m_enemies.size()) {
        const auto& enemy = m_enemies[eIdx];
        if (!enemy) {
            if (eIdx + 1 < m_enemies.size()) {
                m_enemies[eIdx] = std::move(m_enemies.back());
            }
            m_enemies.pop_back();
            continue;
        }

        if (enemy->isDead()) {
            CircleCollider col = enemy->getCollider(gameGrid);
            EventBus::publish({ EventType::EnemyDied, enemy->getReward(), 10, col.center.x, col.center.y, enemy->getDeathSound(), enemy->getType(), enemy->getRewardStep(), enemy->getWaveIndex() });

            ParticleEmitterProps deathProps = enemy->getDeathParticleProps();
            deathProps.position = col.center;
            m_particleSystem->emit(deathProps, deathProps.spawnCount, false);

            if (!enemy->isFalling() && enemy->getSplitCount() > 0 && !enemy->getSplitChildType().empty()) {
                float scatterRadius = enemy->getSplitScatterRadius();
                for (int i = 0; i < enemy->getSplitCount(); ++i) {
                    float randX = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatterRadius;
                    float randY = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatterRadius;
                    glm::vec2 childPos = enemy->getPixelPos() + glm::vec2(randX, randY);

                    auto child = std::make_unique<Enemy>(
                        enemy->getPath(),
                        gameGrid,
                        enemy->getSplitChildType(),
                        enemy->getTargetBaseIndex(),
                        enemy->getCurrentWaypoint(),
                        childPos,
                        enemy->getEnableFadeIn(),
                        enemy->getEnableFadeOut()
                    );
                    child->setDistanceTraveled(enemy->getDistanceTraveled());
                    child->setWaveIndex(enemy->getWaveIndex());
                    child->setReward(enemy->getReward());
                    child->setRewardStep(enemy->getRewardStep());
                    m_spawnedChildrenBuffer.push_back(std::move(child));
                }
            }

            if (eIdx + 1 < m_enemies.size()) {
                m_enemies[eIdx] = std::move(m_enemies.back());
            }
            m_enemies.pop_back();
            continue;
        }

        if (enemy->isReachedEnd()) {
            EventBus::publish({ EventType::EnemyReachedBase, 1 });
            if (eIdx + 1 < m_enemies.size()) {
                m_enemies[eIdx] = std::move(m_enemies.back());
            }
            m_enemies.pop_back();
            continue;
        }

        ++eIdx;
    }

    m_particleSystem->update(dt);

    for (auto& child : m_spawnedChildrenBuffer) {
        m_enemies.push_back(std::move(child));
    }
    m_spawnedChildrenBuffer.clear();
}

void EntityManager::render(SpriteRenderer* renderer, const Texture2D* mainAtlas, const Texture2D* enemyAtlas, const Texture2D* radiusTex, const Texture2D* arrowTex, const Texture2D* particleTex, Grid& gameGrid, Tower* selectedTower) {
    (void)enemyAtlas;
    for (const auto& tower : m_towers) {
        if (tower) {
            bool isSelected = (tower.get() == selectedTower);
            tower->render(renderer, mainAtlas, radiusTex, arrowTex, gameGrid, isSelected);
        }
    }

    // Многопроходный батченный рендер врагов: группируем по текстурам, чтобы не сбрасывать батч на каждом враге
    m_renderEnemyBuffer.clear();
    for (const auto& enemy : m_enemies) {
        if (enemy && !enemy->isReachedEnd() && enemy->getVisibilityAlpha() > 0.001f) {
            m_renderEnemyBuffer.push_back(enemy.get());
        }
    }

    if (!m_renderEnemyBuffer.empty()) {
        std::sort(m_renderEnemyBuffer.begin(), m_renderEnemyBuffer.end(),
            [](const Enemy* a, const Enemy* b) {
                return a->getBodyTexture() < b->getBodyTexture();
            });

        // Проход 1: основные спрайты (1 draw call на тип врага)
        for (const Enemy* enemy : m_renderEnemyBuffer) {
            enemy->renderBody(renderer, gameGrid);
        }

        // Проход 2: раскалённое свечение Coal (1 draw call на все Coal)
        if (!m_coalGlowResolved) {
            m_coalGlowResolved = true;
            if (ResourceManager::hasTexture("enemy_coal_glow")) {
                m_coalGlowTexture = ResourceManager::getTexture("enemy_coal_glow");
            }
        }
        if (m_coalGlowTexture) {
            for (const Enemy* enemy : m_renderEnemyBuffer) {
                if (enemy->isCoal()) {
                    enemy->renderCoalGlow(renderer, m_coalGlowTexture, gameGrid);
                }
            }
        }

        // Проход 3: звёздочки стана
        const Texture2D* starTex = particleTex;
        if (!starTex) {
            starTex = radiusTex;
        }
        if (starTex) {
            for (const Enemy* enemy : m_renderEnemyBuffer) {
                if (enemy->isStunned()) {
                    enemy->renderStunStars(renderer, starTex, gameGrid);
                }
            }
        }

        // Проход 4: полоски здоровья (1 draw call на всех врагов)
        if (!m_whiteTexture) {
            m_whiteTexture = ResourceManager::getWhiteTexture();
        }
        if (m_whiteTexture) {
            for (const Enemy* enemy : m_renderEnemyBuffer) {
                enemy->renderHealthBar(renderer, m_whiteTexture, gameGrid);
            }
        }

        // Проход 5: хитбоксы (кроме Coal и Spark)
        for (const Enemy* enemy : m_renderEnemyBuffer) {
            if (!enemy->isCoal() && !enemy->isSpark()) {
                enemy->renderDebugHitbox(renderer, radiusTex, gameGrid);
            }
        }
    }

    for (auto& proj : m_projectiles) {
        if (proj.isActive()) {
            proj.render(renderer, mainAtlas, gameGrid);
        }
    }

    m_particleSystem->render(renderer, particleTex, gameGrid);
}

Tower* EntityManager::getTowerAt(int gridX, int gridY) {
    for (const auto& tower : m_towers) {
        if (tower && tower->getGridX() == gridX && tower->getGridY() == gridY) {
            return tower.get();
        }
    }
    return nullptr;
}

void EntityManager::removeTower(int gridX, int gridY) {
    m_towers.erase(
        std::remove_if(m_towers.begin(), m_towers.end(),
            [gridX, gridY](const std::unique_ptr<Tower>& tower) {
                return tower->getGridX() == gridX && tower->getGridY() == gridY;
            }),
        m_towers.end()
    );
}

Projectile* EntityManager::getFreeProjectile() {
    if (m_projectiles.size() >= MAX_ACTIVE_PROJECTILES) {
        return nullptr;
    }
    m_projectiles.emplace_back();
    return &m_projectiles.back();
}