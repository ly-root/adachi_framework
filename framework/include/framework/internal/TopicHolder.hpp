#pragma once
#include "framework/internal/Reflection.hpp"
#include <memory>
#include <nlohmann/json.hpp>

namespace framework {
class TopicInterface {
public:
  virtual ~TopicInterface() = default;
  virtual auto cancel() -> void = 0;
  virtual auto load() -> std::shared_ptr<void> = 0;
  virtual auto wait(std::shared_ptr<void> last) -> void = 0;
  virtual auto serialize(std::shared_ptr<void> data) -> std::string = 0;
  virtual auto schema() -> std::string = 0;
};
template <typename T> class TopicHolder : public TopicInterface {
public:
  void cancel() {
    topic->store(std::make_shared<T>());
    topic->notify_all();
  }
  auto load() -> std::shared_ptr<void> override { return topic->load(); }
  auto wait(std::shared_ptr<void> last) -> void override {
    auto typed = std::static_pointer_cast<T>(last);
    topic->wait(typed);
  }
  auto serialize(std::shared_ptr<void> data) -> std::string override {
    if (!data)
      return "{}";
    auto *typed = static_cast<const T *>(data.get());
    nlohmann::json j;
    Reflect::serialize(j, *typed);
    if (!j.is_object())
      j = {{"value", std::move(j)}};
    return j.dump();
  }
  auto schema() -> std::string override {
    nlohmann::json j;
    Reflect::schema<T>(j);
    if (j["type"] != "object")
      j = {{"type", "object"}, {"properties", {{"value", j}}}};
    return j.dump();
  }
  std::shared_ptr<std::atomic<std::shared_ptr<T>>> topic =
      std::make_shared<std::atomic<std::shared_ptr<T>>>();
};

} // namespace framework
