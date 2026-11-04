// classifier: runs the exported curl/idle/other model on the Nano ESP32.
// On boot: self-test against selftest.h (features and probabilities computed in Python
// for one recorded window) and timing of one inference. Then: LSM6DSOX at 104 Hz into
// a ring buffer, one classification per second over the last 2 s, printed as
//   C,<t_ms>,<label>,<conf>,<inference_us>
// which is the interface in ml/workplan.md section 4. Also prints the raw stream as
//   t_ms,ax,ay,az,gx,gy,gz
// so record_serial.py keeps working (set STREAM_RAW 0 to silence it).
// Libraries: Adafruit LSM6DS. Replace model_weights.h / selftest.h with real training output.

#include <Adafruit_LSM6DSOX.h>
#include "classifier.h"
#include "selftest.h"

#define STREAM_RAW 1

Adafruit_LSM6DSOX sox;
const uint32_t PERIOD_US = 1000000UL / CLF_FS;
uint32_t next_us = 0;

static float ring[CLF_WINDOW][6];
static int ring_head = 0, ring_count = 0, since_last = 0;
static float window[CLF_WINDOW][6];
static float feats[CLF_N_FEATURES];

static bool self_test() {
  uint32_t t0 = micros();
  clf_features(SELFTEST_RAW, feats);
  ClfResult r = clf_infer(feats);
  uint32_t dt = micros() - t0;
  float max_feat_err = 0, max_prob_err = 0;
  for (int i = 0; i < CLF_N_FEATURES; i++) {
    float scale = fabsf(SELFTEST_FEATURES[i]) > 1 ? fabsf(SELFTEST_FEATURES[i]) : 1;
    float e = fabsf(feats[i] - SELFTEST_FEATURES[i]) / scale;
    if (e > max_feat_err) max_feat_err = e;
  }
  for (int j = 0; j < CLF_N_CLASSES; j++) {
    float e = fabsf(r.probs[j] - SELFTEST_PROBS[j]);
    if (e > max_prob_err) max_prob_err = e;
  }
  bool ok = max_feat_err < 1e-3f && max_prob_err < 1e-3f;
  Serial.printf("SELFTEST,%s,label=%s,conf=%.3f,max_feat_rel_err=%.2e,max_prob_err=%.2e,features+inference_us=%lu\n",
                ok ? "PASS" : "FAIL", CLF_CLASSES[r.label], r.conf, max_feat_err, max_prob_err, (unsigned long)dt);
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.printf("classifier: %d features, %d layers, window %d, hop %d\n",
                CLF_N_FEATURES, CLF_N_LAYERS, CLF_WINDOW, CLF_HOP);
  self_test();
  if (!sox.begin_I2C()) {
    Serial.println("LSM6DSOX not found; self-test only");
    while (true) delay(1000);
  }
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
  sox.setAccelDataRate(LSM6DS_RATE_104_HZ);
  sox.setGyroDataRate(LSM6DS_RATE_104_HZ);
  Serial.println("t_ms,ax,ay,az,gx,gy,gz");
  next_us = micros();
}

void loop() {
  if ((int32_t)(micros() - next_us) < 0) return;
  next_us += PERIOD_US;

  sensors_event_t a, g, t;
  sox.getEvent(&a, &g, &t);
  uint32_t t_ms = millis();
  float* s = ring[ring_head];
  s[0] = a.acceleration.x; s[1] = a.acceleration.y; s[2] = a.acceleration.z;
  s[3] = g.gyro.x * 57.2957795f; s[4] = g.gyro.y * 57.2957795f; s[5] = g.gyro.z * 57.2957795f;
  ring_head = (ring_head + 1) % CLF_WINDOW;
  if (ring_count < CLF_WINDOW) ring_count++;
#if STREAM_RAW
  Serial.printf("%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n", (unsigned long)t_ms, s[0], s[1], s[2], s[3], s[4], s[5]);
#endif

  if (ring_count == CLF_WINDOW && ++since_last >= CLF_HOP) {
    since_last = 0;
    for (int i = 0; i < CLF_WINDOW; i++) {
      const float* src = ring[(ring_head + i) % CLF_WINDOW];   // oldest first
      for (int c = 0; c < 6; c++) window[i][c] = src[c];
    }
    uint32_t t0 = micros();
    clf_features(window, feats);
    ClfResult r = clf_infer(feats);
    uint32_t dt = micros() - t0;
    Serial.printf("C,%lu,%s,%.2f,%lu\n", (unsigned long)t_ms, CLF_CLASSES[r.label], r.conf, (unsigned long)dt);
  }
}
