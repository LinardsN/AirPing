#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include "esp_sleep.h"

// === Wi-Fi Credentials ===
const char* ssid = "Pretty-Fly-For-A-WiFi";
const char* password = "Simsons1982";

// === Discord Webhook URL ===
const char* webhookUrl = "https://discord.com/api/webhooks/1355250299434172597/NERGOBYhZGbssnJx9O32151kHp3857ZBUMazs5PdFLEujspVOdNp3aj1RtEfMawYGrSM";

// === OTA Firmware URL ===
const char* firmwareUrl = "https://raw.githubusercontent.com/LinardsN/AirPing/main/sketch_mar28a/build/esp32.esp32.esp32/sketch_mar28a.ino.bin";

// === GPIO Pins ===
const gpio_num_t buttonPin = GPIO_NUM_13;
const int ledPin = 2;

// === Timing ===
const unsigned long waitDuration = 60 * 1000UL; // 1 minute
const unsigned long debounceTime = 1000;
unsigned long lastPressTime = 0;

// === Discord Messages ===
const char* messages[] = {
  "Your chair called—it needs a break from you. Go outside!",
  "Let's see if sunlight still recognizes you!",
  "The outdoors just opened a Jira ticket: ‘Missing colleague!’",
  "You’ve been flagged as an indoor bug—go resolve it outside.",
  "Time to manually verify if the sun is still there!",
  "Stand up, stretch, and debug your vitamin D deficiency!",
  "Fresh air: the feature nobody requested but everyone needs.",
  "Outside mode activated—your indoors license has expired!",
  "Deploying you to production: the outside world!",
  "You can't CTRL+Z your health—step outside!",
  "Alert: Excessive indoor sessions detected, go refresh outdoors!",
  "Today's most critical issue: your lack of sunlight exposure!",
  "The fresh air is lonely—someone go keep it company!",
  "It's sunny outside, unlike your screen brightness!",
  "Real-life just sent you a friend request—go accept outside!",
  "Step away from your screens and test reality for once!",
  "You've maxed out your indoors quota—go outside!",
  "Urgent patch needed: Your vitamin D firmware is outdated!",
  "Life has an uptime too—improve it outdoors!",
  "IRL needs more of your test coverage—head outside!",
  "Outside world build successful—waiting for your verification!",
  "Friendly reminder: Sunlight doesn't bite (much).",
  "The outdoors requested your immediate attendance!",
  "Bug report: Colleagues stuck indoors. Resolution: Move outside!",
  "Your keyboard requests some alone time. Go outside!",
  "Leave your desk—it's had enough for today.",
  "Step outside and experience the world's best UI: reality!",
  "Warning: Too much indoor latency—fresh air required!",
  "Go generate some real-world logs by stepping outside.",
  "The air outside doesn't require authentication—enjoy it!",
  "Sunlight available—please update your location settings.",
  "Fresh air deployment complete—manual acceptance required!",
  "Sunshine is now available for your beta testing.",
  "Warning: You haven't synced with nature lately!",
  "Time to physically log out—exit building immediately!",
  "Outdoors: 100% uptime, zero downtime guaranteed!",
  "Notification: Your daily sunshine quota unmet.",
  "Outside: The ultimate bug-free zone!",
  "Critical issue: Your outdoors experience is severely lacking!",
  "Stand up now, unless you've become furniture!",
  "Emergency fix: Your system needs fresh air urgently!",
  "Your monitor voted—take a break and head outside!",
  "You've successfully tested indoors; outdoors is next!",
  "Alert: Vitamin D update required—proceed outdoors!",
  "Low sunshine error—please recharge immediately!",
  "Nature’s calling—time to answer outdoors!",
  "Overdue task: Stepping out and refreshing your brain!",
  "Health check failed: immediate outdoors required!",
  "You've reached your indoor limit—upgrade by going outside!",
  "Your battery needs solar recharging—move outdoors now!",
  "Sprint to the outdoors; it’s good cardio!",
  "Nature misses your lovely face—go visit it!",
  "Outside awaits: your daily reboot starts now!",
  "Vitamin D deficiency detected—outside patch available!",
  "Pro tip: Bugs hate fresh air—go test outdoors!",
  "Forget dark mode; sunlight mode is now active!",
  "Critical system alert: Time to stretch those legs!",
  "Outside is calling. Ignore indoors for a bit!",
  "Update available: Outdoors v2.0—please step outside!",
  "Indoors server maintenance—relocate outside temporarily!",
  "Go outside—your sanity thanks you in advance!",
  "Nature opened a pull request—approve by stepping outdoors!",
  "Sunshine notification: You're overdue for a visit!",
  "Outdoor server ping successful—awaiting your response!",
  "Task reminder: Breathe fresh air—due NOW!",
  "Step outside—manual sanity check in progress!",
  "Your posture requested immediate outdoor recalibration!",
  "Sunlight fix deployed—please manually verify.",
  "Caution: Your indoor streak is dangerously high!",
  "Mandatory outdoor break: deploy yourself now!",
  "Critical outdoors update: please accept immediately!",
  "Low oxygen alert—refill lungs outdoors ASAP!",
  "New real-life event: Get-up-and-go-outside initiated!",
  "Outdoor server reports 0 visitors—fix this now!",
  "Sunshine has requested your immediate presence!",
  "Vitamin D is getting lonely—pay it a visit!",
  "Bug detected: You've become indoor-exclusive!",
  "Fresh air loaded—waiting for you outside!",
  "You've reached maximum indoors—upgrade outdoors!",
  "Notification: outside is feeling neglected by you!",
  "Outdoor validation pending—your input required!",
  "Deploy fresh air to your system now—go outside!",
  "Your chair emailed: 'Please give me a break!'",
  "Human update required: fresh air download available!",
  "Friendly warning: Too much screen, too little green!",
  "Your eyes requested sunlight calibration immediately!",
  "Nature's ticket priority raised to urgent!",
  "Reminder: Outdoors misses your daily visits!",
  "Low sunlight detected—please step outdoors!",
  "Your coffee can't replace oxygen—go outside!",
  "Mandatory patch: get up, walk outside now!",
  "Alert: Risk of becoming furniture. Move immediately!",
  "Fresh air notification—do not snooze!",
  "Outdoor exposure patch ready—deploy yourself now!",
  "Vitamin D needs an update—step outdoors immediately!",
  "Warning: Excessive sitting—deploy legs outdoors!",
  "Fresh air feature launched—experience immediately!",
  "Take your human unit outdoors for a maintenance cycle!",
  "Bug report: Your presence is missing outdoors!",
  "Nature deployment complete—verification required!",
  "Reminder: Grass won't touch itself, go help it!",
  "Stand up, log out, and go outside—it's mandatory!",
  "Update required: immediate fresh-air intake!",
  "Outdoor stability test: please participate now!",
  "Alert: Your health has raised a P0 bug—fix outdoors immediately!"
};


