#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <RTClib.h>
#include <WiFi.h>

// GPIO Definitions
#define BTN_ENROLL 12
#define BTN_DELETE 14
#define BTN_BACK   15

// Display and Fingerprint Setup
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, 32, 33);
RTC_DS3231 rtc;

// WiFi Credentials
const char* ssid = "Geckotech Intern ^_^";
const char* password = "Jeremiah33:3";

const unsigned char dino24_1[] PROGMEM = {
  0x00,0x00,0x00, 0x00,0x38,0x00, 0x00,0x7C,0x00,
  0x00,0xFE,0x00, 0x01,0xFF,0x00, 0x01,0xFF,0x00,
  0x01,0xFF,0x00, 0x00,0xFC,0x00, 0x01,0xFE,0x00,
  0x01,0xB6,0x00, 0x01,0xB6,0x00, 0x01,0xB6,0x00,
  0x01,0xFE,0x00, 0x00,0x60,0x00, 0x00,0xF0,0x00,
  0x00,0xD0,0x00, 0x00,0x10,0x00, 0x00,0x30,0x00,
  0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00
};

const unsigned char dino24_2[] PROGMEM = {
  0x00,0x00,0x00, 0x00,0x38,0x00, 0x00,0x7C,0x00,
  0x00,0xFE,0x00, 0x01,0xFF,0x00, 0x01,0xFF,0x00,
  0x01,0xFF,0x00, 0x00,0xFC,0x00, 0x01,0xFE,0x00,
  0x01,0xB6,0x00, 0x01,0xB6,0x00, 0x01,0xB6,0x00,
  0x01,0xFE,0x00, 0x00,0xF0,0x00, 0x00,0x60,0x00,
  0x00,0xD0,0x00, 0x00,0x00,0x00, 0x00,0x30,0x00,
  0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00
};

const unsigned char dino24_3[] PROGMEM = {
  0x00,0x00,0x00, 0x00,0x38,0x00, 0x00,0x7C,0x00,
  0x00,0xFE,0x00, 0x01,0xFF,0x00, 0x01,0xFF,0x00,
  0x01,0xFF,0x00, 0x00,0xFC,0x00, 0x01,0xFE,0x00,
  0x01,0xB6,0x00, 0x01,0xB6,0x00, 0x01,0xB6,0x00,
  0x01,0xFE,0x00, 0x00,0x60,0x00, 0x00,0xF0,0x00,
  0x00,0x90,0x00, 0x00,0x10,0x00, 0x00,0x38,0x00,
  0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00
};

const unsigned char deadDino [] PROGMEM = {
  0x00,0x00,0x0E,0x00,0x1F,0x00,0x3F,0x80,
  0x3F,0x80,0x3F,0x80,0x3C,0x00,0x7F,0x80,
  0x6B,0x80,0x6B,0x80,0x6B,0x80,0x7F,0x80,
  0x00,0x00,0x0A,0x00,0x15,0x00,0x00,0x00
};

const unsigned char* dinoFrames[] = { dino24_1, dino24_2, dino24_3 };
const int numFrames = 3;

// System State
int currentFrame = 0;
unsigned long lastFrameTime = 0;
int dinoX = 0;
int dinoY = 40;
unsigned long lastBlink = 0;
bool showColon = true;
unsigned long lastRefresh = 0;
bool needsRefresh = true;
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;

// Success Message System
struct {
  bool active = false;
  String title;
  String detail;
  unsigned long startTime;
} successMsg;
const unsigned long MESSAGE_DURATION = 2000; // 2 seconds

enum Mode { SCAN, ENROLL, DELETE };
Mode currentMode = SCAN;
uint8_t id;

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 4, 5);
  finger.begin(57600);

  Wire.begin(33, 32);
  u8g2.begin();
  rtc.begin();
  WiFi.begin(ssid, password);
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Optional first time
  pinMode(BTN_ENROLL, INPUT_PULLUP);
  pinMode(BTN_DELETE, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor OK");
  } else {
    Serial.println("Sensor not found");
    while(1);
  }
 
}

void showSuccessMessage(String title, String detail) {
  successMsg.active = true;
  successMsg.title = title;
  successMsg.detail = detail;
  successMsg.startTime = millis();
  needsRefresh = true;
}

