#pragma once
#include <concepts>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace framework {

using ParameterValue =
    std::variant<double, bool, std::string, std::vector<double>,
                 std::vector<std::string>>;

template <typename T>
concept ParameterType =
    std::same_as<T, double> || std::same_as<T, bool> ||
    std::same_as<T, std::string> || std::same_as<T, std::vector<double>> ||
    std::same_as<T, std::vector<std::string>>;

using ParameterReference =
    std::variant<std::reference_wrapper<double>, std::reference_wrapper<bool>,
                 std::reference_wrapper<std::string>,
                 std::reference_wrapper<std::vector<double>>,
                 std::reference_wrapper<std::vector<std::string>>>;

template <typename T> struct inner_type {
  using type = T;
};
template <typename U> struct inner_type<std::vector<U>> {
  using type = U;
};

template <typename T> inline constexpr bool is_parameter_v = ParameterType<T>;

} // namespace framework
