#pragma once
#include "framework/internal/LuaServer.hpp"
#include "framework/internal/ParameterTraits.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace framework {

class ParameterManager {
public:
  ParameterManager() = default;
  ~ParameterManager();
  auto init(const std::string &config_path) -> void;
  auto get_value(const std::string &name) -> ParameterValue;
  auto set_value(const std::string &name, const ParameterValue &value) -> void;
  auto add_update_listener(
      std::string prefix,
      std::function<void(const std::vector<std::string> &names)> callback)
      -> void;
  auto parameter_get_callback()
      -> std::function<ParameterValue(const std::string &name)>;
  auto parameter_set_callback()
      -> std::function<void(const std::string &name,
                            const ParameterValue &value)>;
  auto get_all_parameter_names() -> std::vector<std::string>;
  auto parameter_update_callback()
      -> std::function<void(const std::vector<std::string> &names)>;
  auto stop_listeners() -> void;

private:
  mutable std::shared_mutex config_map_mutex_;
  std::unordered_map<std::string, ParameterValue> config_map_;
  std::vector<std::jthread> update_listener_threads_;
  std::unordered_map<std::string,
                     std::atomic<std::shared_ptr<std::vector<std::string>>>>
      update_parameter_names_;
  std::unique_ptr<LuaServer> lua_server_;
};
} // namespace framework
