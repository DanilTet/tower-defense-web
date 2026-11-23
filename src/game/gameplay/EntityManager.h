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
	static constexpr size_t MAX_ACTIVE_PROJECTILES = 1024;

	std::vector<Projectile> m_projectiles; // проджектайлы
	std::vector<std::unique_ptr<Tower>> m_towers;// БАШНИ
	std::vector<std::unique_ptr<Enemy>> m_enemies; // вектор для хранения всех врагов на уровне
	std::unique_ptr<ParticleSystem> m_particleSystem; // партиклы

	// Кэшированные буферы для минимизации аллокаций и O(1) поиска цели пулями
	std::vector<std::unique_ptr<Enemy>> m_spawnedChildrenBuffer;
	std::vector<Enemy*> m_enemyLookup;
	int m_enemyLookupBaseId = 0;
	bool m_enemyLookupValid = false;
	std::vector<Enemy*> m_renderEnemyBuffer;
	const Texture2D* m_coalGlowTexture = nullptr;
	bool m_coalGlowResolved = false;
	const Texture2D* m_whiteTexture = nullptr;

	void rebuildEnemyLookup();
	Enemy* findAliveEnemyById(int id) const;

public:
	// конструктор
	EntityManager();

	// метод поиска свободной пули для башен
	Projectile* getFreeProjectile();

	// геттер пула для передачи в башни
	std::vector<Projectile>& getProjectilePool() { return m_projectiles; }

	// методы обновления и рендера
	void update(float dt, Grid& gameGrid, float timeScale = 1.0f);
	void render(SpriteRenderer* renderer,
		const Texture2D* mainAtlas, // атлас карты и башен
		const Texture2D* enemyAtlas, // атлас врагов
		const Texture2D* radiusTex,
		const Texture2D* arrowTex,
		const Texture2D* particleTex,
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