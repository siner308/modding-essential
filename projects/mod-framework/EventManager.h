#pragma once
#include <map>
#include <vector>
#include <functional>
#include <string>

using EventHandler = std::function<void(void*)>;

class EventManager {
public:
    static EventManager& GetInstance();
    void Subscribe(const std::string& eventName, EventHandler handler);
    void Unsubscribe(const std::string& eventName, EventHandler handler);
    void Dispatch(const std::string& eventName, void* eventArgs);

private:
    EventManager() = default;
    std::map<std::string, std::vector<EventHandler>> m_events;
};