void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  connectWiFi();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    // === Wakeup from Deep Sleep (Button pressed) ===
    Serial.println("🔘 Woke up from deep sleep (button pressed). Sending Discord & OTA...");
    sendDiscordMessage();
    performOTAUpdate();

    Serial.println("♻️ Restarting...");
    ESP.restart();
  } else {
    // === Regular power-up or reboot ===
    Serial.println("⚡️ Regular boot. Awaiting button press for 1 min...");

    unsigned long startWait = millis();
    bool pressed = false;

    while (millis() - startWait < waitDuration) {
      if (digitalRead(buttonPin) == LOW && (millis() - lastPressTime > debounceTime)) {
        Serial.println("🔘 Button pressed during initial wait. Sending Discord & OTA...");
        sendDiscordMessage();
        performOTAUpdate();

        Serial.println("♻️ Restarting...");
        ESP.restart();
      }
      delay(10);
    }

    Serial.println("⏳ 1 min elapsed, entering deep sleep.");
    enterDeepSleep();
  }
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
    Serial.printf("❌ Discord message failed. HTTP code: %d\n", response);
  }

  http.end();
}

// === Perform OTA Update ===
void performOTAUpdate() {
  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("🔄 Checking OTA update...");

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, "1.0");

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
  Serial.println("💤 Deep sleep now...");
  esp_sleep_enable_ext0_wakeup(buttonPin, 0);
  delay(200);
  esp_deep_sleep_start();
}
