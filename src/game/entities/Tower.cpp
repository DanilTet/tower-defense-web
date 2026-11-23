#include "Tower.h"
#include "world/Grid.h"
#include "Enemy.h"
#include "renderer/SpriteRenderer.h"
#include "Projectile.h"
#include "core/ConfigManager.h"
#include "../core/EventBus.h"
#include "../../particles/ParticleSystem.h"
#include "../gameplay/EntityManager.h"
#include "TowerTargetingSystem.h"
#include "../../resources/ResourceManager.h"
#include <cmath>

TowerStats Tower::getStatsfromTowerType(const std::string& type) {
	return ConfigManager::getTowerStats(type);
}

// конструктор
Tower::Tower(int gridX, int gridY, const std::string& type, float angle)
	: m_gridX(gridX),
	m_gridY(gridY),
	m_type(type),
	m_angle(angle),
	m_currentLevel(1), // бам бам с первого уровня
	m_maxLevel(3) // макс левел башни
{
	m_isPiston = (m_type == "Piston");
	m_pistonHitBuffer.reserve(16);

	// считываем статистику
	TowerStats stats = Tower::getStatsfromTowerType(type);
	applyStats(stats);

	m_shotTimer = 0.0f; // переменная таймер
}

void Tower::rotate90() {
	if (m_type == "Piston") {
		if (std::abs(m_angle - 270.0f) < 5.0f || std::abs(m_angle - (-90.0f)) < 5.0f) m_angle = 0.0f;    // Вверх -> Вправо
		else if (std::abs(m_angle - 0.0f) < 5.0f) m_angle = 90.0f;                                         // Вправо -> Вниз
		else if (std::abs(m_angle - 90.0f) < 5.0f) m_angle = 180.0f;                                       // Вниз -> Влево
		else m_angle = 270.0f;                                                                             // Влево -> Вверх
	}
}

glm::ivec2 Tower::getPistonTargetCell() const {
	if (std::abs(m_angle - 270.0f) < 5.0f || std::abs(m_angle - (-90.0f)) < 5.0f) return glm::ivec2(m_gridX, m_gridY - 1); // North (Вверх)
	if (std::abs(m_angle - 0.0f) < 5.0f) return glm::ivec2(m_gridX + 1, m_gridY);                                          // East (Вправо)
	if (std::abs(m_angle - 90.0f) < 5.0f) return glm::ivec2(m_gridX, m_gridY + 1);                                         // South (Вниз)
	return glm::ivec2(m_gridX - 1, m_gridY);                                                                                 // West (Влево)
}

