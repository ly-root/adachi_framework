#pragma once
#include <memory>
#include <string>
#include <vector>
namespace framework {
class FoxgloveServer;
class ParameterManager;
class Node;
class LogManager;
class TopicManager;
class Framework {
public:
  ~Framework();
  static auto instance() -> Framework &;
  auto init(const std::string &project_path) -> void;
  auto parameter_manager() -> ParameterManager &;
  auto log_manager() -> LogManager &;
  auto topic_manager() -> TopicManager &;
  auto stop() -> void;

private:
  Framework();
  std::unique_ptr<LogManager> log_manager_;
  std::unique_ptr<ParameterManager> parameter_manager_;
  std::unique_ptr<TopicManager> topic_manager_;
  std::unique_ptr<FoxgloveServer> foxglove_server_;
  std::vector<std::unique_ptr<Node>> nodes_;
};
} // namespace framework
