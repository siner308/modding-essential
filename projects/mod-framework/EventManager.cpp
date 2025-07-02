#include "EventManager.h"

EventManager& EventManager::GetInstance() {
    static EventManager instance;
    return instance;
}

void EventManager::Subscribe(const std::string& eventName, EventHandler handler) {
    m_events[eventName].push_back(handler);
}

void EventManager::Unsubscribe(const std::string& eventName, EventHandler handler) {
    // This is a simplified implementation. A real one would need to compare function objects.
}

void EventManager::Dispatch(const std::string& eventName, void* eventArgs) {
    if (m_events.find(eventName) != m_events.end()) {
        for (auto& handler : m_events[eventName]) {
            handler(eventArgs);
        }
    }
}
