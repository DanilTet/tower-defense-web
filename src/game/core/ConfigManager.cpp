#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// инициализация деревьев которіе как словари работают
std::map<std::string, std::vector<TowerStats>> ConfigManager::s_towerStats;
std::map<std::string, EnemyStats> ConfigManager::s_enemyStats;
std::map<std::string, ParticleEmitterProps> ConfigManager::s_particleProps;
static std::unordered_map<std::string, AtlasConfig> s_atlases;

bool ConfigManager::loadTextureConfig(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) return false;

	nlohmann::json j;
	file >> j;

	for (auto& [atlasName, atlasData] : j["texture_atlases"].items()) {
		AtlasConfig atlas;
		atlas.filePath = atlasData["file_path"].get<std::string>();
		atlas.width = atlasData["width"].get<int>();
		atlas.height = atlasData["height"].get<int>();

		for (auto& [regionName, regionData] : atlasData["regions"].items()) {
			AtlasRegion reg;
			reg.x = regionData["x"].get<int>();
			reg.y = regionData["y"].get<int>();
			reg.w = regionData["w"].get<int>();
			reg.h = regionData["h"].get<int>();
			atlas.regions[regionName] = reg;
		}
		s_atlases[atlasName] = atlas;
	}
	return true;
}

// функция получения готового UV для рендерера
SpriteUV ConfigManager::getUV(const std::string& atlasName, const std::string& regionName) {
	auto atlasIt = s_atlases.find(atlasName);
	if (atlasIt != s_atlases.end()) {
		auto regIt = atlasIt->second.regions.find(regionName);
		if (regIt != atlasIt->second.regions.end()) {
			AtlasRegion r = regIt->second;
			return SpriteUV::fromPixels(r.x, r.y, r.w, r.h, atlasIt->second.width, atlasIt->second.height);
		}
	}
	// если чет не то тогда возращаем все вообще
	return SpriteUV::fromPixels(0, 0, 64, 64, 1472, 832);
}

bool ConfigManager::hasUV(const std::string& atlasName, const std::string& regionName) {
	auto atlasIt = s_atlases.find(atlasName);
	if (atlasIt != s_atlases.end()) {
		return atlasIt->second.regions.find(regionName) != atlasIt->second.regions.end();
	}
	return false;
}

