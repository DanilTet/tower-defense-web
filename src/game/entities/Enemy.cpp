#include "Enemy.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "world/Grid.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "core/ConfigManager.h"
#include "world/Pathfinder.h"
#include "../../resources/ResourceManager.h"
#include "../ui/HealthBarRenderer.h"
#include "../../particles/ParticleSystem.h"

int Enemy::s_nextId = 0; // инициализация общего счетчика

EnemyStats Enemy::getStatsfromEnemyType(const std::string& type) {
    return ConfigManager::getEnemyStats(type);
}

void Enemy::initFromStats(const EnemyStats& stats) {
    m_speed = stats.speed;
    m_maxHealth = stats.Maxhealth;
    m_health = stats.Maxhealth;
    m_reward = stats.reward;
    m_color = stats.color;
    m_deathSound = stats.deathSound;
    m_deathParticle = stats.deathParticle;
    m_splitChildType = stats.splitChildType;
    m_splitCount = stats.splitCount;
    m_splitScatterRadius = stats.splitScatterRadius;
    m_sizeScale = stats.sizeScale;
    m_textureId = stats.textureId;
    m_radiusMultiplier = stats.sizeScale / 2.0f;

    m_isSpark = (m_type == "Spark");
    m_isCoal = (m_type == "Coal");
    m_isMercury = (m_type == "Mercury_Large" || m_type == "Mercury_Medium" || m_type == "Mercury_Small" || m_type.rfind("Mercury", 0) == 0);
    m_mercuryImmune = stats.mercuryImmune;

    if (!m_textureId.empty() && ResourceManager::hasTexture(m_textureId)) {
        m_cachedTexture = ResourceManager::getTexture(m_textureId);
    }
    else if (ResourceManager::hasTexture("towerTexture")) {
        m_cachedTexture = ResourceManager::getTexture("towerTexture");
    }
    else {
        m_cachedTexture = nullptr;
    }

    if (m_deathParticle.empty()) {
        m_deathParticleProps = ConfigManager::getParticleProps("BloodSplatter");
    }
    else {
        m_deathParticleProps = ConfigManager::getParticleProps(m_deathParticle);
    }

    if (m_isSpark) {
        m_sparkTrailProps = ConfigManager::getParticleProps("SparkTrail");
        if (m_sparkTrailProps.lifeTime <= 0.0f) {
            m_sparkTrailProps.velocityVariation = 40.0f;
            m_sparkTrailProps.startColor = glm::vec3(1.0f, 0.90f, 0.35f);
            m_sparkTrailProps.endColor = glm::vec3(0.95f, 0.25f, 0.02f);
            m_sparkTrailProps.startSize = 7.0f;
            m_sparkTrailProps.endSize = 1.0f;
            m_sparkTrailProps.lifeTime = 0.35f;
            m_sparkTrailProps.spawnCount = 1;
        }
    }

    for (const auto& pair : stats.animations) {
        m_animator.addAnimation(pair.first, pair.second);
    }
    m_animator.play("Walk");
}

// конструктор который вызывается при спавне крипа
Enemy::Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, const std::string& type, int targetBaseIndex, bool enableFadeIn, bool enableFadeOut)
    : m_path(gridPath), // сохраняем ссылку на маршрут врага по клеткам сетки
    m_currentWayPoint(0), // начинаем с первой контрольной точки маршрута
    m_reachedEnd(false), // изначально враг не достиг конца маршрута
    m_type(type), // сохраняем тип врага
    m_distanceTraveled(0.0f),// инициализация одометра
    m_aliveTime(0.0f),
    m_targetBaseIndex(targetBaseIndex), // СОХРАНЯЕМ ИНДЕКС БАЗЫ
    m_enableFadeIn(enableFadeIn),
    m_enableFadeOut(enableFadeOut)
{
    m_id = s_nextId++; // выдаем уникальный айди

    EnemyStats stats = Enemy::getStatsfromEnemyType(type);
    initFromStats(stats);

    if (!m_path.empty()) {
        m_pixelPos = grid.gridToPixel(m_path[0].x, m_path[0].y);
    }

    m_visibilityAlpha = m_enableFadeIn ? 0.0f : 1.0f;
    updateVisibilityAlpha(grid);
}

Enemy::Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, const std::string& type, int targetBaseIndex, size_t currentWaypoint, glm::vec2 spawnPixelPos, bool enableFadeIn, bool enableFadeOut)
    : m_path(gridPath),
    m_currentWayPoint(currentWaypoint),
    m_reachedEnd(false),
    m_type(type),
    m_distanceTraveled(0.0f),
    m_aliveTime(0.0f),
    m_targetBaseIndex(targetBaseIndex),
    m_pixelPos(spawnPixelPos),
    m_enableFadeIn(enableFadeIn),
    m_enableFadeOut(enableFadeOut)
{
    m_id = s_nextId++;

    EnemyStats stats = Enemy::getStatsfromEnemyType(type);
    initFromStats(stats);

    if (m_currentWayPoint >= m_path.size()) {
        m_currentWayPoint = m_path.empty() ? 0 : m_path.size() - 1;
    }

    m_distanceTraveled = 0.0f;
    for (size_t i = 1; i <= m_currentWayPoint && i < m_path.size(); ++i) {
        m_distanceTraveled += glm::distance(
            grid.gridToPixel(m_path[i].x, m_path[i].y),
            grid.gridToPixel(m_path[i - 1].x, m_path[i - 1].y)
        );
    }
    updateVisibilityAlpha(grid);
}

void Enemy::updateVisibilityAlpha(const Grid& grid) {
    float cellSize = grid.getCellSize();
    if (cellSize <= 0.001f) {
        m_visibilityAlpha = 1.0f;
        return;
    }

    float fadeDist = 1.5f * cellSize;

    // 1. При спавне: alpha плавно растет от 0.0f до 1.0f на первых 1.5 тайлах пройденного пути
    float spawnAlpha = 1.0f;
    if (m_enableFadeIn) {
        spawnAlpha = std::clamp(m_distanceTraveled / fadeDist, 0.0f, 1.0f);
    }

    // 2. При подходе к базе: быстрый выход, если до конца пути больше 3 клеток
    float baseAlpha = 1.0f;
    if (m_enableFadeOut) {
        if (m_reachedEnd) {
            baseAlpha = 0.0f;
        }
        else if (m_currentWayPoint + 3 < m_path.size()) {
            baseAlpha = 1.0f;
        }
        else if (m_currentWayPoint < m_path.size()) {
            glm::vec2 targetPixelPos = grid.gridToPixel(m_path[m_currentWayPoint].x, m_path[m_currentWayPoint].y);
            float remainingDist = glm::distance(m_pixelPos, targetPixelPos);
            for (size_t i = m_currentWayPoint; i + 1 < m_path.size(); ++i) {
                if (remainingDist >= fadeDist) {
                    break;
                }
                remainingDist += glm::distance(
                    grid.gridToPixel(m_path[i].x, m_path[i].y),
                    grid.gridToPixel(m_path[i + 1].x, m_path[i + 1].y)
                );
            }
            baseAlpha = std::clamp(remainingDist / fadeDist, 0.0f, 1.0f);
        }
        else {
            baseAlpha = 0.0f;
        }
    }

    m_visibilityAlpha = std::clamp(std::min(spawnAlpha, baseAlpha), 0.0f, 1.0f);
}

