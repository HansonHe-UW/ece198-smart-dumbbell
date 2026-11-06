// app: first integrated firmware for the smart dumbbell (v0, 2026-11-06).
// Everything on the bench in one non-blocking loop:
//   LSM6DSOX 104 Hz -> 2 s ring buffer -> curl/idle/other classifier every 1 s (classifier.h)
//   -> hysteresis rep counter on the handle gyro axis, gated by session state (and by the
//      classifier label once COUNT_GATE is 1)
//   MAX30102 100 Hz -> beat intervals -> bpm with a validity flag (FR-08)
//   SSD1306 OLED (frame sent in 64-byte chunks between IMU ticks, see serviceOLED), one pushbutton, LED, optional buzzer.
// Not in this sketch: Wi-Fi (FR-11/12), target sync from the computer (FR-01, TARGET is a constant).
//
// ONE BUTTON on D2 (replaces SW1/SW2/SW3 of 07_oled_buttons):
//   short press  (< 500 ms)   IDLE -> RUNNING,  RUNNING -> PAUSED,  PAUSED -> RUNNING
//   long press   (>= 1.5 s)   RUNNING or PAUSED -> IDLE (session ends, summary on serial); in IDLE: nothing
//   0.5 to 1.5 s              ignored, so a hesitant press does neither
//
// Serial 115200, one record per line (all t_ms = millis()):
//   EVENT,<t_ms>,<what>[,<detail>]
//   C,<t_ms>,<label>,<conf>,<inference_us>       every 1 s, format of ml/workplan.md section 4
//   R,<t_ms>,<reps>,<rep_ms>                     every counted rep
//   H,<t_ms>,<bpm>,<valid>,<ir>                  every 1 s
//   S,<t_ms>,imu_n,imu_hz,imu_dt_max_ms,imu_late,hr_n,loop_max_us    every 5 s (same meaning as in 06)
//   I,<t_ms>,ax,ay,az,gx,gy,gz                   every IMU sample when STREAM_RAW is 1
//
// Libraries: Adafruit LSM6DS, Adafruit SSD1306 (+GFX, BusIO, Unified Sensor), SparkFun MAX3010x.
// classifier.h / model_weights.h are copies of firmware/classifier/ (see README.md).

#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "classifier.h"

// ---------------- configuration ----------------
#define STREAM_RAW   0     // 1: print every IMU sample as I,... (floods the port; for recording only)
#define USE_BUZZER   0     // 1: Grove buzzer on PIN_BUZZ, driven HIGH to sound. 0: LED only.
#define COUNT_GATE   0     // 1: a rep counts only while the last classifier label is "curl".
                           // Leave 0 until model_weights.h comes from real recordings (now synthetic).
#define CURL_AXIS    3     // ring column of the gyro axis along the handle: 3=gx 4=gy 5=gz
                           // (ml/data-protocol.md proposes X along the handle -> gx). Confirm after mounting.

const int PIN_BTN = D2, PIN_BUZZ = D5, PIN_LED = D7;
const int TARGET = 10;                        // FR-01 demo default; computer sync not implemented

const uint32_t DEBOUNCE_MS = 30, SHORT_MAX_MS = 500, LONG_MIN_MS = 1500;

const float    REP_TH      = 60.0f;           // deg/s, filtered gyro must exceed +TH then -TH (either order)
const float    REP_REST_TH = 25.0f;           // deg/s, rep closes when the rate falls back under this
const uint32_t REP_MIN_MS  = 600;             // faster than this is a shake, not a curl
const uint32_t REP_MAX_MS  = 5000;            // slower than this is a stall; restart tracking
const float    REP_LPF     = 0.2f;            // first-order low-pass on the gyro axis

const long     IR_FINGER   = 50000;           // from bring-up 05
const uint32_t HR_STALE_MS = 3000;            // no beat for this long -> invalid (FR-08)
const int      HR_MIN_BEATS = 4;              // beats needed inside the interval window before a value is shown
const int      HR_N_INT    = 8;               // beat intervals kept; bpm = 60000 / median
// --------------------------------------------------

