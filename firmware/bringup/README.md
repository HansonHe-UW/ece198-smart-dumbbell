# 板子到货测试（bring-up）

目的：按顺序证明 Nano ESP32 能烧录、无线能用、I2C 总线通、IMU 出数。四个 sketch 各测一件事，前一个不过就不要做下一个。每一步把结果写进 `tests/`（模板在 `tests/templates/test-record.md`），通过和失败都记。

写于 2026-11-03，板子到货当天。这些 sketch 是按库文档写的；"编译验证"一栏在本页末尾说明哪些真的编过。

## 0. 电脑端准备（一次）

1. Arduino IDE 2.x。
2. **Tools → Board → Boards Manager**，搜 `Arduino ESP32 Boards`（作者 Arduino，不是 Espressif 那个），安装。下载几百 MB，提前装。
3. **Tools → Board → Arduino ESP32 Boards → Arduino Nano ESP32**。
4. **Tools → Pin Numbering** 保持默认 **By Arduino pin**。选成 GPIO 编号的话 A4/A5、D2 这些名字全部错位。
5. **Library Manager** 装 `Adafruit LSM6DS`，提示一并安装依赖（Adafruit Unified Sensor、Adafruit BusIO）时选 Install all；再装 `SparkFun MAX3010x Pulse and Proximity Sensor Library`、`Adafruit SSD1306`（会带上 Adafruit GFX）。
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

通过标准：串口每 2 秒一行 `AP SmartDumbbell-57  ip=192.168.4.1  stations=0 ...`；手机 Wi-Fi 列表出现 `SmartDumbbell-57`，密码 `ece198test` 能连上，连上后那一行变成 `stations=1`；浏览器打开 `http://192.168.4.1` 显示一行 uptime 文本。

**串口工具的坑（2026-11-03 踩过）：** 不要在这块板的串口上切换 DTR/RTS。Arduino ESP32 内核把"DTR/RTS 翻转"当成进入下载模式的信号，板子会重启进 ROM 下载模式，端口名变成 `usbmodem1101` 之类，串口只剩一行 `waiting for download`，之后 IDE 上传报 `exit status 74`。IDE 的串口监视器和 `cat` 不会触发；pyserial 脚本里不要碰 `dtr`/`rts`。恢复：按一下 RST；没有人在板子旁边时可以用 esptool 5.x 执行 `esptool --chip esp32s3 --port <端口> --after watchdog-reset chip-id`，普通 `hard_reset` 出不来。

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

## 5. `05_max30102` — 心率模块出数

接线（GY-MAX30102 模块用杜邦线接到面包板，和 IMU 共用同一条 I2C 总线）：

| 模块引脚 | 接到 |
|---|---|
| VIN | 3V3（和 4209 红线同一行） |
| GND | GND |
| SCL | A5（和黄线同一行） |
| SDA | A4（和蓝线同一行） |
| INT、IRD、RD | 不接 |

很多 GY-MAX30102 到手时排针是散装的，要先焊；只焊 VIN、GND、SCL、SDA 四根也行。接好后先跑一遍 `03_i2c_scan`，应该多出 `0x57 MAX30102`，共 3 个设备。扫不到 0x57 但 IMU 还在，就是这块模块的 1.8 V 上拉问题，见 `hardware/wiring/README.md`。

通过标准：
- 串口第一行 `part id 0x15`。
- 没手指时 `ir` 小于 20000；手指轻放在窗口上 `ir` 跳到 50000 以上，`finger` 变 1。
- 等 20 秒稳定后，`beat` 大约每次心跳置 1 一次，`bpm_avg` 和你自己数 30 秒脉搏乘 2 的结果相差不超过 5。
- 手指拿开 2 秒内 `finger` 回 0，`bpm` 归零（这是 FR-08、AT-06 的雏形）。

手指要**轻放**，用力压会把血挤走，信号反而没了。环境光强时用手遮一下。这一步测的是静止心率；运动中心率是另一回事，要等 AT-04 的方法定下来再测。

## 6. `06_imu_hr_combined` — 两个传感器同时跑（协同性）

接线同第 5 步，4517 和 GY-MAX30102 都挂在总线上。这一步回答的是 FR-13 和 AT-07 的前半句：心率处理会不会拖慢动作采样。两个传感器在一个非阻塞 loop 里轮流服务，IMU 按 104 Hz 定时，MAX30102 从 FIFO 里有多少取多少。

串口三种行：`I,` 开头是 IMU 样本（104 Hz），`H,` 开头是心率样本（约 25 Hz），`S,` 开头每 5 秒一条统计。统计列：IMU 样本数、实际频率、最大间隔 ms、累计迟到次数、心率样本数、心率频率、单次 loop 最长耗时 µs。

