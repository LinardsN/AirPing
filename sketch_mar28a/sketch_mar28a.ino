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
const char* firmwareUrl = "https://raw.githubusercontent.com/YourUsername/yourrepo/main/firmware.bin";

// GPIO Definitions
const gpio_num_t buttonPin = GPIO_NUM_13;
const int ledPin = 2;

// Messages array (same as before)
const char* messages[] = {
  "Build failed: Your vitamin D levels are below acceptable thresholds. Go debug outside.",
  "Attention: The Jira backlog just overtook your life expectancy. Go outside immediately.",
  "Reminder: Zero known critical bugs exist outside—your office chair disagrees.",
  "Edgars merged his sanity to master branch by mistake. Get him outside quickly.",
  "Renārs hit his daily quota of sighs. Mandatory outdoor session required.",
  "Linards' bug count exceeded available integers. Reset him outside.",
  "Gatis delayed his blog post so much it's now legacy software. Go discuss outside.",
  "Each failed Jenkins build equals one minute outside. You now owe 43 hours.",
  "Manual testers detected sunlight. Investigate immediately.",
  "Dev team’s pull request to remove windows and doors approved—clearly wants fresh air.",
  "Wi-Fi password updated to 'GoOutsideAlready!'. Try connecting outdoors.",
  "Scrum Master says this sprint is a marathon. Pace yourself—outside.",
  "Current build status: 'Passed, but emotionally compromised.' Outdoor therapy required.",
  "Code coverage 99%, sunlight coverage 0%. Fix immediately outside.",
  "Deploy successful: Oxygen to lungs. Confirm deployment outdoors.",
  "Office chair uptime: 100%. Leg usage uptime: 0%. Balance required.",
  "Vitamin D deficiency detected in testers. Execute 'Go Outside' test case.",
  "The only bug-free environment is outdoors. Investigate immediately.",
  "Edgars’ beard evolved sentience. Escort him outdoors.",
  "Production incident: Your posture crashed. Emergency hotfix outside.",
  "Git blame points directly at sedentary lifestyle. Refactor outdoors.",
  "API response: 200ms. Sunlight response: infinity. Adjust latency outside.",
  "Sprint retrospective: lack of sunlight is the root cause of all issues.",
  "Deploy outdoors approved. Execute manual verification.",
  "Server load normal. Mental load critical. Fresh-air reboot required.",
  "Code review: Too many unused limbs. Optimize immediately outdoors.",
  "QA found 50 bugs and your social life—both fixed by stepping outside.",
  "Docs updated: 'Best practices include regular sunlight exposure.'",
  "CRITICAL: Your sanity is deprecated. Update available outdoors.",
  "BREAKING: Outdoors proven less buggy than your codebase. Confirm now."
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
