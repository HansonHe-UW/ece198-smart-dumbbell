# 11-10 周一 09:10 进度汇报：讲稿、问答预案、清单

格式（课程说明）：5 分钟进度 + 5 分钟问答，带原型，可用笔记本展示。老师"看进度，不指望做完"。迟到立减 50%，缺席 0 分，**9:10 前全组到场**。

幻灯片 8 页，英文（第 8 页是问答页）（2026-11-08 下午更新：现状页加了固件 v1，新增第 5 页数据采集，布局图换成英文版）。讲稿按页给，每页标了秒数，总共 4 分 30 秒，留 30 秒给举台架。

## 1. 讲稿（4:30）

**第 1 页 封面（0:20）**
We are Group 57. Our project is a smart dumbbell for CCCARE's at-home rehab participants. The dumbbell itself recognizes and counts bicep curls, reads heart rate at the grip, and logs each set to a laptop. What you see on the right is a concept render, not the prototype. The prototype is here on the table.

**第 2 页 现状三栏（0:40）**
Three weeks after parts arrived, every module works on the bench and the system is about half integrated. On the left: the controller, the IMU, the pulse sensor and the display all share one I2C bus with no conflicts; the IMU streams at 104 hertz; Wi-Fi works; an integrated firmware with one button, display, pulse sampling and the model call has run on the board; and the schematic has been drawn twice, in EasyEDA and KiCad, and rule-checked. Also merged this week: the schematic drawn in EasyEDA and Zoe's Bluetooth web dashboard. In the middle, this week: we are rewiring the bench after the first breadboard failed; the rep counter and the heart-rate detector in the integrated app are placeholders that still need tuning on real motion; the recording rig is ready. On the right, honestly not started: a model trained on real data, the device-side Bluetooth service for the dashboard, all ten acceptance tests, and the printed enclosure.
（台架接好的话此处举起来按一下按钮，屏幕亮，说 "This is the bench version; the counter is a placeholder."，20 秒内放下。）

**第 3 页 台架实测（0:45）**
This is the schematic as built, and the numbers we read from the serial port. Four of four I2C devices; 104.0 hertz with zero lost samples while the display is on; pulse sampling at 100 hertz; model inference 2.3 milliseconds; button events logged. Two things are not done: the rep counter has not been tested on a real curl, and the heart-rate value is still 9 to 14 beats high. One bug we found and fixed: the display library quietly dropped the bus to 100 kilohertz and a full-frame refresh delayed IMU samples, so we now send the frame in 64-byte chunks between samples.

**第 4 页 计数与分类（0:45）**
Counting is two parts. A classifier looks at 2-second windows of IMU data, computes 31 simple features and runs logistic regression on the Nano: curl, idle or other. A counter watches the gyro for an up-then-down cycle, and it only counts while the label is curl. Either part alone fails: the counter alone counts any swing, the classifier alone cannot tell one rep from two. Status: the training pipeline and the on-device code agree to one part in a million, but only on synthetic data; the counter thresholds are guesses and have not been tested on a real curl. Real recordings start this week, and we evaluate by person, never by random windows. Targets: macro-F1 at least 0.85, count within one on 18 of 20 sets. Those are proposed targets, not results.

**第 5 页 数据采集（0:35）**
You will ask where the training data comes from, so here is the plan. Until the enclosure is printed we tape the IMU to a three-pound hex dumbbell, X axis along the grip, photograph that orientation and never change it; the printed head mounts the sensor the same way. Six people with anonymous IDs, each recording slow, normal and fast curls, three kinds of idle, two minutes of other movements and a minute of half reps, as 104 hertz CSV over USB. We evaluate by person, never by random windows. The weight: main recordings at 1.4 kilograms with checks at 0.9 and 2.3. The product target is 1.5 kilograms, adjustable from 1 to 3 with washers, because senior strength classes use 1 to 3 pound dumbbells and early cardiac rehab starts at 1 to 5. As of today we have zero recordings; the first three people are this week.

**第 6 页 心率（0:45）**
Heart rate at the grip is our biggest open risk, and we want your advice on it. At rest, with a still finger, the pulse is only 0.1 to 0.4 percent of the DC level, and our best offline estimate is 9 to 14 beats above a watch reading 86. The library's beat detector is worse; a band-pass and peak-picking replacement is planned but not on the board yet. A curl is about one hertz, the same band as heart rate, so motion will make it harder, not easier. The firmware already does the safe thing: no finger, too few beats or stale data shows dashes and logs invalid. Next: the new detector, a borrowed reference device, and a 60-second synchronized comparison. The question is whether we should restate the requirement before we freeze the test method.

**第 7 页 外壳、计划、问题（0:40）**
The enclosure: electronics and the 5000 milliamp-hour power bank in one hex head, steel washers as ballast in the other, the pulse sensor flush in the grip; 1.5 kilograms, adjustable by washer count. The first print is this week. The road to the 25th: real model and offline counter next week; model on the device and the assembled dumbbell the week after; freeze on the 22nd and rehearse. On the 25th we want to show a 90-second loop: start, ten counted curls, a swing that does not count, grip heart rate, and the set appearing on the laptop. Three questions for you. One: may we restate the in-motion heart-rate requirement? Two: what does the symposium look like, a table, power, how long per visitor? Three: we have spent 120 dollars of our own; is a 200-dollar ceiling reasonable?

