#include "framework/ParameterHandle.hpp"
#include "framework/internal/Framework.hpp"
#include "framework/internal/ParameterManager.hpp"
#include <type_traits>
namespace framework {

template <typename T>
static constexpr bool is_parameter_vector =
    std::is_same_v<std::decay_t<T>, std::vector<double>> ||
    std::is_same_v<std::decay_t<T>, std::vector<std::string>>;

ParameterHandle::ParameterHandle(const std::string &prefix) : prefix_(prefix) {}

auto ParameterHandle::bind_parameter(const std::string &name,
                                     ParameterReference param) -> void {
  parameter_bind_.emplace(name, param);
  framework::Framework::instance().parameter_manager().listen_parameter(prefix_,
                                                                        name);
  apply_parameter_value(name);
}

auto ParameterHandle::apply_parameter_value(const std::string &name) -> void {
  auto it = parameter_bind_.find(name);
  if (it == parameter_bind_.end()) {
    throw std::runtime_error("Parameter not found: " + prefix_ + "." + name);
  }
  auto value =
      Framework::instance().parameter_manager().get_value(prefix_ + "." + name);
  std::visit(
      [&](auto &ref, const auto &val) {
        using RefType = std::remove_cvref_t<decltype(ref.get())>;
        using ValType = std::decay_t<decltype(val)>;
        const auto full_name = prefix_ + "." + name;

        if constexpr (std::is_same_v<RefType, ValType>) {
          ref.get() = val;
        } else if constexpr (is_parameter_vector<RefType> &&
                             is_parameter_vector<ValType>) {
          if (val.empty()) {
            ref.get() = RefType{};
            Framework::instance().parameter_manager().set_value(full_name,
                                                                RefType{});
          } else {
            throw std::runtime_error("Type mismatch for parameter: " +
                                     full_name);
          }
        } else {
          throw std::runtime_error("Type mismatch for parameter: " + full_name);
        }
      },
      it->second, value);
}
auto ParameterHandle::set_callback(
    std::function<void(const std::vector<std::string> &names)> callback)
    -> void {
  Framework::instance().parameter_manager().add_update_listener(prefix_,
                                                                callback);
}

} // namespace framework
