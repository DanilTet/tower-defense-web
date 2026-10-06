#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include "CircleCollider.h"
#include <string>
#include "../core/Animator.h"
#include "../core/LevelManager.h"

// Структура для хранения характеристик врага
struct EnemyStats {
	float speed;
	int Maxhealth;
	float sizeScale;
	int reward;
	std::string textureId;
	glm::vec3 color;
	std::string deathSound;
	std::string deathParticle;

	int atlasWidth;
	int atlasHeight;
	std::unordered_map<std::string, AnimationClip> animations;
};

enum class StatusType {
	Slow,
	Poison,
	Stun
};

struct StatusEffect {
	StatusType type;
	float duration;        // оставшееся время действия
	float intensity;       // сила (для Slow: 0.35f = 35% замедления)
	float tickInterval;    // интервал тиков урона (для Poison)
	float tickTimer = 0.0f;// таймер текущего тика
	int damagePerTick = 0; // урон за тик
};

class SpriteRenderer;
class Texture2D;
class Grid;
class Pathfinder;
class ParticleSystem;

class Enemy
{
private:
	std::string m_type; // тип характеристик врага
	int m_health; // здоровье врага
	float m_speed; // скорость врага
	int m_reward; // награда за убийство врага
	glm::vec3 m_color;
	std::string m_deathSound; // звук смерти
	std::string m_deathParticle; // кеш для звука смерти

	Animator m_animator; // компонент аниматора

	std::vector<glm::ivec2> m_path; // маршрут врага
	size_t m_currentWayPoint; // текущая точка к которой враг идет
	glm::vec2 m_pixelPos; // координаты врага в пикселях экрнана
	bool m_reachedEnd; // флаг дошел ли враг до конца

	float m_radiusMultiplier; // соотношение врага отностительно клетки

	static int s_nextId; // счетчик врагов общий
	int m_id; // личный номер врага

	float m_distanceTraveled;// пройденная дистанция врага

	int m_targetBaseIndex; //индекс базы

	float m_angle = 0.0f; // угол поворота
	std::vector<StatusEffect> m_statusEffects; // активные статус-эффекты
	float m_speedModifier = 1.0f; // текущий множитель скорости

	// Физика отталкивания поршнем на 1 клетку
	bool m_isKnockedBack = false;
	float m_knockbackTimer = 0.0f;
	float m_knockbackDuration = 0.20f;
	glm::vec2 m_knockbackStartPos = glm::vec2(0.0f);
	glm::vec2 m_knockbackTargetPos = glm::vec2(0.0f);
	glm::ivec2 m_knockbackDestCell = glm::ivec2(0);
	glm::ivec2 m_knockbackFromCell = glm::ivec2(0);
	float m_stunTimer = 0.0f; // таймер оглушения
	float m_wallImpactTimer = 0.0f; // таймер отдачи при ударе о стену
	glm::vec2 m_wallImpactOffset = glm::vec2(0.0f); // смещение спрайта при ударе о стену
	float m_stunAnimAngle = 0.0f; // угол вращения звездочек стана
	std::vector<std::pair<glm::ivec2, float>> m_pistonCooldowns; // кулдауны конкретных поршней для защиты от локального зацикливания

	// Состояние падения в шурф / обрыв (CellType::Chasm)
	bool m_isFalling = false;
	float m_fallTimer = 0.0f;
	const float FALL_DURATION = 0.45f;

	void applyPostKnockbackPath(glm::ivec2 destCell, glm::ivec2 fromCell, const Grid& grid);

public:

	// функци для перерасчета пути
	void recalculatePath(Pathfinder* pathfinder, const Grid& grid, const std::vector<BaseData>& bases);
	bool isPathIntersecting(glm::ivec2 cell) const;

	glm::ivec2 getTargetBase() const { return m_path.empty() ? glm::ivec2(0) : m_path.back(); }
	int getTargetBaseIndex() const { return m_targetBaseIndex; }

	// Функция для получения характеристик врага в зависимости от его типа
	static EnemyStats getStatsfromEnemyType(const std::string& type);

	void recalculatePosition(const Grid& oldGrid, const Grid& newGrid); // вызывается при изменении размера окна, чтобы враг всегда был точно на клетке, даже если размер клеток изменится при ресайзе окна

	Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, const std::string& type, int targetBaseIndex);
	void update(float dt, const Grid& grid);

	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, std::shared_ptr<Texture2D> radiusTex, glm::vec2 offset, const Grid& grid);
	glm::vec2 getPixelPos() const { return m_pixelPos; }
	bool isReachedEnd() const { return m_reachedEnd; }
	void takeDamage(int damage) {
		if (m_isFalling) return;
		m_health -= damage;
	}

	bool isDead() const { // возвращает true если враг имеет мельше 0 хп и false если больше 
		if (m_health <= 0) {
			return true;
		}
		return false;
	}

	int getReward() const { return m_reward; } // геттер который возгращает награду за убийство врага
	std::string getDeathSound() const { return m_deathSound; }
	CircleCollider getCollider(const Grid& grid) const; // генераттор хитбокса
	std::string getDeathParticle() const { return m_deathParticle; } // геттер партиклов
	int getId() const { return m_id; } // геттер для получения айди врага

	float getDistanceTraveled() const { return m_distanceTraveled; } // геттер для того щоб отримати пройдений шлях ворога

	int getHealth() const { return m_health; } // получить хп

	// Статус-эффекты
	void applySlow(float duration, float slowPercent);
	void applyPoison(float duration, float tickInterval, int damagePerTick);
	bool isSlowed() const;
	bool isPoisoned() const;
	float getSpeedModifier() const { return m_speedModifier; }
	bool isKnockedBack() const { return m_isKnockedBack; }
	bool isStunned() const { return m_stunTimer > 0.0f; }
	bool isFalling() const { return m_isFalling; }
	bool isKnockbackImmune() const { return false; } // глобального иммунитета больше нет
	void addPistonCooldown(glm::ivec2 pistonCell, float duration = 5.0f);
	bool isImmuneToPiston(glm::ivec2 pistonCell) const;
	glm::vec2 getWallImpactOffset() const { return m_wallImpactOffset; }
	void pushOneCell(glm::ivec2 fromCell, glm::ivec2 punchDir, const Grid& grid, ParticleSystem* particleSystem = nullptr);
	void startFalling(glm::ivec2 chasmCell, const Grid& grid);
	void applyKnockback(glm::vec2 direction, float force);
};