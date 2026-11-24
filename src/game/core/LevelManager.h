#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <glm/glm.hpp>
#include "WaveData.h"

// новая структура для базы с поддержкой ID для связи со спавнерами
struct BaseData {
    int x = 0;
    int y = 0;
    int id = 0; // ID базы (0, 1, 2...). Спавнеры с таким же targetBaseIndex направляются сюда
    bool visibleInGame = false; // Скрывать базу в бою
    bool enableFadeOut = true;  // Плавное растворение мобов перед базой

    BaseData() = default;
    BaseData(int _x, int _y, int _id = 0, bool _visibleInGame = false, bool _enableFadeOut = true)
        : x(_x), y(_y), id(_id), visibleInGame(_visibleInGame), enableFadeOut(_enableFadeOut) {}
    BaseData(glm::ivec2 p, int _id = 0, bool _visibleInGame = false, bool _enableFadeOut = true)
        : x(p.x), y(p.y), id(_id), visibleInGame(_visibleInGame), enableFadeOut(_enableFadeOut) {}

    glm::ivec2 pos() const { return glm::ivec2(x, y); }
    operator glm::ivec2() const { return glm::ivec2(x, y); }
    bool operator==(const BaseData& other) const {
        return x == other.x && y == other.y && id == other.id &&
               visibleInGame == other.visibleInGame && enableFadeOut == other.enableFadeOut;
    }
};

// новая структура для спавнера чтобі он помнил свою базу и группу
struct SpawnerData {
    glm::ivec2 pos{0, 0};
    int targetBaseIndex = -1; // -1 = искать ближайшую базу, >= 0 = конкретный ID базы
    int groupId = -1;         // Логический ID группы / маршрута / целевой базы
    bool visibleInGame = false; // По умолчанию скрыты в бою, чтобы под туманом был чистый асфальт
    bool enableFadeIn = true;   // Плавное проявление мобов из тумана/темноты

    SpawnerData() = default;
    SpawnerData(glm::ivec2 p, int targetBase = -1, int group = -1, bool _visibleInGame = false, bool _enableFadeIn = true)
        : pos(p), targetBaseIndex(targetBase), groupId(group >= 0 ? group : targetBase),
          visibleInGame(_visibleInGame), enableFadeIn(_enableFadeIn) {}

    int getLogicalId() const {
        if (groupId >= 0) return groupId;
        if (targetBaseIndex >= 0) return targetBaseIndex;
        return 0;
    }
};

// Конфигурация вагонетки (Hazard-ловушка)
struct MinecartData {
    glm::ivec2 start = glm::ivec2(-1, -1); // Депо / Точка старта (Grid X, Y)
    glm::ivec2 end = glm::ivec2(-1, -1);   // Тупик / Точка финиша (Grid X, Y)
    float interval = 25.0f;                // Интервал между рейсами (сек)
    float warningTime = 3.0f;              // Время предупреждения перед стартом (сек)
    float speed = 8.0f;                    // Скорость движения (клеток/сек)

    bool hasStart() const { return start.x >= 0 && start.y >= 0; }
    bool hasEnd() const { return end.x >= 0 && end.y >= 0; }
    bool isConfigured() const { return hasStart() && hasEnd(); }
};

struct ParticleEmitterConfig {
    int id = 0;
    std::string type = "steam_jet"; // "steam_jet", "water_drip", "sparks", "smoke", "fog"
    glm::vec2 tilePos = glm::vec2(0.0f); // координаты привязки в клетках сетки
    float angleDeg = 90.0f; // угол струи: 0=вправо, 90=вниз, 180=влево, 270=вверх
    float speed = 180.0f;       // Сила / начальная скорость вылета частиц
    float particleScale = 1.0f; // Размер частиц (0.5x .. 2.5x)
    float periodMin = 5.0f;     // Мин. интервал между выбросами
    float periodMax = 12.0f;    // Макс. интервал между выбросами
    float burstDuration = 2.2f; // Длительность залпа
    bool loopContinuous = false; // если true, эмиттер работает непрерывно без пауз
    int zIndex = 0;             // Слой отрисовки (-10 .. 10)
};

struct DecorationConfig {
    int id = 0;
    std::string type = "bush"; // "bush", "grass_tuft", "grass_field", "flower", "stone", "helmet", "pickaxe", "puddle", "crack", "fog", "wooden_crate", "broken_mercury_cart", "scrap_debris", "dead_tree", "dry_grass", "ore_debris"
    glm::vec2 tilePos = glm::vec2(0.0f);  // координаты в единицах сетки (float tileX, tileY)
    glm::vec2 worldPos = glm::vec2(0.0f); // абсолютная мировая позиция (актуализируется через tilePos)
    float scale = 1.0f;     // общий масштаб (0.5x .. 2.5x)
    float scaleX = 1.0f;    // вытягивание по горизонтали
    float scaleY = 1.0f;    // вытягивание по вертикали
    float opacity = 0.5f;   // прозрачность / глубина (0.1f .. 1.0f)
    float rotation = 0.0f;  // угол поворота (градусы)
    bool destructible = true; // сносится ли при постройке башни
    int zIndex = 0;         // Слой отрисовки (-10 .. 10)
};

