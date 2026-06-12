#pragma once
#include "framework/internal/Framework.hpp"
#include "framework/internal/TopicManager.hpp"
#include <functional>
#include <memory>
#include <string>
#include <thread>
namespace framework {
template <typename T> class Subscriber {
public:
  Subscriber(const std::string &topic_name) : topic_name_(topic_name) {
    topic_ = Framework::instance().topic_manager().get_topic<T>(topic_name);
  }
  ~Subscriber() { unsubscribe(); }
  auto on_receive(std::function<void(std::shared_ptr<const T>)> function)
      -> void {
    Framework::instance().topic_manager().subscribe(
        this,
        std::jthread([topic = topic_, function](std::stop_token stop_token) {
          std::shared_ptr<T> topic_data{nullptr};
          while (!stop_token.stop_requested()) {
            topic_data = topic->load();
            if (topic_data) {
              function(topic_data);
            }
            topic->wait(topic_data);
          }
        }));
  }
  auto unsubscribe() -> void {
    Framework::instance().topic_manager().unsubscribe(this);
  }
  auto is_subscribed() -> bool {
    return Framework::instance().topic_manager().is_subscribed(this);
  }

private:
  std::string topic_name_;
  std::shared_ptr<std::atomic<std::shared_ptr<T>>> topic_;
};
} // namespace framework
