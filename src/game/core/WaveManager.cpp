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
	m_spawningWaveIndex(0),
	m_activeWaveNumber(1),
	m_currentPartIndex(0),
	m_enemiesSpawnedInCurrentPart(0),
	m_spawnTimer(0.0f),
	m_currentSpawnerIndex(0),
	m_timeBetweenWaves(DEFAULT_NEXT_WAVE_DELAY),
	m_waveCountdown(0.0f),
	m_countdownActive(false),
	m_gameStarted(false),
	m_noActiveEnemies(true),
	m_autoStartWaves(false),
	m_autoStartTimer(0.0f)
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
	m_noActiveEnemies = true;
	m_waveCountdown = 0.0f;
	m_countdownActive = false;
	m_autoStartTimer = 0.0f;
	m_currentWaveIndex = 0;
	m_spawningWaveIndex = 0;
	m_activeWaveNumber = 1;
	m_currentSpawnerIndex = 0;

	m_waves = waves;
	for (auto& wave : m_waves) {
		if (wave.nextWaveDelay < 0.0f) {
			wave.nextWaveDelay = DEFAULT_NEXT_WAVE_DELAY;
		}
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
	m_enemiesSpawnedInCurrentPart = 0;
	m_spawnTimer = 0.0f;
	m_isWaveActive = false;
	m_gameStarted = false;
	m_noActiveEnemies = true;
	m_waveCountdown = 0.0f;
	m_countdownActive = false;
	m_autoStartTimer = 0.0f;
	m_currentWaveIndex = 0;
	m_spawningWaveIndex = 0;
	m_activeWaveNumber = 1;
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
			if (waveJson.contains("nextWaveDelay") && waveJson["nextWaveDelay"].is_number()) {
				wave.nextWaveDelay = std::max(0.0f, waveJson["nextWaveDelay"].get<float>());
			} else if (waveJson.contains("waveDuration") && waveJson["waveDuration"].is_number()) {
				wave.nextWaveDelay = std::max(0.0f, waveJson["waveDuration"].get<float>());
			} else {
				wave.nextWaveDelay = DEFAULT_NEXT_WAVE_DELAY;
			}
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

void WaveManager::finishSpawningCurrentWave() {
	m_isWaveActive = false;
	int finishedWaveIdx = m_spawningWaveIndex;
	m_currentWaveIndex = finishedWaveIdx + 1;

	if (m_currentWaveIndex < static_cast<int>(m_waves.size())) {
		// Если на карте ещё ползут враги — запускаем бонусный таймер досрочного вызова (волну сам он НЕ запускает)
		if (!m_noActiveEnemies) {
			float delay = DEFAULT_NEXT_WAVE_DELAY;
			if (finishedWaveIdx >= 0 && finishedWaveIdx < static_cast<int>(m_waves.size())) {
				if (m_waves[finishedWaveIdx].nextWaveDelay >= 0.0f) {
					delay = m_waves[finishedWaveIdx].nextWaveDelay;
				}
			}
			m_waveCountdown = std::max(0.0f, delay);
			m_countdownActive = (m_waveCountdown > 0.0f);
			m_autoStartTimer = 0.0f;
		} else {
			// Поле уже чистое (например, пустая волна) — бонусный таймер риска не нужен
			m_waveCountdown = 0.0f;
			m_countdownActive = false;
			if (m_autoStartWaves && m_gameStarted && m_currentWaveIndex > 0) {
				m_autoStartTimer = AUTO_START_DELAY;
			} else {
				m_autoStartTimer = 0.0f;
			}
		}
	} else {
		// Это была последняя волна уровня — ждём зачистки живых врагов
		m_waveCountdown = 0.0f;
		m_countdownActive = false;
		m_autoStartTimer = 0.0f;
	}
}

void WaveManager::startNextWave() {
	// если спавн текущей волны ещё идёт или волны закончились — ничего не делаем
	if (m_isWaveActive || m_currentWaveIndex >= static_cast<int>(m_waves.size())) {
		return;
	}

	m_gameStarted = true;
	m_waveCountdown = 0.0f;
	m_countdownActive = false;
	m_autoStartTimer = 0.0f;
	m_spawningWaveIndex = m_currentWaveIndex;
	m_activeWaveNumber = m_spawningWaveIndex + 1;
	m_isWaveActive = true;
	m_currentPartIndex = 0;
	m_enemiesSpawnedInCurrentPart = 0;
	m_spawnTimer = 0.0f; // Первый враг вылетает моментально

	const WaveConfig& currentWave = m_waves[m_spawningWaveIndex];
	while (m_currentPartIndex < currentWave.parts.size() && currentWave.parts[m_currentPartIndex].count <= 0) {
		m_currentPartIndex++;
	}
	if (m_currentPartIndex >= currentWave.parts.size()) {
		// Если волна пустая/мгновенная — запускаем отсчёт сразу после её старта
		finishSpawningCurrentWave();
	}
}

float WaveManager::getEffectiveWaveCountdown() const {
	if (!m_isWaveActive && !m_noActiveEnemies && m_countdownActive && m_waveCountdown > 0.0f) {
		return m_waveCountdown;
	}
	return 0.0f;
}

bool WaveManager::canCallEarlyWave() const {
	if (m_waves.empty() || m_currentWaveIndex >= static_cast<int>(m_waves.size())) {
		return false;
	}
	// До старта первой волны
	if (!m_gameStarted && !m_isWaveActive) {
		return true;
	}
	// Сразу после выхода последнего врага текущей волны (и пока живы мобы, и после полной зачистки)
	if (!m_isWaveActive) {
		return true;
	}
	return false;
}

int WaveManager::getEarlyCallBonus() const {
	// Денежный бонус даётся только за риск досрочного вызова, пока на карте ещё живы враги и не истёк таймер
	if (m_isWaveActive || m_noActiveEnemies || !m_countdownActive || m_waveCountdown <= 0.0f) {
		return 0;
	}
	// +2$ за каждые полные 4 секунды оставшегося бонусного таймера
	int remainingSeconds = static_cast<int>(std::ceil(m_waveCountdown));
	return (remainingSeconds / 4) * 2;
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
	m_countdownActive = false;
	m_autoStartTimer = 0.0f;
	startNextWave();
}

void WaveManager::setAutoStartWaves(bool enabled) {
	m_autoStartWaves = enabled;
	if (m_autoStartWaves) {
		// Если все мобы текущей волны уже вышли и на карте не осталось живых врагов (после 1-й волны) — запускаем 1.0 сек паузу
		if (!m_isWaveActive && m_noActiveEnemies && m_gameStarted && m_currentWaveIndex > 0 && m_currentWaveIndex < static_cast<int>(m_waves.size())) {
			if (m_autoStartTimer <= 0.0f) {
				m_autoStartTimer = AUTO_START_DELAY;
			}
		}
	} else {
		m_autoStartTimer = 0.0f;
	}
}

void WaveManager::toggleAutoStartWaves() {
	setAutoStartWaves(!m_autoStartWaves);
}

void WaveManager::setCurrentWaveIndex(int index) {
	if (index < 0) {
		index = 0;
	}
	m_currentWaveIndex = index;
	m_spawningWaveIndex = index;
	int maxWaveNum = std::max(1, static_cast<int>(m_waves.size()));
	m_activeWaveNumber = std::min(index + 1, maxWaveNum);
	m_gameStarted = (index > 0);
	m_isWaveActive = false;
	m_noActiveEnemies = true;
	m_currentPartIndex = 0;
	m_enemiesSpawnedInCurrentPart = 0;
	m_spawnTimer = 0.0f;
	m_waveCountdown = 0.0f;
	m_countdownActive = false;
	m_autoStartTimer = 0.0f;
}

void WaveManager::setGameStarted(bool started) {
	m_gameStarted = started;
}

void WaveManager::onEnemiesUpdated(bool noActiveEnemies) {
	m_noActiveEnemies = noActiveEnemies;
	if (!noActiveEnemies) {
		return;
	}

	// Все враги на карте добиты — бонусный таймер риска больше не актуален
	if (!m_isWaveActive) {
		m_waveCountdown = 0.0f;
		m_countdownActive = false;
	}

	// Если Авто включено, все пачки волны вышли и на карте не осталось живых врагов — через 1.0 сек пускаем следующую
	if (m_autoStartWaves && !m_isWaveActive && m_gameStarted && m_currentWaveIndex > 0 && m_currentWaveIndex < static_cast<int>(m_waves.size())) {
		if (m_autoStartTimer <= 0.0f) {
			m_autoStartTimer = AUTO_START_DELAY;
		}
	}
}

std::vector<SpawnRequest> WaveManager::update(float dt, const std::vector<SpawnerData>& spawners) {
	std::vector<SpawnRequest> spawnRequests;

	// Фаза после выхода последнего врага текущей волны
	if (!m_isWaveActive) {
		if (m_currentWaveIndex >= static_cast<int>(m_waves.size())) {
			return spawnRequests;
		}

		// 1. Пока на карте живы враги — тикает ТОЛЬКО бонусный таймер раннего вызова (саму волну он НЕ запускает!)
		if (!m_noActiveEnemies) {
			m_autoStartTimer = 0.0f;
			if (m_gameStarted && m_countdownActive) {
				m_waveCountdown -= dt;
				if (m_waveCountdown <= 0.0f) {
					m_waveCountdown = 0.0f;
					m_countdownActive = false;
				}
			}
			return spawnRequests;
		}

		// 2. Когда ВСЕ враги на карте убиты:
		//    - если Авто ВЫКЛ: ждём ручного клика игрока бесконечно;
		//    - если Авто ВКЛ (после 1-й волны): делаем паузу 1.0 сек и запускаем следующую волну.
		if (m_autoStartWaves && m_gameStarted && m_currentWaveIndex > 0) {
			if (m_autoStartTimer <= 0.0f) {
				m_autoStartTimer = AUTO_START_DELAY;
			}
			m_autoStartTimer -= dt;
			if (m_autoStartTimer <= 0.0f) {
				m_autoStartTimer = 0.0f;
				startNextWave();
			}
		}

		if (!m_isWaveActive) {
			return spawnRequests;
		}
	}

	if (spawners.empty()) {
		return spawnRequests;
	}
	if (m_spawningWaveIndex < 0 || m_spawningWaveIndex >= static_cast<int>(m_waves.size())) {
		finishSpawningCurrentWave();
		return spawnRequests;
	}

	// берем конфиг текущей волны
	const WaveConfig& currentWave = m_waves[m_spawningWaveIndex];
	while (m_currentPartIndex < currentWave.parts.size() && currentWave.parts[m_currentPartIndex].count <= 0) {
		m_currentPartIndex++;
		m_enemiesSpawnedInCurrentPart = 0;
	}
	if (m_currentPartIndex >= currentWave.parts.size()) {
		finishSpawningCurrentWave();
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
		auto bounty = getEnemyBountyForWave(m_bountyRules, currentPart.type, m_spawningWaveIndex);
		int finalReward = bounty.first;
		int finalRewardStep = bounty.second;

		spawnRequests.push_back({currentPart.type, targetSpawnerIdx, finalReward, finalRewardStep, m_spawningWaveIndex});
		m_enemiesSpawnedInCurrentPart++;
		m_noActiveEnemies = false;

		// закончилась ли текущая пачка???
		if (m_enemiesSpawnedInCurrentPart >= currentPart.count) {
			// переходим к следующей пачке
			m_currentPartIndex++;
			m_enemiesSpawnedInCurrentPart = 0;

			while (m_currentPartIndex < currentWave.parts.size() && currentWave.parts[m_currentPartIndex].count <= 0) {
				m_currentPartIndex++;
			}

			// закончились ли все пачки в этой волне???
			if (m_currentPartIndex < currentWave.parts.size()) {
				m_spawnTimer = currentPart.delayAfter;
			} else {
				// Вышел последний враг текущей волны — сразу запускаем отсчёт волны
				finishSpawningCurrentWave();
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