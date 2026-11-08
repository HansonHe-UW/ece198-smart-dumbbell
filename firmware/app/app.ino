// app v1 (2026-11-08): integrated smart-dumbbell firmware.
//   LSM6DSOX 104 Hz -> 2 s ring buffer -> curl/idle/other classifier every 1 s (classifier.h)
//   -> rep counter v1: angle integration of the handle gyro axis with a minimum range of motion,
//      so a cable tug or a shake (large rate, small angle) does not count (FR-05, FR-06)
//   MAX30102 100 Hz -> band-pass -> adaptive peak detector -> interval consistency -> bpm + validity (FR-07, FR-08)
//   SSD1306 OLED (frame sent in 64-byte chunks between IMU ticks), one pushbutton, LED, optional buzzer
//   Wi-Fi access point + HTTP  GET /status  POST /target   (FR-01, FR-11, FR-12; matches software/data_logger.py)
//   BLE Nordic UART service: status JSON line every second on TX, commands on RX    (same JSON as /status)
//
// ONE BUTTON on D2:
//   short press  (< 500 ms)   IDLE -> RUNNING,  RUNNING -> PAUSED,  PAUSED -> RUNNING
//   long press   (>= 1.5 s)   RUNNING or PAUSED -> IDLE (session ends); in IDLE: nothing
//
// Status JSON (one object per second, identical on /status, BLE TX and the serial J line):
//   {"session_id":"S3-2","sequence":17,"elapsed_ms":16400,"session_state":"running","target_reps":10,
//    "rep_count":4,"motion_class":"curl","motion_confidence":0.91,"heart_rate_bpm":72,"heart_rate_valid":true,
//    "signal_quality":"good","connection_state":"ble"}
//   heart_rate_bpm is null when heart_rate_valid is false. signal_quality: good | insufficient | no_finger | unstable | n/a
//   connection_state: none | wifi | ble | wifi+ble
// Commands (BLE RX, one per line; HTTP uses POST /target with {"target_reps":N}):
//   T=<n>  set target reps (1..10000)      START / PAUSE / END  same as the button
//
// Serial 115200, one record per line (t_ms = millis()):
//   EVENT,<t>,<what>[,<detail>]   C,<t>,<label>,<conf>,<us>   R,<t>,<reps>,<rep_ms>,<rom_deg>,<tilt_deg>
//   H,<t>,<bpm>,<valid>,<quality>,<ir>,<ac>     J,<json>     S,<t>,imu_n,imu_hz,imu_dt_max_ms,imu_late,hr_n,loop_max_us,wifi_sta,ble
//   I,<t>,ax,ay,az,gx,gy,gz  every IMU sample when STREAM_RAW is 1
//
// Libraries: Adafruit LSM6DS, Adafruit SSD1306 (+GFX, BusIO, Unified Sensor), SparkFun MAX3010x,
// WiFi / WebServer / BLE from the arduino:esp32 core (2.0.18). classifier.h / model_weights.h: copies of firmware/classifier/.

#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "classifier.h"

// ---------------- configuration ----------------
#define STREAM_RAW        0    // 1: print every IMU sample as I,... (floods the port; for recording only)
#define USE_BUZZER        0    // 1: buzzer on PIN_BUZZ
#define BUZZER_ACTIVE_LOW 1    // 1: DAOKAI low-level-trigger module (S8550): LOW = sound. 0: Grove buzzer, HIGH = sound
#define COUNT_GATE        0    // 1: a rep counts only while the last classifier label is "curl" (wait for a real model)
#define TILT_CHECK        1    // 1: a rep also needs the gravity tilt to change by TILT_MIN_DEG (rejects spins and tugs)
#define CURL_AXIS         3    // ring column of the gyro axis along the handle: 3=gx 4=gy 5=gz (X along the handle -> 3)
#define USE_WIFI          1    // soft-AP + HTTP server
#define USE_BLE           1    // Nordic UART service

const int PIN_BTN = D2, PIN_BUZZ = D5, PIN_LED = D7;
int targetReps = 10;                           // FR-01 demo default; set from the computer via /target or T=

