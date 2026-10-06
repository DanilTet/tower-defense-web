#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include "CircleCollider.h"
#include "Enemy.h"

class SpriteRenderer;
class Texture2D;
class Grid;
class ParticleSystem;

class Projectile {
public:
	Projectile(); // конструктор прям дефолтнейший

	// метод инициализации пули при выстреле
	void init(glm::vec2 startPos, float startAngle, float speed, int damage, int targetId, float splashRadius, float searchRadius);

	//метод для записи ефектов из башни берет
	void setParticleEffects(const std::string& trail, const std::string& impact) {
		m_trailParticle = trail;
		m_impactParticle = impact;
	}

	void setVisuals(const std::string& textureId, float baseSize) {
		m_textureId = textureId;
		m_baseSize = baseSize;
	}

	void setStatusEffects(float slowDuration, float slowPercent, float poisonDuration, float poisonInterval, int poisonDamagePerTick) {
		m_slowDuration = slowDuration;
		m_slowPercent = slowPercent;
		m_poisonDuration = poisonDuration;
		m_poisonInterval = poisonInterval;
		m_poisonDamagePerTick = poisonDamagePerTick;
	}

	// двигаем пулу и проверяем столкновение
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid, ParticleSystem& particleSystem);
	// отрисовка
	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> mainAtlas, const Grid& grid);
	// жива ли еще пуля
	bool isDestroyed() const {
		return m_destroyed;
	}

	bool isActive() const { return m_isActive; } // проверяет активна ли пуля
	void setActive(bool active) { m_isActive = active; } // устанавливает состояние пули

	void recalculatePosition(const Grid& oldGrid, const Grid& newGrid);

private:
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

	// ефекты
	std::string m_trailParticle;
	std::string m_impactParticle;
	//пуля
	std::string m_textureId;
	float m_baseSize;

	// статус-эффекты
	float m_slowDuration = 0.0f;
	float m_slowPercent = 0.0f;
	float m_poisonDuration = 0.0f;
	float m_poisonInterval = 0.5f;
	int m_poisonDamagePerTick = 0;
};