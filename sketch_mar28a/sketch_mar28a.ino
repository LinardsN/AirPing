#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"
#include "Preferences.h"

Preferences prefs;

// === CONFIGURATION ===
const char* ssid = "TestDevLab-Guest";
const char* password = "ThinkQualityFirst";

const char* webhookUrl = "https://discord.com/api/webhooks/1355142639048986684/RdtSC3huBSFZ2GA6rC5Gl3frSySLFpsSxrHCBINNybU9vjlxoaXE1Ee4okFnWHjcOY9V";
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";
const char* messagesUrl = "https://raw.githack.com/LinardsN/AirPing/main/sketch_mar28a/messages.txt";

const gpio_num_t buttonPin = GPIO_NUM_13;
String messages[200];
int messageCount = 0;

// === FORWARD DECLARATIONS ===
void loadMessages();
void saveMessages();
void enterDeepSleep();
void connectWiFi();
void fetchMessages();
void parseMessages(String payload);
bool sendDiscordMessage();
void performOTAUpdate();

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
    Serial.println("🔘 Wake from deep sleep by button press.");
    connectWiFi();
    sendDiscordMessage();
    enterDeepSleep();
  }
  else if (maintenanceMode) {
    Serial.println("🛠️ Maintenance mode (button held at boot): Fetching messages + OTA...");
    connectWiFi();
    fetchMessages();
    saveMessages();
    performOTAUpdate();
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

// === CONNECT TO WIFI (retry until success) ===
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi (no timeout)");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\n✅ WiFi connected! Signal strength: %d dBm\n", WiFi.RSSI());
}

// === FETCH MESSAGES WITH RETRY ===
void fetchMessages() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  const int timeoutMs = 15000;
  const int maxRetries = 2;
  int attempt = 0;
  bool success = false;

  while (attempt < maxRetries && !success) {
    https.setTimeout(timeoutMs);
    Serial.printf("⬇️ Fetching messages (attempt %d)...\n", attempt + 1);

    if (https.begin(client, messagesUrl)) {
      int httpCode = https.GET();

      if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        parseMessages(payload);
        Serial.printf("✅ Loaded %d messages.\n", messageCount);
        success = true;
      } else {
        Serial.printf("❌ Fetch failed (HTTP %d)\n", httpCode);
      }

      https.end();
    } else {
      Serial.println("❌ HTTPS connection failed.");
    }

    if (!success) {
      attempt++;
      if (attempt < maxRetries) {
        delay(2000);
        Serial.println("🔁 Retrying fetch...");
      } else {
        Serial.println("❌ All attempts to fetch messages failed.");
      }
    }
  }
}

// === PARSE MESSAGES ===
void parseMessages(String payload) {
  int from = 0, to = 0;
  messageCount = 0;
  while ((to = payload.indexOf('\n', from)) >= 0 && messageCount < 200) {
    messages[messageCount++] = payload.substring(from, to);
    from = to + 1;
  }
  if (from < payload.length() && messageCount < 200) {
    messages[messageCount++] = payload.substring(from);
  }
}

// === SEND DISCORD MESSAGE (RETRY UNTIL SUCCESS) ===
bool sendDiscordMessage() {
  if (messageCount == 0) {
    Serial.println("⚠️ No messages available to send.");
    return false;
  }

  String content = "@here " + messages[random(messageCount)];
  String payload = "{\"content\":\"" + content + "\"}";

  while (true) {
    HTTPClient http;
    http.setTimeout(15000);
    http.begin(webhookUrl);
    http.addHeader("Content-Type", "application/json");

    int response = http.POST(payload);

    if (response > 0) {
      Serial.println("✅ Discord sent: " + content);
      http.end();
      return true;
    } else {
      Serial.printf("❌ Discord failed (HTTP %d). Retrying in 2s...\n", response);
      http.end();
      delay(2000);
    }
  }
}

// === OTA UPDATE ===
void performOTAUpdate() {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);

  Serial.println("🔄 Checking for OTA update...");
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, "1.0");

  if (ret == HTTP_UPDATE_OK)
    Serial.println("✅ OTA update successful!");
  else if (ret == HTTP_UPDATE_NO_UPDATES)
    Serial.println("✅ Firmware already up to date.");
  else
    Serial.printf("❌ OTA failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
}

// === DEEP SLEEP ===
void enterDeepSleep() {
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  Serial.println("💤 Entering deep sleep...");
  delay(200);
  esp_deep_sleep_start();
}

// === STORE & LOAD MESSAGES ===
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
