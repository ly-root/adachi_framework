#include "framework/internal/NodeRegistry.hpp"
namespace framework {

auto NodeRegistry::instance() -> NodeRegistry & {
  static NodeRegistry instance;
  return instance;
}

auto NodeRegistry::register_node(const std::string &name,
                                 std::function<std::unique_ptr<Node>()> factory)
    -> void {
  registry_[name] = factory;
}

auto NodeRegistry::start_nodes() -> std::vector<std::unique_ptr<Node>> {
  auto nodes = std::vector<std::unique_ptr<Node>>();
  for (const auto &[name, func] : registry_) {
    nodes.push_back(func());
  }
  return nodes;
}
} // namespace framework
