#include "EventBus.h"

// инициализируем статическое бинарное дерево подписчиков
std::map<EventType, std::vector<EventBus::EventCallback>> EventBus::m_subscribers;

void EventBus::subscribe(EventType type, EventCallback callback) {
    m_subscribers[type].push_back(callback);
}

void EventBus::publish(const Event& event) {
    // если на это событие кто-то подписан то вызываем всех подписчиков
    if (m_subscribers.find(event.type) != m_subscribers.end()) {
        for (auto& callback : m_subscribers[event.type]) {
            callback(event);
        }
    }
}

void EventBus::clear() {
    m_subscribers.clear();
}