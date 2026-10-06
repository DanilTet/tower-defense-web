#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "WaveData.h"

// новая структура для базы с поддержкой ID для связи со спавнерами
struct BaseData {
    int x = 0;
    int y = 0;
    int id = 0; // ID базы (0, 1, 2...). Спавнеры с таким же targetBaseIndex направляются сюда

    BaseData() = default;
    BaseData(int _x, int _y, int _id = 0) : x(_x), y(_y), id(_id) {}
    BaseData(glm::ivec2 p, int _id = 0) : x(p.x), y(p.y), id(_id) {}

    glm::ivec2 pos() const { return glm::ivec2(x, y); }
    operator glm::ivec2() const { return glm::ivec2(x, y); }
    bool operator==(const BaseData& other) const {
        return x == other.x && y == other.y && id == other.id;
    }
};

// новая структура для спавнера чтобі он помнил свою базу
struct SpawnerData {
    glm::ivec2 pos;
    int targetBaseIndex = -1; // -1 = искать ближайшую базу, >= 0 = конкретный ID базы
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

// четенькая структура где храниться структура левела
struct LevelMapData {
    std::string name = "";          // Отображаемое имя (UTF-8, RU/UA/EN)
    bool isCampaign = false;        // true = Кампания, false = Тестовый / Кастомный
    std::vector<std::string> tags;  // Теги для фильтрации и поиска (например: "Ртуть", "Тест", "Сложный")

    int gridWidth = 10;
    int gridHeight = 7;
    float cellSize = 64.0f;
    float offsetX = 20.0f;
    float offsetY = 20.0f;
    std::vector<SpawnerData> spawners;
    std::vector<BaseData> bases;
    std::vector<std::vector<int>> layout;
    std::vector<MinecartData> minecarts;
    std::vector<WaveConfig> waves;
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