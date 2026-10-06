#include "CampaignManager.h"
#include "LevelManager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::string CampaignManager::s_campaignTitle = "Горловские Рудники";
std::vector<CampaignMission> CampaignManager::s_missions;
std::unordered_set<std::string> CampaignManager::s_completedMissions;
bool CampaignManager::s_devMode = false;
bool CampaignManager::s_initialized = false;

void CampaignManager::init() {
    if (s_initialized) return;
    s_initialized = true;
    loadCampaign();
    loadProgress();
}

std::string CampaignManager::getCampaignFilePath() {
    auto dirs = LevelManager::getLevelDirectories();
    for (const auto& dir : dirs) {
        fs::path p = fs::path(dir) / "campaign.json";
        std::error_code ec;
        if (fs::exists(p, ec)) {
            return p.string();
        }
    }
    return "res/levels/campaign.json";
}

bool CampaignManager::loadCampaign(const std::string& filepath) {
    std::string path = filepath.empty() ? getCampaignFilePath() : filepath;
    std::ifstream file(path);

    if (!file.is_open()) {
        auto dirs = LevelManager::getLevelDirectories();
        for (const auto& dir : dirs) {
            fs::path p = fs::path(dir) / "campaign.json";
            file.open(p);
            if (file.is_open()) {
                path = p.string();
                break;
            }
        }
    }

    if (!file.is_open()) {
        std::cout << "[CampaignManager] campaign.json not found, initializing default..." << std::endl;
        s_campaignTitle = "Горловские Рудники";
        s_missions = {
            { "mission_1", "level_1.json", "Шурф №1: Вход в выработку", "Первая линия обороны рудника от мутантов", 1 },
            { "mission_2", "level_2.json", "Горизонт -200: Ртутные пути", "Оборона узких переходов над глубокими обрывами", 2 }
        };
        saveCampaign();
        return true;
    }

    try {
        json j;
        file >> j;
        s_campaignTitle = j.value("title", "Горловские Рудники");
        s_missions.clear();

        if (j.contains("missions") && j["missions"].is_array()) {
            int autoOrder = 1;
            for (const auto& item : j["missions"]) {
                CampaignMission m;
                m.id = item.value("id", "mission_" + std::to_string(autoOrder));
                m.file = item.value("file", "");
                m.name = item.value("name", m.file);
                m.description = item.value("description", "");
                m.order = item.value("order", autoOrder);
                if (!m.file.empty()) {
                    s_missions.push_back(m);
                    autoOrder++;
                }
            }
        }

        if (s_missions.empty()) {
            s_missions = {
                { "mission_1", "level_1.json", "Шурф №1: Вход в выработку", "Первая линия обороны рудника от мутантов", 1 },
                { "mission_2", "level_2.json", "Горизонт -200: Ртутные пути", "Оборона узких переходов над глубокими обрывами", 2 }
            };
            saveCampaign();
        }

        std::cout << "[CampaignManager] Loaded " << s_missions.size() << " missions from " << path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ERROR::CAMPAIGNMANAGER: Failed to parse campaign.json: " << e.what() << std::endl;
        return false;
    }
}

bool CampaignManager::saveCampaign(const std::string& filepath) {
    json j;
    j["title"] = s_campaignTitle;
    json missionsArr = json::array();

    for (size_t i = 0; i < s_missions.size(); ++i) {
        s_missions[i].order = static_cast<int>(i + 1);
        json item;
        item["id"] = s_missions[i].id;
        item["file"] = s_missions[i].file;
        item["name"] = s_missions[i].name;
        item["description"] = s_missions[i].description;
        item["order"] = s_missions[i].order;
        missionsArr.push_back(item);
    }
    j["missions"] = missionsArr;

    bool savedAny = false;
    auto dirs = LevelManager::getLevelDirectories();
    for (const auto& dir : dirs) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        fs::path p = fs::path(dir) / "campaign.json";
        std::ofstream out(p);
        if (out.is_open()) {
            out << j.dump(2) << std::endl;
            savedAny = true;
        }
    }

    if (!filepath.empty()) {
        std::ofstream out(filepath);
        if (out.is_open()) {
            out << j.dump(2) << std::endl;
            savedAny = true;
        }
    }

    std::cout << "[CampaignManager] Saved campaign manifest (" << s_missions.size() << " missions)" << std::endl;
    return savedAny;
}

const std::string& CampaignManager::getCampaignTitle() {
    init();
    return s_campaignTitle;
}

void CampaignManager::setCampaignTitle(const std::string& title) {
    s_campaignTitle = title;
    saveCampaign();
}

const std::vector<CampaignMission>& CampaignManager::getMissions() {
    init();
    return s_missions;
}

bool CampaignManager::moveMissionUp(int index) {
    init();
    if (index <= 0 || index >= static_cast<int>(s_missions.size())) return false;
    std::swap(s_missions[index], s_missions[index - 1]);
    for (size_t i = 0; i < s_missions.size(); ++i) {
        s_missions[i].order = static_cast<int>(i + 1);
    }
    saveCampaign();
    return true;
}

