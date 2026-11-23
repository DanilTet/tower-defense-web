#include "MinecartManager.h"
#include "../entities/Enemy.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../textures/Texture2D.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>

void MinecartManager::rebuildWaypoints(CartInstance& cart) {
    cart.waypoints.clear();
    cart.waypoints.reserve(cart.railGridCells.size());
    for (const auto& pt : cart.railGridCells) {
        glm::vec2 center = m_gridOffset + glm::vec2((pt.x + 0.5f) * m_cellSize, (pt.y + 0.5f) * m_cellSize);
        cart.waypoints.push_back(center);
    }
}

void MinecartManager::init(const LevelMapData& levelData, float cellSize, glm::vec2 gridOffset) {
    m_carts.clear();
    m_particles.clear();
    m_floatingTexts.clear();
    m_floatingTexts.reserve(8);
    m_popupCooldown = 0.0f;
    m_killCounter = 0;
    m_cellSize = cellSize;
    m_gridOffset = gridOffset;

    if (levelData.minecarts.empty()) {
        m_particles.reserve(64);
        return;
    }

    int gw = levelData.gridWidth;
    int gh = levelData.gridHeight;
    if (gw <= 0 || gh <= 0) {
        return;
    }

    auto isRail = [&](int x, int y) -> bool {
        if (x < 0 || x >= gw || y < 0 || y >= gh) return false;
        if (levelData.layout.empty() || y >= static_cast<int>(levelData.layout.size()) || x >= static_cast<int>(levelData.layout[y].size())) return false;
        return levelData.layout[y][x] == 5; // CellType::Rail
    };

    const glm::ivec2 dirs[4] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
    std::vector<glm::ivec2> parent(static_cast<size_t>(gw) * static_cast<size_t>(gh), {-1, -1});
    std::vector<uint8_t> visited(static_cast<size_t>(gw) * static_cast<size_t>(gh), 0);

    m_carts.reserve(levelData.minecarts.size());

    for (const auto& mc : levelData.minecarts) {
        if (!mc.hasStart() || !mc.hasEnd()) continue;

        glm::ivec2 startPt = mc.start;
        glm::ivec2 endPt = mc.end;
        if (!isRail(startPt.x, startPt.y) || !isRail(endPt.x, endPt.y)) continue;

        std::fill(visited.begin(), visited.end(), 0);
        std::fill(parent.begin(), parent.end(), glm::ivec2(-1, -1));

        std::queue<glm::ivec2> q;
        q.push(startPt);
        visited[static_cast<size_t>(startPt.y) * gw + startPt.x] = 1;
        parent[static_cast<size_t>(startPt.y) * gw + startPt.x] = startPt;

        bool found = false;
        while (!q.empty()) {
            glm::ivec2 cur = q.front();
            q.pop();
            if (cur.x == endPt.x && cur.y == endPt.y) {
                found = true;
                break;
            }
            for (const auto& d : dirs) {
                int nx = cur.x + d.x;
                int ny = cur.y + d.y;
                if (isRail(nx, ny)) {
                    size_t nIdx = static_cast<size_t>(ny) * gw + nx;
                    if (!visited[nIdx]) {
                        visited[nIdx] = 1;
                        parent[nIdx] = cur;
                        q.push({nx, ny});
                    }
                }
            }
        }

        if (!found) continue;

        // Восстановление цепочки рельсов от Depot к Buffer
        std::vector<glm::ivec2> path;
        glm::ivec2 cur = endPt;
        while (!(cur.x == startPt.x && cur.y == startPt.y)) {
            path.push_back(cur);
            cur = parent[static_cast<size_t>(cur.y) * gw + cur.x];
        }
        path.push_back(startPt);
        std::reverse(path.begin(), path.end());

        CartInstance cart;
        cart.railGridCells = std::move(path);
        cart.interval = std::max(5.0f, mc.interval);
        cart.warningTime = std::clamp(mc.warningTime, 1.0f, cart.interval);
        cart.speed = std::max(1.0f, mc.speed);

        rebuildWaypoints(cart);

        if (!cart.waypoints.empty()) {
            cart.state = CartState::Cooldown;
            cart.intervalTimer = cart.interval;
            cart.currentPos = cart.waypoints[0];
            m_carts.push_back(std::move(cart));
        }
    }

    size_t reserveCount = std::min<size_t>(180, 64 * std::max<size_t>(1, m_carts.size()));
    m_particles.reserve(reserveCount);
}

