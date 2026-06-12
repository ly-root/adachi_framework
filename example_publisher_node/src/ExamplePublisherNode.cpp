#include "ExamplePublisherNode.hpp"
#include <cmath>
#include <opencv2/imgproc.hpp>

auto ExamplePublisherNode::now_seconds() -> double {
  return std::chrono::duration<double>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

auto ExamplePublisherNode::sync_atomics() -> void {
  temperature_rate_.store(parameters_.temperature.rate);
  imu_rate_.store(parameters_.imu.rate);
  detection_rate_.store(parameters_.detection.rate);
  image_rate_.store(parameters_.image.rate);
}

ExamplePublisherNode::ExamplePublisherNode() {
  parameter_handle_.auto_bind_parameter(parameters_);
  sync_atomics();

  logger_.info(
      "initialized: node='{}', temperature={:.1f}±{:.1f}°C @ {:.1f}hz, "
      "imu @ {:.1f}hz, detections @ {:.1f}hz, image @ {:.1f}hz",
      parameters_.node_name, parameters_.temperature.base,
      parameters_.temperature.amplitude, temperature_rate_.load(),
      imu_rate_.load(), detection_rate_.load(), image_rate_.load());

  parameter_handle_.set_callback([this](const std::vector<std::string> &names) {
    for (const auto &name : names) {
      try {
        parameter_handle_.apply_parameter_value(name);
        logger_.info("'{}' updated", name);
      } catch (const std::exception &e) {
        logger_.error("failed to apply parameter '{}': {}", name, e.what());
      }
    }
    sync_atomics();
  });

  temperature_thread_ = std::jthread(
      [this](std::stop_token token) { publish_temperature(token); });
  imu_thread_ =
      std::jthread([this](std::stop_token token) { publish_imu(token); });
  detection_thread_ = std::jthread(
      [this](std::stop_token token) { publish_detections(token); });
  image_thread_ =
      std::jthread([this](std::stop_token token) { publish_image(token); });
}

ExamplePublisherNode::~ExamplePublisherNode() {
  logger_.info("shutting down");
  temperature_thread_.request_stop();
  imu_thread_.request_stop();
  detection_thread_.request_stop();
  image_thread_.request_stop();
  if (temperature_thread_.joinable())
    temperature_thread_.join();
  if (imu_thread_.joinable())
    imu_thread_.join();
  if (detection_thread_.joinable())
    detection_thread_.join();
  if (image_thread_.joinable())
    image_thread_.join();
}

auto ExamplePublisherNode::publish_temperature(std::stop_token token) -> void {
  while (!token.stop_requested()) {
    double t = now_seconds();
    double rate = temperature_rate_.load();
    auto interval =
        std::chrono::duration<double>(rate > 0.0 ? 1.0 / rate : 1.0);
    auto msg = std::make_unique<TemperatureReading>();
    msg->value = parameters_.temperature.base +
                 parameters_.temperature.amplitude * std::sin(t * 0.5);
    msg->unit = parameters_.temperature.unit;
    temperature_pub_.publish(std::move(msg));
    std::this_thread::sleep_for(interval);
  }
}

auto ExamplePublisherNode::publish_imu(std::stop_token token) -> void {
  while (!token.stop_requested()) {
    double t = now_seconds();
    double rate = imu_rate_.load();
    auto interval =
        std::chrono::duration<double>(rate > 0.0 ? 1.0 / rate : 1.0);
    double angle = t * 0.3;
    auto msg = std::make_unique<ImuReading>();
    msg->orientation =
        Vector3{std::sin(angle) * 0.5, 0.0, std::cos(angle) * 0.5};
    msg->angular_velocity = Vector3{0.1, 0.0, 0.3 * std::cos(angle)};
    msg->linear_acceleration = Vector3{0.0, 0.0, 9.81};
    msg->timestamp = t;
    imu_pub_.publish(std::move(msg));
    std::this_thread::sleep_for(interval);
  }
}

auto ExamplePublisherNode::publish_detections(std::stop_token token) -> void {
  while (!token.stop_requested()) {
    double t = now_seconds();
    double rate = detection_rate_.load();
    auto interval =
        std::chrono::duration<double>(rate > 0.0 ? 1.0 / rate : 1.0);
    auto msg = std::make_unique<DetectionArray>();
    {
      Detection d;
      d.label = "person";
      d.confidence = 0.85 + 0.1 * std::sin(t * 0.2);
      d.box = BoundingBox{100.0, 200.0, 50.0, 120.0};
      msg->detections.push_back(std::move(d));
    }
    {
      Detection d;
      d.label = "car";
      d.confidence = 0.92 + 0.05 * std::cos(t * 0.15);
      d.box = BoundingBox{300.0, 150.0, 180.0, 100.0};
      msg->detections.push_back(std::move(d));
    }
    msg->timestamp = t;
    detection_pub_.publish(std::move(msg));
    std::this_thread::sleep_for(interval);
  }
}

auto ExamplePublisherNode::publish_image(std::stop_token token) -> void {
  size_t tick = 0;
  while (!token.stop_requested()) {
    double t = now_seconds();
    double rate = image_rate_.load();
    auto interval =
        std::chrono::duration<double>(rate > 0.0 ? 1.0 / rate : 1.0);
    auto msg = std::make_unique<cv::Mat>(
        static_cast<int>(parameters_.image.resolution.height),
        static_cast<int>(parameters_.image.resolution.width), CV_8UC3);
    double temp = parameters_.temperature.base +
                  parameters_.temperature.amplitude * std::sin(t * 0.5);
    int hue = static_cast<int>(t * 10) % 180;
    cv::Mat hsv(static_cast<int>(parameters_.image.resolution.height),
                static_cast<int>(parameters_.image.resolution.width), CV_8UC3,
                cv::Scalar(hue, 180, 220));
    cv::cvtColor(hsv, *msg, cv::COLOR_HSV2BGR);

    cv::circle(
        *msg,
        cv::Point(static_cast<int>(parameters_.image.resolution.width / 2),
                  static_cast<int>(parameters_.image.resolution.height / 2)),
        80, cv::Scalar(0, 0, 0), 4);
    cv::circle(
        *msg,
        cv::Point(static_cast<int>(parameters_.image.resolution.width / 2 +
                                   80 * std::cos(t)),
                  static_cast<int>(parameters_.image.resolution.height / 2 +
                                   80 * std::sin(t))),
        12, cv::Scalar(255, 255, 255), -1);

    cv::putText(*msg, "temp: " + std::to_string(static_cast<int>(temp)) + "C",
                cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.8,
                cv::Scalar(255, 255, 255), 2);
    cv::putText(*msg, "frame: " + std::to_string(tick), cv::Point(20, 460),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(200, 200, 200), 1);
    image_pub_.publish(std::move(msg));
    std::this_thread::sleep_for(interval);
    tick++;
  }
}