bool CampaignManager::moveMissionDown(int index) {
    init();
    if (index < 0 || index + 1 >= static_cast<int>(s_missions.size())) return false;
    std::swap(s_missions[index], s_missions[index + 1]);
    for (size_t i = 0; i < s_missions.size(); ++i) {
        s_missions[i].order = static_cast<int>(i + 1);
    }
    saveCampaign();
    return true;
}

bool CampaignManager::addMission(const std::string& file, const std::string& name, const std::string& description) {
    init();
    if (file.empty()) return false;
    for (const auto& m : s_missions) {
        if (m.file == file) return false;
    }
    CampaignMission m;
    m.id = "mission_" + std::to_string(s_missions.size() + 1);
    m.file = file;
    m.name = name.empty() ? file : name;
    m.description = description;
    m.order = static_cast<int>(s_missions.size() + 1);
    s_missions.push_back(m);
    saveCampaign();
    return true;
}

bool CampaignManager::removeMission(int index) {
    init();
    if (index < 0 || index >= static_cast<int>(s_missions.size())) return false;
    s_missions.erase(s_missions.begin() + index);
    for (size_t i = 0; i < s_missions.size(); ++i) {
        s_missions[i].order = static_cast<int>(i + 1);
    }
    saveCampaign();
    return true;
}

bool CampaignManager::isMissionUnlocked(int index, bool devMode) {
    init();
    if (devMode || s_devMode) return true;
    if (index <= 0) return true;
    if (index >= static_cast<int>(s_missions.size())) return false;

    // Предыдущая миссия должна быть завершена
    const auto& prev = s_missions[index - 1];
    return isMissionCompleted(prev.file) || isMissionCompleted(prev.id);
}

bool CampaignManager::isDevMode() {
    init();
    return s_devMode;
}

void CampaignManager::setDevMode(bool enabled) {
    init();
    s_devMode = enabled;
    saveProgress();
}

void CampaignManager::toggleDevMode() {
    init();
    s_devMode = !s_devMode;
    saveProgress();
}

bool CampaignManager::isMissionCompleted(const std::string& fileOrId) {
    init();
    return s_completedMissions.find(fileOrId) != s_completedMissions.end();
}

void CampaignManager::completeMission(const std::string& fileOrId) {
    init();
    s_completedMissions.insert(fileOrId);
    std::string fname = fs::path(fileOrId).filename().string();
    s_completedMissions.insert(fname);
    for (const auto& m : s_missions) {
        if (m.file == fileOrId || m.id == fileOrId || m.file == fname) {
            s_completedMissions.insert(m.file);
            s_completedMissions.insert(m.id);
            break;
        }
    }
    saveProgress();
}

void CampaignManager::resetProgress() {
    s_completedMissions.clear();
    saveProgress();
}

void CampaignManager::unlockAllProgress() {
    init();
    for (const auto& m : s_missions) {
        s_completedMissions.insert(m.id);
        s_completedMissions.insert(m.file);
    }
    saveProgress();
}

static std::string cleanLevelFileName(const std::string& filepath) {
    fs::path p(filepath);
    return p.filename().string();
}

bool CampaignManager::isFileInCampaign(const std::string& filename) {
    init();
    std::string clean = cleanLevelFileName(filename);
    for (const auto& m : s_missions) {
        if (m.file == clean || m.file == filename) return true;
    }
    return false;
}

int CampaignManager::getMissionIndexByFile(const std::string& filename) {
    init();
    std::string clean = cleanLevelFileName(filename);
    for (size_t i = 0; i < s_missions.size(); ++i) {
        if (s_missions[i].file == clean || s_missions[i].file == filename) return static_cast<int>(i);
    }
    return -1;
}

std::string CampaignManager::getNextMissionFile(const std::string& currentFile) {
    init();
    int idx = getMissionIndexByFile(currentFile);
    if (idx >= 0 && idx + 1 < static_cast<int>(s_missions.size())) {
        return s_missions[idx + 1].file;
    }
    return "";
}

void CampaignManager::loadProgress() {
    s_completedMissions.clear();
    fs::path savePath = "saves/campaign_progress.json";
    if (!fs::exists(savePath)) return;

    std::ifstream file(savePath);
    if (!file.is_open()) return;

    try {
        json j;
        file >> j;
        s_devMode = j.value("dev_mode", false);
        if (j.contains("completed") && j["completed"].is_array()) {
            for (const auto& item : j["completed"]) {
                if (item.is_string()) {
                    s_completedMissions.insert(item.get<std::string>());
                }
            }
        }
    } catch (...) {}
}

void CampaignManager::saveProgress() {
    std::error_code ec;
    fs::create_directories("saves", ec);
    fs::path savePath = "saves/campaign_progress.json";
    std::ofstream file(savePath);
    if (!file.is_open()) return;

    json j;
    j["dev_mode"] = s_devMode;
    json arr = json::array();
    for (const auto& item : s_completedMissions) {
        arr.push_back(item);
    }
    j["completed"] = arr;
    file << j.dump(2) << std::endl;
}