void Tower::render(SpriteRenderer* renderer, const Texture2D* atlasTexture, const Texture2D* radiusTexture, const Texture2D* arrowTexture, const Grid& grid, bool isSelected) {
	float cellSize = grid.getCellSize(); //подтягиваем рязмер чтобы в клетку попала башня
	glm::vec2 size(cellSize, cellSize); // создаем вектор чтобы башня идеально стала в клетку
	glm::vec2 pixelPos = grid.gridToPixel(m_gridX, m_gridY); // получаем пиксели клетки
	glm::vec2 towerCenter = pixelPos + glm::vec2(cellSize / 2.0f);

	// БАШНЯ
	glm::vec3 color = m_color;
	if (m_currentLevel == 2) color += glm::vec3(0.2f, 0.2f, 0.2f);
	if (m_currentLevel == 3) color += glm::vec3(0.4f, 0.4f, 0.4f);

	// Если поршень бьёт — анимируем выдвижение бойка вперед
	glm::vec2 drawPos = pixelPos;
	if (m_isPiston && m_punchAnimTimer > 0.0f) {
		glm::vec2 punchDir = glm::vec2(getPistonTargetCell() - glm::ivec2(m_gridX, m_gridY));
		float progress = 0.0f;
		if (m_punchAnimTimer > 0.10f) {
			progress = (0.20f - m_punchAnimTimer) / 0.10f;
		}
		else {
			progress = m_punchAnimTimer / 0.10f;
		}
		drawPos += punchDir * (cellSize * 0.28f * progress);
	}

	// рисуем (добавляем 90 градусов, чтобы выровнять текстуру дула/бойка с направлением)
	renderer->drawSprite(atlasTexture, drawPos, size, m_angle + 90.0f, color, m_cachedTowerUV);

	// Для T3 Поршня ("Шипованный боёк") отрисовываем стальные шипы на передней кромке ударной плиты
	if (m_isPiston && m_currentLevel >= 3) {
		const Texture2D* whiteTex = m_cachedWhiteTex;
		if (!whiteTex) {
			whiteTex = ResourceManager::getWhiteTexture();
		}
		if (whiteTex) {
			glm::vec2 currentCenter = drawPos + size * 0.5f;
			float angleRad = glm::radians(m_angle);
			glm::vec2 forward = glm::vec2(std::cos(angleRad), std::sin(angleRad));
			glm::vec2 right = glm::vec2(-std::sin(angleRad), std::cos(angleRad));

			// 1. Усилительная стальная планка на ударной плите
			glm::vec2 barCenter = currentCenter + forward * (cellSize * 0.38f);
			glm::vec2 barSize = glm::vec2(cellSize * 0.58f, cellSize * 0.08f);
			renderer->drawSprite(whiteTex, barCenter - barSize * 0.5f, barSize, m_angle + 90.0f, glm::vec3(0.18f, 0.20f, 0.24f));

			glm::vec2 barEdgeCenter = currentCenter + forward * (cellSize * 0.40f);
			glm::vec2 barEdgeSize = glm::vec2(cellSize * 0.54f, cellSize * 0.02f);
			renderer->drawSprite(whiteTex, barEdgeCenter - barEdgeSize * 0.5f, barEdgeSize, m_angle + 90.0f, glm::vec3(0.50f, 0.55f, 0.62f));

			// 2. Ряд из 4 стальных шипов вдоль передней кромки бойка
			float lateralOffsets[4] = { -0.19f * cellSize, -0.065f * cellSize, 0.065f * cellSize, 0.19f * cellSize };
			for (int i = 0; i < 4; ++i) {
				glm::vec2 spikeBase = currentCenter + forward * (cellSize * 0.38f) + right * lateralOffsets[i];

				// Основание-хвостовик шипа
				glm::vec2 shankCenter = spikeBase + forward * (cellSize * 0.035f);
				glm::vec2 shankSize = glm::vec2(cellSize * 0.08f, cellSize * 0.07f);
				renderer->drawSprite(whiteTex, shankCenter - shankSize * 0.5f, shankSize, m_angle + 90.0f, glm::vec3(0.15f, 0.17f, 0.20f));

				// Основное тело клиновидного шипа (темная сталь)
				glm::vec2 spikeCenter = spikeBase + forward * (cellSize * 0.08f);
				glm::vec2 spikeSize = glm::vec2(cellSize * 0.12f, cellSize * 0.12f);
				renderer->drawSprite(whiteTex, spikeCenter - spikeSize * 0.5f, spikeSize, m_angle + 45.0f, glm::vec3(0.26f, 0.29f, 0.34f));

				// Продольная стальная грань / светлый металлический блеск
				glm::vec2 hlCenter = spikeBase + forward * (cellSize * 0.095f);
				glm::vec2 hlSize = glm::vec2(cellSize * 0.07f, cellSize * 0.07f);
				renderer->drawSprite(whiteTex, hlCenter - hlSize * 0.5f, hlSize, m_angle + 45.0f, glm::vec3(0.70f, 0.77f, 0.88f));

				// Яркий точечный блик на самом острие кончика
				glm::vec2 tipCenter = spikeBase + forward * (cellSize * 0.135f);
				glm::vec2 tipSize = glm::vec2(cellSize * 0.032f, cellSize * 0.032f);
				renderer->drawSprite(whiteTex, tipCenter - tipSize * 0.5f, tipSize, m_angle + 45.0f, glm::vec3(0.96f, 0.98f, 1.0f));
			}
		}
	}

	if (isSelected) {
		if (m_type == "Piston") {
			// Для поршня подсвечиваем конкретную клетку удара перед ним
			glm::ivec2 targetCell = getPistonTargetCell();
			if (targetCell.x >= 0 && targetCell.x < grid.getWidth() && targetCell.y >= 0 && targetCell.y < grid.getHeight()) {
				glm::vec2 targetPixelPos = grid.gridToPixel(targetCell.x, targetCell.y);
				renderer->drawSprite(radiusTexture, targetPixelPos, glm::vec2(cellSize), 0.0f, glm::vec3(1.0f, 0.85f, 0.2f));
			}
		} else {
			float currentPixelRange = m_range * cellSize;
			glm::vec2 radiusSize(currentPixelRange * 2.0f, currentPixelRange * 2.0f);
			glm::vec2 radiusPos = towerCenter - glm::vec2(currentPixelRange);
			renderer->drawSprite(radiusTexture, radiusPos, radiusSize, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
		}
	}

	// debug стрелочка
	if (m_showDebugArrow && arrowTexture != nullptr) {
		glm::vec2 arrowSize = size * 0.8f;
		glm::vec2 arrowPos = pixelPos + (size - arrowSize) * 0.5f;
		renderer->drawSprite(arrowTexture, arrowPos, arrowSize, m_angle, glm::vec3(0.5f, 1.0f, 0.5f));
	}
}

void Tower::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, EntityManager& entityManager, const Grid& grid, ParticleSystem& particleSystem) {
	//если башня еще не перезарядилась, перезаряжаем
	if (m_shotTimer > 0.0f) {
		m_shotTimer -= dt;
	}
	if (m_punchAnimTimer > 0.0f) {
		m_punchAnimTimer -= dt;
	}
	
	// считаем центр башни
	float cellSize = grid.getCellSize();
	glm::vec2 towerCenter = grid.gridToPixel(m_gridX, m_gridY) + glm::vec2(cellSize / 2.0f);

	if (m_isPiston) {
		// Поршень не крутится за врагами! Он строго контролирует 1 целевую клетку прямо перед собой
		glm::ivec2 targetCell = getPistonTargetCell();
		if (targetCell.x < 0 || targetCell.x >= grid.getWidth() || targetCell.y < 0 || targetCell.y >= grid.getHeight()) {
			return;
		}

		glm::vec2 targetCenter = grid.gridToPixel(targetCell.x, targetCell.y) + glm::vec2(cellSize * 0.5f);
		float triggerRadius = cellSize * 0.55f;
		float triggerRadiusSq = triggerRadius * triggerRadius;

		if (m_shotTimer <= 0.0f) {
			m_pistonHitBuffer.clear();
			for (const auto& enemy : enemies) {
				if (!enemy || enemy->isDead() || enemy->isReachedEnd() || enemy->isKnockedBack() || enemy->isStunned() || enemy->isFalling()) continue;
				if (enemy->isImmuneToPiston(glm::ivec2(m_gridX, m_gridY))) continue; // Защита от бесконечного зацикливания именно этим поршнем!
				glm::vec2 diff = enemy->getCollider(grid).center - targetCenter;
				float distSq = diff.x * diff.x + diff.y * diff.y;
				if (distSq <= triggerRadiusSq) {
					m_pistonHitBuffer.push_back(enemy.get());
				}
			}

			if (!m_pistonHitBuffer.empty()) {
				m_punchAnimTimer = 0.20f;
				glm::ivec2 punchDir = targetCell - glm::ivec2(m_gridX, m_gridY);

				for (Enemy* victim : m_pistonHitBuffer) {
					float pushDist = m_pistonStats.pushDistance;
					if (m_pistonStats.heavyPushDistance >= 0.0f) {
						const std::string& type = victim->getType();
						for (const std::string& heavyType : m_pistonStats.heavyEnemyTypes) {
							if (type == heavyType) {
								pushDist = m_pistonStats.heavyPushDistance;
								break;
							}
						}
					}

					if (m_pistonStats.impactDamage > 0) {
						victim->takeDamage(m_pistonStats.impactDamage);
					}
					if (m_pistonStats.bleedDuration > 0.0f) {
						victim->applyBleed(m_pistonStats.bleedDuration, m_pistonStats.bleedInterval, m_pistonStats.bleedDamagePerTick);
					}
					if (m_pistonStats.vulnerableDuration > 0.0f) {
						victim->applyVulnerable(m_pistonStats.vulnerableDuration, m_pistonStats.vulnerableMultiplier);
					}

					victim->addPistonCooldown(glm::ivec2(m_gridX, m_gridY), m_pistonStats.immunityCooldown);
					victim->pushOneCell(targetCell, punchDir, grid, &particleSystem, pushDist);
				}

				// Базовые частицы удара бойка в точке удара
				if (m_hasImpactParticle) {
					ParticleEmitterProps impact = m_impactProps;
					impact.position = targetCenter;
					particleSystem.emit(impact, impact.spawnCount, false);
				}

				// Визуальный отклик T3: брызги и искры шипов в точке удара бойка
				if (m_currentLevel >= 3) {
					ParticleEmitterProps sparks = m_pistonSparkProps;
					sparks.position = targetCenter;
					sparks.velocityVariation = 160.0f;
					particleSystem.emit(sparks, sparks.spawnCount, false);

					ParticleEmitterProps blood = m_pistonBloodProps;
					blood.position = targetCenter;
					particleSystem.emit(blood, blood.spawnCount, false);
				}

				Event e;
				e.type = EventType::TowerFired;
				e.textData = m_attackSound;
				EventBus::publish(e);

				m_shotTimer = m_fireRate;
			}
		}
		return;
	}

	// Для остальных башен (Basic, Mercury) — классический поиск цели и плавное наведение
	float currentPixelRange = m_range * cellSize;
	Enemy* bestTarget = TowerTargetingSystem::selectTarget(towerCenter, currentPixelRange, m_targetMode, enemies, grid);

	if (bestTarget != nullptr) {
		glm::vec2 enemyCenter = bestTarget->getCollider(grid).center;
		bool isAimed = TowerTargetingSystem::updateAim(m_angle, towerCenter, enemyCenter, m_rotationSpeed, dt);

		if (isAimed) {
			if (m_shotTimer <= 0.0f) {
				Projectile* freeProj = entityManager.getFreeProjectile();
				if (freeProj) {
					freeProj->setParticleEffects(m_hasTrailParticle, m_trailProps, m_hasImpactParticle, m_impactProps);
					freeProj->setVisuals(m_cachedBulletUV, m_bulletBaseSize);
					freeProj->init(towerCenter, m_angle, m_bulletSpeed, m_damage, bestTarget->getId(), m_splashRadius, currentPixelRange);
					freeProj->setStatusEffects(m_slowDuration, m_slowPercent, m_poisonDuration, m_poisonInterval, m_poisonDamagePerTick);
				}

				if (m_hasMuzzleParticle) {
					ParticleEmitterProps muzzle = m_muzzleProps;
					float angleRad = glm::radians(m_angle);
					glm::vec2 shootDirection = glm::vec2(std::cos(angleRad), std::sin(angleRad));
					muzzle.position = towerCenter + shootDirection * (cellSize * 0.4f);
					float force = glm::length(muzzle.velocityDir);
					muzzle.velocityDir = shootDirection * force;
					particleSystem.emit(muzzle, muzzle.spawnCount, false);
				}

				Event e;
				e.type = EventType::TowerFired;
				e.textData = m_attackSound;
				EventBus::publish(e);

				m_shotTimer = m_fireRate;
			}
		}
	}
}

