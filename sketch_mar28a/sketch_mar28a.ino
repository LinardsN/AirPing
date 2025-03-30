#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"

// === CONFIGURATION ===
const char* ssid = "Pretty-Fly-For-A-WiFi";
const char* password = "Simsons1982";

const char* webhookUrl = "https://discord.com/api/webhooks/1355250299434172597/NERGOBYhZGbssnJx9O32151kHp3857ZBUMazs5PdFLEujspVOdNp3aj1RtEfMawYGrSM";
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";
const char* messagesUrl = "https://github.com/LinardsN/AirPing/raw/77a08e66767596047716cd91901feca495dc4087/sketch_mar28a/messages.txt";
//const char* messagesUrl = "https://raw.githack.com/LinardsN/AirPing/main/sketch_mar28a/messages.txt";
const gpio_num_t buttonPin = GPIO_NUM_13;
const unsigned long waitDuration = 60 * 1000UL; // 1 min
const unsigned long debounceTime = 1000;

String messages[200];
int messageCount = 0;
unsigned long lastPressTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  connectWiFi();
  fetchMessages();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("🔘 Woke from deep sleep (button). Sending Discord + OTA...");
    sendDiscordMessage();
    performOTAUpdate();
    ESP.restart();
  } else {
    Serial.println("⚡️ Normal boot. Waiting 1 min for button...");
    unsigned long startWait = millis();

    while (millis() - startWait < waitDuration) {
      if (digitalRead(buttonPin) == LOW && (millis() - lastPressTime > debounceTime)) {
        Serial.println("🔘 Button pressed! Discord + OTA...");
        sendDiscordMessage();
        performOTAUpdate();
        ESP.restart();
      }
      delay(10);
    }

    Serial.println("⏳ No button press. Going to deep sleep...");
    enterDeepSleep();
  }
}

void loop() {
  // unused
}

// === FUNCTIONS ===

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi...");
  unsigned long timeout = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - timeout < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("\n✅ WiFi connected!");
  else
    Serial.println("\n⚠️ WiFi failed!");
}

void fetchMessages() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  messageCount = 0; // Reset messages each boot

  Serial.println("⬇️ Fetching messages...");
  if (https.begin(client, messagesUrl)) {
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      parseMessages(payload);
      Serial.printf("✅ Loaded %d messages.\n", messageCount);
    } else {
      Serial.printf("❌ Fetch messages failed (HTTP %d)\n", httpCode);
    }
    https.end();
  } else {
    Serial.println("❌ HTTPS connection failed.");
  }
}

void parseMessages(String payload) {
  int from = 0, to = 0;
  while ((to = payload.indexOf('\n', from)) >= 0 && messageCount < 200) {
    messages[messageCount++] = payload.substring(from, to);
    from = to + 1;
  }
  if (from < payload.length() && messageCount < 200) {
    messages[messageCount++] = payload.substring(from);
  }
}

void sendDiscordMessage() {
  if (messageCount == 0) {
    Serial.println("⚠️ No messages to send!");
    return;
  }

  String content = "@here " + messages[random(messageCount)];

  HTTPClient http;
  http.begin(webhookUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"content\":\"" + content + "\"}";
  int response = http.POST(payload);

  if (response > 0)
    Serial.println("✅ Discord sent: " + content);
  else
    Serial.printf("❌ Discord failed (HTTP %d)\n", response);

  http.end();
}

void performOTAUpdate() {
  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("🔄 Checking OTA update...");
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, "1.0");

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA failed (%d): %s\n",
                    httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("✅ Firmware up to date.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA update successful!");
      break;
  }
}

void enterDeepSleep() {
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  delay(200);
  esp_deep_sleep_start();
}
