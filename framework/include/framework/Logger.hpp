#pragma once
#include "framework/internal/Framework.hpp"
#include "framework/internal/LogManager.hpp"
#include <fmt/core.h>
#include <string>
#include <string_view>
namespace framework {
class Logger {
public:
  Logger(std::string_view node_name) : node_name_(node_name) {}

  template <typename... Args>
  auto info(fmt::format_string<Args...> fmt, Args &&...args) -> void {
    Framework::instance().log_manager().info(
        node_name_, fmt::format(std::move(fmt), std::forward<Args>(args)...));
  }

  template <typename... Args>
  auto warn(fmt::format_string<Args...> fmt, Args &&...args) -> void {
    Framework::instance().log_manager().warn(
        node_name_, fmt::format(std::move(fmt), std::forward<Args>(args)...));
  }

  template <typename... Args>
  auto error(fmt::format_string<Args...> fmt, Args &&...args) -> void {
    Framework::instance().log_manager().error(
        node_name_, fmt::format(std::move(fmt), std::forward<Args>(args)...));
  }

private:
  std::string node_name_;
};
} // namespace framework
