#pragma once
#include "framework/Logger.hpp"
#include "framework/Node.hpp"
#include "framework/ParameterHandle.hpp"
#include "framework/Publisher.hpp"
#include "interfaces/ExampleTypes.hpp"
#include "utilities/CvMatReflection.hpp"
#include <atomic>
#include <opencv2/core.hpp>
#include <string>
#include <thread>
#include <vector>

class ExamplePublisherNode : public framework::Node {
public:
  ExamplePublisherNode();
  ~ExamplePublisherNode();

private:
  struct TemperatureConfig {
    double rate;
    double base;
    double amplitude;
    std::string unit;
  };

  struct SensorConfig {
    double rate;
    bool enabled;
    std::string frame_id;
  };

  struct ImageConfig {
    struct Resolution {
      double width;
      double height;
    } resolution;
    double rate;
    bool flip_horizontal;
  };

  struct Parameters {
    std::string node_name;
    std::vector<double> calibration;
    std::vector<std::string> tags;
    TemperatureConfig temperature;
    SensorConfig imu;
    SensorConfig detection;
    ImageConfig image;
  } parameters_;

  static auto now_seconds() -> double;

  auto sync_atomics() -> void;
  auto publish_temperature(std::stop_token token) -> void;
  auto publish_imu(std::stop_token token) -> void;
  auto publish_detections(std::stop_token token) -> void;
  auto publish_image(std::stop_token token) -> void;

  framework::Publisher<TemperatureReading> temperature_pub_{
      "/example/temperature"};
  framework::Publisher<ImuReading> imu_pub_{"/example/imu"};
  framework::Publisher<DetectionArray> detection_pub_{"/example/detections"};
  framework::Publisher<cv::Mat> image_pub_{"/example/image"};

  framework::ParameterHandle parameter_handle_{"example_publisher_node"};
  framework::Logger logger_{"example_publisher_node"};

  std::atomic<double> temperature_rate_{1.0};
  std::atomic<double> imu_rate_{1.0};
  std::atomic<double> detection_rate_{1.0};
  std::atomic<double> image_rate_{1.0};

  std::jthread temperature_thread_;
  std::jthread imu_thread_;
  std::jthread detection_thread_;
  std::jthread image_thread_;
};

#include "framework/NodeRegistrar.hpp"
REGISTER_NODE(ExamplePublisherNode)
