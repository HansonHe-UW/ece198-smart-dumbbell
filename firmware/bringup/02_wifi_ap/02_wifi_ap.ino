// 02_wifi_ap: proves the radio works. The board opens its own access point.
// Pass: the SSID shows up on a phone, you can join it, and http://192.168.4.1 answers.
// Change the password before using this outside the bench.

#include <WiFi.h>
#include <WiFiServer.h>

const char* AP_SSID = "SmartDumbbell-57";
const char* AP_PASS = "ece198test";   // at least 8 characters

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(500);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("softAP %s -> %s\n", AP_SSID, ok ? "started" : "FAILED");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());   // normally 192.168.4.1
  server.begin();
}

void loop() {
  // heartbeat every 2 s so the test does not depend on catching the boot message
  static unsigned long last = 0;
  if (millis() - last >= 2000) {
    last = millis();
    Serial.printf("AP %s  ip=%s  stations=%d  uptime=%lu ms\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str(),
                  WiFi.softAPgetStationNum(), millis());
  }
  WiFiClient client = server.available();
  if (!client) return;
  unsigned long t0 = millis();
  while (client.connected() && !client.available() && millis() - t0 < 1000) delay(1);
  while (client.available()) client.read();  // discard the request
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.printf("Smart dumbbell bring-up. uptime %lu ms, clients %d\n", millis(), WiFi.softAPgetStationNum());
  client.stop();
  Serial.printf("served a request, stations=%d\n", WiFi.softAPgetStationNum());
}
