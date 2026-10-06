#include "SettingsManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

float SettingsManager::s_masterVolume = 0.8f;
bool SettingsManager::s_isMuted = false;
std::string SettingsManager::s_language = "ru";
int SettingsManager::s_uiScalePercent = 100;
bool SettingsManager::s_loaded = false;

static std::vector<std::string> getSettingsDirectories() {
    std::vector<std::string> dirs;
    std::vector<std::string> candidates = {
        "res",
        "../res",
        "../../res",
        "../../../res",
        "out/build/x64-Debug/res",
        "out/build/x64-Release/res"
    };

    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c, ec) && fs::is_directory(c, ec)) {
            std::string canonical = fs::weakly_canonical(c, ec).string();
            if (std::find(dirs.begin(), dirs.end(), canonical) == dirs.end()) {
                dirs.push_back(canonical);
            }
        }
    }

    if (dirs.empty()) {
        dirs.push_back("res");
    }
    return dirs;
}

void SettingsManager::init() {
    load();
}

void SettingsManager::load() {
    if (s_loaded) return;

    auto dirs = getSettingsDirectories();
    for (const auto& d : dirs) {
        fs::path p = fs::path(d) / "settings.json";
        std::error_code ec;
        if (fs::exists(p, ec)) {
            std::ifstream file(p);
            if (file.is_open()) {
                try {
                    json j;
                    file >> j;
                    s_masterVolume = j.value("masterVolume", 0.8f);
                    s_isMuted = j.value("isMuted", false);
                    s_language = j.value("language", "ru");
                    s_uiScalePercent = j.value("uiScale", 100);
                    if (s_uiScalePercent < 50 || s_uiScalePercent > 200) {
                        s_uiScalePercent = 100;
                    }
                    s_loaded = true;
                    break;
                } catch (...) {
                    std::cerr << "WARNING::SettingsManager: parse error in " << p << std::endl;
                }
            }
        }
    }
    s_loaded = true;
}

void SettingsManager::save() {
    json j;
    j["masterVolume"] = s_masterVolume;
    j["isMuted"] = s_isMuted;
    j["language"] = s_language;
    j["uiScale"] = s_uiScalePercent;

    auto dirs = getSettingsDirectories();
    for (const auto& d : dirs) {
        std::error_code ec;
        fs::create_directories(d, ec);
        fs::path p = fs::path(d) / "settings.json";
        std::ofstream out(p);
        if (out.is_open()) {
            out << j.dump(4);
            out.close();
        }
    }
}

float SettingsManager::getVolume() {
    return s_masterVolume;
}

void SettingsManager::setVolume(float volume) {
    s_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    save();
}

bool SettingsManager::isMuted() {
    return s_isMuted;
}

void SettingsManager::setMuted(bool muted) {
    s_isMuted = muted;
    save();
}

std::string SettingsManager::getLanguage() {
    return s_language;
}

void SettingsManager::setLanguage(const std::string& lang) {
    if (lang == "ru" || lang == "ua" || lang == "en") {
        s_language = lang;
        save();
    }
}

int SettingsManager::getUIScalePercent() {
    return s_uiScalePercent;
}

void SettingsManager::setUIScalePercent(int percent) {
    s_uiScalePercent = std::clamp(percent, 50, 200);
    save();
}

float SettingsManager::getUIScaleMultiplier() {
    return static_cast<float>(s_uiScalePercent) / 100.0f;
}

