#pragma once
#include "framework/internal/Reflection.hpp"
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

namespace framework {
template <> struct Reflect::has_explicit_json<cv::Mat> : std::true_type {};
template <> inline void Reflect::schema<cv::Mat>(nlohmann::json &j) {
  j = {
      {"type", "object"},
      {"title", "foxglove::CompressedImage"},
      {"properties",
       {{"timestamp",
         {{"type", "object"},
          {"properties",
           {{"sec", {{"type", "integer"}}}, {"nsec", {{"type", "integer"}}}}}}},
        {"frame_id", {{"type", "string"}}},
        {"data", {{"type", "string"}, {"contentEncoding", "base64"}}},
        {"format", {{"type", "string"}}}}}};
}
static inline auto base64_encode(const std::vector<uchar> &data)
    -> std::string {
  static const char *table =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((data.size() + 2) / 3) * 4);
  for (size_t i = 0; i < data.size(); i += 3) {
    uint32_t v =
        (static_cast<uint32_t>(data[i]) << 16) |
        (i + 1 < data.size() ? static_cast<uint32_t>(data[i + 1]) << 8 : 0) |
        (i + 2 < data.size() ? static_cast<uint32_t>(data[i + 2]) : 0);
    out += table[(v >> 18) & 0x3F];
    out += table[(v >> 12) & 0x3F];
    out += (i + 1 < data.size()) ? table[(v >> 6) & 0x3F] : '=';
    out += (i + 2 < data.size()) ? table[v & 0x3F] : '=';
  }
  return out;
}
} // namespace framework

namespace cv {
inline void to_json(nlohmann::json &j, const cv::Mat &mat) {
  std::vector<uchar> buf;
  cv::imencode(".jpeg", mat, buf);
  auto now = std::chrono::system_clock::now();
  auto secs =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();
  auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   now.time_since_epoch())
                   .count() %
               1000000000;
  j = {{"timestamp", {{"sec", secs}, {"nsec", nsecs}}},
       {"frame_id", "camera"},
       {"data", framework::base64_encode(buf)},
       {"format", "jpeg"}};
}
} // namespace cv
