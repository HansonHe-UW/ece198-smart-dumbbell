# Recording rig for IMU training data before the enclosure exists. Run: python3 ml/figures/gen_recording_rig.py ml/figures/recording-rig-v1.svg
import sys, math
W, H = 1500, 1000
F = "PingFang SC, Helvetica Neue, Arial, sans-serif"
o = []
def a(s): o.append(s)
def text(x, y, s, size=14, anchor="start", weight="normal", fill="#222"):
    a(f"<text x='{x}' y='{y}' font-size='{size}' text-anchor='{anchor}' font-weight='{weight}' fill='{fill}' font-family='{F}'>{s}</text>")
def rect(x, y, w, h, fill, stroke="#222", sw=1.5, rx=6, dash=None):
    d = f" stroke-dasharray='{dash}'" if dash else ""
    a(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='{rx}' fill='{fill}' stroke='{stroke}' stroke-width='{sw}'{d}/>")
def line(x1, y1, x2, y2, stroke="#222", w=1.5, dash=None):
    d = f" stroke-dasharray='{dash}'" if dash else ""
    a(f"<line x1='{x1}' y1='{y1}' x2='{x2}' y2='{y2}' stroke='{stroke}' stroke-width='{w}' stroke-linecap='round'{d}/>")
def arrow(x1, y1, x2, y2, c, label):
    line(x1, y1, x2, y2, c, 3)
    ang = math.atan2(y2 - y1, x2 - x1)
    p = [(x2, y2), (x2 - 14 * math.cos(ang - 0.4), y2 - 14 * math.sin(ang - 0.4)), (x2 - 14 * math.cos(ang + 0.4), y2 - 14 * math.sin(ang + 0.4))]
    a(f"<polygon points='{' '.join(f'{x},{y}' for x, y in p)}' fill='{c}'/>")
    if x2 < x1: text(x2 - 8, y2 + 5, label, 15, "end", "bold", c)
    elif y2 < y1: text(x2, y2 - 8, label, 15, "middle", "bold", c)
    else: text(x2 + 8, y2 + 5, label, 15, "start", "bold", c)
def hexside(cx, cy, w, h, fill):   # hex prism seen from the side: rectangle with the two flat-edge lines
    rect(cx - w / 2, cy - h / 2, w, h, fill, "#222", 2, 4)
    line(cx - w / 2, cy - h / 4, cx + w / 2, cy - h / 4, "#555", 1); line(cx - w / 2, cy + h / 4, cx + w / 2, cy + h / 4, "#555", 1)
def label(px, py, tx, ty, s, anchor="start"):
    line(px, py, tx, ty, "#777", 1); a(f"<circle cx='{px}' cy='{py}' r='3' fill='#777'/>"); text(tx + (4 if anchor == "start" else -4), ty + 5, s, 13.5, anchor, fill="#333")

a(f"<svg xmlns='http://www.w3.org/2000/svg' width='{W}' height='{H}' viewBox='0 0 {W} {H}'><rect width='{W}' height='{H}' fill='white'/>")
text(30, 40, "IMU 录数据临时台架 v1（外壳打出来之前用）", 24, weight="bold")
text(30, 66, "只有 Nano 和 4517 跟着哑铃动，电脑这边只有一根 USB-C 线。4517 的朝向一旦定下就不再改：X 轴沿握把指向握把，元件面朝上（哑铃平放、OLED 那一面朝上时）。", 14, fill="#444")

# ---------- Panel A: 2 lb hex neoprene dumbbell ----------
text(30, 110, "方案 A（推荐）：2 lb / 1 kg 六角泡棉哑铃，Amazon 约 CAD 10–15，明天到", 17, weight="bold", fill="#1a4d8f")
cy = 300
hexside(230, cy, 150, 150, "#7a7f88"); hexside(770, cy, 150, 150, "#7a7f88")
rect(305, cy - 18, 390, 36, "#9aa0a8", "#222", 2, 18)     # handle
text(500, cy + 5, "握把 Ø30 左右，长约 120", 13, "middle", fill="#fff")
# 4517 on the top flat of the right head, near the handle
rect(700, cy - 75 - 14, 50, 14, "#1e5aa8", "#123", 1.5, 2)
text(694, cy - 78, "4517", 12, "end", "bold", "#1e5aa8")
# half breadboard strapped on the far side of the right head (shown behind, dashed) + Nano
rect(735, cy - 40, 110, 70, "#f0f0f0", "#444", 1.5, 4, "6 4")
rect(752, cy - 25, 60, 24, "#2e7d32", "#1b5e20", 1.2, 3)
text(782, cy - 8, "Nano", 11, "middle", "bold", "#fff")
text(790, cy + 48, "半长面包板，橡皮筋绑在头的外侧面", 12, "middle", fill="#444")
# QT cable from 4517 to breadboard
a(f"<path d='M750,{cy-82} C790,{cy-82} 800,{cy-60} 790,{cy-40}' fill='none' stroke='#b58900' stroke-width='3'/>")
text(860, cy - 96, "4209 线 150 mm", 12, fill="#8a6d00")
# USB cable out
a(f"<path d='M845,{cy-5} C900,{cy-5} 950,{cy+60} 1040,{cy+60}' fill='none' stroke='#222' stroke-width='3'/>")
text(1050, cy + 65, "USB-C 1–2 m → 笔记本（供电 + 录制）", 13, fill="#222")
# axes at the 4517
arrow(725, cy - 120, 640, cy - 120, "#c62828", "X 沿握把，指向握把")
arrow(725, cy - 120, 725, cy - 185, "#2e7d32", "Z 朝上（元件面）")
a(f"<circle cx='725' cy='{cy-120}' r='5' fill='#1565c0'/>"); text(738, cy - 126, "Y 朝读者", 13, "start", "bold", "#1565c0")
label(740, cy - 82, 1000, cy - 150, "双面泡棉胶贴在六角头顶面，靠近握把，元件面朝上", "start")
text(30, 420, "质量：哑铃 0.9 kg + 板子和线约 80 g ≈ 1 kg，和成品目标一致。六角面是平的，胶带贴得牢、哑铃放桌上不滚，Z 轴方向每次一样。", 13.5, fill="#333")

# ---------- Panel B: pipe + bottles ----------
text(30, 480, "方案 B（今晚零成本）：Ø30 左右的硬杆（PVC 管、擀面杖、拖把杆）两端各绑一瓶 500 mL 水", 17, weight="bold", fill="#1a4d8f")
cy = 680
rect(150, cy - 70, 90, 180, "#9ec5e8", "#355", 2, 24); rect(840, cy - 70, 90, 180, "#9ec5e8", "#355", 2, 24)
text(195, cy + 30, "500 mL", 12, "middle", fill="#123"); text(885, cy + 30, "500 mL", 12, "middle", fill="#123")
rect(120, cy - 10, 840, 26, "#c8b68a", "#5a4a2a", 2, 13)   # rod
text(540, cy + 8, "硬杆，长 40–50 cm，中间 12 cm 留给手", 13, "middle", fill="#3a2e14")
for x in (170, 220, 860, 910):
    line(x, cy - 72, x, cy + 112, "#222", 2, "3 3")      # tape bands
text(195, cy + 128, "胶带绕两圈", 12, "middle", fill="#444"); text(885, cy + 128, "胶带绕两圈", 12, "middle", fill="#444")
rect(760, cy - 10 - 14, 50, 14, "#1e5aa8", "#123", 1.5, 2); text(754, cy - 14, "4517", 12, "end", "bold", "#1e5aa8")
rect(845, cy - 140, 110, 60, "#f0f0f0", "#444", 1.5, 4, "6 4"); rect(862, cy - 125, 60, 24, "#2e7d32", "#1b5e20", 1.2, 3); text(892, cy - 108, "Nano", 11, "middle", "bold", "#fff")
text(900, cy - 150, "半长面包板用橡皮筋绑在瓶身上", 12, "middle", fill="#444")
a(f"<path d='M810,{cy-20} C830,{cy-40} 840,{cy-90} 850,{cy-100}' fill='none' stroke='#b58900' stroke-width='3'/>")
a(f"<path d='M955,{cy-110} C1000,{cy-110} 1020,{cy-40} 1080,{cy-40}' fill='none' stroke='#222' stroke-width='3'/>")
text(1090, cy - 35, "USB-C → 笔记本", 13, fill="#222")
arrow(785, cy - 60, 700, cy - 60, "#c62828", "X 指向握把")
arrow(785, cy - 60, 785, cy - 125, "#2e7d32", "Z 元件面朝上")
label(770, cy - 24, 600, cy - 150, "4517 贴在杆上、紧挨右瓶；杆是圆的，先用记号笔画一条线当'上'，每次贴同一条线", "end")
text(30, 830, "质量：两瓶水 1 kg + 杆 0.2–0.4 kg ≈ 1.3 kg，偏重一点没关系。缺点：杆是圆的，Z 方向靠画线对齐，误差几度；握感和成品不同。", 13.5, fill="#333")

# ---------- Rules ----------
text(30, 890, "两个方案通用的规矩", 16, weight="bold")
rules = ["1. 4517 必须贴死，不能晃：泡棉双面胶 + 外面一圈胶带。晃动会把 104 Hz 的振动当成动作特征学进去。",
         "2. 朝向拍照存 ml/figures/mounting-v1.jpg，写进 ml/data-protocol.md 第 3 节，然后不改。将来壳里的 4517 也按同样朝向装，数据才能复用。",
         "3. 面包板上只插 Nano，4209 的四根公头直接插在 Nano 引脚旁边的孔：3V3、GND、A4、A5。不接 OLED、心率模块。",
         "4. 录之前空放桌上 5 秒看 az 约 +9.8、gx gy gz 约 0，说明 Z 朝上没贴反；做两次弯举看 gx 峰值明显大于 gy gz，说明 X 沿握把。"]
for i, r in enumerate(rules): text(30, 916 + 20 * i, r, 13.5, fill="#333")
a("</svg>")
open(sys.argv[1], "w").write("\n".join(o)); print("ok")
