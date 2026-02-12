# 💖 LoveBuddy – Interactive Valentine Smart Box

An expressive, interactive smart box powered by **ESP32-C6**, featuring animated OLED eyes and a synchronized WS2812B LED ring.

Designed to be cute, expressive, and emotionally engaging.

---

## ✨ Features

### 👀 Expressive OLED Eyes
- Large blunt-edge rectangle idle eyes
- Natural blinking behavior
- Subtle micro eye movements
- Playful squish animation (Left touch)
- Exaggerated animated heart-shaped eyes (Right touch)
- Full 128×64 display coverage

### 🌈 WS2812B LED Ring (16 LEDs)
- Soft romantic idle breathing glow
- Playful sparkle reaction
- Long synchronized heart pulse
- Smooth, non-blocking animations
- Perfect sync with eye animation states

### 👆 Dual Touch Interaction
- **Left Touch → Playful animation**
- **Right Touch → Love animation**
- Edge-trigger detection (no random retriggering)
- Clean state-based animation control

### ⚡ Non-Blocking Architecture
- Uses `millis()` timing
- State-driven animation engine
- No flicker
- No timing drift
- LED and eye animations are phase-synchronized

---

## 🧠 Animation States

| State     | OLED Eyes                  | LED Ring Effect              |
|------------|---------------------------|------------------------------|
| Idle       | Blunt rectangle eyes      | Soft breathing blush         |
| Playful    | Squish + blink            | Sparkle ripple               |
| Love       | Large animated hearts     | Romantic synchronized pulse  |

---

## 🛠 Hardware Used

- **ESP32-C6**
- **0.96" OLED Display (SSD1306, 128×64, I2C)**
- **WS2812B 16-LED Ring**
- **TTP223 Touch Sensors**
- **DFPlayer Mini**
---

## 🔌 Pin Configuration

| Component     | ESP32-C6 Pin |
|--------------|--------------|
| OLED SDA     | GPIO 6       |
| OLED SCL     | GPIO 7       |
| Touch Left   | GPIO 1       |
| Touch Right  | GPIO 2       |
| WS2812B DIN  | GPIO 18      |
| RX DFPlayer  | GPIO 16      |
| TX DFPlayer  | GPIO 17      |
| Vibration Motor  | GPIO 19      |

---

## 📚 Required Libraries

Install via Arduino Library Manager:

- Adafruit SSD1306
- Adafruit GFX
- Adafruit NeoPixel
- DFRobotDFPlayerMini

---

## 🚀 Getting Started

1. Clone this repository
2. Open `LOVE_BOX_2.ino` in Arduino IDE
3. Select **ESP32-C6** board
4. Install required libraries
5. Upload the code
6. Power the device
7. Tap and enjoy 💕

---

## 🎨 Design Philosophy

This project focuses on:

- Emotional interaction through visuals
- Smooth and synchronized animation
- Clean and expandable firmware structure
- Premium visual aesthetics

---

## 🔮 Future Expansion Ideas

- WiFi + Web configuration panel
- AI-based emotional learning
- Remote interaction support

---

## 📜 License

MIT License – free to modify and build upon.
