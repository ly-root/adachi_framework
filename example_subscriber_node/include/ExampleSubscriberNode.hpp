#pragma once
#include "framework/Logger.hpp"
#include "framework/Node.hpp"
#include "framework/ParameterHandle.hpp"
#include "framework/Subscriber.hpp"
#include "interfaces/ExampleTypes.hpp"
#include "utilities/CvMatReflection.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <opencv2/core.hpp>
#include <string>
#include <thread>
#include <vector>

class ExampleSubscriberNode : public framework::Node {
public:
  ExampleSubscriberNode();
  ~ExampleSubscriberNode();

private:
  struct DisplayConfig {
    double interval;
    bool show_temperature;
    bool show_imu;
    bool show_detections;
    bool show_image;
  };

  struct ThresholdsConfig {
    struct TemperatureThreshold {
      double high;
      double low;
    } temperature;
    double detection_min_confidence;
  };

  struct Parameters {
    std::string node_name;
    std::vector<std::string> alert_targets;
    DisplayConfig display;
    ThresholdsConfig thresholds;
  } parameters_;

  static auto now_seconds() -> double;

  auto sync_atomics() -> void;
  auto setup_temperature_subscriber() -> void;
  auto setup_imu_subscriber() -> void;
  auto setup_detection_subscriber() -> void;
  auto setup_image_subscriber() -> void;
  auto display_loop(std::stop_token token) -> void;

  framework::Subscriber<TemperatureReading> temperature_sub_{
      "/example/temperature"};
  framework::Subscriber<ImuReading> imu_sub_{"/example/imu"};
  framework::Subscriber<DetectionArray> detection_sub_{"/example/detections"};
  framework::Subscriber<cv::Mat> image_sub_{"/example/image"};

  framework::ParameterHandle parameter_handle_{"example_subscriber_node"};
  framework::Logger logger_{"example_subscriber_node"};

  std::atomic<double> display_interval_{1.0};
  std::atomic<double> temperature_high_{30.0};
  std::atomic<double> temperature_low_{20.0};
  std::atomic<double> detection_min_confidence_{0.5};

  std::shared_ptr<const TemperatureReading> latest_temperature_;
  std::shared_ptr<const ImuReading> latest_imu_;
  std::shared_ptr<const DetectionArray> latest_detections_;
  std::shared_ptr<const cv::Mat> latest_image_;
  std::mutex data_mutex_;
  std::size_t total_messages_{0};
  std::size_t warnings_{0};

  std::jthread display_thread_;
};

#include "framework/NodeRegistrar.hpp"
REGISTER_NODE(ExampleSubscriberNode)
