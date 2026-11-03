// 01_blink: proves the board enumerates, uploads and runs.
// Arduino Nano ESP32. Tools > Board > Arduino ESP32 Boards > Arduino Nano ESP32.
// The RGB LED pins are ACTIVE LOW on this board: LOW = on. LED_BUILTIN (yellow) is active HIGH.

const int rgb[3] = {LED_RED, LED_GREEN, LED_BLUE};

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 3; i++) {
    pinMode(rgb[i], OUTPUT);
    digitalWrite(rgb[i], HIGH);  // off
  }
}

void loop() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(rgb[i], LOW);   // on
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.printf("rgb %d on, uptime %lu ms\n", i, millis());
    delay(300);
    digitalWrite(rgb[i], HIGH);  // off
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
  }
}
