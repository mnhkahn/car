# ESP32-S3 BLE 遥控小车

基于 **ESP32-S3** + **L298N** 双路电机驱动 + **4 个直流减速电机** 的蓝牙遥控车方案。

> 🔍 检测到的主控芯片：**ESP32-S3**（VID:PID = `303a:1001`）  
> ⚠️ ESP32-S3 **仅有 BLE（低功耗蓝牙）**，没有经典蓝牙串口。

---

## 当前固件方案：MicroPython

由于网络环境限制，Arduino C++ 编译环境（PlatformIO / Arduino IDE）所需的大型依赖包无法在当前环境下载。因此本方案采用 **MicroPython**，已直接烧录到板子上。

**优势**：
- 无需 IDE，无需编译，改代码直接用命令行上传
- 固件体积小，烧录快
- 代码是 Python，逻辑清晰易改

---

## 项目结构

```
.
├── firmware/
│   └── main.py                     # MicroPython 主程序（已烧录到板子）
├── controller/
│   └── index.html                  # Web 蓝牙遥控器（Chrome / Edge）
├── docs/
│   ├── WIRING.md                   # 详细接线说明
│   └── APP_GUIDE.md                # 手机 App 连接与操作指南
└── README.md                       # 本文件
```

---

## 快速开始

### 1. 接线

详见 [docs/WIRING.md](docs/WIRING.md)。核心要点：
- 4 个电机分为 **左右两组并联**，接 L298N 的两路输出
- ESP32-S3 GPIO `5/6/7` 接左轮，`8/9/10` 接右轮
- **务必共地**：L298N 的 GND 必须接 ESP32-S3 的 GND
- 推荐 **电机电源与 ESP32 电源分离**，避免电机启动压降导致重启

### 2. 手机遥控

#### 方式 A：nRF Connect App（iOS / Android）
1. 下载 **nRF Connect**
2. 扫描并连接 `ESP32-S3-Car`
3. 找到 `Nordic UART Service` → `RX` 特征值
4. 发送指令：`F` 前进、`B` 后退、`L` 左转、`R` 右转、`S` 停止、`0`~`9` 调速度

#### 方式 B：Web 遥控器
用 Chrome / Edge 打开本地文件 `controller/index.html`，点击 **连接小车** 即可用按钮、键盘 WASD/方向键控制。

更多细节见 [docs/APP_GUIDE.md](docs/APP_GUIDE.md)。

---

## 指令协议

| 指令 | 说明 | 备注 |
|------|------|------|
| `F` | 前进 | 可加速度，如 `F200` |
| `B` | 后退 | 可加速度，如 `B200` |
| `L` | 原地左转 | 左轮反转，右轮正转 |
| `R` | 原地右转 | 右轮反转，左轮正转 |
| `l` | 缓左转 | 左轮慢速，右轮全速 |
| `r` | 缓右转 | 右轮慢速，左轮全速 |
| `S` | 停止 | 切断电机供电 |
| `0` ~ `9` | 速度档位 | `9`=最快，`0`=停止 |

---

## 开发者：修改代码 / 重新烧录

### 需要的环境

```bash
pip3 install esptool mpremote
```

### 修改代码并上传

编辑 `firmware/main.py`，然后执行：

```bash
mpremote connect /dev/cu.usbmodem1234561 fs cp firmware/main.py :main.py
mpremote connect /dev/cu.usbmodem1234561 reset
```

板子会自动重启并运行新代码。**全程不需要 IDE**。

### 查看串口日志

```bash
mpremote connect /dev/cu.usbmodem1234561 run -
# 或者直接用 screen / picocom
```

### 重新烧录 MicroPython 固件（如果搞坏了）

```bash
esptool --chip esp32s3 --port /dev/cu.usbmodem1234561 erase_flash
esptool --chip esp32s3 --port /dev/cu.usbmodem1234561 write_flash -z 0x0 micropython_esp32s3.bin
mpremote connect /dev/cu.usbmodem1234561 fs cp firmware/main.py :main.py
```

固件下载地址（ESP32-S3 Generic）：
https://micropython.org/download/ESP32_GENERIC_S3/

---

## 安全设计

- **断连自动停车**：BLE 客户端断开后，小车立即停车
- **超时保护**：超过 500ms 未收到新指令，自动停车
- **速度约束**：所有 PWM 值限制在合理范围

---

## 后续可扩展

- [ ] 接入超声波模块（HC-SR04）实现自动避障
- [ ] 接入舵机 + 云台摄像头，实现 FPV 图传
- [ ] 改用 PS4 / Xbox 手柄通过 BLE HID 控制
- [ ] 微信小程序自定义 UI 遥控
