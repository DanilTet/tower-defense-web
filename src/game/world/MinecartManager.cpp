#include "MinecartManager.h"
#include "../entities/Enemy.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../textures/Texture2D.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>

void MinecartManager::rebuildWaypoints() {
    m_waypoints.clear();
    for (const auto& pt : m_railGridCells) {
        glm::vec2 center = m_gridOffset + glm::vec2((pt.x + 0.5f) * m_cellSize, (pt.y + 0.5f) * m_cellSize);
        m_waypoints.push_back(center);
    }
}

void MinecartManager::init(const LevelMapData& levelData, float cellSize, glm::vec2 gridOffset) {
    m_state = CartState::Inactive;
    m_railGridCells.clear();
    m_waypoints.clear();
    m_particles.clear();
    m_floatingTexts.clear();
    m_particleSpawnTimer = 0.0f;
    m_popupCooldown = 0.0f;
    m_cellSize = cellSize;
    m_gridOffset = gridOffset;
    m_currentWaypointIndex = 0;
    m_currentAngle = 0.0f;
    m_pulseTimer = 0.0f;

    if (levelData.minecarts.empty()) return;
    const auto& mc = levelData.minecarts[0];
    if (!mc.hasStart() || !mc.hasEnd()) return;

    glm::ivec2 startPt = mc.start;
    glm::ivec2 endPt = mc.end;
    int gw = levelData.gridWidth;
    int gh = levelData.gridHeight;

    auto isRail = [&](int x, int y) -> bool {
        if (x < 0 || x >= gw || y < 0 || y >= gh) return false;
        if (levelData.layout.empty() || y >= static_cast<int>(levelData.layout.size()) || x >= static_cast<int>(levelData.layout[y].size())) return false;
        return levelData.layout[y][x] == 5; // CellType::Rail
    };

    if (!isRail(startPt.x, startPt.y) || !isRail(endPt.x, endPt.y)) return;

    // 4-направлений BFS від Depot до Buffer
    const glm::ivec2 dirs[4] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
    std::vector<std::vector<glm::ivec2>> parent(gh, std::vector<glm::ivec2>(gw, {-1, -1}));
    std::vector<std::vector<bool>> visited(gh, std::vector<bool>(gw, false));
    std::queue<glm::ivec2> q;

    q.push(startPt);
    visited[startPt.y][startPt.x] = true;
    parent[startPt.y][startPt.x] = startPt;

    bool found = false;
    while (!q.empty()) {
        glm::ivec2 cur = q.front(); q.pop();
        if (cur.x == endPt.x && cur.y == endPt.y) {
            found = true;
            break;
        }
        for (const auto& d : dirs) {
            int nx = cur.x + d.x, ny = cur.y + d.y;
            if (isRail(nx, ny) && !visited[ny][nx]) {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                q.push({nx, ny});
            }
        }
    }

    if (!found) return;

    // Відновлення ланцюжка
    std::vector<glm::ivec2> path;
    glm::ivec2 cur = endPt;
    while (!(cur.x == startPt.x && cur.y == startPt.y)) {
        path.push_back(cur);
        cur = parent[cur.y][cur.x];
    }
    path.push_back(startPt);
    std::reverse(path.begin(), path.end());

    m_railGridCells = std::move(path);
    m_interval = std::max(5.0f, mc.interval);
    m_warningTime = std::clamp(mc.warningTime, 1.0f, m_interval);
    m_speed = std::max(1.0f, mc.speed);

    rebuildWaypoints();

    if (!m_waypoints.empty()) {
        m_state = CartState::Cooldown;
        m_intervalTimer = m_interval;
        m_currentPos = m_waypoints[0];
    }
}

