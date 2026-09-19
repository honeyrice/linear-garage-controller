# Home Assistant

这些是可移植示例，需要先核对自己的实体 ID。适配时在此目录的 YAML、Jinja 和生成脚本中统一替换；只改一个文件会使统计或测试引用不一致。

## 实体映射

| 示例 ID | 来源 |
|---|---|
| `sensor.linear_garage_controller_gate_state` | ESPHome Gate State |
| `sensor.linear_garage_controller_motion` | ESPHome Motion |
| `sensor.linear_garage_controller_opening` | ESPHome Opening |
| `binary_sensor.linear_garage_controller_controller_online` | ESPHome Controller Online |
| `button.linear_garage_controller_open_gate` | Open Gate |
| `button.linear_garage_controller_close_gate` | Close Gate |
| `button.linear_garage_controller_gate_button` | Gate Button |
| `binary_sensor.garage_closed_contact` | 独立门磁，off=全关 |
| `notify.mobile_app_your_phone` | 自己的手机通知 action |

HA 实际自动生成的 ID 可能不同，以“开发者工具 → 状态”为准。用 ESPHome 集成加入设备后，允许该设备执行所需 HA 状态订阅，并确认全关参考可用。

## 安装模板

将以下三个文件放入 HA 的 `templates/` 目录：

- `garage_gate_controller.yaml`
- `linear_garage_cache.yaml`
- `linear_garage_statistics.yaml`

如使用目录合并方式，`configuration.yaml` 中为：

```yaml
template: !include_dir_merge_list templates/
homeassistant:
  packages: !include_dir_named packages/
```

如已有 `template` 或 `homeassistant` 配置，合并进去，不要重复顶层键。把 `linear_garage_statistics_package.yaml` 放入 `packages/`，并先修改通知 action。检查配置后重载模板和自动化，或按自己已有包含结构重载。

如果已经存在 `cover.garage_gate_controller`，替换原 cover 的定义，保留原来的 unique_id 与实体 ID；**不要同时加载两个定义**。新安装可使用示例中的 ID。这样现有自动化、仪表盘和 HomeKit 可继续引用同一 cover。

Cover 的 Open/Close 调用本地定向按钮；Stop 仅在 motion 为 opening/closing 时发原始按键。离线时 cover 不可用。没有实现任意百分比位置控制，显示开度不等于支持 `set_cover_position`。

## 缓存与统计

Last Known Gate State 只保存上一次有效状态、开度及记录时间；实时 unknown/unavailable 不覆盖缓存。缓存可能过时，不能据此判断当前安全状况或直接用于定向控制。

统计包含今日/累计开关运动起步次数、最近完整行程耗时、按方向学习的耗时基准、门保持未关闭的时长。计数依据观察到的运动，包括原厂按钮和中途重新启动；不是命令次数或完整往返次数。

完整行程才训练基准；中停、反转、离线和重启中断的行程不纳入正常样本。每个方向至少五次样本后启用慢行程提醒，取最近十次正常完整行程的中位数；同时超过基准 30% 且多出两秒才触发。提醒只通知，不自动关门；网络与停止检测延迟也计入测量，不能单凭提醒诊断机械故障。

生成统计 YAML：

```sh
python home-assistant/build_statistics.py
```

开门超过十分钟通知的可选示例为 `gate_left_open_package.yaml`，放入 packages 前先设置自己的通知 action。已有同类提醒时复用现有自动化，避免重复提醒。HA 的 `for` 计时会在重启/自动化重载时重置。

## HomeKit Bridge

在现有 HomeKit Bridge 的包含实体列表中加入 **`cover.garage_gate_controller`**。不要把原始按钮模拟成灯。若同一 cover 原本已导出且身份未变，后端替换通常无需重建桥接；仍需在 Apple Home 客户端实际确认状态和操作。

项目记录验证了 HA 侧 bridge 包含与原有 accessory 身份，未把 HA 截图当成 Apple Home 客户端验收。使用 HomeKit 时遵循其针对门锁/车库门的操作要求。