void drawDisplay() {
  u8g2.clearBuffer();
  
  // Show success message if active
  if (successMsg.active) {
    if (millis() - successMsg.startTime < MESSAGE_DURATION) {
      u8g2.setFont(u8g2_font_7x14B_tr);
      centerText(successMsg.title.c_str(), 20);
      
      u8g2.setFont(u8g2_font_6x10_tr);
      centerText(successMsg.detail.c_str(), 40);
      
      centerText("Press any button", 55);
      centerText("to continue", 65);
      
      u8g2.sendBuffer();
      return;
    } else {
      successMsg.active = false;
    }
  }

  // Normal display
  DateTime now = rtc.now();

  // Top Bar - Mode and Time
  u8g2.setFont(u8g2_font_7x14B_tr);
  switch(currentMode) {
    case SCAN:   u8g2.drawStr(0, 12, "SCAN"); break;
    case ENROLL: u8g2.drawStr(0, 12, "ENROLL"); break;
    case DELETE: u8g2.drawStr(0, 12, "DELETE"); break;
  }

  char timeBuf[9];
  sprintf(timeBuf, "%02d%c%02d", now.hour(), showColon ? ':' : ' ', now.minute());
  u8g2.drawStr(128 - u8g2.getStrWidth(timeBuf), 12, timeBuf);

  // Main Content
  u8g2.setFont(u8g2_font_6x10_tr);
  char dateBuf[20];
  sprintf(dateBuf, "%02d/%02d/%04d", now.month(), now.day(), now.year());
  centerText(dateBuf, 25);
  centerText(wifiConnected ? "WiFi: CONNECTED" : "WiFi: OFFLINE", 40);

  switch(currentMode) {
    case SCAN:   centerText("Ready to scan", 55); break;
    case ENROLL: centerText("Press finger", 55); break;
    case DELETE: centerText("Enter ID to delete", 55); break;
  }

  // Animation Area
  u8g2.drawHLine(0, 63, 128);
  if (wifiConnected) drawRunningDino(); else drawDeadDino();
  
  u8g2.sendBuffer();

  if (millis() - lastBlink > 500) { 
    showColon = !showColon; 
    lastBlink = millis();
    needsRefresh = true;
  }
}

void centerText(const char* txt, int y) {
  int w = u8g2.getStrWidth(txt);
  u8g2.setCursor((128 - w) / 2, y);
  u8g2.print(txt);
}

void drawRunningDino() {
  if (millis() - lastFrameTime > 150) {
    currentFrame = (currentFrame + 1) % numFrames;
    lastFrameTime = millis();
    dinoX += 4;
    if (dinoX > 128) dinoX = -16;
    needsRefresh = true;
  }
  u8g2.drawXBMP(dinoX, dinoY, 24, 24, dinoFrames[currentFrame]);
}

void drawDeadDino() {
  u8g2.drawXBMP((128 - 16) / 2, 47, 16, 16, deadDino);
}

void checkButtons() {
  if (successMsg.active && (digitalRead(BTN_ENROLL) == LOW || 
                          digitalRead(BTN_DELETE) == LOW || 
                          digitalRead(BTN_BACK) == LOW)) {
    successMsg.active = false;
    needsRefresh = true;
    delay(200);
    return;
  }

  if (digitalRead(BTN_ENROLL) == LOW) { 
    currentMode = ENROLL; 
    needsRefresh = true;
    delay(500);
  }
  if (digitalRead(BTN_DELETE) == LOW) { 
    currentMode = DELETE;
    needsRefresh = true;
    delay(500);
  }
  if (digitalRead(BTN_BACK) == LOW) { 
    currentMode = SCAN;
    needsRefresh = true;
    delay(500);
  }
}

uint8_t promptForID(String label) {
  Serial.print("\n" + label + " MODE\nEnter ID (1-127): ");
  String input = "";
  while (true) {
    if (digitalRead(BTN_BACK) == LOW) {
      Serial.println("\nCancelled");
      delay(500); 
      return 0; 
    }
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) return input.toInt();
      } else {
        input += c;
        Serial.print(c);
      }
    }
  }
}

bool enrollFingerprint(uint8_t id) {
  int p = -1;
  Serial.println("Place your finger");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (digitalRead(BTN_BACK) == LOW) return false;
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p != FINGERPRINT_OK) return false;
  }
  
  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;
  
  Serial.println("Remove finger");
  while (finger.getImage() != FINGERPRINT_NOFINGER);
  
  Serial.println("Place same finger again");
  while (finger.getImage() != FINGERPRINT_OK) {
    if (digitalRead(BTN_BACK) == LOW) return false;
  }
  
  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;
  if (finger.createModel() != FINGERPRINT_OK) return false;
  
  if (finger.storeModel(id) == FINGERPRINT_OK) {
    showSuccessMessage("ENROLLED!", "ID: " + String(id) + " saved");
    return true;
  }
  return false;
}

void logAttendance() {
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) return;
  
  if (finger.image2Tz() != FINGERPRINT_OK) return;
  
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    showSuccessMessage("VERIFIED!", "ID: " + String(finger.fingerID));
  } else {
    showSuccessMessage("NOT RECOGNIZED", "Try again");
  }
}

void deleteFingerprint(uint8_t id) {
  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    showSuccessMessage("DELETED!", "ID: " + String(id) + " removed");
  } else {
    showSuccessMessage("DELETE FAILED", "ID not found");
  }
}

void loop() {
  checkButtons();
  
  if (millis() - lastWifiCheck > 2000) {
    bool newWifiStatus = (WiFi.status() == WL_CONNECTED);
    if (newWifiStatus != wifiConnected) {
      wifiConnected = newWifiStatus;
      needsRefresh = true;
    }
    lastWifiCheck = millis();
  }

  if (needsRefresh || millis() - lastRefresh > 100) {
    drawDisplay();
    lastRefresh = millis();
    needsRefresh = false;
  }

  switch (currentMode) {
    case SCAN:   logAttendance(); break;
    case ENROLL: id = promptForID("ENROLL"); 
                 if (id) enrollFingerprint(id); 
                 currentMode = SCAN; 
                 needsRefresh = true; 
                 break;
    case DELETE: id = promptForID("DELETE"); 
                 if (id) deleteFingerprint(id); 
                 currentMode = SCAN; 
                 needsRefresh = true; 
                 break;
  }
}