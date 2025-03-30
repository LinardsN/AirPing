#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"

// Wi-Fi Credentials
const char* ssid = "Pretty-Fly-For-A-WiFi";
const char* password = "Simsons1982";

// Discord Webhook URL
const char* webhookUrl = "https://discord.com/api/webhooks/1355250299434172597/NERGOBYhZGbssnJx9O32151kHp3857ZBUMazs5PdFLEujspVOdNp3aj1RtEfMawYGrSM";

// OTA Firmware URL
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";

// GPIO Definitions
const gpio_num_t buttonPin = GPIO_NUM_13;
const int ledPin = 2;

// OTA Update Window (2 min)
const unsigned long otaDuration = 2 * 60 * 1000UL;
const unsigned long debounceTime = 1000;

// Discord Messages
const char* messages[] = {
  "WORKS!"
};

unsigned long lastPressTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi...");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
      Serial.println("🔘 Button pressed - sending message.");
      sendDiscordMessage();
    } else {
      Serial.println("⚠️ Power-up or reset detected. No message sent.");
    }

    performOTAUpdate();

    Serial.println("⏳ OTA active for 2 min. Press button to send message.");

    unsigned long otaStartTime = millis();
    lastPressTime = millis();

    while (millis() - otaStartTime < otaDuration) {
      if (digitalRead(buttonPin) == LOW && (millis() - lastPressTime > debounceTime)) {
        Serial.println("🔘 Button pressed! Sending another message...");
        sendDiscordMessage();
        lastPressTime = millis();
      }
      delay(10);
    }

  } else {
    Serial.println("\n⚠️ WiFi failed.");
  }

  enterDeepSleep();
}

void loop() {
  // unused (deep sleep)
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
  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("🔄 Checking OTA update...");

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, "1.0"); // <-- Version here clearly

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


void enterDeepSleep() {
  Serial.println("💤 Deep sleep now...");
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  delay(200);
  esp_deep_sleep_start();
}
