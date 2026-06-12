#pragma once
#include "framework/internal/Framework.hpp"
#include "framework/internal/TopicManager.hpp"
#include <memory>
#include <string>
namespace framework {
template <typename T> class Publisher {
public:
  Publisher(const std::string &topic_name) {
    topic_ = Framework::instance().topic_manager().get_topic<T>(topic_name);
  }
  auto publish(std::unique_ptr<T> message) -> void {
    std::shared_ptr<T> message_ptr = std::move(message);
    topic_->store(message_ptr);
    topic_->notify_all();
  }

private:
  std::shared_ptr<std::atomic<std::shared_ptr<T>>> topic_;
};
} // namespace framework