## 2. 分工建议

| 页 | 讲 | 备注 |
|---|---|---|
| 1–2 | Hanson | 举台架的也是 Hanson（台架在他手上） |
| 3 | Zoe | 硬件线负责人讲实测 |
| 4 | Julia | 计数线；Hanson 补分类一句 |
| 5 | Hanson | 数据采集和重量依据（分类线） |
| 6 | Zoe | 心率归硬件线 |
| 7 | Eric | 外壳 + 计划 + 问题 |
| 8 | 全组 | 问答页：三个问题的卡片，提问和答问都停在这页 |

四个人都要开口，老师看的是团队。每人讲自己那页前先对一遍讲稿。

## 3. 问答预案（5 分钟）

| 可能的问题 | 答法 |
|---|---|
| 为什么不用课程给的 R4 Minima？ | 32 kB SRAM 跑不了模型加 Wi-Fi，3.3 V 脚带不动 Wi-Fi 模块；Nano ESP32 一块板解决（decisions.md 10-31）。R4 留作桌面调试。 |
| 为什么自己买 IMU？ | 借的 ADXL335 没有陀螺仪，弯举计数要角速度；LSM6DSOX 有陀螺仪、I2C、104 Hz。 |
| 模型多大、在板上跑得动吗？ | 31 维特征的逻辑回归，几 KB；实测推理 2.3 ms，Flash 10%、RAM 12%。 |
| 数据从哪来、够不够？ | 第 5 页已答：临时台架 + 匿名志愿者 6 人、每类 10 分钟以上、按人划分评估。目前 0 人，是最大的进度缺口。 |
| 为什么是 1.5 kg？ | 老年力量课让学员带 1–3 lb，心脏康复前几周 1–5 lb，CCCARE 资料说负荷按人定、从轻起步、做满 20 次再加。所以演示 1.5 kg，垫圈可调 1–3 kg。5 kg 高于所有查到的范围。 |
| 为什么同时写了 Wi-Fi 和蓝牙？ | 规格写 Wi-Fi，热点模式不依赖校园网；蓝牙是备选，连接更简单、功耗低。两者发同一条 JSON，记录器不用改。想听老师意见选一个。 |
| 运动中心率做不到怎么办？ | 实话：大概率做不到规格原文。兜底是信号允许时给读数、组间静止保证输出，并把无效标记出来；我们带着数据来问能不能这样改。 |
| 怎么防止晃动被数成一次？ | 两道关：分类器标签必须是 curl，计数器要求完整的上举加下放周期，半次、停住不算。AT-03 专门测 20 次干扰。 |
| 和手机 app 比有什么意义？ | 老人不用手机操作，一个按钮一块屏；数据自动进电脑给康复师看。 |
| 11-25 能演示什么？ | 90 秒循环：开始、数 10 次、晃动不计、握把心率、电脑记录。模型不行就只演计数并说明。 |
| 预算超了怎么办？ | 自费 120，超原定 100；剩下外壳打印约 60–80。想确认 200 的上限。 |
| 为什么一个按钮而不是三个？ | 老人找一个大按钮比三个小的容易；短按开始/暂停、长按结束，固件已实现。规格还写三个，等组内定了改。 |

不会答的就说 "We haven't measured that yet" 加上打算怎么测，不要编数字。

## 4. 周一前要做的（按时间）

**周六晚 / 周日**

- [ ] Hanson：半长面包板按 `hardware/wiring/halfboard-v1.png` 接好；`bus_check` 四个地址 `rc=0`；`firmware/app` 烧回去，屏幕有 `IDLE 0/10`。接不好就不带台架，幻灯片不变。
- [ ] Hanson：把幻灯片（链接见下）过一遍，改措辞；导出 PDF 一份存本地，防止现场没网。
- [ ] 全组：各自读自己那页的讲稿，群里确认谁讲哪页。
- [ ] Zoe：带 MAX30102 静止测试那份记录里的数字（已经在第 5 页）。
- [ ] Eric：如果 CAD 新版本有截图，替换第 6 页左图；没有就用现在的布局图。

**周一早上**

- [ ] 8:40 到场；笔记本充满电；幻灯片 PDF 在本地；台架用笔记本 USB 供电（不用充电宝，少一个变量）。
- [ ] 上台前在走廊通一次电，看屏幕亮。
- [ ] 带：Nano 台架、USB-C 线 ×2、备用 Nano（如果有）、这份文件打印或手机上开着。
- [ ] 讲完记下老师对三个问题的回答，当天写进 `docs/decisions.md`。

## 5. 幻灯片

**正式版（UW Engineering 模板）**：`presentation/Smart_Dumbbell_Update_2026-11-10.pptx`，同目录有 PDF 备份（现场没网也能放）。模板字体 Barlow Condensed（标题）+ Georgia（正文），紫色系按模板主题色。演讲者备注里是每页的中文讲稿。由 `presentation/build_update_deck_2026-11-10.py` 从 `engineering_powerpoint_template_16-9_widescreen.potx` 生成，改内容改脚本重跑。

草稿版在 Claude 的 Slides 页面：https://claude.ai/artifact/EA2WWZK6PHP8GUEYefHGGR （私有，只有 Hanson 能开），内容相同，仅作备份。
