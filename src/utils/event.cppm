export module event;

import std;

export class EventManager {
 public:
  template <typename EventData>
  class Event {
   public:
    using Listener = std::function<void(const EventData&)>;

    std::size_t addEventListener(Listener callback) {
      std::size_t id = nextListenerId++;
      listeners[id] = callback;
      return id;
    }

    bool removeEventListener(std::size_t id) { return listeners.erase(id) > 0; }

    void dispatch(const EventData& data) const {
      for (const auto& [id, listener] : listeners) listener(data);
    }

   private:
    std::unordered_map<std::size_t, Listener> listeners;
    static inline std::size_t nextListenerId = 0;
  };

  template <typename EventData>
  Event<EventData>& getEvent(const std::string& eventName) {
    auto key = std::make_pair(eventName, std::type_index(typeid(EventData)));
    if (events.find(key) == events.end())
      events[key] = std::make_any<Event<EventData>>();
    return std::any_cast<Event<EventData>&>(events[key]);
  }

  template <typename EventData>
  std::size_t addEventListener(const std::string& eventName,
                          std::function<void(const EventData&)> callback) {
    return getEvent<EventData>(eventName).addEventListener(callback);
  }

  template <typename EventData>
  bool removeEventListener(const std::string& eventName, std::size_t id) {
    return getEvent<EventData>(eventName).removeEventListener(id);
  }

  template <typename EventData>
  void dispatchEvent(const std::string& eventName, const EventData& data) {
    getEvent<EventData>(eventName).dispatch(data);
  }

 private:
  struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const {
      return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
  };

  std::unordered_map<std::pair<std::string, std::type_index>, std::any,
                     PairHash>
      events;
};
