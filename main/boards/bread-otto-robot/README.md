# Bread Otto Robot (ESP32-S3-DevKitC-1)

本板型将 `otto-robot` 控制逻辑迁移到双 400 孔面包板硬件：

- ESP32-S3-DevKitC-1 (WROOM N16R8, 44-pin)
- INMP441 数字麦克风
- MAX98357A 功放 + 8 ohm 喇叭
- 128x32 I2C OLED (SSD1306/SH1106)
- 6 路舵机

## 接线坐标系约定

以下“逐孔位”按常见 400 孔面包板标注：

- 每块板行号：`1..30`
- 列号：左半区 `a-e`，右半区 `f-j`
- 中间有断槽（`e` 与 `f` 之间不导通）
- 两块板分别记为：
  - `B1`：左侧面包板（放 DevKitC、INMP441、MAX98357A）
  - `B2`：右侧面包板（放 OLED、舵机信号排针）

## 按你照片朝向的固定视角

为了避免镜像错误，后续所有位置都按下面这个视角：

- 你面对面包板拍照时，USB 口在下边，天线在上边。
- 左边那块板是 `B1`，右边那块板是 `B2`。
- `B1`、`B2` 的红蓝电源轨都按“红在最外侧，蓝在内侧”识别。
- DevKitC 放在 `B1` 中缝，左右排针跨过中间断槽。

可用下面这个简图对照：

```text
            (上方)
   ┌────────────────────────────┐
   │  B1(左)      |    B2(右)   │
   │  INMP441     |    OLED     │
   │  MAX98357A   |    Servo区  │
   │      ESP32-S3-DevKitC      │
   │        (USB朝下)           │
   └────────────────────────────┘
            (下方)
```

如果你把整板旋转 180 度后再接线，`e/f` 与 `a/j` 的方位会全反，必须先转回这个视角再按表接。

## 推荐器件摆放（先摆再接）

1. `ESP32-S3-DevKitC-1` 放在 `B1` 中间断槽上，USB 口朝下。  
左排针落在 `e4..e25`，右排针落在 `f4..f25`（每边 22 脚）。
2. `INMP441` 放 `B1` 左上区域（建议 `a26..d30`）。
3. `MAX98357A` 放 `B1` 左下区域（建议 `a10..d16`）。
4. `OLED` 放 `B2` 上方（建议 `g4..j7`，4pin 竖插）。
5. `B2` 中部放 3 组 3Pin 舵机母座（每组 2 个舵机，信号/5V/GND）。

## DevKitC 引脚落孔速查（本摆放专用）

左排针（`e4..e25`）：

- `e7=GPIO4`, `e8=GPIO5`, `e9=GPIO6`, `e10=GPIO7`
- `e11=GPIO15`, `e12=GPIO16`
- `e13=GPIO17`, `e14=GPIO18`, `e15=GPIO8`
- `e24=5V`, `e25=GND`

右排针（`f4..f25`）：

- `f9=GPIO42`, `f10=GPIO41`
- `f11=GPIO40`, `f12=GPIO39`
- `e15=GPIO8`, `e16=GPIO9`, `e17=GPIO10`, `e18=GPIO11`
- `e19=GPIO12`, `e20=GPIO13`
- `f17=GPIO0`
- `f24/f25=GND`

## 逐孔位接线图（按两块 400 孔板）

### 1) 电源总线与共地

1. `B1 e24(5V)` -> `B1` 红色电源轨 `+`
2. `B1 e25(GND)` -> `B1` 蓝色电源轨 `-`
3. `B1 +` -> `B2 +`（任意一对同色轨，建议 `+` 轨中部对中部）
4. `B1 -` -> `B2 -`
5. `B1 e4(3V3)` -> `B1` 一条独立 3V3 分配行（建议 `a3-e3`）

### 2) INMP441（B1）

按 INMP441 模块丝印 `VDD/GND/SCK/WS/SD` 接：

- `VDD` -> `B1 a3`（3V3）
- `GND` -> `B1 -`
- `SCK` -> `B1 e8`（GPIO5）
- `WS` -> `B1 e7`（GPIO4）
- `SD` -> `B1 e9`（GPIO6）

### 3) MAX98357A + 喇叭（B1）

按模块丝印 `VIN/GND/BCLK/LRC/DIN` 接：

- `VIN` -> `B1 +`（5V）
- `GND` -> `B1 -`
- `BCLK` -> `B1 e11`（GPIO15）
- `LRC` -> `B1 e12`（GPIO16）
- `DIN` -> `B1 e10`（GPIO7）
- 喇叭接 `SPK+ / SPK-`

### 4) OLED 128x32 I2C（B2）

按 OLED 丝印 `GND/VCC/SCL/SDA` 接：

- `GND` -> `B2 -`
- `VCC` -> `B2 a3`（由 `B1` 3V3 引来）
- `SCL` -> `B1 f9`（GPIO42）通过跨板杜邦线到 OLED `SCL`
- `SDA` -> `B1 f10`（GPIO41）通过跨板杜邦线到 OLED `SDA`

### 5) 6 路舵机信号（B2）

舵机电源统一接 `B2 +5V` 与 `B2 GND`，信号线如下：

- 右腿舵机信号 -> `B1 e15`（GPIO8）
- 右脚舵机信号 -> `B1 e16`（GPIO9）
- 左腿舵机信号 -> `B1 e17`（GPIO10）
- 左脚舵机信号 -> `B1 e18`（GPIO11）
- 左手舵机信号 -> `B1 e19`（GPIO12）
- 右手舵机信号 -> `B1 e20`（GPIO13）

## 逻辑引脚映射（代码一致）

| 模块 | 信号 | GPIO |
|---|---|---|
| INMP441 | WS/SCK/SD | GPIO4/GPIO5/GPIO6 |
| MAX98357A | DIN/BCLK/LRCK | GPIO7/GPIO15/GPIO16 |
| OLED(I2C) | SDA/SCL | GPIO41/GPIO42 |
| Servo Right Leg | SIG | GPIO8 |
| Servo Right Foot | SIG | GPIO9 |
| Servo Left Leg | SIG | GPIO10 |
| Servo Left Foot | SIG | GPIO11 |
| Servo Left Hand | SIG | GPIO12 |
| Servo Right Hand | SIG | GPIO13 |

> 注意：`ESP32-S3-WROOM N16R8` 不可使用 `GPIO35/36/37` 连接舵机，这些脚与板载 Octal Flash/PSRAM 总线复用，会导致启动卡死或反复重启。

## 供电建议（务必遵守）

- DevKitC：USB-C 供电，或 `5V` 引脚供电。
- 舵机：独立 5V 大电流电源（建议 >= 5V/3A，6 路舵机建议更高）。
- 必须共地：舵机电源 `GND` 与 ESP32 `GND` 必须连接。
- 禁止将舵机电源接到 ESP32 `3V3`。
- `3.7V` 锂电池需先升压到稳定 `5V` 再给系统供电。

## 说明

- 本板型保留 `otto-robot` 的动作控制、MCP 工具、WebSocket 控制逻辑。
- 表情/文本显示改为 OLED 显示路径（参考 bread-compact-wifi 风格）。
