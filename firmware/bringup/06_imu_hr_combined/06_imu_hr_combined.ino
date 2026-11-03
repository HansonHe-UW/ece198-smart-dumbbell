// 06_imu_hr_combined: LSM6DSOX at 104 Hz and MAX30102 at ~25 Hz on the same I2C bus,
// both serviced from one non-blocking loop. This is the first evidence for FR-13
// (heart-rate processing must not stall motion sampling) and AT-07 (run together).
//
// Output, three line types:
//   I,t_ms,ax,ay,az,gx,gy,gz            every IMU tick (104 Hz)
//   H,t_ms,ir,red,finger,beat,bpm_avg   every MAX30102 sample (~25 Hz)
//   S,t_ms,imu_n,imu_hz,imu_dt_max_ms,imu_late,hr_n,hr_hz,loop_max_us   every 5 s
// Pass: imu_hz stays 103-105 and imu_late stays 0 while a finger is on the MAX30102 and
// beats are being detected; hr_hz about 25; loop_max_us well under 9600 (one IMU period).

#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include "MAX30105.h"
#include "heartRate.h"

Adafruit_LSM6DSOX sox;
MAX30105 hr;

const uint32_t IMU_PERIOD_US = 1000000UL / 104;
const long FINGER_THRESHOLD = 50000;

uint32_t imuNext = 0;
uint32_t imuN = 0, hrN = 0, imuLate = 0;
uint32_t imuLastMs = 0, imuDtMax = 0, loopMaxUs = 0;
uint32_t statNext = 0;

float rates[4]; byte rateIdx = 0; long lastBeatMs = 0; float bpmAvg = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Wire.begin();
  Wire.setClock(400000);

  if (!sox.begin_I2C()) { Serial.println("ERROR: LSM6DSOX not found"); while (true) delay(1000); }
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
  sox.setAccelDataRate(LSM6DS_RATE_104_HZ);
  sox.setGyroDataRate(LSM6DS_RATE_104_HZ);

  if (!hr.begin(Wire, I2C_SPEED_FAST)) { Serial.println("ERROR: MAX30102 not found"); while (true) delay(1000); }
  hr.setup(0x1F, 4, 2, 100, 411, 4096);   // ~6 mA, avg 4, red+IR, 100 sps -> 25 Hz out
  hr.setPulseAmplitudeGreen(0);

  Serial.println("# I,t_ms,ax,ay,az,gx,gy,gz | H,t_ms,ir,red,finger,beat,bpm_avg | S,t_ms,imu_n,imu_hz,imu_dt_max_ms,imu_late,hr_n,hr_hz,loop_max_us");
  imuNext = micros();
  statNext = millis() + 5000;
  imuLastMs = millis();
}

void serviceIMU() {
  if ((int32_t)(micros() - imuNext) < 0) return;
  imuNext += IMU_PERIOD_US;
  if ((int32_t)(micros() - imuNext) > 0) imuLate++;   // already behind by a full period

  sensors_event_t a, g, t;
  sox.getEvent(&a, &g, &t);
  uint32_t now = millis();
  uint32_t dt = now - imuLastMs; imuLastMs = now;
  if (imuN > 0 && dt > imuDtMax) imuDtMax = dt;
  imuN++;
  const float R2D = 57.2957795f;
  Serial.printf("I,%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n", now,
                a.acceleration.x, a.acceleration.y, a.acceleration.z,
                g.gyro.x * R2D, g.gyro.y * R2D, g.gyro.z * R2D);
}

void serviceHR() {
  hr.check();                                   // pull whatever is in the FIFO, non-blocking
  while (hr.available()) {
    long ir = hr.getFIFOIR();
    long red = hr.getFIFORed();
    hr.nextSample();
    hrN++;
    bool finger = ir > FINGER_THRESHOLD;
    int beat = 0;
    if (checkForBeat(ir)) {
      long now = millis(); long delta = now - lastBeatMs; lastBeatMs = now;
      float b = 60000.0f / delta;
      if (b > 30 && b < 220) {
        beat = 1; rates[rateIdx++] = b; rateIdx %= 4;
        float s = 0; for (byte i = 0; i < 4; i++) s += rates[i]; bpmAvg = s / 4;
      }
    }
    if (!finger) { bpmAvg = 0; rateIdx = 0; for (byte i = 0; i < 4; i++) rates[i] = 0; }
    Serial.printf("H,%lu,%ld,%ld,%d,%d,%.1f\n", millis(), ir, red, finger ? 1 : 0, beat, bpmAvg);
  }
}

void loop() {
  uint32_t t0 = micros();
  serviceIMU();
  serviceHR();
  uint32_t el = micros() - t0;
  if (el > loopMaxUs) loopMaxUs = el;

  if ((int32_t)(millis() - statNext) >= 0) {
    Serial.printf("S,%lu,%lu,%.1f,%lu,%lu,%lu,%.1f,%lu\n", millis(),
                  imuN, imuN / 5.0f, imuDtMax, imuLate, hrN, hrN / 5.0f, loopMaxUs);
    imuN = 0; hrN = 0; imuDtMax = 0; loopMaxUs = 0;   // imuLate is cumulative
    statNext += 5000;
  }
}