struct EnemyWaveBountyRule {
    int fromWave = 1;   // с какой волны (включительно, нумерация с 1)
    int toWave = 99;    // по какую волну (включительно)
    int reward = 1;     // награда в $
    int rewardStep = 1; // каждый N-й убитый (1 = каждый, 2 = каждый 2-й, 3 = каждый 3-й и т.д.)
};

struct EnemyHpRule {
    int fromWave = 1;   // с какой волны (включительно, нумерация с 1)
    int toWave = 99;    // по какую волну (включительно)
    int hp = 10;        // переопределённое здоровье (HP) врага
};

// Для обратной совместимости
struct GlobalBountyConfig {
    int reward = 1;     // сколько $ давать
    int rewardStep = 1; // каждый N-й моб (1 = каждый, 2 = каждый 2-й, 3 = каждый 3-й)
};

inline std::string normalizeEnemyType(const std::string& type) {
    if (type == "enemy_coal" || type == "coal" || type == "Coal") {
        return "Coal";
    }
    if (type == "enemy_spark" || type == "spark" || type == "Spark") {
        return "Spark";
    }
    if (type == "mercury" || type == "Mercury") {
        return "Mercury";
    }
    return type;
}

inline std::pair<int, int> getEnemyBountyForWave(const std::map<std::string, std::vector<EnemyWaveBountyRule>>& rulesMap,
                                                const std::string& enemyType, int currentWaveIndex)
{
    // Если currentWave в коде начинается с 0 (первая волна = 0), перед поиском правила делаем int displayWave = currentWaveIndex + 1;
    int displayWave = (currentWaveIndex >= 0) ? (currentWaveIndex + 1) : 1;

    std::string normType = normalizeEnemyType(enemyType);

    auto findRule = [&](const std::string& key) -> const EnemyWaveBountyRule* {
        // Прямой поиск по ключу
        auto it = rulesMap.find(key);
        if (it != rulesMap.end()) {
            for (const auto& r : it->second) {
                if (displayWave >= r.fromWave && displayWave <= r.toWave) {
                    return &r;
                }
            }
        }
        // Поиск по нормализованным ключам карты
        for (const auto& [k, rules] : rulesMap) {
            if (normalizeEnemyType(k) == key) {
                for (const auto& r : rules) {
                    if (displayWave >= r.fromWave && displayWave <= r.toWave) {
                        return &r;
                    }
                }
            }
        }
        return nullptr;
    };

    // 1. Поиск правила по точному / нормализованному типу
    const EnemyWaveBountyRule* rule = findRule(normType);

    // 2. Для Mercury проверяем как точные имена ("Mercury_Large", "Mercury_Medium", "Mercury_Small"), так и префикс "Mercury"
    if (!rule) {
        if (normType == "Mercury_Large" || normType == "Mercury_Medium" || normType == "Mercury_Small" || normType.rfind("Mercury", 0) == 0) {
            rule = findRule("Mercury");
        }
    }

    if (rule) {
        return { rule->reward, rule->rewardStep };
    }
    return { 1, 1 };
}

// Возвращает переопределённое HP из enemyHpRules, либо -1 если правило не задано (используется базовое из enemies.json)
inline int getEnemyHpForWave(const std::map<std::string, std::vector<EnemyHpRule>>& rulesMap,
                             const std::string& enemyType, int currentWaveIndex)
{
    int displayWave = 1;
    if (currentWaveIndex >= 0) {
        displayWave = currentWaveIndex + 1;
    }

    std::string normType = normalizeEnemyType(enemyType);

    auto findRule = [&](const std::string& key) -> const EnemyHpRule* {
        auto it = rulesMap.find(key);
        if (it != rulesMap.end()) {
            for (const auto& r : it->second) {
                if (displayWave >= r.fromWave && displayWave <= r.toWave) {
                    return &r;
                }
            }
        }
        for (const auto& [k, rules] : rulesMap) {
            if (normalizeEnemyType(k) == key) {
                for (const auto& r : rules) {
                    if (displayWave >= r.fromWave && displayWave <= r.toWave) {
                        return &r;
                    }
                }
            }
        }
        return nullptr;
    };

    const EnemyHpRule* rule = findRule(normType);

    if (!rule) {
        if (normType == "Mercury_Large" || normType == "Mercury_Medium" || normType == "Mercury_Small" || normType.rfind("Mercury", 0) == 0) {
            rule = findRule("Mercury");
        }
    }

    if (rule && rule->hp > 0) {
        return rule->hp;
    }
    return -1;
}

