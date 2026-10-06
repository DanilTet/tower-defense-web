#pragma once
#include <functional>
#include <vector>
#include <map>
#include <string>

enum class EventType {
    EnemyDied, // враг помер
    EnemyReachedBase, // враг дошел до базі
    TowerFired, // башня вістрелела
    TowerBuilt // башня построена
};

// структура которую передает радио
struct Event {
    EventType type;
    int value1 = 0; // Универсальное число 1 (например, награда или урон)
    int value2 = 0; // Универсальное число 2 (например, очки)
    float x = 0.0f; // Координата X (для частиц или звука в пространстве)
    float y = 0.0f; // Координата Y
    std::string textData = "";
};

class EventBus {
public:
    using EventCallback = std::function<void(const Event&)>;

    // подписаться на событие
    static void subscribe(EventType type, EventCallback callback);

    // опубликовать событие в эфир
    static void publish(const Event& event);

    // очистить все подписки ВІЗІВАТЬ ПРИ РЕСТАРТЕ
    static void clear();

private:
    static std::map<EventType, std::vector<EventCallback>> m_subscribers;
};