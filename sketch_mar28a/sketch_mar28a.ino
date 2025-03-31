#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"
#include "Preferences.h"

Preferences prefs;

// === CONFIGURATION ===
const char* ssid = "TestDevLab-Guest";
const char* password = "";

const char* webhookUrl = "https://discord.com/api/webhooks/1355142639048986684/RdtSC3huBSFZ2GA6rC5Gl3frSySLFpsSxrHCBINNybU9vjlxoaXE1Ee4okFnWHjcOY9V";
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";
const char* messagesUrl = "https://raw.githack.com/LinardsN/AirPing/main/sketch_mar28a/messages.txt";

const gpio_num_t buttonPin = GPIO_NUM_13;
String messages[200];
int messageCount = 0;

// === SETUP ===
void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  prefs.begin("airping", false);
  loadMessages();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool wokeFromButton = (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0);
  bool maintenanceMode = !wokeFromButton && digitalRead(buttonPin) == LOW;

  if (wokeFromButton) {
    Serial.println("🔘 Woke from deep sleep by button: Sending Discord message...");
    if (connectWiFi()) {
      sendDiscordMessage();
    } else {
      Serial.println("❌ WiFi not connected. Skipping message.");
    }
    enterDeepSleep();
  }
  else if (maintenanceMode) {
    Serial.println("🛠️ Maintenance mode (button held at reset): Fetching messages + OTA...");
    if (connectWiFi()) {
      fetchMessages();
      saveMessages();
      performOTAUpdate();
    } else {
      Serial.println("❌ WiFi failed. Skipping OTA & message fetch.");
    }
    enterDeepSleep();
  }
  else {
    Serial.println("⏳ Normal boot/reset. Sleeping...");
    enterDeepSleep();
  }
}

void loop() {
  // unused
}

// === FUNCTIONS ===

bool connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    return true;
  } else {
    Serial.println("\n⚠️ WiFi failed.");
    return false;
  }
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
    Serial.println("⚠️ No messages available to send.");
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

  Serial.println("🔄 Checking for OTA update...");
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, "1.0");

  if (ret == HTTP_UPDATE_OK)
    Serial.println("✅ OTA update successful!");
  else if (ret == HTTP_UPDATE_NO_UPDATES)
    Serial.println("✅ Firmware already up to date.");
  else
    Serial.printf("❌ OTA failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
}

void enterDeepSleep() {
  esp_sleep_enable_ext0_wakeup(buttonPin, 0); // Wake on button press
  Serial.println("💤 Entering deep sleep...");
  delay(200);
  esp_deep_sleep_start();
}

void saveMessages() {
  prefs.putInt("count", messageCount);
  for (int i = 0; i < messageCount; i++)
    prefs.putString(("msg" + String(i)).c_str(), messages[i]);
  Serial.println("💾 Messages saved.");
}

void loadMessages() {
  messageCount = prefs.getInt("count", 0);
  for (int i = 0; i < messageCount; i++)
    messages[i] = prefs.getString(("msg" + String(i)).c_str(), "");
  Serial.printf("📂 Loaded %d messages from memory.\n", messageCount);
}