const char* AP_SSID = "SmartDumbbell-57";
const char* AP_PASS = "ece198test";
const char* BLE_NAME = "SmartDumbbell-57";

const uint32_t DEBOUNCE_MS = 30, SHORT_MAX_MS = 500, LONG_MIN_MS = 1500;

// rep counter v1 (angle based). A curl moves the forearm through 60..130 deg; a cable tug or a shake moves it a few deg.
const float    ROM_MIN_DEG    = 45.0f;        // peak |angle| needed before a rep can count
const float    ROM_RETURN_DEG = 15.0f;        // angle must come back under this to close the rep
const float    TILT_MIN_DEG   = 25.0f;        // gravity tilt range needed inside the rep (TILT_CHECK)
const uint32_t REP_MIN_MS     = 400;          // time above ROM_RETURN_DEG; a 0.8 s curl spends ~0.55 s there, a tug ~0.15 s
const uint32_t REP_MAX_MS     = 5000;         // slower than this is a stall; restart tracking
const float    ANGLE_LEAK_S   = 10.0f;        // leaky integrator time constant while moving (drift control)
const float    ANGLE_REZERO_S = 0.5f;         // and a fast pull to zero while still, so each rep starts from the rest position
const float    GYRO_LPF       = 0.3f;         // first-order low-pass on the gyro axis
const float    BIAS_STILL_DPS = 4.0f;         // |rate| under this for BIAS_STILL_MS -> update gyro bias
const uint32_t BIAS_STILL_MS  = 800;

// heart rate v1
const long     IR_FINGER      = 50000;        // from bring-up 05: IR > 50 000 with a finger on the window
const float    HR_FS          = 100.0f;       // MAX30102 400 sps / avg 4
const float    HR_AC_MIN      = 0.0003f;      // positive-lobe amplitude / DC under this -> "insufficient" (bring-up 05: peak-to-peak 0.0013..0.0035 at rest)
const uint32_t HR_REFRACT_MS  = 300;          // min beat spacing (200 bpm)
const uint32_t HR_STALE_MS    = 3000;         // no beat for this long -> invalid (FR-08, AT-06)
const int      HR_N_INT       = 8;            // intervals kept
const int      HR_MIN_INT     = 5;            // consistent intervals needed before a value is shown
const float    HR_MAX_SPREAD  = 0.20f;        // median absolute deviation / median must be under this
// --------------------------------------------------

#if USE_WIFI
#include <WiFi.h>
#include <WebServer.h>
WebServer http(80);
#endif
#if USE_BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#define NUS_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
BLECharacteristic* bleTx = nullptr;
volatile bool bleConnected = false;
String bleRxBuf;
#endif

Adafruit_LSM6DSOX sox;
MAX30105 hr;
Adafruit_SSD1306 oled(128, 64, &Wire, -1, 400000UL, 400000UL);   // clkAfter 400 kHz too, see v0 note / README
bool oledOk = false, hrOk = false;
uint8_t oledAddr = 0;

enum State { IDLE, RUNNING, PAUSED };
State state = IDLE;
int reps = 0;
uint32_t sessionStartMs = 0, doneUntilMs = 0, lastRepMs = 0;
int sessionNo = 0; uint32_t bootId = 0; uint32_t statusSeq = 0;
bool uiDirty = true;

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
inline void buzzerWrite(bool on) {
#if USE_BUZZER
  digitalWrite(PIN_BUZZ, (on ^ (BUZZER_ACTIVE_LOW != 0)) ? HIGH : LOW);
#else
  digitalWrite(PIN_BUZZ, BUZZER_ACTIVE_LOW ? HIGH : LOW);   // keep an unused low-trigger module silent
#endif
}
void cue(uint32_t ms) { digitalWrite(PIN_LED, HIGH); buzzerWrite(true); cueUntil = millis() + ms; }
void serviceCue() {
  if (cueUntil && (int32_t)(millis() - cueUntil) >= 0) { digitalWrite(PIN_LED, LOW); buzzerWrite(false); cueUntil = 0; }
}

