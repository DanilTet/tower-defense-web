#pragma once
#include <string>
#include <vector>

struct WavePart {
	std::string type = "Basic"; // тип врага: "Basic", "Fast", "Tank"
	int count = 10;            // сколько врагов заспавнить в пачке
	float spawnInterwal = 0.8f; // интервал между спавном мобов внутри пачки (в секундах)
	float delayAfter = 2.0f;    // пауза после завершения пачки перед следующей (в секундах)
};

struct WaveConfig {
	std::vector<WavePart> parts;
};

