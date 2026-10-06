#include "SaveManager.h"
#include "../gameplay/PlayerStats.h"
#include "WaveManager.h"
#include "../gameplay/EntityManager.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

SaveManager::SaveManager(std::string saveDir) : m_saveDirectory(std::move(saveDir)) {
    if (!std::filesystem::exists(m_saveDirectory)) {
        std::filesystem::create_directories(m_saveDirectory);
    }
}

void SaveManager::writeSave(const std::string& saveName,
    const std::string& levelPath,
    const PlayerStats& stats,
    const WaveManager& waveManager,
    EntityManager& entityManager) {
    json saveJson;

    // сохраняем информацию об уровне и текущей волне (0-based индекс)
    saveJson["level_path"] = levelPath;
    saveJson["current_wave_index"] = waveManager.getCurrentWaveNumber() - 1;

    // статистика игрока
    saveJson["player_stats"] = {
        {"money", stats.money},
        {"base_health", stats.baseHealth},
        {"score", stats.score}
    };

    // сериализация всех установленных башен
    json towersArray = json::array();
    for (const auto& tower : entityManager.getTowers()) {
        if (tower) {
            towersArray.push_back({
                {"type", tower->getType()},
                {"grid_x", tower->getGridX()},
                {"grid_y", tower->getGridY()},
                {"level", tower->getLevel()},
                {"target_mode", static_cast<int>(tower->getTargetMode())}
                });
        }
    }
    saveJson["towers"] = towersArray;

    // запись файла на диск
    std::filesystem::path filePath = m_saveDirectory / (saveName + ".json");
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << saveJson.dump(4);
        std::cout << "[SaveManager] Игра успешно сохранена: " << filePath << std::endl;
    }
    else {
        std::cerr << "[SaveManager] Не удалось открыть файл для записи: " << filePath << std::endl;
    }
}

bool SaveManager::readSave(const std::string& saveName,
    std::string& outLevelPath,
    PlayerStats& outStats,
    int& outWaveIndex,
    json& outTowersArray) {
    std::filesystem::path filePath = m_saveDirectory / (saveName + ".json");
    if (!std::filesystem::exists(filePath)) return false;

    std::ifstream inFile(filePath);
    if (!inFile.is_open()) return false;

    try {
        json saveJson;
        inFile >> saveJson;

        outLevelPath = saveJson.at("level_path").get<std::string>();
        outWaveIndex = saveJson.at("current_wave_index").get<int>();

        auto& statsJson = saveJson.at("player_stats");
        outStats.money = statsJson.at("money").get<int>();
        outStats.baseHealth = statsJson.at("base_health").get<int>();
        outStats.score = statsJson.at("score").get<int>();

        outTowersArray = saveJson.at("towers");
        return true;
    }
    catch (const json::exception& e) {
        std::cerr << "[SaveManager] Ошибка парсинга файла сохранения: " << e.what() << std::endl;
        return false;
    }
}