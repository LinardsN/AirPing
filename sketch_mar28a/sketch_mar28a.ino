#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"

// Wi-Fi Credentials
const char* ssid = "Pretty-Fly-For-A-WiFi";
const char* password = "Simsons1982";

// Discord Webhook
const char* webhookUrl = "https://discord.com/api/webhooks/1355250299434172597/NERGOBYhZGbssnJx9O32151kHp3857ZBUMazs5PdFLEujspVOdNp3aj1RtEfMawYGrSM";

// Firmware update URL (your uploaded .bin file)
const char* firmwareUrl = "https://github.com/LinardsN/AirPing/raw/refs/heads/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";

// GPIO Definitions
const gpio_num_t buttonPin = GPIO_NUM_13;
const int ledPin = 2;

// Messages array (same as before)
const char* messages[] = {
  "Works!!!",
};

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(buttonPin, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    sendDiscordMessage();

    Serial.println("🔍 Checking for OTA update...");
    performOTAUpdate();

    delay(2000);  // short delay after update check
  } else {
    Serial.println("\n⚠️ WiFi connection failed.");
  }

  enterDeepSleep();
}

void loop() {
  // unused due to deep sleep
}

void sendDiscordMessage() {
  int msgIndex = random(0, sizeof(messages) / sizeof(messages[0]));
  String content = "@here " + String(messages[msgIndex]);

  HTTPClient http;
  http.begin(webhookUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"content\":\"" + content + "\"}";
  int response = http.POST(payload);

  if (response > 0) {
    Serial.println("✅ Message sent: " + content);
  } else {
    Serial.printf("❌ Failed to send message. HTTP code: %d\n", response);
  }

  http.end();
}

void performOTAUpdate() {
  WiFiClient client;
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Update Failed. Error (%d): %s\n", 
        httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("✅ Firmware already latest version.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA Update Successful!");
      break;
  }
}

void enterDeepSleep() {
  Serial.println("💤 Entering deep sleep now...");
  esp_sleep_enable_ext0_wakeup(buttonPin, 0); // Wake on button LOW
  delay(200);
  esp_deep_sleep_start();
}
