#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"
#include "Preferences.h"
#include <time.h>

Preferences prefs;

// === CONFIGURATION ===
const char* ssid = "TestDevLab-Guest";
const char* password = "";

const char* webhookUrl = "https://discord.com/api/webhooks/1355142639048986684/RdtSC3huBSFZ2GA6rC5Gl3frSySLFpsSxrHCBINNybU9vjlxoaXE1Ee4okFnWHjcOY9V";
const char* maintenanceWebhookUrl = "https://discord.com/api/webhooks/1356533458519724032/Yk-EVARE1Y_mU1YVANwETHQocCJBQhMf4Bq30Pr3PqqZmAXu_n7qEyH4AMR9obCk2GsS";
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";
const char* messagesUrl = "https://raw.githack.com/LinardsN/AirPing/main/sketch_mar28a/messages.txt";

const gpio_num_t buttonPin = GPIO_NUM_13;
const unsigned long cooldownSeconds = 15 * 60; // 15 minutes

String messages[200];
int messageCount = 0;
bool isWiFiReady = false;
unsigned long pressCount = 0;

// === VERSIONING ===
String monthNum(const char* dateStr) {
  String monthStr = String(dateStr).substring(0, 3);
  if (monthStr == "Jan") return "01";
  if (monthStr == "Feb") return "02";
  if (monthStr == "Mar") return "03";
  if (monthStr == "Apr") return "04";
  if (monthStr == "May") return "05";
  if (monthStr == "Jun") return "06";
  if (monthStr == "Jul") return "07";
  if (monthStr == "Aug") return "08";
  if (monthStr == "Sep") return "09";
  if (monthStr == "Oct") return "10";
  if (monthStr == "Nov") return "11";
  if (monthStr == "Dec") return "12";
  return "00";
}

String firmwareVersion = "v" + String(__DATE__).substring(7,11) + 
                         monthNum(__DATE__) +
                         String(__DATE__).substring(4,6) + "-" +
                         String(__TIME__).substring(0,2) + 
                         String(__TIME__).substring(3,5);

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  prefs.begin("airping", false);
  loadMessages();
  pressCount = prefs.getULong("pressCount", 0);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool wokeFromButton = (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0);
  bool maintenanceMode = !wokeFromButton && digitalRead(buttonPin) == LOW;

  log("📂 Loaded " + String(messageCount) + " messages from memory.");

  if (wokeFromButton) {
    log("🔘 Wake from deep sleep by button press.");
    connectWiFi();
    syncTime();

    time_t now = time(nullptr);
    time_t lastSent = prefs.getULong("lastSent", 0);

    if (lastSent == 0 || now - lastSent >= cooldownSeconds) {
      if (sendDiscordMessage()) {
        prefs.putULong("lastSent", now);
        pressCount++;
        prefs.putULong("pressCount", pressCount);
        log("✅ Public message sent. Total calls outside: " + String(pressCount));
      } else {
        log("❌ Failed to send public message.");
      }
    } else {
      unsigned long remaining = cooldownSeconds - (now - lastSent);
      unsigned long minutes = remaining / 60;
      unsigned long seconds = remaining % 60;
      log("⏳ Cooldown active: " + String(minutes) + "m " + String(seconds) + "s remaining.");
    }

    enterDeepSleep();
  }
  else if (maintenanceMode) {
    log("🛠️ Maintenance mode (button held at boot): Fetching messages + OTA...");
    connectWiFi();
    syncTime();
    fetchMessages();
    saveMessages();
    log("📥 Fetched and saved messages.");
    performOTAUpdate();
    enterDeepSleep();
  }
  else {
    log("⏳ Normal boot/reset. Sleeping...");
    enterDeepSleep();
  }
}

void loop() {}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  log("\n✅ WiFi connected! Signal strength: " + String(WiFi.RSSI()) + " dBm");
  isWiFiReady = true;
  log("📶 WiFi connected with RSSI: " + String(WiFi.RSSI()) + " dBm");
}

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("⏳ Syncing time");
  time_t now = time(nullptr);
  int retries = 0;
  while (now < 100000 && retries < 20) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retries++;
  }
  Serial.println();

  if (now > 100000) {
    log("✅ Time synced: " + String(ctime(&now)));
  } else {
    log("❌ Failed to sync time.");
  }
}

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
    log("⬇️ Fetching messages (attempt " + String(attempt + 1) + ")...");

    if (https.begin(client, messagesUrl)) {
      int httpCode = https.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        parseMessages(payload);
        log("✅ Loaded " + String(messageCount) + " messages.");
        success = true;
      } else {
        log("❌ Fetch failed (HTTP " + String(httpCode) + ")");
      }
      https.end();
    } else {
      log("❌ HTTPS connection failed.");
    }

    if (!success) {
      attempt++;
      if (attempt < maxRetries) {
        delay(2000);
        log("🔁 Retrying fetch...");
      } else {
        log("❌ All attempts to fetch messages failed.");
      }
    }
  }
}

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

bool sendDiscordMessage() {
  if (messageCount == 0) {
    log("⚠️ No messages available to send.");
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
      log("✅ Discord sent: " + content);
      http.end();
      return true;
    } else {
      log("❌ Discord failed (HTTP " + String(response) + "). Retrying...");
      http.end();
      delay(1000);
    }
  }
}

void performOTAUpdate() {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);
  log("🔄 Checking for OTA update...");
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, "1.0");

  if (ret == HTTP_UPDATE_OK)
    log("✅ OTA update successful!");
  else if (ret == HTTP_UPDATE_NO_UPDATES)
    log("✅ Firmware already up to date.");
  else
    log("❌ OTA failed: " + String(httpUpdate.getLastErrorString()));
}

void enterDeepSleep() {
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  log("💤 Entering deep sleep...");
  delay(200);
  esp_deep_sleep_start();
}

void saveMessages() {
  prefs.putInt("count", messageCount);
  for (int i = 0; i < messageCount; i++)
    prefs.putString(("msg" + String(i)).c_str(), messages[i]);
  log("💾 Messages saved.");
}

void loadMessages() {
  messageCount = prefs.getInt("count", 0);
  for (int i = 0; i < messageCount; i++)
    messages[i] = prefs.getString(("msg" + String(i)).c_str(), "");
}

void log(String msg) {
  String versioned = firmwareVersion + " | " + msg;
  Serial.println(msg);

  if (!isWiFiReady || WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(maintenanceWebhookUrl);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"content\":\"🛠️ " + versioned + "\"}";
  http.POST(payload);
  http.end();
}
