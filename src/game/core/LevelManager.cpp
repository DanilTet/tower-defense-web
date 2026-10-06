#include "LevelManager.h"
#include "CampaignManager.h"
#include "InputManager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <unordered_set>
#include <cctype>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

LevelMapData LevelManager::loadLevelMap(const std::string& filepath) {
    LevelMapData data;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::string fname = fs::path(filepath).filename().string();
        for (const auto& dir : getLevelDirectories()) {
            fs::path alt = fs::path(dir) / fname;
            file.open(alt);
            if (file.is_open()) {
                break;
            }
        }
    }

    if (!file.is_open()) {
        std::cerr << "ERROR::LEVELMANAGER: Could not open level file: " << filepath << std::endl;
        return data;
    }

    try {
        json j;
        file >> j;

        data.name = j.value("name", "");
        std::string fname = fs::path(filepath).filename().string();
        bool defaultCampaign = (fname == "level_1.json" || fname == "level_2.json");
        data.isCampaign = j.value("isCampaign", defaultCampaign);

        if (j.contains("tags") && j["tags"].is_array()) {
            for (const auto& t : j["tags"]) {
                if (t.is_string()) {
                    data.tags.push_back(t.get<std::string>());
                }
            }
        }
        if (data.tags.empty() && data.isCampaign) {
            data.tags.push_back("Кампания");
        }

        // если есть блок map в json то читаем его 
        if (j.contains("map")) {
            data.gridWidth = j["map"].value("width", 10);
            data.gridHeight = j["map"].value("height", 7);
            data.cellSize = j["map"].value("cellSize", 64.0f);
            data.offsetX = j["map"].value("offsetX", 20.0f);
            data.offsetY = j["map"].value("offsetY", 20.0f);

            // читаем спавнеры
            for (const auto& spawner : j["map"]["spawners"]) {
                SpawnerData sd;
                sd.pos = { spawner["x"], spawner["y"] };
                sd.targetBaseIndex = spawner.value("targetBaseIndex", -1);
                data.spawners.push_back(sd);
            }

            // читаем базы
            for (const auto& base : j["map"]["bases"]) {
                BaseData bd;
                bd.x = base["x"];
                bd.y = base["y"];
                bd.id = base.value("id", 0);
                data.bases.push_back(bd);
            }
            // парсинг сетки
            //0 = Земля(Ground)
            //1 = Дорога(Path)
            //2 = Платформа(Platform)
            //3 = Вода / Скала(Scenery)
            //4 = Шурф / Обрыв(Chasm)
            //5 = Рельсы(Rail)
            if (j["map"].contains("layout")) {
                for (const auto& row : j["map"]["layout"]) {
                    std::vector<int> rowData;
                    for (const auto& cell : row) {
                        if (cell.is_number_integer()) {
                            rowData.push_back(cell.get<int>());
                        } else if (cell.is_string()) {
                            std::string s = cell.get<std::string>();
                            if (s == "path") rowData.push_back(1);
                            else if (s == "platform") rowData.push_back(2);
                            else if (s == "scenery" || s == "wall") rowData.push_back(3);
                            else if (s == "chasm") rowData.push_back(4);
                            else if (s == "rail") rowData.push_back(5);
                            else rowData.push_back(0);
                        } else {
                            rowData.push_back(0);
                        }
                    }
                    data.layout.push_back(rowData);
                }
            }

            // читаем вагонетки если есть
            const json* minecartsJson = nullptr;
            if (j["map"].contains("minecarts") && j["map"]["minecarts"].is_array()) {
                minecartsJson = &j["map"]["minecarts"];
            } else if (j.contains("minecarts") && j["minecarts"].is_array()) {
                minecartsJson = &j["minecarts"];
            }

            if (minecartsJson) {
                auto parsePoint = [](const json& p) -> glm::ivec2 {
                    if (p.is_array() && p.size() >= 2) {
                        return glm::ivec2(p[0].get<int>(), p[1].get<int>());
                    } else if (p.is_object()) {
                        return glm::ivec2(p.value("x", -1), p.value("y", -1));
                    }
                    return glm::ivec2(-1, -1);
                };

                for (const auto& cartNode : *minecartsJson) {
                    MinecartData cart;
                    if (cartNode.contains("start")) {
                        cart.start = parsePoint(cartNode["start"]);
                    }
                    if (cartNode.contains("end")) {
                        cart.end = parsePoint(cartNode["end"]);
                    }
                    cart.interval = cartNode.value("interval", 25.0f);
                    cart.warningTime = cartNode.value("warningTime", 3.0f);
                    cart.speed = cartNode.value("speed", 8.0f);
                    data.minecarts.push_back(cart);
                }
            }
        }
        else {
            std::cerr << "WARNING::LEVELMANAGER: No 'map' section found in " << filepath << std::endl;
        }

        // читаем волны если есть
        if (j.contains("waves") && j["waves"].is_array()) {
            for (const auto& waveJson : j["waves"]) {
                WaveConfig wave;
                if (waveJson.contains("parts") && waveJson["parts"].is_array()) {
                    for (const auto& partJson : waveJson["parts"]) {
                        WavePart part;
                        part.type = partJson.value("type", "Basic");
                        part.count = partJson.value("count", 10);
                        if (partJson.contains("interval")) {
                            part.spawnInterwal = partJson["interval"].get<float>();
                        } else if (partJson.contains("spawnInterwal")) {
                            part.spawnInterwal = partJson["spawnInterwal"].get<float>();
                        }
                        part.delayAfter = partJson.value("delayAfter", 2.0f);
                        wave.parts.push_back(part);
                    }
                }
                data.waves.push_back(wave);
            }
        }
    }
    catch (json::parse_error& e) {
        std::cerr << "ERROR::LEVELMANAGER: JSON parse error: " << e.what() << std::endl;
    }
    return data;
}

