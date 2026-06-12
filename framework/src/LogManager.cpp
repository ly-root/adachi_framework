#include "framework/internal/LogManager.hpp"
#include <chrono>
#include <filesystem>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
namespace framework {
LogManager::~LogManager() { spdlog::shutdown(); }

auto LogManager::init(const std::string &log_path) -> void {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  auto tm = *std::localtime(&t);
  auto file_path =
      fmt::format("{}/{:04d}-{:02d}-{:02d}_{:02d}_{:02d}_{:02d}.log", log_path,
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                  tm.tm_min, tm.tm_sec);

  std::filesystem::create_directories(log_path);

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path, true);

  console_sink->set_color(spdlog::level::info, console_sink->white);
  console_sink->set_color(spdlog::level::warn, console_sink->yellow);
  console_sink->set_color(spdlog::level::err, console_sink->red);
  console_sink->set_pattern("[%^%l] %v%$");
  file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
  std::shared_ptr<spdlog::logger> logger;
  logger = std::make_shared<spdlog::logger>(
      "global", spdlog::sinks_init_list{console_sink, file_sink});
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::flush_on(spdlog::level::warn);
  spdlog::set_default_logger(logger);
  logger_ = spdlog::default_logger();
}

auto LogManager::info(std::string_view node_name, std::string_view message)
    -> void {
  logger_->log(spdlog::level::info, "[{}] {}", node_name, message);
}
auto LogManager::warn(std::string_view node_name, std::string_view message)
    -> void {
  logger_->log(spdlog::level::warn, "[{}] {}", node_name, message);
}
auto LogManager::error(std::string_view node_name, std::string_view message)
    -> void {
  logger_->log(spdlog::level::err, "[{}] {}", node_name, message);
}
} // namespace framework
