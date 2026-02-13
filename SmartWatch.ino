#include <WiFi.h>
#include <FirebaseESP32.h> 
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_BMP280.h>
#include <TFT_eSPI.h>
#include <QMC5883LCompass.h> 
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "MAX30105.h" 
#include <heartRate.h>

// --- PIN DEFINITIONS ---
#define BTN_UP    25
#define BTN_DOWN  26
#define BTN_OK    14 
#define BTN_MENU  33

// --- WIFI & FIREBASE CONFIG ---
const char* ssid = "ACT101637594020";
const char* password = "54164887";
#define FIREBASE_HOST "smartwatch-esp32-cf3b5-default-rtdb.asia-southeast1.firebasedatabase.app" 
#define FIREBASE_AUTH "PJGJCpLE5nR0Wu5J8Tb0KTMswczAa8yG8Xhro1gx"

// Objects
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
TFT_eSPI tft = TFT_eSPI();
RTC_DS1307 rtc;
Adafruit_BMP280 bmp;
QMC5883LCompass compass; 
TinyGPSPlus gps;

// ONLY CHANGE: Using HardwareSerial 2 for stability
HardwareSerial GPS_Serial(2); 

MAX30105 particleSensor;

// State Variables
enum Screen {TIME_SCREEN, MENU_SCREEN, TEMP_SCREEN, COMPASS_SCREEN, GPS_SCREEN, PPG_SCREEN, ADJUST_TIME};
Screen currentScreen = TIME_SCREEN;
Screen lastScreen = TIME_SCREEN;

int menuIndex = 0, editStep = 0;
float bpmAvg = 0, spo2Avg = 98;
int tH, tM, tD, tMo, tY;
bool isPM = false, firstDraw = true;
unsigned long lastBtnTime = 0, lastUploadTime = 0;
unsigned long lastBeatTime = 0; 
bool fingerDetected = false; 

// Function Prototypes
bool checkForBeat(long sample);
void uploadToFirebase();

void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP, INPUT_PULLUP); pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP); pinMode(BTN_MENU, INPUT_PULLUP);

  Wire.begin(21, 22); 
  tft.init(); tft.setRotation(0); tft.fillScreen(TFT_BLACK);

  WiFi.begin(ssid, password);
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  rtc.begin(); 
  bmp.begin(0x76); 
  compass.init(); 
  
  // GPS CONFIG: Pins 16 (RX) and 17 (TX)
  GPS_Serial.begin(9600, SERIAL_8N1, 16, 17); 

  if (particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    particleSensor.setup(0x1F, 1, 2, 400, 411, 4096); 
  }
}

void loop() {
  handleButtons();
  
  // IMPROVED GPS READING: Constant background feeding
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }

  // --- BACKGROUND SENSING (KEEPING YOUR EXACT LOGIC) ---
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();
  
  if (irValue > 40000) { 
    fingerDetected = true;
    if (checkForBeat(irValue)) {
      unsigned long delta = millis() - lastBeatTime;
      lastBeatTime = millis();
      float bpmNow = 60000.0 / delta;
      if (bpmNow > 55 && bpmNow < 135) { 
        if (bpmAvg == 0) bpmAvg = bpmNow;
        else bpmAvg = (bpmAvg * 0.6) + (bpmNow * 0.4); 
      }
    }
    double ratio = (double)redValue / (double)irValue;
    float s = 104 - (17 * ratio);
    if (s > 85 && s <= 100) {
      if (spo2Avg == 0) spo2Avg = s;
      else spo2Avg = (spo2Avg * 0.8) + (s * 0.2);
    }
  } else {
    fingerDetected = false;
    bpmAvg = 0;
    spo2Avg = 0;
  }

  // --- UI REFRESH ---
  if (currentScreen != lastScreen) { 
    tft.fillScreen(TFT_BLACK); 
    lastScreen = currentScreen; 
    firstDraw = true; 
  }

  switch (currentScreen) {
    case TIME_SCREEN:    drawTime(); break;
    case MENU_SCREEN:    drawMenu(); break;
    case TEMP_SCREEN:    drawTemp(); break;
    case COMPASS_SCREEN: drawCompass(); break;
    case GPS_SCREEN:     drawGPS(); break;
    case PPG_SCREEN:     drawPPG(); break;
    case ADJUST_TIME:    drawAdjustTime(); break;
  }

  if (millis() - lastUploadTime > 3000 && WiFi.status() == WL_CONNECTED) {
    uploadToFirebase();
    lastUploadTime = millis();
  }
}

// ---------------- UI SCREENS (NO CHANGES) ----------------