bool LevelManager::saveLevelMap(const std::string& filepath, const LevelMapData& data) {
    json j;
    // Попытаться прочесть существующий файл чтобы сохранить прочие секции (например waves)
    std::ifstream inFile(filepath);
    if (inFile.is_open()) {
        try {
            inFile >> j;
        }
        catch (...) {
            j = json::object();
        }
        inFile.close();
    }

    if (!j.is_object()) {
        j = json::object();
    }

    if (!data.name.empty()) {
        j["name"] = data.name;
    }
    j["isCampaign"] = data.isCampaign;
    j["tags"] = data.tags;

    j["map"]["width"] = data.gridWidth;
    j["map"]["height"] = data.gridHeight;
    j["map"]["cellSize"] = data.cellSize;
    j["map"]["offsetX"] = data.offsetX;
    j["map"]["offsetY"] = data.offsetY;
    j["map"]["layout"] = data.layout;

    json spawnersJson = json::array();
    for (const auto& spawner : data.spawners) {
        spawnersJson.push_back({
            { "x", spawner.pos.x },
            { "y", spawner.pos.y },
            { "targetBaseIndex", spawner.targetBaseIndex }
        });
    }
    j["map"]["spawners"] = spawnersJson;

    json basesJson = json::array();
    for (const auto& base : data.bases) {
        basesJson.push_back({
            { "x", base.x },
            { "y", base.y },
            { "id", base.id }
        });
    }
    j["map"]["bases"] = basesJson;

    // Сохраняем вагонетки
    if (!data.minecarts.empty()) {
        json minecartsJson = json::array();
        for (const auto& cart : data.minecarts) {
            minecartsJson.push_back({
                { "start", { { "x", cart.start.x }, { "y", cart.start.y } } },
                { "end", { { "x", cart.end.x }, { "y", cart.end.y } } },
                { "interval", cart.interval },
                { "warningTime", cart.warningTime },
                { "speed", cart.speed }
            });
        }
        j["map"]["minecarts"] = minecartsJson;
    } else if (j["map"].contains("minecarts")) {
        j["map"].erase("minecarts");
    }

    // Сохраняем волны
    if (!data.waves.empty()) {
        json wavesJson = json::array();
        for (const auto& wave : data.waves) {
            json partsJson = json::array();
            for (const auto& part : wave.parts) {
                partsJson.push_back({
                    { "type", part.type },
                    { "count", part.count },
                    { "interval", part.spawnInterwal },
                    { "delayAfter", part.delayAfter }
                });
            }
            wavesJson.push_back({
                { "parts", partsJson }
            });
        }
        j["waves"] = wavesJson;
    } else if (!j.contains("waves") || !j["waves"].is_array() || j["waves"].empty()) {
        // Если нет секции waves, добавим базовую волну для тестирования
        j["waves"] = json::array({
            {
                { "parts", json::array({
                    {
                        { "type", "Basic" },
                        { "count", 10 },
                        { "interval", 1.0f },
                        { "delayAfter", 2.0f }
                    }
                }) }
            }
        });
    }

    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        std::cerr << "ERROR::LEVELMANAGER: Could not open level file for writing: " << filepath << std::endl;
        return false;
    }

    outFile << j.dump(2) << std::endl;
    outFile.close();
    std::cout << "SUCCESS::LEVELMANAGER: Level map saved to " << filepath << std::endl;
    return true;
}

