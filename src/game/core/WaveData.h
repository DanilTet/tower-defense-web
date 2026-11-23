#pragma once
#include <string>
#include <vector>

struct WavePart {
	std::string type = "Basic"; // тип врага: "Basic", "Fast", "Tank"
	int count = 10;            // сколько врагов заспавнить в пачке
	float spawnInterwal = 0.8f; // интервал между спавном мобов внутри пачки (в секундах)
	float delayAfter = 2.0f;    // пауза после завершения пачки перед следующей (в секундах)
	int spawnerId = -1;         // -1 = Все (Round-Robin), 0 = S#0, 1 = S#1, 2 = S#2 и т.д.
	int reward = -1;            // -1 = наследовать из карты (enemyGlobalBounties), >= 0 = переопределение
	int rewardStep = -1;        // -1 = наследовать из карты (enemyGlobalBounties), >= 1 = переопределение
};

using WaveGroup = WavePart;
using WaveEnemyGroupData = WavePart;

struct WaveConfig {
	std::vector<WavePart> parts;
};

