#include "ExampleSubscriberNode.hpp"

auto ExampleSubscriberNode::now_seconds() -> double {
  return std::chrono::duration<double>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

auto ExampleSubscriberNode::sync_atomics() -> void {
  display_interval_.store(parameters_.display.interval);
  temperature_high_.store(parameters_.thresholds.temperature.high);
  temperature_low_.store(parameters_.thresholds.temperature.low);
  detection_min_confidence_.store(
      parameters_.thresholds.detection_min_confidence);
}

ExampleSubscriberNode::ExampleSubscriberNode() {
  parameter_handle_.auto_bind_parameter(parameters_);
  sync_atomics();

  logger_.info(
      "initialized: node='{}', interval={:.1f}s, "
      "warn=({:.1f}..{:.1f}), min_conf={:.2f}, "
      "show=[t:{} imu:{} det:{} img:{}]",
      parameters_.node_name, display_interval_.load(), temperature_low_.load(),
      temperature_high_.load(), detection_min_confidence_.load(),
      parameters_.display.show_temperature, parameters_.display.show_imu,
      parameters_.display.show_detections, parameters_.display.show_image);

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

  setup_temperature_subscriber();
  setup_imu_subscriber();
  setup_image_subscriber();
  setup_detection_subscriber();

  display_thread_ =
      std::jthread([this](std::stop_token token) { display_loop(token); });
}

ExampleSubscriberNode::~ExampleSubscriberNode() {
  logger_.info("shutting down");
  display_thread_.request_stop();
  if (display_thread_.joinable())
    display_thread_.join();
}

auto ExampleSubscriberNode::setup_temperature_subscriber() -> void {
  temperature_sub_.on_receive(
      [this](std::shared_ptr<const TemperatureReading> data) {
        std::lock_guard lock(data_mutex_);
        latest_temperature_ = data;
        total_messages_++;

        double high = temperature_high_.load();
        double low = temperature_low_.load();
        if (data->value > high) {
          logger_.warn("HIGH TEMPERATURE: {:.1f} °C (threshold: {:.1f})",
                       data->value, high);
          warnings_++;
        } else if (data->value < low) {
          logger_.warn("LOW TEMPERATURE: {:.1f} °C (threshold: {:.1f})",
                       data->value, low);
          warnings_++;
        }
      });
}

auto ExampleSubscriberNode::setup_imu_subscriber() -> void {
  imu_sub_.on_receive([this](std::shared_ptr<const ImuReading> data) {
    std::lock_guard lock(data_mutex_);
    latest_imu_ = data;
    total_messages_++;
  });
}

auto ExampleSubscriberNode::setup_image_subscriber() -> void {
  image_sub_.on_receive([this](std::shared_ptr<const cv::Mat> data) {
    std::lock_guard lock(data_mutex_);
    latest_image_ = data;
    total_messages_++;
  });
}

auto ExampleSubscriberNode::setup_detection_subscriber() -> void {
  detection_sub_.on_receive([this](std::shared_ptr<const DetectionArray> data) {
    std::lock_guard lock(data_mutex_);
    latest_detections_ = data;
    total_messages_++;

    double min_conf = detection_min_confidence_.load();
    for (const auto &det : data->detections) {
      if (det.confidence < min_conf) {
        logger_.warn("low confidence detection: '{}' ({:.2f} < {:.2f})",
                     det.label, det.confidence, min_conf);
        warnings_++;
      }
    }
  });
}

auto ExampleSubscriberNode::display_loop(std::stop_token token) -> void {
  while (!token.stop_requested()) {
    double interval = display_interval_.load();
    if (interval <= 0.0)
      interval = 1.0;
    std::this_thread::sleep_for(std::chrono::duration<double>(interval));
    if (token.stop_requested())
      break;

    std::lock_guard lock(data_mutex_);
    logger_.info("=== Summary ({} msgs, {} warnings) ===", total_messages_,
                 warnings_);

    if (latest_temperature_ && parameters_.display.show_temperature) {
      logger_.info("  Temperature: {:.1f} °C", latest_temperature_->value);
    }
    if (latest_imu_ && parameters_.display.show_imu) {
      logger_.info("  IMU: orientation=({:.2f}, {:.2f}, {:.2f}), "
                   "accel=({:.2f}, {:.2f}, {:.2f})",
                   latest_imu_->orientation.x, latest_imu_->orientation.y,
                   latest_imu_->orientation.z,
                   latest_imu_->linear_acceleration.x,
                   latest_imu_->linear_acceleration.y,
                   latest_imu_->linear_acceleration.z);
    }
    if (latest_detections_ && parameters_.display.show_detections) {
      for (const auto &det : latest_detections_->detections) {
        logger_.info("  Detection: '{}' ({:.2f}) at ({:.0f}, {:.0f})",
                     det.label, det.confidence, det.box.x, det.box.y);
      }
    }
    if (latest_image_ && parameters_.display.show_image) {
      logger_.info("  Image: {}x{} {}ch", latest_image_->cols,
                   latest_image_->rows, latest_image_->channels());
    }
  }
}