void drawTime() {
  DateTime now = rtc.now();
  if (firstDraw) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WiFi.status() == WL_CONNECTED ? TFT_BLUE : TFT_RED, TFT_BLACK);
    tft.drawString(WiFi.status() == WL_CONNECTED ? "ONLINE" : "OFFLINE", 60, 15, 2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("DEVICE SYNC", 180, 15, 2);
    tft.drawFastHLine(10, 30, 220, TFT_DARKGREY);
    
    int h = now.hour(); const char* ampm = (h >= 12) ? "PM" : "AM";
    if (h > 12) h -= 12; if (h == 0) h = 12;
    char timeBuf[16];
    sprintf(timeBuf, "%d:%02d", h, now.minute());
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(timeBuf, 110, 100, 7); 
    tft.setTextColor(TFT_GOLD, TFT_BLACK); tft.drawString(ampm, 205, 105, 4);

    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char dateBuf[32];
    sprintf(dateBuf, "%02d %s %d", now.day(), months[now.month()-1], now.year());
    tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.drawCentreString(dateBuf, 120, 180, 4);
    firstDraw = false;
  }
}

void drawMenu() {
  const char* items[6] = {"TEMPERATURE", "COMPASS", "GPS TRACKER", "HEART RATE", "SET WATCH", "CLOSE"};
  tft.setTextDatum(TL_DATUM);
  for (int i = 0; i < 6; i++) {
    int yPos = 45 + (i * 32);
    if (i == menuIndex) { tft.fillRect(10, yPos-2, 220, 28, TFT_GREEN); tft.setTextColor(TFT_BLACK, TFT_GREEN); }
    else { tft.fillRect(10, yPos-2, 220, 28, TFT_BLACK); tft.setTextColor(TFT_WHITE, TFT_BLACK); }
    tft.drawString(items[i], 20, yPos + 4, 2);
  }
}

void drawTemp() {
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("ENVIRONMENT", 120, 30, 4);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Temp:", 20, 90, 4); tft.drawFloat(bmp.readTemperature(), 1, 120, 90, 4); tft.drawString("C", 200, 90, 4);
  tft.drawString("Pres:", 20, 140, 4); tft.drawFloat(bmp.readPressure() / 100.0F, 1, 110, 140, 4); tft.drawString("hPa", 195, 145, 2);
}

void drawCompass() {
  compass.read();
  int h = (compass.getAzimuth() + 163) % 360; if(h < 0) h += 360;
  String d = (h>=337||h<22)?"NORTH":(h<67)?"N-EAST":(h<112)?"EAST":(h<157)?"S-EAST":(h<202)?"SOUTH":(h<247)?"S-WEST":(h<292)?"WEST":"N-WEST";
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawCentreString("COMPASS", 120, 30, 4);
  tft.fillRect(0, 70, 240, 130, TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.drawCentreString(d, 120, 95, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawNumber(h, 120, 150, 7);
}

void drawGPS() {
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawCentreString("GPS TRACKER", 120, 25, 4);
  tft.fillRect(0, 60, 240, 120, TFT_BLACK); 
  if (gps.location.isValid()) {
    tft.setTextDatum(TL_DATUM); tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(20, 80); tft.printf("LAT: %.5f", gps.location.lat());
    tft.setCursor(20, 115); tft.printf("LNG: %.5f", gps.location.lng());
  } else {
    tft.drawCentreString("SEARCHING...", 120, 100, 2);
  }
}

void drawPPG() {
  static bool lastFingerState = false;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_RED, TFT_BLACK); 
  tft.drawCentreString("HEALTH MONITOR", 120, 25, 4);

  if (fingerDetected != lastFingerState || firstDraw) {
    tft.fillRect(0, 60, 240, 180, TFT_BLACK);
    lastFingerState = fingerDetected;
    firstDraw = false;
  }

  if (!fingerDetected) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK); 
    tft.drawCentreString("PLACE FINGER", 120, 110, 4);
  } else {
    tft.setTextDatum(TL_DATUM); 
    tft.setTextColor(TFT_RED, TFT_BLACK); tft.drawString("BPM:", 40, 90, 4);
    tft.fillRect(120, 80, 100, 40, TFT_BLACK); tft.drawNumber((int)bpmAvg, 140, 90, 4);
    tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.drawString("SpO2:", 40, 150, 4);
    tft.fillRect(120, 140, 100, 40, TFT_BLACK); tft.drawNumber((int)spo2Avg, 140, 150, 4); 
    tft.drawString("%", 205, 150, 4);
  }
}

void drawAdjustTime() {
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_MAGENTA, TFT_BLACK); tft.drawCentreString("SET WATCH", 120, 30, 4);
  tft.fillRect(0, 80, 240, 140, TFT_BLACK); 
  char tStr[16], dStr[16];
  sprintf(tStr, "%02d:%02d %s", tH, tM, isPM ? "PM" : "AM");
  sprintf(dStr, "%02d/%02d/%d", tD, tMo, tY);
  tft.setTextColor(editStep < 3 ? TFT_GREEN : TFT_WHITE, TFT_BLACK); tft.drawString(tStr, 120, 100, 4);
  tft.setTextColor(editStep >= 3 ? TFT_GREEN : TFT_WHITE, TFT_BLACK); tft.drawString(dStr, 120, 150, 4);
  const char* labels[] = {"HOUR", "MINUTE", "AM/PM", "DAY", "MONTH", "YEAR"};
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawCentreString(labels[editStep], 120, 210, 2);
}