// функция улучшения башни
bool Tower::upgrade(int& playerMoney) {
	if (m_currentLevel >= m_maxLevel) return false; // достигнут максимум

	// увеличиваем лвл
	int nextLevel = m_currentLevel + 1;
	const TowerStats& nextStats = ConfigManager::getTowerStats(m_type, nextLevel);
	if (nextStats.cost <= 0 && nextStats.damage <= 0 && nextStats.range <= 0.0f) {
		return false; // защита от невалидных статов следующего уровня
	}

	if (playerMoney >= nextStats.cost) {
		playerMoney -= nextStats.cost; // забираем деньги
		m_currentLevel = nextLevel;

		applyStats(nextStats);

		// формируем посылку с кастомным звуком апгрейда
		Event e;
		e.type = EventType::TowerBuilt;
		e.textData = nextStats.buildSound;
		EventBus::publish(e);

		return true;
	}

	return false; // Не хватило денег
}

int Tower::getUpgradeCost() const {
	if (m_currentLevel >= m_maxLevel) return 0;
	return ConfigManager::getTowerStats(m_type, m_currentLevel + 1).cost;
}

void Tower::forceLevel(int level) {
	if (level < 1 || level > m_maxLevel) return;
	m_currentLevel = level;
	const TowerStats& stats = ConfigManager::getTowerStats(m_type, level);// получаем статы для этого типа

	applyStats(stats);
}

