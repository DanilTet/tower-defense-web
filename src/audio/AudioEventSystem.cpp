#include "AudioEventSystem.h"
#include "AudioManager.h"
#include "../game/core/EventBus.h"

void AudioEventSystem::init() {
    EventBus::subscribe(EventType::EnemyDied, [](const Event& e) {
        if (!e.textData.empty()) {
            AudioManager::playSound(e.textData.c_str(), 0.1f);
        }
    });

    EventBus::subscribe(EventType::TowerFired, [](const Event& e) {
        if (!e.textData.empty()) {
            AudioManager::playSound(e.textData.c_str(), 0.1f);
        }
    });

    EventBus::subscribe(EventType::TowerBuilt, [](const Event& e) {
        if (!e.textData.empty()) {
            AudioManager::playSound(e.textData.c_str(), 0.1f);
        }
    });
}

void AudioEventSystem::playThemeMusic(const std::string& musicPath) {
    AudioManager::playMusic(musicPath.c_str());
}

void AudioEventSystem::cleanup() {
    // Ничего особенного освобождать не нужно, подписки очищаются при EventBus::clear()
}