// ---------------- LOGIC ----------------

void handleButtons() {
  if (millis() - lastBtnTime < 200) return;
  if (digitalRead(BTN_MENU) == LOW) { currentScreen = (currentScreen == TIME_SCREEN) ? MENU_SCREEN : TIME_SCREEN; lastBtnTime = millis(); }
  
  if (currentScreen == MENU_SCREEN) {
    if (digitalRead(BTN_UP) == LOW) { menuIndex = (menuIndex - 1 + 6) % 6; lastBtnTime = millis(); }
    if (digitalRead(BTN_DOWN) == LOW) { menuIndex = (menuIndex + 1) % 6; lastBtnTime = millis(); }
    if (digitalRead(BTN_OK) == LOW) {
      if (menuIndex == 4) {
        currentScreen = ADJUST_TIME; editStep = 0;
        DateTime n = rtc.now(); tH=n.hour(); tM=n.minute(); tD=n.day(); tMo=n.month(); tY=n.year();
        if(tH>=12){isPM=true; if(tH>12)tH-=12;}else{isPM=false; if(tH==0)tH=12;}
      } else {
        if(menuIndex==0) currentScreen=TEMP_SCREEN; if(menuIndex==1) currentScreen=COMPASS_SCREEN;
        if(menuIndex==2) currentScreen=GPS_SCREEN; if(menuIndex==3) currentScreen=PPG_SCREEN;
        if(menuIndex==5) currentScreen=TIME_SCREEN;
      }
      lastBtnTime = millis();
    }
  } else if (currentScreen == ADJUST_TIME) {
    if (digitalRead(BTN_UP) == LOW) {
      if(editStep==0){tH++; if(tH>12)tH=1;} if(editStep==1){tM++; if(tM>59)tM=0;}
      if(editStep==2)isPM=!isPM; if(editStep==3){tD++; if(tD>31)tD=1;}
      if(editStep==4){tMo++; if(tMo>12)tMo=1;} if(editStep==5)tY++;
      lastBtnTime = millis();
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      if(editStep==0){tH--; if(tH<1)tH=12;} if(editStep==1){tM--; if(tM<0)tM=59;}
      if(editStep==2)isPM=!isPM; if(editStep==3){tD--; if(tD<1)tD=31;}
      if(editStep==4){tMo--; if(tMo<1)tMo=12;} if(editStep==5)tY--;
      lastBtnTime = millis();
    }
    if (digitalRead(BTN_OK) == LOW) {
      editStep++;
      if (editStep > 5) {
        int fh = tH; if(isPM && fh<12) fh+=12; if(!isPM && fh==12) fh=0;
        rtc.adjust(DateTime(tY, tMo, tD, fh, tM, 0)); currentScreen = TIME_SCREEN;
      }
      lastBtnTime = millis();
    }
  }
}

void uploadToFirebase() {
  FirebaseJson json;
  DateTime now = rtc.now();
  json.set("temp", bmp.readTemperature());
  json.set("pres", bmp.readPressure() / 100.0F);
  compass.read();
  int h = (compass.getAzimuth() + 163) % 360; 
  if(h < 0) h += 360;
  json.set("compass", h);
  if (gps.location.isValid()) {
    json.set("lat", gps.location.lat());
    json.set("lng", gps.location.lng());
  } else { json.set("lat", 0.0); json.set("lng", 0.0); }
  json.set("bpm", (int)bpmAvg);
  json.set("spo2", (int)spo2Avg);
  int h12 = now.hour();
  const char* ampm = (h12 >= 12) ? "PM" : "AM";
  if (h12 > 12) h12 -= 12;
  if (h12 == 0) h12 = 12;
  char timeStr[16], dateStr[12];
  sprintf(timeStr, "%d:%02d:%02d %s", h12, now.minute(), now.second(), ampm);
  sprintf(dateStr, "%02d/%02d/%d", now.day(), now.month(), now.year());
  json.set("time", timeStr); 
  json.set("date", dateStr);
  Firebase.setJSON(firebaseData, "/DeviceData", json);
}

bool checkForBeat(long sample) {
  static float mAvg=0; static bool rising=false; static long lastP=0;
  mAvg=(mAvg*0.92)+(sample*0.08); float diff=sample-mAvg;
  if (diff>20 && !rising) { rising=true; if (millis()-lastP>350){ lastP=millis(); return true; } }
  else if (diff<0) rising=false; return false;
}