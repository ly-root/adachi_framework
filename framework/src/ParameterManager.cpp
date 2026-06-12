#include "framework/internal/ParameterManager.hpp"
#include "framework/internal/LuaServer.hpp"
#include <mutex>
#include <string>
namespace framework {
ParameterManager::~ParameterManager() { stop_listeners(); }

auto ParameterManager::init(const std::string &config_path) -> void {
  lua_server_ = std::make_unique<LuaServer>();
  lua_server_->run(config_path);
  std::unique_lock lock(config_map_mutex_);
  config_map_ = lua_server_->get_config_map();
}
auto ParameterManager::get_value(const std::string &name) -> ParameterValue {
  std::shared_lock lock(config_map_mutex_);
  auto it = config_map_.find(name);
  if (it != config_map_.end()) {
    return it->second;
  }
  throw std::runtime_error("Parameter not found: " + name);
}

auto ParameterManager::set_value(const std::string &name,
                                 const ParameterValue &value) -> void {
  std::unique_lock lock(config_map_mutex_);
  config_map_[name] = value;
}

auto ParameterManager::add_update_listener(
    std::string prefix,
    std::function<void(const std::vector<std::string> &names)> callback)
    -> void {
  if (update_parameter_names_.contains(prefix)) {
    throw std::runtime_error("Update listener for prefix already exists: " +
                             prefix);
  }
  update_parameter_names_[prefix] = nullptr;
  update_listener_threads_.emplace_back(
      [this, prefix, callback](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
          auto update_parameter_names =
              update_parameter_names_[prefix].exchange(nullptr);
          if (update_parameter_names) {
            callback(*update_parameter_names);
          }
          update_parameter_names_[prefix].wait(nullptr);
        }
      });
}

auto ParameterManager::parameter_get_callback()
    -> std::function<ParameterValue(const std::string &name)> {
  return [this](const std::string &name) { return get_value(name); };
}

auto ParameterManager::parameter_set_callback()
    -> std::function<void(const std::string &name,
                          const ParameterValue &value)> {
  return [this](const std::string &name, const ParameterValue &value) {
    std::unique_lock lock(config_map_mutex_);
    config_map_[name] = value;
  };
}

auto ParameterManager::get_all_parameter_names() -> std::vector<std::string> {
  std::shared_lock lock(config_map_mutex_);
  std::vector<std::string> names;
  for (const auto &entry : config_map_) {
    names.push_back(entry.first);
  }
  return names;
}

auto ParameterManager::parameter_update_callback()
    -> std::function<void(const std::vector<std::string> &names)> {
  return [this](const std::vector<std::string> &names) {
    std::unordered_map<std::string, std::shared_ptr<std::vector<std::string>>>
        updated_names;
    for (const auto &name : names) {
      auto pos = name.find('.');
      if (pos == std::string::npos)
        continue;
      std::string prefix = name.substr(0, pos);
      std::string suffix = name.substr(pos + 1);
      auto &names_ptr = updated_names[prefix];
      if (!names_ptr) {
        names_ptr = std::make_shared<std::vector<std::string>>();
      }
      names_ptr->push_back(std::move(suffix));
    }
    for (const auto &[key, value] : updated_names) {
      if (!update_parameter_names_.contains(key))
        continue;
      update_parameter_names_[key] = value;
      update_parameter_names_[key].notify_all();
    }
  };
}

auto ParameterManager::stop_listeners() -> void {
  for (auto &thread : update_listener_threads_) {
    thread.request_stop();
  }
  for (auto &[key, value] : update_parameter_names_) {
    value.store(std::make_shared<std::vector<std::string>>());
    value.notify_all();
  }
  for (auto &thread : update_listener_threads_) {
    if (thread.joinable())
      thread.join();
  }
}

} // namespace framework