// Обновление логики и расчет движения
void Enemy::update(float dt, const Grid& grid, ParticleSystem* particleSystem, float timeScale, int activeSparkCount) {
    if (m_reachedEnd) return;
    m_aliveTime += dt;

    // 0. Состояние падения в шурф / обрыв (CellType::Chasm)
    if (m_isFalling) {
        m_fallTimer += dt;
        if (m_fallTimer >= FALL_DURATION) {
            // Мгновенная гибель (Instakill) на дне шурфа без нанесения урона базе!
            m_health = 0;
        }
        return; // Во время падения любое другое движение и действия заблокированы
    }

    // Если не падает и нет пути, и не в толчке — выходим
    if (m_path.empty() && !m_isKnockedBack) return;

    // --- ОБРАБОТКА СТАТУС-ЭФФЕКТОВ (Замедление и Яд) ---
    float maxSlow = 0.0f;
    bool anyEffectExpired = false;
    for (auto& effect : m_statusEffects) {
        effect.duration -= dt;
        if (effect.duration <= 0.0f) {
            anyEffectExpired = true;
        }
        if (effect.type == StatusType::Slow) {
            if (effect.intensity > maxSlow) {
                maxSlow = effect.intensity;
            }
        }
        else if (effect.type == StatusType::Poison) {
            effect.tickTimer += dt;
            if (effect.tickTimer >= effect.tickInterval) {
                effect.tickTimer -= effect.tickInterval;
                m_health -= effect.damagePerTick;
                if (m_health <= 0) {
                    m_health = 0;
                    return; // Враг погиб от яда, движение прекращается
                }
            }
        }
        else if (effect.type == StatusType::Bleed) {
            effect.tickTimer += dt;
            if (effect.tickTimer >= effect.tickInterval) {
                effect.tickTimer -= effect.tickInterval;
                m_health -= effect.damagePerTick;
                if (m_health <= 0) {
                    m_health = 0;
                    return; // Враг погиб от кровотечения, движение прекращается
                }
            }
        }
    }

    // Удаляем истекшие эффекты через swap-and-pop
    if (anyEffectExpired) {
        size_t idx = 0;
        while (idx < m_statusEffects.size()) {
            if (m_statusEffects[idx].duration <= 0.0f) {
                if (idx + 1 < m_statusEffects.size()) {
                    m_statusEffects[idx] = m_statusEffects.back();
                }
                m_statusEffects.pop_back();
            }
            else {
                ++idx;
            }
        }
    }

    // Кап замедления (макс 80%, чтобы мобы не застревали намертво)
    if (maxSlow > 0.8f) maxSlow = 0.8f;
    m_speedModifier = 1.0f - maxSlow;

    // Таймеры кулдауна для конкретных поршней (swap-and-pop без сдвига массива)
    size_t cdIdx = 0;
    while (cdIdx < m_pistonCooldowns.size()) {
        m_pistonCooldowns[cdIdx].second -= dt;
        if (m_pistonCooldowns[cdIdx].second <= 0.0f) {
            if (cdIdx + 1 < m_pistonCooldowns.size()) {
                m_pistonCooldowns[cdIdx] = m_pistonCooldowns.back();
            }
            m_pistonCooldowns.pop_back();
        }
        else {
            ++cdIdx;
        }
    }

    // Анимация отдачи при ударе о стену
    if (m_wallImpactTimer > 0.0f) {
        m_wallImpactTimer -= dt;
        if (m_wallImpactTimer <= 0.0f) {
            m_wallImpactTimer = 0.0f;
            m_wallImpactOffset = glm::vec2(0.0f);
        }
        else {
            float t = m_wallImpactTimer / 0.12f;
            m_wallImpactOffset *= t;
        }
    }

    // Таймер оглушения (при ударе о препятствие)
    if (m_stunTimer > 0.0f) {
        m_stunAnimAngle += dt * 7.0f;
        m_stunTimer -= dt;
        if (m_stunTimer <= 0.0f) {
            m_stunTimer = 0.0f;
        }
        // Во время оглушения враг стоит на месте
        return;
    }

    // Обработка контролируемого отталкивания ровно на 1 клетку (поршень)
    if (m_isKnockedBack) {
        m_knockbackTimer += dt;
        float t = m_knockbackTimer / m_knockbackDuration;
        if (t >= 1.0f) {
            m_pixelPos = m_knockbackTargetPos;
            m_isKnockedBack = false;

            // Определяем клетку приземления
            glm::ivec2 landingCell = grid.pixelToGrid(m_pixelPos + glm::vec2(grid.getCellSize() * 0.5f));

            // Если конечная клетка или клетка приземления оказалась шурфом/обрывом — гарантированно вызываем падение (инстакилл)!
            if (m_knockbackDestCell.x >= 0 && m_knockbackDestCell.x < grid.getWidth() &&
                m_knockbackDestCell.y >= 0 && m_knockbackDestCell.y < grid.getHeight() &&
                grid.getCellType(m_knockbackDestCell.x, m_knockbackDestCell.y) == CellType::Chasm) {
                startFalling(m_knockbackDestCell, grid);
                return;
            }
            if (landingCell.x >= 0 && landingCell.x < grid.getWidth() &&
                landingCell.y >= 0 && landingCell.y < grid.getHeight() &&
                grid.getCellType(landingCell.x, landingCell.y) == CellType::Chasm) {
                startFalling(landingCell, grid);
                return;
            }

            // Перенастраиваем маршрут, чтобы враг дальше шёл от новой клетки
            applyPostKnockbackPath(m_knockbackDestCell, m_knockbackFromCell, grid);
        }
        else {
            // Эффект удара (ease-out quad: сначала резкий импульс, затем плавное торможение)
            float easeOut = 1.0f - (1.0f - t) * (1.0f - t);
            m_pixelPos = glm::mix(m_knockbackStartPos, m_knockbackTargetPos, easeOut);
        }

        updateVisibilityAlpha(grid);
        // Во время отталкивания стандартное движение по маршруту заблокировано
        return;
    }

    // Дополнительная проверка на нахождение на клетке обрыва
    glm::ivec2 curCell = grid.pixelToGrid(m_pixelPos + glm::vec2(grid.getCellSize() * 0.5f));
    if (curCell.x >= 0 && curCell.x < grid.getWidth() &&
        curCell.y >= 0 && curCell.y < grid.getHeight() &&
        grid.getCellType(curCell.x, curCell.y) == CellType::Chasm) {
        startFalling(curCell, grid);
        return;
    }

    // Проверка безопасности: если целевая клетка заблокирована башней, ищем проходимую точку дальше
    if (m_currentWayPoint < m_path.size()) {
        glm::ivec2 checkCell = m_path[m_currentWayPoint];
        if (!Pathfinder::isCellWalkable(grid, checkCell.x, checkCell.y)) {
            // Ищем дальше по m_path первую проходимую точку
            size_t nextWalkable = m_currentWayPoint + 1;
            while (nextWalkable < m_path.size() && !Pathfinder::isCellWalkable(grid, m_path[nextWalkable].x, m_path[nextWalkable].y)) {
                nextWalkable++;
            }
            if (nextWalkable < m_path.size()) {
                m_currentWayPoint = nextWalkable;
            } else {
                // Впереди по текущему пути нет проходимых точек.
                // Не очищаем m_path вслепую, чтобы враг не завис навсегда,
                // а ждем пересчета пути (recalculatePath).
                return;
            }
        }
    }

    if (m_currentWayPoint >= m_path.size()) {
        m_reachedEnd = true;
        return;
    }

    m_animator.update(dt); // прокрутка анимации

    // Определяем контрольную точку маршрута к которой идем
    glm::ivec2 targetCell = m_path[m_currentWayPoint];

	// переводим индексы контрольной точки в пиксели экрана
    glm::vec2 targetPixelPos = grid.gridToPixel(targetCell.x, targetCell.y);

	// Считаем вектор от текущей позиции врага до контрольной точки
    glm::vec2 toTarget = targetPixelPos - m_pixelPos;
    float distance = glm::length(toTarget);

    if (distance > 0.001f) {
        m_angle = glm::degrees(atan2(toTarget.y, toTarget.x));
    }
    
    // перевод скорости в пиксели в секунду с учетом замедления
    float currentPixelSpeed = m_speed * m_speedModifier * grid.getCellSize();

	// Считаем сколько пикселей враг может пройти за этот кадр, умножая скорость на deltaTime
    float moveDistance = currentPixelSpeed * dt;

    // если расстояние до чекпоинта меньше или равно шагу, который мы можем сделать
    if (distance <= moveDistance) {
        m_distanceTraveled += distance;// плюсуем пройденое расстояние за кадр
        m_pixelPos = targetPixelPos; // приравниваем позицию врага к координатам цели
        m_currentWayPoint++; // теперь целью становится следующий чекпоинт

        // если индекс новой цели превысил размер массива маршрута — чекпоинты закончились
        if (m_currentWayPoint >= m_path.size()) {
            m_reachedEnd = true; // Выставляем флаг, что враг успешно закончил свой путь
        }
    }
    // если до контрольной точк далеко
    else {
		// Нормализуем вектор до контрольной точки, чтобы получить направление движения
        glm::vec2 direction = toTarget / distance;
		// Двигаем врага в направлении контрольной точки на расстояние, которое он может пройти за этот кадр
        m_pixelPos += direction * moveDistance;
        m_distanceTraveled += moveDistance; // плюсуем пройденое расстояние за кадр
    }


    // Эффект движения (Шлейф искр) для типа "Spark" с адаптивным троттлингом под 2x-4x и толпу
    if (m_isSpark && particleSystem != nullptr && !m_reachedEnd && !m_isFalling && m_health > 0 && distance > 0.001f && m_speedModifier > 0.01f) {
        m_sparkTrailTimer += dt;

        float effectiveTimeScale = 1.0f;
        if (timeScale > 1.0f) {
            effectiveTimeScale = timeScale;
        }
        float spawnInterval = 0.040f * effectiveTimeScale;

        if (activeSparkCount > 15) {
            float crowdFactor = static_cast<float>(activeSparkCount) / 15.0f;
            if (crowdFactor > 4.0f) {
                crowdFactor = 4.0f;
            }
            spawnInterval *= crowdFactor;
        }

        size_t activeParticles = particleSystem->getActiveCount();
        size_t trailCap = particleSystem->getTrailSoftCap();
        if (activeParticles < trailCap) {
            if (activeParticles > trailCap / 2) {
                spawnInterval *= 1.75f;
            }

            if (m_sparkTrailTimer >= spawnInterval) {
                m_sparkTrailTimer = 0.0f;
                glm::vec2 moveDir = toTarget / distance;
                glm::vec2 enemyCenter = m_pixelPos + glm::vec2(grid.getCellSize() * 0.5f);

                ParticleEmitterProps props = m_sparkTrailProps;
                props.position = enemyCenter;
                props.velocityDir = -moveDir * 65.0f;
                particleSystem->emit(props, props.spawnCount, true);
            }
        }
        else if (m_sparkTrailTimer > spawnInterval) {
            m_sparkTrailTimer = spawnInterval;
        }
    }
    updateVisibilityAlpha(grid);
}

