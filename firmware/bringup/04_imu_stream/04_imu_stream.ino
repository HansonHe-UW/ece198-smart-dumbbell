// 04_imu_stream: reads the LSM6DSOX on the Adafruit 4517 and prints CSV at a fixed rate.
// Libraries (Library Manager): "Adafruit LSM6DS" (pulls in Adafruit Unified Sensor and BusIO).
// Output: t_ms,ax,ay,az,gx,gy,gz   accel in m/s^2, gyro in deg/s.
// Pass: values update, flat board shows az about +9.8, rotating it swings the gyro columns.
// The same format is the recording format for the ML data set. Keep it stable.

#include <Adafruit_LSM6DSOX.h>

Adafruit_LSM6DSOX sox;
const uint32_t PERIOD_US = 1000000UL / 104;   // 104 Hz, matches the sensor ODR below
uint32_t next_us = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  if (!sox.begin_I2C()) {                       // 0x6A
    Serial.println("ERROR: LSM6DSOX not found at 0x6A. Run 03_i2c_scan.");
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
  const float R2D = 57.2957795f;
  Serial.printf("%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n",
                millis(),
                a.acceleration.x, a.acceleration.y, a.acceleration.z,
                g.gyro.x * R2D, g.gyro.y * R2D, g.gyro.z * R2D);
}
