#include "WaveManager.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <nlohmann/json.hpp>
#include "../gameplay/PlayerStats.h"

WaveManager::WaveManager()
	: m_isWaveActive(false),
	m_currentWaveIndex(0),
	m_currentPartIndex(0),
	m_enemiesSpawnedInCurrentPart(0),
	m_spawnTimer(0.0f),
	m_currentSpawnerIndex(0),
	m_timeBetweenWaves(15.0f),
	m_waveCountdown(0.0f),
	m_gameStarted(false)
{
	
}

bool WaveManager::loadWaves(const std::vector<WaveConfig>& waves) {
	// все очищаем перед загрузкой
	m_waves.clear();
	m_currentPartIndex = 0;
	m_enemiesSpawnedInCurrentPart = 0;
	m_spawnTimer = 0.0f;
	m_isWaveActive = false;
	m_gameStarted = false;
	m_waveCountdown = 0.0f;
	m_currentWaveIndex = 0;
	m_currentSpawnerIndex = 0;

	m_waves = waves;
	for (auto& wave : m_waves) {
		for (auto& part : wave.parts) {
			part.spawnInterwal = std::max(0.05f, part.spawnInterwal);
		}
	}
	return !m_waves.empty();
}

bool WaveManager::loadLevel(const std::string& filepath) {
	// все очищаем перед загрузкой
	m_waves.clear();
	m_currentPartIndex = 0;
	m_isWaveActive = false;
	m_gameStarted = false;
	m_waveCountdown = 0.0f;
	m_currentWaveIndex = 0;
	m_currentSpawnerIndex = 0;

	std::ifstream file(filepath); // открываем файл

	// если файл не открылся
	if (!file.is_open()) {
		std::cerr << "ERROR::WAVEMANAGER: Could not open level file: " << filepath << std::endl;
		return false;
	}

	try {
		nlohmann::json j;
		file >> j;
		// проходим по всем волнам в уровне
		if (!j.contains("waves") || !j["waves"].is_array()) {
			std::cerr << "ERROR::WAVEMANAGER: Level has no \"waves\" array: " << filepath << std::endl;
			return false;
		}
		for (const auto& waveJson : j["waves"]) {
			WaveConfig wave;
			// проходим по всем пачкам в волне
			if (!waveJson.contains("parts") || !waveJson["parts"].is_array()) {
				m_waves.push_back(wave);
				continue;
			}
			for (const auto& partJson : waveJson["parts"]) {
				WavePart part;
				part.count = partJson.value("count", 10); // присваевем количество врагов из текущей пачки
				float rawSpawnInterval = 0.8f;
				if (partJson.contains("interval")) {
					rawSpawnInterval = partJson["interval"].get<float>();
				} else if (partJson.contains("spawnInterwal")) {
					rawSpawnInterval = partJson["spawnInterwal"].get<float>();
				}
				part.spawnInterwal = std::max(0.05f, rawSpawnInterval); // интервал между спавном
				part.delayAfter = partJson.value("delayAfter", 2.0f); // интервал между пачками

				part.type = partJson.value("type", "Basic");
				if (partJson.contains("spawner_id")) {
					part.spawnerId = partJson["spawner_id"].get<int>();
				} else if (partJson.contains("spawnerId")) {
					part.spawnerId = partJson["spawnerId"].get<int>();
				} else {
					part.spawnerId = -1;
				}

				part.reward = partJson.value("reward", -1);
				if (partJson.contains("reward_step")) {
					part.rewardStep = partJson.value("reward_step", -1);
				} else if (partJson.contains("rewardStep")) {
					part.rewardStep = partJson.value("rewardStep", -1);
				} else {
					part.rewardStep = -1;
				}

				wave.parts.push_back(part); // добаляем пачку в вектор
			}
			m_waves.push_back(wave); // добавляем волну в вектор
		}
	}
	// если не получилось считать файл json
	catch (const std::exception& e) {
		m_waves.clear();
		std::cerr << "ERROR::WAVEMANAGER: JSON error: " << e.what() << std::endl;
		return false;
	}
	return true;
}

void WaveManager::startNextWave() {
	// если волна активна или волны закончились то ниче не делаем
	if (m_isWaveActive || m_currentWaveIndex >= static_cast<int>(m_waves.size())) {
		return;
	}

	m_gameStarted = true;
	m_waveCountdown = 0.0f;
	m_isWaveActive = true; // делаем что волна началась
	m_currentPartIndex = 0; // Начинаем с первой пачки
	m_enemiesSpawnedInCurrentPart = 0;
	m_spawnTimer = 0.0f; // Первый враг вылетает моментально
}

bool WaveManager::canCallEarlyWave() const {
	if (m_waves.empty() || m_currentWaveIndex >= static_cast<int>(m_waves.size())) {
		return false;
	}
	// До старта игры (перед первой волной)
	if (!m_gameStarted && !m_isWaveActive) {
		return true;
	}
	// Во время межволновой паузы (после полной зачистки предыдущей волны)
	if (!m_isWaveActive) {
		return true;
	}
	return false;
}

int WaveManager::getEarlyCallBonus() const {
	if (m_waveCountdown <= 0.0f) {
		return 0;
	}
	return static_cast<int>(std::ceil(m_waveCountdown)) * 2;
}

void WaveManager::triggerEarlyWave(PlayerStats* player) {
	if (!canCallEarlyWave()) {
		return;
	}
	int bonus = getEarlyCallBonus();
	if (player && bonus > 0) {
		player->addMoney(bonus);
	}
	m_waveCountdown = 0.0f;
	startNextWave();
}

