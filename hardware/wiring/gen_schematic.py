# Generates schematic-v1.svg. Run: python3 hardware/wiring/gen_schematic.py hardware/wiring/schematic-v1.svg
import sys
W, H = 1420, 1080
F = "Helvetica, Arial, sans-serif"
C_GND, C_3V3, C_SCL, C_SDA, C_SIG, C_TXT, C_SUB = "#111", "#c62828", "#b58900", "#1565c0", "#333", "#111", "#555"
out = []
def add(s): out.append(s)
def text(x, y, s, size=12, anchor="start", weight="normal", fill=C_TXT, family=F, extra=""):
    s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    add(f"<text x='{x}' y='{y}' font-size='{size}' text-anchor='{anchor}' font-weight='{weight}' fill='{fill}' font-family='{family}' {extra}>{s}</text>")
def line(x1, y1, x2, y2, stroke=C_SIG, w=1.8, dash=None):
    d = f" stroke-dasharray='{dash}'" if dash else ""
    add(f"<line x1='{x1}' y1='{y1}' x2='{x2}' y2='{y2}' stroke='{stroke}' stroke-width='{w}' stroke-linecap='round'{d}/>")
def poly(pts, stroke=C_SIG, w=1.8, fill="none", dash=None):
    d = f" stroke-dasharray='{dash}'" if dash else ""
    add(f"<polyline points='{' '.join(f'{x},{y}' for x, y in pts)}' fill='{fill}' stroke='{stroke}' stroke-width='{w}' stroke-linejoin='round' stroke-linecap='round'{d}/>")
