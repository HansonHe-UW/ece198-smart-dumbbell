# 板子到货测试（bring-up）

目的：按顺序证明 Nano ESP32 能烧录、无线能用、I2C 总线通、IMU 出数。四个 sketch 各测一件事，前一个不过就不要做下一个。每一步把结果写进 `tests/`（模板在 `tests/templates/test-record.md`），通过和失败都记。

写于 2026-11-03，板子到货当天。这些 sketch 是按库文档写的；"编译验证"一栏在本页末尾说明哪些真的编过。

## 0. 电脑端准备（一次）

1. Arduino IDE 2.x。
2. **Tools → Board → Boards Manager**，搜 `Arduino ESP32 Boards`（作者 Arduino，不是 Espressif 那个），安装。下载几百 MB，提前装。
3. **Tools → Board → Arduino ESP32 Boards → Arduino Nano ESP32**。
4. **Tools → Pin Numbering** 保持默认 **By Arduino pin**。选成 GPIO 编号的话 A4/A5、D2 这些名字全部错位。
5. **Library Manager** 装 `Adafruit LSM6DS`，提示一并安装依赖（Adafruit Unified Sensor、Adafruit BusIO）时选 Install all。
6. 用一根**能传数据**的 USB-C 线。只能充电的线插上后电脑看不到端口，这是最常见的"板子坏了"假警报。

端口：macOS 显示为 `/dev/cu.usbmodemXXXX`，Windows 为 `COMx`，IDE 里名字会标 Arduino Nano ESP32。

## 1. `01_blink` — 板子活着

接线：只插 USB-C。

通过标准：板载 RGB 灯红、绿、蓝轮流亮，黄灯同步闪；串口监视器（115200）每 300 ms 一行 uptime。

不通过时：
- 端口不出现：换线、换 USB 口。
- 上传失败或板子卡死：**双击 RST**（两次间隔 0.3 到 1 秒，比别的 Arduino 慢），RST 旁边的灯慢速绿色呼吸表示进入了 Arduino bootloader 模式，再上传一次即可恢复。
- 双击也救不回来：用一根杜邦线把 **B1 焊盘和 GND 短接**（板子上 B1 是焊盘，不是按钮，板上唯一的按钮是 RST），短接状态下按一下 RST，RGB 灯常绿表示进入固件下载模式，然后按 Arduino 官方"Reset the Arduino bootloader on the Nano ESP32"文章走 Burn Bootloader。这一步是最后手段。

## 2. `02_wifi_ap` — 无线

接线：只插 USB-C。

通过标准：手机 Wi-Fi 列表出现 `SmartDumbbell-57`，密码 `ece198test` 能连上，浏览器打开 `http://192.168.4.1` 显示一行 uptime 文本。串口里能看到 `stations=1`。

顺便记录：从上电到热点出现用了几秒（之后 AT-08 断线重连要用）。

## 3. `03_i2c_scan` — 总线和 IMU 在不在

接线（4209 线，JST 端插 4517 任一 QT 座，杜邦端插 Nano）：

| 线色 | Nano 引脚 |
|---|---|
| 黑 | GND |
| 红 | 3V3 |
| 蓝 | A4（SDA） |
| 黄 | A5（SCL） |

Nano 上 A4、A5 在印着 SDA、SCL 的那一侧。4517 板上绿色电源灯应该亮。

通过标准：串口每 3 秒打印一次，列出 `0x6A LSM6DSOX` 和 `0x1C LIS3MDL`，共 2 个设备。

不通过时：
- 0 个设备且 4517 灯不亮：红线没到 3V3。
- 0 个设备但灯亮：蓝黄接反，或 JST 头没插到底（要听到咔哒）。
- 只有 0x6A 没有 0x1C：正常也可能，LIS3MDL 偶尔在冷启动后第一轮扫描不应答，看第二轮。

以后接上 MAX30102 和 OLED，再跑一次这个 sketch，应该多出 `0x57` 和 `0x3C`。

## 4. `04_imu_stream` — IMU 出数，也是录数据的格式

接线同第 3 步。

通过标准：串口以 104 Hz 输出 CSV，表头 `t_ms,ax,ay,az,gx,gy,gz`。板子平放不动时 az 约 +9.8，ax、ay 接近 0，三个 gyro 列在 ±2 以内抖动。绕某个轴快速转动，对应 gyro 列明显摆到几十到几百。

顺便做两件事：
- 用 IDE 的 Serial Plotter 看一眼波形，确认没有周期性丢包（时间戳每行应递增约 9 到 10 ms）。
- 用 `record_serial.py` 录 30 秒存成文件，验证录数据流程：

```bash
pip install pyserial
```

```bash
python3 firmware/bringup/record_serial.py /dev/cu.usbmodem1101 data/raw/bench_flat_01.csv 30
```

端口名换成自己电脑上的。`data/raw/` 被 `.gitignore` 忽略，录音不会进 Git。

量程设置：加速度 ±4 g，陀螺仪 ±500 dps，104 Hz。这是 ML 计划里提议的采集设置，改之前先改 `ml/README.md`。

## 5. 可选：TensorFlow Lite Micro 编译检查

ML 那条线最大的未知数是推理库能不能在这块板上编译。Library Manager 搜 `Chirale_TensorFlowLite`，打开它的 `hello_world` 示例，选 Nano ESP32 只点 **Verify** 不用上传。能编过就记一笔"工具链可用"；编不过把完整报错贴进 Issue，备选方案是 emlearn 把 scikit-learn 模型导成 C 代码。

## 6. 记录什么

每个 sketch 一条记录：日期、谁测的、IDE 和开发板包版本、通过与否、异常现象、串口输出截图或前 20 行。IMU 那一步额外记静止时 az 的均值和 gyro 的标准差，以后对比传感器有没有漂移。

## 编译验证

2026-11-03，macOS，arduino-cli 1.x，开发板包 Arduino ESP32 Boards 2.0.18-arduino.5，FQBN `arduino:esp32:nano_nora`。四个 sketch 都编译通过，**尚未在板子上运行**：

| Sketch | Flash | RAM（全局变量） |
|---|---|---|
| 01_blink | 286 kB（9%） | 30.6 kB（9%） |
| 02_wifi_ap | 706 kB（22%） | 56.3 kB（17%） |
| 03_i2c_scan | 303 kB（9%） | 30.9 kB（9%） |
| 04_imu_stream | 315 kB（10%） | 31.0 kB（9%） |

库版本：Adafruit LSM6DS 4.7.4、Adafruit Unified Sensor 1.1.15、Adafruit BusIO 1.17.4。

第 5 步的 TensorFlow Lite Micro 检查也做了：Chirale_TensorFlowLite 2.0.0 的 `hello_world` 示例在同一配置下编译通过，Flash 579 kB（18%），RAM 39 kB（11%）。工具链可用，剩下的是模型大小和推理时间要在板上实测。

命令行复现（装了 arduino-cli 的话）：

```bash
arduino-cli compile --fqbn arduino:esp32:nano_nora firmware/bringup/01_blink
```
