# Project summary

[English](#english) | [简体中文](#简体中文)

## English

Final assembly, wiring, and installation inside the opener were completed on 2026-09-18. The project grew from sensing the garage door's state to providing explicit Open/Close control, HA integration, and a compact enclosure.

### Design evolution

The original Linear LDCO800 system is retained. A four-wire Y cable adds a parallel encoder branch, a level converter feeds the ESP32, and an independent contact sensor supplies the fully closed reference. A relay momentarily closes the factory button terminals. The ESP32 has its own USB supply and executes the control algorithm locally.

The first actual closed → open → closed run produced raw counts of `0 → -1049 → 0`. Software normalizes opening to positive counts and uses the closed contact to calibrate zero. The 1049-count travel is an initial measurement from this particular installation, not a substitute for calibrating another unit.

A single button must be interpreted according to state: it stops motion and usually reverses direction after a stop. The controller therefore gives priority to the latest target, leaves repeated same-direction requests uninterrupted, queues rapid reversal requests, and probes once when direction is unknown. Regression tests cover cooldowns, endpoints, cancellation, and unknown-position cases.

The enclosure went through revisions to board length, header clearance, clips, wire exits, and insertion direction. The latest CAD revision is V0.27. Final physical assembly and installation are confirmed, but the photographs cannot uniquely identify the printed STL revision. Dimensions and model publication details are in the enclosure directory.

### Validation evidence

- The Y cable passed pin-to-pin continuity checks and retained normal factory operation.
- An actual round trip returned the count to zero; the user also observed HA feedback over multiple opening/closing cycles.
- With relay COM/NO disconnected, one energize/release cycle and a subsequent released state were confirmed.
- Final photographs confirm button-terminal wiring and installation inside the opener. HA screenshots record opening / 30%, and closed / stopped / 0%, online, and calibrated states.
- Software validation includes 63 directional-control simulation scenarios and replay of 301 actual count samples, plus 19 statistics tests and 8 cover tests.

Passing simulation does not mean every mechanical corner case has been physically tested. Fully open remains an estimate, while long-term Bluetooth coexistence and notification behavior require continued operational observation.

### Bluetooth Proxy tradeoff

Bluetooth enabled and disabled configurations each received 60 read-only API information queries, plus warmup queries. The timeout was 5 seconds and the query interval was at least 1 second.

| Metric | Bluetooth enabled | Bluetooth disabled |
|---|---:|---:|
| Success / failure or timeout | 60 / 0 | 60 / 0 |
| Median | 36.97 ms | 97.72 ms |
| Maximum | 1861.15 ms | 174.76 ms |

With Bluetooth enabled, the tail reached about 1.86 seconds, but the median was lower. One sequential A/B run cannot establish causation. These were not door-command latency measurements and do not prove zero packet loss at the transport level. The final choice was to retain the full Bluetooth Proxy functionality and avoid a separate proxy board.

### HA integration

A garage cover provides Open, Close, limited Stop, and position display, replacing the previous light-style button backend. The last-valid-state cache is separate from live state; statistics and alerts only observe and notify. Existing dashboards, automations, and HomeKit retain the cover identity. The publication uses generic entity examples; private configuration and original field logs remain local.

---

## 简体中文

2026-09-18 完成最终组装、接线和原机内部安装。目标从“知道车库门开关状态”扩展为明确的 Open/Close 控制、HA 集成与紧凑外壳。

### 方案演进

保留 Linear LDCO800 原系统，通过四芯 Y 线引出编码器支路。电平板将信号送给 ESP32，独立门磁提供全关参考；继电器短时闭合原厂按钮端子。ESP32 USB 单独供电，控制算法在本地执行。

真实首次全关 → 全开 → 全关读数为 `0 → -1049 → 0`。软件将开门方向统一成正值，并使用全关门磁校准零点。行程 1049 是本台设备的初始测量，不能代替另一台机器的标定。

单按钮操作必须按状态解释：运动时停车，停止后一般反向。因此实现最新目标优先、重复同向请求不打断、快速反向请求排队，以及未知方向时一次试探。回归测试覆盖冷却、端点、取消和未知位置场景。

外壳经历板长、排针高度、卡扣、导线出口和装入方向的多次修订；最新 CAD 为 V0.27。最终实物组装与安装已确认，照片不能唯一鉴定使用的 STL 修订号。发布的尺寸与模型许可状态见 enclosure 目录。

### 验证证据

- Y 线逐端连通与原厂功能验证通过。
- 真实往返计数回到零；用户多次开关时 HA 状态有反馈。
- 继电器 COM/NO 留空时，已确认一次吸合、释放并保持释放。
- 最终照片确认按钮端子接线与机内安装；HA 截图记录 opening / 30%，以及 closed / stopped / 0%、在线和校准完成。
- 软件验证包含 63 个定向控制模拟场景和 301 条实际计数回放；另有 19 个统计与 8 个 cover 测试。

模拟通过不代表每个机械边界场景都已现场实测。全开仍为估算，长期蓝牙共存及提醒效果仍依靠运行积累。

### Bluetooth Proxy 的取舍

对启用和关闭蓝牙各进行了 60 次只读 API 信息查询，另有预热，超时阈值为 5 秒，查询间隔至少 1 秒。

| 指标 | 蓝牙启用 | 蓝牙关闭 |
|---|---:|---:|
| 成功 / 失败或超时 | 60 / 0 | 60 / 0 |
| 中位数 | 36.97 ms | 97.72 ms |
| 最慢 | 1861.15 ms | 174.76 ms |

启用时有约 1.86 秒长尾，但中位数更低。一次顺序 A/B 不能确定因果；这不是开关门命令延迟测试，也不证明底层零丢包。最终选择保留完整 Bluetooth Proxy，以减少另一块独立代理板。

### HA 集成

统一使用 garage cover 提供开、关、受限 Stop 及开度显示，替换旧 light 形式的按钮后端。最后有效状态缓存与实时状态分开，统计和提醒只观察/通知。原有仪表盘、自动化及 HomeKit 保留 cover 身份。发布版使用通用实体示例，私人配置和现场原始日志留在本地。