def dot(x, y, c=C_SIG): add(f"<circle cx='{x}' cy='{y}' r='3.6' fill='{c}'/>")
def box(x, y, w, h, fill="#f7f7f7", stroke="#333", rx=5, dash=None):
    d = f" stroke-dasharray='{dash}'" if dash else ""
    add(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='{rx}' fill='{fill}' stroke='{stroke}' stroke-width='1.5'{d}/>")
def gnd_symbol(x, y):  # stem down from (x,y), then three bars
    line(x, y, x, y + 14, C_GND); line(x - 12, y + 14, x + 12, y + 14, C_GND, 2)
    line(x - 8, y + 19, x + 8, y + 19, C_GND, 2); line(x - 4, y + 24, x + 4, y + 24, C_GND, 2)
def v33_flag(x, y):  # stem up from (x,y), bar and label
    line(x, y, x, y - 14, C_3V3); line(x - 10, y - 14, x + 10, y - 14, C_3V3, 2.2)
    text(x, y - 19, "3V3", 11, "middle", "bold", C_3V3)
def resistor(x1, x2, y, label):  # horizontal IEC rectangle
    w = x2 - x1; line(x1, y, x1 + 10, y); box(x1 + 10, y - 7, w - 20, 14, "white", C_SIG, 1); line(x2 - 10, y, x2, y)
    text((x1 + x2) / 2, y - 12, label, 11, "middle")
def led(x, y, label):  # anode at (x,y) pointing right, cathode at x+36
    line(x, y, x + 10, y); add(f"<polygon points='{x+10},{y-9} {x+10},{y+9} {x+26},{y}' fill='white' stroke='{C_SIG}' stroke-width='1.8'/>")
    line(x + 26, y - 9, x + 26, y + 9); line(x + 26, y, x + 36, y)
    for dx in (14, 20):
        line(x + dx, y - 10, x + dx + 7, y - 18, C_SIG, 1.4); add(f"<polygon points='{x+dx+7},{y-18} {x+dx+2},{y-17} {x+dx+6},{y-13}' fill='{C_SIG}'/>")
    text(x + 18, y + 24, label, 11, "middle")
def switch(x, y, label, sub):  # NO pushbutton from (x,y) to (x+60,y)
    line(x, y, x + 14, y); line(x + 46, y, x + 60, y)
    add(f"<circle cx='{x+16}' cy='{y}' r='3' fill='white' stroke='{C_SIG}' stroke-width='1.6'/>")
    add(f"<circle cx='{x+44}' cy='{y}' r='3' fill='white' stroke='{C_SIG}' stroke-width='1.6'/>")
    line(x + 16, y - 10, x + 44, y - 10, C_SIG, 2); line(x + 30, y - 10, x + 30, y - 20, C_SIG, 1.6)
    text(x + 30, y - 26, label, 11, "middle", "bold"); text(x + 30, y + 18, sub, 10.5, "middle", "normal", C_SUB)

add(f"<svg xmlns='http://www.w3.org/2000/svg' width='{W}' height='{H}' viewBox='0 0 {W} {H}'>")
add(f"<rect width='{W}' height='{H}' fill='white'/>")
text(20, 30, "Smart dumbbell - schematic v1 (2026-11-04)", 19, weight="bold")
text(20, 50, "Modules as photographed 2026-11-04. I2C section matches the bench build of 2026-11-03 (scan: 0x1C 0x3C 0x57 0x6A). Buttons, LED and buzzer are the planned connections, not yet tested.", 11.5, fill=C_SUB)
text(20, 66, "Pin names are the Arduino names printed on the Nano ESP32 header. All logic is 3.3 V. A dot is a junction; a crossing without a dot is not a connection.", 11.5, fill=C_SUB)

# ---------- Nano ESP32 ----------
NX, NY, NW, NH = 640, 180, 240, 640
box(NX, NY, NW, NH, "#e8f0fe")
text(NX + NW / 2, NY + 22, "Arduino Nano ESP32", 14, "middle", "bold")
text(NX + NW / 2, NY + 40, "ABX00083, u-blox NORA-W106 (ESP32-S3)", 10.5, "middle", fill=C_SUB)
text(NX + NW / 2, NY + 56, "on-board 3.3 V LDO feeds the 3V3 pin", 10.5, "middle", fill=C_SUB)

# USB-C from power bank
PBX, PBY, PBW, PBH = 660, 86, 200, 56
box(PBX, PBY, PBW, PBH, "#fff8e1")
text(PBX + PBW / 2, PBY + 22, "INIU SnapGo P81 power bank", 12, "middle", "bold")
text(PBX + PBW / 2, PBY + 40, "5 V over its built-in USB-C cable", 10.5, "middle", fill=C_SUB)
line(760, PBY + PBH, 760, NY, C_3V3, 2.2)
text(766, NY - 8, "USB-C (VBUS)", 11, fill=C_3V3)

# Left pins: order chosen so rails fan out without crossing each other at the Nano
LP = [("GND", 260, C_GND, 420), ("3V3", 300, C_3V3, 470), ("A5 / SCL", 340, C_SCL, 520), ("A4 / SDA", 380, C_SDA, 570)]
RAIL_BOTTOM = 1030
for name, y, c, rx in LP:
    line(NX, y, NX - 30, y, c); text(NX + 8, y + 4, name, 12, "start", "bold")
    line(NX - 30, y, rx, y, c); line(rx, y, rx, RAIL_BOTTOM, c, 2.2)
    dot(rx, y, c)
text(420, 1052, "GND", 11, "middle", "bold", C_GND); text(470, 1052, "3V3", 11, "middle", "bold", C_3V3)
text(520, 1052, "SCL", 11, "middle", "bold", C_SCL); text(570, 1052, "SDA", 11, "middle", "bold", C_SDA)
RAIL = {"GND": (420, C_GND), "3V3": (470, C_3V3), "SCL": (520, C_SCL), "SDA": (570, C_SDA)}

# Right pins
RP = [("D2", 260), ("D3", 330), ("D4", 400), ("D5", 490), ("D7", 640), ("D8", 720), ("D9", 760)]
for name, y in RP:
    text(NX + NW - 8, y + 4, name, 12, "end", "bold")
for name, y in RP[:5]:
    line(NX + NW, y, NX + NW + 30, y)
line(NX + NW, 720, NX + NW + 30, 720, C_SIG, 1.5, "4 4"); line(NX + NW, 760, NX + NW + 30, 760, C_SIG, 1.5, "4 4")
text(NX + NW + 36, 724, "reserved: GY-MAX30102 INT (not wired in v1, polling used)", 10.5, fill=C_SUB)
text(NX + NW + 36, 764, "reserved: 4517 INT1 data-ready (not wired in v1)", 10.5, fill=C_SUB)

# ---------- I2C modules on the left ----------
MX, MW = 60, 290
def module(y0, h, title, sub, pins, notes):
    box(MX, y0, MW, h, "#f3f3f3")
    text(MX + 10, y0 + 20, title, 13, "start", "bold"); text(MX + 10, y0 + 36, sub, 10.5, fill=C_SUB)
    for i, (pname, net, y) in enumerate(pins):
        rx, c = RAIL[net]
        text(MX + MW - 8, y + 4, pname, 12, "end", "bold")
        line(MX + MW, y, rx, y, c); dot(rx, y, c)
    for j, n in enumerate(notes):
        text(MX + 10, y0 + h - 8 - 14 * (len(notes) - 1 - j), n, 10, fill=C_SUB)

module(430, 195, "Adafruit 4517  LSM6DSOX + LIS3MDL", "STEMMA QT cable 4209 (either JST connector)",
       [("GND  black", "GND", 490), ("V+  red", "3V3", 520), ("SDA  blue", "SDA", 550), ("SCL  yellow", "SCL", 580)],
       ["I2C 0x6A (LSM6DSOX), 0x1C (LIS3MDL, unused)", "10 k pull-ups to 3.3 V on board"])
module(645, 215, "GY-MAX30102 pulse oximeter", "0.1 in header, pins per silkscreen",
       [("VIN", "3V3", 705), ("GND", "GND", 735), ("SCL", "SCL", 765), ("SDA", "SDA", 795)],
       ["I2C 0x57;  INT pin left open (see D8)", "4.7 k pull-ups to on-board 1.8 V: measure bus high level"])
module(880, 165, "Kit OLED 0.96 in  SSD1306 128x64", "pin order GND VCC SCL SDA (silkscreen, photo 2026-11-04)",
       [("GND", "GND", 940), ("VCC", "3V3", 970), ("SCL", "SCL", 1000), ("SDA", "SDA", 1030)],
       ["I2C 0x3C"])
# MAX30102 INT stub
text(MX + MW - 8, 829, "INT", 12, "end", "bold", C_SUB); line(MX + MW, 825, MX + MW + 20, 825, C_SUB, 1.4, "3 3")
text(MX + MW + 26, 829, "n.c.", 10.5, fill=C_SUB)

# ---------- Right side: buttons, buzzer, LED ----------
SX = NX + NW + 30  # 910
for (name, y), (lab, sub) in zip(RP[:3], [("SW1  Start", "kit pushbutton, to GND"), ("SW2  Pause / resume", "kit pushbutton, to GND"), ("SW3  End", "kit pushbutton, to GND")]):
    line(SX, y, 1000, y); switch(1000, y, lab, sub); line(1060, y, 1100, y); gnd_symbol(1100, y)
text(1180, 250, "INPUT_PULLUP on D2 / D3 / D4;", 10.5, fill=C_SUB); text(1180, 264, "no external resistors.", 10.5, fill=C_SUB)
text(1180, 282, "Pressed = LOW.", 10.5, fill=C_SUB)

# Grove buzzer module
GX, GY, GW, GH = 1090, 430, 260, 192
box(GX, GY, GW, GH, "#f3f3f3")
text(GX + 10, GY + 20, "Grove Buzzer (Seeed)", 13, "start", "bold"); text(GX + 10, GY + 36, "J1 Grove 4-pin; Q1 transistor drives BUZ1", 10.5, fill=C_SUB)
gp = [("SIG  yellow", 490), ("NC  white", 520), ("VCC  red", 550), ("GND  black", 580)]
for pname, y in gp:
    text(GX + 10, y + 4, pname, 12, "start", "bold")
line(SX, 490, GX, 490)                       # D5 -> SIG
line(GX, 550, 1040, 550, C_3V3); v33_flag(1040, 550)
line(GX, 580, 1040, 580, C_GND); gnd_symbol(1040, 580)
text(GX + 10, GY + GH - 22, "Needs Grove-to-jumper cable (Seeed 110990210)", 10, fill=C_SUB)
text(GX + 10, GY + GH - 8, "or a cut Grove cable. HIGH on SIG = sound.", 10, fill=C_SUB)

# LED + resistor on D7
line(SX, 640, 960, 640); resistor(960, 1050, 640, "R1  220 ohm"); led(1060, 640, "LED1 (kit, red or yellow)"); line(1096, 640, 1140, 640); gnd_symbol(1140, 640)
text(960, 690, "Alternative: use the on-board RGB LED (LED_RED etc., active low) and drop this net.", 10.5, fill=C_SUB)

# Notes box
bx, by, bw, bh = 920, 800, 480, 200
box(bx, by, bw, bh, "#fffdf5", "#999", 4)
text(bx + 12, by + 22, "Notes", 12.5, "start", "bold")
notes = [
    "1. Power: Nano only from USB-C. No VIN, no bare LiPo. 3V3 pin supplies every module.",
    "2. One I2C bus at 400 kHz: 0x6A, 0x1C, 0x57, 0x3C. Confirmed by 03_i2c_scan on 2026-11-03.",
    "3. Bus high level: 4517 10 k to 3.3 V in parallel with MAX30102 4.7 k to 1.8 V gives about 2.7 V.",
    "   Measure SDA/SCL high with a meter; below 2.5 V, move or remove the MAX30102 pull-ups.",
    "4. Buttons: three kit pushbuttons, one leg to the pin, the other to GND. Debounce in firmware.",
    "5. Buzzer and LED pins are the ones used by firmware/bringup/07_oled_buttons.",
    "6. Not drawn: Grove vibration motor (D6, only if available), UNO R4 bench board,",
    "   ADXL335 backup, LIS3MDL magnetometer (same board, not used).",
    "Source: hardware/wiring/README.md net list. Regenerate from gen_schematic.py if pins change.",
]
for i, n in enumerate(notes):
    text(bx + 12, by + 42 + 17 * i, n, 10.5, fill="#333")
add("</svg>")
open(sys.argv[1], "w").write("\n".join(out) + "\n")
print("wrote", sys.argv[1], len("\n".join(out)), "bytes")
