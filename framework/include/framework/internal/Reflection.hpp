#pragma once
#include "framework/internal/ParameterTraits.hpp"
#include <boost/pfr.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace cv {
class Mat;
} // namespace cv

namespace framework {

class Reflect {
public:
  template <typename T, typename Func>
  static auto for_each_field(T &obj, Func &&func) -> void {
    boost::pfr::for_each_field_with_name(obj, std::forward<Func>(func));
  }

  template <typename T>
  static constexpr bool is_aggregate =
      std::is_aggregate_v<T> && !std::is_array_v<T> && !std::is_union_v<T>;

  template <typename T>
  static constexpr bool is_parameter = is_parameter_v<std::decay_t<T>>;

  template <typename T, typename = void>
  struct has_explicit_json : std::false_type {};

  template <typename T> static constexpr auto type_name() -> const char * {
    if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>)
      return "number";
    else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return "integer";
    else if constexpr (std::is_same_v<T, bool>)
      return "boolean";
    else if constexpr (std::is_same_v<T, std::string>)
      return "string";
    else
      return "object";
  }

  template <typename T>
  static auto serialize(nlohmann::json &j, const T &obj) -> void {
    if constexpr (is_aggregate<T>) {
      for_each_field(obj, [&](std::string_view name, const auto &field) {
        serialize(j[std::string(name)], field);
      });
    } else {
      if constexpr (std::is_same_v<std::decay_t<T>, cv::Mat> &&
                    !has_explicit_json<T>::value) {
        warn_skipped<T>();
        j = nullptr;
      } else if constexpr (requires { to_json(j, obj); }) {
        to_json(j, obj);
      } else if constexpr (requires {
                             obj.begin();
                             obj.end();
                           }) {
        j = nlohmann::json::array();
        for (const auto &elem : obj) {
          nlohmann::json elem_json;
          serialize(elem_json, elem);
          j.push_back(std::move(elem_json));
        }
      } else if constexpr (requires { nlohmann::json(std::declval<T>()); }) {
        j = obj;
      } else {
        warn_skipped<T>();
        j = nullptr;
      }
    }
  }

  template <typename T> static auto schema(nlohmann::json &json) -> void {
    if constexpr (std::is_same_v<std::decay_t<T>, cv::Mat> &&
                  !has_explicit_json<T>::value) {
      warn_skipped<T>();
      json = nullptr;
    } else if constexpr (is_aggregate<T>) {
      json["type"] = "object";
      nlohmann::json props;
      T obj{};
      for_each_field(obj, [&](std::string_view name, const auto &field) {
        using U = std::decay_t<decltype(field)>;
        schema<U>(props[std::string(name)]);
      });
      json["properties"] = std::move(props);
    } else if constexpr (requires {
                           std::declval<const T &>().begin();
                           std::declval<const T &>().end();
                         }) {
      using ElemT = typename T::value_type;
      json["type"] = "array";
      nlohmann::json items;
      schema<ElemT>(items);
      json["items"] = std::move(items);
    } else {
      json["type"] = type_name<T>();
    }
  }

private:
  template <typename T>
  [[deprecated("type has no JSON serialization")]]
  static void warn_skipped() {}
};

} // namespace framework
