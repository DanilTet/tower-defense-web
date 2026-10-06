#pragma once
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

struct PlayerStats;
class WaveManager;
class EntityManager;

class SaveManager {
public:
    explicit SaveManager(std::string saveDir = "saves");

    // запись состояния в JSON файл
    void writeSave(const std::string& saveName,
        const std::string& levelPath,
        const PlayerStats& stats,
        const WaveManager& waveManager,
        EntityManager& entityManager);

    // чтение файла сохранения
    bool readSave(const std::string& saveName,
        std::string& outLevelPath,
        PlayerStats& outStats,
        int& outWaveIndex,
        nlohmann::json& outTowersArray);

private:
    std::filesystem::path m_saveDirectory;
};