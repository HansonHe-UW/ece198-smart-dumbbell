# app — 单按钮整合固件 v1

状态 2026-11-08：v1 **编译通过、还没上板**（`arduino:esp32:nano_nora`，Flash 1.38 MB / 43 %，静态 RAM 89 kB / 27 %）。v0 在 2026-11-06 上过板：四个模块全部扫到，IMU 104 Hz 无丢样，按钮状态机可用（`tests/results/2026-11-06_app-v0-first-run.md`）。v1 在 v0 之上改了四块：

1. **计数器 v1**：从"角速度过阈值"改成"角度积分 + 最小幅度"，拉一下线、晃一下不再计数（FR-05、FR-06）。
2. **心率 v1**：换掉 SparkFun 的 `checkForBeat`，带通 + 自适应找峰 + 间隔一致性判定，输出有效标志和信号质量（FR-07、FR-08）。
3. **Wi-Fi 热点 + HTTP**：`GET /status`、`POST /target`，字段和 `software/data_logger.py` 的提案一致（FR-01、FR-11、FR-12）。
4. **BLE**（Nordic UART 服务）：同一条状态 JSON 每秒推一次，接收 `T=<n>` / `START` / `PAUSE` / `END` 命令。

规格 FR-11 写的是 Wi-Fi。BLE 是 2026-11-08 Hanson 提出的备选，两个都编进去，由组里决定留哪个；两者发的是同一个 JSON，记录器换传输层不用重写。

## 操作：一个按钮（D2，另一脚接 GND，内部上拉）

| 按法 | IDLE | RUNNING | PAUSED |
|---|---|---|---|
| 短按（< 0.5 s） | 开始，次数清零 | 暂停 | 继续 |
| 长按（≥ 1.5 s，按住到 1.5 s 立即生效） | 没反应 | 结束 | 结束 |
| 0.5 ～ 1.5 s | 忽略 | 忽略 | 忽略 |

结束后屏幕 `DONE` 3 秒。暂停时不计数（FR-02）。到达目标次数屏幕反色并长亮 LED 一次，之后继续计数（FR-10）。目标次数默认 10，可由电脑改（下文）。

## 接线（原理图 `hardware/wiring/kicad/smart-dumbbell.pdf` v2.1）

| 器件 | 引脚 | 说明 |
|---|---|---|
| 4517、GY-MAX30102、OLED | A4 SDA / A5 SCL、3V3、GND | 一条总线 |
| 按钮 SW1 | D2 → 按钮 → GND | |
| LED | D7 → 220 Ω → LED → GND | |
| 蜂鸣器 | D5 | 可选。默认 `USE_BUZZER 0`；DAOKAI 低电平触发模块（S8550）接 3V3、GND、D5，`BUZZER_ACTIVE_LOW 1` 时空闲为高电平，不会常响 |

## 代码里的开关（文件开头）

| 宏 | 默认 | 说明 |
|---|---|---|
| `STREAM_RAW` | 0 | 1 时每个 IMU 样本打一行 `I,...`。**录训练数据不用它**：`record_serial.py` 认的是 `04_imu_stream` 的无前缀 CSV，录数据烧 04 |
| `USE_BUZZER` | 0 | 1 时提示音开 |
| `BUZZER_ACTIVE_LOW` | 1 | 1：低电平响（DAOKAI 模块）；0：高电平响（Grove）。即使 `USE_BUZZER 0`，D5 的空闲电平也按它设，所以插着低电平模块不会叫 |
| `COUNT_GATE` | 0 | 1 时只有分类器最近一次输出 `curl` 才允许加一。`model_weights.h` 还是合成数据训的，默认关 |
| `TILT_CHECK` | 1 | 计数时额外要求重力方向转过 ≥ 25°。拒绝绕轴空转和拉线。误判漏计时可先关掉看是不是它 |
| `CURL_AXIS` | 3（gx） | 沿握把的陀螺仪轴，`ml/data-protocol.md`：X 沿握把 |
| `USE_WIFI` / `USE_BLE` | 1 / 1 | 任一关掉就少编译那部分 |

## 各部分怎么工作

### 计数器 v1（`counterSample`）

v0 的问题：只看角速度，拉一下线产生 200 deg/s 的尖峰也过阈值，被数成一次。v1 看**角度**：

1. 握把轴角速度一阶低通（α 0.3），静止 0.8 s 以上时慢慢学习零偏。
2. 漏积分成角度（运动中时间常数 10 s；静止且没在动作中时 0.5 s 快速归零，保证每次都从静止位置起算）。角度就是前臂相对静止位置转过的度数。弯举 60 ～ 130°，拉线 200 deg/s × 0.1 s 只有 20°。
3. 状态机：角度离开 ±15° 开始一次，记峰值；回到 15° 以内结束。峰值 ≥ 45°、在 15° 以外停留 0.4 ～ 5 s 才算一次。停在半空超过 5 s 归零重来。
4. `TILT_CHECK`：同时用垂直握把的两个加速度轴算重力倾角，一次动作内倾角范围 ≥ 25° 才算。绕握把轴纯转动陀螺仪有读数但重力不变，被这一条拒绝。

串口 `R` 行多了两列：`rom_deg` 峰值角度、`tilt_deg` 倾角范围，用来调阈值。`firmware/app/tools/algo_sim.py` 用合成信号复现这段逻辑：2 s / 4 s / 0.8 s 的弯举都计数，拉线、绕轴转、半次、4 Hz 晃动、零偏漂移都不计数。参数没在真人数据上调过，归线 2 / Julia 用录音离线定。

### 心率 v1（`hrSample` / `hrEvaluate`）