void MinecartManager::updateCellSize(float cellSize, glm::vec2 gridOffset) {
    m_cellSize = cellSize;
    m_gridOffset = gridOffset;
    for (auto& cart : m_carts) {
        rebuildWaypoints(cart);
        if (cart.currentWaypointIndex < cart.waypoints.size()) {
            cart.currentPos = cart.waypoints[cart.currentWaypointIndex];
        } else if (!cart.waypoints.empty()) {
            cart.currentPos = cart.waypoints.back();
        }
    }
}

void MinecartManager::spawnKillText(glm::vec2 pos) {
    FloatingTextEffect ft;
    ft.text = (m_killCounter++ % 2 == 0) ? "CRUNCH!" : "SPLAT!";
    ft.pos = pos + glm::vec2(0.0f, -m_cellSize * 0.15f);
    float randX = (static_cast<float>(std::rand() % 100) / 100.0f - 0.5f) * m_cellSize * 0.8f;
    ft.vel = glm::vec2(randX, -m_cellSize * 1.5f);
    ft.life = 0.8f;
    ft.maxLife = 0.8f;
    m_floatingTexts.push_back(ft);
}

void MinecartManager::spawnMovementParticles(CartInstance& cart, float dt) {
    cart.particleSpawnTimer += dt;
    float headingCos = std::cos(cart.currentAngle);
    float headingSin = std::sin(cart.currentAngle);
    glm::vec2 heading(headingCos, headingSin);
    glm::vec2 lateral(-headingSin, headingCos);

    const size_t maxParticles = std::min<size_t>(180, 45 * std::max<size_t>(1, m_carts.size()));
    const float spawnInterval = 0.06f; // Прореженный интервал спавна для предотвращения сплошного ковра
    while (cart.particleSpawnTimer >= spawnInterval) {
        cart.particleSpawnTimer -= spawnInterval;

        // Ограничиваем максимальное количество одновременных частиц
        if (m_particles.size() >= maxParticles) break;

        // 1. Искры: спавним только с 1-2 случайных колес за тик вместо всех четырех
        const glm::vec2 wheelOffsets[4] = {
            {  m_cellSize * 0.28f, -m_cellSize * 0.28f },
            {  m_cellSize * 0.28f,  m_cellSize * 0.28f },
            { -m_cellSize * 0.28f, -m_cellSize * 0.28f },
            { -m_cellSize * 0.28f,  m_cellSize * 0.28f }
        };

        int sparksToSpawn = 1 + (std::rand() % 2); // 1 или 2 искры
        for (int s = 0; s < sparksToSpawn; ++s) {
            int wIdx = std::rand() % 4;
            glm::vec2 wPos = cart.currentPos + glm::vec2(
                wheelOffsets[wIdx].x * headingCos - wheelOffsets[wIdx].y * headingSin,
                wheelOffsets[wIdx].x * headingSin + wheelOffsets[wIdx].y * headingCos
            );

            float sideSign = (wheelOffsets[wIdx].y > 0.0f) ? 1.0f : -1.0f;
            float sparkSpeed = (cart.speed * m_cellSize * 0.45f) + static_cast<float>(std::rand() % 45);
            float spreadAngle = (static_cast<float>(std::rand() % 50) - 25.0f) * 0.0174533f;

            glm::vec2 baseDir = -heading * 0.40f + (lateral * sideSign * 0.70f);
            float cSp = std::cos(spreadAngle), sSp = std::sin(spreadAngle);
            glm::vec2 dirRot(baseDir.x * cSp - baseDir.y * sSp, baseDir.x * sSp + baseDir.y * cSp);
            if (glm::length(dirRot) > 0.001f) dirRot = glm::normalize(dirRot);

            CartParticle spark;
            spark.type = CartParticleType::Spark;
            spark.pos = wPos;
            spark.vel = dirRot * sparkSpeed;
            spark.life = 0.14f + static_cast<float>(std::rand() % 14) / 100.0f; // 0.14 - 0.28s
            spark.maxLife = spark.life;
            spark.size = std::max(2.0f, m_cellSize * 0.06f);
            spark.endSize = std::max(1.0f, m_cellSize * 0.02f);
            if (std::rand() % 4 == 0) {
                spark.color = glm::vec4(1.0f, 1.0f, 0.90f, 1.0f); // раскаленная белая искра
            } else {
                spark.color = glm::vec4(1.0f, 0.80f, 0.20f, 1.0f); // золотисто-оранжевая искра
            }
            spark.endColor = glm::vec4(1.0f, 0.25f, 0.05f, 0.0f); // угасает в темно-красный
            m_particles.push_back(spark);
        }

        // 2. Клубы пыли и дыма позади вагонетки (прореживаем - спавним через раз)
        if (std::rand() % 2 == 0) {
            float rearYOffset = (static_cast<float>(std::rand() % 24) - 12.0f) * 0.01f * m_cellSize;
            glm::vec2 rearLocal(-m_cellSize * 0.44f, rearYOffset);
            glm::vec2 rearPos = cart.currentPos + glm::vec2(
                rearLocal.x * headingCos - rearLocal.y * headingSin,
                rearLocal.x * headingSin + rearLocal.y * headingCos
            );

            CartParticle smoke;
            smoke.type = CartParticleType::Smoke;
            smoke.pos = rearPos;
            glm::vec2 randVel = glm::vec2(std::rand() % 30 - 15, std::rand() % 30 - 15) * 0.35f;
            smoke.vel = -heading * (cart.speed * m_cellSize * 0.15f) + randVel;
            smoke.life = 0.40f + static_cast<float>(std::rand() % 20) / 100.0f; // 0.40 - 0.60s
            smoke.maxLife = smoke.life;
            smoke.size = m_cellSize * 0.16f;
            smoke.endSize = m_cellSize * 0.40f;
            smoke.color = glm::vec4(0.35f, 0.33f, 0.30f, 0.38f);
            smoke.endColor = glm::vec4(0.55f, 0.52f, 0.50f, 0.0f);
            m_particles.push_back(smoke);
        }
    }
}