// ---------- IMU + classifier ----------
const uint32_t IMU_PERIOD_US = 1000000UL / CLF_FS;
uint32_t imuNext = 0, imuN = 0, imuLate = 0, imuLastMs = 0, imuDtMax = 0, loopMaxUs = 0, hrN = 0, statNext = 0;
static float ring[CLF_WINDOW][6];
static float window[CLF_WINDOW][6];
static float feats[CLF_N_FEATURES];
int ringHead = 0, ringCount = 0, sinceInfer = 0;
int lastLabel = -1; float lastConf = 0; uint32_t lastInferUs = 0;
int curlIdx = 0;
float gyroMag = 0;                              // |gyro| low-passed, used by the heart-rate quality flag

// ---------- rep counter v1 ----------
float wFilt = 0, gyroBias = 0, angle = 0, tiltLP = 0;
bool tiltInit = false;
uint32_t stillSince = 0, lastSampleMs = 0;
int repPhase = 0; float repSign = 0, repPeak = 0, tiltMin = 0, tiltMax = 0; uint32_t repT0 = 0;

void countRep(uint32_t t, uint32_t dur, float rom, float tilt) {
  reps++; lastRepMs = t; uiDirty = true;
  Serial.printf("R,%lu,%d,%lu,%.0f,%.0f\n", (unsigned long)t, reps, (unsigned long)dur, rom, tilt);
  if (reps == targetReps) { cue(400); Serial.printf("EVENT,%lu,target_reached\n", (unsigned long)t); }
  else cue(60);
}

