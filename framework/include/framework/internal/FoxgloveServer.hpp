#pragma once
#include "framework/Logger.hpp"
#include "framework/ParameterHandle.hpp"
#include "framework/internal/ParameterTraits.hpp"
#include <fmt/base.h>
#include <foxglove/channel.hpp>
#include <foxglove/websocket.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
namespace framework {
class FoxgloveServer {
public:
  auto start() -> void;
  auto
  setup_parameters(std::function<ParameterValue(const std::string &name)>
                       get_parameter_callback,
                   std::function<void(const std::string &name, ParameterValue)>
                       set_parameter_callback,
                   std::function<void(const std::vector<std::string> &name)>
                       parameter_update_callback,
                   std::vector<std::string> parameter_names) -> void;
  auto subscribe(const std::string &topic_name) -> bool;
  auto unsubscribe(const std::string &topic_name) -> void;

private:
  struct Parameter {
    std::vector<std::string> subscribe_topics;
  } parameter_;

  auto create_parameter_handler() -> foxglove::ParameterHandler;
  auto create_parameter_update_callback()
      -> std::function<void(const std::vector<std::string> &name)>;
  template <ParameterType T>
  auto set_parameter(const foxglove::ParameterView &parameter_view) -> bool;
  auto get_parameter(const std::string &name) -> foxglove::Parameter;

  std::mutex topic_mutex_;
  framework::Logger logger_{"FoxgloveServer"};
  framework::ParameterHandle parameter_handle_{"foxglove"};
  std::function<void(const std::string &name, ParameterValue)>
      parameter_set_callback_;
  std::function<ParameterValue(const std::string &name)>
      parameter_get_callback_;
  std::function<void(const std::vector<std::string> &name)>
      parameter_update_callback_;
  std::vector<std::string> parameter_names_;
  std::unique_ptr<foxglove::WebSocketServer> server_;
  std::unordered_map<std::string, std::shared_ptr<foxglove::RawChannel>>
      topic_channels_;
};
} // namespace framework
