#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include "CircleCollider.h"
#include <string>
#include "../core/Animator.h"
#include "../core/LevelManager.h"
#include "../../particles/ParticleSystem.h"
#include "../../renderer/SpriteRenderer.h"

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

	// Параметры распада при гибели
	std::string splitChildType = "";
	int splitCount = 0;
	float splitScatterRadius = 14.0f;
};

enum class StatusType {
	Slow,
	Poison,
	Stun,
	Bleed,
	Vulnerable
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
	int m_maxHealth = 1; // максимальное здоровье врага
	float m_speed; // скорость врага
	int m_reward = 1; // награда за убийство врага
	int m_rewardStep = 1; // шаг выплаты награды
	int m_waveIndex = 0; // индекс волны спавна (0-based)
	glm::vec3 m_color;
	std::string m_deathSound; // звук смерти
	std::string m_deathParticle; // кеш для звука смерти
	std::string m_splitChildType = ""; // тип дочернего врага при распаде
	int m_splitCount = 0;              // количество дочерних врагов
	float m_splitScatterRadius = 14.0f;// радиус разлета при распаде

	Animator m_animator; // компонент аниматора

	std::vector<glm::ivec2> m_path; // маршрут врага
	size_t m_currentWayPoint; // текущая точка к которой враг идет
	glm::vec2 m_pixelPos; // координаты врага в пикселях экрнана
	bool m_reachedEnd; // флаг дошел ли враг до конца

	float m_radiusMultiplier; // соотношение врага отностительно клетки
	float m_sizeScale = 0.5f;
	std::string m_textureId;
	const Texture2D* m_cachedTexture = nullptr;
	bool m_isSpark = false;
	bool m_isCoal = false;
	ParticleEmitterProps m_deathParticleProps;
	ParticleEmitterProps m_sparkTrailProps;

	struct RenderGeometry {
		glm::vec2 drawPos;
		glm::vec2 cellCenter;
		glm::vec2 size;
		float baseEnemyDim;
		float currentEnemyDim;
		float drawAngle;
		glm::vec3 renderColor;
		SpriteUV uv;
	};

	static int s_nextId; // счетчик врагов общий
	int m_id; // личный номер врага

	float m_distanceTraveled;// пройденная дистанция врага
	float m_aliveTime = 0.0f; // время жизни инстанса для процедурной анимации и пульсации
	float m_sparkTrailTimer = 0.0f; // таймер интервала спавна шлейфа искр
	float m_visibilityAlpha = 1.0f; // коэффициент видимости тумана (0.0f - 1.0f)
	bool m_enableFadeIn = true;
	bool m_enableFadeOut = true;

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

	void initFromStats(const EnemyStats& stats);
	RenderGeometry computeRenderGeometry(const Grid& grid) const;
	void applyPostKnockbackPath(glm::ivec2 destCell, glm::ivec2 fromCell, const Grid& grid);
	void updateVisibilityAlpha(const Grid& grid);

public:

	// функци для перерасчета пути
	void recalculatePath(Pathfinder* pathfinder, const Grid& grid, const std::vector<BaseData>& bases);
	void recalculatePath(const struct DijkstraMap& dmap, const Pathfinder& pathfinder, const Grid& grid, const std::vector<BaseData>& bases);
	bool isPathIntersecting(glm::ivec2 cell) const;

	glm::ivec2 getTargetBase() const { return m_path.empty() ? glm::ivec2(0) : m_path.back(); }
	int getTargetBaseIndex() const { return m_targetBaseIndex; }

	// Функция для получения характеристик врага в зависимости от его типа
	static EnemyStats getStatsfromEnemyType(const std::string& type);

