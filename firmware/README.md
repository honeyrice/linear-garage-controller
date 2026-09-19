# ESPHome 固件

已使用版本：ESPHome 2026.8.2，ESP-IDF 框架，ESP32 4 MB。发布包保留实际控制算法，统一了示例名称：`linear-garage-controller` / Linear Garage Controller、Gate State、Open Gate、Close Gate、Controller Online。

## 配置

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

## 校验和烧录

```sh
esphome config firmware/ha-wifi.yaml
esphome compile firmware/ha-wifi.yaml
```

编译完成后使用适合本板的 USB 端口烧录。测试机器默认 **115200 baud**，避免自动尝试 460800/921600；停止串口监视后再刷写。使用 esptool 时明确指定 `--baud 115200`，镜像路径/偏移以 ESPHome 本次生成结果为准，不能盲套其他板型。

生成二进制包含自己的网络/API/OTA 凭据，不能上传公开仓库或 release。

## 软件组成

- `position_tracker.h`：计数、方向、零点、开度、关闭参考有效性。
- `gate_controller.h`：带反馈的定向按钮状态机。
- `position.yaml` / `relay.yaml`：ESPHome 输入、输出和状态发布。
- `bluetooth.yaml`：完整 Bluetooth Proxy；如不需要可移除主配置中的这一项 include 并重编译。
- `memory.yaml`：堆内存诊断。

Wi-Fi/API 的 `reboot_timeout: 0s` 避免单纯断网重启。连续本地计数与控制状态机不依赖每一步网络往返，但远程请求送达、门磁参考及 HA 显示仍依赖网络。
