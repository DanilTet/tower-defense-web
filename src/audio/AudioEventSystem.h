#pragma once
#include <string>

class AudioEventSystem {
public:
    static void init();
    static void playThemeMusic(const std::string& musicPath = "res/sounds/background.mp3");
    static void cleanup();
};

