#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "CircleCollider.h"
#include <string>

class ParticleSystem;
class EntityManager;

enum class TargetMode {
	First, // ближе всего к базе
	Last, // дальше всего от базы (мин. пройденный путь)
	Close, // физически ближе всего к башне
	Weak // меньше всего здоровья
};

//конфиг характеристик башни
struct TowerStats {
	float range; // радиус атаки
	float fireRate; // скорость атаки
	int damage; // урон
	int cost; // цена башни
	float splashRadius; // сплеш урон
	float rotationSpeed; // скорость поворота башни

	std::string buildSound; // звук строительства
	std::string attackSound; // звук атаки

	std::string textureId; // ID текстуры
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

	// режим наводки
	TargetMode m_targetMode = TargetMode::First;

	float m_punchAnimTimer = 0.0f; // анимация выталкивания бойка поршня

	void applyStats(const TowerStats& stats);

public:
	// получаем характеристики башни
	static TowerStats getStatsfromTowerType(const std::string& type);

	//конструктор
	Tower(int gridX, int gridY, const std::string& type, float angle = 270.0f);

	//обновление логики
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, EntityManager& entityManager, const Grid& grid, ParticleSystem& particleSystem);
	// отрисовка
	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> atlasTexture, std::shared_ptr<Texture2D> radiusTexture, std::shared_ptr<Texture2D> arrowTexture, const Grid& grid, bool isSelected = false);

	bool upgrade(int& playerMoney);
	int getUpgradeCost() const;

	int getGridX() const { return m_gridX; }
	int getGridY() const { return m_gridY; }
	int getLevel() const { return m_currentLevel; }	
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