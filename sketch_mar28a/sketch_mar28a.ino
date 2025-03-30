#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"

// === Wi-Fi Credentials ===
const char* ssid = "Pretty-Fly-For-A-WiFi";
const char* password = "Simsons1982";

// === Discord Webhook ===
const char* webhookUrl = "https://discord.com/api/webhooks/1355250299434172597/NERGOBYhZGbssnJx9O32151kHp3857ZBUMazs5PdFLEujspVOdNp3aj1RtEfMawYGrSM";

// === OTA Firmware URL ===
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";

// === GPIO Pins ===
const gpio_num_t buttonPin = GPIO_NUM_13;
const int ledPin = 2;

// Debounce settings
const unsigned long debounceTime = 1000;
unsigned long lastPressTime = 0;

// Discord Messages
const char* messages[] = {
  "Fresh air time! 🌬️ Let's head outside!",
  "Time for a breather, team! 🚀"
};

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  connectWiFi();

  Serial.println("Awaiting button press to trigger Discord message and OTA...");
  
  while (true) {
    if (digitalRead(buttonPin) == LOW && (millis() - lastPressTime > debounceTime)) {
      Serial.println("🔘 Button pressed!");

      sendDiscordMessage();
      performOTAUpdate();

      break; // only execute once, no loops
    }
    delay(10);
  }

  enterDeepSleep();
}

void loop() {
  // unused (deep sleep)
}

// === Connect Wi-Fi ===
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi...");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
  } else {
    Serial.println("\n⚠️ WiFi connection failed!");
  }
}

// === Send Discord Message ===
void sendDiscordMessage() {
  int msgIndex = random(0, sizeof(messages) / sizeof(messages[0]));
  String content = "@here " + String(messages[msgIndex]);

  HTTPClient http;
  http.begin(webhookUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"content\":\"" + content + "\"}";
  int response = http.POST(payload);

  if (response > 0) {
    Serial.println("✅ Discord message sent: " + content);
  } else {
    Serial.printf("❌ Failed to send Discord message. HTTP code: %d\n", response);
  }

  http.end();
}

// === OTA Update (Single Execution) ===
void performOTAUpdate() {
  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("🔄 Checking for OTA update...");

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, "1.0"); // Update "1.0" as needed

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA Failed (%d): %s\n",
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

// === Enter Deep Sleep ===
void enterDeepSleep() {
  Serial.println("💤 Entering deep sleep mode...");
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  delay(200);
  esp_deep_sleep_start();
}
