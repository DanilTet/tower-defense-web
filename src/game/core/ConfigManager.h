#pragma once
#include <map>
#include <string>
#include <vector>
#include "entities/Tower.h"
#include "entities/Enemy.h"
#include "../../particles/ParticleSystem.h"
#include "core/Animator.h"
#include "renderer/SpriteRenderer.h"

struct AtlasRegion {
    int x, y, w, h;
};

struct AtlasConfig {
    std::string filePath;
    int width;
    int height;
    std::unordered_map<std::string, AtlasRegion> regions;
};

class ConfigManager {
public:
    // загрузка всех конфигов при старте игры
    static bool loadConfigs(const std::string& towersPath, const std::string& enemiesPath, const std::string& particlesPath);

    // получение готовых стат
    static TowerStats getTowerStats(const std::string& type, int level = 1);
    static EnemyStats getEnemyStats(const std::string& type);
    // метод для получения всех загруженых имен
    static std::vector<std::string> getAllTowerTypes();
    // геттер конфига партиклов
    static ParticleEmitterProps getParticleProps(const std::string& name);

    static bool loadTextureConfig(const std::string& filepath);
    static SpriteUV getUV(const std::string& atlasName, const std::string& regionName);
    static bool hasUV(const std::string& atlasName, const std::string& regionName);

private:
    static std::map<std::string, std::vector<TowerStats>> s_towerStats; // виды статы башен
    static std::map<std::string, EnemyStats> s_enemyStats; // виды статы врагов
    static std::map<std::string, ParticleEmitterProps> s_particleProps; // хранилище эффектов
};