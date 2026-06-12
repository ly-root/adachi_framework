#pragma once
#include "framework/Node.hpp"
#include "framework/internal/NodeRegistry.hpp"
#include <functional>
#include <memory>
#include <string>
namespace framework {
class NodeRegistrar {
public:
  NodeRegistrar(const std::string &name,
                std::function<std::unique_ptr<Node>()> factory) {
    NodeRegistry::instance().register_node(name, factory);
  }
};

#define REGISTER_NODE(TYPE)                                                    \
  __attribute__((used)) inline framework::NodeRegistrar node_registrar_##TYPE( \
      #TYPE, [] { return std::make_unique<TYPE>(); });
} // namespace framework