100 Hz 红外：慢 EMA 去直流 → 两级低通约 4.6 Hz → 两级高通约 0.65 Hz = 带通脉搏波。**一拍 = 带通信号由负转正的过零点**，且随后那个正半波的峰值高于跟踪包络的 25 %（滤掉无脉搏时的噪声过零）。不应期 = max(300 ms, 当前中位间隔 × 0.6)，把重搏切迹吞掉。保留最近 8 个间隔，**至少 5 个且中位绝对偏差 ≤ 中位数的 20 %** 才输出 bpm = 60000 / 中位数，再做 0.3 的 EMA 平滑。第一版用的"局部极大值高于包络一半"在合成信号上只有 40 % 的时间有效（切迹被当第二个峰），换成过零后 45 ～ 150 bpm 覆盖 80 ～ 92 %、误差 1 ～ 3 bpm（`tools/algo_sim.py`）。

`signal_quality`（串口 `H` 行和 JSON 都有）：

| 值 | 含义 |
|---|---|
| `no_finger` | IR < 50000 |
| `insufficient` | 脉搏幅度 / 直流 < 0.05 %，或拍数不够，或 3 s 没有新拍（FR-08、AT-06） |
| `unstable` | 拍数够但间隔不一致（动作干扰、手指滑动） |
| `good` / `good_moving` | 输出有效；`_moving` 表示 IMU 角速度 > 80 deg/s 时给出的值，AT-04 要单独统计 |

无效时 `heart_rate_bpm` 为 `null`、屏幕 `HR --`（无手指）或 `HR ...`（有手指未稳），不显示旧值。v0 "0 然后突然跳很大再没数"的现象来自 `checkForBeat` 加手指检测抖动，v1 没上板验证，**先在静止手指下对手表测，再看运动**。

### 电脑接口

**Wi-Fi**：板子开热点 `SmartDumbbell-57` / `ece198test`，IP `192.168.4.1`。

```bash
curl http://192.168.4.1/status
```

```bash
curl -X POST http://192.168.4.1/target -H 'Content-Type: application/json' -d '{"target_reps":12}'
```

Julia 的记录器不改即可用：电脑连上热点后 `python3 software/data_logger.py --url http://192.168.4.1 --target 10 --csv data/session_logs/x.csv`。

**BLE**：广播名 `SmartDumbbell-57`，Nordic UART 服务。TX 每秒一行 JSON（按 `\n` 拼包），RX 收 `T=12`、`START`、`PAUSE`、`END`。测试脚本：

```bash
uv run --with bleak python firmware/app/tools/ble_log.py --target 12 --seconds 60 --csv data/session_logs/ble_log.csv
```

**状态 JSON**（三条通道相同，串口是 `J,` 行）：`session_id`（每次开始换新）、`sequence`、`elapsed_ms`、`session_state`（idle / running / paused）、`target_reps`、`rep_count`、`motion_class`、`motion_confidence`、`heart_rate_bpm`（无效为 null）、`heart_rate_valid`、`signal_quality`、`connection_state`（none / wifi / ble / wifi+ble）。屏幕第一行 `W` `B` 两个字母显示连接状态（FR-09）。

断开电脑后本地计数、显示照常（FR-11 后半句），电脑重连后拿到的是当前累计值和新的 `sequence`，记录器按 `sequence` 判断缺口。

### 其余

- **IMU / 分类器**：104 Hz，208 点环形缓冲，每秒跑一次 `firmware/classifier` 的模型，`C` 行输出。
- **屏幕**：帧分 64 字节块、只在离下一次 IMU 采样 ≥ 3 ms 时发（v0 实测整帧刷屏让 IMU 每次迟 3 拍）；`Adafruit_SSD1306` 构造时 `clkDuring`、`clkAfter` 都给 400 kHz，否则总线被降到 100 kHz。
- **串口命令**：串口里直接敲 `T=15`、`START` 也认，调试方便。
- **`S` 行**多了 `wifi_sta`、`ble` 两列。Wi-Fi + BLE 同时开后要重点看 `imu_late` 是否仍为 0。

## 和 `firmware/classifier` 的关系

`classifier.h`、`model_weights.h` 是从 `firmware/classifier/` 复制的。重训后同步：

```bash
cp firmware/classifier/classifier.h firmware/classifier/model_weights.h firmware/app/
```

## 编译 / 上传

```bash
arduino-cli compile --fqbn arduino:esp32:nano_nora firmware/app
```

```bash
arduino-cli upload --fqbn arduino:esp32:nano_nora -p /dev/cu.usbmodemXXXX firmware/app
```

## v1 上板要看的（按顺序）

1. 串口首行 `# app v1: ... wifi 1; ble 1; oled 1; hr 1`，后面两行 `# wifi softAP ... up` 和 `# ble advertising`。
2. 不碰：`S` 行 `imu_hz` 103 ～ 105、`imu_late` 0、`imu_dt_max_ms` ≈ 10。Wi-Fi 和 BLE 的协议栈跑在另一个核上，预期不影响；涨了就先关一个看是谁。
3. 按钮：同 v0。
4. **拉线、晃板子、绕握把轴转**：不计数。做弯举（≥ 45°、0.6 ～ 5 s）：计数，`R` 行里 `rom_deg` 和 `tilt_deg` 记下来。漏计就看这两列离阈值多远。
5. 手指静止放上：`HR ...` → 约 8 s 后出数字，和手表比；`H` 行的 `ac` 列应在 0.001 以上。拿开 3 s 内 `HR --`。
6. 手机或电脑连热点，`curl /status`；`ble_log.py` 连上，`T=12` 后屏幕目标变 12。
7. 两个都连着再看 `S` 行。

结果写进 `tests/results/`，模板 `tests/templates/test-record.md`。