Adafruit_LSM6DSOX sox;
MAX30105 hr;
// clkDuring and clkAfter both 400 kHz. The library default clkAfter is 100 kHz, which it applies after
// EVERY SSD1306 transaction, so with the defaults the whole shared bus (IMU, MAX30102) silently ran at
// 100 kHz once the display was present (Wire.getClock() measured 2026-11-06; a 64-byte chunk took 6 ms).
Adafruit_SSD1306 oled(128, 64, &Wire, -1, 400000UL, 400000UL);
bool oledOk = false, hrOk = false;
uint8_t oledAddr = 0;

enum State { IDLE, RUNNING, PAUSED };
State state = IDLE;
int reps = 0;
uint32_t sessionStartMs = 0, doneUntilMs = 0, lastRepMs = 0;
bool targetShown = false;

// ---------- button ----------
struct Button { bool raw = true, stable = true; uint32_t tRaw = 0, tDown = 0; bool longFired = false; } btn;
enum BtnEvent { BTN_NONE, BTN_SHORT, BTN_LONG };

BtnEvent pollButton() {
  bool r = digitalRead(PIN_BTN);
  uint32_t now = millis();
  if (r != btn.raw) { btn.raw = r; btn.tRaw = now; }
  if (r != btn.stable && now - btn.tRaw >= DEBOUNCE_MS) {
    btn.stable = r;
    if (r == LOW) { btn.tDown = now; btn.longFired = false; }
    else if (!btn.longFired && now - btn.tDown < SHORT_MAX_MS) return BTN_SHORT;
  }
  if (btn.stable == LOW && !btn.longFired && now - btn.tDown >= LONG_MIN_MS) { btn.longFired = true; return BTN_LONG; }
  return BTN_NONE;
}

// ---------- LED / buzzer, non-blocking ----------
uint32_t cueUntil = 0;
void cue(uint32_t ms) {
  digitalWrite(PIN_LED, HIGH);
#if USE_BUZZER
  digitalWrite(PIN_BUZZ, HIGH);
#endif
  cueUntil = millis() + ms;
}
void serviceCue() {
  if (cueUntil && (int32_t)(millis() - cueUntil) >= 0) {
    digitalWrite(PIN_LED, LOW); digitalWrite(PIN_BUZZ, LOW); cueUntil = 0;
  }
}

// ---------- IMU + classifier ----------
const uint32_t IMU_PERIOD_US = 1000000UL / CLF_FS;
uint32_t imuNext = 0, imuN = 0, imuLate = 0, imuLastMs = 0, imuDtMax = 0, loopMaxUs = 0, hrN = 0, statNext = 0;
static float ring[CLF_WINDOW][6];
static float window[CLF_WINDOW][6];
static float feats[CLF_N_FEATURES];
int ringHead = 0, ringCount = 0, sinceInfer = 0;
int lastLabel = -1; float lastConf = 0; uint32_t lastInferUs = 0;
int curlIdx = 0;                               // index of "curl" in CLF_CLASSES

// ---------- rep counter ----------
float wFilt = 0; int repPhase = 0; float repSign = 0; uint32_t repT0 = 0;
bool uiDirty = true;

void countRep(uint32_t t, uint32_t dur) {
  reps++; lastRepMs = t; uiDirty = true;
  Serial.printf("R,%lu,%d,%lu\n", (unsigned long)t, reps, (unsigned long)dur);
  if (reps == TARGET) { cue(400); Serial.printf("EVENT,%lu,target_reached\n", (unsigned long)t); }
  else cue(60);
}

void counterSample(float w, uint32_t t) {
  wFilt += REP_LPF * (w - wFilt);
  bool allow = (state == RUNNING);
#if COUNT_GATE
  allow = allow && (lastLabel == curlIdx);
#endif
  switch (repPhase) {
    case 0:                                                 // waiting for the first excursion
      if (fabsf(wFilt) > REP_TH) { repPhase = 1; repSign = wFilt > 0 ? 1.0f : -1.0f; repT0 = t; }
      break;
    case 1:                                                 // waiting for the opposite excursion
      if (wFilt * repSign < -REP_TH) repPhase = 2;
      else if (t - repT0 > REP_MAX_MS) repPhase = 0;
      break;
    case 2:                                                 // waiting for the rate to settle
      if (fabsf(wFilt) < REP_REST_TH) {
        uint32_t dur = t - repT0; repPhase = 0;
        if (dur >= REP_MIN_MS && dur <= REP_MAX_MS && allow) countRep(t, dur);
      } else if (t - repT0 > REP_MAX_MS) repPhase = 0;
      break;
  }
}

