#include "framework/internal/LuaServer.hpp"
#include <filesystem>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <sol/table.hpp>
#include <string>
namespace framework {
auto LuaServer::run(const std::string &config_dir_path) -> void {
  config_path_ = config_dir_path + "/config.lua";
  lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                      sol::lib::table, sol::lib::os, sol::lib::io);
  lua_["package"]["path"] =
      config_dir_path + "/?.lua;" + config_dir_path + "/?/init.lua;";
  lua_["project_dir"] =
      std::filesystem::path(config_dir_path).parent_path().string();
  lua_.set_function("list_dirs", [](const std::string &root) {
    std::vector<std::string> dirs;
    for (auto const &entry : std::filesystem::directory_iterator(root)) {
      if (entry.is_directory()) {
        dirs.push_back(entry.path().filename().string());
      }
    }
    return dirs;
  });

  sol::load_result chunk = lua_.load_file(config_path_);
  if (!chunk.valid()) {
    sol::error e = chunk;
    throw std::runtime_error(e.what());
  }
  sol::protected_function_result result = chunk();
  if (!result.valid()) {
    sol::error e = result;
    throw std::runtime_error(e.what());
  }
  config_ = result;
}

auto LuaServer::get_config_map()
    -> std::unordered_map<std::string, ParameterValue> {
  std::unordered_map<std::string, ParameterValue> config_map;
  if (!config_.is<sol::table>()) {
    throw std::runtime_error("Top-level configuration must be a table");
  }
  if (config_.empty()) {
    return config_map;
  }
  for (auto &[key, _] : config_) {
    if (!key.is<std::string>()) {
      throw std::runtime_error(
          "Top-level configuration must contain named entries only");
    }
  }
  flatten(config_, "", config_map);
  return config_map;
}

auto LuaServer::flatten(const sol::object &obj, const std::string &prefix,
                        std::unordered_map<std::string, ParameterValue> &out)
    -> void {
  if (obj.is<sol::table>()) {
    auto table = obj.as<sol::table>();
    if (is_array<double>(table)) {
      out[prefix] = to_array<double>(table);
      return;
    } else if (is_array<std::string>(table)) {
      out[prefix] = to_array<std::string>(table);
      return;
    } else if (is_object_table(table)) {
      for (auto &[key, value] : table) {
        std::string key_str = key.as<std::string>();
        std::string full = prefix.empty() ? key_str : prefix + "." + key_str;
        flatten(value, full, out);
      }
      return;
    }
  } else {
    if (obj.is<double>()) {
      out[prefix] = obj.as<double>();
      return;
    } else if (obj.is<bool>()) {
      out[prefix] = obj.as<bool>();
      return;
    } else if (obj.is<std::string>()) {
      out[prefix] = obj.as<std::string>();
      return;
    }
  }
  throw std::runtime_error("Unsupported value type: " + prefix);
}

template <typename T>
auto LuaServer::is_array(const sol::table &table) -> bool {
  std::size_t count = 0;
  for (auto &&[key, value] : table) {
    if (!key.is<int>()) {
      return false;
    }
    if (!value.is<T>()) {
      return false;
    }
    ++count;
  }
  return count == table.size();
}

template <typename T>
auto LuaServer::to_array(const sol::table &table) -> std::vector<T> {
  std::vector<T> result;
  result.reserve(table.size());
  for (std::size_t i = 1; i <= table.size(); ++i) {
    sol::object obj = table[i];
    result.push_back(obj.as<T>());
  }
  return result;
}

auto LuaServer::is_object_table(const sol::table &table) -> bool {
  for (auto &&[key, value] : table) {
    if (!key.is<std::string>()) {
      return false;
    }
  }
  return true;
}
} // namespace framework