void counterSample(const float* s, uint32_t t) {
  float w = s[CURL_AXIS];
  float dt = lastSampleMs ? (t - lastSampleMs) * 0.001f : 1.0f / CLF_FS; lastSampleMs = t;
  if (dt > 0.05f) dt = 0.05f;
  wFilt += GYRO_LPF * (w - wFilt);
  float mag = sqrtf(s[3] * s[3] + s[4] * s[4] + s[5] * s[5]);
  gyroMag += 0.1f * (mag - gyroMag);

  // gyro bias: track slowly while the handle axis is still for a while
  if (fabsf(wFilt - gyroBias) < BIAS_STILL_DPS) { if (!stillSince) stillSince = t; if (t - stillSince > BIAS_STILL_MS) gyroBias += 0.01f * (wFilt - gyroBias); }
  else stillSince = 0;

  // leaky integration -> forearm angle relative to the recent rest position
  angle += (wFilt - gyroBias) * dt;
  angle -= angle * (dt / ANGLE_LEAK_S);
  if (repPhase == 0 && fabsf(wFilt - gyroBias) < BIAS_STILL_DPS) angle -= angle * (dt / ANGLE_REZERO_S);

  // gravity tilt about the handle axis, from the two accelerometer axes perpendicular to it
  int p1 = (CURL_AXIS - 3 + 1) % 3, p2 = (CURL_AXIS - 3 + 2) % 3;
  float tilt = atan2f(s[p1], s[p2]) * 57.2957795f;
  if (!tiltInit) { tiltLP = tilt; tiltInit = true; }
  float d = tilt - tiltLP; while (d > 180) d -= 360; while (d < -180) d += 360;
  tiltLP += 0.15f * d;

  bool allow = (state == RUNNING);
#if COUNT_GATE
  allow = allow && (lastLabel == curlIdx);
#endif
  switch (repPhase) {
    case 0:                                                  // at rest, waiting for the forearm to leave
      if (fabsf(angle) > ROM_RETURN_DEG) { repPhase = 1; repSign = angle > 0 ? 1 : -1; repT0 = t; repPeak = fabsf(angle); tiltMin = tiltMax = tiltLP; }
      break;
    case 1: {                                                // excursion: track the peak and the tilt range
      float a = angle * repSign;
      if (a > repPeak) repPeak = a;
      if (tiltLP < tiltMin) tiltMin = tiltLP; if (tiltLP > tiltMax) tiltMax = tiltLP;
      if (a < ROM_RETURN_DEG) {                              // back near the start
        uint32_t dur = t - repT0; float tiltRange = tiltMax - tiltMin; repPhase = 0;
        bool ok = repPeak >= ROM_MIN_DEG && dur >= REP_MIN_MS && dur <= REP_MAX_MS;
#if TILT_CHECK
        ok = ok && tiltRange >= TILT_MIN_DEG;
#endif
        if (ok && allow) countRep(t, dur, repPeak, tiltRange);
      } else if (t - repT0 > REP_MAX_MS) { repPhase = 0; angle = 0; }   // stalled mid-air: re-zero and wait
      break; }
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
  counterSample(s, now);

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

// ---------- heart rate v1 ----------
// IR -> DC (slow EMA) -> 2-pole low-pass (~4 Hz) -> 2-pole high-pass (~0.6 Hz) = band-passed pulse.
// Beat = upward zero crossing of that signal whose positive lobe then peaks above HR_LOBE_MIN of the tracked lobe
// envelope; refractory = max(300 ms, 0.6 x current median interval), which swallows the dicrotic notch.
// bpm = 60000 / median of the last HR_N_INT intervals, shown only when HR_MIN_INT of them agree within HR_MAX_SPREAD.
// Chosen on synthetic PPG (firmware/app/tools/algo_sim.py): 45..150 bpm covered 80..92 %, MAE 1..3 bpm; strong motion
// at a non-cardiac frequency still captures the detector (that needs IMU-based rejection, not done).
const float HR_LOBE_MIN = 0.25f;
long lastIr = 0; bool finger = false;
float hrDc = 0, hrLp1 = 0, hrLp2 = 0, hrHp1 = 0, hrHp2 = 0, hrPrev = 0, hrEnv = 0, hrLobeMax = 0;
uint32_t hrLobeT0 = 0; bool hrInit = false, hrLobe = false;
uint32_t lastBeatMs = 0; uint32_t beatIv[HR_N_INT]; int beatIvN = 0, beatIvHead = 0;
float bpm = 0, bpmSmooth = 0; bool hrValid = false; const char* hrQuality = "no_finger";
float acRatio = 0;

void hrReset() { beatIvN = 0; beatIvHead = 0; lastBeatMs = 0; bpm = 0; bpmSmooth = 0; hrValid = false; hrInit = false; hrEnv = 0; hrLobe = false; }

static void sortf(float* v, int n) { for (int i = 1; i < n; i++) { float x = v[i]; int j = i - 1; while (j >= 0 && v[j] > x) { v[j + 1] = v[j]; j--; } v[j + 1] = x; } }

static float medianIv() {
  float iv[HR_N_INT]; for (int i = 0; i < beatIvN; i++) iv[i] = beatIv[i];
  sortf(iv, beatIvN);
  return (beatIvN & 1) ? iv[beatIvN / 2] : 0.5f * (iv[beatIvN / 2 - 1] + iv[beatIvN / 2]);
}

void hrBeat(uint32_t t) {
  uint32_t refr = HR_REFRACT_MS;
  if (beatIvN >= 3) { float r = 0.6f * medianIv(); if (r > refr) refr = (uint32_t)r; }
  if (lastBeatMs && t - lastBeatMs < refr) return;
  if (lastBeatMs) {
    uint32_t iv = t - lastBeatMs;
    if (iv >= 300 && iv <= 2000) {                 // 30..200 bpm
      beatIv[beatIvHead] = iv; beatIvHead = (beatIvHead + 1) % HR_N_INT; if (beatIvN < HR_N_INT) beatIvN++;
    } else { beatIvN = 0; beatIvHead = 0; }        // gap or glitch: start the window again
  }
  lastBeatMs = t;
}

void hrSample(long ir, uint32_t now) {
  if (!hrInit) { hrDc = ir; hrLp1 = hrLp2 = hrHp1 = hrHp2 = 0; hrPrev = 0; hrInit = true; return; }
  hrDc += 0.02f * (ir - hrDc);                     // ~0.3 Hz DC tracker
  float ac = ir - hrDc;
  hrLp1 += 0.25f * (ac - hrLp1); hrLp2 += 0.25f * (hrLp1 - hrLp2);     // 2 x ~4.6 Hz
  hrHp1 += 0.04f * (hrLp2 - hrHp1); float y = hrLp2 - hrHp1;
  hrHp2 += 0.04f * (y - hrHp2);     float x = y - hrHp2;                // 2 x ~0.65 Hz
  if (x > 0) {
    if (hrPrev <= 0) { hrLobe = true; hrLobeT0 = now; hrLobeMax = 0; }  // upward crossing: candidate beat
    if (x > hrLobeMax) hrLobeMax = x;
  } else if (hrPrev > 0 && hrLobe) {               // lobe finished: decide
    hrEnv += (hrLobeMax > hrEnv ? 0.2f : 0.03f) * (hrLobeMax - hrEnv);
    acRatio = hrDc > 1 ? hrEnv / hrDc : 0;
    if (hrLobeMax > HR_LOBE_MIN * hrEnv) hrBeat(hrLobeT0);
    hrLobe = false;
  }
  hrPrev = x;
}

void hrEvaluate(uint32_t now) {
  const char* q; bool v = false;
  if (!hrOk) q = "n/a";
  else if (!finger) q = "no_finger";
  else if (acRatio < HR_AC_MIN) q = "insufficient";
  else if (beatIvN < HR_MIN_INT || !lastBeatMs || now - lastBeatMs > HR_STALE_MS) q = "insufficient";
  else {
    float med = medianIv();
    float dev[HR_N_INT]; for (int i = 0; i < beatIvN; i++) dev[i] = fabsf((float)beatIv[i] - med);
    sortf(dev, beatIvN);
    float mad = (beatIvN & 1) ? dev[beatIvN / 2] : 0.5f * (dev[beatIvN / 2 - 1] + dev[beatIvN / 2]);
    if (mad / med <= HR_MAX_SPREAD) { v = true; bpm = 60000.0f / med; bpmSmooth = bpmSmooth > 0 ? bpmSmooth + 0.3f * (bpm - bpmSmooth) : bpm; q = gyroMag > 80 ? "good_moving" : "good"; }
    else q = "unstable";
  }
  if (!v) bpmSmooth = 0;
  if (v != hrValid || q != hrQuality) uiDirty = true;
  hrValid = v; hrQuality = q;
}

void serviceHR() {
  if (!hrOk) return;
  hr.check();
  int budget = 8;                                   // bound the work per loop so the IMU tick is never late
  while (hr.available() && budget-- > 0) {
    long ir = hr.getFIFOIR();
    hr.nextSample();
    hrN++; lastIr = ir;
    bool f = ir > IR_FINGER;
    if (f != finger) { finger = f; if (!f) hrReset(); uiDirty = true; }
    if (f) hrSample(ir, millis());
  }
  hrEvaluate(millis());
}

// ---------- status JSON (shared by /status, BLE TX and the serial J line) ----------
const char* stateName() { return state == RUNNING ? "running" : state == PAUSED ? "paused" : "idle"; }
const char* linkName() {
  bool w = false, b = false;
#if USE_WIFI
  w = WiFi.softAPgetStationNum() > 0;
#endif
#if USE_BLE
  b = bleConnected;
#endif
  return w && b ? "wifi+ble" : w ? "wifi" : b ? "ble" : "none";
}
char statusBuf[320];
const char* buildStatus(bool newSeq) {
  if (newSeq) statusSeq++;
  uint32_t now = millis();
  char bpmTxt[8]; if (hrValid) snprintf(bpmTxt, sizeof bpmTxt, "%d", (int)(bpmSmooth + 0.5f)); else strcpy(bpmTxt, "null");
  snprintf(statusBuf, sizeof statusBuf,
    "{\"session_id\":\"S%lu-%d\",\"sequence\":%lu,\"elapsed_ms\":%lu,\"session_state\":\"%s\",\"target_reps\":%d,"
    "\"rep_count\":%d,\"motion_class\":\"%s\",\"motion_confidence\":%.2f,\"heart_rate_bpm\":%s,\"heart_rate_valid\":%s,"
    "\"signal_quality\":\"%s\",\"connection_state\":\"%s\"}",
    (unsigned long)bootId, sessionNo, (unsigned long)statusSeq, (unsigned long)(state == IDLE ? 0 : now - sessionStartMs),
    stateName(), targetReps, reps, lastLabel >= 0 ? CLF_CLASSES[lastLabel] : "none", lastConf, bpmTxt,
    hrValid ? "true" : "false", hrQuality, linkName());
  return statusBuf;
}

// ---------- session control (button, BLE command, HTTP) ----------
void doStart(const char* src) {
  if (state != IDLE) return;
  uint32_t t = millis();
  state = RUNNING; reps = 0; sessionStartMs = t; sessionNo++; repPhase = 0; angle = 0; doneUntilMs = 0;
  Serial.printf("EVENT,%lu,start,%s\n", (unsigned long)t, src); cue(40); uiDirty = true;
}
void doPauseResume(const char* src) {
  uint32_t t = millis();
  if (state == RUNNING) { state = PAUSED; Serial.printf("EVENT,%lu,pause,%d,%s\n", (unsigned long)t, reps, src); }
  else if (state == PAUSED) { state = RUNNING; Serial.printf("EVENT,%lu,resume,%d,%s\n", (unsigned long)t, reps, src); }
  else return;
  cue(40); uiDirty = true;
}
void doEnd(const char* src) {
  if (state == IDLE) return;
  uint32_t t = millis();
  Serial.printf("EVENT,%lu,end,reps=%d,duration_s=%lu,%s\n", (unsigned long)t, reps, (unsigned long)((t - sessionStartMs) / 1000), src);
  state = IDLE; doneUntilMs = t + 3000; cue(150); uiDirty = true;
}
bool setTarget(long n, const char* src) {
  if (n < 1 || n > 10000) return false;
  targetReps = (int)n; uiDirty = true;
  Serial.printf("EVENT,%lu,target,%d,%s\n", (unsigned long)millis(), targetReps, src);
  return true;
}
void onShort() { if (state == IDLE) doStart("button"); else doPauseResume("button"); }
void onLong()  { doEnd("button"); }

// one text command, used by BLE RX (and handy over serial for tests)
void handleCommand(String cmd, const char* src) {
  cmd.trim(); if (!cmd.length()) return;
  if (cmd.startsWith("T=")) { if (!setTarget(cmd.substring(2).toInt(), src)) Serial.printf("EVENT,%lu,bad_target,%s\n", (unsigned long)millis(), src); }
  else if (cmd.equalsIgnoreCase("START")) doStart(src);
  else if (cmd.equalsIgnoreCase("PAUSE") || cmd.equalsIgnoreCase("RESUME")) { if (state == IDLE) doStart(src); else doPauseResume(src); }
  else if (cmd.equalsIgnoreCase("END")) doEnd(src);
  else Serial.printf("EVENT,%lu,unknown_cmd,%s\n", (unsigned long)millis(), src);
}

// ---------- Wi-Fi: soft-AP + HTTP ----------
#if USE_WIFI
void httpStatus() { http.sendHeader("Cache-Control", "no-store"); http.send(200, "application/json", buildStatus(true)); }
void httpTarget() {
  long n = -1;
  if (http.hasArg("plain")) {                       // JSON body {"target_reps": 12}
    String body = http.arg("plain"); int k = body.indexOf("target_reps");
    if (k >= 0) { int c = body.indexOf(':', k); if (c > 0) n = body.substring(c + 1).toInt(); }
  }
  if (http.hasArg("n")) n = http.arg("n").toInt();  // or /target?n=12
  if (setTarget(n, "http")) http.send(200, "application/json", String("{\"ok\":true,\"target_reps\":") + targetReps + "}");
  else http.send(400, "application/json", "{\"error\":\"target_reps must be an integer between 1 and 10000\"}");
}
void httpRoot() {
  http.send(200, "text/plain", String("Smart dumbbell app v1\nGET /status  POST /target {\"target_reps\":N}\nstations ") + WiFi.softAPgetStationNum() + " uptime_ms " + millis() + "\n");
}
void wifiSetup() {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  http.on("/", httpRoot); http.on("/status", HTTP_GET, httpStatus); http.on("/target", HTTP_POST, httpTarget); http.on("/target", HTTP_GET, httpTarget);
  http.onNotFound([]() { http.send(404, "text/plain", "not found"); });
  http.begin();
  Serial.printf("# wifi softAP %s -> %s, ip %s\n", AP_SSID, ok ? "up" : "FAILED", WiFi.softAPIP().toString().c_str());
}
#endif

// ---------- BLE: Nordic UART service ----------
#if USE_BLE
class ServerCb : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { bleConnected = true; uiDirty = true; }
  void onDisconnect(BLEServer* s) override { bleConnected = false; uiDirty = true; s->getAdvertising()->start(); }
};
class RxCb : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    std::string v = c->getValue();
    for (char ch : v) { if (ch == '\n' || ch == '\r') { handleCommand(bleRxBuf, "ble"); bleRxBuf = ""; } else if (bleRxBuf.length() < 64) bleRxBuf += ch; }
    if (bleRxBuf.length() && v.size() < 20) { handleCommand(bleRxBuf, "ble"); bleRxBuf = ""; }   // short write without newline
  }
};
void bleSetup() {
  BLEDevice::init(BLE_NAME);
  BLEDevice::setMTU(247);
  BLEServer* srv = BLEDevice::createServer();
  srv->setCallbacks(new ServerCb());
  BLEService* svc = srv->createService(NUS_SERVICE);
  bleTx = svc->createCharacteristic(NUS_TX, BLECharacteristic::PROPERTY_NOTIFY);
  bleTx->addDescriptor(new BLE2902());
  BLECharacteristic* rx = svc->createCharacteristic(NUS_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(new RxCb());
  svc->start();
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE); adv->setScanResponse(true); adv->setMinPreferred(0x06); adv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.printf("# ble advertising as %s (NUS)\n", BLE_NAME);
}
void bleSendLine(const char* line) {                // chunked, newline terminated; receivers reassemble on '\n'
  if (!bleConnected || !bleTx) return;
  size_t n = strlen(line), pos = 0; const size_t CH = 180;
  while (pos < n) { size_t k = n - pos < CH ? n - pos : CH; bleTx->setValue((uint8_t*)line + pos, k); bleTx->notify(); pos += k; }
  bleTx->setValue((uint8_t*)"\n", 1); bleTx->notify();
}
#endif