void MinecartManager::updateParticles(float dt) {
    if (m_popupCooldown > 0.0f) {
        m_popupCooldown -= dt;
    }

    size_t pIdx = 0;
    while (pIdx < m_particles.size()) {
        CartParticle& p = m_particles[pIdx];
        p.pos += p.vel * dt;
        if (p.type == CartParticleType::Spark) {
            p.vel *= 0.88f;
        }
        else {
            p.vel *= 0.94f;
        }
        p.life -= dt;
        if (p.life <= 0.0f) {
            if (pIdx + 1 < m_particles.size()) {
                m_particles[pIdx] = m_particles.back();
            }
            m_particles.pop_back();
        }
        else {
            ++pIdx;
        }
    }

    size_t ftIdx = 0;
    while (ftIdx < m_floatingTexts.size()) {
        FloatingTextEffect& ft = m_floatingTexts[ftIdx];
        ft.pos += ft.vel * dt;
        ft.vel.y += dt * (m_cellSize * 0.6f); // плавное замедление подъема
        ft.life -= dt;
        if (ft.life <= 0.0f) {
            if (ftIdx + 1 < m_floatingTexts.size()) {
                m_floatingTexts[ftIdx] = std::move(m_floatingTexts.back());
            }
            m_floatingTexts.pop_back();
        }
        else {
            ++ftIdx;
        }
    }
}

template <typename TEnemyPtr>
void MinecartManager::checkCollisions(const CartInstance& cart, std::vector<TEnemyPtr>& enemies) {
    float killRadius = m_cellSize * 0.55f;
    for (auto& enemy : enemies) {
        if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;
        glm::vec2 enemyCenter = enemy->getPixelPos() + glm::vec2(m_cellSize * 0.5f);
        if (glm::distance(cart.currentPos, enemyCenter) <= killRadius) {
            enemy->takeDamage(999999);
            // Спавним всплывающий текст ТОЛЬКО если m_popupCooldown <= 0.0f И на экране не больше 2 активных текстов
            if (m_popupCooldown <= 0.0f && m_floatingTexts.size() <= 2) {
                spawnKillText(enemyCenter);
                m_popupCooldown = 0.30f; // Задержка ~0.3 с для предотвращения наложения текста
            }
        }
    }
}

