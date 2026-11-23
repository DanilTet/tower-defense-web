#include "EventBus.h"

// инициализируем статическое бинарное дерево подписчиков
std::map<EventType, std::vector<EventBus::EventCallback>> EventBus::m_subscribers;

void EventBus::subscribe(EventType type, EventCallback callback) {
    m_subscribers[type].push_back(callback);
}

void EventBus::publish(const Event& event) {
    // если на это событие кто-то подписан то вызываем всех подписчиков
    auto it = m_subscribers.find(event.type);
    if (it != m_subscribers.end()) {
        for (auto& callback : it->second) {
            callback(event);
        }
    }
}

void EventBus::clear() {
    m_subscribers.clear();
}