// ---------- OLED transfer in chunks (see v0 note: full-frame display() delayed IMU samples) ----------
const int      OLED_FRAME_BYTES = 128 * 64 / 8;
const int      OLED_CHUNK       = 64;
const uint32_t OLED_CHUNK_US    = 3000;
int oledPos = -1;

void oledStartFrame() {
  Wire.beginTransmission(oledAddr);
  Wire.write((uint8_t)0x00);
  Wire.write((uint8_t)0x22); Wire.write((uint8_t)0); Wire.write((uint8_t)7);
  Wire.write((uint8_t)0x21); Wire.write((uint8_t)0); Wire.write((uint8_t)127);
  Wire.endTransmission();
  oledPos = 0;
}
void serviceOLED() {
  if (!oledOk || oledPos < 0) return;
  if ((int32_t)(imuNext - micros()) < (int32_t)OLED_CHUNK_US) return;
  Wire.beginTransmission(oledAddr);
  Wire.write((uint8_t)0x40);
  Wire.write(oled.getBuffer() + oledPos, OLED_CHUNK);
  Wire.endTransmission();
  oledPos += OLED_CHUNK;
  if (oledPos >= OLED_FRAME_BYTES) oledPos = -1;
}

// ---------- display (FR-09: reps/target, state, heart rate + validity, link state) ----------
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
  const char* lk = linkName();                      // link flags at column 7
  oled.setCursor(42, 0);
  oled.print(strstr(lk, "wifi") ? "W" : " ");
  oled.print(strstr(lk, "ble") ? "B" : " ");
  oled.setCursor(76, 0);
  if (!hrOk) oled.print("HR n/a");
  else if (hrValid) oled.printf("HR %3d", (int)(bpmSmooth + 0.5f));
  else oled.print(finger ? "HR ..." : "HR --");

  oled.setTextSize(4);
  oled.setCursor(8, 16);
  oled.printf("%2d", reps);
  oled.setTextSize(2);
  oled.setCursor(72, 30);
  oled.printf("/%d", targetReps);

  oled.setTextSize(1);
  oled.setCursor(0, 56);
  if (state == IDLE) oled.print("press=start hold=end");
  else {
    if (lastLabel >= 0) oled.printf("%-5s %.2f", CLF_CLASSES[lastLabel], lastConf);
    else oled.print("...");
    oled.setCursor(92, 56);
    oled.printf("%3lus", (unsigned long)((now - sessionStartMs) / 1000));
  }
  oled.invertDisplay(state != IDLE && reps >= targetReps);   // FR-10
  oledStartFrame();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZ, OUTPUT); buzzerWrite(false);    // first thing: a low-trigger buzzer must not sing at boot
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT); digitalWrite(PIN_LED, LOW);
  while (!Serial && millis() < 3000) {}
  Wire.begin(); Wire.setClock(400000);
  bootId = (uint32_t)(esp_random() % 1000);

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
  if (hrOk) { hr.setup(0x1F, 4, 2, 400, 411, 4096); hr.setPulseAmplitudeGreen(0); }   // ~6 mA, avg 4, red+IR, 400 sps -> 100 Hz
  else Serial.println("WARN: MAX30102 not found; heart rate disabled");