template <typename TEnemyPtr>
void MinecartManager::updateInternal(float dt, std::vector<TEnemyPtr>& enemies) {
    updateParticles(dt);

    if (m_carts.empty()) return;

    float maxStepDist = std::max(1.0f, m_cellSize * 0.35f);

    for (auto& cart : m_carts) {
        if (cart.state == CartState::Inactive || cart.waypoints.empty()) continue;

        cart.pulseTimer += dt;

        if (cart.state == CartState::Cooldown) {
            cart.intervalTimer -= dt;
            if (cart.intervalTimer <= cart.warningTime) {
                cart.state = CartState::Warning;
                cart.warningTimer = cart.warningTime;
                cart.pulseTimer = 0.0f;
            }
        } else if (cart.state == CartState::Warning) {
            cart.warningTimer -= dt;
            if (cart.warningTimer <= 0.0f) {
                cart.state = CartState::Moving;
                cart.currentWaypointIndex = 0;
                cart.currentPos = cart.waypoints[0];
                if (cart.waypoints.size() > 1) {
                    glm::vec2 dir = cart.waypoints[1] - cart.waypoints[0];
                    cart.currentAngle = std::atan2(dir.y, dir.x);
                } else {
                    cart.currentAngle = 0.0f;
                }
            }
        } else if (cart.state == CartState::Moving) {
            spawnMovementParticles(cart, dt);

            float moveDist = cart.speed * m_cellSize * dt;

            // Проверяем столкновение в стартовой точке (на случай врага прямо у депо)
            checkCollisions(cart, enemies);

            while (moveDist > 0.0001f && cart.currentWaypointIndex < cart.waypoints.size()) {
                glm::vec2 target = cart.waypoints[cart.currentWaypointIndex];
                glm::vec2 toTarget = target - cart.currentPos;
                float distToTarget = glm::length(toTarget);

                if (distToTarget > 0.001f) {
                    cart.currentAngle = std::atan2(toTarget.y, toTarget.x);
                }

                float stepAllowance = std::min(moveDist, maxStepDist);
                if (distToTarget <= stepAllowance) {
                    cart.currentPos = target;
                    moveDist -= distToTarget;
                    cart.currentWaypointIndex++;
                } else {
                    cart.currentPos += (toTarget / distToTarget) * stepAllowance;
                    moveDist -= stepAllowance;
                }

                // Уничтожение врагов внутри каждого подшага, чтобы на 4x не проскакивать на поворотах
                checkCollisions(cart, enemies);
            }

            // Достигли конца маршрута (Тупик)
            if (cart.currentWaypointIndex >= cart.waypoints.size()) {
                cart.state = CartState::Cooldown;
                cart.intervalTimer = cart.interval;
            }
        }
    }
}

void MinecartManager::update(float dt, std::vector<std::unique_ptr<Enemy>>& enemies) {
    updateInternal(dt, enemies);
}

void MinecartManager::update(float dt, std::vector<std::shared_ptr<Enemy>>& enemies) {
    updateInternal(dt, enemies);
}

