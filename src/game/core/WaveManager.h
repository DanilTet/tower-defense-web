#pragma once

#include <vector>
#include <string>
#include "entities/Enemy.h"
#include "WaveData.h"

struct SpawnRequest {
	std::string type;
	int spawnerIndex;
};

class WaveManager {
public:
	//конструктор
	WaveManager();

	bool loadLevel(const std::string& filepath); // считываем левел с файла

	void startNextWave(); // начинаем новую волну
	std::vector<SpawnRequest> update(float dt, int totalSpawners); // обновляем менеджер врагов

	// гетері для дебага потом убрать
	int getCurrentWaveNumber() const { return m_currentWaveIndex + 1; }
	bool isWaveActive() const { return m_isWaveActive; }

	bool isAllWavesCompleted() const {
		return !m_isWaveActive && m_currentWaveIndex >= m_waves.size();
	}
	// сеттер волны
	void setCurrentWaveIndex(int index) { m_currentWaveIndex = index; }

private:
	std::vector<WaveConfig> m_waves; // Вектор со всеми волнами игры
	bool m_isWaveActive; // флаг активна ли волна
	int m_currentWaveIndex; // индекс текущей волны

	size_t m_currentPartIndex; // какая пачка щас идет
	int m_enemiesSpawnedInCurrentPart;; // количество врагов заспавненых из текущей пачки
	float m_spawnTimer; // время спавна между врагами

	int m_currentSpawnerIndex = 0;
};