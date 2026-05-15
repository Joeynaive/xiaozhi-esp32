# 小智 AI 聊天机器人项目代码结构

本项目基于乐鑫 ESP-IDF 框架使用 C/C++ 编写。以下是项目核心代码结构的详细说明，重点剖析了 `main/` 源码目录的内容。

```text
xiaozhi-esp32/
├── docs/                      # 项目相关文档、图片、教程资源
├── partitions/                # ESP32 的分区表文件 (用于 OTA、NVS 等)
├── scripts/                   # 构建和自动化相关的实用脚本
├── sdkconfig.defaults.*       # 针对各款芯片 (ESP32-S3/C3/C5/C6/P4) 的默认编译配置
├── CMakeLists.txt             # ESP-IDF 根目录 CMake 构建脚本
├── README.md                  # 各语言版本的自述文件
└── main/                      # 核心业务逻辑代码目录 (★ 核心)
    ├── application.cc/h       # 应用的主入口和核心调度类，集成所有子系统
    ├── device_state_machine.* # 设备状态机 (管理 idle, listening, thinking, speaking 等状态流转)
    ├── mcp_server.cc/h        # MCP (Model Context Protocol) 协议的服务端实现，允许大模型控制硬件
    ├── ota.cc/h               # OTA (Over-The-Air) 固件无线升级逻辑
    ├── settings.cc/h          # 设备的本地配置和参数存储模块
    ├── system_info.cc/h       # 读取设备硬件信息 (MAC, 内存, 电量等)
    │
    ├── audio/                 # 音频子系统
    │   ├── audio_service.*    # 音频核心服务 (整合录音、播放、唤醒)
    │   ├── codecs/            # 音频编解码器 (如 OPUS 编解码)
    │   ├── wake_words/        # 离线语音唤醒词模型配置
    │   ├── processors/        # 音频数据处理器 (如重采样等)
    │   └── demuxer/           # 音频解复用器
    │
    ├── display/               # 显示子系统
    │   ├── display.*          # 显示基类
    │   ├── lcd_display.*      # LCD 屏幕驱动
    │   ├── oled_display.*     # OLED 屏幕驱动
    │   ├── emote_display.*    # 表情显示逻辑
    │   └── lvgl_display/      # 基于 LVGL 的图形界面 (GUI) 封装
    │
    ├── protocols/             # 网络通信协议层 (与服务器的数据交互)
    │   ├── protocol.*         # 通信协议基类定义
    │   ├── websocket_protocol.* # 基于 WebSocket 的通信实现
    │   └── mqtt_protocol.*    # 基于 MQTT (信令) + UDP (音频流) 的通信实现
    │
    ├── led/                   # LED 状态灯控制模块
    │
    ├── assets/                # 内置的静态资源文件 (如内置的默认字体、表情资源等)
    │
    └── boards/                # 硬件抽象层 (HAL)
        ├── common/            # 各类开发板共用的底层驱动和基础配置
        ├── esp-box/           # 官方 ESP-BOX 系列配置
        ├── m5stack-*/         # M5Stack 系列开发板配置
        ├── lilygo-*/          # LILYGO 系列开发板配置
        └── ...                # 约包含 100 种市面上各种第三方 ESP32 开发板的特定管脚、外设配置文件
```

## 核心设计理念

1. **面向对象的模块化设计**：项目将音频、显示、网络、LED等功能模块高度解耦。由 `Application` 类在 `main.cc` 中进行组装和初始化。
2. **状态机驱动**：设备的各种行为（待机、唤醒、聆听、思考、播放）通过 `DeviceStateMachine` 严格控制流转，确保各个子系统（如屏幕显示、LED状态、录音开关）能根据当前状态做出正确的响应。
3. **出色的硬件抽象 (`boards/`)**：这是本项目能适配如此多硬件的关键。每个不同的开发板只需在 `boards` 下新建自己的目录，继承并配置特定的麦克风、扬声器、屏幕管脚和型号，即可无缝接入主流程。
4. **灵活的协议切换 (`protocols/`)**：抽象了 `Protocol` 接口，使得项目可以通过后台配置，在 WebSocket 模式和低延迟的 MQTT+UDP 模式之间自由切换，而不影响业务层逻辑。