void Tower::applyStats(const TowerStats& stats) {
	m_range = stats.range;
	m_damage = stats.damage;
	m_fireRate = stats.fireRate;
	m_rotationSpeed = stats.rotationSpeed;
	m_splashRadius = stats.splashRadius;
	m_attackSound = stats.attackSound;
	m_buildSound = stats.buildSound;
	m_textureId = stats.textureId;
	m_color = stats.color;
	m_muzzleParticle = stats.muzzleParticle;
	m_trailParticle = stats.trailParticle;
	m_impactParticle = stats.impactParticle;
	m_bulletTextureId = stats.bulletTextureId;
	m_bulletBaseSize = stats.bulletBaseSize;
	m_bulletSpeed = stats.bulletSpeed;
	m_slowDuration = stats.slowDuration;
	m_slowPercent = stats.slowPercent;
	m_poisonDuration = stats.poisonDuration;
	m_poisonInterval = stats.poisonInterval;
	m_poisonDamagePerTick = stats.poisonDamagePerTick;
	m_pistonStats = stats.piston;

	// Предвычисление UV и свойств частиц, чтобы не искать их каждый кадр
	std::string towerRegion = "tower_basic";
	if (m_type == "Mercury") {
		towerRegion = "tower_mercury";
	}
	else if (m_isPiston) {
		towerRegion = "tower_piston";
	}
	m_cachedTowerUV = ConfigManager::getUV("main_atlas", towerRegion);

	std::string bulletRegion = m_bulletTextureId;
	if (bulletRegion == "bulletBasic") {
		bulletRegion = "bullet_basic";
	}
	else if (bulletRegion == "bulletSniper") {
		bulletRegion = "bullet_sniper";
	}
	else if (bulletRegion == "bulletCannon") {
		bulletRegion = "bullet_cannon";
	}
	else if (bulletRegion == "bulletMinigun") {
		bulletRegion = "bullet_minigun";
	}
	m_cachedBulletUV = ConfigManager::getUV("main_atlas", bulletRegion);
	m_cachedWhiteTex = ResourceManager::getWhiteTexture();

	m_hasMuzzleParticle = !m_muzzleParticle.empty();
	if (m_hasMuzzleParticle) {
		m_muzzleProps = ConfigManager::getParticleProps(m_muzzleParticle);
	}
	m_hasTrailParticle = !m_trailParticle.empty();
	if (m_hasTrailParticle) {
		m_trailProps = ConfigManager::getParticleProps(m_trailParticle);
	}
	m_hasImpactParticle = !m_impactParticle.empty();
	if (m_hasImpactParticle) {
		m_impactProps = ConfigManager::getParticleProps(m_impactParticle);
	}
	if (m_isPiston) {
		m_pistonSparkProps = ConfigManager::getParticleProps("SparkImpact");
		m_pistonBloodProps = ConfigManager::getParticleProps("BloodSplatter");
	}
}