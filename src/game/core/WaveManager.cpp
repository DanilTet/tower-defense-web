#include "WaveManager.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

WaveManager::WaveManager()
	: m_isWaveActive(false),
	m_currentWaveIndex(0),
	m_currentPartIndex(0),
	m_enemiesSpawnedInCurrentPart(0),
	m_spawnTimer(0.0f) 
{
	
}

bool WaveManager::loadLevel(const std::string& filepath) {
	// все очищаем перед загрузкой
	m_waves.clear();
	m_currentPartIndex = 0;
	m_isWaveActive = false;
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
		for (const auto& waveJson : j["waves"]) {
			WaveConfig wave;
			// проходим по всем пачкам в волне
			for (const auto& partJson : waveJson["parts"]) {
				WavePart part;
				part.count = partJson["count"]; // присваевем количество врагов из текущей пачки
				part.spawnInterwal = partJson["interval"]; // интервал между спавном
				part.delayAfter = partJson["delayAfter"]; // интервал между пачками

				part.type = partJson["type"].get<std::string>();

				wave.parts.push_back(part); // добаляем пачку в вектор
			}
			m_waves.push_back(wave); // добавляем волну в вектор
		}
	}
	// если не получилось считать файл json
	catch (nlohmann::json::parse_error& e) {
		std::cerr << "ERROR::WAVEMANAGER: JSON parse error: " << e.what() << std::endl;
		return false;
	}
	// скачать
	std::cout << "Level loaded succesfuli! Total waves: " << m_waves.size() << std::endl;
	return true;
}

void WaveManager::startNextWave() {
	// если волна активна или волны закончились то ниче не делаем
	if (m_isWaveActive || m_currentWaveIndex >= m_waves.size()) {
        return;
    }

	m_isWaveActive = true; // делаем что волна началась
	m_currentPartIndex = 0; // Начинаем с первой пачки
	m_enemiesSpawnedInCurrentPart = 0;
	m_spawnTimer = 0.0f; // Первый враг вылетает моментально

	std::cout << "Wave " << (m_currentWaveIndex + 1) << " started!" << std::endl;
}

std::vector<SpawnRequest> WaveManager::update(float dt, int totalSpawners) {
	std::vector<SpawnRequest> spawnRequests;
	if (!m_isWaveActive || m_currentWaveIndex >= m_waves.size()) {
		return spawnRequests;
	}
	// берем конфиг текущей волны
	const WaveConfig& currentWave = m_waves[m_currentWaveIndex];

	m_spawnTimer -= dt;
	if (m_spawnTimer <= 0.0f) {
		// берем конфиг текущей пачки
		const WavePart& currentPart = currentWave.parts[m_currentPartIndex];

		// выбор спавнера
		if (totalSpawners > 0) {
			spawnRequests.push_back({currentPart.type, m_currentSpawnerIndex});
			// крутим в каком спавнере спавнить
			m_currentSpawnerIndex++;
			if (m_currentSpawnerIndex >= totalSpawners) {
				m_currentSpawnerIndex = 0;
			}
		}
		m_enemiesSpawnedInCurrentPart++;

		// закончилась ли текущая пачка???
		if (m_enemiesSpawnedInCurrentPart >= currentPart.count) {
			// переходим к следующей пачке
			m_currentPartIndex++;
			m_enemiesSpawnedInCurrentPart = 0;

			// закончились ли все пачки в єтой волне???
			if (m_currentPartIndex >= currentWave.parts.size()) {
				m_isWaveActive = false;
				m_currentWaveIndex++;
				std::cout << "all enemies in this wave vipusheni!" << std::endl;
			}
			else {
				m_spawnTimer = currentPart.delayAfter;
			}
		}
		else {
			m_spawnTimer = currentPart.spawnInterwal;
		}
	}
	return spawnRequests;
}