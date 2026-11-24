#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "CircleCollider.h"
#include <string>
#include "../../particles/ParticleSystem.h"
#include "../../renderer/SpriteRenderer.h"

class ParticleSystem;
class EntityManager;

enum class TargetMode {
	First, // ближе всего к базе
	Last, // дальше всего от базы (мин. пройденный путь)
	Close, // физически ближе всего к башне
	Weak // меньше всего здоровья
};

//параметры Поршня (data-driven, безопасные значения по умолчанию)
struct PistonStats {
	float pushDistance = 1.0f; // дистанция отталкивания (клеток)
	float heavyPushDistance = -1.0f; // для тяжёлых врагов; < 0 => как pushDistance
	std::vector<std::string> heavyEnemyTypes; // типы тяжёлых врагов
	int impactDamage = 0; // мгновенный урон шипами
	float bleedDuration = 0.0f; // 0 => без кровотечения
	float bleedInterval = 0.5f;
	int bleedDamagePerTick = 0;
	float vulnerableDuration = 0.0f; // 0 => без уязвимости
	float vulnerableMultiplier = 1.0f;
	float immunityCooldown = 5.0f; // иммунитет врага к этому поршню (сек)
};

//конфиг характеристик башни
struct TowerStats {
	float range = 2.0f; // радиус атаки
	float fireRate = 0.8f; // скорость атаки
	int damage; // урон
	int cost = 5; // цена башни
	float splashRadius; // сплеш урон
	float rotationSpeed; // скорость поворота башни

	std::string buildSound; // звук строительства
	std::string attackSound; // звук атаки

	std::string textureId; // ID текстуры
	float visualScale = 1.0f; // масштаб отрисовки спрайта относительно клетки
	float muzzleOffset = 0.42f; // смещение точки дула от центра (в долях от размера спрайта башни)
	glm::vec3 color; // цвет башни

	// ефекты партиклы
	std::string muzzleParticle;
	std::string trailParticle;
	std::string impactParticle;
	// пуля
	std::string bulletTextureId;
	float bulletBaseSize;
	float bulletSpeed;

	// статус-эффекты
	float slowDuration = 0.0f;
	float slowPercent = 0.0f;
	float poisonDuration = 0.0f;
	float poisonInterval = 0.5f;
	int poisonDamagePerTick = 0;
	float knockbackForce = 0.0f;

	// баланс Поршня (из towers.json, блок "piston" уровня)
	PistonStats piston;
};

class SpriteRenderer;
class Texture2D;
class Grid;
class Enemy;
class Projectile;

class Tower {
private:
	std::string m_type; //тип башни
	float m_range; //радиус атаки
	float m_fireRate; //скорость атаки
	int m_damage; //урон

	int m_gridX; // індекс столбца Х
	int m_gridY; // индекс рядка У

	//float m_splashDamage;

	float m_shotTimer;

	int m_currentLevel; // текущий левел
	int m_maxLevel; // макс левел


	float m_angle = 0.0f;
	float m_rotationSpeed;

	// debug
	bool m_showDebugArrow = false;

	// кеш
	float m_splashRadius;
	std::string m_attackSound;
	std::string m_buildSound;
	std::string m_textureId;
	float m_visualScale = 1.0f;
	float m_muzzleOffset = 0.42f;
	glm::vec3 m_color;

	std::string m_muzzleParticle;
	std::string m_trailParticle;
	std::string m_impactParticle;
	std::string m_bulletTextureId;
	float m_bulletBaseSize;
	float m_bulletSpeed;

	// статус-эффекты
	float m_slowDuration = 0.0f;
	float m_slowPercent = 0.0f;
	float m_poisonDuration = 0.0f;
	float m_poisonInterval = 0.5f;
	int m_poisonDamagePerTick = 0;
	float m_knockbackForce = 0.0f;
	PistonStats m_pistonStats; // баланс Поршня из конфига

	// режим наводки
	TargetMode m_targetMode = TargetMode::First;

	float m_punchAnimTimer = 0.0f; // анимация выталкивания бойка поршня

	// Кэш для горячих циклов update/render
	bool m_isPiston = false;
	SpriteUV m_cachedTowerUV;
	SpriteUV m_cachedBulletUV;
	const Texture2D* m_cachedCustomTexture = nullptr;
	const Texture2D* m_cachedWhiteTex = nullptr;
	bool m_hasMuzzleParticle = false;
	bool m_hasTrailParticle = false;
	bool m_hasImpactParticle = false;
	ParticleEmitterProps m_muzzleProps{};
	ParticleEmitterProps m_trailProps{};
	ParticleEmitterProps m_impactProps{};
	ParticleEmitterProps m_pistonSparkProps{};
	ParticleEmitterProps m_pistonBloodProps{};
	std::vector<Enemy*> m_pistonHitBuffer;

	void applyStats(const TowerStats& stats);

public:
	// получаем характеристики башни
	static TowerStats getStatsfromTowerType(const std::string& type);

	//конструктор
	Tower(int gridX, int gridY, const std::string& type, float angle = 270.0f);

	//обновление логики
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, EntityManager& entityManager, const Grid& grid, ParticleSystem& particleSystem);
	// отрисовка
	void render(SpriteRenderer* renderer, const Texture2D* atlasTexture, const Texture2D* radiusTexture, const Texture2D* arrowTexture, const Grid& grid, bool isSelected = false);

	bool upgrade(int& playerMoney);
	int getUpgradeCost() const;

	int getGridX() const { return m_gridX; }
	int getGridY() const { return m_gridY; }
	int getLevel() const { return m_currentLevel; }	
	int getMaxLevel() const { return m_maxLevel; }
	const std::string& getType() const { return m_type; }

	float getAngle() const { return m_angle; }
	void setAngle(float angle) { m_angle = angle; }
	void rotate90();
	glm::ivec2 getPistonTargetCell() const;

	// геттері и сеттер режима наводки
	TargetMode getTargetMode() const { return m_targetMode; }
	void toggleTargetMode() {
		int next = (static_cast<int>(m_targetMode) + 1) % 4;
		m_targetMode = static_cast<TargetMode>(next);
	}

	std::string getTargetModeString() const {
		switch (m_targetMode) {
		case TargetMode::First: return "FIRST";
		case TargetMode::Last:  return "LAST";
		case TargetMode::Close: return "CLOSE";
		case TargetMode::Weak:  return "WEAK";
		}
		return "FIRST";
	}

	// стетер для заргузки режима наведения
	void setTargetMode(TargetMode mode) { m_targetMode = mode; }
	// принудительное изменение уровня бащни
	void forceLevel(int level);
};