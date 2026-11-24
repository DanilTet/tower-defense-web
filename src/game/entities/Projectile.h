#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include "CircleCollider.h"
#include "Enemy.h"
#include "../../particles/ParticleSystem.h"
#include "../../renderer/SpriteRenderer.h"

class SpriteRenderer;
class Texture2D;
class Grid;
class ParticleSystem;

class Projectile {
public:
	Projectile(); // конструктор прям дефолтнейший

	// метод инициализации пули при выстреле
	void init(glm::vec2 startPos, float startAngle, float speed, int damage, int targetId, float splashRadius, float searchRadius);

	//метод для записи эффектов из башни (кешированные структуры без строковых аллокаций)
	void setParticleEffects(bool hasTrail, const ParticleEmitterProps& trailProps, bool hasImpact, const ParticleEmitterProps& impactProps) {
		m_hasTrailParticle = hasTrail;
		m_trailProps = trailProps;
		m_hasImpactParticle = hasImpact;
		m_impactProps = impactProps;
	}

	void setVisuals(const SpriteUV& uv, float baseSize) {
		m_cachedUV = uv;
		m_baseSize = baseSize;
	}

	void setStatusEffects(float slowDuration, float slowPercent, float poisonDuration, float poisonInterval, int poisonDamagePerTick) {
		m_slowDuration = slowDuration;
		m_slowPercent = slowPercent;
		m_poisonDuration = poisonDuration;
		m_poisonInterval = poisonInterval;
		m_poisonDamagePerTick = poisonDamagePerTick;
	}

	void setMercuryDamage(bool isMercury) {
		m_isMercuryDamage = isMercury;
	}

	// двигаем пулу и проверяем столкновение
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid, ParticleSystem& particleSystem, Enemy* resolvedTarget = nullptr, bool hasResolvedTarget = false, float timeScale = 1.0f);
	// отрисовка
	void render(SpriteRenderer* renderer, const Texture2D* mainAtlas, const Grid& grid);
	// жива ли еще пуля
	bool isDestroyed() const {
		return m_destroyed;
	}

	int getTargetId() const { return m_targetId; }
	bool isActive() const { return m_isActive; } // проверяет активна ли пуля
	void setActive(bool active) { m_isActive = active; } // устанавливает состояние пули

	void recalculatePosition(const Grid& oldGrid, const Grid& newGrid);

private:
	bool tryHitEnemy(Enemy* enemy, const glm::vec2& prevPos, const glm::vec2& travelVec, float travelDistSq, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid, ParticleSystem& particleSystem);
	glm::vec2 m_pos; // текущая позиция пули
	glm::vec2 m_direction; // нормализированій вектор направления

	float m_angle; // текущий угол пули
	float m_turnSpeed; // скорость поворота пули градусы в секунду

	glm::vec2 m_originPoint; // Точка старта
	float m_searchRadius; // радиус атаки башни


	float m_speed; // скорость пиксели в секунду
	int m_damage; // урон
	float m_radius; // размер хитбокса пули

	bool m_destroyed; // флаг на удаление
	float m_lifeTime; // таймер самоуничтожения

	int m_targetId; // айди врага к которому летит

	float m_splashRadius; // радиус сплеш урона

	// флаш режима столкновений
	bool m_hitOnlyTarget = true; // true = Аркада false = Реализм

	bool m_isActive; // флаг для пула обьектов

	// эффекты (кэшированные POD-структуры)
	ParticleEmitterProps m_trailProps{};
	ParticleEmitterProps m_impactProps{};
	bool m_hasTrailParticle = false;
	bool m_hasImpactParticle = false;
	float m_trailTimer = 0.0f;
	// пуля
	SpriteUV m_cachedUV;
	float m_baseSize = 12.0f;

	// статус-эффекты
	float m_slowDuration = 0.0f;
	float m_slowPercent = 0.0f;
	float m_poisonDuration = 0.0f;
	float m_poisonInterval = 0.5f;
	int m_poisonDamagePerTick = 0;
	bool m_isMercuryDamage = false;
};