const Texture2D* Enemy::getBodyTexture() const {
    if (m_cachedTexture != nullptr) {
        return m_cachedTexture;
    }
    if (!m_textureId.empty() && ResourceManager::hasTexture(m_textureId)) {
        return ResourceManager::getTexture(m_textureId);
    }
    if (ResourceManager::hasTexture("towerTexture")) {
        return ResourceManager::getTexture("towerTexture");
    }
    return nullptr;
}

Enemy::RenderGeometry Enemy::computeRenderGeometry(const Grid& grid) const {
    RenderGeometry geom{};
    float cellSize = grid.getCellSize();

    geom.baseEnemyDim = cellSize * m_sizeScale;
    geom.currentEnemyDim = geom.baseEnemyDim;
    float baseAngle = m_isMercury ? 0.0f : m_angle;
    geom.drawAngle = baseAngle;
    geom.renderColor = m_color;

    if (m_isFalling) {
        float progress = std::clamp(m_fallTimer / FALL_DURATION, 0.0f, 1.0f);
        float scale = 1.0f - progress;
        geom.currentEnemyDim = geom.baseEnemyDim * scale;
        geom.drawAngle = baseAngle + progress * 720.0f;
        float shade = (1.0f - progress * 0.90f);
        geom.renderColor = glm::mix(m_color, glm::vec3(0.02f, 0.02f, 0.04f), progress) * shade;
    }
    else if (m_stunTimer > 0.0f) {
        geom.renderColor = glm::vec3(1.0f, 0.85f, 0.25f);
    }
    else if (!m_statusEffects.empty()) {
        bool poisoned = false;
        bool slowed = false;
        bool bleedingOrVulnerable = false;
        for (const auto& e : m_statusEffects) {
            if (e.duration <= 0.0f) {
                continue;
            }
            if (e.type == StatusType::Poison) {
                poisoned = true;
            }
            else if (e.type == StatusType::Slow) {
                slowed = true;
            }
            else if (e.type == StatusType::Bleed || e.type == StatusType::Vulnerable) {
                bleedingOrVulnerable = true;
            }
        }

        if (bleedingOrVulnerable) {
            geom.renderColor = glm::vec3(1.0f, 0.35f, 0.35f);
        }
        else if (poisoned && slowed) {
            geom.renderColor = glm::vec3(0.3f, 0.95f, 0.75f);
        }
        else if (poisoned) {
            geom.renderColor = glm::vec3(0.35f, 1.0f, 0.35f);
        }
        else if (slowed) {
            geom.renderColor = glm::vec3(0.45f, 0.75f, 1.0f);
        }
    }

    geom.uv = m_animator.getCurrentUV();
    geom.size = glm::vec2(geom.currentEnemyDim, geom.currentEnemyDim);
    const Texture2D* bodyTex = getBodyTexture();
    if (bodyTex != nullptr && bodyTex->getWidth() > 0 && bodyTex->getHeight() > 0) {
        float srcW = static_cast<float>(bodyTex->getWidth()) * std::abs(geom.uv.uvMax.x - geom.uv.uvMin.x);
        float srcH = static_cast<float>(bodyTex->getHeight()) * std::abs(geom.uv.uvMax.y - geom.uv.uvMin.y);
        if (srcW > 0.0f && srcH > 0.0f) {
            if (srcH >= srcW) {
                geom.size = glm::vec2(geom.currentEnemyDim * (srcW / srcH), geom.currentEnemyDim);
            }
            else {
                geom.size = glm::vec2(geom.currentEnemyDim, geom.currentEnemyDim * (srcH / srcW));
            }
        }
    }
    geom.cellCenter = m_pixelPos + glm::vec2(cellSize * 0.5f);
    geom.drawPos = geom.cellCenter - geom.size * 0.5f + m_wallImpactOffset;
    return geom;
}

