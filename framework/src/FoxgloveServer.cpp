#include "framework/internal/FoxgloveServer.hpp"
#include "framework/internal/Framework.hpp"
#include "framework/internal/TopicManager.hpp"
#include <cstdint>
#include <foxglove/parameter_handler.hpp>
#include <foxglove/websocket.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
namespace framework {

auto FoxgloveServer::subscribe(const std::string &topic_name) -> bool {
  if (!Framework::instance().topic_manager().topic_exists(topic_name)) {
    logger_.warn("Topic '{}' not found, skipping", topic_name);
    return false;
  }
  auto &topic_interface =
      Framework::instance().topic_manager().get_topic(topic_name);
  auto schema_str = topic_interface.schema();
  auto schema_json = nlohmann::json::parse(schema_str);
  auto schema_name = schema_json.contains("title")
                         ? schema_json["title"].get<std::string>()
                         : topic_name;
  foxglove::Schema schema{
      schema_name, "jsonschema",
      reinterpret_cast<const std::byte *>(schema_str.data()),
      schema_str.size()};
  auto channel_result =
      foxglove::RawChannel::create(topic_name, "json", std::move(schema));
  if (!channel_result) {
    logger_.error("Failed to create channel for topic '{}'", topic_name);
    return false;
  }
  auto channel =
      std::make_shared<foxglove::RawChannel>(std::move(*channel_result));
  auto thread =
      std::jthread([&topic_interface, channel](std::stop_token stop_token) {
        std::shared_ptr<void> last{nullptr};
        while (!stop_token.stop_requested()) {
          auto data = topic_interface.load();
          if (data) {
            auto json = topic_interface.serialize(data);
            channel->log(reinterpret_cast<const std::byte *>(json.data()),
                         json.size());
          }
          topic_interface.wait(data);
        }
      });
  Framework::instance().topic_manager().subscribe(channel.get(),
                                                  std::move(thread));
  topic_channels_[topic_name] = channel;
  logger_.info("Foxglove subscribed to topic '{}'", topic_name);
  return true;
}

auto FoxgloveServer::unsubscribe(const std::string &topic_name) -> void {
  auto it = topic_channels_.find(topic_name);
  if (it != topic_channels_.end()) {
    Framework::instance().topic_manager().unsubscribe(it->second.get());
    topic_channels_.erase(it);
  }
}

auto FoxgloveServer::start() -> void {
  foxglove::WebSocketServerOptions options;
  options.port = 8765;
  options.capabilities = {foxglove::WebSocketServerCapabilities::Parameters};
  options.parameter_handler = std::move(create_parameter_handler());
  auto result = foxglove::WebSocketServer::create(std::move(options));
  if (!result) {
    throw std::runtime_error(foxglove::strerror(result.error()));
  }
  server_ =
      std::make_unique<foxglove::WebSocketServer>(std::move(result.value()));
  parameter_handle_.bind_parameter("subscribe_topics",
                                   parameter_.subscribe_topics);
  parameter_handle_.set_callback(create_parameter_update_callback());
  std::lock_guard lock(topic_mutex_);
  for (const auto &name : parameter_.subscribe_topics) {
    subscribe(name);
  }
}

auto FoxgloveServer::setup_parameters(
    std::function<ParameterValue(const std::string &name)>
        get_parameter_callback,
    std::function<void(const std::string &name, ParameterValue)>
        set_parameter_callback,
    std::function<void(const std::vector<std::string> &name)>
        parameter_update_callback,
    std::vector<std::string> parameter_names) -> void {
  parameter_get_callback_ = get_parameter_callback;
  parameter_set_callback_ = set_parameter_callback;
  parameter_update_callback_ = parameter_update_callback;
  parameter_names_ = parameter_names;
}

auto FoxgloveServer::create_parameter_update_callback()
    -> std::function<void(const std::vector<std::string> &name)> {
  return [this](const std::vector<std::string> &names) {
    for (const auto &name : names) {
      parameter_handle_.apply_parameter_value(name);
      if (name == "subscribe_topics") {
        std::lock_guard lock(topic_mutex_);
        std::unordered_set<std::string> new_topics(
            parameter_.subscribe_topics.begin(),
            parameter_.subscribe_topics.end());
        std::vector<std::string> to_unsubscribe;
        for (const auto &topic : topic_channels_) {
          if (!new_topics.contains(topic.first)) {
            to_unsubscribe.push_back(topic.first);
          }
        }
        std::vector<std::string> to_subscribe;
        for (const auto &topic_name : parameter_.subscribe_topics) {
          if (!topic_channels_.contains(topic_name)) {
            to_subscribe.push_back(topic_name);
          }
        }
        for (const auto &topic_name : to_unsubscribe) {
          unsubscribe(topic_name);
        }
        for (const auto &topic_name : to_subscribe) {
          subscribe(topic_name);
        }
      }
    }
  };
}

auto FoxgloveServer::create_parameter_handler() -> foxglove::ParameterHandler {
  foxglove::ParameterHandler handler;
  handler.onGet = [this](std::uint32_t, std::optional<std::string_view>,
                         const std::vector<std::string_view> &names,
                         foxglove::GetParametersResponder &&responder) {
    std::vector<foxglove::Parameter> out;
    std::vector<std::string> updated_names;
    if (names.empty()) {
      updated_names = parameter_names_;
    } else {
      for (const auto &name : names) {
        updated_names.emplace_back(name);
      }
    }
    for (auto &name : updated_names) {
      out.emplace_back(get_parameter(name));
    }
    std::move(responder).respond(std::move(out));
  };

  handler.onSet = [this](std::uint32_t, std::optional<std::string_view>,
                         const std::vector<foxglove::ParameterView> &params,
                         foxglove::SetParametersResponder &&responder) {
    std::vector<foxglove::Parameter> reply;
    std::vector<std::string> updated_names;
    std::vector<std::string> not_updated_names;
    std::vector<std::string> names;
    for (const auto &param : params) {
      const std::string name(param.name());
      names.push_back(name);
      auto original_value = parameter_get_callback_(name);
      auto value = param.value();
      std::visit(
          [&](auto &&v) {
            using T = std::decay_t<decltype(v)>;
            if (set_parameter<T>(param)) {
              updated_names.push_back(name);
            } else {
              not_updated_names.push_back(name);
            }
          },
          original_value);
    }
    for (const auto &name : not_updated_names) {
      logger_.warn("Failed to update parameter '{}'", name);
    }
    for (const auto &name : names) {
      reply.emplace_back(get_parameter(name));
    }
    parameter_update_callback_(updated_names);
    std::move(responder).respond(std::move(reply));
  };
  return handler;
}

template <ParameterType T>
auto FoxgloveServer::set_parameter(
    const foxglove::ParameterView &parameter_view) -> bool {
  const std::string name(parameter_view.name());
  auto value = parameter_view.value();
  if constexpr (std::is_same_v<T, std::vector<std::string>> ||
                std::is_same_v<T, std::vector<double>>) {
    if (value->is<foxglove::ParameterValueView::Array>()) {
      const auto &array = value->get<foxglove::ParameterValueView::Array>();
      if (array.empty()) {
        parameter_set_callback_(name, T{});
        return true;
      }
      using U = inner_type<T>::type;
      T val;
      for (const auto &element : array) {
        if constexpr (std::is_same_v<U, std::string>) {
          if (!element.is<std::string_view>()) {
            return false;
          }
          val.push_back(std::string(element.get<std::string_view>()));
        } else if constexpr (std::is_same_v<U, double>) {
          if (!element.is<double>() && !element.is<std::int64_t>()) {
            return false;
          }
          val.push_back(element.get<double>());
        } else {
          return false;
        }
      }
      parameter_set_callback_(name, val);
      return true;
    }
  } else if (value->is<T>()) {
    auto val = value->get<T>();
    parameter_set_callback_(name, val);
    return true;
  }
  return false;
}

auto FoxgloveServer::get_parameter(const std::string &name)
    -> foxglove::Parameter {
  auto value = parameter_get_callback_(name);
  return std::visit(
      [&](auto &&v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::vector<std::string>>) {
          std::vector<foxglove::ParameterValue> param_values;
          param_values.reserve(v.size());
          for (auto &str : v) {
            param_values.emplace_back(std::string_view(str));
          }
          return foxglove::Parameter(
              name, foxglove::ParameterType::None,
              foxglove::ParameterValue(std::move(param_values)));
        } else {
          return foxglove::Parameter(name, v);
        }
      },
      value);
}
} // namespace framework
