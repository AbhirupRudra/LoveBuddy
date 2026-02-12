#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>

/* ================= OLED ================= */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_SDA 6
#define OLED_SCL 7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* =================  ================= */
/* ================= TOUCH ================= */
#define TOUCH_LEFT  1
#define TOUCH_RIGHT 2

bool lastLeftState  = false;
bool lastRightState = false;

/* ================= LED ================= */
#define LED_PIN 18
#define LED_COUNT 16
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/* ================= STATES ================= */
enum AnimState { IDLE, PLAYFUL, LOVE };
AnimState animState = IDLE;

unsigned long stateStartTime = 0;

/* ================= IDLE ================= */
unsigned long lastBlink = 0;
int eyeOffset = 0;
unsigned long ledTimer = 0;
int breathe = 6;
bool breatheUp = true;

/* ================= SETUP ================= */
void setup() {
  pinMode(TOUCH_LEFT, INPUT_PULLDOWN);
  pinMode(TOUCH_RIGHT, INPUT_PULLDOWN);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  drawIdleEyes(0);

  ring.begin();
  ring.setBrightness(60);
  ring.show();
}

/* ================= LOOP ================= */
void loop() {
  handleTouch();
  runAnimation();
}

/* ================= TOUCH ================= */
void handleTouch() {
  bool leftNow  = digitalRead(TOUCH_LEFT);
  bool rightNow = digitalRead(TOUCH_RIGHT);

  if (leftNow && !lastLeftState && animState == IDLE) {
    animState = PLAYFUL;
    stateStartTime = millis();
  }

  if (rightNow && !lastRightState && animState == IDLE) {
    animState = LOVE;
    stateStartTime = millis();
  }

  lastLeftState  = leftNow;
  lastRightState = rightNow;
}

/* ================= ANIMATION ENGINE ================= */
void runAnimation() {
  unsigned long now = millis();

  switch (animState) {

    case IDLE:
      idleEyes();
      idleLED();
      break;

    case PLAYFUL:
      playfulEyes(now - stateStartTime);
      playfulLED(now - stateStartTime);
      if (now - stateStartTime > 700) {
        animState = IDLE;
        drawIdleEyes(0);
      }
      break;

    case LOVE:
      loveEyes(now - stateStartTime);
      loveLED(now - stateStartTime);
      if (now - stateStartTime > 3500) {
        animState = IDLE;
        drawIdleEyes(0);
      }
      break;
  }
}

/* ================= IDLE ================= */
void idleEyes() {
  unsigned long now = millis();

  if (now - lastBlink > random(3500, 6000)) {
    blink();
    lastBlink = now;
  }

  if (random(0, 1200) < 4) {
    eyeOffset = random(-2, 3);
    drawIdleEyes(eyeOffset);
  }
}

void idleLED() {
  unsigned long now = millis();
  if (now - ledTimer > 40) {
    breathe += breatheUp ? 1 : -1;
    if (breathe >= 30) breatheUp = false;
    if (breathe <= 6)  breatheUp = true;

    for (int i = 0; i < LED_COUNT; i++) {
      ring.setPixelColor(i, ring.Color(breathe * 4, breathe * 2, breathe * 3));
    }
    ring.show();
    ledTimer = now;
  }
}

/* ================= PLAYFUL ================= */
void playfulEyes(unsigned long t) {
  int squish = min(6, (int)(t / 40));
  display.clearDisplay();
  display.fillRoundRect(12 + squish, 22, 44 - squish*2, 20, 6, SSD1306_WHITE);
  display.fillRoundRect(72 + squish, 22, 44 - squish*2, 20, 6, SSD1306_WHITE);
  display.display();
}

void playfulLED(unsigned long t) {
  int sparkle = (t / 80) % LED_COUNT;
  ring.clear();
  ring.setPixelColor(sparkle, ring.Color(255, 160, 210));
  ring.show();
}

/* ================= LOVE ================= */
void loveEyes(unsigned long t) {
  int pulse = 20 + 4 * sin(t * 0.005);
  display.clearDisplay();
  drawHeart(40, 32, pulse);
  drawHeart(88, 32, pulse);
  display.display();
}

void loveLED(unsigned long t) {
  float phase = (sin(t * 0.004) + 1.0) / 2.0;
  int r = 200 + 55 * phase;
  int g = 80  + 40 * phase;
  int b = 140 + 60 * phase;

  for (int i = 0; i < LED_COUNT; i++) {
    ring.setPixelColor(i, ring.Color(r, g, b));
  }
  ring.show();
}

/* ================= EYES ================= */
void drawIdleEyes(int offset) {
  display.clearDisplay();
  display.fillRoundRect(12 + offset, 18, 44, 28, 6, SSD1306_WHITE);
  display.fillRoundRect(72 + offset, 18, 44, 28, 6, SSD1306_WHITE);
  display.display();
}

void blink() {
  display.clearDisplay();
  display.fillRect(12, 32, 44, 4, SSD1306_WHITE);
  display.fillRect(72, 32, 44, 4, SSD1306_WHITE);
  display.display();
  delay(90);
  drawIdleEyes(eyeOffset);
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

/* ================= DFPlayer ================= */
void playSound(int f, int n)
{

}