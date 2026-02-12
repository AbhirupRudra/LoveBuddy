#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ================= OLED ================= */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define OLED_SDA 6
#define OLED_SCL 7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ================= TOUCH ================= */
#define TOUCH_LEFT  1
#define TOUCH_RIGHT 2

/* ================= STATE ================= */
unsigned long lastBlink = 0;
int eyeOffset = 0;

/* ================= SETUP ================= */
void setup() {
  pinMode(TOUCH_LEFT, INPUT);
  pinMode(TOUCH_RIGHT, INPUT);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (1);
  }

  display.clearDisplay();
  drawIdleEyes(0);
}

/* ================= LOOP ================= */
void loop() {
  if (digitalRead(TOUCH_LEFT) == HIGH) {
    playfulAnimation();
    delay(300);
  }

  if (digitalRead(TOUCH_RIGHT) == HIGH) {
    loveAnimation();
    delay(300);
  }

  idleBehavior();
}

/* ================= IDLE ================= */
void idleBehavior() {
  unsigned long now = millis();

  if (now - lastBlink > random(3000, 6000)) {
    blink();
    lastBlink = now;
  }

  if (random(0, 1000) < 5) {
    eyeOffset = random(-2, 3);
    drawIdleEyes(eyeOffset);
  }
}

/* ================= DRAW EYES ================= */
void drawIdleEyes(int offset) {
  display.clearDisplay();

  // LEFT EYE
  display.fillRoundRect(12 + offset, 18, 44, 28, 6, SSD1306_WHITE);
  // RIGHT EYE
  display.fillRoundRect(72 + offset, 18, 44, 28, 6, SSD1306_WHITE);

  display.display();
}

void blink() {
  display.clearDisplay();
  display.fillRect(12, 32, 44, 4, SSD1306_WHITE);
  display.fillRect(72, 32, 44, 4, SSD1306_WHITE);
  display.display();
  delay(120);
  drawIdleEyes(eyeOffset);
}

/* ================= PLAYFUL ================= */
void playfulAnimation() {
  // squish
  for (int i = 0; i < 6; i++) {
    display.clearDisplay();
    display.fillRoundRect(12 + i, 22, 44 - 2*i, 20, 6, SSD1306_WHITE);
    display.fillRoundRect(72 + i, 22, 44 - 2*i, 20, 6, SSD1306_WHITE);
    display.display();
    delay(30);
  }

  blink();
  drawIdleEyes(0);
}

/* ================= LOVE ================= */
void loveAnimation() {
  for (int pulse = 0; pulse < 2; pulse++) {
    display.clearDisplay();

    drawHeart(40, 32, 18 + pulse * 3);
    drawHeart(88, 32, 18 + pulse * 3);

    display.display();
    delay(300);
  }

  drawIdleEyes(0);
}

/* ================= HEART ================= */
void drawHeart(int x, int y, int s) {
  display.fillCircle(x - s/2, y - s/3, s/2, SSD1306_WHITE);
  display.fillCircle(x + s/2, y - s/3, s/2, SSD1306_WHITE);
  display.fillTriangle(
    x - s, y - s/4,
    x + s, y - s/4,
    x, y + s,
    SSD1306_WHITE
  );
}
