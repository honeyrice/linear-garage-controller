# ESPHome firmware

[English](#english) | [简体中文](#简体中文)

## English

Version used: ESPHome 2026.8.2, ESP-IDF framework, ESP32 with 4 MB flash. The publication retains the actual control algorithms and normalizes example names to `linear-garage-controller` / Linear Garage Controller, Gate State, Open Gate, Close Gate, and Controller Online.

### Configuration

From the repository root, install dependencies in your chosen Python environment:

```sh
python -m pip install -r requirements.txt
```

`ha-wifi.yaml` reads credentials from environment variables. Set `LINEAR_WIFI_SSID`, `LINEAR_WIFI_PASSWORD`, `LINEAR_API_KEY`, and `LINEAR_OTA_PASSWORD`. Do not commit their actual values to Git.

The API key is a Base64-encoded random 32-byte value. Generate credentials locally:

```sh
python -c 'import secrets; print(secrets.token_urlsafe(24))' # Suitable for an OTA password
python -c 'import secrets,base64; print(base64.b64encode(secrets.token_bytes(32)).decode())'
```

Example variable names are in [`.env.example`](../.env.example). ESPHome does not automatically load `.env`; export the variables through your shell or secret-management tool.

Set `closed_contact_entity` in `ha-wifi.yaml` to your contact sensor entity. Ensure **off = fully closed, on = not fully closed**. Normalize a different polarity in HA first. Without this valid reference, the device cannot establish absolute position after restarting.

`FULL_TRAVEL = 1049` and `positive = -raw` in `position_tracker.h` are calibrations for this installation. Initially verify full travel and sign using read-only observation, then adjust them to your measurements. They are not universal LDCO800 parameters. GPIO22/23 and GPIO13 are configured in `position.yaml` / `relay.yaml`.

### Validation and flashing

```sh
esphome config firmware/ha-wifi.yaml
esphome compile firmware/ha-wifi.yaml
```

After compilation, flash through the appropriate USB port for the board. The test machine defaults to **115200 baud**; avoid automatically trying 460800/921600. Stop serial monitoring before flashing. When using esptool, explicitly pass `--baud 115200`. Use image paths and offsets from the current ESPHome build rather than copying offsets from another board type.

Generated binaries contain your network/API/OTA credentials and must not be uploaded to a public repository or release.

### Software components

- `position_tracker.h`: counts, direction, zero reference, opening percentage, and closed-reference validity.
- `gate_controller.h`: directional button state machine with feedback.
- `position.yaml` / `relay.yaml`: ESPHome inputs, outputs, and state publication.
- `bluetooth.yaml`: full Bluetooth Proxy. If unnecessary, remove its include from the main configuration and rebuild.
- `memory.yaml`: heap-memory diagnostics.

Wi-Fi/API `reboot_timeout: 0s` prevents reboots caused solely by network disconnection. Continuous local counting and the control state machine do not require a network round trip for each step, but remote request delivery, the contact-sensor reference, and HA display still depend on the network.

---

## 简体中文

已使用版本：ESPHome 2026.8.2，ESP-IDF 框架，ESP32 4 MB。发布包保留实际控制算法，统一了示例名称：`linear-garage-controller` / Linear Garage Controller、Gate State、Open Gate、Close Gate、Controller Online。

### 配置

在本仓库根目录、所用 Python 环境中安装：

```sh
python -m pip install -r requirements.txt
```

`ha-wifi.yaml` 通过环境变量读取凭据。设置 `LINEAR_WIFI_SSID`、`LINEAR_WIFI_PASSWORD`、`LINEAR_API_KEY` 和 `LINEAR_OTA_PASSWORD`；不要把实际值提交到 Git。

API 密钥为 32 字节随机值的 Base64 编码，可本地生成：

```sh
python -c 'import secrets; print(secrets.token_urlsafe(24))' # 可用作 OTA 密码
python -c 'import secrets,base64; print(base64.b64encode(secrets.token_bytes(32)).decode())'
```

示例变量名见 [`.env.example`](../.env.example)。`.env` 不会被 ESPHome 自动加载，需要通过自己的 shell 或秘密管理工具导出变量。

修改 `ha-wifi.yaml` 的 `closed_contact_entity` 为自己的门磁实体，确保 **off = 完全关闭，on = 未完全关闭**。不同极性先在 HA 规范化。没有这个有效参考，设备重启后不能确定绝对开度。

`position_tracker.h` 的 `FULL_TRAVEL = 1049` 和 `positive = -raw` 是本机标定。首次以只读方式核对完整行程和正负，再按自己的测量调整；这里不是所有 LDCO800 都适用的固定参数。GPIO22/23 和 GPIO13 在 `position.yaml` / `relay.yaml` 设置。

### 校验和烧录

```sh
esphome config firmware/ha-wifi.yaml
esphome compile firmware/ha-wifi.yaml
```

编译完成后使用适合本板的 USB 端口烧录。测试机器默认 **115200 baud**，避免自动尝试 460800/921600；停止串口监视后再刷写。使用 esptool 时明确指定 `--baud 115200`，镜像路径/偏移以 ESPHome 本次生成结果为准，不能盲套其他板型。

生成二进制包含自己的网络/API/OTA 凭据，不能上传公开仓库或 release。

### 软件组成

- `position_tracker.h`：计数、方向、零点、开度、关闭参考有效性。
- `gate_controller.h`：带反馈的定向按钮状态机。
- `position.yaml` / `relay.yaml`：ESPHome 输入、输出和状态发布。
- `bluetooth.yaml`：完整 Bluetooth Proxy；如不需要可移除主配置中的这一项 include 并重编译。
- `memory.yaml`：堆内存诊断。

Wi-Fi/API 的 `reboot_timeout: 0s` 避免单纯断网重启。连续本地计数与控制状态机不依赖每一步网络往返，但远程请求送达、门磁参考及 HA 显示仍依赖网络。