bool ConfigManager::loadConfigs(const std::string& towerPath, const std::string& enemiesPath, const std::string& particlesPath) {
	s_towerStats.clear();
	s_enemyStats.clear();
	
	//загрузка башен
	std::ifstream tFile(towerPath); // откріваем файл
	if (tFile.is_open()) {
		json j;
		tFile >> j;

		// парсим все башни из json
		for (auto& element : j.items()) {
			std::string towerName = element.key(); // имя башни
			json towerData = element.value(); // так называемые внутрености башни

			// cчитываем визуальные настройки из корня башни
			std::string tex = towerData.value("textureId", "towerTexture");
			glm::vec3 col = glm::vec3(1.0f); // по умолчанию белый

			if (towerData.contains("color") && towerData["color"].is_array() && towerData["color"].size() >= 3) {
				col = glm::vec3(
					towerData["color"][0].get<float>(),
					towerData["color"][1].get<float>(),
					towerData["color"][2].get<float>()
				);
			}

			// если есть уровни
			if (towerData.contains("levels") && towerData["levels"].is_array()) {
				for (const auto& lvlJson : towerData["levels"]) {
					TowerStats stats;
					stats.range = lvlJson["range"];
					stats.fireRate = lvlJson["fireRate"];
					stats.damage = lvlJson["damage"];
					stats.cost = lvlJson["cost"];
					stats.splashRadius = lvlJson["splashRadius"];
					stats.rotationSpeed = lvlJson.value("rotationSpeed", 300.0f);
					stats.buildSound = lvlJson["buildSound"];
					stats.attackSound = lvlJson["attackSound"];

					// присваиваем визуал
					stats.textureId = tex;
					stats.color = col;

					stats.muzzleParticle = lvlJson.value("muzzleParticle", "");
					stats.trailParticle = lvlJson.value("trailParticle", "");
					stats.impactParticle = lvlJson.value("impactParticle", "");

					stats.bulletTextureId = lvlJson.value("bulletTextureId", "bulletBasic");

					if (lvlJson.contains("bulletBaseSize")) {
						stats.bulletBaseSize = lvlJson["bulletBaseSize"].get<float>();
					}
					else {
						stats.bulletBaseSize = 16.0f;
					}

					if (lvlJson.contains("bulletSpeed")) {
						stats.bulletSpeed = lvlJson["bulletSpeed"].get<float>();
					}
					else {
						stats.bulletSpeed = 800.0f;
					}

					// статус-эффекты
					stats.slowDuration = lvlJson.value("slowDuration", 0.0f);
					stats.slowPercent = lvlJson.value("slowPercent", 0.0f);
					stats.poisonDuration = lvlJson.value("poisonDuration", 0.0f);
					stats.poisonInterval = lvlJson.value("poisonInterval", 0.5f);
					stats.poisonDamagePerTick = lvlJson.value("poisonDamagePerTick", 0);
					stats.knockbackForce = lvlJson.value("knockbackForce", 0.0f);

					// записываем статы в словарь по имени башни
					s_towerStats[towerName].push_back(stats);
				}
			}
		}
		tFile.close();
	}
	else {
		std::cerr << "Failed to load:" << towerPath << std::endl;
		return false;
	}

	// загрузка врагов
	std::ifstream eFile(enemiesPath); // откріваем файл
	if (eFile.is_open()) {
		json j;
		eFile >> j;
		// динамически парсим врагов
		for (auto& element : j.items()) {
			std::string enemyName = element.key();
			json eData = element.value();

			EnemyStats stats;
			stats.speed = eData.value("speed", 1.0f);
			stats.Maxhealth = eData.value("Maxhealth", 100);
			stats.sizeScale = eData.value("sizeScale", 0.5f);
			stats.reward = eData.value("reward", 10);
			stats.textureId = eData.value("textureId", "towerTexture");
			stats.deathSound = eData.value("deathSound", "");
			stats.deathParticle = eData.value("deathParticle", "BloodSplatter");

			stats.color = glm::vec3(1.0f);
			if (eData.contains("color") && eData["color"].is_array() && eData["color"].size() >= 3) {
				stats.color = glm::vec3(
					eData["color"][0].get<float>(),
					eData["color"][1].get<float>(),
					eData["color"][2].get<float>()
				);
			}
			stats.atlasWidth = eData.value("atlasWidth", 64);
			stats.atlasHeight = eData.value("atlasHeight", 64);

			if (eData.contains("animations")) {
				for (auto& animElement : eData["animations"].items()) {
					std::string animName = animElement.key();
					json animData = animElement.value();

					AnimationClip clip;
					clip.loop = animData.value("loop", true);

					if (animData.contains("frames") && animData["frames"].is_array()) {
						for (auto& frameJson : animData["frames"]) {
							int x = frameJson.value("x", 0);
							int y = frameJson.value("y", 0);
							int w = frameJson.value("w", 64);
							int h = frameJson.value("h", 64);
							float duration = frameJson.value("duration", 0.1f);

							AnimationFrame frame;
							frame.uv = SpriteUV::fromPixels(x, y, w, h, stats.atlasWidth, stats.atlasHeight);
							frame.duration = duration;
							clip.frames.push_back(frame);
						}
					}
					stats.animations[animName] = clip;
				}
			}

			s_enemyStats[enemyName] = stats;
		}
		eFile.close(); // закріваем файл
	}
	else {
		std::cerr << "Failed to load:" << enemiesPath << std::endl;
		return false;
	}

	std::ifstream pFile(particlesPath);
	if (pFile.is_open()) {
		json j;
		pFile >> j;
		for (auto& element : j.items()) {
			std::string pName = element.key();
			json pData = element.value();
			ParticleEmitterProps props;

			props.position = glm::vec2(0.0f);
			props.velocityDir = glm::vec2(0.0f);
			if (pData.contains("velocityDir") && pData["velocityDir"].is_array() && pData["velocityDir"].size() >= 2) {
				props.velocityDir = glm::vec2(pData["velocityDir"][0].get<float>(), pData["velocityDir"][1].get<float>());
			}

			props.velocityVariation = pData.value("velocityVariation", 50.0f);
			props.startColor = glm::vec3(1.0f);
			if (pData.contains("startColor") && pData["startColor"].is_array() && pData["startColor"].size() >= 3) {
				props.startColor = glm::vec3(pData["startColor"][0].get<float>(), pData["startColor"][1].get<float>(), pData["startColor"][2].get<float>());
			}
			props.endColor = glm::vec3(0.0f);
			if (pData.contains("endColor") && pData["endColor"].is_array() && pData["endColor"].size() >= 3) {
				props.endColor = glm::vec3(pData["endColor"][0].get<float>(), pData["endColor"][1].get<float>(), pData["endColor"][2].get<float>());
			}

			props.startSize = pData.value("startSize", 10.0f);
			props.endSize = pData.value("endSize", 2.0f);
			props.lifeTime = pData.value("lifeTime", 0.5f);
			props.spawnCount = pData.value("spawnCount", 10);

			s_particleProps[pName] = props;
		}
		pFile.close();
	}
	else { 
		std::cerr << "Failed to load:" << particlesPath << std::endl; 
		return false; 
	}



	std::cout << "Configs loaded successfully!" << std::endl;
	return true;
}

TowerStats ConfigManager::getTowerStats(const std::string& type, int level) {
	auto it = s_towerStats.find(type);
	if (it != s_towerStats.end() && !it->second.empty()) {
		// защита от выхода за границы вектора уровней
		int idx = level - 1;
		if (idx >= (int)it->second.size()) {
			return it->second.back(); // если запросили уровень выше максимального то возвращаем последний
		}
		return it->second[idx];
	}
	return TowerStats{}; // возвращаем пустую структуру в случае ошибки
}

EnemyStats ConfigManager::getEnemyStats(const std::string& type) {
	return s_enemyStats[type];
}

std::vector<std::string> ConfigManager::getAllTowerTypes() {
	std::vector<std::string> keys;
	for (const auto& pair : s_towerStats) {
		keys.push_back(pair.first);
	}
	return keys;
}

ParticleEmitterProps ConfigManager::getParticleProps(const std::string& name) {
	auto it = s_particleProps.find(name);
	if (it != s_particleProps.end()) {
		return it->second;
	}
	return ParticleEmitterProps{};
}