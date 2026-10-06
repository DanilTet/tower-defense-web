#pragma once
#include <string>

enum class GameLanguage {
    Russian,
    Ukrainian,
    English
};

class LocalizationManager {
public:
    static void init();
    static void setLanguage(GameLanguage lang);
    static void setLanguageByCode(const std::string& code);
    static GameLanguage getLanguage();
    static std::string getLanguageCode();

    // Получить локализованную строку по ключу
    static std::string get(const std::string& key);

    // Удобный хелпер-сокращение
    static inline std::string tr(const std::string& key) {
        return get(key);
    }
};

// Макрос для быстрого использования в коде
#define LOC(key) LocalizationManager::tr(key)