void Enemy::renderBody(SpriteRenderer* renderer, const Grid& grid) const {
    if (m_reachedEnd || m_visibilityAlpha <= 0.001f) {
        return;
    }

    RenderGeometry geom = computeRenderGeometry(grid);
    if (geom.currentEnemyDim <= 0.5f) {
        return;
    }

    const Texture2D* enemyTexPtr = getBodyTexture();
    if (m_isSpark) {
        float phase = static_cast<float>(m_id) * 2.391f;
        float vib1 = std::sin(m_aliveTime * 22.0f + phase);
        float vib2 = std::sin(m_aliveTime * 37.0f + phase * 1.618f);
        float vib3 = std::cos(m_aliveTime * 53.0f + phase * 2.718f);
        float flamePulse = 0.5f + 0.30f * vib1 + 0.12f * vib2 + 0.08f * vib3;

        // 1. Внешний огненно-оранжевый ореол
        float haloScale = 1.30f + 0.22f * flamePulse;
        glm::vec2 haloSize = geom.size * haloScale;
        glm::vec2 haloPos = geom.cellCenter - haloSize * 0.5f + m_wallImpactOffset;
        glm::vec3 flameOrange = glm::vec3(1.0f, 0.40f, 0.05f) * geom.renderColor;
        float haloAlpha = std::clamp((0.50f + 0.45f * flamePulse) * m_visibilityAlpha, 0.0f, 1.0f);
        renderer->drawSpriteRGBA(enemyTexPtr, haloPos, haloSize, geom.drawAngle, glm::vec4(flameOrange, haloAlpha), geom.uv);

        // 2. Основное тело искры
        renderer->drawSpriteRGBA(enemyTexPtr, geom.drawPos, geom.size, geom.drawAngle, glm::vec4(geom.renderColor, m_visibilityAlpha), geom.uv);

        // 3. Яркое раскаленное ядро
        float coreScale = 0.70f + 0.12f * vib1;
        glm::vec2 coreSize = geom.size * coreScale;
        glm::vec2 corePos = geom.cellCenter - coreSize * 0.5f + m_wallImpactOffset;
        glm::vec3 coreGlow = glm::mix(glm::vec3(1.0f, 0.90f, 0.40f), glm::vec3(1.22f, 1.18f, 0.96f), flamePulse) * geom.renderColor;
        float coreAlpha = std::clamp((0.80f + 0.20f * flamePulse) * m_visibilityAlpha, 0.0f, 1.0f);
        renderer->drawSpriteRGBA(enemyTexPtr, corePos, coreSize, geom.drawAngle, glm::vec4(coreGlow, coreAlpha), geom.uv);
    }
    else {
        renderer->drawSpriteRGBA(enemyTexPtr, geom.drawPos, geom.size, geom.drawAngle, glm::vec4(geom.renderColor, m_visibilityAlpha), geom.uv);
    }
}

void Enemy::renderCoalGlow(SpriteRenderer* renderer, const Texture2D* coalGlowTex, const Grid& grid) const {
    if (!m_isCoal || m_reachedEnd || m_visibilityAlpha <= 0.001f || !coalGlowTex) {
        return;
    }

    RenderGeometry geom = computeRenderGeometry(grid);
    if (geom.currentEnemyDim <= 0.5f) {
        return;
    }

    float phase = static_cast<float>(m_id) * 1.731f;
    float p1 = 0.5f + 0.5f * std::sin(m_aliveTime * 3.4f + phase);
    float p2 = 0.5f + 0.5f * std::sin(m_aliveTime * 5.8f + phase * 1.37f);
    float pulse = p1 * 0.75f + p2 * 0.25f;

    glm::vec3 coreGlow = glm::mix(glm::vec3(1.0f, 0.45f, 0.12f), glm::vec3(1.18f, 0.92f, 0.38f), pulse);
    glm::vec3 finalGlowColor = geom.renderColor * coreGlow;
    float glowAlpha = std::clamp((0.45f + 0.55f * pulse) * m_visibilityAlpha, 0.0f, 1.0f);

    renderer->drawSpriteRGBA(coalGlowTex, geom.drawPos, geom.size, geom.drawAngle, glm::vec4(finalGlowColor, glowAlpha), geom.uv);
}

