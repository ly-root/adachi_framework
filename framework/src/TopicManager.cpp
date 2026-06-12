#include "framework/internal/TopicManager.hpp"
namespace framework {
auto TopicManager::get_topic(const std::string &name) -> TopicInterface & {
  std::lock_guard<std::mutex> lock(topics_mutex_);
  auto it = topics_.find(name);
  if (it == topics_.end()) {
    throw std::runtime_error("Topic not found: " + name);
  }
  return *it->second;
}

auto TopicManager::topic_exists(const std::string &name) -> bool {
  std::lock_guard<std::mutex> lock(topics_mutex_);
  return topics_.find(name) != topics_.end();
}

auto TopicManager::subscribe(void *subscriber_ptr, std::jthread thread)
    -> void {
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  if (stopped_) {
    return;
  }
  if (subscriber_threads_.find(subscriber_ptr) != subscriber_threads_.end()) {
    throw std::runtime_error("Subscriber already exists");
  }
  subscriber_threads_[subscriber_ptr] = std::move(thread);
}
TopicManager::~TopicManager() { stop_subscribers(); }

auto TopicManager::unsubscribe(void *subscriber_ptr) -> void {
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  if (stopped_) {
    return;
  }
  auto it = subscriber_threads_.find(subscriber_ptr);
  if (it != subscriber_threads_.end()) {
    it->second.request_stop();
    unsubscribed_threads_.push_back(std::move(it->second));
    subscriber_threads_.erase(it);
  }
}

auto TopicManager::stop_subscribers() -> void {
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  for (auto &thread : subscriber_threads_) {
    thread.second.request_stop();
  }
  for (auto &topic : topics_) {
    topic.second->cancel();
  }
  for (auto &thread : subscriber_threads_) {
    if (thread.second.joinable()) {
      thread.second.join();
    }
  }
  for (auto &thread : unsubscribed_threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  stopped_ = true;
}
auto TopicManager::is_subscribed(void *subscriber_ptr) -> bool {
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  return subscriber_threads_.find(subscriber_ptr) != subscriber_threads_.end();
}
} // namespace framework
