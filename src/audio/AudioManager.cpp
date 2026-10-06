#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include "AudioManager.h"
#include "../game/core/SettingsManager.h"
#include <iostream>

// глобальные обьекты аудио движка тут именно для звуков
static ma_engine engine;
static bool isInitialized = false;

// а тут именно для музыки
static ma_sound bgmSound;
static bool isMusicLoaded = false;

// буффер звуков или пул еще назіваеют
const int MAX_SOUNDS = 32; // одновременно может звучать до 32 звуков
static ma_sound sfxPool[MAX_SOUNDS]; // массив звуковых объектов
static bool sfxAllocated[MAX_SOUNDS]; // флаг занят ли слот
static int currentSfxIndex = 0; // текущий слод для нового звука

// глобальная громкость и состояние mute
static float s_masterVolume = 0.8f;
static float s_savedVolume = 0.8f;
static bool s_isMuted = false;

bool AudioManager::init() {
	// инициализация аудиодвижка с настройками по умолчанию
	ma_result result = ma_engine_init(NULL, &engine);

    if (result != MA_SUCCESS) {
        std::cerr << "ERROR::AUDIO: Failed to initialize audio engine." << std::endl;
        return false;
    }

    // обнуляем все слоты в буфере
    for (int i = 0; i < MAX_SOUNDS; i++) {
        sfxAllocated[i] = false;
    }

    SettingsManager::load();
    s_masterVolume = SettingsManager::getVolume();
    s_savedVolume = (s_masterVolume > 0.05f) ? s_masterVolume : 0.8f;
    s_isMuted = SettingsManager::isMuted();

    isInitialized = true;
    ma_engine_set_volume(&engine, s_isMuted ? 0.0f : s_masterVolume);
    std::cout << "Audio Engine initialized successfully!" << std::endl;
    return true;
}

// выгрузка из памяти музыки
void AudioManager::cleanup() {
    if (isMusicLoaded) {
        ma_sound_uninit(&bgmSound);
    }

    // очищаем буффер
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (sfxAllocated[i]) {
            ma_sound_uninit(&sfxPool[i]);
        }
    }

    if (isInitialized) {
        ma_engine_uninit(&engine);
        isInitialized = false;
    }
}

void AudioManager::playSound(const std::string& filepath,  float volume) {
    if (!isInitialized) return;

    // если в слоте есть уже звук то выгружаем его из памяти
    if (sfxAllocated[currentSfxIndex]) {
        ma_sound_uninit(&sfxPool[currentSfxIndex]);
        sfxAllocated[currentSfxIndex] = false;
    }

    // загружаем новый звук в текущий слот буфера
    ma_result result = ma_sound_init_from_file(&engine, filepath.c_str(), 0, NULL, NULL, &sfxPool[currentSfxIndex]);

    if (result == MA_SUCCESS) {
        sfxAllocated[currentSfxIndex] = true;

        // устанавливаем громкость 0.0f єто тишина ноль просто, а 1.0f максимум 
        ma_sound_set_volume(&sfxPool[currentSfxIndex], volume);

        // запускаем звук
        ma_sound_start(&sfxPool[currentSfxIndex]);
    }
    else {
        std::cerr << "ERROR::AUDIO: Failed to load sound: " << filepath << std::endl;
    }
    // пускаем индекс по кругу
    currentSfxIndex = (currentSfxIndex + 1) % MAX_SOUNDS;
}

// фоновая музыка
void AudioManager::playMusic(const std::string& filepath) {
    if (!isInitialized) return;

    // если музыка уже играет
    if (isMusicLoaded) {
        // выгружаем чтобы новую включить
        ma_sound_uninit(&bgmSound);
        isMusicLoaded = false;
    }

    // инициализация звука из файла
    ma_result result = ma_sound_init_from_file(&engine, filepath.c_str(), 0, NULL, NULL, &bgmSound);

    if (result == MA_SUCCESS) {
        isMusicLoaded = true;

        ma_sound_set_looping(&bgmSound, MA_TRUE); // включаем бесконечный повтор
        ma_sound_set_volume(&bgmSound, 0.05f); // делаем громкость 

        // воспроизведение музыки
        ma_sound_start(&bgmSound);
    }
    else { // ошибкоэээ
        std::cerr << "ERROR::AUDIO: Failed to load background music: " << filepath << std::endl;
    }
}

void AudioManager::stopMusic() {
    if (isMusicLoaded) {
        ma_sound_stop(&bgmSound);
    }
}

void AudioManager::setMasterVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    s_masterVolume = volume;
    if (volume > 0.001f) {
        s_savedVolume = volume;
        s_isMuted = false;
    }

    SettingsManager::setVolume(s_masterVolume);
    SettingsManager::setMuted(s_isMuted);

    if (isInitialized) {
        ma_engine_set_volume(&engine, s_isMuted ? 0.0f : s_masterVolume);
    }
}

float AudioManager::getMasterVolume() {
    return s_isMuted ? 0.0f : s_masterVolume;
}

float AudioManager::getRawVolume() {
    return s_savedVolume;
}

bool AudioManager::isMuted() {
    return s_isMuted || s_masterVolume <= 0.001f;
}

void AudioManager::setMuted(bool mute) {
    s_isMuted = mute;
    SettingsManager::setMuted(s_isMuted);

    if (isInitialized) {
        if (s_isMuted) {
            ma_engine_set_volume(&engine, 0.0f);
        }
        else {
            if (s_masterVolume <= 0.001f) {
                s_masterVolume = (s_savedVolume > 0.05f) ? s_savedVolume : 0.8f;
            }
            SettingsManager::setVolume(s_masterVolume);
            ma_engine_set_volume(&engine, s_masterVolume);
        }
    }
}

void AudioManager::toggleMute() {
    setMuted(!isMuted());
}