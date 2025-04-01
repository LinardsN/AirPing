# 🟢 AirPing – ESP32 Fresh Air Button

**AirPing** is a battery-powered ESP32 button that sends messages to a Discord channel when pressed. Use it in the office to call teammates outside, announce coffee breaks, or lunch time — all from a physical button.

---

## ✨ Features

- 🔘 **1 press** → Sends a random "call outside" message
- ☕ **2 presses** → Coffee time message
- 🍔 **3 presses** → Lunch time message *(available only 11:30–12:30 Riga time)*
- 📅 **Cooldowns**:
  - 1 press = 15 min
  - 2 presses = 10 min
  - 3 presses = 30 min
- 🕒 **Real-time clock** syncing via NTP
- 🔧 **Maintenance mode**:
  - Trigger by holding button at power-on
  - Automatically fetch messages from GitHub and check for OTA update
- 📦 **OTA updates**:
  - Automatically check version from GitHub
  - Update firmware if version has changed
- 📟 All serial monitor logs mirrored to a private Discord webhook (for maintenance/debug)

---

## 🔋 Power Requirements

- Designed for battery use (6V or 3.3–3.7V with proper wiring)
- Enters **deep sleep** after use to conserve power
- Estimated runtime:
  - ~10+ days with 800mAh battery
  - Extendable with power optimizations (cut PWR LED, OLED shutdown, etc.)

---

## 🛠️ Hardware

| Component       | Purpose               |
|----------------|------------------------|
| ESP32 Dev Board| Main microcontroller   |
| Button (GPIO 13)| Input for press detection |
| (Optional) OLED| Display messages        |
| Battery         | 6V or 3.7V Li-ion recommended |

**Wiring:**

- **Button** → GPIO 13 with `INPUT_PULLUP`
- (Optional) **OLED** → I2C (SDA = GPIO 21, SCL = GPIO 22)

---

## 🔁 Press Behavior

| Action          | Description                        |
|-----------------|------------------------------------|
| 1 press         | Sends random message from list     |
| 2 presses       | Sends fixed coffee message ☕       |
| 3 presses       | Sends fixed lunch message 🍔 *(11:30–12:30 only)*

---

## 🌐 Webhooks & GitHub Integration

- `webhookUrl` → Public message Discord channel
- `maintenanceWebhookUrl` → Logs and debug channel (private)
- `firmwareUrl` → Hosted `.bin` for OTA updates
- `remoteVersionUrl` → Hosted `.txt` file containing latest firmware version

---

## 🔧 Maintenance Mode

Hold button while powering on:
- Downloads new messages from GitHub
- Saves them to ESP32 flash
- Runs OTA update if version has changed

---

## 💾 Flashing & Customization

- Use Arduino IDE or PlatformIO
- Set your Wi-Fi credentials and Discord webhook URLs in the config section
- Manually bump `firmwareVersion` string and update `version.txt` on GitHub when releasing new code


---

## 🤝 Credits

Project by [Linards](https://github.com/LinardsN)  
Powered by ESP32 and a whole lot of ☀️ fresh air

---