	void recalculatePosition(const Grid& oldGrid, const Grid& newGrid); // вызывается при изменении размера окна, чтобы враг всегда был точно на клетке, даже если размер клеток изменится при ресайзе окна

	Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, const std::string& type, int targetBaseIndex, bool enableFadeIn = true, bool enableFadeOut = true);
	Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, const std::string& type, int targetBaseIndex, size_t currentWaypoint, glm::vec2 spawnPixelPos, bool enableFadeIn = true, bool enableFadeOut = true);
	void update(float dt, const Grid& grid, ParticleSystem* particleSystem = nullptr, float timeScale = 1.0f, int activeSparkCount = 0);

	void render(SpriteRenderer* renderer, const Texture2D* texture, const Texture2D* radiusTex, glm::vec2 offset, const Grid& grid);
	const Texture2D* getBodyTexture() const;
	void renderBody(SpriteRenderer* renderer, const Grid& grid) const;
	void renderCoalGlow(SpriteRenderer* renderer, const Texture2D* coalGlowTex, const Grid& grid) const;
	void renderStunStars(SpriteRenderer* renderer, const Texture2D* starTex, const Grid& grid) const;
	void renderHealthBar(SpriteRenderer* renderer, const Texture2D* barTex, const Grid& grid) const;
	void renderDebugHitbox(SpriteRenderer* renderer, const Texture2D* radiusTex, const Grid& grid) const;
	glm::vec2 getPixelPos() const { return m_pixelPos; }
	bool isReachedEnd() const { return m_reachedEnd; }
	void takeDamage(int damage) {
		if (m_isFalling) return;
		float mult = getDamageMultiplier();
		int actualDamage = (mult > 1.0f) ? static_cast<int>(std::round(damage * mult)) : damage;
		if (damage > 0 && actualDamage < 1) actualDamage = 1;
		m_health -= actualDamage;
	}

	bool isDead() const { // возвращает true если враг имеет мельше 0 хп и false если больше 
		if (m_health <= 0) {
			return true;
		}
		return false;
	}

	int getReward() const { return m_reward; } // геттер который возгращает награду за убийство врага
	void setReward(int reward) { m_reward = std::max(0, reward); }
	int getRewardStep() const { return m_rewardStep; }
	void setRewardStep(int step) { m_rewardStep = std::max(1, step); }
	int getWaveIndex() const { return m_waveIndex; }
	void setWaveIndex(int wave) { m_waveIndex = wave; }
	const std::string& getDeathSound() const { return m_deathSound; }
	CircleCollider getCollider(const Grid& grid) const; // генераттор хитбокса
	const std::string& getDeathParticle() const { return m_deathParticle; } // геттер партиклов
	const ParticleEmitterProps& getDeathParticleProps() const { return m_deathParticleProps; }
	bool isSpark() const { return m_isSpark; }
	bool isCoal() const { return m_isCoal; }
	int getId() const { return m_id; } // геттер для получения айди врага

	float getDistanceTraveled() const { return m_distanceTraveled; } // геттер для того щоб отримати пройдений шлях ворога
	void setDistanceTraveled(float dist) { m_distanceTraveled = dist; }
	float getVisibilityAlpha() const { return m_visibilityAlpha; }
	void setFadeSettings(bool fadeIn, bool fadeOut) { m_enableFadeIn = fadeIn; m_enableFadeOut = fadeOut; }
	bool getEnableFadeIn() const { return m_enableFadeIn; }
	bool getEnableFadeOut() const { return m_enableFadeOut; }

	const std::string& getType() const { return m_type; }
	const std::vector<glm::ivec2>& getPath() const { return m_path; }
	size_t getCurrentWaypoint() const { return m_currentWayPoint; }
	const std::string& getSplitChildType() const { return m_splitChildType; }
	int getSplitCount() const { return m_splitCount; }
	float getSplitScatterRadius() const { return m_splitScatterRadius; }

	int getHealth() const { return m_health; } // получить хп
	int getMaxHealth() const { return m_maxHealth; }
	void setMaxHealth(int maxHp) {
		if (maxHp < 1) {
			maxHp = 1;
		}
		m_maxHealth = maxHp;
	}
	void setHealth(int hp) {
		if (hp < 1) {
			hp = 1;
		}
		m_health = hp;
	}

	// Статус-эффекты
	void applySlow(float duration, float slowPercent);
	void applyPoison(float duration, float tickInterval, int damagePerTick);
	void applyBleed(float duration, float tickInterval, int damagePerTick);
	void applyVulnerable(float duration, float multiplier = 1.4f);
	bool isSlowed() const;
	bool isPoisoned() const;
	bool isBleeding() const;
	bool isVulnerable() const;
	float getDamageMultiplier() const;
	float getSpeedModifier() const { return m_speedModifier; }
	bool isKnockedBack() const { return m_isKnockedBack; }
	bool isStunned() const { return m_stunTimer > 0.0f; }
	bool isFalling() const { return m_isFalling; }
	bool isKnockbackImmune() const { return false; } // глобального иммунитета больше нет
	void addPistonCooldown(glm::ivec2 pistonCell, float duration = 5.0f);
	bool isImmuneToPiston(glm::ivec2 pistonCell) const;
	glm::vec2 getWallImpactOffset() const { return m_wallImpactOffset; }
	void pushOneCell(glm::ivec2 fromCell, glm::ivec2 punchDir, const Grid& grid, ParticleSystem* particleSystem = nullptr, float pushDistance = 1.0f);
	void startFalling(glm::ivec2 chasmCell, const Grid& grid);
	void applyKnockback(glm::vec2 direction, float force);
};