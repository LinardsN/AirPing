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
const char* password = "ThinkQualityFirst";

const char* webhookUrl = "https://discord.com/api/webhooks/1355142639048986684/RdtSC3huBSFZ2GA6rC5Gl3frSySLFpsSxrHCBINNybU9vjlxoaXE1Ee4okFnWHjcOY9V";
const char* maintenanceWebhookUrl = "https://discord.com/api/webhooks/1356533458519724032/Yk-EVARE1Y_mU1YVANwETHQocCJBQhMf4Bq30Pr3PqqZmAXu_n7qEyH4AMR9obCk2GsS";
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";
const char* remoteVersionUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/version.txt";

const gpio_num_t buttonPin = GPIO_NUM_13;
const unsigned long cooldown1 = 15 * 60;
const unsigned long cooldown2 = 10 * 60;
const unsigned long cooldown3 = 30 * 60;

String firmwareVersion = "v20250401-1520";
String messages[200];
int messageCount = 0;
bool isWiFiReady = false;
unsigned long pressCount = 0;

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

    // Start press counting immediately
    unsigned long pressStart = millis();
    int pressCountThisSession = 1;
    bool released = false;
    while (millis() - pressStart < 3000) {
      if (digitalRead(buttonPin) == HIGH) released = true;
      if (released && digitalRead(buttonPin) == LOW) {
        pressCountThisSession++;
        released = false;
        log("🔁 Detected additional press. Count: " + String(pressCountThisSession));
        delay(1000);
      }
    }
    log("🔢 Total presses detected: " + String(pressCountThisSession));

    connectWiFi();
    configTzTime("EET-2EEST,M3.5.0/3,M10.5.0/4", "pool.ntp.org", "time.nist.gov");
    syncTime();

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    if (pressCountThisSession == 1) {
      time_t lastSent = prefs.getULong("lastSent1", 0);
      if (lastSent == 0 || now - lastSent >= cooldown1) {
        if (sendDiscordMessage(1)) {
          prefs.putULong("lastSent1", now);
          pressCount++;
          prefs.putULong("pressCount", pressCount);
          log("✅ Public message sent. Total calls outside: " + String(pressCount));
        } else {
          log("❌ Failed to send public message.");
        }
      } else {
        unsigned long remaining = cooldown1 - (now - lastSent);
        log("⏳ Cooldown (1 press): " + String(remaining / 60) + "m " + String(remaining % 60) + "s");
      }
    } else if (pressCountThisSession == 2) {
      time_t lastSent = prefs.getULong("lastSent2", 0);
      if (lastSent == 0 || now - lastSent >= cooldown2) {
        if (sendDiscordMessage(2)) {
          prefs.putULong("lastSent2", now);
          log("☕ Coffee message sent.");
        }
      } else {
        unsigned long remaining = cooldown2 - (now - lastSent);
        log("⏳ Cooldown (2 presses): " + String(remaining / 60) + "m " + String(remaining % 60) + "s");
      }
    } else if (pressCountThisSession >= 3) {
      if (timeinfo->tm_hour == 11 && timeinfo->tm_min >= 30 || timeinfo->tm_hour == 12 && timeinfo->tm_min <= 30) {
        time_t lastSent = prefs.getULong("lastSent3", 0);
        if (lastSent == 0 || now - lastSent >= cooldown3) {
          if (sendDiscordMessage(3)) {
            prefs.putULong("lastSent3", now);
            log("🍔 Lunch message sent.");
          }
        } else {
          unsigned long remaining = cooldown3 - (now - lastSent);
          log("⏳ Cooldown (3+ presses): " + String(remaining / 60) + "m " + String(remaining % 60) + "s");
        }
      } else {
        log("❌ Lunch message can only be sent between 11:30 and 12:30 (Latvia/Riga).");
      }
    }

    checkForRemoteUpdate();
    enterDeepSleep();

  } else if (maintenanceMode) {
    log("🛠️ Maintenance mode (button held at boot): Fetching messages + OTA...");
    connectWiFi();
    syncTime();
    fetchMessages();
    saveMessages();
    log("📅 Fetched and saved messages.");
    performOTAUpdate();
    enterDeepSleep();
  } else {
    log("⏳ Normal boot/reset. Sleeping...");
    enterDeepSleep();
  }
}

