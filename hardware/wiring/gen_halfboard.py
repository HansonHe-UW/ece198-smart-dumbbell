# Half-size breadboard wiring, Nano off-board on male-female jumpers, one button, no rails.
import sys
W, H = 1540, 1230
out = []
def t(x, y, s, size=12, anchor="start", weight="normal", fill="#222", rot=None):
    tr = f' transform="rotate({rot} {x} {y})"' if rot is not None else ""
    out.append(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" fill="{fill}"{tr}>{s}</text>')
def rect(x, y, w, h, fill, stroke="#333", rx=4, sw=1.2):
    out.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
def wire(x1, y1, x2, y2, color, bulge=60, w=3):
    cx, cy = (x1 + x2) / 2, min(y1, y2) - bulge
    out.append(f'<path d="M{x1},{y1} Q{cx},{cy} {x2},{y2}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linecap="round"/>')
    for x, y in ((x1, y1), (x2, y2)):
        out.append(f'<circle cx="{x}" cy="{y}" r="4" fill="{color}" stroke="#fff" stroke-width="1"/>')
def badge(x, y, n):
    out.append(f'<circle cx="{x}" cy="{y}" r="10" fill="#fff" stroke="#333" stroke-width="1.2"/>')
    t(x, y + 4, str(n), 11, "middle", "bold")

# ---------- breadboard ----------
P = 30
BX, BY = 440, 430
COLS = 30
ROWS_TOP = ["j", "i", "h", "g", "f"]
ROWS_BOT = ["e", "d", "c", "b", "a"]
def cx(c): return BX + 45 + (c - 1) * P
def ry(r):
    if r in ROWS_TOP: return BY + 70 + ROWS_TOP.index(r) * P
    return BY + 70 + 5 * P + 45 + ROWS_BOT.index(r) * P
BW = 45 + (COLS - 1) * P + 45
BH = 70 + 10 * P + 45 + 50
rect(BX, BY, BW, BH, "#f4f1e8", "#999", 10)
# rails (drawn, crossed out)
for yy in (BY + 18, BY + 34, BY + BH - 34, BY + BH - 18):
    for c in range(1, COLS + 1):
        if c % 6 == 0: continue
        out.append(f'<rect x="{cx(c)-3}" y="{yy-3}" width="6" height="6" fill="#d8d4c8"/>')
t(cx(25), BY + 48, "电源轨不用（上下两条都不用），避免轨中间断开", 12, "middle", fill="#a33")
t(cx(25), BY + BH - 44, "电源轨不用", 12, "middle", fill="#a33")
# holes
for r in ROWS_TOP + ROWS_BOT:
    for c in range(1, COLS + 1):
        out.append(f'<rect x="{cx(c)-4}" y="{ry(r)-4}" width="8" height="8" fill="#b8b4a8"/>')
    t(BX + 22, ry(r) + 4, r, 12, "middle", fill="#666")
    t(BX + BW - 22, ry(r) + 4, r, 12, "middle", fill="#666")
for c in range(1, COLS + 1):
    if c == 1 or c % 5 == 0:
        t(cx(c), ry("j") - 14, str(c), 11, "middle", fill="#666")
        t(cx(c), ry("a") + 20, str(c), 11, "middle", fill="#666")
# centre gap
out.append(f'<line x1="{BX+10}" y1="{(ry("f")+ry("e"))/2}" x2="{BX+BW-10}" y2="{(ry("f")+ry("e"))/2}" stroke="#ccc" stroke-width="10"/>')
# hub column labels
for c, name, col in ((1, "3V3", "#d33"), (2, "GND", "#222"), (3, "SDA", "#2a62d6"), (4, "SCL", "#d9a800")):
    out.append(f'<rect x="{cx(c)-11}" y="{ry("j")-12}" width="22" height="{4*P+24}" rx="6" fill="none" stroke="{col}" stroke-width="2" stroke-dasharray="4 3"/>')
    t(cx(c), ry("j") - 28, name, 11, "middle", "bold", col, rot=-60)
t(cx(2), ry("a") + 36, "GND", 11, "middle", "bold", "#222")
out.append(f'<rect x="{cx(2)-11}" y="{ry("e")-12}" width="22" height="{4*P+24}" rx="6" fill="none" stroke="#222" stroke-width="2" stroke-dasharray="4 3"/>')

# ---------- modules hanging off the top edge ----------
def module(c0, pins, title, sub, color):
    x0 = cx(c0) - 22; w = (len(pins) - 1) * P + 44
    rect(x0, BY - 150, w, 150, color, "#223", 8)
    t(x0 + w / 2, BY - 125, title, 13, "middle", "bold", "#fff")
    t(x0 + w / 2, BY - 108, sub, 10, "middle", fill="#dde")
    for i, p in enumerate(pins):
        x = cx(c0 + i)
        out.append(f'<line x1="{x}" y1="{BY-12}" x2="{x}" y2="{ry("j")}" stroke="#444" stroke-width="3"/>')
        t(x, BY - 20, p, 10, "middle", "bold", "#fff", rot=-90)
module(7, ["GND", "VCC", "SCL", "SDA"], "OLED 0.96\"", "SSD1306  0x3C", "#2b4a7a")
module(13, ["VIN", "GND", "SCL", "SDA", "INT"], "GY-MAX30102", "0x57   INT 不接", "#5a3a8a")
# 4517 off board, top-left
rect(BX - 20, BY - 290, 250, 120, "#4a2d8a", "#223", 8)
t(BX + 105, BY - 262, "Adafruit 4517", 14, "middle", "bold", "#fff")
t(BX + 105, BY - 244, "LSM6DSOX + LIS3MDL   0x6A / 0x1C", 10, "middle", fill="#dde")
t(BX + 105, BY - 228, "QT 口插 4209 线，杜邦头直接插第 1–4 列 g 行", 10, "middle", fill="#dde")
t(BX + 105, BY - 212, "元件面朝上，绿灯要亮", 10, "middle", fill="#ffd")
rect(BX + 60, BY - 176, 90, 14, "#e8e8e8", "#555", 2)

# ---------- Nano off board, left ----------
NX, NY, NW = 130, 430, 130
PP = 24
left = ["D13", "3V3", "B0", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "VUSB", "B1", "GND", "VIN"]
right = ["D12", "D11", "D10", "D9", "D8", "D7", "D6", "D5", "D4", "D3", "D2", "GND", "RST", "RX0", "TX1"]
NH = PP * 15 + 50
rect(NX, NY, NW, NH, "#1f6f73", "#123", 8)
rect(NX + NW / 2 - 18, NY - 8, 36, 22, "#bbb", "#555", 3)
t(NX + NW / 2, NY + 30, "USB-C 朝上", 10, "middle", fill="#fff")
t(NX + NW / 2, NY + NH / 2 - 10, "Nano", 13, "middle", "bold", "#fff")
t(NX + NW / 2, NY + NH / 2 + 8, "ESP32", 13, "middle", "bold", "#fff")
t(NX + NW / 2, NY + NH / 2 + 26, "俯视，元件面朝上", 9, "middle", fill="#dfe")
t(NX + NW / 2, NY + NH + 20, "离板，不插面包板", 12, "middle", "bold")
t(NX + NW / 2, NY + NH + 36, "6 根公母杜邦线，母头套在针脚上", 11, "middle")
hl = {"3V3": "#d33", "A4": "#2a62d6", "A5": "#d9a800", "GND": "#222", "D7": "#1aa", "D2": "#2a9d3a"}
pinpos = {}
for i, p in enumerate(left):
    y = NY + 45 + i * PP
    col = hl.get(p) if p in ("3V3", "A4", "A5", "GND") else None
    out.append(f'<circle cx="{NX}" cy="{y}" r="5" fill="{col or "#ccc"}" stroke="#333"/>')
    t(NX - 10, y + 4, p, 10, "end", "bold" if col else "normal", col or "#333")
    pinpos[("L", p)] = (NX, y)
for i, p in enumerate(right):
    y = NY + 45 + i * PP
    col = hl.get(p) if p in ("D7", "D2") else None
    out.append(f'<circle cx="{NX+NW}" cy="{y}" r="5" fill="{col or "#ccc"}" stroke="#333"/>')
    t(NX + NW + 10, y + 4, p, 10, "start", "bold" if col else "normal", col or "#333")
    pinpos[("R", p)] = (NX + NW, y)
t(NX - 10, NY + 45 + 13 * PP + 16, "(VIN 旁边那个)", 8, "end", fill="#555")

# ---------- wires ----------
def nano_wire(side, pin, c, r, color, n, bulge):
    x1, y1 = pinpos[(side, pin)]
    if side == "L":   # route out to the left then over the top of the board
        out.append(f'<path d="M{x1},{y1} C{x1-70},{y1} {x1-70},{NY-60} {NX+NW/2},{NY-60} S{cx(c)-60},{ry(r)-120} {cx(c)},{ry(r)}" fill="none" stroke="{color}" stroke-width="3" stroke-linecap="round"/>')
        out.append(f'<circle cx="{cx(c)}" cy="{ry(r)}" r="4" fill="{color}" stroke="#fff"/>')
        badge(NX + NW / 2 + 40 + 22 * n, NY - 60, n)
    else:
        wire(x1, y1, cx(c), ry(r), color, bulge)
        badge((x1 + cx(c)) / 2, min(y1, ry(r)) - bulge / 2 + 6, n)
nano_wire("L", "3V3", 1, "f", "#d33", 1, 0)
nano_wire("L", "GND", 2, "f", "#222", 2, 0)
nano_wire("L", "A4", 3, "f", "#2a62d6", 3, 0)
nano_wire("L", "A5", 4, "f", "#d9a800", 4, 0)
nano_wire("R", "D2", 20, "a", "#2a9d3a", 5, -230)
nano_wire("R", "D7", 26, "a", "#1aa", 6, -300)
# 4517 QT cable ends into row g, cols 1-4
for i, (col, n) in enumerate(((("#d33"), 7), ("#222", 8), ("#2a62d6", 9), ("#d9a800", 10))):
    x0 = BX + 68 + i * 18
    out.append(f'<path d="M{x0},{BY-162} C{x0},{BY-80} {cx(1+i)},{ry("g")-80} {cx(1+i)},{ry("g")}" fill="none" stroke="{col}" stroke-width="3" stroke-linecap="round"/>')
    out.append(f'<circle cx="{cx(1+i)}" cy="{ry("g")}" r="4" fill="{col}" stroke="#fff"/>')
t(BX + 105, BY - 186, "7 红  8 黑  9 蓝  10 黄", 10, "middle", "bold")
# board jumpers
def bj(c1, r1, c2, r2, color, n, bulge=40):
    wire(cx(c1), ry(r1), cx(c2), ry(r2), color, bulge)
    badge((cx(c1) + cx(c2)) / 2, min(ry(r1), ry(r2)) - bulge / 2 + 2, n)
bj(7, "i", 2, "h", "#222", 11, 40)
bj(8, "i", 1, "h", "#d33", 12, 64)
bj(9, "i", 4, "h", "#d9a800", 13, 28)
bj(10, "i", 3, "h", "#2a62d6", 14, 52)
bj(13, "i", 1, "i", "#d33", 15, 110)
bj(14, "i", 2, "i", "#222", 16, 134)
bj(15, "i", 4, "i", "#d9a800", 17, 86)
bj(16, "i", 3, "i", "#2a62d6", 18, 160)
# 19: GND bridge from top col 2 to bottom col 2, drawn beside the column
out.append(f'<path d="M{cx(2)},{ry("j")} L{cx(2)-16},{ry("j")+8} L{cx(2)-16},{ry("e")-8} L{cx(2)},{ry("e")}" fill="none" stroke="#222" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>')
out.append(f'<circle cx="{cx(2)}" cy="{ry("j")}" r="4" fill="#222" stroke="#fff"/><circle cx="{cx(2)}" cy="{ry("e")}" r="4" fill="#222" stroke="#fff"/>')
badge(cx(2) - 30, (ry("f") + ry("e")) / 2, 19)
bj(22, "b", 2, "d", "#222", 20, -150)
bj(29, "d", 2, "c", "#222", 21, -210)
# button straddling the gap, cols 20 & 22
bx0, by0 = cx(21) - 34, (ry("f") + ry("e")) / 2 - 26
rect(bx0, by0, 68, 52, "#333", "#111", 6)
out.append(f'<circle cx="{cx(21)}" cy="{by0+26}" r="14" fill="#2a9d3a" stroke="#111"/>')
for c in (20, 22):
    for r in ("f", "e"):
        out.append(f'<circle cx="{cx(c)}" cy="{ry(r)}" r="4" fill="#999" stroke="#333"/>')
t(cx(21), ry("e") + 44, "SW1 跨中沟，脚在列 20、22", 10, "middle", "bold")
# resistor + LED
out.append(f'<line x1="{cx(26)}" y1="{ry("b")}" x2="{cx(28)}" y2="{ry("b")}" stroke="#555" stroke-width="2"/>')
rect(cx(27) - 18, ry("b") - 6, 36, 12, "#e9d8a6", "#555", 2)
t(cx(27), ry("b") - 10, "R1 220Ω", 9, "middle")
out.append(f'<circle cx="{cx(28)}" cy="{ry("b")}" r="3" fill="#555"/><circle cx="{cx(26)}" cy="{ry("b")}" r="3" fill="#555"/>')
out.append(f'<circle cx="{cx(28.5)}" cy="{ry("c")+22}" r="11" fill="#e33" stroke="#900"/>')
out.append(f'<line x1="{cx(28)}" y1="{ry("c")}" x2="{cx(28)}" y2="{ry("c")+14}" stroke="#555" stroke-width="2"/><line x1="{cx(29)}" y1="{ry("c")}" x2="{cx(29)}" y2="{ry("c")+14}" stroke="#555" stroke-width="2"/>')
t(cx(28) - 8, ry("c") + 48, "长脚 28", 9, "end")
t(cx(29) + 8, ry("c") + 48, "短脚 29", 9, "start")

# ---------- title and table ----------
t(40, 40, "Smart dumbbell — 半长面包板接线（单按钮版，Nano 离板）", 22, "start", "bold")
t(40, 64, "2026-11-06。不用电源轨；四条总线各占一列当集线柱：列 1 = 3V3，列 2 = GND，列 3 = SDA，列 4 = SCL（f–j 行）。下半区的列 2 也是 GND（跳线 19 桥过去）。先插 Nano 的 6 根，再插模块。", 12)
t(40, 84, "蜂鸣器不装（固件 USE_BUZZER 0）。D3、D4、D5 空着。所有逻辑 3.3 V，Nano 只由 USB-C 供电。", 12)
rows = [
 ("1", "红", "Nano 3V3（左排第 2 个）", "→ 列 1 f"),
 ("2", "黑", "Nano GND（左排第 14 个，VIN 旁）", "→ 列 2 f"),
 ("3", "蓝", "Nano A4 / SDA（左排第 8 个）", "→ 列 3 f"),
 ("4", "黄", "Nano A5 / SCL（左排第 9 个）", "→ 列 4 f"),
 ("5", "绿", "Nano D2（右排第 11 个）", "→ 列 20 a"),
 ("6", "青", "Nano D7（右排第 6 个）", "→ 列 26 a"),
 ("7–10", "红黑蓝黄", "4517 QT 线 4209 的杜邦头", "→ 列 1、2、3、4 的 g 行"),
 ("11", "黑", "OLED GND  列 7 i", "→ 列 2 h"),
 ("12", "红", "OLED VCC  列 8 i", "→ 列 1 h"),
 ("13", "黄", "OLED SCL  列 9 i", "→ 列 4 h"),
 ("14", "蓝", "OLED SDA  列 10 i", "→ 列 3 h"),
 ("15", "红", "MAX30102 VIN  列 13 i", "→ 列 1 i"),
 ("16", "黑", "MAX30102 GND  列 14 i", "→ 列 2 i"),
 ("17", "黄", "MAX30102 SCL  列 15 i", "→ 列 4 i"),
 ("18", "蓝", "MAX30102 SDA  列 16 i", "→ 列 3 i"),
 ("19", "黑", "GND 桥到下半区  列 2 j", "→ 列 2 e"),
 ("20", "黑", "SW1 另一脚  列 22 b", "→ 列 2 d"),
 ("21", "黑", "LED 短脚那列  列 29 d", "→ 列 2 c"),
]
TY = 960
t(40, TY, "跳线清单（1–6 公母线，其余公公线）", 14, "start", "bold")
for i, (n, col, src, dst) in enumerate(rows):
    colx = 40 if i < 9 else 560
    y = TY + 24 + (i % 9) * 20
    t(colx, y, n, 11, "start", "bold"); t(colx + 42, y, col, 11); t(colx + 110, y, src, 11); t(colx + 330, y, dst, 11)
t(1080, TY, "元件", 14, "start", "bold")
for i, s in enumerate([
    "SW1 轻触开关跨中沟，四脚在列 20、22 的 e、f 行；按下时列 20 和列 22 接通。",
    "R1 220 Ω：列 26 b 到列 28 b。LED 长脚列 28 c，短脚列 29 c。",
    "OLED 和 MAX30102 板子悬在面包板上沿外，针脚插 j 行。",
    "4517 元件面朝上放在旁边，绿灯亮才算有电。",
    "每列 5 个孔是通的，上半区和下半区分开；列 2 上半区正好用满 5 个。",
    "插完先跑 bus_check：四个地址都要 rc=0。",
]):
    t(1080, TY + 24 + i * 20, s, 11)
# legend
LX = 1100
t(LX, 120, "线色", 12, "start", "bold")
for i, (c, s) in enumerate((("#d33", "3V3"), ("#222", "GND"), ("#2a62d6", "SDA (A4)"), ("#d9a800", "SCL (A5)"), ("#2a9d3a", "D2 按钮"), ("#1aa", "D7 LED"))):
    x = LX + (i % 3) * 130; y = 142 + (i // 3) * 22
    out.append(f'<line x1="{x}" y1="{y-4}" x2="{x+30}" y2="{y-4}" stroke="{c}" stroke-width="4"/>'); t(x + 38, y, s, 11)

svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="Helvetica, Arial, PingFang SC, sans-serif"><rect width="{W}" height="{H}" fill="#fff"/>' + "".join(out) + "</svg>"
open(sys.argv[1], "w").write(svg)
