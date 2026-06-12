#pragma once
#include <string>
#include <vector>

struct Vector3 {
  double x, y, z;
};

struct TemperatureReading {
  double value;
  std::string unit;
};

struct ImuReading {
  Vector3 angular_velocity;
  Vector3 linear_acceleration;
  Vector3 orientation;
  double timestamp;
};

struct BoundingBox {
  double x, y, width, height;
};

struct Detection {
  std::string label;
  double confidence;
  BoundingBox box;
};

struct DetectionArray {
  std::vector<Detection> detections;
  double timestamp;
};