void serviceIMU() {
  if ((int32_t)(micros() - imuNext) < 0) return;
  imuNext += IMU_PERIOD_US;
  if ((int32_t)(micros() - imuNext) > 0) imuLate++;

  sensors_event_t a, g, tmp;
  sox.getEvent(&a, &g, &tmp);
  uint32_t now = millis();
  uint32_t dt = now - imuLastMs; imuLastMs = now;
  if (imuN > 0 && dt > imuDtMax) imuDtMax = dt;
  imuN++;

  const float R2D = 57.2957795f;
  float* s = ring[ringHead];
  s[0] = a.acceleration.x; s[1] = a.acceleration.y; s[2] = a.acceleration.z;
  s[3] = g.gyro.x * R2D;   s[4] = g.gyro.y * R2D;   s[5] = g.gyro.z * R2D;
  ringHead = (ringHead + 1) % CLF_WINDOW;
  if (ringCount < CLF_WINDOW) ringCount++;
#if STREAM_RAW
  Serial.printf("I,%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n", (unsigned long)now, s[0], s[1], s[2], s[3], s[4], s[5]);
#endif
  counterSample(s[CURL_AXIS], now);

  if (ringCount == CLF_WINDOW && ++sinceInfer >= CLF_HOP) {
    sinceInfer = 0;
    for (int i = 0; i < CLF_WINDOW; i++) {
      const float* src = ring[(ringHead + i) % CLF_WINDOW];
      for (int c = 0; c < 6; c++) window[i][c] = src[c];
    }
    uint32_t t0 = micros();
    clf_features(window, feats);
    ClfResult r = clf_infer(feats);
    lastInferUs = micros() - t0;
    lastLabel = r.label; lastConf = r.conf; uiDirty = true;
    Serial.printf("C,%lu,%s,%.2f,%lu\n", (unsigned long)now, CLF_CLASSES[r.label], r.conf, (unsigned long)lastInferUs);
  }
}

// ---------- heart rate ----------
long lastIr = 0; bool finger = false;
uint32_t beatMs[HR_N_INT + 1]; int beatCount = 0;   // ring of the last beat times
float bpm = 0; bool hrValid = false;

void hrReset() { beatCount = 0; bpm = 0; hrValid = false; }

float medianInterval() {
  int n = beatCount - 1; if (n < 1) return 0;
  float iv[HR_N_INT]; int m = 0;
  for (int i = 0; i < n; i++) {
    float d = (float)(beatMs[i + 1] - beatMs[i]);
    float b = 60000.0f / d;
    if (b >= 30 && b <= 220) iv[m++] = d;
  }
  if (m < HR_MIN_BEATS) return 0;
  for (int i = 1; i < m; i++) { float v = iv[i]; int j = i - 1; while (j >= 0 && iv[j] > v) { iv[j + 1] = iv[j]; j--; } iv[j + 1] = v; }
  return (m & 1) ? iv[m / 2] : 0.5f * (iv[m / 2 - 1] + iv[m / 2]);
}

void serviceHR() {
  if (!hrOk) return;
  hr.check();
  int budget = 8;                                 // bound the work per loop so the IMU tick is never late
  while (hr.available() && budget-- > 0) {
    long ir = hr.getFIFOIR();
    hr.nextSample();
    hrN++; lastIr = ir;
    bool f = ir > IR_FINGER;
    if (f != finger) { finger = f; if (!f) hrReset(); uiDirty = true; }
    if (f && checkForBeat(ir)) {
      uint32_t now = millis();
      if (beatCount == HR_N_INT + 1) { for (int i = 0; i < HR_N_INT; i++) beatMs[i] = beatMs[i + 1]; beatCount--; }
      beatMs[beatCount++] = now;
    }
  }
  float med = medianInterval();
  bool stale = beatCount == 0 || (millis() - beatMs[beatCount - 1]) > HR_STALE_MS;
  bool v = finger && med > 0 && !stale;
  if (v) bpm = 60000.0f / med;
  if (v != hrValid) { hrValid = v; uiDirty = true; }
}

