

/* =========== Libraries =========== */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobotDFPlayerMini.h>
#include <Adafruit_NeoPixel.h>

/* =========== Pins =========== */
#define OLED_SDA 6
#define OLED_SCL 7

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define MAG_PIN 2
#define BUSY 21

#define LED_PIN 18
#define NUM_LEDS 10
#define VIB_PIN 19

/* =========== Objects =========== */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini dfPlayer;

/* =========== State =========== */
bool musicPlayingStart = false;
bool music2PlayingStart = false;
bool second = false;
bool magState_ON = false;
bool first = false;
unsigned long last;
bool start = false;

/* =========== Hearts =========== */
#define HEART_COUNT 30

struct Heart {
  int x, y;
  uint8_t size, speed;
};

Heart hearts[HEART_COUNT];

unsigned long lastFrame = 0;
const unsigned long frameInterval = 40;
unsigned long lastTextFrame = 0;

bool heartbeatEnabled = false;

unsigned long hbStartTime = 0;

// timings (ms)
const uint16_t BEAT_INTERVAL = 900;
const uint16_t DECAY_TIME   = 250;

// brightness
const uint8_t PEAK_BRIGHT   = 255;
const uint8_t MIN_BRIGHT    = 30;

// motor pulse time
const uint16_t MOTOR_PULSE  = 80;

void heartbeatEffect(unsigned long now) {

  if (!heartbeatEnabled) {
    pixels.clear();
    pixels.show();
    digitalWrite(VIB_PIN, LOW);
    return;
  }

  if (hbStartTime == 0) hbStartTime = now;

  unsigned long t = (now - hbStartTime) % BEAT_INTERVAL;

  uint8_t brightness = MIN_BRIGHT;
  bool motorOn = false;

  // ---- LUB (sudden peak) ----
  if (t < 10) {
    brightness = PEAK_BRIGHT;
    motorOn = true;
  }
  // ---- LUB decay ----
  else if (t < DECAY_TIME) {
    brightness = map(t, 10, DECAY_TIME, PEAK_BRIGHT, MIN_BRIGHT);
  }
  // ---- DUB (sudden peak) ----
  else if (t >= DECAY_TIME + 120 && t < DECAY_TIME + 130) {
    brightness = PEAK_BRIGHT;
    motorOn = true;
  }
  // ---- DUB decay ----
  else if (t < DECAY_TIME + 350) {
    brightness = map(t, DECAY_TIME + 130, DECAY_TIME + 350,
                     PEAK_BRIGHT, MIN_BRIGHT);
  }

  // apply LED
  pixels.fill(pixels.Color(brightness, 0, 0));
  pixels.show();

  // motor control (pulse only)
  static unsigned long motorTimer = 0;
  if (motorOn) {
    digitalWrite(VIB_PIN, HIGH);
    motorTimer = now;
  }
  if (digitalRead(VIB_PIN) == HIGH && now - motorTimer > MOTOR_PULSE) {
    digitalWrite(VIB_PIN, LOW);
  }
}


/* =========== Heart Draw =========== */
void drawHeart(int x, int y, uint8_t s) {
  display.fillCircle(x - s, y, s, SSD1306_WHITE);
  display.fillCircle(x + s, y, s, SSD1306_WHITE);
  display.fillTriangle(
    x - (2 * s), y,
    x + (2 * s), y,
    x, y + (3 * s),
    SSD1306_WHITE
  );
}

/* =========== Setup =========== */
void setup() {
  Serial.begin(9600);

  pinMode(MAG_PIN, INPUT_PULLUP);
  pinMode(VIB_PIN, OUTPUT);
  pinMode(BUSY, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  dfSerial.begin(9600, SERIAL_8N1, 17, 16);
  if (dfPlayer.begin(dfSerial)) {
    dfPlayer.volume(25);
  }

  randomSeed(analogRead(0));

  for (int i = 0; i < HEART_COUNT; i++) {
    hearts[i] = {
      random(10, SCREEN_WIDTH - 10),
      random(SCREEN_HEIGHT),
      random(2, 4),
      random(1, 3)
    };
  }
  display.clearDisplay();
  display.display();
}

/* =========== Loop =========== */
void loop() {
  unsigned long now = millis();
  heartbeatEffect(now);

  if (now - last >= 200 && digitalRead(MAG_PIN) == LOW && !first && !start)
  {
    heartbeatEnabled = true;
    musicPlayingStart = true;
    first = true;
    last = millis();
    start = true;
  }

if (musicPlayingStart == true && start == true)
    lovePatterns(now);

  if (now - last >= 20000 && !music2PlayingStart && start == true){
    display.clearDisplay();
    display.display();
    showCenteredText();
    musicPlayingStart = false;
    music2PlayingStart = true;
  }

  if (now-last >= 60000 && start == true)
  {
    display.clearDisplay();
    display.display();
    heartbeatEnabled = false;
    first = false;
    music2PlayingStart = false;
    start = false;
  }
}

/* =========== Love Animation (NON-BLOCKING) =========== */
void lovePatterns(unsigned long now) {
  if (now - lastFrame < frameInterval) return;
  lastFrame = now;

  display.clearDisplay();

  for (int i = 0; i < HEART_COUNT; i++) {
    drawHeart(hearts[i].x, hearts[i].y, hearts[i].size);
    hearts[i].y -= hearts[i].speed;

    if (hearts[i].y < -10) {
      hearts[i].y = SCREEN_HEIGHT + random(10, 30);
      hearts[i].x = random(10, SCREEN_WIDTH - 10);
      hearts[i].size = random(2, 4);
      hearts[i].speed = random(1, 3);
    }
  }

  display.display();
}

void oledText(const char* text, int x = 0, int y = 0, uint8_t size = 1) {
  display.clearDisplay();
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.println(text);
  display.display();
}

void showCenteredText() {
  display.clearDisplay();
  display.setTextSize(2);                 // medium size
  display.setTextColor(SSD1306_WHITE);

  const char* lines[] = {
    "Will You",
    "Be My",
    "Valentine"
  };

  const uint8_t lineCount = 3;

  int16_t x1, y1;
  uint16_t w, h;

  // height of one line
  display.getTextBounds("Test", 0, 0, &x1, &y1, &w, &h);

  int totalTextHeight = lineCount * h + (lineCount - 1) * 2;
  int startY = (SCREEN_HEIGHT - totalTextHeight) / 2;

  for (int i = 0; i < lineCount; i++) {
    display.getTextBounds(lines[i], 0, 0, &x1, &y1, &w, &h);
    int x = (SCREEN_WIDTH - w) / 2;
    int y = startY + i * (h + 2);

    display.setCursor(x, y);
    display.println(lines[i]);
  }

  display.display();
}
