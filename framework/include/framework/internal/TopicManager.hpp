#pragma once
#include "framework/internal/TopicHolder.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
namespace framework {
class TopicManager {
public:
  template <typename T>
  auto get_topic(const std::string &name)
      -> std::shared_ptr<std::atomic<std::shared_ptr<T>>> {
    std::lock_guard<std::mutex> lock(topics_mutex_);
    auto it = topics_.find(name);
    if (it != topics_.end()) {
      auto holder = std::dynamic_pointer_cast<TopicHolder<T>>(it->second);
      if (!holder) {
        throw std::runtime_error("Topic type mismatch for topic: " + name);
      }
      return holder->topic;
    }
    auto holder = std::make_shared<TopicHolder<T>>();
    topics_[name] = holder;
    return holder->topic;
  }
  auto get_topic(const std::string &name) -> TopicInterface &;
  auto topic_exists(const std::string &name) -> bool;
  auto subscribe(void *subscriber_ptr, std::jthread thread) -> void;
  auto unsubscribe(void *subscriber_ptr) -> void;
  auto stop_subscribers() -> void;
  auto is_subscribed(void *subscriber_ptr) -> bool;
  ~TopicManager();

private:
  bool stopped_{false};
  std::mutex topics_mutex_;
  std::mutex subscribers_mutex_;
  std::vector<std::jthread> unsubscribed_threads_;
  std::unordered_map<void *, std::jthread> subscriber_threads_;
  std::unordered_map<std::string, std::shared_ptr<TopicInterface>> topics_;
};
} // namespace framework
