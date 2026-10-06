#pragma once
#include <string>

class SettingsManager {
public:
    static void init();
    static void load();
    static void save();

    static float getVolume();
    static void setVolume(float volume);

    static bool isMuted();
    static void setMuted(bool muted);

    static std::string getLanguage();
    static void setLanguage(const std::string& lang);

    static int getUIScalePercent();
    static void setUIScalePercent(int percent);
    static float getUIScaleMultiplier();

private:
    static float s_masterVolume;
    static bool s_isMuted;
    static std::string s_language; // "ru", "ua", "en"
    static int s_uiScalePercent;   // 50, 75, 100, 125, 150
    static bool s_loaded;
};