std::string LevelManager::sanitizeLevelFileName(const std::string& name) {
    if (name.empty()) return "custom_map.json";

    std::string clean = fs::path(name).filename().string();
    if (clean.length() >= 5 && clean.substr(clean.length() - 5) == ".json") {
        clean = clean.substr(0, clean.length() - 5);
    }

    std::string result;
    for (char c : clean) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            result += c;
        } else if (c == ' ') {
            result += '_';
        }
    }

    if (result.empty()) {
        result = "custom_map";
    }

    return result + ".json";
}

std::vector<std::string> LevelManager::getLevelDirectories() {
    std::vector<std::string> candidates = {
        "res/levels",
        "../../res/levels",
        "../res/levels",
        "../../../res/levels",
        "out/build/x64-Debug/res/levels",
        "out/build/x64-Release/res/levels",
        "build/res/levels"
    };

    std::vector<std::string> validDirs;
    std::vector<std::string> canonicalPaths;

    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c, ec) && fs::is_directory(c, ec)) {
            std::string can = fs::canonical(c, ec).string();
            if (!ec) {
                if (std::find(canonicalPaths.begin(), canonicalPaths.end(), can) == canonicalPaths.end()) {
                    canonicalPaths.push_back(can);
                    validDirs.push_back(c);
                }
            } else {
                validDirs.push_back(c);
            }
        }
    }

    if (validDirs.empty()) {
        validDirs.push_back("res/levels");
    }

    return validDirs;
}

void LevelManager::syncLevelsBetweenSourceAndBuild() {
    auto dirs = getLevelDirectories();
    if (dirs.size() < 2) return;

    for (size_t i = 0; i < dirs.size(); ++i) {
        for (size_t j = i + 1; j < dirs.size(); ++j) {
            std::error_code ec;
            if (!fs::exists(dirs[i], ec) || !fs::exists(dirs[j], ec)) continue;

            // Синхронизируем из dirs[i] в dirs[j]
            for (const auto& entry : fs::directory_iterator(dirs[i], ec)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    std::string fname = entry.path().filename().string();
                    if (fname == "textures.json") continue;

                    fs::path target = fs::path(dirs[j]) / fname;
                    if (!fs::exists(target, ec)) {
                        fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
                    } else {
                        auto t1 = fs::last_write_time(entry.path(), ec);
                        auto t2 = fs::last_write_time(target, ec);
                        if (t1 > t2) {
                            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
                        } else if (t2 > t1) {
                            fs::copy_file(target, entry.path(), fs::copy_options::overwrite_existing, ec);
                        }
                    }
                }
            }

            // Синхронизируем из dirs[j] в dirs[i]
            for (const auto& entry : fs::directory_iterator(dirs[j], ec)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    std::string fname = entry.path().filename().string();
                    if (fname == "textures.json") continue;

                    fs::path target = fs::path(dirs[i]) / fname;
                    if (!fs::exists(target, ec)) {
                        fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
                    }
                }
            }
        }
    }
}

