#pragma once
#include "framework/Node.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
namespace framework {

class NodeRegistry {
public:
  NodeRegistry() = default;
  static auto instance() -> NodeRegistry &;
  auto register_node(const std::string &name,
                     std::function<std::unique_ptr<Node>()> factory) -> void;
  auto start_nodes() -> std::vector<std::unique_ptr<Node>>;

private:
  std::unordered_map<std::string, std::function<std::unique_ptr<Node>()>>
      registry_;
};
} // namespace framework