void WaveManager::setCurrentWaveIndex(int index) {
	if (index < 0) {
		index = 0;
	}
	m_currentWaveIndex = index;
	m_gameStarted = (index > 0);
	m_isWaveActive = false;
	m_currentPartIndex = 0;
	m_enemiesSpawnedInCurrentPart = 0;
	m_spawnTimer = 0.0f;
	m_waveCountdown = 0.0f;
}

void WaveManager::setGameStarted(bool started) {
	m_gameStarted = started;
}

void WaveManager::onEnemiesUpdated(bool noActiveEnemies) {
	if (!m_isWaveActive || !noActiveEnemies) {
		return;
	}
	if (m_currentWaveIndex < 0 || m_currentWaveIndex >= static_cast<int>(m_waves.size())) {
		return;
	}
	const WaveConfig& currentWave = m_waves[m_currentWaveIndex];
	if (m_currentPartIndex < currentWave.parts.size()) {
		return;
	}

	m_isWaveActive = false;
	m_currentWaveIndex++;
	if (m_currentWaveIndex < static_cast<int>(m_waves.size())) {
		m_waveCountdown = m_timeBetweenWaves;
	} else {
		m_waveCountdown = 0.0f;
	}
}

std::vector<SpawnRequest> WaveManager::update(float dt, const std::vector<SpawnerData>& spawners) {
	std::vector<SpawnRequest> spawnRequests;
	if (m_currentWaveIndex >= static_cast<int>(m_waves.size())) {
		return spawnRequests;
	}

	// Межволновая пауза: отсчет таймера до следующей волны
	if (!m_isWaveActive) {
		if (m_gameStarted && m_waveCountdown > 0.0f) {
			m_waveCountdown -= dt;
			if (m_waveCountdown <= 0.0f) {
				m_waveCountdown = 0.0f;
				startNextWave();
			}
		}
		return spawnRequests;
	}

	if (spawners.empty()) {
		return spawnRequests;
	}
	// берем конфиг текущей волны
	const WaveConfig& currentWave = m_waves[m_currentWaveIndex];
	if (m_currentPartIndex >= currentWave.parts.size()) {
		// Все враги волны уже заспавнены; ждём полной зачистки в onEnemiesUpdated()
		return spawnRequests;
	}

	m_spawnTimer -= dt;
	if (m_spawnTimer <= 0.0f) {
		// берем конфиг текущей пачки
		const WavePart& currentPart = currentWave.parts[m_currentPartIndex];
		if (currentPart.count <= 0) {
			m_currentPartIndex++;
			m_enemiesSpawnedInCurrentPart = 0;
			return spawnRequests;
		}

		// выбор спавнера по логическому ID / Round-Robin
		int targetSpawnerIdx = 0;
		if (currentPart.spawnerId >= 0) {
			// Найти спавнер, у которого логический ID совпадает с выбранным spawnerId (0, 1, 2...)
			std::vector<int> matchingIndices;
			for (size_t i = 0; i < spawners.size(); ++i) {
				if (spawners[i].getLogicalId() == currentPart.spawnerId) {
					matchingIndices.push_back(static_cast<int>(i));
				}
			}

			if (!matchingIndices.empty()) {
				// Спавнить моба СТРОГО на координатах и по пути соответствующего спавнера
				targetSpawnerIdx = matchingIndices[m_currentSpawnerIndex % matchingIndices.size()];
				m_currentSpawnerIndex++;
			} else {
				// Безопасный откат: если спавнер с указанным ID не найден, спавним по round-robin
				targetSpawnerIdx = m_currentSpawnerIndex % spawners.size();
				m_currentSpawnerIndex = (m_currentSpawnerIndex + 1) % spawners.size();
			}
		} else {
			// Если spawnerId < 0 ("Все"): спавнить по кругу (Round-Robin) по всем доступным спавнерам
			targetSpawnerIdx = m_currentSpawnerIndex % spawners.size();
			m_currentSpawnerIndex = (m_currentSpawnerIndex + 1) % spawners.size();
		}

		// Награда ДОЛЖНА БРАТЬСЯ СТРОГО из enemyBountyRules текущей карты через getEnemyBountyForWave!
		auto bounty = getEnemyBountyForWave(m_bountyRules, currentPart.type, m_currentWaveIndex);
		int finalReward = bounty.first;
		int finalRewardStep = bounty.second;

		spawnRequests.push_back({currentPart.type, targetSpawnerIdx, finalReward, finalRewardStep, m_currentWaveIndex});
		m_enemiesSpawnedInCurrentPart++;

		// закончилась ли текущая пачка???
		if (m_enemiesSpawnedInCurrentPart >= currentPart.count) {
			// переходим к следующей пачке
			m_currentPartIndex++;
			m_enemiesSpawnedInCurrentPart = 0;

			// закончились ли все пачки в этой волне???
			if (m_currentPartIndex < currentWave.parts.size()) {
				m_spawnTimer = currentPart.delayAfter;
			}
		}
		else {
			m_spawnTimer = currentPart.spawnInterwal;
		}
	}
	return spawnRequests;
}

std::vector<SpawnRequest> WaveManager::update(float dt, int totalSpawners) {
	std::vector<SpawnerData> dummySpawners;
	for (int i = 0; i < totalSpawners; ++i) {
		SpawnerData sd;
		sd.groupId = i;
		dummySpawners.push_back(sd);
	}
	return update(dt, dummySpawners);
}