std::vector<LevelInfo> LevelManager::getAvailableLevels() {
    syncLevelsBetweenSourceAndBuild();

    std::vector<LevelInfo> levels;
    std::unordered_set<std::string> seen;

    std::vector<std::string> builtIns = { "level_1.json", "level_2.json" };
    auto dirs = getLevelDirectories();

    for (const auto& bi : builtIns) {
        for (const auto& dir : dirs) {
            fs::path p = fs::path(dir) / bi;
            std::error_code ec;
            if (fs::exists(p, ec)) {
                LevelMapData mapData = loadLevelMap(p.string());
                LevelInfo info;
                info.filename = bi;
                if (!mapData.name.empty()) {
                    info.name = mapData.name;
                } else {
                    info.name = (bi == "level_1.json") ? "LEVEL 1" : "LEVEL 2";
                }
                info.fullPath = "res/levels/" + bi;
                info.isCampaign = mapData.isCampaign;
                info.isBuiltIn = mapData.isCampaign;
                info.tags = mapData.tags;
                levels.push_back(info);
                seen.insert(bi);
                break;
            }
        }
    }

    for (const auto& dir : dirs) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string fname = entry.path().filename().string();
                if (fname == "textures.json" || fname == "settings.json" || fname == "campaign.json") continue;
                if (seen.find(fname) != seen.end()) continue;

                LevelMapData mapData = loadLevelMap(entry.path().string());
                if (mapData.gridWidth <= 0 || mapData.gridHeight <= 0) continue;

                LevelInfo info;
                info.filename = fname;
                std::string stem = entry.path().stem().string();
                if (!mapData.name.empty()) {
                    info.name = mapData.name;
                } else if (stem == "level_editor") {
                    info.name = "MY MAP";
                } else {
                    info.name = stem;
                }
                info.fullPath = entry.path().string();
                info.isCampaign = mapData.isCampaign;
                info.isBuiltIn = false;
                info.tags = mapData.tags;
                levels.push_back(info);
                seen.insert(fname);
            }
        }
    }

    return levels;
}

std::vector<LevelInfo> LevelManager::getCustomLevels() {
    auto all = getAvailableLevels();
    std::vector<LevelInfo> custom;
    for (const auto& lvl : all) {
        if (!CampaignManager::isFileInCampaign(lvl.filename)) {
            custom.push_back(lvl);
        }
    }
    return custom;
}

bool LevelManager::saveLevel(const std::string& levelFileName, const LevelMapData& data) {
    std::string cleanName = sanitizeLevelFileName(levelFileName);
    auto dirs = getLevelDirectories();
    bool anySaved = false;

    for (const auto& dir : dirs) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        fs::path p = fs::path(dir) / cleanName;
        if (saveLevelMap(p.string(), data)) {
            anySaved = true;
        }
    }

    syncLevelsBetweenSourceAndBuild();
    return anySaved;
}

bool LevelManager::renameLevel(const std::string& oldFileName, const std::string& newFileName) {
    std::string oldClean = sanitizeLevelFileName(oldFileName);
    std::string newClean = sanitizeLevelFileName(newFileName);

    if (oldClean == newClean) return true;

    auto dirs = getLevelDirectories();
    bool anyRenamed = false;

    for (const auto& dir : dirs) {
        fs::path oldP = fs::path(dir) / oldClean;
        fs::path newP = fs::path(dir) / newClean;
        std::error_code ec;
        if (fs::exists(oldP, ec)) {
            fs::rename(oldP, newP, ec);
            if (!ec) anyRenamed = true;
        }
    }

    syncLevelsBetweenSourceAndBuild();
    return anyRenamed;
}

bool LevelManager::setLevelCampaign(const std::string& levelFileName, bool isCampaign) {
    std::string clean = sanitizeLevelFileName(levelFileName);
    auto dirs = getLevelDirectories();
    for (const auto& dir : dirs) {
        fs::path p = fs::path(dir) / clean;
        std::error_code ec;
        if (fs::exists(p, ec)) {
            LevelMapData data = loadLevelMap(p.string());
            if (data.gridWidth > 0 && data.gridHeight > 0) {
                data.isCampaign = isCampaign;
                return saveLevel(clean, data);
            }
        }
    }
    return false;
}