// ---------- OLED transfer in chunks ----------
// Adafruit's display() pushes the whole 1 KB frame in one go: ~32 ms on the shared 400 kHz bus,
// which made 3 IMU samples late per refresh (measured 2026-11-06, S line imu_late). Instead the frame
// is sent 64 bytes per loop, and only when the next IMU tick is at least OLED_CHUNK_US away.
// The SSD1306 is in horizontal addressing mode (set by oled.begin), so after one address setup its
// write pointer walks the whole frame; other devices' traffic on the bus does not disturb it.
const int      OLED_FRAME_BYTES = 128 * 64 / 8;
const int      OLED_CHUNK       = 64;          // bytes per I2C transaction (ESP32 Wire buffer holds 128)
const uint32_t OLED_CHUNK_US    = 3000;        // one chunk takes ~1.5 ms at 400 kHz; margin on top
int oledPos = -1;                              // -1: no frame in flight; else next byte offset

void oledStartFrame() {                        // called by draw() after rendering into the buffer
  Wire.beginTransmission(oledAddr);
  Wire.write((uint8_t)0x00);                   // Co=0, D/C=0: command bytes follow
  Wire.write((uint8_t)0x22); Wire.write((uint8_t)0); Wire.write((uint8_t)7);     // page 0..7
  Wire.write((uint8_t)0x21); Wire.write((uint8_t)0); Wire.write((uint8_t)127);   // column 0..127
  Wire.endTransmission();
  oledPos = 0;
}

void serviceOLED() {
  if (!oledOk || oledPos < 0) return;
  if ((int32_t)(imuNext - micros()) < (int32_t)OLED_CHUNK_US) return;   // IMU tick is near: wait
  Wire.beginTransmission(oledAddr);
  Wire.write((uint8_t)0x40);                   // Co=0, D/C=1: data bytes follow
  Wire.write(oled.getBuffer() + oledPos, OLED_CHUNK);
  Wire.endTransmission();
  oledPos += OLED_CHUNK;
  if (oledPos >= OLED_FRAME_BYTES) oledPos = -1;
}

// ---------- display ----------
void draw() {
  if (!oledOk) return;
  uint32_t now = millis();
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  if (state == RUNNING) oled.print("RUN");
  else if (state == PAUSED) oled.print("PAUSE");
  else if ((int32_t)(doneUntilMs - now) > 0) oled.print("DONE");
  else oled.print("IDLE");
  oled.setCursor(76, 0);
  if (!hrOk) oled.print("HR n/a");
  else if (hrValid) oled.printf("HR %3d", (int)(bpm + 0.5f));
  else oled.print(finger ? "HR ..." : "HR --");

  oled.setTextSize(4);
  oled.setCursor(8, 16);
  oled.printf("%2d", reps);
  oled.setTextSize(2);
  oled.setCursor(72, 30);
  oled.printf("/%d", TARGET);

  oled.setTextSize(1);
  oled.setCursor(0, 56);
  if (state == IDLE) oled.print("press=start hold=end");
  else {
    if (lastLabel >= 0) oled.printf("%-5s %.2f", CLF_CLASSES[lastLabel], lastConf);
    else oled.print("...");
    oled.setCursor(92, 56);
    oled.printf("%3lus", (unsigned long)((now - sessionStartMs) / 1000));
  }
  oled.invertDisplay(state != IDLE && reps >= TARGET);   // FR-10: unmistakable at target, keeps counting
  oledStartFrame();
}

// ---------- session ----------
void onShort() {
  uint32_t t = millis();
  if (state == IDLE) {
    state = RUNNING; reps = 0; sessionStartMs = t; repPhase = 0; doneUntilMs = 0;
    Serial.printf("EVENT,%lu,start\n", (unsigned long)t);
  } else if (state == RUNNING) {
    state = PAUSED; Serial.printf("EVENT,%lu,pause,%d\n", (unsigned long)t, reps);
  } else {
    state = RUNNING; Serial.printf("EVENT,%lu,resume,%d\n", (unsigned long)t, reps);
  }
  cue(40); uiDirty = true;
}