#if USE_WIFI
  wifiSetup();
#endif
#if USE_BLE
  bleSetup();
#endif

  Serial.printf("# app v1: button D2; target %d; classifier %d feats window %d hop %d; gate %d; tilt %d; axis col %d; buzzer %d (active_low %d); wifi %d; ble %d; oled %d; hr %d\n",
                targetReps, CLF_N_FEATURES, CLF_WINDOW, CLF_HOP, COUNT_GATE, TILT_CHECK, CURL_AXIS, USE_BUZZER, BUZZER_ACTIVE_LOW, USE_WIFI, USE_BLE, oledOk, hrOk);
  Serial.println("# EVENT,t,what | C,t,label,conf,us | R,t,reps,rep_ms,rom,tilt | H,t,bpm,valid,quality,ir,ac | J,json | S,t,imu_n,imu_hz,imu_dt_max_ms,imu_late,hr_n,loop_max_us,wifi_sta,ble");
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
#if USE_WIFI
  http.handleClient();
#endif
  while (Serial.available()) { static String sbuf; char c = Serial.read(); if (c == '\n' || c == '\r') { handleCommand(sbuf, "serial"); sbuf = ""; } else if (sbuf.length() < 64) sbuf += c; }

  static uint32_t lastDraw = 0, lastStatus = 0;
  uint32_t now = millis();
  if ((uiDirty || now - lastDraw >= 250) && oledPos < 0) { draw(); lastDraw = now; uiDirty = false; }
  serviceOLED();
  uint32_t el = micros() - t0;
  if (el > loopMaxUs) loopMaxUs = el;

  if (now - lastStatus >= 1000) {
    lastStatus = now;
    if (hrOk) Serial.printf("H,%lu,%.0f,%d,%s,%ld,%.4f\n", (unsigned long)now, hrValid ? bpmSmooth : 0.0f, hrValid ? 1 : 0, hrQuality, lastIr, acRatio);
    const char* js = buildStatus(true);
    Serial.printf("J,%s\n", js);
#if USE_BLE
    bleSendLine(js);
#endif
  }
  if ((int32_t)(now - statNext) >= 0) {
    int sta = 0, ble = 0;
#if USE_WIFI
    sta = WiFi.softAPgetStationNum();
#endif
#if USE_BLE
    ble = bleConnected ? 1 : 0;
#endif
    Serial.printf("S,%lu,%lu,%.1f,%lu,%lu,%lu,%lu,%d,%d\n", (unsigned long)now, (unsigned long)imuN, imuN / 5.0f,
                  (unsigned long)imuDtMax, (unsigned long)imuLate, (unsigned long)hrN, (unsigned long)loopMaxUs, sta, ble);
    imuN = 0; imuDtMax = 0; hrN = 0; loopMaxUs = 0;
    statNext += 5000;
  }
}
