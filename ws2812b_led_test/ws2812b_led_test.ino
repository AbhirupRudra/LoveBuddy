#include <Adafruit_NeoPixel.h>

#define LED_PIN    18
#define LED_COUNT  16

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // clear
  randomSeed(micros());
}

void loop() {
  int mode = random(0, 4);

  switch (mode) {
    case 0: randomFill(); break;
    case 1: chase(); break;
    case 2: sparkle(); break;
    case 3: rainbowSpin(); break;
  }
}

// ---------- MODES ----------

void randomFill() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(
      i,
      random(0, 255),
      random(0, 255),
      random(0, 255)
    );
  }
  strip.setBrightness(random(20, 150));
  strip.show();
  delay(500);
}

void chase() {
  strip.clear();
  int c = strip.Color(random(255), random(255), random(255));

  for (int i = 0; i < LED_COUNT; i++) {
    strip.clear();
    strip.setPixelColor(i, c);
    strip.show();
    delay(60);
  }
}

void sparkle() {
  strip.clear();
  for (int i = 0; i < 10; i++) {
    int p = random(LED_COUNT);
    strip.setPixelColor(p, 255, 255, 255);
    strip.show();
    delay(50);
    strip.setPixelColor(p, 0, 0, 0);
  }
}

void rainbowSpin() {
  for (int j = 0; j < 256; j += 8) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, wheel((i * 16 + j) & 255));
    }
    strip.show();
    delay(40);
  }
}

// ---------- COLOR WHEEL ----------

uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return strip.Color(pos * 3, 255 - pos * 3, 0);
}