void MinecartManager::renderSemaphore(const CartInstance& cart, SpriteRenderer* renderer, const Texture2D* whiteTexture) {
    if (cart.waypoints.empty()) return;

    glm::vec2 depotCenter = cart.waypoints[0];
    glm::vec2 semBase = depotCenter + glm::vec2(-m_cellSize * 0.32f, -m_cellSize * 0.30f);

    float mastW = std::max(3.0f, std::round(m_cellSize * 0.09f));
    float mastH = std::round(m_cellSize * 0.52f);
    glm::vec2 mastPos = semBase - glm::vec2(mastW * 0.5f, mastH);

    // 1. Опорний башмак / фланець мачти
    float baseW = std::round(m_cellSize * 0.22f);
    float baseH = std::max(3.0f, std::round(m_cellSize * 0.08f));
    renderer->drawSprite(whiteTexture, semBase - glm::vec2(baseW * 0.5f, baseH), glm::vec2(baseW, baseH), 0.0f, glm::vec3(0.18f, 0.20f, 0.23f));

    // 2. Вертикальна мачта семафора
    renderer->drawSprite(whiteTexture, mastPos, glm::vec2(mastW, mastH), 0.0f, glm::vec3(0.26f, 0.29f, 0.33f));
    renderer->drawSprite(whiteTexture, mastPos, glm::vec2(std::max(1.0f, mastW * 0.35f), mastH), 0.0f, glm::vec3(0.42f, 0.46f, 0.50f));

    // 3. Коробка сигнальної головки
    float headW = std::round(m_cellSize * 0.26f);
    float headH = std::round(m_cellSize * 0.32f);
    glm::vec2 headPos = mastPos - glm::vec2((headW - mastW) * 0.5f, headH * 0.70f);

    renderer->drawSprite(whiteTexture, headPos - glm::vec2(1.0f), glm::vec2(headW + 2.0f, headH + 2.0f), 0.0f, glm::vec3(0.08f, 0.09f, 0.11f));
    renderer->drawSprite(whiteTexture, headPos, glm::vec2(headW, headH), 0.0f, glm::vec3(0.15f, 0.16f, 0.18f));

    // Козирок від сонця над лінзою
    float visorW = headW + 2.0f;
    float visorH = std::max(2.0f, std::round(m_cellSize * 0.06f));
    renderer->drawSprite(whiteTexture, headPos - glm::vec2(1.0f, visorH - 1.0f), glm::vec2(visorW, visorH), 0.0f, glm::vec3(0.10f, 0.11f, 0.12f));

    // 4. Лінза / лампа
    float lensSize = std::round(m_cellSize * 0.16f);
    glm::vec2 lensPos = headPos + glm::vec2((headW - lensSize) * 0.5f, (headH - lensSize) * 0.5f + 1.0f);
    glm::vec2 lampCenter = lensPos + glm::vec2(lensSize * 0.5f);

    if (cart.state == CartState::Warning) {
        float flash = (std::sin(cart.pulseTimer * 14.0f) + 1.0f) * 0.5f;

        // Розширювана тривожна аура навколо семафора (2 хвилі)
        float wave1 = std::fmod(cart.pulseTimer * 1.6f, 1.0f);
        float wave2 = std::fmod(cart.pulseTimer * 1.6f + 0.5f, 1.0f);
        float r1 = m_cellSize * (0.25f + wave1 * 0.85f);
        float r2 = m_cellSize * (0.25f + wave2 * 0.85f);
        float alpha1 = (1.0f - wave1) * 0.40f * (0.4f + 0.6f * flash);
        float alpha2 = (1.0f - wave2) * 0.40f * (0.4f + 0.6f * flash);

        renderer->drawSpriteRGBA(whiteTexture, lampCenter - glm::vec2(r1), glm::vec2(r1 * 2.0f), 0.0f, glm::vec4(1.0f, 0.15f, 0.10f, alpha1));
        renderer->drawSpriteRGBA(whiteTexture, lampCenter - glm::vec2(r2), glm::vec2(r2 * 2.0f), 0.0f, glm::vec4(1.0f, 0.15f, 0.10f, alpha2));

        // М'яке свічення на зоні депо
        renderer->drawSpriteRGBA(whiteTexture, depotCenter - glm::vec2(m_cellSize * 0.42f), glm::vec2(m_cellSize * 0.84f), 0.0f, glm::vec4(1.0f, 0.18f, 0.10f, 0.14f * flash));

        // Сигнальна лампа: блимає червоним
        glm::vec3 lampCol = glm::mix(glm::vec3(0.45f, 0.08f, 0.06f), glm::vec3(1.0f, 0.20f, 0.12f), flash);
        glm::vec3 coreCol = glm::mix(glm::vec3(0.65f, 0.12f, 0.10f), glm::vec3(1.0f, 0.90f, 0.85f), flash);

        renderer->drawSprite(whiteTexture, lensPos, glm::vec2(lensSize), 0.0f, lampCol);
        float coreSz = lensSize * 0.5f;
        renderer->drawSprite(whiteTexture, lampCenter - glm::vec2(coreSz * 0.5f), glm::vec2(coreSz), 0.0f, coreCol);

    } else if (cart.state == CartState::Moving) {
        // Яскраво-зелений сигнал "Колія відкрита / Вагонетка в русі"
        float haloSz = lensSize * 2.2f;
        renderer->drawSpriteRGBA(whiteTexture, lampCenter - glm::vec2(haloSz * 0.5f), glm::vec2(haloSz), 0.0f, glm::vec4(0.15f, 0.95f, 0.35f, 0.35f));

        glm::vec3 lampCol(0.15f, 0.92f, 0.35f);
        glm::vec3 coreCol(0.80f, 1.0f, 0.85f);
        renderer->drawSprite(whiteTexture, lensPos, glm::vec2(lensSize), 0.0f, lampCol);
        float coreSz = lensSize * 0.5f;
        renderer->drawSprite(whiteTexture, lampCenter - glm::vec2(coreSz * 0.5f), glm::vec2(coreSz), 0.0f, coreCol);

    } else {
        // Cooldown: тьмяний черговий бурштиновий сигнал
        glm::vec3 lampCol(0.62f, 0.40f, 0.10f);
        glm::vec3 coreCol(0.82f, 0.60f, 0.22f);
        renderer->drawSprite(whiteTexture, lensPos, glm::vec2(lensSize), 0.0f, lampCol);
        float coreSz = lensSize * 0.45f;
        renderer->drawSprite(whiteTexture, lampCenter - glm::vec2(coreSz * 0.5f), glm::vec2(coreSz), 0.0f, coreCol);
    }
}

