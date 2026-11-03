// 07_oled_buttons: usability check of the kit OLED, the three pushbuttons, an LED and a buzzer.
// A tiny mock of the real UI: SW1 start/pause, SW2 adds one "rep", SW3 ends and resets.
// Libraries (Library Manager): Adafruit SSD1306, Adafruit GFX.
// Wiring (see README step 7): OLED on the shared I2C bus (0x3C or 0x3D), buttons to GND with
// INPUT_PULLUP on D2/D3/D4, LED + 220 ohm on D7, buzzer SIG on D5 (optional).
// Pass: text visible and sharp, big count readable at arm's length, every press registers once
// (no double counts), LED blinks and buzzer beeps on each rep, serial logs every event.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
uint8_t oledAddr = 0;

const int PIN_START = D2, PIN_REP = D3, PIN_END = D4;
const int PIN_BUZZ = D5, PIN_LED = D7;

enum State { IDLE, RUNNING, PAUSED } state = IDLE;
int reps = 0, target = 10;
unsigned long sessionStartMs = 0;

struct Btn { int pin; bool last; unsigned long tChange; };
Btn btns[3] = {{PIN_START, true, 0}, {PIN_REP, true, 0}, {PIN_END, true, 0}};

bool pressed(Btn &b) {                       // debounced falling edge, 30 ms
  bool now = digitalRead(b.pin);
  if (now != b.last && millis() - b.tChange > 30) {
    b.tChange = millis(); b.last = now;
    return now == LOW;
  }
  return false;
}

void beep(int ms) { digitalWrite(PIN_BUZZ, HIGH); digitalWrite(PIN_LED, HIGH); delay(ms); digitalWrite(PIN_BUZZ, LOW); digitalWrite(PIN_LED, LOW); }

void draw() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(state == IDLE ? "IDLE" : state == RUNNING ? "RUNNING" : "PAUSED");
  oled.setCursor(80, 0);
  oled.print("HR --");
  oled.setTextSize(4);                       // 24 px high digits
  oled.setCursor(10, 20);
  oled.printf("%2d", reps);
  oled.setTextSize(2);
  oled.setCursor(70, 34);
  oled.printf("/%d", target);
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  if (state == IDLE) oled.print("SW1 start  SW2 rep  SW3 end");
  else oled.printf("t=%lus", (millis() - sessionStartMs) / 1000);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  pinMode(PIN_START, INPUT_PULLUP); pinMode(PIN_REP, INPUT_PULLUP); pinMode(PIN_END, INPUT_PULLUP);
  pinMode(PIN_BUZZ, OUTPUT); pinMode(PIN_LED, OUTPUT);
  Wire.begin(); Wire.setClock(400000);
  for (uint8_t a : {0x3C, 0x3D}) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { oledAddr = a; break; }
  }
  if (!oledAddr || !oled.begin(SSD1306_SWITCHCAPVCC, oledAddr)) {
    Serial.println("ERROR: no SSD1306 at 0x3C/0x3D. Check VCC/GND order on the module and run 03_i2c_scan.");
    while (true) { digitalWrite(PIN_LED, !digitalRead(PIN_LED)); delay(200); }
  }
  Serial.printf("OLED at 0x%02X\n", oledAddr);
  oled.setRotation(0);
  draw();
  beep(60);
}

void loop() {
  bool changed = false;
  if (pressed(btns[0])) {
    if (state == IDLE) { state = RUNNING; reps = 0; sessionStartMs = millis(); Serial.println("EVENT start"); }
    else if (state == RUNNING) { state = PAUSED; Serial.println("EVENT pause"); }
    else { state = RUNNING; Serial.println("EVENT resume"); }
    changed = true; beep(40);
  }
  if (pressed(btns[1])) {
    if (state == RUNNING) { reps++; Serial.printf("EVENT rep %d\n", reps); beep(reps == target ? 300 : 40); }
    else Serial.println("EVENT rep ignored (not running)");
    changed = true;
  }
  if (pressed(btns[2])) {
    Serial.printf("EVENT end, total reps %d, %lu s\n", reps, (millis() - sessionStartMs) / 1000);
    state = IDLE; changed = true; beep(120);
  }
  static unsigned long lastDraw = 0;
  if (changed || millis() - lastDraw > 500) { draw(); lastDraw = millis(); }   // 2 Hz refresh for the timer
}
