#pragma once
#include <memory>
#include <string>
#include <string_view>
namespace spdlog {
class logger;
}
namespace framework {
class LogManager {
public:
  ~LogManager();
  auto init(const std::string &log_path) -> void;
  auto info(std::string_view node_name, std::string_view message) -> void;
  auto warn(std::string_view node_name, std::string_view message) -> void;
  auto error(std::string_view node_name, std::string_view message) -> void;

private:
  std::shared_ptr<spdlog::logger> logger_;
};
} // namespace framework