// четенькая структура где храниться структура левела
struct LevelMapData {
    std::string name = "";          // Отображаемое имя (UTF-8, RU/UA/EN)
    bool isCampaign = false;        // true = Кампания, false = Тестовый / Кастомный
    std::vector<std::string> tags;  // Теги для фильтрации и поиска (например: "Ртуть", "Тест", "Сложный")
    std::string background = "default"; // "default", "pipes_canal", "mine_entrance", "cinnabar_mine"
    bool hasSidePockets = false;        // Включает боковые карманы (настил слева и вход в шахту справа) внутри матрицы

    struct CameraSettings {
        glm::vec2 targetTile = glm::vec2(-1.0f, -1.0f);   // центр обзора в координатах сетки (в клетках/тайлах)
        float zoom = 1.0f;                                // базовый зум камеры
        bool isCustom = false;
        float viewTilesW = 0.0f;                          // ширина кадра в количестве клеток/тайлов сетки
        float viewTilesH = 0.0f;                          // высота кадра в количестве клеток/тайлов сетки
    };
    CameraSettings camera;

    int gridWidth = 10;
    int gridHeight = 7;
    float cellSize = 64.0f;
    float offsetX = 20.0f;
    float offsetY = 20.0f;
    std::vector<SpawnerData> spawners;
    std::vector<BaseData> bases;
    std::vector<std::vector<int>> layout;
    std::vector<std::vector<bool>> noBuildMask; // [y][x], true = no-build zone

    struct OuterCellData {
        int x = 0;
        int y = 0;
        int type = 0;
        bool noBuild = false;
    };
    std::vector<OuterCellData> outerCells;
    std::vector<MinecartData> minecarts;
    std::vector<ParticleEmitterConfig> emitters;
    std::vector<DecorationConfig> decorations;
    std::vector<WaveConfig> waves;
    int startingMoney = 50;  // Бюджет на постройку базового лабиринта (на 10 башен по $5)
    int startingHealth = 20; // Жизни базы
    std::vector<std::string> allowedTowers = { "Basic", "Mercury", "Piston" }; // Доступные типы башен
    std::unordered_map<std::string, int> towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } }; // Максимальный тир прокачки для каждой башни
    int maxUpgradeTier = 3; // Максимальный тир апгрейда (1, 2 или 3) для совместимости
    std::map<std::string, std::vector<EnemyWaveBountyRule>> enemyBountyRules = {
        { "Coal",    { { 1, 99, 1, 1 } } },
        { "Spark",   { { 1, 99, 1, 1 } } },
        { "Mercury", { { 1, 99, 1, 1 } } }
    };
    std::map<std::string, GlobalBountyConfig> enemyGlobalBounties = {
        { "Coal", { 1, 1 } },
        { "Spark", { 1, 1 } },
        { "Mercury", { 1, 1 } }
    };
    std::map<std::string, std::vector<EnemyHpRule>> enemyHpRules;

    int getMaxTierForTower(const std::string& towerType) const {
        auto it = towerMaxTiers.find(towerType);
        if (it != towerMaxTiers.end()) {
            return std::clamp(it->second, 1, 3);
        }
        return std::clamp(maxUpgradeTier, 1, 3);
    }
};

// Информация об уровне для отображения в меню и редакторе
struct LevelInfo {
    std::string filename; // например: "level_1.json", "custom_map.json"
    std::string name;     // отображаемое имя, например: "Рівень 1", "Шахта Горловки"
    std::string fullPath; // путь для запуска: "res/levels/level_1.json"
    bool isCampaign = false; // true для сюжетных уровней кампании
    bool isBuiltIn = false;  // совместимость со старым кодом
    std::vector<std::string> tags; // список тегов уровня
};

class LevelManager {
public:
    static LevelMapData loadLevelMap(const std::string& filepath);
    static bool saveLevelMap(const std::string& filepath, const LevelMapData& data);

    // Управление и синхронизация уровней
    static std::vector<std::string> getLevelDirectories();
    static void syncLevelsBetweenSourceAndBuild();
    static std::vector<LevelInfo> getAvailableLevels();
    static std::vector<LevelInfo> getCustomLevels();
    static bool saveLevel(const std::string& levelFileName, const LevelMapData& data);
    static bool renameLevel(const std::string& oldFileName, const std::string& newFileName);
    static bool setLevelCampaign(const std::string& levelFileName, bool isCampaign);
    static bool setLevelTags(const std::string& levelFileName, const std::vector<std::string>& tags);
    static bool setLevelDisplayName(const std::string& levelFileName, const std::string& displayName);
    static bool deleteLevel(const std::string& levelFileName);
    static std::string createNewLevel(const std::string& displayName = "Новая карта", bool isCampaign = false);
    static std::string sanitizeLevelFileName(const std::string& name);
};
