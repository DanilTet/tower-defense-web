#pragma once

#include <vector>
#include <string>
#include "entities/Enemy.h"
#include "WaveData.h"
#include "LevelManager.h"

struct SpawnRequest {
	std::string type;
	int spawnerIndex;
	int reward = 1;
	int rewardStep = 1;
	int waveIndex = 0;
};

struct PlayerStats;

class WaveManager {
public:
	//конструктор
	WaveManager();

	bool loadLevel(const std::string& filepath); // считываем левел с файла
	bool loadWaves(const std::vector<WaveConfig>& waves); // загрузка уже распарсенных волн из LevelMapData

	void startNextWave(); // начинаем новую волну
	std::vector<SpawnRequest> update(float dt, const std::vector<SpawnerData>& spawners); // обновляем менеджер врагов по логическим ID
	std::vector<SpawnRequest> update(float dt, int totalSpawners); // совместимость
	void onEnemiesUpdated(bool noActiveEnemies); // завершение волны только после полной зачистки врагов

	// Ранний вызов волны (Фаза 4.2)
	bool canCallEarlyWave() const;
	int getEarlyCallBonus() const;
	void triggerEarlyWave(PlayerStats* player = nullptr);

	// Автопродолжение волн (Auto-Start Waves)
	void toggleAutoStartWaves();
	void setAutoStartWaves(bool enabled);
	bool isAutoStartWaves() const { return m_autoStartWaves; }

	// геттеры состояния
	void setBountyRules(const std::map<std::string, std::vector<EnemyWaveBountyRule>>& rules) { m_bountyRules = rules; }
	const std::map<std::string, std::vector<EnemyWaveBountyRule>>& getBountyRules() const { return m_bountyRules; }
	void setGlobalBounties(const std::map<std::string, std::vector<EnemyWaveBountyRule>>& rules) { m_bountyRules = rules; }
	void setGlobalBounties(const std::map<std::string, GlobalBountyConfig>& bounties) {
		m_bountyRules.clear();
		for (const auto& kv : bounties) {
			m_bountyRules[kv.first] = { { 1, 99, kv.second.reward, kv.second.rewardStep } };
		}
	}
	const std::map<std::string, std::vector<EnemyWaveBountyRule>>& getGlobalBounties() const { return m_bountyRules; }

	int getCurrentWaveIndex() const { return m_currentWaveIndex; }
	int getCurrentWaveNumber() const { return m_activeWaveNumber; }
	int getTotalWaves() const { return static_cast<int>(m_waves.size()); }
	bool isWaveActive() const { return m_isWaveActive; }
	bool isGameStarted() const { return m_gameStarted; }
	bool areAllEnemiesCleared() const { return m_noActiveEnemies; }
	float getWaveCountdown() const { return m_waveCountdown; }
	float getEffectiveWaveCountdown() const;
	float getTimeBetweenWaves() const { return m_timeBetweenWaves; }

	bool isAllWavesCompleted() const {
		if (m_waves.empty()) {
			return false;
		}
		return !m_isWaveActive && m_noActiveEnemies && m_currentWaveIndex >= static_cast<int>(m_waves.size());
	}
	// Реестр поддерживаемых типов врагов
	static const std::vector<std::string>& getRegisteredEnemyTypes() {
		static const std::vector<std::string> s_types = {
			"Coal",
			"Spark",
			"Mercury_Large",
			"Mercury_Medium",
			"Mercury_Small"
			// , "Basic", "Fast", "Tank"
		};
		return s_types;
	}

	// сеттеры состояния волны (для загрузки сохранения)
	void setCurrentWaveIndex(int index);
	void setGameStarted(bool started);

private:
	void finishSpawningCurrentWave();

	std::vector<WaveConfig> m_waves;
	std::map<std::string, std::vector<EnemyWaveBountyRule>> m_bountyRules;
	bool m_isWaveActive; // идёт ли сейчас спавн пачек текущей волны
	int m_currentWaveIndex; // индекс следующей волны для запуска (0-based)
	int m_spawningWaveIndex = 0; // индекс волны, которая сейчас спавнится
	int m_activeWaveNumber = 1;  // отображаемый номер текущей волны (1-based)

	size_t m_currentPartIndex; // какая пачка щас идет
	int m_enemiesSpawnedInCurrentPart; // количество врагов заспавненых из текущей пачки
	float m_spawnTimer; // время спавна между врагами

	int m_currentSpawnerIndex = 0;

	// Таймер после выхода последнего моба волны и ранний старт
	static constexpr float DEFAULT_NEXT_WAVE_DELAY = 12.0f;
	float m_timeBetweenWaves = DEFAULT_NEXT_WAVE_DELAY; // дефолтное время до следующей волны (сек)
	float m_waveCountdown = 0.0f;    // оставшиеся секунды до следующей волны
	bool m_countdownActive = false;  // запущен ли отсчёт до следующей волны
	bool m_gameStarted = false;      // запущена ли игра (первая волна)
	bool m_noActiveEnemies = true;   // нет ли живых врагов на карте

	// Автопродолжение волн
	static constexpr float AUTO_START_DELAY = 1.0f;
	bool m_autoStartWaves = false;
	float m_autoStartTimer = 0.0f;
};