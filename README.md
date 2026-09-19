# Linear Garage Controller

[English](#english) | [简体中文](#简体中文)

## English

ESP32 + ESPHome retrofit for a **Linear LDCO800** garage opener: encoder-based motion and estimated position, feedback-driven Open/Close control, Home Assistant integration, and Bluetooth Proxy.

**Completed: 2026-09-18.** The original opener, motor, remotes, wall button, and factory protections are retained. The module reads the factory encoder and simulates button presses through a relay. The ESP32 locally translates Open / Close intentions into button sequences driven by feedback.

### Project photos

| Opener model | Factory logic board |
|---|---|
| <img src="docs/images/01-gate-model.jpg" alt="Linear LDCO800 model nameplate" width="420"> | <img src="docs/images/02-gate-logic-board.jpg" alt="Linear LDCO800 factory logic board" width="420"> |
| **Linear LDCO800** model nameplate and electrical specifications. | Original logic board and power section, retaining the encoder connection. |

| Components in the 3D-printed box | Finished module |
|---|---|
| <img src="docs/images/03-components-in-box.jpg" alt="ESP32, level converter, and relay inside the printed enclosure" width="420"> | <img src="docs/images/04-finished-module.jpg" alt="Closed controller module and external leads" width="420"> |
| ESP32 + level converter + 3.3 V relay. | Encoder Y cable, dry-contact leads, and mounting ears. |

These are actual project photographs. The model and board photographs received ordinary cropping; the other two retain their original framing. Camera metadata was removed from all four. No AI redrawing, retouching, or added annotations were used.

### Features

- **Encoder feedback:** GPIO22/23 read the two encoder channels to detect opening / closing / stopped and estimate opening percentage.
- **Closed-position reference:** an independent fully closed contact sensor provides a reference through HA; a stable confirmation recalibrates zero.
- **Directional control:** Open Gate / Close Gate prioritize the latest target; Gate Button remains available for a raw single press.
- **Home Assistant:** a HA `cover`, last-known-state cache, motion-start counts, travel durations, and abnormal-duration notifications.
- **Apple Home:** export as a garage door through HA HomeKit Bridge.
- **Bluetooth Proxy:** advertisement forwarding and active BLE connections, with three connection slots by default.

### Documentation

| Document | Contents |
|---|---|
| [Wiring & BOM](docs/WIRING.md#english) | Encoder, converter, ESP32, and 3.3 V relay |
| [Control logic](docs/CONTROL.md#english) | Repeated requests, reversal, unknown position, and pulse limits |
| [Firmware](firmware/README.md#english) | Build, credentials, calibration, and first connection |
| [Home Assistant](home-assistant/README.md#english) | Cover, cache, statistics, notifications, and HomeKit |
| [Enclosure & STL](enclosure/README.md#english) | V0.27 base/lid STLs, dimensions, and mesh checks |
| [Tests](tests/README.md#english) | Standalone C++ simulations and Python template tests |
| [Project summary](docs/PROJECT.md#english) | Measurements, installation, and Bluetooth tradeoffs |

### Quick start

1. Read the wiring guide and verify your opener's terminals, voltages, wire order, and relay specifications.
2. Install `requirements.txt` in a Python environment and supply your own configuration using the [firmware guide](firmware/README.md#english).
3. During initial commissioning, leave relay COM/NO disconnected while checking encoder direction, the contact sensor, and relay pulses.
4. Add the ESPHome device to HA, verify entity IDs, and install the desired templates following the [HA guide](home-assistant/README.md#english).
5. Connect the factory PUSHBUTTON / COMMON terminals after physical validation.

Run software-only checks, which do not connect to HA or actuate the door:

```sh
python tests/run_tests.py
```

### Scope and limitations

This documents a retrofit for one LDCO800, not a universal plug-and-play controller. The `1049` full-travel count and count direction were measured on this unit and must be calibrated for your installation. `open_estimated` means fully open as estimated from counts, without an independent fully open limit sensor. Unknown position after reboot is intentional; a historical cache does not replace current feedback.

When direction is unknown, a directional request may start with one probe and briefly move the opposite way. HA Stop is not an immediate emergency stop. Retain the factory beam sensor, obstacle reversal, and mechanical limits. Disconnect opener power before internal wiring. Relay contacts connect only to the low-voltage button terminals and **do not switch 120 V power**.

This is a separate, sanitized publication using generic node names and entity IDs. It excludes household credentials, network addresses, device MAC addresses/serial numbers, original operational logs, and flash binaries. When upgrading an existing installation, retain your node and entity identities rather than overwriting them with the example names.

See [NOTICE.md](NOTICE.md#english) for licensing and third-party provenance.

---

## 简体中文

这是基于 **Linear LDCO800** 的 ESP32 + ESPHome 改造项目：通过编码器识别运动方向、估算开度，以反馈驱动开关门控制，并集成 Home Assistant 与 Bluetooth Proxy。

**项目完成：2026-09-18。** 保留原开门器、电机、遥控器、墙壁按钮和原厂保护功能。读取原厂编码器，通过继电器模拟按钮，在 ESP32 本地把 Open / Close 意图转换为有反馈的按钮序列。

### 实物照片

| 开门器型号 | 原厂主板 |
|---|---|
| <img src="docs/images/01-gate-model.jpg" alt="Linear LDCO800 型号铭牌" width="420"> | <img src="docs/images/02-gate-logic-board.jpg" alt="Linear LDCO800 原厂主板" width="420"> |
| **Linear LDCO800** 型号铭牌与电气规格。 | 原厂主板与电源区域，保留编码器接口。 |

| 打印盒内组件 | 合盖成品 |
|---|---|
| <img src="docs/images/03-components-in-box.jpg" alt="打印盒内的 ESP32、电平转换板和继电器" width="420"> | <img src="docs/images/04-finished-module.jpg" alt="合盖后的控制模块与引出线" width="420"> |
| ESP32、电平转换板与 3.3 V 继电器。 | 编码器 Y 线、按钮干接点引线与固定耳。 |

以上为项目实物照片。型号与主板照片仅做普通裁剪，另外两张保留原画面；四张均去除相机元数据，没有 AI 重绘、修饰或添加标注。

### 功能

- **编码器反馈**：GPIO22/23 读取两路编码器信号，识别 opening / closing / stopped，估算开度。
- **全关参考**：独立全关门磁经 HA 提供参考，稳定确认后校准零点。
- **定向控制**：Open Gate / Close Gate 支持最新目标优先；保留 Gate Button 原始单次按键。
- **Home Assistant**：HA `cover`、最后已知状态缓存、运动起步次数、行程耗时及异常提醒。
- **Apple Home**：可经 HA HomeKit Bridge 暴露为车库门。
- **Bluetooth Proxy**：支持广播转发与主动 BLE 连接；默认三个连接名额。

### 资料导航

| 资料 | 内容 |
|---|---|
| [接线与物料](docs/WIRING.md#简体中文) | 编码器、转换板、ESP32、3.3 V 继电器 |
| [控制逻辑](docs/CONTROL.md#简体中文) | 重复请求、换向、未知位置、脉冲限制 |
| [固件](firmware/README.md#简体中文) | 构建、凭据、标定与首次接入 |
| [Home Assistant](home-assistant/README.md#简体中文) | Cover、缓存、统计、通知与 HomeKit |
| [外壳模型](enclosure/README.md#简体中文) | V0.27 底壳 / 上盖 STL、尺寸与网格核验 |
| [测试](tests/README.md#简体中文) | 独立 C++ 模拟与 Python 模板测试 |
| [项目总结](docs/PROJECT.md#简体中文) | 实测、安装与蓝牙取舍 |

### 快速开始

1. 阅读接线说明，核对自己开门器的端子、电压、线序及继电器规格。
2. 在 Python 环境安装 `requirements.txt`，按 [固件说明](firmware/README.md#简体中文) 填入自己的配置。
3. 初次调试先断开继电器 COM/NO 控制线，确认编码器方向、门磁与继电器脉冲。
4. 将 ESPHome 设备加入 HA，按 [HA 说明](home-assistant/README.md#简体中文) 核对实体 ID 并安装所需模板。
5. 完成现场验证后，再接入原厂 PUSHBUTTON / COMMON。

运行纯软件检查（不会连接 HA 或触发门）：

```sh
python tests/run_tests.py
```

### 适用范围

这是针对一台 LDCO800 的改造记录，不是所有开门器的通用即插即用方案。`1049` 全行程计数和计数方向来自本机实测，必须按自己的安装校准。`open_estimated` 表示计数估算的全开，没有独立全开限位。重启后位置未知是有意设计，历史缓存不替代当前反馈。

未知方向时，定向请求可先试探一次，因此可能短暂反向；HA Stop 也不是即时急停。原机的光束、遇阻反转与机械限位必须保留。进行机内接线前断开开门器电源；继电器触点仅连接低压按钮端子，**不切换 120 V 电源**。

这是经过私人信息清理的独立发布版：使用通用节点名和实体 ID，未包含家中凭据、网络地址、设备 MAC/序列号、原始运行日志或烧录二进制。已有安装升级时应保留自己的节点和实体身份，不要直接用示例改名覆盖。

许可及第三方来源见 [NOTICE.md](NOTICE.md#简体中文)。
