// 05_max30102: raw red/IR stream from the borrowed GY-MAX30102 plus a simple beat detector.
// Library (Library Manager): "SparkFun MAX3010x Pulse and Proximity Sensor Library".
// Bus: same I2C as the IMU (A4 SDA, A5 SCL). Module VIN -> 3V3, GND -> GND. Address 0x57.
// Output CSV at about 100 Hz: t_ms,ir,red,finger,beat,bpm_inst,bpm_avg
// Pass: with no finger ir is small (< 20000); with a finger resting on the window ir jumps
// above 50000 and a beat is flagged roughly once per heartbeat; bpm_avg within +-5 of a
// manual 30 s pulse count after 20 s of settling. Press lightly: squeezing kills the signal.

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 sensor;

const uint32_t FINGER_THRESHOLD = 50000;   // IR counts; tune after seeing real values
const byte N_AVG = 4;
float rates[N_AVG];
byte rateIdx = 0;
long lastBeatMs = 0;
float bpmInst = 0, bpmAvg = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Wire.begin();
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {        // 400 kHz
    Serial.println("ERROR: MAX30102 not found at 0x57. Run 03_i2c_scan; check VIN=3V3 and the pull-up rail.");
    while (true) delay(1000);
  }
  // ledBrightness 0x1F = ~6 mA, sampleAverage 4, ledMode 2 = red+IR, 400 sps -> 100 Hz out, 411 us, 4096 nA
  // (100 Hz effective is what SparkFun's checkForBeat() was tuned for; at 25 Hz it misses most beats.)
  sensor.setup(0x1F, 4, 2, 400, 411, 4096);
  sensor.setPulseAmplitudeGreen(0);                 // MAX30102 has no green LED
  Serial.printf("part id 0x%02X (0x15 expected)\n", sensor.readPartID());
  Serial.println("t_ms,ir,red,finger,beat,bpm_inst,bpm_avg");
}

void loop() {
  // Read through the FIFO. (getIR()+getRed() each wait for a *new* sample, which halves the rate.)
  sensor.check();
  if (!sensor.available()) return;
  long ir = sensor.getFIFOIR();
  long red = sensor.getFIFORed();
  sensor.nextSample();
  bool finger = ir > FINGER_THRESHOLD;
  int beat = 0;

  if (checkForBeat(ir)) {
    long now = millis();
    long delta = now - lastBeatMs;
    lastBeatMs = now;
    float b = 60000.0f / delta;
    if (b > 30 && b < 220) {
      beat = 1; bpmInst = b;
      rates[rateIdx++] = b; rateIdx %= N_AVG;
      float s = 0; for (byte i = 0; i < N_AVG; i++) s += rates[i];
      bpmAvg = s / N_AVG;
    }
  }
  if (!finger) { bpmInst = 0; bpmAvg = 0; rateIdx = 0; for (byte i = 0; i < N_AVG; i++) rates[i] = 0; }

  Serial.printf("%lu,%ld,%ld,%d,%d,%.1f,%.1f\n", millis(), ir, red, finger ? 1 : 0, beat, bpmInst, bpmAvg);
}