void MinecartManager::renderMovingCart(const CartInstance& cart, SpriteRenderer* renderer, const Texture2D* whiteTexture) {
    float rotDeg = glm::degrees(cart.currentAngle);
    float cosA = std::cos(cart.currentAngle);
    float sinA = std::sin(cart.currentAngle);

    auto drawLocalQuad = [&](glm::vec2 localOffset, glm::vec2 size, glm::vec3 color) {
        glm::vec2 worldCenter = cart.currentPos + glm::vec2(
            localOffset.x * cosA - localOffset.y * sinA,
            localOffset.x * sinA + localOffset.y * cosA
        );
        renderer->drawSprite(whiteTexture, worldCenter - size * 0.5f, size, rotDeg, color);
    };

    auto drawLocalQuadRGBA = [&](glm::vec2 localOffset, glm::vec2 size, glm::vec4 color) {
        glm::vec2 worldCenter = cart.currentPos + glm::vec2(
            localOffset.x * cosA - localOffset.y * sinA,
            localOffset.x * sinA + localOffset.y * cosA
        );
        renderer->drawSpriteRGBA(whiteTexture, worldCenter - size * 0.5f, size, rotDeg, color);
    };

    // 1. М'яка падаюча тінь під вагонеткою (зміщення вниз-вправо у світових координатах)
    glm::vec2 shadowWorldCenter = cart.currentPos + glm::vec2(m_cellSize * 0.06f, m_cellSize * 0.08f);
    glm::vec2 shadowSize(m_cellSize * 0.94f, m_cellSize * 0.68f);
    renderer->drawSpriteRGBA(whiteTexture, shadowWorldCenter - shadowSize * 0.5f, shadowSize, rotDeg, glm::vec4(0.0f, 0.0f, 0.0f, 0.36f));

    // 2. Чотири колеса по боках з металевими ребордами / ободами
    const glm::vec2 wheelOffsets[4] = {
        {  m_cellSize * 0.28f, -m_cellSize * 0.31f },
        {  m_cellSize * 0.28f,  m_cellSize * 0.31f },
        { -m_cellSize * 0.28f, -m_cellSize * 0.31f },
        { -m_cellSize * 0.28f,  m_cellSize * 0.31f }
    };
    for (int i = 0; i < 4; ++i) {
        drawLocalQuad(wheelOffsets[i], glm::vec2(m_cellSize * 0.26f, m_cellSize * 0.11f), glm::vec3(0.18f, 0.19f, 0.22f));
        drawLocalQuad(wheelOffsets[i], glm::vec2(m_cellSize * 0.22f, m_cellSize * 0.05f), glm::vec3(0.58f, 0.62f, 0.68f));
        drawLocalQuad(wheelOffsets[i], glm::vec2(m_cellSize * 0.08f, m_cellSize * 0.04f), glm::vec3(0.84f, 0.86f, 0.90f));
    }

    // 3. Масивний металевий кузов вагонетки
    // Базова рама / шасі
    drawLocalQuad(glm::vec2(0.0f), glm::vec2(m_cellSize * 0.88f, m_cellSize * 0.56f), glm::vec3(0.14f, 0.15f, 0.18f));
    // Борти кузова
    drawLocalQuad(glm::vec2(0.0f), glm::vec2(m_cellSize * 0.80f, m_cellSize * 0.48f), glm::vec3(0.26f, 0.28f, 0.32f));
    // Внутрішня порожнина
    drawLocalQuad(glm::vec2(0.0f), glm::vec2(m_cellSize * 0.72f, m_cellSize * 0.40f), glm::vec3(0.10f, 0.10f, 0.12f));

    // 4. Гірка кіноварної ртутної руди з градієнтом глибини
    // Базовий насип руди (глибокий червоний)
    drawLocalQuad(glm::vec2(-m_cellSize * 0.02f, 0.0f), glm::vec2(m_cellSize * 0.60f, m_cellSize * 0.34f), glm::vec3(0.72f, 0.18f, 0.12f));
    // Середній ярус руди (яскравий кіноварний)
    drawLocalQuad(glm::vec2(-m_cellSize * 0.01f, 0.0f), glm::vec2(m_cellSize * 0.44f, m_cellSize * 0.24f), glm::vec3(0.88f, 0.28f, 0.16f));
    // Вершина насипу (теракотово-помаранчевий)
    drawLocalQuad(glm::vec2(0.0f), glm::vec2(m_cellSize * 0.28f, m_cellSize * 0.14f), glm::vec3(0.96f, 0.42f, 0.20f));

    // Кристалічні вкраплення та краплі ртуті
    drawLocalQuad(glm::vec2(-m_cellSize * 0.14f, -m_cellSize * 0.06f), glm::vec2(m_cellSize * 0.07f), glm::vec3(0.98f, 0.65f, 0.35f));
    drawLocalQuad(glm::vec2( m_cellSize * 0.10f,  m_cellSize * 0.05f), glm::vec2(m_cellSize * 0.08f), glm::vec3(0.98f, 0.70f, 0.40f));
    drawLocalQuad(glm::vec2(-m_cellSize * 0.02f,  m_cellSize * 0.04f), glm::vec2(m_cellSize * 0.05f), glm::vec3(0.92f, 0.94f, 0.98f));

    // 5. Металеві смуги жорсткості та заклепки
    drawLocalQuad(glm::vec2(0.0f), glm::vec2(m_cellSize * 0.08f, m_cellSize * 0.52f), glm::vec3(0.44f, 0.46f, 0.50f));
    drawLocalQuad(glm::vec2( m_cellSize * 0.26f, 0.0f), glm::vec2(m_cellSize * 0.06f, m_cellSize * 0.50f), glm::vec3(0.40f, 0.42f, 0.46f));
    drawLocalQuad(glm::vec2(-m_cellSize * 0.26f, 0.0f), glm::vec2(m_cellSize * 0.06f, m_cellSize * 0.50f), glm::vec3(0.40f, 0.42f, 0.46f));

    // Заклепки по кутах і ребрах
    glm::vec2 rivetSize(m_cellSize * 0.035f);
    glm::vec3 rivetCol(0.82f, 0.84f, 0.88f);
    drawLocalQuad(glm::vec2( m_cellSize * 0.26f, -m_cellSize * 0.24f), rivetSize, rivetCol);
    drawLocalQuad(glm::vec2( m_cellSize * 0.26f,  m_cellSize * 0.24f), rivetSize, rivetCol);
    drawLocalQuad(glm::vec2(0.0f, -m_cellSize * 0.25f), rivetSize, rivetCol);
    drawLocalQuad(glm::vec2(0.0f,  m_cellSize * 0.25f), rivetSize, rivetCol);
    drawLocalQuad(glm::vec2(-m_cellSize * 0.26f, -m_cellSize * 0.24f), rivetSize, rivetCol);
    drawLocalQuad(glm::vec2(-m_cellSize * 0.26f,  m_cellSize * 0.24f), rivetSize, rivetCol);

    // 6. Масивний передній скотоскидач / клиновий відбійник (cowcatcher / wedge bumper)
    drawLocalQuad(glm::vec2(m_cellSize * 0.43f, 0.0f), glm::vec2(m_cellSize * 0.09f, m_cellSize * 0.52f), glm::vec3(0.30f, 0.32f, 0.36f));
    drawLocalQuad(glm::vec2(m_cellSize * 0.48f, 0.0f), glm::vec2(m_cellSize * 0.08f, m_cellSize * 0.42f), glm::vec3(0.48f, 0.50f, 0.54f));
    drawLocalQuad(glm::vec2(m_cellSize * 0.53f, 0.0f), glm::vec2(m_cellSize * 0.07f, m_cellSize * 0.26f), glm::vec3(0.70f, 0.72f, 0.76f));
    // Жовта попереджувальна смуга на клині
    drawLocalQuad(glm::vec2(m_cellSize * 0.51f, 0.0f), glm::vec2(m_cellSize * 0.04f, m_cellSize * 0.18f), glm::vec3(0.92f, 0.80f, 0.15f));
}

