# app — 单按钮整合固件 v0

状态 2026-11-06：编译通过（`arduino:esp32:nano_nora`，Flash 339 kB / 10 %，RAM 41.6 kB / 12 %），**已上板**：四个模块全部扫到，IMU 104 Hz 无丢样，按钮状态机可用；弯举计数、LED、手指心率还没测（见 `tests/results/2026-11-06_app-v0-first-run.md`）。把 bring-up 里验证过的几块（04 IMU、05 心率、07 OLED、`firmware/classifier`）合到一个非阻塞 loop 里，按钮改成一个。Wi-Fi 不在这里（FR-11、FR-12 是另一条线）。

## 操作：一个按钮（D2，另一脚接 GND，内部上拉）

| 按法 | IDLE | RUNNING | PAUSED |
|---|---|---|---|
| 短按（< 0.5 s） | 开始，次数清零 | 暂停 | 继续 |
| 长按（≥ 1.5 s，按住到 1.5 s 立即生效，不用等松手） | 没反应 | 结束 | 结束 |
| 0.5 ～ 1.5 s | 忽略 | 忽略 | 忽略 |

结束后屏幕显示 `DONE` 3 秒，串口打印一行 `EVENT,...,end,reps=N,duration_s=T`。暂停时不计数（FR-02）。到达目标次数屏幕反色显示并长响一次，之后继续计数（FR-10）。

这个方案和规格 `FR-02` 的"提供开始、暂停／继续、结束操作"不冲突，但 `specification-zh.md` 第 57 行元件表写的是"按钮 ×3"，`07_oled_buttons` 的 README 第 7 步也是三键。等组内确认后一起改。

## 接线（在 `hardware/wiring/schematic-v1.svg` 基础上）

| 器件 | 引脚 | 变化 |
|---|---|---|
| 4517、GY-MAX30102、OLED | A4 SDA / A5 SCL、3V3、GND | 不变 |
| 按钮 | D2 → 按钮 → GND | 只留 SW1；D3、D4 空出 |
| LED | D7 → 220 Ω → LED → GND | 不变 |
| 蜂鸣器 | D5 | 可选。`USE_BUZZER 0` 时 D5 保持低电平，不接也行 |

## 代码里的开关（文件开头）

| 宏 | 默认 | 说明 |
|---|---|---|
| `STREAM_RAW` | 0 | 1 时每个 IMU 样本打一行 `I,...`，录训练数据用；平时关掉，不然串口被刷满 |
| `USE_BUZZER` | 0 | 1 时 D5 高电平响。Grove 蜂鸣器是高电平响；如果换成 KY-012 一类带三极管的模块，先测极性 |
| `COUNT_GATE` | 0 | 1 时只有分类器最近一次输出 `curl` 才允许加一（workplan 第 4 节的门控）。**现在的 `model_weights.h` 是合成数据训出来的，对真实动作输出不可信，所以默认关**；真模型进来后改成 1 |
| `CURL_AXIS` | 3（gx） | 沿握把的陀螺仪轴。`data-protocol.md` 提议 X 轴沿握把，所以是 gx。安装方向定了之后核对 |

## 各部分怎么工作

- **IMU**：104 Hz 定时读，和 `firmware/classifier` 一样进 208 点环形缓冲，每 104 个样本（1 s）算一次 31 个特征、跑一次模型，串口打 `C,t,label,conf,us`。
- **计数器**（占位实现，归线 2 / Julia）：对 `CURL_AXIS` 那一列做一阶低通（α = 0.2），一次完整动作 = 角速度先超过 ±60 deg/s、再反向超过 ∓60 deg/s、最后回落到 25 deg/s 以内，整段 0.6 ～ 5 s。太快当晃动，太慢当停住，都不算。只做一半不算。参数全是猜的，要用录好的数据离线调（workplan 线 2 第 1 步）。
- **心率**：MAX30102 400 sps 平均 4 = 100 Hz 出数（bring-up 05 的结论：25 Hz 时 SparkFun 的 `checkForBeat` 几乎不工作）。每个 loop 最多处理 8 个 FIFO 样本，避免拖慢 IMU。记录最近 9 次心跳时刻，取 8 个间隔的**中位数**算 bpm。**有效**的条件：手指在（IR > 50000）、有效间隔 ≥ 4 个、最近一次心跳在 3 s 内；否则显示 `HR --`（手指不在）或 `HR ...`（手指在但还没稳），并且串口 `H` 行 `valid=0`、`bpm=0`，不拿旧值冒充（FR-08）。bring-up 05 里静止心率仍比手表高 9 ～ 14 bpm，这个 sketch 没有解决那个问题，只是把有效性判断接上了。
- **屏幕**：第一行状态 + 心率，中间大号次数 / 目标，底行分类标签和置信度 + 计时。每 250 ms 或有事件时重画。**发送是分块的**（`serviceOLED()`）：Adafruit 的 `display()` 一次推 1 KB，在总线上占 32 ms，实测每刷一次 IMU 迟 3 个采样周期、补读出 3 个重复样本（每秒 4 次刷屏 → 约 11 % 的样本是重复值）。现在每次 loop 只发 64 字节，而且只在离下一次 IMU 采样还有 3 ms 以上时才发；一帧没发完不重画。改完 `imu_late` 为 0、`imu_dt_max` 10 ms，和不接屏一样。
- **I2C 时钟**：`Adafruit_SSD1306` 构造函数默认 `clkAfter = 100 kHz`，每次对屏操作后把整条总线降到 100 kHz，IMU 和 MAX30102 也跟着慢（`Wire.getClock()` 实测 100000，64 字节分块要 6 ms）。构造时把 `clkDuring`、`clkAfter` 都给 400000 才真的是 400 kHz。
- **串口**：所有行都以类型字母开头，每 5 s 一条 `S` 统计，和 06 一样用来验证 FR-13（`imu_hz` 103 ～ 105、`imu_late` 不涨、`imu_dt_max_ms` 接近 10、`loop_max_us` 小于 9600）。`loop_max_us` 现在包含重画和分块发送（v0 第一版不包含，所以看不出刷屏阻塞；判断阻塞看 `imu_late` 和 `imu_dt_max_ms`）。

## 和 `firmware/classifier` 的关系

`classifier.h`、`model_weights.h` 是从 `firmware/classifier/` **复制**过来的（Arduino 不能 include 兄弟目录）。重新训练后两边都要更新：

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

上板第一次要看的东西，按顺序：

1. 串口第一行 `# app v0: ... oled 1 hr 1`。`oled 0` 或 `hr 0` 说明那个模块没扫到，先跑 `03_i2c_scan`。
2. 不碰按钮，看 `S` 行：`imu_hz` 103 ～ 105，`imu_late` 为 0，`imu_dt_max_ms` 10 ～ 11。（2026-11-06 实测：104.0 / 0 / 10，`loop_max_us` 约 6400，`hr_n` 约 498。）
3. 短按：屏幕 `IDLE` → `RUN`，串口 `EVENT,...,start`。再短按 `PAUSE`，再短按 `RUN`。按住 1.5 s：`DONE`，串口 `end`。按 1 s 松开：什么都不发生。
4. `RUN` 状态下做弯举：每次有 `R,` 行、LED 闪、次数加一。晃一下、只举一半：不加。暂停时做：不加。
5. 手指放到 MAX30102：`HR --` → `HR ...` → 约 10 s 后出数字；拿开 3 s 内回到 `HR --`。
6. 做 10 次：屏幕反色、LED 长亮一次；继续做，次数继续涨。

结果写进 `tests/results/`，模板 `tests/templates/test-record.md`。