void log(String msg) {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeStr[30];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

  String versioned = firmwareVersion + " | " + msg + " (" + String(timeStr) + ")";
  Serial.println(versioned);

  if (!isWiFiReady || WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(maintenanceWebhookUrl);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"content\":\"🛠️ " + versioned + "\"}";
  http.POST(payload);
  http.end();
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  isWiFiReady = true;
  log("✅ WiFi connected! RSSI: " + String(WiFi.RSSI()));
}

void syncTime() {
  time_t now = time(nullptr);
  int retries = 0;
  while (now < 100000 && retries < 20) {
    delay(500);
    now = time(nullptr);
    retries++;
  }
  if (now > 100000) log("🕒 Time synced: " + String(ctime(&now)));
  else log("❌ Failed to sync time.");
}

bool sendDiscordMessage(int type) {
  String content;
  if (type == 1) {
    if (messageCount == 0) {
      log("⚠️ No messages available to send.");
      return false;
    }
    content = "@here " + messages[random(messageCount)];
  } else if (type == 2) {
    content = "@here ☕ Coffee break!";
  } else if (type == 3) {
    content = "@here 🍔 Lunch time!";
  }

  String payload = "{\"content\":\"" + content + "\"}";

  while (true) {
    HTTPClient http;
    http.setTimeout(15000);
    http.begin(webhookUrl);
    http.addHeader("Content-Type", "application/json");
    int response = http.POST(payload);
    http.end();
    if (response > 0) {
      log("✅ Discord sent: " + content);
      return true;
    } else {
      log("❌ Discord failed (HTTP " + String(response) + "). Retrying...");
      delay(1000);
    }
  }
}

void checkForRemoteUpdate() {
  log("🌐 Checking remote .txt file for firmware version...");

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  https.setTimeout(10000);

  if (https.begin(client, remoteVersionUrl)) {
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      String remoteVersion = https.getString();
      remoteVersion.trim();
      log("🔎 Remote version: " + remoteVersion);
      log("📦 Local version: " + firmwareVersion);

      if (remoteVersion != firmwareVersion) {
        log("⬆️ New version detected! Starting OTA...");
        performOTAUpdate();
      } else {
        log("✅ Firmware is up to date.");
      }
    } else {
      log("❌ Failed to fetch version (HTTP " + String(httpCode) + ")");
    }
    https.end();
  } else {
    log("❌ HTTPS connection failed for version check.");
  }
}

void performOTAUpdate() {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);
  log("🔄 Checking for OTA update...");
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, firmwareVersion);

  if (ret == HTTP_UPDATE_OK) {
    log("✅ OTA update successful! Rebooting into new firmware...");
    delay(500); // Give time for log to send
  } else if (ret == HTTP_UPDATE_NO_UPDATES) {
    log("✅ Firmware already up to date.");
  } else {
    log("❌ OTA failed: " + String(httpUpdate.getLastErrorString()));
  }
}

void enterDeepSleep() {
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  log("😴 Entering deep sleep...");
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

void fetchMessages() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  const int timeoutMs = 15000;
  https.setTimeout(timeoutMs);
  log("🌐 Fetching messages from GitHub...");

  if (https.begin(client, remoteVersionUrl)) {
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      log("✅ Successfully fetched state JSON.");
    } else {
      log("❌ Failed to fetch messages (HTTP " + String(httpCode) + ")");
    }
    https.end();
  } else {
    log("❌ HTTPS connection failed while fetching messages.");
  }
}
void loop() {
  // Not used, since everything is done in setup()
}
