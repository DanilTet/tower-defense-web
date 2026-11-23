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
        data.background = j.value("background", "default");
        if (data.background.empty() && j.contains("map") && j["map"].contains("background")) {
            data.background = j["map"].value("background", "default");
        }
        if (data.background.empty()) {
            data.background = "default";
        }
        data.hasSidePockets = j.value("hasSidePockets", false);
        if (!data.hasSidePockets && j.contains("map") && j["map"].contains("hasSidePockets")) {
            data.hasSidePockets = j["map"].value("hasSidePockets", false);
        }
        std::string fname = fs::path(filepath).filename().string();
        bool defaultCampaign = (fname == "kanal.json" || fname == "rtutnaya_shchakhta.json");
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

        data.startingMoney = 50;
        if (j.contains("startingMoney") && j["startingMoney"].is_number_integer()) {
            data.startingMoney = j["startingMoney"].get<int>();
        } else if (j.contains("map") && j["map"].contains("startingMoney") && j["map"]["startingMoney"].is_number_integer()) {
            data.startingMoney = j["map"]["startingMoney"].get<int>();
        }

        data.startingHealth = 20;
        if (j.contains("startingHealth") && j["startingHealth"].is_number_integer()) {
            data.startingHealth = j["startingHealth"].get<int>();
        } else if (j.contains("map") && j["map"].contains("startingHealth") && j["map"]["startingHealth"].is_number_integer()) {
            data.startingHealth = j["map"]["startingHealth"].get<int>();
        }

        data.allowedTowers = { "Basic", "Mercury", "Piston" };
        if (j.contains("allowedTowers") && j["allowedTowers"].is_array()) {
            data.allowedTowers.clear();
            for (const auto& item : j["allowedTowers"]) {
                if (item.is_string()) {
                    data.allowedTowers.push_back(item.get<std::string>());
                }
            }
            if (data.allowedTowers.empty()) {
                data.allowedTowers = { "Basic", "Mercury", "Piston" };
            }
        } else if (j.contains("map") && j["map"].contains("allowedTowers") && j["map"]["allowedTowers"].is_array()) {
            data.allowedTowers.clear();
            for (const auto& item : j["map"]["allowedTowers"]) {
                if (item.is_string()) {
                    data.allowedTowers.push_back(item.get<std::string>());
                }
            }
            if (data.allowedTowers.empty()) {
                data.allowedTowers = { "Basic", "Mercury", "Piston" };
            }
        }

        data.maxUpgradeTier = 3;
        if (j.contains("maxUpgradeTier") && j["maxUpgradeTier"].is_number_integer()) {
            data.maxUpgradeTier = std::clamp(j["maxUpgradeTier"].get<int>(), 1, 3);
        } else if (j.contains("map") && j["map"].contains("maxUpgradeTier") && j["map"]["maxUpgradeTier"].is_number_integer()) {
            data.maxUpgradeTier = std::clamp(j["map"]["maxUpgradeTier"].get<int>(), 1, 3);
        }

        data.towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
        if (j.contains("towerMaxTiers") && j["towerMaxTiers"].is_object()) {
            for (auto& [tName, val] : j["towerMaxTiers"].items()) {
                if (val.is_number_integer()) {
                    data.towerMaxTiers[tName] = std::clamp(val.get<int>(), 1, 3);
                }
            }
        } else if (j.contains("map") && j["map"].contains("towerMaxTiers") && j["map"]["towerMaxTiers"].is_object()) {
            for (auto& [tName, val] : j["map"]["towerMaxTiers"].items()) {
                if (val.is_number_integer()) {
                    data.towerMaxTiers[tName] = std::clamp(val.get<int>(), 1, 3);
                }
            }
        } else {
            for (const auto& t : data.allowedTowers) {
                data.towerMaxTiers[t] = std::clamp(data.maxUpgradeTier, 1, 3);
            }
        }

        // Экономика врагов (Правила наград по волнам)
        bool isKanal = (data.name == "Канал" || filepath.find("kanal") != std::string::npos);
        if (isKanal) {
            data.enemyBountyRules = {
                { "Coal", {
                    { 1, 3, 1, 1 },
                    { 4, 11, 1, 2 },
                    { 12, 15, 1, 1 }
                }},
                { "Spark", {
                    { 1, 99, 1, 1 }
                }},
                { "Mercury", {
                    { 1, 99, 1, 1 }
                }}
            };
        } else {
            data.enemyBountyRules = {
                { "Coal",    { { 1, 99, 1, 1 } } },
                { "Spark",   { { 1, 99, 1, 1 } } },
                { "Mercury", { { 1, 99, 1, 1 } } }
            };
        }

        auto parseBounties = [&data](const json& bj) {
            for (auto& [eType, bVal] : bj.items()) {
                if (bVal.is_array()) {
                    std::vector<EnemyWaveBountyRule> rules;
                    for (const auto& item : bVal) {
                        if (!item.is_object()) continue;
                        EnemyWaveBountyRule r;
                        if (item.contains("fromWave")) r.fromWave = item["fromWave"].get<int>();
                        else if (item.contains("from_wave")) r.fromWave = item["from_wave"].get<int>();
                        else if (item.contains("from")) r.fromWave = item["from"].get<int>();
                        else r.fromWave = 1;

                        if (item.contains("toWave")) r.toWave = item["toWave"].get<int>();
                        else if (item.contains("to_wave")) r.toWave = item["to_wave"].get<int>();
                        else if (item.contains("to")) r.toWave = item["to"].get<int>();
                        else r.toWave = 99;

                        r.reward = item.value("reward", 1);
                        if (item.contains("reward_step")) r.rewardStep = item.value("reward_step", 1);
                        else if (item.contains("rewardStep")) r.rewardStep = item.value("rewardStep", 1);
                        else if (item.contains("step")) r.rewardStep = item.value("step", 1);
                        else r.rewardStep = 1;

                        if (r.fromWave < 1) r.fromWave = 1;
                        if (r.toWave < r.fromWave) r.toWave = r.fromWave;
                        if (r.reward < 0) r.reward = 0;
                        if (r.rewardStep < 1) r.rewardStep = 1;

                        rules.push_back(r);
                    }
                    if (!rules.empty()) {
                        data.enemyBountyRules[eType] = rules;
                    }
                } else if (bVal.is_object()) {
                    EnemyWaveBountyRule r;
                    r.fromWave = 1;
                    r.toWave = 99;
                    r.reward = bVal.value("reward", 1);
                    if (bVal.contains("reward_step")) r.rewardStep = bVal.value("reward_step", 1);
                    else if (bVal.contains("rewardStep")) r.rewardStep = bVal.value("rewardStep", 1);
                    else if (bVal.contains("step")) r.rewardStep = bVal.value("step", 1);
                    else r.rewardStep = 1;
                    if (r.reward < 0) r.reward = 0;
                    if (r.rewardStep < 1) r.rewardStep = 1;

                    data.enemyBountyRules[eType] = { r };
                }
            }
        };

        if (j.contains("enemyBountyRules") && j["enemyBountyRules"].is_object()) {
            parseBounties(j["enemyBountyRules"]);
        } else if (j.contains("map") && j["map"].contains("enemyBountyRules") && j["map"]["enemyBountyRules"].is_object()) {
            parseBounties(j["map"]["enemyBountyRules"]);
        } else if (j.contains("enemyGlobalBounties") && j["enemyGlobalBounties"].is_object()) {
            parseBounties(j["enemyGlobalBounties"]);
        } else if (j.contains("map") && j["map"].contains("enemyGlobalBounties") && j["map"]["enemyGlobalBounties"].is_object()) {
            parseBounties(j["map"]["enemyGlobalBounties"]);
        }

        // Для обратной совместимости
        data.enemyGlobalBounties.clear();
        for (const auto& [eType, rules] : data.enemyBountyRules) {
            if (!rules.empty()) {
                data.enemyGlobalBounties[eType] = { rules.front().reward, rules.front().rewardStep };
            }
        }

        // Правила прогрессии HP врагов по диапазонам волн (опционально, по умолчанию пусто -> базовое HP из enemies.json)
        data.enemyHpRules.clear();
        auto parseHpRules = [&data](const json& hj) {
            for (auto& [eType, hVal] : hj.items()) {
                if (hVal.is_array()) {
                    std::vector<EnemyHpRule> rules;
                    for (const auto& item : hVal) {
                        if (!item.is_object()) continue;
                        EnemyHpRule r;
                        if (item.contains("fromWave")) {
                            r.fromWave = item["fromWave"].get<int>();
                        } else if (item.contains("from_wave")) {
                            r.fromWave = item["from_wave"].get<int>();
                        } else if (item.contains("from")) {
                            r.fromWave = item["from"].get<int>();
                        } else {
                            r.fromWave = 1;
                        }

                        if (item.contains("toWave")) {
                            r.toWave = item["toWave"].get<int>();
                        } else if (item.contains("to_wave")) {
                            r.toWave = item["to_wave"].get<int>();
                        } else if (item.contains("to")) {
                            r.toWave = item["to"].get<int>();
                        } else {
                            r.toWave = 99;
                        }

                        r.hp = item.value("hp", 0);

                        if (r.fromWave < 1) r.fromWave = 1;
                        if (r.toWave < r.fromWave) r.toWave = r.fromWave;
                        if (r.hp >= 1) {
                            rules.push_back(r);
                        }
                    }
                    if (!rules.empty()) {
                        data.enemyHpRules[eType] = rules;
                    }
                }
            }
        };

        if (j.contains("enemyHpRules") && j["enemyHpRules"].is_object()) {
            parseHpRules(j["enemyHpRules"]);
        } else if (j.contains("map") && j["map"].contains("enemyHpRules") && j["map"]["enemyHpRules"].is_object()) {
            parseHpRules(j["map"]["enemyHpRules"]);
        }

        // Читаем настройки камеры (Camera Viewport)
        bool hadExplicitTileCoord = false;
        auto parseCamera = [&data, &hadExplicitTileCoord](const json& cam) {
            float tx = -1.0f;
            float ty = -1.0f;
            if (cam.contains("tileX")) {
                tx = cam["tileX"].get<float>();
                hadExplicitTileCoord = true;
            } else if (cam.contains("x")) {
                tx = cam["x"].get<float>();
            }

            if (cam.contains("tileY")) {
                ty = cam["tileY"].get<float>();
                hadExplicitTileCoord = true;
            } else if (cam.contains("y")) {
                ty = cam["y"].get<float>();
            }

            data.camera.targetTile = glm::vec2(tx, ty);
            data.camera.zoom = cam.value("zoom", 1.0f);
            data.camera.isCustom = cam.value("isCustom", (tx >= -100.0f && ty >= -100.0f && (tx >= 0.0f || ty >= 0.0f)));
            data.camera.viewTilesW = cam.value("viewTilesW", 0.0f);
            data.camera.viewTilesH = cam.value("viewTilesH", 0.0f);
        };

        if (j.contains("camera") && j["camera"].is_object()) {
            parseCamera(j["camera"]);
        } else if (j.contains("map") && j["map"].contains("camera") && j["map"]["camera"].is_object()) {
            parseCamera(j["map"]["camera"]);
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
                sd.groupId = spawner.value("groupId", spawner.value("group_id", sd.targetBaseIndex));
                sd.visibleInGame = spawner.value("visibleInGame", false);
                sd.enableFadeIn = spawner.value("enableFadeIn", true);
                data.spawners.push_back(sd);
            }

            // читаем базы
            for (const auto& base : j["map"]["bases"]) {
                BaseData bd;
                bd.x = base["x"];
                bd.y = base["y"];
                bd.id = base.value("id", 0);
                bd.visibleInGame = base.value("visibleInGame", false);
                bd.enableFadeOut = base.value("enableFadeOut", true);
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

            // no-build mask (backward compat if missing: all false)
            const json* nbmJson = nullptr;
            if (j["map"].contains("noBuildMask")) {
                nbmJson = &j["map"]["noBuildMask"];
            } else if (j.contains("noBuildMask")) {
                nbmJson = &j["noBuildMask"];
            } else if (j["map"].contains("blockedTiles")) {
                nbmJson = &j["map"]["blockedTiles"];
            } else if (j.contains("blockedTiles")) {
                nbmJson = &j["blockedTiles"];
            } else if (j["map"].contains("forbiddenCells")) {
                nbmJson = &j["map"]["forbiddenCells"];
            } else if (j.contains("forbiddenCells")) {
                nbmJson = &j["forbiddenCells"];
            }

            if (nbmJson && nbmJson->is_array()) {
                for (const auto& row : *nbmJson) {
                    std::vector<bool> rowData;
                    if (row.is_array()) {
                        for (const auto& cell : row) {
                            bool val = false;
                            if (cell.is_boolean()) val = cell.get<bool>();
                            else if (cell.is_number()) val = (cell.get<int>() != 0);
                            rowData.push_back(val);
                        }
                    }
                    data.noBuildMask.push_back(rowData);
                }
            }
            data.noBuildMask.resize(
                static_cast<size_t>(data.gridHeight),
                std::vector<bool>(static_cast<size_t>(data.gridWidth), false));
            for (auto& row : data.noBuildMask) {
                row.resize(static_cast<size_t>(data.gridWidth), false);
            }

            const json* outerCellsJson = nullptr;
            if (j["map"].contains("outerCells") && j["map"]["outerCells"].is_array()) {
                outerCellsJson = &j["map"]["outerCells"];
            } else if (j.contains("outerCells") && j["outerCells"].is_array()) {
                outerCellsJson = &j["outerCells"];
            }
            if (outerCellsJson) {
                for (const auto& ocNode : *outerCellsJson) {
                    LevelMapData::OuterCellData oc;
                    oc.x = ocNode.value("x", 0);
                    oc.y = ocNode.value("y", 0);
                    oc.type = ocNode.value("type", 0);
                    oc.noBuild = ocNode.value("noBuild", false);
                    data.outerCells.push_back(oc);
                }
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

            // Читаем эмиттеры частиц ("emitters")
            const json* emittersJson = nullptr;
            if (j["map"].contains("emitters") && j["map"]["emitters"].is_array()) {
                emittersJson = &j["map"]["emitters"];
            } else if (j.contains("emitters") && j["emitters"].is_array()) {
                emittersJson = &j["emitters"];
            }

            if (emittersJson) {
                for (const auto& emNode : *emittersJson) {
                    ParticleEmitterConfig em;
                    em.id = emNode.value("id", 0);
                    em.type = emNode.value("type", "steam_jet");
                    if (emNode.contains("tilePos")) {
                        const auto& tp = emNode["tilePos"];
                        if (tp.is_array() && tp.size() >= 2) {
                            em.tilePos = glm::vec2(tp[0].get<float>(), tp[1].get<float>());
                        } else if (tp.is_object()) {
                            em.tilePos = glm::vec2(tp.value("x", 0.0f), tp.value("y", 0.0f));
                        }
                    } else {
                        em.tilePos = glm::vec2(emNode.value("x", 0.0f), emNode.value("y", 0.0f));
                    }
                    em.angleDeg = emNode.value("angleDeg", 90.0f);
                    em.speed = emNode.value("speed", 180.0f);
                    em.particleScale = emNode.value("particleScale", 1.0f);
                    em.periodMin = emNode.value("periodMin", 5.0f);
                    em.periodMax = emNode.value("periodMax", 12.0f);
                    em.burstDuration = emNode.value("burstDuration", 2.2f);
                    em.loopContinuous = emNode.value("loop", emNode.value("loopContinuous", false));
                    em.zIndex = emNode.value("zIndex", 0);
                    data.emitters.push_back(em);
                }
                std::stable_sort(data.emitters.begin(), data.emitters.end(), [](const ParticleEmitterConfig& a, const ParticleEmitterConfig& b) {
                    return a.zIndex < b.zIndex;
                });
            }

            // Читаем свободные декорации ("decorations")
            const json* decorJson = nullptr;
            if (j["map"].contains("decorations") && j["map"]["decorations"].is_array()) {
                decorJson = &j["map"]["decorations"];
            } else if (j.contains("decorations") && j["decorations"].is_array()) {
                decorJson = &j["decorations"];
            }

            if (decorJson) {
                float cs = (data.cellSize > 0.001f) ? data.cellSize : 64.0f;
                for (const auto& decNode : *decorJson) {
                    DecorationConfig dec;
                    dec.id = decNode.value("id", 0);
                    dec.type = decNode.value("type", "bush");
                    if (decNode.contains("tilePos")) {
                        const auto& tp = decNode["tilePos"];
                        if (tp.is_array() && tp.size() >= 2) {
                            dec.tilePos = glm::vec2(tp[0].get<float>(), tp[1].get<float>());
                        } else if (tp.is_object()) {
                            dec.tilePos = glm::vec2(tp.value("x", 0.0f), tp.value("y", 0.0f));
                        }
                    } else if (decNode.contains("tileX") || decNode.contains("tileY")) {
                        dec.tilePos = glm::vec2(decNode.value("tileX", 0.0f), decNode.value("tileY", 0.0f));
                    } else if (decNode.contains("worldPos")) {
                        const auto& wp = decNode["worldPos"];
                        glm::vec2 rawPos(0.0f);
                        if (wp.is_array() && wp.size() >= 2) {
                            rawPos = glm::vec2(wp[0].get<float>(), wp[1].get<float>());
                        } else if (wp.is_object()) {
                            rawPos = glm::vec2(wp.value("x", 0.0f), wp.value("y", 0.0f));
                        }
                        // Обратная совместимость: если сохранено в абсолютных мировых координатах, конвертируем в tilePos
                        dec.tilePos = (rawPos - glm::vec2(data.offsetX, data.offsetY)) / cs;
                    } else if (decNode.contains("pos")) {
                        const auto& wp = decNode["pos"];
                        glm::vec2 rawPos(0.0f);
                        if (wp.is_array() && wp.size() >= 2) {
                            rawPos = glm::vec2(wp[0].get<float>(), wp[1].get<float>());
                        } else if (wp.is_object()) {
                            rawPos = glm::vec2(wp.value("x", 0.0f), wp.value("y", 0.0f));
                        }
                        dec.tilePos = (rawPos - glm::vec2(data.offsetX, data.offsetY)) / cs;
                    } else {
                        glm::vec2 rawPos = glm::vec2(decNode.value("x", 0.0f), decNode.value("y", 0.0f));
                        dec.tilePos = (rawPos - glm::vec2(data.offsetX, data.offsetY)) / cs;
                    }
                    dec.worldPos = glm::vec2(data.offsetX, data.offsetY) + dec.tilePos * cs;
                    dec.scale = decNode.value("scale", 1.0f);
                    dec.scaleX = decNode.value("scaleX", 1.0f);
                    dec.scaleY = decNode.value("scaleY", 1.0f);
                    dec.opacity = decNode.value("opacity", 0.5f);
                    dec.rotation = decNode.value("rotation", 0.0f);
                    dec.destructible = decNode.value("destructible", true);
                    dec.zIndex = decNode.value("zIndex", 0);
                    data.decorations.push_back(dec);
                }
                std::stable_sort(data.decorations.begin(), data.decorations.end(), [](const DecorationConfig& a, const DecorationConfig& b) {
                    return a.zIndex < b.zIndex;
                });
            }

            // Обратная совместимость для старых карт, где x/y были сохранены в пикселях (только если не было явного tileX/tileY и координаты явно в пикселях > 50px)
            if (data.camera.isCustom && !hadExplicitTileCoord) {
                if ((data.camera.targetTile.x > static_cast<float>(data.gridWidth) * 2.0f && data.camera.targetTile.x > 50.0f) ||
                    (data.camera.targetTile.y > static_cast<float>(data.gridHeight) * 2.0f && data.camera.targetTile.y > 50.0f)) {
                    float cs = (data.cellSize > 0.001f) ? data.cellSize : 64.0f;
                    data.camera.targetTile.x = (data.camera.targetTile.x - data.offsetX) / cs;
                    data.camera.targetTile.y = (data.camera.targetTile.y - data.offsetY) / cs;
                }
            }

            // Вычисляем viewTilesW и viewTilesH, если они не были сохранены явно
            if (data.camera.isCustom) {
                if (data.camera.viewTilesW <= 0.01f || data.camera.viewTilesH <= 0.01f) {
                    float cs = (data.cellSize > 0.001f) ? data.cellSize : 64.0f;
                    float z = (data.camera.zoom > 0.001f) ? data.camera.zoom : 1.0f;
                    data.camera.viewTilesW = (1280.0f / z) / cs;
                    data.camera.viewTilesH = (720.0f / z) / cs;
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
                        if (partJson.contains("spawner_id")) {
                            part.spawnerId = partJson.value("spawner_id", -1);
                        } else if (partJson.contains("spawnerId")) {
                            part.spawnerId = partJson.value("spawnerId", -1);
                        } else {
                            part.spawnerId = -1;
                        }
                        part.reward = partJson.value("reward", -1);
                        if (partJson.contains("reward_step")) {
                            part.rewardStep = partJson.value("reward_step", -1);
                        } else if (partJson.contains("rewardStep")) {
                            part.rewardStep = partJson.value("rewardStep", -1);
                        } else {
                            part.rewardStep = -1;
                        }
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
    j["background"] = data.background.empty() ? "default" : data.background;
    j["hasSidePockets"] = data.hasSidePockets;

    if (data.camera.isCustom) {
        j["camera"] = {
            { "tileX", data.camera.targetTile.x },
            { "tileY", data.camera.targetTile.y },
            { "x", data.camera.targetTile.x },
            { "y", data.camera.targetTile.y },
            { "zoom", data.camera.zoom },
            { "isCustom", true },
            { "viewTilesW", data.camera.viewTilesW },
            { "viewTilesH", data.camera.viewTilesH }
        };
    } else if (j.contains("camera")) {
        j.erase("camera");
    }

    j["map"]["width"] = data.gridWidth;
    j["map"]["height"] = data.gridHeight;
    j["map"]["cellSize"] = data.cellSize;
    j["map"]["offsetX"] = data.offsetX;
    j["map"]["offsetY"] = data.offsetY;
    j["map"]["layout"] = data.layout;
    // сохраняем маску запрета постройки как 2D массив 0/1
    {
        json nbm = json::array();
        for (int y = 0; y < data.gridHeight; ++y) {
            json jrow = json::array();
            for (int x = 0; x < data.gridWidth; ++x) {
                bool val = (y < (int)data.noBuildMask.size() && x < (int)data.noBuildMask[y].size()) ? data.noBuildMask[y][x] : false;
                jrow.push_back(val ? 1 : 0);
            }
            nbm.push_back(jrow);
        }
        j["map"]["noBuildMask"] = nbm;
    }

    // Сохраняем внешние клетки outerCells
    {
        json ocArr = json::array();
        for (const auto& oc : data.outerCells) {
            ocArr.push_back({
                { "x", oc.x },
                { "y", oc.y },
                { "type", oc.type },
                { "noBuild", oc.noBuild }
            });
        }
        j["map"]["outerCells"] = ocArr;
    }
    j["startingMoney"] = data.startingMoney;
    j["startingHealth"] = data.startingHealth;
    j["allowedTowers"] = data.allowedTowers;
    j["maxUpgradeTier"] = data.maxUpgradeTier;
    j["towerMaxTiers"] = data.towerMaxTiers;
    j["map"]["startingMoney"] = data.startingMoney;
    j["map"]["startingHealth"] = data.startingHealth;
    j["map"]["allowedTowers"] = data.allowedTowers;
    j["map"]["maxUpgradeTier"] = data.maxUpgradeTier;
    j["map"]["towerMaxTiers"] = data.towerMaxTiers;

    json bountiesJson = json::object();
    for (const auto& [eType, rules] : data.enemyBountyRules) {
        json arr = json::array();
        for (const auto& r : rules) {
            arr.push_back({
                { "fromWave", r.fromWave },
                { "toWave", r.toWave },
                { "reward", r.reward },
                { "rewardStep", r.rewardStep }
            });
        }
        bountiesJson[eType] = arr;
    }
    j["enemyBountyRules"] = bountiesJson;
    j["map"]["enemyBountyRules"] = bountiesJson;
    j["enemyGlobalBounties"] = bountiesJson;
    j["map"]["enemyGlobalBounties"] = bountiesJson;

    json hpRulesJson = json::object();
    for (const auto& [eType, rules] : data.enemyHpRules) {
        if (rules.empty()) continue;
        json arr = json::array();
        for (const auto& r : rules) {
            arr.push_back({
                { "fromWave", r.fromWave },
                { "toWave", r.toWave },
                { "hp", r.hp }
            });
        }
        hpRulesJson[eType] = arr;
    }
    if (!hpRulesJson.empty()) {
        j["enemyHpRules"] = hpRulesJson;
        j["map"]["enemyHpRules"] = hpRulesJson;
    }

    json spawnersJson = json::array();
    for (const auto& spawner : data.spawners) {
        spawnersJson.push_back({
            { "x", spawner.pos.x },
            { "y", spawner.pos.y },
            { "targetBaseIndex", spawner.targetBaseIndex },
            { "groupId", spawner.groupId >= 0 ? spawner.groupId : spawner.targetBaseIndex },
            { "visibleInGame", spawner.visibleInGame },
            { "enableFadeIn", spawner.enableFadeIn }
        });
    }
    j["map"]["spawners"] = spawnersJson;

    json basesJson = json::array();
    for (const auto& base : data.bases) {
        basesJson.push_back({
            { "x", base.x },
            { "y", base.y },
            { "id", base.id },
            { "visibleInGame", base.visibleInGame },
            { "enableFadeOut", base.enableFadeOut }
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

    // Сохраняем эмиттеры частиц
    if (!data.emitters.empty()) {
        json emittersJson = json::array();
        for (const auto& em : data.emitters) {
            emittersJson.push_back({
                { "id", em.id },
                { "type", em.type },
                { "tilePos", { { "x", em.tilePos.x }, { "y", em.tilePos.y } } },
                { "angleDeg", em.angleDeg },
                { "speed", em.speed },
                { "particleScale", em.particleScale },
                { "periodMin", em.periodMin },
                { "periodMax", em.periodMax },
                { "burstDuration", em.burstDuration },
                { "loop", em.loopContinuous },
                { "zIndex", em.zIndex }
            });
        }
        j["map"]["emitters"] = emittersJson;
    } else if (j["map"].contains("emitters")) {
        j["map"].erase("emitters");
    }

    // Сохраняем свободные декорации
    if (!data.decorations.empty()) {
        json decsJson = json::array();
        for (const auto& dec : data.decorations) {
            decsJson.push_back({
                { "id", dec.id },
                { "type", dec.type },
                { "tilePos", { { "x", dec.tilePos.x }, { "y", dec.tilePos.y } } },
                { "tileX", dec.tilePos.x },
                { "tileY", dec.tilePos.y },
                { "worldPos", { { "x", dec.worldPos.x }, { "y", dec.worldPos.y } } },
                { "scale", dec.scale },
                { "scaleX", dec.scaleX },
                { "scaleY", dec.scaleY },
                { "opacity", dec.opacity },
                { "rotation", dec.rotation },
                { "destructible", dec.destructible },
                { "zIndex", dec.zIndex }
            });
        }
        j["map"]["decorations"] = decsJson;
    } else if (j["map"].contains("decorations")) {
        j["map"].erase("decorations");
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
                    { "delayAfter", part.delayAfter },
                    { "spawner_id", part.spawnerId },
                    { "reward", part.reward },
                    { "reward_step", part.rewardStep }
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
                        { "delayAfter", 2.0f },
                        { "spawner_id", -1 },
                        { "reward", 1 },
                        { "reward_step", 1 }
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

    const auto& primaryDir = dirs[0];
    std::error_code ec;
    if (!fs::exists(primaryDir, ec)) return;

    for (size_t i = 1; i < dirs.size(); ++i) {
        if (!fs::exists(dirs[i], ec)) continue;

        // 1. Синхронизируем из primaryDir в secondary dir (копируем если отсутствует или обновляем если новее)
        for (const auto& entry : fs::directory_iterator(primaryDir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string fname = entry.path().filename().string();
                if (fname == "textures.json" || fname == "settings.json") continue;

                fs::path target = fs::path(dirs[i]) / fname;
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

        // 2. НЕ воскрешаем удаленные файлы из secondary dirs в primaryDir!
        // Если файл уровня есть в secondary dir, но отсутствует в primaryDir — удаляем его из secondary dir,
        // чтобы удаленные/устаревшие уровни не плодились обратно при каждом запуске.
        for (const auto& entry : fs::directory_iterator(dirs[i], ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string fname = entry.path().filename().string();
                if (fname == "textures.json" || fname == "settings.json" || fname == "campaign.json") continue;

                fs::path primaryFile = fs::path(primaryDir) / fname;
                if (!fs::exists(primaryFile, ec)) {
                    fs::remove(entry.path(), ec);
                }
            }
        }
    }
}

std::vector<LevelInfo> LevelManager::getAvailableLevels() {
    syncLevelsBetweenSourceAndBuild();

    std::vector<LevelInfo> levels;
    std::unordered_set<std::string> seen;

    std::vector<std::string> builtIns = { "kanal.json", "rtutnaya_shchakhta.json" };
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
                    info.name = (bi == "kanal.json") ? "Канал" : "Вход в ртутную шахту";
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
    newMap.noBuildMask.assign(12, std::vector<bool>(20, false));
    newMap.startingMoney = 50;
    newMap.startingHealth = 20;
    newMap.allowedTowers = { "Basic", "Mercury", "Piston" };
    newMap.maxUpgradeTier = 3;
    newMap.towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
    newMap.enemyBountyRules = {
        { "Coal",    { { 1, 99, 1, 1 } } },
        { "Spark",   { { 1, 99, 1, 1 } } },
        { "Mercury", { { 1, 99, 1, 1 } } }
    };
    newMap.enemyGlobalBounties = {
        { "Coal", { 1, 1 } },
        { "Spark", { 1, 1 } },
        { "Mercury", { 1, 1 } }
    };

    newMap.bases.push_back(BaseData(17, 6, 0));
    newMap.layout[6][17] = 0;

    SpawnerData sp;
    sp.pos = glm::ivec2(2, 6);
    sp.targetBaseIndex = 0;
    sp.groupId = 0;
    newMap.spawners.push_back(sp);
    newMap.layout[6][2] = 0;

    WaveConfig wave;
    wave.parts.push_back({ "Basic", 10, 0.8f, 2.0f });
    newMap.waves.push_back(wave);

    saveLevel(candidate, newMap);
    return candidate;
}