void MinecartManager::renderParticles(SpriteRenderer* renderer, const Texture2D* whiteTexture) {
    for (const auto& p : m_particles) {
        float t = 1.0f - std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        float curSize = glm::mix(p.size, p.endSize, t);
        glm::vec4 curCol = glm::mix(p.color, p.endColor, t);
        renderer->drawSpriteRGBA(whiteTexture, p.pos - glm::vec2(curSize * 0.5f), glm::vec2(curSize), 0.0f, curCol);
    }
}

static uint8_t getCharBitmapRow(char c, int row) {
    switch (c) {
    case 'C': {
        const uint8_t r[5] = { 0b01110, 0b10000, 0b10000, 0b10000, 0b01110 };
        return r[row];
    }
    case 'R': {
        const uint8_t r[5] = { 0b11110, 0b10001, 0b11110, 0b10100, 0b10001 };
        return r[row];
    }
    case 'U': {
        const uint8_t r[5] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 };
        return r[row];
    }
    case 'N': {
        const uint8_t r[5] = { 0b10001, 0b11001, 0b10101, 0b10011, 0b10001 };
        return r[row];
    }
    case 'H': {
        const uint8_t r[5] = { 0b10001, 0b10001, 0b11111, 0b10001, 0b10001 };
        return r[row];
    }
    case '!': {
        const uint8_t r[5] = { 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 };
        return r[row];
    }
    case 'S': {
        const uint8_t r[5] = { 0b01111, 0b10000, 0b01110, 0b00001, 0b11110 };
        return r[row];
    }
    case 'P': {
        const uint8_t r[5] = { 0b11110, 0b10001, 0b11110, 0b10000, 0b10000 };
        return r[row];
    }
    case 'L': {
        const uint8_t r[5] = { 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 };
        return r[row];
    }
    case 'A': {
        const uint8_t r[5] = { 0b01110, 0b10001, 0b11111, 0b10001, 0b10001 };
        return r[row];
    }
    case 'T': {
        const uint8_t r[5] = { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100 };
        return r[row];
    }
    default: return 0;
    }
}

