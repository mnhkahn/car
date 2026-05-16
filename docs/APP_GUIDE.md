# 手机遥控指南

当前固件同时支持两种控制方式：板子自带 WiFi 热点页面，以及 BLE（低功耗蓝牙）备用控制。

## 方案一：WiFi 热点控制页面（推荐）

板子启动后会打开热点：

| 项目 | 值 |
|------|----|
| 热点名 | `ESP32-S3-Car` |
| 密码 | `12345678` |
| 控制页面 | `http://192.168.4.1` |

### 使用方法
1. 手机连接 WiFi 热点 `ESP32-S3-Car`
2. 如果系统没有自动弹出页面，就手动打开浏览器访问 `http://192.168.4.1`
3. 页面上可以直接遥控、自动寻迹、学习路线、记忆跑、停止保存、清除路线
4. 驾驶舱会实时显示模式、状态、速度、延迟、五路雷达、找线方向和路线段时间轴
5. 底部红色 **急停** 常驻显示，任何时候都可以先停住小车

项目里的 `controller/wifi.html` 是 WiFi 控制页面的本地源码参考；实际使用时，页面已经内置在板子固件里。

## 方案二：nRF Connect App（BLE 备用）

nRF Connect 是 Nordic 官方出品的 BLE 调试工具，完全免费，iOS / Android 都有。

### 安装
- **iOS**: App Store 搜索 "nRF Connect"
- **Android**: Google Play 或酷安搜索 "nRF Connect"

### 连接步骤
1. 打开 nRF Connect，点击 **SCANNER** 开始扫描
2. 找到广播名称 `ESP32-S3-Car`，点击 **CONNECT**
3. 连接后展开 **Nordic UART Service** (UUID 以 `6E400001` 开头)
4. 你会看到两个 Characteristic：
   - `6E400002` (RX) —— 向小车发送指令
   - `6E400003` (TX) —— 接收小车状态（本例中暂未使用）
5. 点击 `6E400002` 旁边的向上箭头 ⬆️
6. 在输入框中输入命令字母，选择 **UTF-8** 格式，点击 **SEND**

### 可用命令

| 命令 | 动作 | 示例 |
|------|------|------|
| `F` | 前进 | 发送 `F` |
| `B` | 后退 | 发送 `B` |
| `L` | 原地左转（差速大） | 发送 `L` |
| `R` | 原地右转（差速大） | 发送 `R` |
| `l` | 缓左转（差速小） | 发送 `l` |
| `r` | 缓右转（差速小） | 发送 `r` |
| `S` | 停止 | 发送 `S` |
| `T` | 自动寻迹 | 发送 `T`，再发送 `S` 停止 |
| `E` | 学习路线 | 慢速跑一圈，发送 `S` 停止并保存 |
| `P` | 记忆路线复跑 | 按保存的路线调速，传感器实时纠偏 |
| `C` | 清除保存路线 | 删除 ESP32 里保存的路线 |
| `0` ~ `9` | 设置速度档位 | 发送 `9` 全速，发送 `5` 中速 |
| `F200` | 以指定速度 200/255 前进 | 发送 `F200` |

> 💡 技巧：点击输入框右侧的 **Down Arrow** 可以将常用指令保存为快捷按钮，方便一键发送。

---

## 方案三：Web 蓝牙控制器（电脑/安卓 Chrome）

项目根目录下的 `controller/index.html` 是一个基于 Web Bluetooth API 的控制器页面。

### 使用方法
1. 用 **Chrome / Edge** 浏览器打开 `controller/index.html`（需要支持 Web Bluetooth，Chrome  on Android 也支持）
2. 点击 **连接小车** 按钮，选择 `ESP32-S3-Car`
3. 使用页面上的方向按钮控制，或按键盘方向键 / WASD
4. 点击 **自动寻迹** 进入循迹模式，再点一次停止寻迹

> ⚠️ Web Bluetooth 要求页面必须通过 `https://` 或 `localhost` / `file://` 访问。直接在本地双击打开 HTML 文件即可使用。
> iOS Safari 暂不支持 Web Bluetooth，请用 nRF Connect。

---

## 方案四：自建 App / 小程序

如果你会开发移动端应用：
- **Android**: 使用 `BluetoothLeGatt` 示例代码连接 Nordic UART Service，向 RX characteristic 写 UTF-8 字符串
- **iOS**: 使用 `CoreBluetooth`，扫描服务 `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`，写特征 `6E400002-...`
- **微信小程序**: 使用 `wx.openBluetoothAdapter` + `wx.writeBLECharacteristicValue`，同样操作 Nordic UART Service
