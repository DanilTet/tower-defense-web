#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include "AudioManager.h"
#include "../game/core/SettingsManager.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <chrono>

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

// кэш предзагруженных декодированных звуков и анти-спам по пути
static std::unordered_map<std::string, ma_sound*> s_sfxCache;
static std::unordered_map<std::string, double> s_lastPlayMs;
static const double SFX_COOLDOWN_MS = 45.0;

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
        isMusicLoaded = false;
    }

    // очищаем буффер
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (sfxAllocated[i]) {
            ma_sound_uninit(&sfxPool[i]);
            sfxAllocated[i] = false;
        }
    }
    currentSfxIndex = 0;

    // копии выгружены - теперь можно освободить кэшированные источники
    for (auto it = s_sfxCache.begin(); it != s_sfxCache.end(); ++it) {
        if (it->second != nullptr) {
            ma_sound_uninit(it->second);
            delete it->second;
        }
    }
    s_sfxCache.clear();
    s_lastPlayMs.clear();

    if (isInitialized) {
        ma_engine_uninit(&engine);
        isInitialized = false;
    }
}

// возвращает закэшированный декодированный источник (грузит с диска только один раз)
static ma_sound* getCachedSource(const std::string& filepath) {
    auto it = s_sfxCache.find(filepath);
    if (it != s_sfxCache.end()) {
        return it->second; // может быть nullptr, если файл ранее не удалось загрузить
    }

    ma_sound* source = new ma_sound();
    ma_result result = ma_sound_init_from_file(&engine, filepath.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, source);
    if (result != MA_SUCCESS) {
        std::cerr << "ERROR::AUDIO: Failed to load sound: " << filepath << std::endl;
        delete source;
        s_sfxCache[filepath] = nullptr; // запоминаем неудачу, чтобы не бить по диску повторно
        return nullptr;
    }

    s_sfxCache[filepath] = source;
    return source;
}

void AudioManager::preloadSound(const std::string& filepath) {
    if (!isInitialized || filepath.empty()) return;
    getCachedSource(filepath);
}

void AudioManager::playSound(const std::string& filepath, float volume) {
    if (!isInitialized || s_isMuted || volume <= 0.0001f || filepath.empty()) {
        return;
    }

    // анти-спам: один и тот же звук не чаще чем раз в SFX_COOLDOWN_MS
    double nowMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    auto cd = s_lastPlayMs.find(filepath);
    if (cd != s_lastPlayMs.end()) {
        if (nowMs - cd->second < SFX_COOLDOWN_MS) {
            return;
        }
    }

    ma_sound* source = getCachedSource(filepath);
    if (source == nullptr) {
        return;
    }
    s_lastPlayMs[filepath] = nowMs;

    // Ищем свободный или уже завершивший проигрывание слот
    int slot = currentSfxIndex;
    for (int offset = 0; offset < MAX_SOUNDS; ++offset) {
        int candidate = (currentSfxIndex + offset) % MAX_SOUNDS;
        if (!sfxAllocated[candidate] || !ma_sound_is_playing(&sfxPool[candidate])) {
            slot = candidate;
            break;
        }
    }

    if (sfxAllocated[slot]) {
        ma_sound_uninit(&sfxPool[slot]);
        sfxAllocated[slot] = false;
    }

    // копия делит уже декодированный буфер с источником, без обращения к диску
    ma_result result = ma_sound_init_copy(&engine, source, 0, NULL, &sfxPool[slot]);

    if (result == MA_SUCCESS) {
        sfxAllocated[slot] = true;
        ma_sound_set_volume(&sfxPool[slot], volume);
        ma_sound_start(&sfxPool[slot]);
    }
    else {
        std::cerr << "ERROR::AUDIO: Failed to copy cached sound: " << filepath << std::endl;
    }
    currentSfxIndex = (slot + 1) % MAX_SOUNDS;
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