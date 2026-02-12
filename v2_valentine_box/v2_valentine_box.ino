/* ================ Libraries ================ */
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <Wire.h>
#include <DFRobotDFPlayerMini.h>

/* ================ PINS ================ */
#define OLED_SDA 6
#define OLED_SCL 7

#define touchPin1 1
#define touchPin2 2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define TOUCH_PN1  1
#define TOUCH_PIN2 2

#define OLED_LESET     -1

/* ================ Libraries Call ================ */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_LESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini dfPlayer;

/* ================ Variables ================ */
enum EyeSelect {
  EYE_LEFT,
  EYE_RIGHT,
  EYE_BOTH
};

bool eyeBlinkActive = false;
EyeSelect activeEye;
unsigned long eyeBlinkStart = 0;
const unsigned long EYE_BLINK_TIME = 120; // ms

bool lastTouchState = false;

#define DOUBLE_TAP_TIME 350

bool lastTouch = false;
bool waitingSecondTap = false;

unsigned long firstTapTime = 0;

uint8_t currentTrack = 1;
uint8_t currentFolder = 1;
uint8_t totalTracks  = 1;
bool sequenceMode    = false;
bool playing         = false;

void setup() {
  Serial.begin(9600);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);

  roboEyes.setWidth(30, 30);
  roboEyes.setHeight(36, 36);
  roboEyes.setBorderradius(8, 8);
  roboEyes.setSpacebetween(10);
  roboEyes.setCyclops(false);
  roboEyes.setAutoblinker(true, 5, 3);

  pinMode(TOUCH_PN1, INPUT);
  pinMode(TOUCH_PIN2, INPUT);

  dfSerial.begin(9600, SERIAL_8N1, 17, 16);
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer init failed");
  } else {
    dfPlayer.volume(30);   // 0–30
  }

  playSequence(2, 1);
}


void loop() {
  roboEyes.update();
  handleDFPlayer();
  handleEyeBlink();
  handleTouchMusicControl();
  handleTouchBlink();
}


void blinkEye(EyeSelect eye) {
  if (eyeBlinkActive) return;

  eyeBlinkActive = true;
  activeEye = eye;
  eyeBlinkStart = millis();

  if (eye == EYE_LEFT)      roboEyes.close(1, 0);
  else if (eye == EYE_RIGHT) roboEyes.close(0, 1);
  else                       roboEyes.close();
}

void handleEyeBlink() {
  if (!eyeBlinkActive) return;

  if (millis() - eyeBlinkStart >= EYE_BLINK_TIME) {
    roboEyes.open();
    eyeBlinkActive = false;
  }
}

void handleTouchBlink() {
  bool touchNow = digitalRead(TOUCH_PN1);

  // Rising edge detection (NEW touch)
  if (touchNow && !lastTouchState) {
    blinkEye(EYE_RIGHT);   // change to EYE_LEFT if you want
  }

  lastTouchState = touchNow;
}

void playTrack(uint8_t folder, uint8_t track) {
  sequenceMode = false;
  currentTrack = track;
  currentFolder = folder;
  dfPlayer.playFolder(folder, track);
  playing = true;
}

void playSequence(uint8_t folder, uint8_t trackCount) {
  totalTracks = trackCount;
  currentTrack = 1;
  currentFolder = folder;
  sequenceMode = true;
  dfPlayer.playFolder(folder, currentTrack);
  playing = true;
}

void stopPlayback() {
  dfPlayer.stop();
  playing = false;
  sequenceMode = false;
}

void handleDFPlayer() {
  if (!dfPlayer.available()) return;

  uint8_t type = dfPlayer.readType();
  uint8_t folder = currentFolder;
  if (type == DFPlayerPlayFinished && sequenceMode && playing) {
    currentTrack++;

    if (currentTrack > totalTracks) {
      currentTrack = 1;   // restart from beginning
    }

    dfPlayer.playFolder(folder, currentTrack);
  }
}

void handleTouchMusicControl() {
  bool touchNow = digitalRead(TOUCH_PIN2);
  unsigned long now = millis();

  // Rising edge detection
  if (touchNow && !lastTouch) {

    if (!waitingSecondTap) {
      // First tap detected
      waitingSecondTap = true;
      firstTapTime = now;
    } else {
      // Second tap within time → DOUBLE TAP
      if (now - firstTapTime <= DOUBLE_TAP_TIME) {
        nextTrack();
        waitingSecondTap = false;
      }
    }
  }

  // Timeout → SINGLE TAP confirmed
  if (waitingSecondTap && (now - firstTapTime > DOUBLE_TAP_TIME)) {
    togglePlayStop();
    waitingSecondTap = false;
  }

  lastTouch = touchNow;
}

void togglePlayStop() {
  if (playing) {
    stopPlayback();
  } else {
    dfPlayer.playFolder(currentFolder, currentTrack);
    playing = true;
  }
}

void nextTrack() {
  if (!playing) return;

  currentTrack++;
  if (currentTrack > totalTracks) {
    currentTrack = 1;
  }
  dfPlayer.playFolder(currentFolder, currentTrack);
}
