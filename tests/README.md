# 离线测试

在仓库根目录安装 requirements.txt 后运行：

```sh
python tests/run_tests.py
```

需要 C++17 编译器、Python、Jinja2 和 PyYAML。脚本只创建临时测试可执行文件，不访问网络、串口、HA 或继电器。

- `gate_controller_v2_test.cpp` / `gate_control_sim.h`：实际头文件算法驱动的确定性门模型，覆盖重复目标、快速反向、端点、排队与错误处理。
- `position_tracker_test.cpp`：启动、校准、过期门磁、计数越界、时间回绕及现场计数回放。
- `field-counts.txt`：首次实际往返记录中提取的 301 个 COUNT 值。删除了时间、启动与网络信息。测试按 500 ms 采样间隔回放，并模拟门磁事件；不声称原日志同时记录了门磁时间。
- `home-assistant/test_statistics.py`：19 个统计场景。
- `home-assistant/test_cover_migration.py`：8 个 cover 状态/路由场景。

测试使用本机行程 1049 作为固定样例。修改行程或控制参数后需相应调整模型和回放预期。通过结果是软件证据，不能替代真实门动作、硬件复位瞬态、打印适配和长期稳定性验证。

macOS 若 Command Line Tools 报标准头文件（如 `cstdint`）找不到，可先修复开发工具选择，或为本次运行设置：

```sh
CPLUS_INCLUDE_PATH="$(xcrun --show-sdk-path)/usr/include/c++/v1" python tests/run_tests.py
```
