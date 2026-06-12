#pragma once
#include "framework/internal/ParameterTraits.hpp"
#include "sol/forward.hpp"
#include <sol/sol.hpp>
#include <string>

namespace framework {
class LuaServer {
public:
  LuaServer() = default;
  auto run(const std::string &config_dir_path) -> void;
  auto get_config_map() -> std::unordered_map<std::string, ParameterValue>;

private:
  auto flatten(const sol::object &obj, const std::string &prefix,
               std::unordered_map<std::string, ParameterValue> &out) -> void;

  template <typename T> auto is_array(const sol::table &table) -> bool;
  template <typename T>
  auto to_array(const sol::table &table) -> std::vector<T>;
  auto is_object_table(const sol::table &obj) -> bool;

  std::string config_path_;
  sol::state lua_;
  sol::table config_;
};
} // namespace framework
