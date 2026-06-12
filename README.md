# Adachi Framework

[![Language](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Build](https://img.shields.io/badge/build-xmake-brightgreen)](https://xmake.io)

轻量级 C++23 Robomaster自瞄中间件框架，提供节点化发布-订阅通信、Lua 配置管理与 Foxglove 可视化桥接。

## 特性

- **节点化架构** — 继承 `Node` + `REGISTER_NODE` 宏即可自动注册，无需中央注册表
- **类型安全 Pub/Sub** — `Publisher<T>` / `Subscriber<T>` 模板，支持任意消息类型
- **Lua 配置** — 每个节点独立 `config/config.lua`，支持嵌套表、数组、运行时热更新
- **Foxglove 可视化** — 自动将 Topic 桥接到 WebSocket（端口 8765），消息自动 JSON 序列化
- **编译期反射** — Boost.PFR 自动推导 struct 字段，无需手写 `to_json`/`from_json`
- **OpenCV 支持** — `cv::Mat` 自动 JPEG 压缩 + base64 编码，适配 Foxglove CompressedImage
- **结构化日志** — spdlog 控制台彩色输出 + 文件日志
- **现代 C++ 并发** — `std::jthread` + `std::stop_token` 实现优雅启停

## 依赖

| 组件 | 用途 |
|------|------|
| xmake | 构建系统 |
| C++23 编译器 (GCC 14+) | 语言标准 |
| sol2 3.5+ / Lua 5.4 | Lua 解析 |
| spdlog | 日志 |
| Foxglove SDK | WebSocket 可视化 |
| Boost.PFR | 编译期反射 |
| nlohmann/json | JSON 序列化 |
| OpenCV (可选) | 图像处理 |
| fmtlib | 格式化（与 spdlog 捆绑）|

依赖由 xmake 自动拉取。

## 快速开始

```bash
# 配置
xmake f --menu

# 编译
xmake

# 运行
xmake run 或 ./build/project
```

启动后节点自动运行，按 Ctrl+C 优雅退出。

### 启用/禁用节点

```bash
xmake f --example_publisher_node=false --example_subscriber_node=false
xmake
```

### 构建选项

```bash
xmake f --my_node=false   # 禁用特定节点
xmake f -m debug          # Debug 构建
xmake f -m asan           # Asan 构建
xmake f -m release        # Release 构建
xmake -j$(nproc)          # 并行编译
```

## 项目结构

```
├── xmake.lua                          # 根构建文件，自动发现所有节点目录
├── config/
│   ├── config.lua                     # 聚合加载各节点配置
│   └── foxglove.lua                   # Foxglove 配置（subscribe_topics 列表）
│
├── app/                               # 主入口
│   └── src/main.cpp
│
├── framework/                         # 核心框架
│   ├── include/framework/
│   │   ├── Node.hpp                   #   Node 基类
│   │   ├── Publisher.hpp              #   Publisher<T>
│   │   ├── Subscriber.hpp             #   Subscriber<T>
│   │   ├── Logger.hpp                 #   每节点日志（info/warn/error）
│   │   ├── ParameterHandle.hpp        #   参数绑定 + auto_bind
│   │   ├── NodeRegistrar.hpp          #   REGISTER_NODE 宏
│   │   └── internal/                  #   内部实现
│   └── src/
│
├── interfaces/                        # 共享消息类型定义
│   └── include/interfaces/ExampleTypes.hpp
│
├── utilities/                         # 工具（cv::Mat 序列化等）
│
├── example_publisher_node/            # 示例 — 传感器数据发布节点
├── example_subscriber_node/           # 示例 — 数据订阅/告警节点
│
├── third_party/                       # Foxglove SDK 预编译包
└── logs/                              # 运行时日志
```

## 编写自定义节点

### 1. 创建目录结构

```
my_node/
├── xmake.lua
├── config/
│   └── config.lua
├── include/
│   └── MyNode.hpp
└── src/
    └── MyNode.cpp
```

### 2. 定义消息类型（如需要）

```cpp
// interfaces/include/interfaces/MyTypes.hpp
struct MotorCommand {
  double left_speed;
  double right_speed;
  double duration;
};
```

### 3. 编写节点

```cpp
// my_node/include/MyNode.hpp
#pragma once
#include "framework/Logger.hpp"
#include "framework/Node.hpp"
#include "framework/ParameterHandle.hpp"
#include "framework/Publisher.hpp"
#include "framework/Subscriber.hpp"
#include "interfaces/MyTypes.hpp"

class MyNode : public framework::Node {
public:
  MyNode();
  ~MyNode();

private:
  struct Parameters {
    std::string node_name;
    double update_rate;
  } parameters_;

  framework::Publisher<MotorCommand> cmd_pub_{"/motor/command"};
  framework::Subscriber<TemperatureReading> temp_sub_{"/sensor/temperature"};
  framework::ParameterHandle param_handle_{"my_node"};
  framework::Logger logger_{"my_node"};
  std::jthread work_thread_;
};

#include "framework/NodeRegistrar.hpp"
REGISTER_NODE(MyNode)
```

```cpp
// my_node/src/MyNode.cpp
#include "MyNode.hpp"

MyNode::MyNode() {
  param_handle_.auto_bind_parameter(parameters_);
  logger_.info("initialized: node='{}', rate={}hz",
               parameters_.node_name, parameters_.update_rate);

  param_handle_.set_callback([this](auto &names) {
    for (auto &name : names) {
      param_handle_.apply_parameter_value(name);
    }
  });

  temp_sub_.on_receive([this](auto data) {
    logger_.info("temperature: {:.1f} °C", data->value);
  });

  work_thread_ = std::jthread([this](std::stop_token token) {
    while (!token.stop_requested()) {
      auto msg = std::make_unique<MotorCommand>();
      msg->left_speed = 0.5;
      msg->right_speed = 0.5;
      msg->duration = 1.0;
      cmd_pub_.publish(std::move(msg));
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
}

MyNode::~MyNode() {
  logger_.info("shutting down");
  work_thread_.request_stop();
  if (work_thread_.joinable())
    work_thread_.join();
}
```

### 4. 配置参数

```lua
-- my_node/config/config.lua
return {
    node_name = "my_node",
    update_rate = 10.0,
    thresholds = {
        max_temperature = 80.0
    }
}
```

### 5. 添加构建文件

`node_name` 由根 `xmake.lua` 自动设为当前节点目录名，直接使用即可。

```lua
-- my_node/xmake.lua
add_requires("opencv")     -- 如不需要可删除
target(node_name)
    add_files("src/*.cpp")
    add_includedirs("include")
    add_deps("framework")
    add_deps("interfaces")
    add_deps("utilities")  -- 如不需要可删除
    add_packages("opencv") -- 如不需要可删除
```

### 6. 编译运行

```bash
xmake run
```

节点会被自动发现、编译、并在运行时实例化。

## 架构

```
+-------------------------------------------------+
|  App (main.cpp)                                 |
|  Framework::instance().init() → stop()          |
+--+--------------------------------------------+-+
   |                                            |
+--v----------+  +--v-----------+  +-----------v--+
| TopicManager|  |ParameterMgr |  |FoxgloveServer |
| (pub/sub)   |  |(Lua config) |  |(WS :8765)    |
+------+------+  +------+------+  +-------+------+
       |                |                  |
+------v------+   +----v----+      +------v------+
| Publisher<T>|   |  Lua    |      |  Foxglove   |
| Subscriber<T>|  | Server  |      |  Client     |
+-------------+   +---------+      +-------------+

+----------+  +----------+  +----------+
| Node #1  |  | Node #2  |  | Node #N  |
| (pub)    |  | (sub)    |  | (pub/sub)|
+----------+  +----------+  +----------+
```

## 与 Foxglove 集成

1. 安装 [Foxglove Studio](https://foxglove.dev/studio)
2. 连接 `ws://localhost:8765`
3. 在 `config/foxglove.lua` 中配置要订阅的 Topic：

```lua
return { subscribe_topics = {
    "/example/temperature",
    "/example/imu",
    "/example/image",
    "/motor/command"
} }
```

重启后 Topic 数据会实时推送到 Foxglove 面板，参数也可在 Foxglove 中调谐。

## 日志

日志自动输出到控制台（彩色）和文件：

```
logs/2026-06-12_01_52_22.log
```

格式：`[时间] [级别] [节点名] 消息`

## 注意事项

### 参数类型约束

`ParameterHandle` 仅支持以下类型及其嵌套组合结构体：

| 支持类型 | 说明 |
|---------|------|
| `double` | 浮点数 |
| `bool` | 布尔值 |
| `std::string` | 字符串 |
| `std::vector<double>` | 浮点数数组 |
| `std::vector<std::string>` | 字符串数组 |

结构体可以嵌套，但最终字段类型必须属于上述集合。例如：

```cpp
// ✅ 合法
struct SensorConfig {
  double rate;
  bool enabled;
  std::string frame_id;
};
struct Parameters {
  double base_value;
  SensorConfig sensor;
  std::vector<double> calibration;
};

// ❌ 非法 — int 不在支持类型中
struct Parameters {
  int count;       // 编译期 static_assert 报错
  uint64_t id;     // 也不支持
};
```

### 参数绑定与回调必须在构造函数

`auto_bind_parameter()` 和 `set_callback()` 必须在节点构造函数内调用，否则参数无法正确映射到 Lua 配置。

`set_callback` 的作用是监听 Lua 配置文件热更新：框架会检测文件变化并更新内部 `config_map_`，但**不会自动同步到结构体字段**，需要在 callback 中手动调用 `apply_parameter_value()` 来应用变更：

```cpp
MyNode::MyNode() {
  param_handle_.auto_bind_parameter(parameters_);  // ✅ 构造函数内绑定
  param_handle_.set_callback([this](auto &names) {  // ✅ 构造函数内注册
    for (auto &name : names) {
      param_handle_.apply_parameter_value(name);  // 应用热更新
    }
  });
}
```

未注册 callback 的节点对配置变更无感知。

### 自定义 Topic 类型必须有默认构造函数

```cpp
// ✅ 合法
struct MotorCommand {
  double left_speed{};    // 默认初始化
  double right_speed{};
};

// ✅ 也合法
struct Status {
  std::string message;
  // 编译器隐式生成默认构造函数
};

// ❌ 非法 — 没有默认构造函数
struct Config {
  Config(int v) : value(v) {}
  int value;
};
```

### Foxglove 序列化

简单 aggregate 类型（仅含基本类型 + 嵌套 struct）由 Boost.PFR 编译期自动序列化，**无需额外代码**。

若类型包含非 PFR 内置支持的数据（如 `cv::Mat`、自定义容器等），需要提供 `to_json(nlohmann::json&, const T&)` 重载：

```cpp
// utilities/CvMatReflection.hpp 中的示例
void to_json(nlohmann::json &j, const cv::Mat &mat) {
  // 自定义序列化逻辑：JPEG 压缩 → base64 编码
}
```

未提供 `to_json` 的复杂类型在序列化时会触发编译期 `deprecated` 警告，并在运行时的 Foxglove 数据中输出 `null`。

### Subscriber 生命周期

`on_receive()` 创建的 `std::jthread` 由 `TopicManager` 托管，Subscriber 析构时自动调用 `unsubscribe()` 停止线程。

避免将 Subscriber 声明为全局或静态对象，因为其析构顺序可能晚于 `Framework` 单例，导致访问已释放的 `TopicManager`。

### 参数前缀必须等于节点目录名

```cpp
framework::ParameterHandle param_handle_{"example_publisher_node"};
```

构造参数必须与**节点目录名**一致（即 xmake 中的 `node_name` 变量），否则 `get_value()` 查找键（`example_publisher_node.temperature.rate`）时会抛异常。

### config/config.lua 不可缺失

每个节点目录下必须有 `config/config.lua`（可返回空表）。如果节点绑定了参数但对应的 Lua 文件缺失或返回空，`apply_parameter_value()` 会因找不到键而抛异常。

### Lua 数组类型推断

框架按 `double → string` 优先级推断 Lua 数组类型：

```lua
return {
    rates = { 1.0, 2.0, 3.0 },  -- → std::vector<double>
    labels = { "a", "b" },       -- → std::vector<std::string>
}
```

混合类型数组不被支持。Lua 中的整数字面量会被 Lua 自身转为 `double`，C++ 侧始终收到 `double`。

### REGISTER_NODE 依赖静态链接

`REGISTER_NODE` 宏在静态初始化阶段将工厂函数注册到 `NodeRegistry`。所有节点必须**静态链接**到最终二进制（当前构建方式如此），无法作为动态库加载。

### 日志文件不自动轮转

每次启动创建新的日志文件（`logs/YYYY-MM-DD_HH_MM_SS.log`），不会覆盖旧日志，但也不会自动删除或归档。长时间运行需自行处理日志清理。
