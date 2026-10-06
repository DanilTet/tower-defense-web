#pragma once
#include <vector>
#include <memory>
#include "entities/Enemy.h"
#include "entities/Tower.h"
#include "entities/Projectile.h"
#include "../../particles/ParticleSystem.h"

class Grid;
class SpriteRenderer;
class Texture2D;
struct PlayerStats;

class EntityManager {
private:
	std::vector<Projectile> m_projectiles; // проджектайлы
	std::vector<std::unique_ptr<Tower>> m_towers;// БАШНИ
	std::vector<std::unique_ptr<Enemy>> m_enemies; // вектор для хранения всех врагов на уровне
	std::unique_ptr<ParticleSystem> m_particleSystem; // партиклы

public:
	// конструктор
	EntityManager();

	// метод поиска свободной пули для башен
	Projectile* getFreeProjectile();

	// геттер пула для передачи в башни
	std::vector<Projectile>& getProjectilePool() { return m_projectiles; }

	// методы обновления и рендера
	void update(float dt, Grid& gameGrid);
	void render(SpriteRenderer* renderer,
		std::shared_ptr<Texture2D> mainAtlas, // атлас карты и башен
		std::shared_ptr<Texture2D> enemyAtlas, // атлас врагов
		std::shared_ptr<Texture2D> radiusTex,
		std::shared_ptr<Texture2D> arrowTex,
		std::shared_ptr<Texture2D> particleTex,
		Grid& gameGrid,
		Tower* selectedTower = nullptr);
	// методы добавления
	void addEnemy(std::unique_ptr<Enemy> enemy) { m_enemies.push_back(std::move(enemy)); }
	void addTower(std::unique_ptr<Tower> tower) { m_towers.push_back(std::move(tower)); }
	//void addProjectile(std::unique_ptr<Projectile> proj) { m_projectiles.push_back(std::move(proj)); }
	// методы взаемодействия с башнями
	Tower* getTowerAt(int gridX, int gridY); // получить указатель на башню по клетке
	void removeTower(int gridX, int gridY);  // удалить башню с поля


	// геттеры
	std::vector<std::unique_ptr<Tower>>& getTowers() { return m_towers; }
	std::vector<std::unique_ptr<Enemy>>& getEnemies() { return m_enemies; }
};