bool LevelManager::setLevelTags(const std::string& levelFileName, const std::vector<std::string>& tags) {
    std::string clean = sanitizeLevelFileName(levelFileName);
    auto dirs = getLevelDirectories();
    for (const auto& dir : dirs) {
        fs::path p = fs::path(dir) / clean;
        std::error_code ec;
        if (fs::exists(p, ec)) {
            LevelMapData data = loadLevelMap(p.string());
            if (data.gridWidth > 0 && data.gridHeight > 0) {
                data.tags = tags;
                return saveLevel(clean, data);
            }
        }
    }
    return false;
}

bool LevelManager::setLevelDisplayName(const std::string& levelFileName, const std::string& displayName) {
    std::string clean = sanitizeLevelFileName(levelFileName);
    auto dirs = getLevelDirectories();
    for (const auto& dir : dirs) {
        fs::path p = fs::path(dir) / clean;
        std::error_code ec;
        if (fs::exists(p, ec)) {
            LevelMapData data = loadLevelMap(p.string());
            if (data.gridWidth > 0 && data.gridHeight > 0) {
                data.name = displayName;
                return saveLevel(clean, data);
            }
        }
    }
    return false;
}

bool LevelManager::deleteLevel(const std::string& levelFileName) {
    std::string clean = sanitizeLevelFileName(levelFileName);
    if (CampaignManager::isFileInCampaign(clean)) return false;

    auto dirs = getLevelDirectories();
    bool anyDeleted = false;

    for (const auto& dir : dirs) {
        fs::path p = fs::path(dir) / clean;
        std::error_code ec;
        if (fs::exists(p, ec)) {
            fs::remove(p, ec);
            if (!ec) anyDeleted = true;
        }
    }

    syncLevelsBetweenSourceAndBuild();
    return anyDeleted;
}

std::string LevelManager::createNewLevel(const std::string& displayName, bool isCampaign) {
    std::string cleanBase = InputManager::transliterateToAscii(displayName);
    if (cleanBase.length() >= 5 && cleanBase.substr(cleanBase.length() - 5) == ".json") {
        cleanBase = cleanBase.substr(0, cleanBase.length() - 5);
    }
    if (cleanBase.empty()) cleanBase = "custom_map";

    auto dirs = getLevelDirectories();
    auto existsAcrossDirs = [&](const std::string& fname) -> bool {
        for (const auto& dir : dirs) {
            fs::path p = fs::path(dir) / fname;
            std::error_code ec;
            if (fs::exists(p, ec)) return true;
        }
        return false;
    };

    std::string candidate = cleanBase + ".json";
    int counter = 1;
    while (existsAcrossDirs(candidate)) {
        candidate = cleanBase + "_" + std::to_string(counter++) + ".json";
    }

    LevelMapData newMap;
    newMap.name = displayName.empty() ? "Новая карта" : displayName;
    newMap.isCampaign = isCampaign;
    if (isCampaign) {
        newMap.tags = { "Кампания" };
    } else {
        newMap.tags = { "Тест" };
    }

    newMap.gridWidth = 20;
    newMap.gridHeight = 12;
    newMap.cellSize = 64.0f;
    newMap.offsetX = 20.0f;
    newMap.offsetY = 20.0f;
    newMap.layout.assign(12, std::vector<int>(20, 0));

    newMap.bases.push_back(BaseData(17, 6, 0));
    newMap.layout[6][17] = 0;

    SpawnerData sp;
    sp.pos = glm::ivec2(2, 6);
    sp.targetBaseIndex = 0;
    newMap.spawners.push_back(sp);
    newMap.layout[6][2] = 0;

    WaveConfig wave;
    wave.parts.push_back({ "Basic", 10, 0.8f, 2.0f });
    newMap.waves.push_back(wave);

    saveLevel(candidate, newMap);
    return candidate;
}