void Enemy::renderStunStars(SpriteRenderer* renderer, const Texture2D* starTex, const Grid& grid) const {
    if (m_reachedEnd || m_isFalling || m_stunTimer <= 0.0f || m_visibilityAlpha <= 0.001f || !starTex) {
        return;
    }

    float cellSize = grid.getCellSize();
    float enemyDim = cellSize * m_sizeScale;
    glm::vec2 size(enemyDim, enemyDim);
    glm::vec2 cellCenter = m_pixelPos + glm::vec2(cellSize * 0.5f);
    glm::vec2 drawPos = cellCenter - size * 0.5f + m_wallImpactOffset;

    glm::vec2 headCenter = drawPos + glm::vec2(size.x * 0.5f, -size.y * 0.05f);
    float rx = size.x * 0.42f;
    float ry = size.y * 0.16f;
    glm::vec2 starSize(size.x * 0.22f, size.x * 0.22f);

    for (int i = 0; i < 3; ++i) {
        float starAngle = m_stunAnimAngle + i * (2.0f * 3.14159265f / 3.0f);
        glm::vec2 starPos = headCenter + glm::vec2(std::cos(starAngle) * rx, std::sin(starAngle) * ry);
        float starRot = glm::degrees(starAngle * 2.5f);
        glm::vec2 starDrawPos = starPos - starSize * 0.5f;
        renderer->drawSpriteRGBA(starTex, starDrawPos, starSize, starRot, glm::vec4(1.0f, 0.95f, 0.25f, m_visibilityAlpha));
    }
}

void Enemy::renderHealthBar(SpriteRenderer* renderer, const Texture2D* barTex, const Grid& grid) const {
    if (m_reachedEnd || m_isFalling || m_visibilityAlpha < 0.2f || !barTex) {
        return;
    }

    float cellSize = grid.getCellSize();
    float baseEnemyDim = cellSize * m_sizeScale;
    glm::vec2 cellCenter = m_pixelPos + glm::vec2(cellSize * 0.5f);
    glm::vec2 drawPos = cellCenter - glm::vec2(baseEnemyDim * 0.5f) + m_wallImpactOffset;

    HealthBarRenderer::draw(renderer, barTex, drawPos, baseEnemyDim, m_health, m_maxHealth, -10.0f, 6.0f, m_visibilityAlpha);
}

void Enemy::renderDebugHitbox(SpriteRenderer* renderer, const Texture2D* radiusTex, const Grid& grid) const {
    if (m_reachedEnd || m_isFalling || m_isCoal || m_isSpark || m_isMercury || m_visibilityAlpha <= 0.001f || !radiusTex) {
        return;
    }

    CircleCollider collider = getCollider(grid);
    glm::vec2 hitboxSize(collider.radius * 2.0f, collider.radius * 2.0f);
    glm::vec2 hitboxPos = (collider.center + m_wallImpactOffset) - glm::vec2(collider.radius, collider.radius);
    renderer->drawSpriteRGBA(radiusTex, hitboxPos, hitboxSize, 0.0f, glm::vec4(1.0f, 0.0f, 0.0f, m_visibilityAlpha));
}

// отрисовка врага на экране
void Enemy::render(SpriteRenderer* renderer, const Texture2D* texture, const Texture2D* radiusTex, glm::vec2 gridOffset, const Grid& grid) {
    (void)texture;
    (void)gridOffset;
    if (m_reachedEnd || m_visibilityAlpha <= 0.001f) return;

    renderBody(renderer, grid);

    if (m_isCoal && ResourceManager::hasTexture("enemy_coal_glow")) {
        renderCoalGlow(renderer, ResourceManager::getTexture("enemy_coal_glow"), grid);
    }

    if (!m_isFalling) {
        if (m_stunTimer > 0.0f) {
            const Texture2D* starTex = nullptr;
            if (ResourceManager::hasTexture("particleTexture")) {
                starTex = ResourceManager::getTexture("particleTexture");
            }
            if (!starTex) {
                starTex = radiusTex;
            }
            renderStunStars(renderer, starTex, grid);
        }

        renderHealthBar(renderer, ResourceManager::getWhiteTexture(), grid);
        renderDebugHitbox(renderer, radiusTex, grid);
    }
}

void Enemy::recalculatePosition(const Grid& oldGrid, const Grid& newGrid) {
    if (m_isKnockedBack) {
        m_knockbackStartPos = newGrid.gridToPixel(m_knockbackFromCell.x, m_knockbackFromCell.y);
        m_knockbackTargetPos = newGrid.gridToPixel(m_knockbackDestCell.x, m_knockbackDestCell.y);
    }

    //пропорционально пересчитываем позицию врага в пикселях на основе его текущей контрольной точки маршрута, чтобы он всегда был точно на клетке, даже если размер клеток изменится при ресайзе окна
    if (m_currentWayPoint > 0 && m_currentWayPoint < m_path.size()) {
        //берем предыдущую точку и целевую
        glm::ivec2 prevCell = m_path[m_currentWayPoint - 1];
        glm::ivec2 targetCell = m_path[m_currentWayPoint];

		// Получаем их пиксельные координаты в старой сетке
        glm::vec2 oldPrevPixel = oldGrid.gridToPixel(prevCell.x, prevCell.y);
        glm::vec2 oldTargetPixel = oldGrid.gridToPixel(targetCell.x, targetCell.y);

        // Считаем общий путь (в пикселях) отрезка и сколько из него враг уже прошёл 
        float oldTotalDist = glm::distance(oldTargetPixel, oldPrevPixel);
        float oldPassedDist = glm::distance(m_pixelPos, oldPrevPixel);

        // Фактор прогресса (от 0.0 до 1.0). Сколько процентов пути пройдено?
        float progress = (oldTotalDist > 0.0f) ? (oldPassedDist / oldTotalDist) : 0.0f;

        // Новые пиксельные позиции на ПОСЛЕ-ресайзовой сетке
        glm::vec2 newPrevPixel = newGrid.gridToPixel(prevCell.x, prevCell.y);
        glm::vec2 newTargetPixel = newGrid.gridToPixel(targetCell.x, targetCell.y);

        // Находим новую пиксельную позицию врага, сохраняя тот же самый процент прогресса!
        m_pixelPos = glm::mix(newPrevPixel, newTargetPixel, progress);
    }
    else if (m_currentWayPoint == 0 && !m_path.empty()) {
        m_pixelPos = newGrid.gridToPixel(m_path[0].x, m_path[0].y);
    }
}

