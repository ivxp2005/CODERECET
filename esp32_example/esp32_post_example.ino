#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://YOUR_PC_IP:5000/api/data";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void loop() {
  int sensorValue = analogRead(34); // Example: GPIO34 for piezo
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"value\":" + String(sensorValue) + "}";
    int httpResponseCode = http.POST(payload);
    Serial.print("POST Response code: ");
    Serial.println(httpResponseCode);
    http.end();
  }
  delay(5000); // Send every 5 seconds
} 