void onLong() {
  uint32_t t = millis();
  if (state == IDLE) return;
  Serial.printf("EVENT,%lu,end,reps=%d,duration_s=%lu\n", (unsigned long)t, reps, (unsigned long)((t - sessionStartMs) / 1000));
  state = IDLE; doneUntilMs = t + 3000;
  cue(150); uiDirty = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT); pinMode(PIN_BUZZ, OUTPUT);
  digitalWrite(PIN_LED, LOW); digitalWrite(PIN_BUZZ, LOW);
  Wire.begin(); Wire.setClock(400000);

  for (int j = 0; j < CLF_N_CLASSES; j++) if (strcmp(CLF_CLASSES[j], "curl") == 0) curlIdx = j;

  for (uint8_t a : {0x3C, 0x3D}) { Wire.beginTransmission(a); if (Wire.endTransmission() == 0) { oledAddr = a; break; } }
  oledOk = oledAddr && oled.begin(SSD1306_SWITCHCAPVCC, oledAddr);
  if (!oledOk) Serial.println("WARN: no SSD1306 at 0x3C/0x3D; running without display");

  if (!sox.begin_I2C()) {
    Serial.println("ERROR: LSM6DSOX not found; run 03_i2c_scan");
    if (oledOk) { oled.clearDisplay(); oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE); oled.setCursor(0, 20); oled.print("NO IMU"); oled.display(); }
    while (true) { digitalWrite(PIN_LED, !digitalRead(PIN_LED)); delay(200); }
  }
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
  sox.setAccelDataRate(LSM6DS_RATE_104_HZ);
  sox.setGyroDataRate(LSM6DS_RATE_104_HZ);

  hrOk = hr.begin(Wire, I2C_SPEED_FAST);
  if (hrOk) {
    hr.setup(0x1F, 4, 2, 400, 411, 4096);        // ~6 mA, avg 4, red+IR, 400 sps -> 100 Hz (bring-up 05)
    hr.setPulseAmplitudeGreen(0);
  } else Serial.println("WARN: MAX30102 not found; heart rate disabled");

  Serial.printf("# app v0: button D2 short=start/pause/resume long=end; target %d; classifier %d feats window %d hop %d; gate %d; axis col %d; buzzer %d; oled %d; hr %d\n",
                TARGET, CLF_N_FEATURES, CLF_WINDOW, CLF_HOP, COUNT_GATE, CURL_AXIS, USE_BUZZER, oledOk, hrOk);
  Serial.println("# EVENT,t,what | C,t,label,conf,us | R,t,reps,rep_ms | H,t,bpm,valid,ir | S,t,imu_n,imu_hz,imu_dt_max_ms,imu_late,hr_n,loop_max_us");
  draw(); cue(60);
  imuNext = micros(); imuLastMs = millis();
  statNext = millis() + 5000;
}

void loop() {
  uint32_t t0 = micros();
  serviceIMU();
  serviceHR();
  BtnEvent e = pollButton();
  if (e == BTN_SHORT) onShort(); else if (e == BTN_LONG) onLong();
  serviceCue();

  static uint32_t lastDraw = 0, lastH = 0;
  uint32_t now = millis();
  if ((uiDirty || now - lastDraw >= 250) && oledPos < 0) { draw(); lastDraw = now; uiDirty = false; }
  serviceOLED();
  uint32_t el = micros() - t0;               // includes draw and the OLED chunk (it did not before)
  if (el > loopMaxUs) loopMaxUs = el;
  if (hrOk && now - lastH >= 1000) {
    lastH = now;
    Serial.printf("H,%lu,%.0f,%d,%ld\n", (unsigned long)now, hrValid ? bpm : 0.0f, hrValid ? 1 : 0, lastIr);
  }
  if ((int32_t)(now - statNext) >= 0) {
    Serial.printf("S,%lu,%lu,%.1f,%lu,%lu,%lu,%lu\n", (unsigned long)now, (unsigned long)imuN, imuN / 5.0f,
                  (unsigned long)imuDtMax, (unsigned long)imuLate, (unsigned long)hrN, (unsigned long)loopMaxUs);
    imuN = 0; imuDtMax = 0; hrN = 0; loopMaxUs = 0;
    statNext += 5000;
  }
}
