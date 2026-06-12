#include "framework/internal/Framework.hpp"
#include "framework/internal/FoxgloveServer.hpp"
#include "framework/internal/LogManager.hpp"
#include "framework/internal/NodeRegistry.hpp"
#include "framework/internal/ParameterManager.hpp"
#include "framework/internal/TopicManager.hpp"
namespace framework {
Framework::Framework() {
  parameter_manager_ = std::make_unique<ParameterManager>();
  log_manager_ = std::make_unique<LogManager>();
  topic_manager_ = std::make_unique<TopicManager>();
  foxglove_server_ = std::make_unique<FoxgloveServer>();
}
Framework::~Framework() { stop(); }

auto Framework::instance() -> Framework & {
  static Framework instance;
  return instance;
}
auto Framework::init(const std::string &project_path) -> void {
  log_manager_->init(project_path + "/logs");
  parameter_manager_->init(project_path + "/config");
  nodes_ = NodeRegistry::instance().start_nodes();
  foxglove_server_->setup_parameters(
      parameter_manager_->parameter_get_callback(),
      parameter_manager_->parameter_set_callback(),
      parameter_manager_->parameter_update_callback(),
      parameter_manager_->get_all_parameter_names());
  foxglove_server_->start();
}
auto Framework::parameter_manager() -> ParameterManager & {
  return *parameter_manager_;
}
auto Framework::log_manager() -> LogManager & { return *log_manager_; }
auto Framework::topic_manager() -> TopicManager & { return *topic_manager_; }
auto Framework::stop() -> void {
  if (parameter_manager_) {
    parameter_manager_->stop_listeners();
  }
  if (topic_manager_) {
    topic_manager_->stop_subscribers();
  }
  foxglove_server_.reset();
  nodes_.clear();
  topic_manager_.reset();
  parameter_manager_.reset();
  log_manager_.reset();
}
} // namespace framework
