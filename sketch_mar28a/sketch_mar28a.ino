#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"
#include "Preferences.h"

Preferences prefs;

// CONFIGURATION
const char* ssid = "TestDevLab-Guest";
const char* password = "";

const char* webhookUrl = "https://discord.com/api/webhooks/1355142639048986684/RdtSC3huBSFZ2GA6rC5Gl3frSySLFpsSxrHCBINNybU9vjlxoaXE1Ee4okFnWHjcOY9V";
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";
const char* messagesUrl = "https://raw.githack.com/LinardsN/AirPing/main/sketch_mar28a/messages.txt";

const gpio_num_t buttonPin = GPIO_NUM_13;

String messages[200];
int messageCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  prefs.begin("airping", false);
  loadMessages();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    // Main button pressed: Send Discord message and sleep
    Serial.println("🔘 Button pressed: Sending Discord message...");
    connectWiFi();
    sendDiscordMessage();
    enterDeepSleep();
  }
  else if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
    // BOOT button pressed or Reset: Perform OTA and fetch messages
    Serial.println("⚙️ Maintenance boot (BOOT button/reset). Fetching messages + OTA...");
    connectWiFi();
    fetchMessages();
    saveMessages();
    performOTAUpdate();
    enterDeepSleep();
  }
  else {
    // Any other boot: just sleep immediately
    Serial.println("⏳ Normal boot. Sleeping immediately...");
    enterDeepSleep();
  }
}

void loop() {
  // unused
}

// FUNCTIONS

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi...");
  unsigned long timeout = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - timeout < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\n✅ WiFi connected!" : "\n⚠️ WiFi failed!");
}

void fetchMessages() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  Serial.println("⬇️ Fetching messages...");
  messageCount = 0;

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
    Serial.println("⚠️ No messages stored!");
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

  if (ret == HTTP_UPDATE_OK)
    Serial.println("✅ OTA update successful!");
  else if (ret == HTTP_UPDATE_NO_UPDATES)
    Serial.println("✅ Firmware up to date.");
  else
    Serial.printf("❌ OTA failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
}

void enterDeepSleep() {
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  Serial.println("💤 Entering deep sleep...");
  delay(200);
  esp_deep_sleep_start();
}

void saveMessages() {
  prefs.putInt("count", messageCount);
  for (int i = 0; i < messageCount; i++)
    prefs.putString(("msg" + String(i)).c_str(), messages[i]);
  Serial.println("💾 Messages saved to memory.");
}

void loadMessages() {
  messageCount = prefs.getInt("count", 0);
  for (int i = 0; i < messageCount; i++)
    messages[i] = prefs.getString(("msg" + String(i)).c_str(), "");
  Serial.printf("📂 Loaded %d messages from memory.\n", messageCount);
}