// функция которая дает хитбокс зависимо от размера окна
CircleCollider Enemy::getCollider(const Grid& grid) const {
    float halfCell = grid.getCellSize() / 2.0f;
    glm::vec2 center = m_pixelPos + glm::vec2(halfCell, halfCell);

    // При падении в бездну хитбокс схлопывается
    if (m_isFalling) {
        return { center, 0.0f };
    }

    // умножаем коэффициент на текущий размер клетки из сетки
    float currentRadius = m_radiusMultiplier * grid.getCellSize();

    // возвращаем готовую структуру центр врага и его динамический радиус
    return { center, currentRadius };
}
bool Enemy::isPathIntersecting(glm::ivec2 cell) const {
    if (m_path.empty()) return false;

    // Проверяем диагональный срез угла на текущем сегменте (от предыдущей клетки к целевой)
    if (m_currentWayPoint > 0 && m_currentWayPoint < m_path.size()) {
        glm::ivec2 prev = m_path[m_currentWayPoint - 1];
        glm::ivec2 next = m_path[m_currentWayPoint];
        int dx = next.x - prev.x;
        int dy = next.y - prev.y;
        if (std::abs(dx) == 1 && std::abs(dy) == 1) {
            if ((cell.x == prev.x + dx && cell.y == prev.y) ||
                (cell.x == prev.x && cell.y == prev.y + dy)) {
                return true;
            }
        }
    }

    for (size_t i = m_currentWayPoint; i < m_path.size(); ++i) {
        if (m_path[i] == cell) {
            return true;
        }
        // Проверяем диагональный срез угла между точками i и i+1
        if (i + 1 < m_path.size()) {
            int dx = m_path[i + 1].x - m_path[i].x;
            int dy = m_path[i + 1].y - m_path[i].y;
            if (std::abs(dx) == 1 && std::abs(dy) == 1) {
                if ((cell.x == m_path[i].x + dx && cell.y == m_path[i].y) ||
                    (cell.x == m_path[i].x && cell.y == m_path[i].y + dy)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// функция для нахождения нового пути по предрасчитанной карте расстояний DijkstraMap
void Enemy::recalculatePath(const DijkstraMap& dmap, const Pathfinder& pathfinder, const Grid& grid, const std::vector<BaseData>& bases) {
    if (bases.empty()) return;

    // Определяем текущее направление движения для сохранения прямой линии при трассировке
    glm::ivec2 preferredDir(0, 0);
    if (!m_path.empty()) {
        glm::ivec2 delta(0, 0);
        if (m_currentWayPoint > 0 && m_currentWayPoint < m_path.size()) {
            delta = m_path[m_currentWayPoint] - m_path[m_currentWayPoint - 1];
        } else if (m_path.size() > 1) {
            delta = m_path[1] - m_path[0];
        }

        if (delta.x > 0) {
            preferredDir.x = 1;
        } else if (delta.x < 0) {
            preferredDir.x = -1;
        }

        if (delta.y > 0) {
            preferredDir.y = 1;
        } else if (delta.y < 0) {
            preferredDir.y = -1;
        }
    }

    // Определяем стартовую клетку по центру врага с безопасным откатом к доступной клетке
    glm::vec2 enemyCenter = m_pixelPos + glm::vec2(grid.getCellSize() * 0.5f);
    glm::ivec2 startCell = grid.pixelToGrid(enemyCenter);
    if (!dmap.isReachable(startCell.x, startCell.y)) {
        glm::ivec2 fallbackCell = grid.pixelToGrid(m_pixelPos);
        if (dmap.isReachable(fallbackCell.x, fallbackCell.y)) {
            startCell = fallbackCell;
        } else if (m_currentWayPoint > 0 && m_currentWayPoint <= m_path.size()) {
            glm::ivec2 prevCell = m_path[m_currentWayPoint - 1];
            if (dmap.isReachable(prevCell.x, prevCell.y)) {
                startCell = prevCell;
            }
        }
    }

    if (!dmap.isReachable(startCell.x, startCell.y)) {
        // Тупик: пути нет. Очищаем маршрут, чтобы моб не шел сквозь башни!
        m_path.clear();
        m_currentWayPoint = 0;
        return;
    }

    std::vector<glm::ivec2> tracedPath = pathfinder.tracePath(dmap, grid, startCell, preferredDir);
    if (tracedPath.empty() && dmap.getDistance(startCell.x, startCell.y) > 0) {
        m_path.clear();
        m_currentWayPoint = 0;
        return;
    }

    std::vector<glm::ivec2> newPath;
    newPath.reserve(tracedPath.size() + 1);
    newPath.push_back(startCell);
    newPath.insert(newPath.end(), tracedPath.begin(), tracedPath.end());

    m_path = std::move(newPath);
    m_currentWayPoint = 0;

    // Если враг уже находится в центре стартовой клетки или прошёл её в сторону следующей точки —
    // не заставляем его возвращаться назад в центр m_path[0], сразу направляем к m_path[1].
    if (m_path.size() > 1) {
        glm::vec2 startPixel = grid.gridToPixel(m_path[0].x, m_path[0].y);
        glm::vec2 nextPixel = grid.gridToPixel(m_path[1].x, m_path[1].y);
        glm::vec2 toNext = nextPixel - startPixel;
        glm::vec2 fromStart = m_pixelPos - startPixel;
        float distToStartSq = glm::dot(fromStart, fromStart);
        float progressAlongSeg = glm::dot(fromStart, toNext);

        if (distToStartSq <= 1.0f || progressAlongSeg > 0.0f) {
            m_currentWayPoint = 1;
        }
    }

    glm::ivec2 endPos = m_path.back();
    for (const auto& b : bases) {
        if (b.x == endPos.x && b.y == endPos.y) {
            m_enableFadeOut = b.enableFadeOut;
            break;
        }
    }
}

// функция для нахождения нового пути (обёртка обратной совместимости)
void Enemy::recalculatePath(Pathfinder* pathfinder, const Grid& grid, const std::vector<BaseData>& bases) {
    if (!pathfinder || bases.empty()) return;

    std::vector<glm::ivec2> targetPositions;
    if (m_targetBaseIndex >= 0) {
        for (const auto& b : bases) {
            if (b.id == m_targetBaseIndex) {
                targetPositions.push_back(glm::ivec2(b.x, b.y));
            }
        }
    }
    // Fallback: если targetBaseIndex == -1 или базы с таким id нет на карте — идем к ближайшей среди ВСЕХ баз
    if (targetPositions.empty()) {
        for (const auto& b : bases) {
            targetPositions.push_back(glm::ivec2(b.x, b.y));
        }
    }

    DijkstraMap dmap = pathfinder->generateDijkstraMap(grid, targetPositions);
    recalculatePath(dmap, *pathfinder, grid, bases);
}

void Enemy::applySlow(float duration, float slowPercent) {
    for (auto& effect : m_statusEffects) {
        if (effect.type == StatusType::Slow) {
            if (slowPercent >= effect.intensity) {
                effect.intensity = slowPercent;
                effect.duration = std::max(effect.duration, duration);
            }
            return;
        }
    }
    m_statusEffects.push_back({ StatusType::Slow, duration, slowPercent, 0.0f, 0.0f, 0 });
}

void Enemy::applyPoison(float duration, float tickInterval, int damagePerTick) {
    if (m_mercuryImmune) {
        return;
    }
    for (auto& effect : m_statusEffects) {
        if (effect.type == StatusType::Poison) {
            effect.duration = std::max(effect.duration, duration);
            if (damagePerTick > effect.damagePerTick) {
                effect.damagePerTick = damagePerTick;
                effect.tickInterval = tickInterval;
            }
            return;
        }
    }
    m_statusEffects.push_back({ StatusType::Poison, duration, 0.0f, tickInterval, 0.0f, damagePerTick });
}

void Enemy::applyBleed(float duration, float tickInterval, int damagePerTick) {
    for (auto& effect : m_statusEffects) {
        if (effect.type == StatusType::Bleed) {
            effect.duration = std::max(effect.duration, duration);
            if (damagePerTick > effect.damagePerTick) {
                effect.damagePerTick = damagePerTick;
                effect.tickInterval = tickInterval;
            }
            return;
        }
    }
    m_statusEffects.push_back({ StatusType::Bleed, duration, 0.0f, tickInterval, 0.0f, damagePerTick });
}

void Enemy::applyVulnerable(float duration, float multiplier) {
    for (auto& effect : m_statusEffects) {
        if (effect.type == StatusType::Vulnerable) {
            effect.duration = std::max(effect.duration, duration);
            effect.intensity = std::max(effect.intensity, multiplier);
            return;
        }
    }
    m_statusEffects.push_back({ StatusType::Vulnerable, duration, multiplier, 0.0f, 0.0f, 0 });
}

bool Enemy::isSlowed() const {
    for (const auto& e : m_statusEffects) {
        if (e.type == StatusType::Slow && e.duration > 0.0f) return true;
    }
    return false;
}

bool Enemy::isPoisoned() const {
    for (const auto& e : m_statusEffects) {
        if (e.type == StatusType::Poison && e.duration > 0.0f) return true;
    }
    return false;
}

bool Enemy::isBleeding() const {
    for (const auto& e : m_statusEffects) {
        if (e.type == StatusType::Bleed && e.duration > 0.0f) return true;
    }
    return false;
}

bool Enemy::isVulnerable() const {
    for (const auto& e : m_statusEffects) {
        if (e.type == StatusType::Vulnerable && e.duration > 0.0f) return true;
    }
    return false;
}

float Enemy::getDamageMultiplier() const {
    float mult = 1.0f;
    for (const auto& e : m_statusEffects) {
        if (e.type == StatusType::Vulnerable && e.duration > 0.0f) {
            if (e.intensity > mult) mult = e.intensity;
        }
    }
    return mult;
}

void Enemy::addPistonCooldown(glm::ivec2 pistonCell, float duration) {
    for (auto& entry : m_pistonCooldowns) {
        if (entry.first == pistonCell) {
            entry.second = std::max(entry.second, duration);
            return;
        }
    }
    m_pistonCooldowns.push_back({ pistonCell, duration });
}

bool Enemy::isImmuneToPiston(glm::ivec2 pistonCell) const {
    for (const auto& entry : m_pistonCooldowns) {
        if (entry.first == pistonCell && entry.second > 0.0f) {
            return true;
        }
    }
    return false;
}

void Enemy::pushOneCell(glm::ivec2 fromCell, glm::ivec2 punchDir, const Grid& grid, ParticleSystem* particleSystem, float pushDistance) {
    if (m_reachedEnd || m_isFalling || isDead()) return;

    glm::ivec2 destCell = fromCell + punchDir;

    // Проверяем, свободна ли клетка назначения (проходимая клетка ИЛИ клетка обрыва/шурфа)
    bool isChasm = false;
    if (destCell.x >= 0 && destCell.x < grid.getWidth() && destCell.y >= 0 && destCell.y < grid.getHeight()) {
        isChasm = (grid.getCellType(destCell.x, destCell.y) == CellType::Chasm);
    }
    bool canPush = Pathfinder::isCellWalkable(grid, destCell.x, destCell.y) || isChasm;

    if (!canPush) {
        // Удар о препятствие (стена, башня, декорация): враг остаётся на месте, оглушается на 0.8с, 0 урона!
        m_stunTimer = 0.8f;
        m_isKnockedBack = false;
        m_wallImpactTimer = 0.12f;
        m_wallImpactOffset = glm::vec2(punchDir) * (grid.getCellSize() * 0.20f);

        // Запуск спавна частиц удара SparkImpact в точке контакта
        if (particleSystem) {
            ParticleEmitterProps impactProps = ConfigManager::getParticleProps("SparkImpact");
            glm::vec2 contactPoint = m_pixelPos + glm::vec2(grid.getCellSize() * 0.5f) + glm::vec2(punchDir) * (grid.getCellSize() * 0.5f);
            impactProps.position = contactPoint;
            impactProps.velocityDir = -glm::vec2(punchDir) * 120.0f;
            particleSystem->emit(impactProps, impactProps.spawnCount);
        }
        return;
    }

    // Если толчок больше 1 клетки, проверяем следующую клетку за destCell, чтобы не выбивать в стену
    if (pushDistance > 1.0f) {
        glm::ivec2 nextCell = destCell + punchDir;
        bool nextWalkable = (nextCell.x >= 0 && nextCell.x < grid.getWidth() && nextCell.y >= 0 && nextCell.y < grid.getHeight()) &&
                            (Pathfinder::isCellWalkable(grid, nextCell.x, nextCell.y) || grid.getCellType(nextCell.x, nextCell.y) == CellType::Chasm);
        if (!nextWalkable) {
            pushDistance = 1.0f; // Ограничиваем дистанцию, чтобы не клипаться сквозь препятствие
        }
    }

    glm::vec2 pushDirVec = glm::vec2(punchDir);
    float cellSize = grid.getCellSize();
    float pushPixels = cellSize * pushDistance;

    // Микро-разброс (расклеивание пачки): перпендикулярный тангенциальный разброс (scatter jitter)
    float spreadScale = (pushDistance < 0.5f) ? 0.35f : 1.0f;
    float randomSpread = ((rand() % 100) / 100.0f - 0.5f) * (cellSize * 0.25f) * spreadScale;
    glm::vec2 perpDir = glm::vec2(-pushDirVec.y, pushDirVec.x);
    glm::vec2 individualOffset = pushDirVec * pushPixels + perpDir * randomSpread;

    glm::vec2 targetPos = m_pixelPos + individualOffset;
    glm::ivec2 targetCell = grid.pixelToGrid(targetPos + glm::vec2(cellSize * 0.5f));

    // Проверяем, не выбивает ли перпендикулярный разброс в непроходимое препятствие
    if (targetCell.x < 0 || targetCell.x >= grid.getWidth() || targetCell.y < 0 || targetCell.y >= grid.getHeight() ||
        (!Pathfinder::isCellWalkable(grid, targetCell.x, targetCell.y) && grid.getCellType(targetCell.x, targetCell.y) != CellType::Chasm)) {
        // Откатываем перпендикулярный разброс, оставляя чистый вектор толчка
        individualOffset = pushDirVec * pushPixels;
        targetPos = m_pixelPos + individualOffset;
        targetCell = grid.pixelToGrid(targetPos + glm::vec2(cellSize * 0.5f));
    }

    if (targetCell.x >= 0 && targetCell.x < grid.getWidth() && targetCell.y >= 0 && targetCell.y < grid.getHeight()) {
        if (grid.getCellType(targetCell.x, targetCell.y) == CellType::Chasm) {
            // Если приземляется в шурф — гарантируем целевую клетку шурфа
            destCell = targetCell;
        }
    }

    m_isKnockedBack = true;
    m_knockbackStartPos = m_pixelPos;
    m_knockbackTargetPos = targetPos;
    m_knockbackTimer = 0.0f;
    m_knockbackFromCell = fromCell;
    m_knockbackDuration = (pushDistance < 0.5f) ? 0.12f : 0.20f;
    m_knockbackDestCell = (pushDistance < 0.5f) ? fromCell : targetCell;
}

void Enemy::startFalling(glm::ivec2 chasmCell, const Grid& grid) {
    m_isFalling = true;
    m_fallTimer = 0.0f;
    m_isKnockedBack = false;
    m_stunTimer = 0.0f;
    m_wallImpactTimer = 0.0f;
    m_wallImpactOffset = glm::vec2(0.0f);
    m_statusEffects.clear();
    m_path.clear();
    m_currentWayPoint = 0;
    m_pixelPos = grid.gridToPixel(chasmCell.x, chasmCell.y);
}

void Enemy::applyPostKnockbackPath(glm::ivec2 destCell, glm::ivec2 fromCell, const Grid& grid) {
    if (m_path.empty()) return;

    // 1. Проверяем, есть ли destCell в существующем маршруте m_path
    int destIndex = -1;
    for (int i = 0; i < static_cast<int>(m_path.size()); ++i) {
        if (m_path[i] == destCell) {
            destIndex = i;
            break;
        }
    }

    if (destIndex != -1) {
        // Клетка назначения уже есть на маршруте (например, враг отброшен назад по тропе).
        // Следующей целью становится следующая контрольная точка
        if (destIndex + 1 < static_cast<int>(m_path.size())) {
            m_currentWayPoint = destIndex + 1;
        }
        else {
            m_currentWayPoint = destIndex;
        }
    }
    else {
        // Клетка была выбита вбок от тропы.
        // Ищем индекс fromCell в маршруте
        int fromIndex = -1;
        for (int i = 0; i < static_cast<int>(m_path.size()); ++i) {
            if (m_path[i] == fromCell) {
                fromIndex = i;
                break;
            }
        }

        std::vector<glm::ivec2> newPath;
        newPath.push_back(destCell); // Текущее положение врага после толчка

        size_t resumeIdx = (fromIndex != -1) ? static_cast<size_t>(fromIndex) : 0;
        // Проверяем: если fromIndex + 1 существует, проходима и смежна с destCell — срезаем сразу на неё
        if (fromIndex != -1 && fromIndex + 1 < static_cast<int>(m_path.size())) {
            glm::ivec2 nextP = m_path[fromIndex + 1];
            int distManhattan = std::abs(destCell.x - nextP.x) + std::abs(destCell.y - nextP.y);
            if (distManhattan == 1 && Pathfinder::isCellWalkable(grid, nextP.x, nextP.y)) {
                resumeIdx = static_cast<size_t>(fromIndex + 1);
            }
            else {
                // Ищем ближайшую проходимую точку вперед по маршруту исходя из новой фактической позиции врага
                float bestDistSq = 1e9f;
                size_t bestIdx = static_cast<size_t>(fromIndex);
                glm::vec2 enemyCenter = m_pixelPos + glm::vec2(grid.getCellSize() * 0.5f);
                for (size_t i = static_cast<size_t>(fromIndex); i < m_path.size(); ++i) {
                    if (!Pathfinder::isCellWalkable(grid, m_path[i].x, m_path[i].y)) continue;
                    glm::vec2 wpCenter = grid.gridToPixel(m_path[i].x, m_path[i].y) + glm::vec2(grid.getCellSize() * 0.5f);
                    glm::vec2 diff = enemyCenter - wpCenter;
                    float dSq = glm::dot(diff, diff);
                    if (dSq < bestDistSq) {
                        bestDistSq = dSq;
                        bestIdx = i;
                    }
                }
                resumeIdx = bestIdx;
            }
        }
        else if (fromIndex == -1) {
            // Если fromCell не найден, ищем ближайшую проходимую точку маршрута исходя из фактической позиции
            float bestDistSq = 1e9f;
            size_t bestIdx = 0;
            glm::vec2 enemyCenter = m_pixelPos + glm::vec2(grid.getCellSize() * 0.5f);
            for (size_t i = 0; i < m_path.size(); ++i) {
                if (!Pathfinder::isCellWalkable(grid, m_path[i].x, m_path[i].y)) continue;
                glm::vec2 wpCenter = grid.gridToPixel(m_path[i].x, m_path[i].y) + glm::vec2(grid.getCellSize() * 0.5f);
                glm::vec2 diff = enemyCenter - wpCenter;
                float dSq = glm::dot(diff, diff);
                if (dSq < bestDistSq) {
                    bestDistSq = dSq;
                    bestIdx = i;
                }
            }
            resumeIdx = bestIdx;
        }

        for (size_t i = resumeIdx; i < m_path.size(); ++i) {
            newPath.push_back(m_path[i]);
        }

        m_path = newPath;
        if (m_path.size() > 1) {
            m_currentWayPoint = 1; // Идем от destCell к следующей точке
        }
        else {
            m_currentWayPoint = 0;
        }
    }

    // Строгая валидация индекса текущей точки во избежание зависания
    if (m_currentWayPoint >= m_path.size()) {
        m_currentWayPoint = m_path.empty() ? 0 : m_path.size() - 1;
    }

    // Пересчитываем пройденную дистанцию для корректного таргетинга башен
    m_distanceTraveled = 0.0f;
    for (size_t i = 1; i <= m_currentWayPoint && i < m_path.size(); ++i) {
        m_distanceTraveled += glm::distance(
            grid.gridToPixel(m_path[i].x, m_path[i].y),
            grid.gridToPixel(m_path[i - 1].x, m_path[i - 1].y)
        );
    }
}

void Enemy::applyKnockback(glm::vec2 direction, float force) {
    // Сохранено для обратной совместимости
}