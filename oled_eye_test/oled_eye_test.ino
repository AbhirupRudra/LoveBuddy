#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET    -1
#define OLED_ADDR     0x3C

// Change pins if needed
#define I2C_SDA 6
#define I2C_SCL 7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Eye parameters
int eyeX = 64;
int eyeY = 32;
int eyeW = 36;
int eyeH = 20;

void drawEye(int h) {
  display.clearDisplay();

  // Left eye
  display.fillRoundRect(32 - eyeW / 2, eyeY - h / 2, eyeW, h, 8, SSD1306_WHITE);
  // Right eye
  display.fillRoundRect(96 - eyeW / 2, eyeY - h / 2, eyeW, h, 8, SSD1306_WHITE);

  display.display();
}

void blink() {
  // Close
  for (int h = eyeH; h > 2; h -= 3) {
    drawEye(h);
    delay(20);
  }
  delay(80);
  // Open
  for (int h = 2; h <= eyeH; h += 3) {
    drawEye(h);
    delay(20);
  }
}

void lookSide(int offset) {
  display.clearDisplay();

  display.fillRoundRect(32 - eyeW / 2 + offset, eyeY - eyeH / 2, eyeW, eyeH, 8, SSD1306_WHITE);
  display.fillRoundRect(96 - eyeW / 2 + offset, eyeY - eyeH / 2, eyeW, eyeH, 8, SSD1306_WHITE);

  display.display();
}

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (1); // OLED not found
  }

  display.clearDisplay();
  drawEye(eyeH);
}

void loop() {
  blink();
  delay(300);

  lookSide(-6);  // look left
  delay(400);

  blink();
  delay(200);

  lookSide(6);   // look right
  delay(400);

  drawEye(eyeH); // center
  delay(600);
}