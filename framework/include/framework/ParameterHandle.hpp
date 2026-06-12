#pragma once
#include "framework/internal/ParameterTraits.hpp"
#include "framework/internal/Reflection.hpp"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace framework {

class ParameterHandle {
public:
  ParameterHandle(const std::string &prefix);
  auto bind_parameter(const std::string &name, ParameterReference param)
      -> void;
  auto apply_parameter_value(const std::string &name) -> void;
  auto set_callback(
      std::function<void(const std::vector<std::string> &names)> callback)
      -> void;

  template <typename T> auto auto_bind_parameter(T &params) -> void {
    auto_bind_parameter_impl(params, "");
  }

private:
  template <typename T>
  auto auto_bind_parameter_impl(T &params, std::string_view prefix) -> void {
    Reflect::for_each_field(
        params, [this, prefix](std::string_view name, auto &field) {
          std::string full_name =
              prefix.empty() ? std::string(name)
                             : std::string(prefix) + "." + std::string(name);
          using U = std::decay_t<decltype(field)>;
          if constexpr (Reflect::is_parameter<U>) {
            bind_parameter(full_name, std::ref(field));
          } else if constexpr (Reflect::is_aggregate<U>) {
            auto_bind_parameter_impl(field, full_name);
          } else {
            static_assert(!std::is_same_v<U, U>,
                          "unsupported parameter type for parameter");
          }
        });
  }
  std::unordered_map<std::string, ParameterReference> parameter_bind_;
  std::string prefix_;
};
} // namespace framework
