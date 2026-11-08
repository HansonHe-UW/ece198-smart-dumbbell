// 03_i2c_scan: lists every device that answers on the I2C bus (A4 = SDA, A5 = SCL).
// Expected with the 4517 connected: 0x6A (LSM6DSOX) and 0x1C (LIS3MDL).
// Later: 0x57 (MAX30102), 0x3C (OLED).
// Nothing found usually means SDA/SCL swapped, no 3V3, or a loose STEMMA QT plug.

#include <Wire.h>

const char* nameFor(uint8_t a) {
  switch (a) {
    case 0x6A: case 0x6B: return "LSM6DSOX";
    case 0x1C: case 0x1E: return "LIS3MDL";
    case 0x57: return "MAX30102";
    case 0x3C: case 0x3D: return "SSD1306 OLED";
    default: return "unknown";
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}   // HWCDC: give the host time to attach before the first print
  Wire.begin();            // A4/A5 on the Nano ESP32
  Wire.setClock(400000);
  Wire.setTimeOut(20);     // ms per transaction; a stuck bus (no pull-ups, SDA held low) then scans in ~2 s instead of ~2 min
}

void loop() {
  int found = 0;
  Serial.println("scanning 0x08..0x77");
  for (uint8_t a = 0x08; a < 0x78; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X  %s\n", a, nameFor(a));
      found++;
    }
  }
  Serial.printf("%d device(s)\n\n", found);
  delay(3000);
}