void MinecartManager::renderFloatingTexts(SpriteRenderer* renderer, const Texture2D* whiteTexture) {
    for (const auto& ft : m_floatingTexts) {
        float progress = 1.0f - std::clamp(ft.life / ft.maxLife, 0.0f, 1.0f);
        float alpha = std::clamp(ft.life / 0.3f, 0.0f, 1.0f);

        // Пульсація масштабу на старті
        float pop = 1.0f + 0.45f * std::sin(std::clamp(progress / 0.25f, 0.0f, 1.0f) * 3.14159f);
        float blockSize = std::max(2.0f, std::round(m_cellSize * 0.045f)) * pop;

        // Блимання жовтим / червоним
        bool flashYellow = (std::sin(ft.life * 28.0f) >= 0.0f);
        glm::vec3 textColor = flashYellow ? glm::vec3(1.0f, 0.95f, 0.15f) : glm::vec3(1.0f, 0.22f, 0.10f);

        int textLen = static_cast<int>(ft.text.size());
        float totalWidth = static_cast<float>(textLen * 6 - 1) * blockSize;
        float totalHeight = 5.0f * blockSize;

        glm::vec2 origin = ft.pos - glm::vec2(totalWidth * 0.5f, totalHeight * 0.5f);

        // Прохід 1: Тінь (чорний контур зі зміщенням)
        glm::vec2 shadowOffset(blockSize * 0.8f);
        glm::vec4 shadowCol(0.0f, 0.0f, 0.0f, alpha * 0.9f);

        for (int cIdx = 0; cIdx < textLen; ++cIdx) {
            char c = ft.text[cIdx];
            float charX = origin.x + static_cast<float>(cIdx * 6) * blockSize;

            for (int r = 0; r < 5; ++r) {
                uint8_t rowBits = getCharBitmapRow(c, r);
                for (int col = 0; col < 5; ++col) {
                    if ((rowBits >> (4 - col)) & 1) {
                        glm::vec2 blockPos(charX + col * blockSize, origin.y + r * blockSize);
                        renderer->drawSpriteRGBA(whiteTexture, blockPos + shadowOffset, glm::vec2(blockSize), 0.0f, shadowCol);
                    }
                }
            }
        }

        // Прохід 2: Основний соковитий коміксний текст
        glm::vec4 mainCol(textColor, alpha);
        for (int cIdx = 0; cIdx < textLen; ++cIdx) {
            char c = ft.text[cIdx];
            float charX = origin.x + static_cast<float>(cIdx * 6) * blockSize;

            for (int r = 0; r < 5; ++r) {
                uint8_t rowBits = getCharBitmapRow(c, r);
                for (int col = 0; col < 5; ++col) {
                    if ((rowBits >> (4 - col)) & 1) {
                        glm::vec2 blockPos(charX + col * blockSize, origin.y + r * blockSize);
                        renderer->drawSpriteRGBA(whiteTexture, blockPos, glm::vec2(blockSize), 0.0f, mainCol);
                    }
                }
            }
        }
    }
}

void MinecartManager::render(SpriteRenderer* renderer, const Texture2D* whiteTexture) {
    if (!renderer || !whiteTexture || m_carts.empty()) return;

    // 1. Семафоры на станциях начала каждой активной вагонетки
    for (const auto& cart : m_carts) {
        if (cart.state != CartState::Inactive) {
            renderSemaphore(cart, renderer, whiteTexture);
        }
    }

    // 2. Системы частиц (дым и искры)
    renderParticles(renderer, whiteTexture);

    // 3. Движущиеся вагонетки с рудой и отбойником
    for (const auto& cart : m_carts) {
        if (cart.state == CartState::Moving) {
            renderMovingCart(cart, renderer, whiteTexture);
        }
    }

    // 4. Всплывающий комиксный текст (CRUNCH! / SPLAT!)
    renderFloatingTexts(renderer, whiteTexture);
}

