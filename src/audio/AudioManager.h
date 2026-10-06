#pragma once
#include <string>

class AudioManager {
public:
    static bool init(); // инициализация
    static void cleanup(); // выгрузка из памяти музыки

    // функция проигрывания звуков передается путь к файлу и громкость (по умолчанию 1.0)
    static void playSound(const std::string& filepath, float volume = 1.0f);

    // для фоновой музыки
    static void playMusic(const std::string& filepath);
    static void stopMusic();

    // Глобальное управление громкостью и отключением звука (Mute)
    static void setMasterVolume(float volume);
    static float getMasterVolume();
    static float getRawVolume();
    static bool isMuted();
    static void setMuted(bool mute);
    static void toggleMute();
};