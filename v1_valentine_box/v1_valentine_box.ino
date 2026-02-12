/* LoveBuddy Firmware for ESP32-C6
   - OLED eyes (SSD1306)
   - WS2812 ring (16)
   - Vibration motor via transistor
   - DFPlayer Mini audio via Serial2
   - TTP223 touch sensors
   - DHT11 sensor
   - On-device AI: emotion, learning, memory
   - Local WiFi AP + WebServer for config & reminders

   Libraries required:
   - Adafruit SSD1306
   - Adafruit GFX
   - Adafruit NeoPixel
   - DFRobotDFPlayerMini
   - DHT
   - Preferences (built-in)
   - WebServer (built-in)
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <DFRobotDFPlayerMini.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ---------- CONFIG ----------
#define OLED_SDA_PIN 6
#define OLED_SCL_PIN 7
#define LED_PIN      18
#define NUM_LEDS     16

#define TOUCH_PIN1   1
#define TOUCH_PIN2   2  // optional second touch
#define VIB_PIN      19

#define DF_RX_PIN    16  // ESP TX -> DFPlayer RX
#define DF_TX_PIN    17  // ESP RX <- DFPlayer TX

#define DHT_PIN      20
#define DHT_TYPE     DHT11

#define AP_SSID      "LoveBuddy-Setup"
#define AP_PWD       "loveyou123"  // change if you want

#define OLED_ADDR    0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ---------- GLOBAL OBJECTS ----------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

DHT dht(DHT_PIN, DHT_TYPE);

HardwareSerial DFSerial(2); // Serial2 on ESP32 (use pins configured)
DFRobotDFPlayerMini dfplayer;

WebServer server(80);
Preferences prefs;

// ---------- TIMERS & STATE ----------
unsigned long lastBlinkMillis = 0;
unsigned long lastIdleLEDMillis = 0;
unsigned long lastHeartbeatMillis = 0;
unsigned long lastTouchMillis = 0;
unsigned long lastDHTread = 0;
unsigned long lastReminderCheck = 0;

bool eyesOpen = true;
int eyeOffsetX = 0;

// Touch handling
bool touchState1 = false;
bool touchState2 = false;
unsigned long touchDownTime = 0;
int tapCount = 0;
unsigned long lastTapTime = 0;

// AI/Memory
struct Memory {
  char name[32];
  bool learningEnabled;
  int touchCountToday;
  unsigned long totalTouchDuration; // ms
  unsigned long lastActive; // epoch millis
} memoryData;

// Reminders (simple fixed-size)
#define MAX_REMINDERS 6
struct Reminder {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t mode; // 0=vibrate,1=audio,2=both
  char text[64];
};
Reminder reminders[MAX_REMINDERS];

// Emotion states
enum Emotion { EM_HAPPY, EM_CALM, EM_SHY, EM_LONELY, EM_SLEEPY, EM_EXCITED };
Emotion curEmotion = EM_CALM;
Emotion prevEmotion = EM_CALM;

// DHT derived zones
int tempZone = 1; // 0=cold,1=comfortable,2=hot
int humidZone = 1; // 0=dry,1=ok,2=humid

// ---------- CONFIGURABLES ----------
int LED_BRIGHTNESS = 40; // cap overall, 0-255
int BLUSH_MAX = 90;      // for internal scaling of color strength

// ---------- FORWARD DECLARATIONS ----------
void setupWiFiAP();
void setupWebServer();
void handleRoot();
void handleSaveSettings();
void handleAddReminder();
void handleDeleteReminder();
void loadPreferences();
void savePreferences();
void playAudioTrack(uint16_t track);
void setLEDColorHSV(uint8_t h, uint8_t s, uint8_t v);
void setAllLEDColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
void updateDHT();
void aiUpdate();
void drawEyes(Emotion e, bool blinkFrame);
void idleEyeBehavior();
void ledIdleBehavior();
void heartbeatBehavior();
void doDailySpecial();
void processTouch();
void schedulerCheckReminders();
void performReminderAction(const Reminder &r);

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(100);

  // Pins
  pinMode(TOUCH_PIN1, INPUT);
  pinMode(TOUCH_PIN2, INPUT);
  pinMode(VIB_PIN, OUTPUT);
  digitalWrite(VIB_PIN, LOW);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();

  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);
  pixels.clear();
  pixels.show();

  dht.begin();

  // DFPlayer using Serial2 (DFSerial)
  DFSerial.begin(9600, SERIAL_8N1, DF_TX_PIN, DF_RX_PIN); // note: Serial2 pins swapped: (txPin, rxPin) in begin
  if (!dfplayer.begin(DFSerial)) {
    Serial.println("DFPlayer not found!");
    // continue - audio optional
  } else {
    dfplayer.setTimeOut(500);
    dfplayer.volume(20); // 0-30
  }

  // Preferences load
  prefs.begin("lovebuddy", false);
  loadPreferences();

  // WiFi AP and webserver
  setupWiFiAP();
  setupWebServer();

  // Initial display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("LoveBuddy booting...");
  display.display();

  lastBlinkMillis = millis();
  lastIdleLEDMillis = millis();
  lastHeartbeatMillis = millis();
  lastDHTread = 0;
}

// ---------- MAIN LOOP ----------
void loop() {
  server.handleClient();

  unsigned long now = millis();

  // read touch and update touch logic
  processTouch();

  // periodic sensor reads
  if (now - lastDHTread > 15000) { // every 15s
    updateDHT();
    lastDHTread = now;
  }

  // AI update (state transitions)
  aiUpdate();

  // Output behaviors: eyes, LED, heartbeat
  idleEyeBehavior();
  ledIdleBehavior();
  heartbeatBehavior();

  // Reminders check every 30s
  if (now - lastReminderCheck > 30000) {
    schedulerCheckReminders();
    lastReminderCheck = now;
  }

  // Minor delay for CPU relief (non-blocking overall)
  delay(10);
}

// ---------- TOUCH PROCESSING (tap, long press, multi-tap) ----------
void processTouch() {
  unsigned long now = millis();
  bool t1 = digitalRead(TOUCH_PIN1) == HIGH;
  bool t2 = digitalRead(TOUCH_PIN2) == HIGH;

  // Primary touch (touch1) logic
  if (t1 && !touchState1) {
    // pressed
    touchState1 = true;
    touchDownTime = now;
  }
  if (!t1 && touchState1) {
    // released
    unsigned long duration = now - touchDownTime;
    memoryData.touchCountToday++;
    memoryData.totalTouchDuration += duration;
    memoryData.lastActive = now;

    // classify
    if (duration > 1500) {
      // long touch -> shy/comfort
      curEmotion = EM_SHY;
      // heartbeat + audio
      digitalWrite(VIB_PIN, HIGH);
      delay(60);
      digitalWrite(VIB_PIN, LOW);
      playAudioTrack(2); // assume track 2 = shy shayari
    } else {
      // tap
      // multi-tap detection
      if (now - lastTapTime < 700) {
        tapCount++;
      } else {
        tapCount = 1;
      }
      lastTapTime = now;

      if (tapCount >= 3) {
        curEmotion = EM_EXCITED;
        // excited reaction
        digitalWrite(VIB_PIN, HIGH);
        delay(40);
        digitalWrite(VIB_PIN, LOW);
        playAudioTrack(3); // giggle
        tapCount = 0;
      } else {
        curEmotion = EM_HAPPY;
        // small thump + small blush
        digitalWrite(VIB_PIN, HIGH);
        delay(30);
        digitalWrite(VIB_PIN, LOW);
        playAudioTrack(1); // small chime/shayari
      }
    }
    touchState1 = false;
  }

  // Optional touch2 -> alternate function (quick toggle learning mode)
  if (t2 && !touchState2) {
    touchState2 = true;
    touchDownTime = now;
  }
  if (!t2 && touchState2) {
    unsigned long duration = now - touchDownTime;
    if (duration > 2000) {
      // long press second touch toggles learning
      memoryData.learningEnabled = !memoryData.learningEnabled;
      // short feedback
      for (int i=0;i<2;i++){
        digitalWrite(VIB_PIN, HIGH); delay(30); digitalWrite(VIB_PIN, LOW); delay(50);
      }
    } else {
      // short press: daily special
      doDailySpecial();
    }
    touchState2 = false;
  }
}

// ---------- DHT read & interpret ----------
void updateDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("DHT read failed");
    return;
  }
  // interpret temp zones
  if (t < 20.0) tempZone = 0;
  else if (t <= 30.0) tempZone = 1;
  else tempZone = 2;
  // humidity
  if (h < 40.0) humidZone = 0;
  else if (h <= 70.0) humidZone = 1;
  else humidZone = 2;

  Serial.printf("Temp: %.1fC Hum: %.1f%% zones %d %d\n", t,h,tempZone,humidZone);
}

// ---------- AI update: simple behavioral heuristics ----------
void aiUpdate() {
  // base mood influenced by time of day (use local time from WiFi if configured)
  // For now use millis-based day wrap or Preferences stored hour if web set; simple heuristic:
  unsigned long now = millis();

  // If last active > threshold -> lonely/sad
  if ((now - memoryData.lastActive) > 6L*60L*60L*1000L) { // 6 hours
    curEmotion = EM_LONELY;
  }
  // Temperature influence
  if (tempZone == 2) { // hot
    if (curEmotion != EM_SLEEPY) curEmotion = EM_CALM;
  } else if (tempZone == 0) { // cold
    if (curEmotion != EM_EXCITED) curEmotion = EM_EXCITED;
  }

  // If learning disabled, cap changing
  if (!memoryData.learningEnabled) {
    // minor damping (hold previous state)
    curEmotion = prevEmotion;
  }

  // Store prev for small hysteresis
  prevEmotion = curEmotion;
}

// ---------- Eye drawing helpers ----------
void drawEyes(Emotion e, bool blinkFrame) {
  display.clearDisplay();

  // Basic coordinates
  int leftX = 36 + eyeOffsetX;
  int rightX = 80 + eyeOffsetX;
  int eyeY = 28;

  // base shapes depend on emotion
  switch (e) {
    case EM_HAPPY:
      // open round eyes with small pupil
      display.fillRoundRect(leftX, eyeY, 18, 12, 6, SSD1306_WHITE);
      display.fillRoundRect(rightX, eyeY, 18, 12, 6, SSD1306_WHITE);
      display.fillCircle(leftX+9, eyeY+6, 2, SSD1306_BLACK);
      display.fillCircle(rightX+9, eyeY+6, 2, SSD1306_BLACK);
      break;
    case EM_SHY:
      // half closed with blush hint (we do glance)
      display.fillRoundRect(leftX, eyeY+3, 18, 6, 6, SSD1306_WHITE);
      display.fillRoundRect(rightX, eyeY+3, 18, 6, 6, SSD1306_WHITE);
      display.fillCircle(leftX+9, eyeY+6, 1, SSD1306_BLACK);
      display.fillCircle(rightX+9, eyeY+6, 1, SSD1306_BLACK);
      break;
    case EM_SLEEPY:
      // thin lines
      display.fillRect(leftX, eyeY+6, 18, 2, SSD1306_WHITE);
      display.fillRect(rightX, eyeY+6, 18, 2, SSD1306_WHITE);
      break;
    case EM_LONELY:
      // slightly droopy eyes
      display.drawRoundRect(leftX, eyeY, 18, 12, 6, SSD1306_WHITE);
      display.drawRoundRect(rightX, eyeY, 18, 12, 6, SSD1306_WHITE);
      display.fillRect(leftX+7, eyeY+6, 4, 4, SSD1306_BLACK);
      display.fillRect(rightX+7, eyeY+6, 4, 4, SSD1306_BLACK);
      break;
    case EM_EXCITED:
      // sparkle eyes (two white blocks)
      display.fillRoundRect(leftX, eyeY, 18, 14, 6, SSD1306_WHITE);
      display.fillRoundRect(rightX, eyeY, 18, 14, 6, SSD1306_WHITE);
      display.fillCircle(leftX+9, eyeY+5, 3, SSD1306_BLACK);
      display.fillCircle(rightX+9, eyeY+5, 3, SSD1306_BLACK);
      display.fillRect(leftX+2, eyeY+1, 3, 3, SSD1306_BLACK);
      display.fillRect(rightX+2, eyeY+1, 3, 3, SSD1306_BLACK);
      break;
    case EM_CALM:
    default:
      display.fillRoundRect(leftX, eyeY, 18, 12, 6, SSD1306_WHITE);
      display.fillRoundRect(rightX, eyeY, 18, 12, 6, SSD1306_WHITE);
      display.fillCircle(leftX+9, eyeY+6, 2, SSD1306_BLACK);
      display.fillCircle(rightX+9, eyeY+6, 2, SSD1306_BLACK);
      break;
  }

  // blink animation overlay
  if (blinkFrame) {
    // draw eyelid
    display.fillRect(leftX, eyeY, 18, 6, SSD1306_BLACK);
    display.fillRect(rightX, eyeY, 18, 6, SSD1306_BLACK);
  }

  // draw tiny name text if exists
  if (strlen(memoryData.name) > 0) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Hi ");
    display.print(memoryData.name);
  }

  display.display();
}

// ---------- Idle eye behavior (blinking / micro-movements) ----------
void idleEyeBehavior() {
  unsigned long now = millis();
  static bool blinkFrame = false;
  static unsigned long blinkStart = 0;

  // random small pupil offset occasionally
  if (now - lastBlinkMillis > random(2000, 4500)) {
    // blink
    blinkFrame = true;
    blinkStart = now;
    lastBlinkMillis = now;
    eyeOffsetX = random(-3, 4);
    drawEyes(curEmotion, true);
  } else if (blinkFrame && (now - blinkStart > 120)) {
    blinkFrame = false;
    drawEyes(curEmotion, false);
  } else {
    // subtle constant draw to show micro movement every loop iteration for smoothness
    // small random jitter occasionally
    if (random(0,1000) < 6) {
      eyeOffsetX = random(-2, 3);
      drawEyes(curEmotion, false);
    }
  }
}

// ---------- LED ring behavior ----------
void ledIdleBehavior() {
  unsigned long now = millis();
  static uint8_t hue = 0;
  static int breatheStep = 0;
  static bool increasing = true;

  // map emotion -> base hue
  uint8_t baseHue = 160; // pink default (hue scaled 0-255 where ~160 = pinkish)
  switch (curEmotion) {
    case EM_HAPPY: baseHue = 150; break; // cyan-pink
    case EM_SHY: baseHue = 165; break; // pink
    case EM_SLEEPY: baseHue = 30; break; // warm amber
    case EM_LONELY: baseHue = 10; break; // dim warm
    case EM_EXCITED: baseHue = 200; break; // bright pink
    case EM_CALM: default: baseHue = 140; break;
  }

  // breathing brightness
  if (increasing) breatheStep++;
  else breatheStep--;
  if (breatheStep > 60) increasing = false;
  if (breatheStep < 6) increasing = true;
  uint8_t brightness = map(breatheStep, 6, 60, 12, 90);
  brightness = min(brightness, (uint8_t)BLUSH_MAX);

  // set simple uniform color by using HSV->RGB approx
  // simple conversion: hue 0-255 map to rgb using wheel function
  for (int i=0;i<NUM_LEDS;i++){
    uint8_t h = baseHue; // we can add slight rotation in future
    uint8_t r, g, b;
    // simple wheel:
    uint8_t region = h / 85;
    uint8_t remainder = (h % 85) * 3;
    switch (region) {
      case 0: r = 255 - remainder; g = remainder; b = 0; break;
      case 1: r = 0; g = 255 - remainder; b = remainder; break;
      default: r = remainder; g = 0; b = 255 - remainder; break;
    }
    // scale by brightness
    r = (uint8_t)((r * brightness) / 255);
    g = (uint8_t)((g * brightness) / 255);
    b = (uint8_t)((b * brightness) / 255);
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// ---------- Gentle heartbeat vibration pattern (periodic or triggered) ----------
void heartbeatBehavior() {
  unsigned long now = millis();
  // periodic low heartbeat in calm states
  static unsigned long lastBeat = 0;
  if (curEmotion == EM_HAPPY || curEmotion == EM_CALM) {
    if (now - lastBeat > 4500) {
      // soft lub-dub
      digitalWrite(VIB_PIN, HIGH);
      delay(40);
      digitalWrite(VIB_PIN, LOW);
      delay(120);
      digitalWrite(VIB_PIN, HIGH);
      delay(30);
      digitalWrite(VIB_PIN, LOW);
      lastBeat = now;
    }
  }
  // other states don't have periodic heartbeat here (triggered on events)
}

// ---------- Daily special (first interaction) ----------
void doDailySpecial() {
  // widen eyes + brighter blush + small tuned vibration + audio
  Emotion prev = curEmotion;
  curEmotion = EM_EXCITED;
  // bright LED flash for 800ms
  for (int b=0;b<3;b++){
    setAllLEDColor(255, 100, 150, 200);
    delay(200);
    setAllLEDColor(0,0,0,0);
    delay(80);
  }
  // heartbeat + audio
  digitalWrite(VIB_PIN, HIGH); delay(120); digitalWrite(VIB_PIN, LOW);
  playAudioTrack(4); // special daily tune
  curEmotion = prev;
}

// ---------- DFPlayer helpers ----------
void playAudioTrack(uint16_t track) {
  if (!dfplayer.available()) return;
  dfplayer.play(track);
}

// ---------- Reminders scheduling ----------
void schedulerCheckReminders() {
  // get local time from WiFi if available
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // if no time, skip (or optionally use millis uptime)
    Serial.println("No time");
    return;
  }
  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;

  for (int i=0;i<MAX_REMINDERS;i++){
    if (!reminders[i].enabled) continue;
    if (reminders[i].hour == hour && reminders[i].minute == minute) {
      performReminderAction(reminders[i]);
      // disable single-run? we leave as-is (user can set repeating)
    }
  }
}

void performReminderAction(const Reminder &r) {
  // emotion cue then action
  curEmotion = EM_HAPPY;
  setAllLEDColor(255, 120, 140, 180); // warm blush
  digitalWrite(VIB_PIN, HIGH); delay(100); digitalWrite(VIB_PIN, LOW);
  if (r.mode == 1 || r.mode == 2) {
    // play associated track (simple mapping: track 10+i)
    uint16_t t = 10; // pick a track for reminders
    playAudioTrack(t);
  }
}

// ---------- Preferences load/save ----------
void loadPreferences() {
  // load memory name
  size_t n = prefs.getBytesLength("name");
  if (n > 0 && n < sizeof(memoryData.name)) {
    prefs.getBytes("name", memoryData.name, sizeof(memoryData.name));
  } else {
    strcpy(memoryData.name, "Friend");
  }
  memoryData.learningEnabled = prefs.getBool("learn", true);
  memoryData.touchCountToday = prefs.getInt("touches", 0);
  memoryData.totalTouchDuration = prefs.getULong("tott", 0);
  memoryData.lastActive = prefs.getULong("lasta", millis());

  // load reminders
  for (int i=0;i<MAX_REMINDERS;i++){
    reminders[i].enabled = prefs.getBool(("remE"+String(i)).c_str(), false);
    reminders[i].hour = prefs.getUChar(("remH"+String(i)).c_str(), 0);
    reminders[i].minute = prefs.getUChar(("remM"+String(i)).c_str(), 0);
    reminders[i].mode = prefs.getUChar(("remMo"+String(i)).c_str(), 0);
    // text (64 char)
    char key[16];
    snprintf(key, sizeof(key), "remT%d", i);
    size_t len = prefs.getBytesLength(key);
    if (len > 0 && len < sizeof(reminders[i].text)) {
      prefs.getBytes(key, (void*)reminders[i].text, sizeof(reminders[i].text));
    } else {
      reminders[i].text[0] = 0;
    }
  }
}

void savePreferences() {
  prefs.putBytes("name", memoryData.name, strlen(memoryData.name)+1);
  prefs.putBool("learn", memoryData.learningEnabled);
  prefs.putInt("touches", memoryData.touchCountToday);
  prefs.putULong("tott", memoryData.totalTouchDuration);
  prefs.putULong("lasta", memoryData.lastActive);
  for (int i=0;i<MAX_REMINDERS;i++){
    prefs.putBool(("remE"+String(i)).c_str(), reminders[i].enabled);
    prefs.putUChar(("remH"+String(i)).c_str(), reminders[i].hour);
    prefs.putUChar(("remM"+String(i)).c_str(), reminders[i].minute);
    prefs.putUChar(("remMo"+String(i)).c_str(), reminders[i].mode);
    char key[16];
    snprintf(key, sizeof(key), "remT%d", i);
    prefs.putBytes(key, reminders[i].text, strlen(reminders[i].text)+1);
  }
}

// ---------- LEDs convenience ----------
void setAllLEDColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  pixels.setBrightness(min((int)brightness, LED_BRIGHTNESS));
  for (int i=0;i<NUM_LEDS;i++){
    pixels.setPixelColor(i, pixels.Color(r,g,b));
  }
  pixels.show();
}

// ---------- WiFi AP + WebServer ----------
void setupWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PWD);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.on("/addReminder", HTTP_POST, handleAddReminder);
  server.on("/deleteReminder", HTTP_POST, handleDeleteReminder);
  server.on("/status", HTTP_GET, [](){
    // return small JSON of status
    String s = "{";
    s += "\"name\":\""; s += memoryData.name; s += "\",";
    s += "\"learning\":"; s += (memoryData.learningEnabled ? "true" : "false"); s += ",";
    s += "\"touches\":"; s += memoryData.touchCountToday;
    s += "}";
    server.send(200, "application/json", s);
  });

  server.begin();
  Serial.println("HTTP server started");
}

String pageRootHtml() {
  String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'/><title>LoveBuddy</title></head><body style='font-family:Arial'>";
  html += "<h2>LoveBuddy Settings</h2>";
  html += "<form action='/save' method='POST'>";
  html += "Name: <input name='name' value='" + String(memoryData.name) + "'/><br/>";
  html += "Learning: <select name='learning'><option value='1'>On</option><option value='0'>Off</option></select><br/>";
  html += "<button type='submit'>Save</button></form><hr/>";
  html += "<h3>Reminders</h3>";
  html += "<form action='/addReminder' method='POST'>Time (HH:MM): <input name='time' placeholder='08:30'/> Mode: <select name='mode'><option value='0'>Vibrate</option><option value='1'>Audio</option><option value='2'>Both</option></select><br/>Text: <input name='text' placeholder='Happy morning'/> <button type='submit'>Add</button></form><hr/>";
  html += "<h4>Existing</h4><ul>";
  for (int i=0;i<MAX_REMINDERS;i++){
    if (reminders[i].enabled) {
      html += "<li>" + String(reminders[i].hour) + ":" + (reminders[i].minute<10?"0":"") + String(reminders[i].minute) + " - " + String(reminders[i].text);
      html += " <form style='display:inline' action='/deleteReminder' method='POST'><input name='idx' value='" + String(i) + "' hidden/><button type='submit'>Delete</button></form></li>";
    }
  }
  html += "</ul><hr/>";
  html += "<small>Connect to WiFi SSID: " + String(AP_SSID) + "</small>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", pageRootHtml());
}

void handleSaveSettings() {
  if (server.hasArg("name")) {
    String n = server.arg("name");
    n.toCharArray(memoryData.name, sizeof(memoryData.name));
  }
  if (server.hasArg("learning")) {
    memoryData.learningEnabled = server.arg("learning") == "1";
  }
  savePreferences();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAddReminder() {
  if (!server.hasArg("time")) { server.send(400, "text/plain", "time missing"); return; }
  String t = server.arg("time");
  int hh=0, mm=0;
  if (sscanf(t.c_str(), "%d:%d", &hh, &mm) != 2) { server.send(400, "text/plain", "time bad"); return; }
  int mode = server.hasArg("mode") ? atoi(server.arg("mode").c_str()) : 0;
  String txt = server.hasArg("text") ? server.arg("text") : "Reminder";
  // find empty slot
  int idx = -1;
  for (int i=0;i<MAX_REMINDERS;i++) if (!reminders[i].enabled) { idx = i; break; }
  if (idx == -1) { server.send(400, "text/plain", "no space"); return; }
  reminders[idx].enabled = true;
  reminders[idx].hour = hh;
  reminders[idx].minute = mm;
  reminders[idx].mode = mode;
  txt.toCharArray(reminders[idx].text, sizeof(reminders[idx].text));
  savePreferences();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDeleteReminder() {
  if (!server.hasArg("idx")) { server.send(400, "text/plain", "idx missing"); return; }
  int idx = atoi(server.arg("idx").c_str());
  if (idx < 0 || idx >= MAX_REMINDERS) { server.send(400, "text/plain", "idx bad"); return; }
  reminders[idx].enabled = false;
  reminders[idx].text[0] = 0;
  savePreferences();
  server.sendHeader("Location", "/");
  server.send(303);
}