void MinecartManager::updateCellSize(float cellSize, glm::vec2 gridOffset) {
    m_cellSize = cellSize;
    m_gridOffset = gridOffset;
    rebuildWaypoints();
    if (m_currentWaypointIndex < m_waypoints.size()) {
        m_currentPos = m_waypoints[m_currentWaypointIndex];
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

void MinecartManager::spawnMovementParticles(float dt) {
    m_particleSpawnTimer += dt;
    float headingCos = std::cos(m_currentAngle);
    float headingSin = std::sin(m_currentAngle);
    glm::vec2 heading(headingCos, headingSin);
    glm::vec2 lateral(-headingSin, headingCos);

    const float spawnInterval = 0.06f; // Прореженный интервал спавна для предотвращения сплошного ковра
    while (m_particleSpawnTimer >= spawnInterval) {
        m_particleSpawnTimer -= spawnInterval;

        // Ограничиваем максимальное количество одновременных частиц
        if (m_particles.size() >= 45) break;

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
            glm::vec2 wPos = m_currentPos + glm::vec2(
                wheelOffsets[wIdx].x * headingCos - wheelOffsets[wIdx].y * headingSin,
                wheelOffsets[wIdx].x * headingSin + wheelOffsets[wIdx].y * headingCos
            );

            float sideSign = (wheelOffsets[wIdx].y > 0.0f) ? 1.0f : -1.0f;
            float sparkSpeed = (m_speed * m_cellSize * 0.45f) + static_cast<float>(std::rand() % 45);
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
            glm::vec2 rearPos = m_currentPos + glm::vec2(
                rearLocal.x * headingCos - rearLocal.y * headingSin,
                rearLocal.x * headingSin + rearLocal.y * headingCos
            );

            CartParticle smoke;
            smoke.type = CartParticleType::Smoke;
            smoke.pos = rearPos;
            glm::vec2 randVel = glm::vec2(std::rand() % 30 - 15, std::rand() % 30 - 15) * 0.35f;
            smoke.vel = -heading * (m_speed * m_cellSize * 0.15f) + randVel;
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

    for (auto& p : m_particles) {
        p.pos += p.vel * dt;
        if (p.type == CartParticleType::Spark) {
            p.vel *= 0.88f;
        } else {
            p.vel *= 0.94f;
        }
        p.life -= dt;
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
        [](const CartParticle& p) { return p.life <= 0.0f; }), m_particles.end());

    for (auto& ft : m_floatingTexts) {
        ft.pos += ft.vel * dt;
        ft.vel.y += dt * (m_cellSize * 0.6f); // плавное замедление подъема
        ft.life -= dt;
    }
    m_floatingTexts.erase(std::remove_if(m_floatingTexts.begin(), m_floatingTexts.end(),
        [](const FloatingTextEffect& ft) { return ft.life <= 0.0f; }), m_floatingTexts.end());
}

template <typename TEnemyPtr>
void MinecartManager::checkCollisions(std::vector<TEnemyPtr>& enemies) {
    float killRadius = m_cellSize * 0.55f;
    for (auto& enemy : enemies) {
        if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;
        glm::vec2 enemyCenter = enemy->getPixelPos() + glm::vec2(m_cellSize * 0.5f);
        if (glm::distance(m_currentPos, enemyCenter) <= killRadius) {
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

    if (m_state == CartState::Inactive || m_waypoints.empty()) return;

    m_pulseTimer += dt;

    if (m_state == CartState::Cooldown) {
        m_intervalTimer -= dt;
        if (m_intervalTimer <= m_warningTime) {
            m_state = CartState::Warning;
            m_warningTimer = m_warningTime;
            m_pulseTimer = 0.0f;
        }
    } else if (m_state == CartState::Warning) {
        m_warningTimer -= dt;
        if (m_warningTimer <= 0.0f) {
            m_state = CartState::Moving;
            m_currentWaypointIndex = 0;
            m_currentPos = m_waypoints[0];
            if (m_waypoints.size() > 1) {
                glm::vec2 dir = m_waypoints[1] - m_waypoints[0];
                m_currentAngle = std::atan2(dir.y, dir.x);
            } else {
                m_currentAngle = 0.0f;
            }
        }
    } else if (m_state == CartState::Moving) {
        spawnMovementParticles(dt);

        float moveDist = m_speed * m_cellSize * dt;

        while (moveDist > 0.0f && m_currentWaypointIndex < m_waypoints.size()) {
            glm::vec2 target = m_waypoints[m_currentWaypointIndex];
            glm::vec2 toTarget = target - m_currentPos;
            float distToTarget = glm::length(toTarget);

            if (distToTarget > 0.001f) {
                m_currentAngle = std::atan2(toTarget.y, toTarget.x);
            }

            if (moveDist >= distToTarget) {
                m_currentPos = target;
                moveDist -= distToTarget;
                m_currentWaypointIndex++;
            } else {
                m_currentPos += (toTarget / distToTarget) * moveDist;
                moveDist = 0.0f;
            }
        }

        // Знищення ворогів на шляху вагонетки
        checkCollisions(enemies);

        // Досягли кінця маршруту (Тупик)
        if (m_currentWaypointIndex >= m_waypoints.size()) {
            m_state = CartState::Cooldown;
            m_intervalTimer = m_interval;
        }
    }
}

void MinecartManager::update(float dt, std::vector<std::unique_ptr<Enemy>>& enemies) {
    updateInternal(dt, enemies);
}

void MinecartManager::update(float dt, std::vector<std::shared_ptr<Enemy>>& enemies) {
    updateInternal(dt, enemies);
}

void MinecartManager::renderSemaphore(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture) {
    if (m_waypoints.empty()) return;

    glm::vec2 depotCenter = m_waypoints[0];
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

    if (m_state == CartState::Warning) {
        float flash = (std::sin(m_pulseTimer * 14.0f) + 1.0f) * 0.5f;

        // Розширювана тривожна аура навколо семафора (2 хвилі)
        float wave1 = std::fmod(m_pulseTimer * 1.6f, 1.0f);
        float wave2 = std::fmod(m_pulseTimer * 1.6f + 0.5f, 1.0f);
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

    } else if (m_state == CartState::Moving) {
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

void MinecartManager::renderMovingCart(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture) {
    float rotDeg = glm::degrees(m_currentAngle);
    float cosA = std::cos(m_currentAngle);
    float sinA = std::sin(m_currentAngle);

    auto drawLocalQuad = [&](glm::vec2 localOffset, glm::vec2 size, glm::vec3 color) {
        glm::vec2 worldCenter = m_currentPos + glm::vec2(
            localOffset.x * cosA - localOffset.y * sinA,
            localOffset.x * sinA + localOffset.y * cosA
        );
        renderer->drawSprite(whiteTexture, worldCenter - size * 0.5f, size, rotDeg, color);
    };

    auto drawLocalQuadRGBA = [&](glm::vec2 localOffset, glm::vec2 size, glm::vec4 color) {
        glm::vec2 worldCenter = m_currentPos + glm::vec2(
            localOffset.x * cosA - localOffset.y * sinA,
            localOffset.x * sinA + localOffset.y * cosA
        );
        renderer->drawSpriteRGBA(whiteTexture, worldCenter - size * 0.5f, size, rotDeg, color);
    };

    // 1. М'яка падаюча тінь під вагонеткою (зміщення вниз-вправо у світових координатах)
    glm::vec2 shadowWorldCenter = m_currentPos + glm::vec2(m_cellSize * 0.06f, m_cellSize * 0.08f);
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

void MinecartManager::renderParticles(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture) {
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

void MinecartManager::renderFloatingTexts(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture) {
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

void MinecartManager::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture) {
    if (!renderer || !whiteTexture || m_state == CartState::Inactive) return;

    // 1. Семафор на станції початку (замість заливки всієї колії)
    renderSemaphore(renderer, whiteTexture);

    // 2. Системи часток (димок і іскри)
    renderParticles(renderer, whiteTexture);

    // 3. Рухома вагонетка з рудою та відбійником
    if (m_state == CartState::Moving) {
        renderMovingCart(renderer, whiteTexture);
    }

    // 4. Спливаючий коміксний текст (CRUNCH! / SPLAT!)
    renderFloatingTexts(renderer, whiteTexture);
}

