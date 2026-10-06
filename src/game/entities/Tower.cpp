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

void Tower::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> atlasTexture, std::shared_ptr<Texture2D> radiusTexture, std::shared_ptr<Texture2D> arrowTexture, const Grid& grid, bool isSelected) {
	float cellSize = grid.getCellSize(); //подтягиваем рязмер чтобы в клетку попала башня
	glm::vec2 size(cellSize, cellSize); // создаем вектор чтобы башня идеально стала в клетку
	glm::vec2 pixelPos = grid.gridToPixel(m_gridX, m_gridY); // получаем пиксели клетки
	glm::vec2 towerCenter = pixelPos + glm::vec2(cellSize / 2.0f);

	// БАШНЯ
	glm::vec3 color = m_color;
	if (m_currentLevel == 2) color += glm::vec3(0.2f, 0.2f, 0.2f);
	if (m_currentLevel == 3) color += glm::vec3(0.4f, 0.4f, 0.4f);

	// режем башню
	std::string regionName = "tower_basic";
	if (m_type == "Mercury") regionName = "tower_mercury";
	else if (m_type == "Piston") regionName = "tower_piston";
	SpriteUV towerUV = ConfigManager::getUV("main_atlas", regionName);

	// Если поршень бьёт — анимируем выдвижение бойка вперед
	glm::vec2 drawPos = pixelPos;
	if (m_type == "Piston" && m_punchAnimTimer > 0.0f) {
		glm::vec2 punchDir = glm::vec2(getPistonTargetCell() - glm::ivec2(m_gridX, m_gridY));
		float progress = (m_punchAnimTimer > 0.10f) ? (0.20f - m_punchAnimTimer) / 0.10f : (m_punchAnimTimer / 0.10f);
		drawPos += punchDir * (cellSize * 0.28f * progress);
	}

	// рисуем (добавляем 90 градусов, чтобы выровнять текстуру дула/бойка с направлением)
	renderer->drawSprite(atlasTexture, drawPos, size, m_angle + 90.0f, color, towerUV);

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

	if (m_type == "Piston") {
		// Поршень не крутится за врагами! Он строго контролирует 1 клетку прямо перед собой
		glm::ivec2 targetCell = getPistonTargetCell();
		glm::vec2 targetCenter = grid.gridToPixel(targetCell.x, targetCell.y) + glm::vec2(cellSize * 0.5f);
		float triggerRadius = cellSize * 0.35f; // Уменьшенный радиус, чтобы враг успевал зайти на клетку перед ударом

		if (m_shotTimer <= 0.0f) {
			Enemy* victim = nullptr;
			for (const auto& enemy : enemies) {
				if (!enemy || enemy->isDead() || enemy->isReachedEnd() || enemy->isKnockedBack() || enemy->isStunned() || enemy->isFalling()) continue;
				if (enemy->isImmuneToPiston(glm::ivec2(m_gridX, m_gridY))) continue; // Защита от бесконечного зацикливания именно этим поршнем!
				float dist = glm::distance(enemy->getCollider(grid).center, targetCenter);
				if (dist < triggerRadius) {
					victim = enemy.get();
					break;
				}
			}

			if (victim != nullptr) {
				victim->addPistonCooldown(glm::ivec2(m_gridX, m_gridY), 5.0f);
				m_punchAnimTimer = 0.20f;

				// Поршень НЕ наносит урон, а отталкивает ровно на 1 клетку!
				glm::ivec2 punchDir = targetCell - glm::ivec2(m_gridX, m_gridY);
				victim->pushOneCell(targetCell, punchDir, grid, &particleSystem);

				if (!m_impactParticle.empty()) {
					ParticleEmitterProps impact = ConfigManager::getParticleProps(m_impactParticle);
					impact.position = victim->getCollider(grid).center;
					particleSystem.emit(impact, impact.spawnCount);
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
			float currentSplash = ConfigManager::getTowerStats(m_type, m_currentLevel).splashRadius;

			if (m_shotTimer <= 0.0f) {
				Projectile* freeProj = entityManager.getFreeProjectile();
				if (freeProj) {
					freeProj->setParticleEffects(m_trailParticle, m_impactParticle);
					freeProj->setVisuals(m_bulletTextureId, m_bulletBaseSize);
					freeProj->init(towerCenter, m_angle, m_bulletSpeed, m_damage, bestTarget->getId(), m_splashRadius, currentPixelRange);
					freeProj->setStatusEffects(m_slowDuration, m_slowPercent, m_poisonDuration, m_poisonInterval, m_poisonDamagePerTick);
				}

				if (!m_muzzleParticle.empty()) {
					ParticleEmitterProps muzzle = ConfigManager::getParticleProps(m_muzzleParticle);
					glm::vec2 shootDirection = glm::vec2(cos(glm::radians(m_angle)), sin(glm::radians(m_angle)));
					muzzle.position = towerCenter + shootDirection * (cellSize * 0.4f);
					float force = glm::length(muzzle.velocityDir);
					muzzle.velocityDir = shootDirection * force;
					particleSystem.emit(muzzle, muzzle.spawnCount);
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
	TowerStats nextStats = ConfigManager::getTowerStats(m_type, nextLevel);

	if (playerMoney >= nextStats.cost) {
		playerMoney -= nextStats.cost; // заберяем деняк
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
	TowerStats stats = ConfigManager::getTowerStats(m_type, level);// получаем статы для этого типа

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
}