通过标准（手指放在 MAX30102 上、同时晃动 IMU 的 30 秒内）：
- `imu_hz` 在 103 到 105 之间，`imu_dt_max_ms` 不超过 12，`imu_late` 不增加。
- `hr_hz` 约 25，`H` 行里仍有 `beat=1` 出现。
- `loop_max_us` 明显小于 9600（一个 IMU 周期）。

任何一项不满足，先看 `loop_max_us`：接近或超过 9600 说明某次 I2C 读取太长，通常是 MAX30102 FIFO 一次攒了太多样本，把 `hr.check()` 改成每次最多处理几个样本即可。

## 7. `07_oled_buttons` — OLED、按钮、LED、蜂鸣器可用性

一个最小的界面模型：SW1 开始／暂停，SW2 手动加一次，SW3 结束归零。屏幕显示状态、大号次数／目标、计时。库：Adafruit SSD1306、Adafruit GFX。

接线：

| 器件 | 接到 | 说明 |
|---|---|---|
| OLED VCC / GND | 3V3 / GND | **先看模块丝印**，有的是 GND-VCC-SCL-SDA，有的是 VCC-GND-SCL-SDA，接反会烧 |
| OLED SCL / SDA | A5 / A4 | 和 IMU、心率共用那两行 |
| SW1 / SW2 / SW3 | D2 / D3 / D4，另一脚到 GND | 内部上拉，不要电阻；套件按钮跨在面包板中沟上 |
| LED | D7 → 220 Ω → LED 长脚，短脚到 GND | 选红或黄 |
| 蜂鸣器 | D5，另一脚 GND | 有 Grove 转接线才接；没有就跳过，LED 代替 |

接好先跑 `03_i2c_scan`，应出现 `0x3C SSD1306 OLED`，共 4 个设备。

通过标准：
- 上电屏幕显示 `IDLE  0 /10` 和按键提示，字清楚不闪。
- 每次按键串口只打印一条 `EVENT`，不重复、不漏；连按 20 次 SW2 计数正好 20。
- 每次加一 LED 闪一下，蜂鸣器响一下，到 10 时长响。
- 大号数字在一臂距离外能认出来。

这一步同时验证了 FR-02 的三键操作和 FR-09 的显示内容雏形。

## 8. 可选：TensorFlow Lite Micro 编译检查

ML 那条线最大的未知数是推理库能不能在这块板上编译。Library Manager 搜 `Chirale_TensorFlowLite`，打开它的 `hello_world` 示例，选 Nano ESP32 只点 **Verify** 不用上传。能编过就记一笔"工具链可用"；编不过把完整报错贴进 Issue，备选方案是 emlearn 把 scikit-learn 模型导成 C 代码。

## 9. 记录什么

每个 sketch 一条记录：日期、谁测的、IDE 和开发板包版本、通过与否、异常现象、串口输出截图或前 20 行。IMU 那一步额外记静止时 az 的均值和 gyro 的标准差，以后对比传感器有没有漂移。

## 编译验证

2026-11-03，macOS，arduino-cli 1.x，开发板包 Arduino ESP32 Boards 2.0.18-arduino.5，FQBN `arduino:esp32:nano_nora`。四个 sketch 都编译通过，**尚未在板子上运行**：

| Sketch | Flash | RAM（全局变量） |
|---|---|---|
| 01_blink | 286 kB（9%） | 30.6 kB（9%） |
| 02_wifi_ap | 706 kB（22%） | 56.3 kB（17%） |
| 03_i2c_scan | 303 kB（9%） | 30.9 kB（9%） |
| 04_imu_stream | 315 kB（10%） | 31.0 kB（9%） |
| 05_max30102 | 308 kB（9%） | 31.1 kB（9%） |
| 06_imu_hr_combined | 319 kB（10%） | 31.2 kB（9%） |
| 07_oled_buttons | 320 kB（10%） | 31.1 kB（9%） |

库版本：Adafruit LSM6DS 4.7.4、Adafruit Unified Sensor 1.1.15、Adafruit BusIO 1.17.4、SparkFun MAX3010x 1.1.2。01 到 05 已在板子上跑过（见 `tests/results/`），06 和 07 只编译过。

第 8 步的 TensorFlow Lite Micro 检查也做了：Chirale_TensorFlowLite 2.0.0 的 `hello_world` 示例在同一配置下编译通过，Flash 579 kB（18%），RAM 39 kB（11%）。工具链可用，剩下的是模型大小和推理时间要在板上实测。

命令行复现（装了 arduino-cli 的话）：

```bash
arduino-cli compile --fqbn arduino:esp32:nano_nora firmware/bringup/01_blink
```
