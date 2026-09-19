# Directional control

[English](#english) | [简体中文](#简体中文)

## English

Open Gate and Close Gate are target requests; Gate Button is a raw single press. The factory button stops a moving door and usually reverses direction on the next activation after stopping, so the same target can require different numbers of pulses depending on the current state.

| State / request | Behavior |
|---|---|
| Already at the target endpoint or moving toward it | Do not add a pulse |
| Moving in the opposite direction | Stop, confirm that motion has stopped, then restart |
| Stopped midway; previous direction opposite to target | Usually one pulse |
| Stopped midway; previous direction same as target | Usually reverse → stop → desired direction, three pulses |
| Close immediately after Open | Retain the latest target, wait for the current pulse/cooldown, then reverse using feedback |
| Absolute position unknown, actual direction known | Use live motion direction |
| Direction also unknown | Probe once, observe feedback, then adjust; a brief movement in the opposite direction is possible |
| Closed reference still settling | Defer action to avoid treating a just-closed door as stopped midway |
| Count out of range | Report a fault instead of using ordinary unknown-position probing |

Each pulse lasts 500 ms, followed by a 1500 ms cooldown. No new encoder edge for 1500 ms means stopped. If there have already been no edges for the last 250 ms, defer a stop pulse based on stale motion status. An active sequence allows at most 6 pulses over 20 seconds, with up to 5 seconds of feedback waiting per step. Rapid changes of target do not extend the sequence's overall deadline.

Startup and reconnection do not trigger movement or restore an old target. Missing feedback does not cause unlimited retries. The sequence ends once the requested motion direction is confirmed: `opening_confirmed` does not mean fully open. A later factory obstacle reversal does not make this module repeatedly attempt to close the door.

Gate Button cancels the directional target and queues one raw press, subject to the pulse cooldown. HA Stop sends a raw press only when HA considers the door to be moving. Network delay, stop detection, and endpoint races can still affect the result; it is not an immediate emergency stop.

State values are `closed`, `opening`, `closing`, `stopped`, `unknown`, and `open_estimated`. Fully closed requires the contact sensor plus a 2.5-second settling period. Fully open is estimated at 99.5% of the calibrated full-travel count.

Offline simulation cannot cover every mechanical response. The current inputs cannot reliably distinguish a frozen encoder from a stationary door, and do not replace the factory protections.

---

## 简体中文

Open Gate / Close Gate 是目标请求；Gate Button 是原始单次按键。原生按钮在运动中停车，停止后通常反向，因此同一目标在不同状态下可能需要不同脉冲数。

| 状态 / 请求 | 行为 |
|---|---|
| 已到目标端点，或已向目标方向运动 | 不追加脉冲 |
| 正向相反方向运动 | 先停，确认停止后再启动 |
| 中途停止，上次方向与目标相反 | 通常一次脉冲 |
| 中途停止，上次方向与目标相同 | 通常反向 → 停止 → 目标方向，共三次 |
| Open 后立即 Close | 保存最新目标，等待当前脉冲/冷却并按反馈换向 |
| 绝对位置未知，实际方向已知 | 按实时方向处理 |
| 方向也未知 | 一次试探，观察反馈再调整，可能短暂反向 |
| 关闭参考正在稳定确认 | 暂缓，避免将刚到底误判成中途停止 |
| 计数越界故障 | 报错，不按一般未知位置试探 |

每个脉冲 500 ms，冷却 1500 ms。无新边沿 1500 ms 判定停止；近期 250 ms 已无边沿时暂缓按旧运动状态发停车脉冲。单轮最多 6 个脉冲 / 20 秒，每一步反馈等待最多 5 秒，快速改目标不延长该轮总时限。

启动、重连不自动移动或恢复旧目标；无反馈不无限重试。确认目标运动方向后结束序列，不能把 `opening_confirmed` 当成到达全开。后续原厂遇阻反向也不会触发本模块持续重新关门。

Gate Button 取消定向目标并排队一次原始按键，受脉冲冷却约束。HA Stop 仅在 HA 认为正在运动时发送原始按键；网络延迟、停止判定和端点竞争仍可能影响行为，它不是即时急停。

状态值：`closed`、`opening`、`closing`、`stopped`、`unknown`、`open_estimated`。全关依赖门磁和 2.5 秒稳定确认；全开为满行程计数 99.5% 阈值估算。

离线模拟不能涵盖全部机械响应。编码器冻结与真实静止仅靠当前输入无法完全区分，不能替代原厂保护。
