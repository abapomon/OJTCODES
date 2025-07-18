#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <RTClib.h>
#include <ETH.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Pins
#define BTN_ENROLL 12
#define BTN_DELETE 14
#define BTN_BACK   15

// Fingerprint UART
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger(&mySerial);

// OLED (SCL=32, SDA=33)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 32, 33);

// RTC
RTC_DS3231 rtc;

// Ethernet config
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    1
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN   16
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18

// State
bool wifiConnected = false;
bool needsRefresh = true;
unsigned long lastBlink = 0;
bool showColon = true;

enum Mode { SCAN, ENROLL, DELETE };
Mode currentMode = SCAN;
uint8_t id;

struct {
  bool active = false;
  String title;
  String detail;
  unsigned long startTime;
} message;

const unsigned long MESSAGE_DURATION = 3000;
unsigned long lastRefresh = 0;
unsigned long lastScanTime = 0;
const unsigned long SCAN_DELAY = 3000; // 3 seconds between scans

// WiFi Event Handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START: ETH.setHostname("esp32-fingerprint"); break;
    case ARDUINO_EVENT_ETH_GOT_IP: wifiConnected = true; needsRefresh = true; break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP: wifiConnected = false; needsRefresh = true; break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 4, 5);
  finger.begin(57600);

  Wire.begin(33, 32); // SDA, SCL
  u8g2.begin();

  if (!rtc.begin()) {
    Serial.println("❌ RTC not found");
    while (1);
  }

  if (rtc.lostPower() || rtc.now().year() < 2023) {
    Serial.println("🛠 Setting RTC time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pinMode(BTN_ENROLL, INPUT_PULLUP);
  pinMode(BTN_DELETE, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);

  if (!finger.verifyPassword()) {
    Serial.println("❌ Fingerprint sensor not found");
    while (1);
  }
}

// ===== Display Drawing =====
void drawDisplay() {
  u8g2.clearBuffer();
  DateTime now = rtc.now();

  if (message.active && millis() - message.startTime < MESSAGE_DURATION) {
    u8g2.setFont(u8g2_font_7x14B_tr);
    centerText(message.title.c_str(), 20);
    u8g2.setFont(u8g2_font_6x10_tr);
    centerText(message.detail.c_str(), 40);
  } else {
    message.active = false;
    u8g2.setFont(u8g2_font_7x14B_tr);
    switch (currentMode) {
      case SCAN: u8g2.drawStr(0, 12, "SCAN MODE"); break;
      case ENROLL: u8g2.drawStr(0, 12, "ENROLL MODE"); break;
      case DELETE: u8g2.drawStr(0, 12, "DELETE MODE"); break;
    }

    char timeBuf[9];
    sprintf(timeBuf, "%02d%c%02d", now.hour(), showColon ? ':' : ' ', now.minute());
    u8g2.drawStr(128 - u8g2.getStrWidth(timeBuf), 12, timeBuf);

    u8g2.setFont(u8g2_font_6x10_tr);
    char dateBuf[20];
    sprintf(dateBuf, "%02d/%02d/%04d", now.month(), now.day(), now.year());
    centerText(dateBuf, 30);

    if (wifiConnected) {
      IPAddress ip = ETH.localIP();
      char ipBuf[20];
      sprintf(ipBuf, "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      centerText(ipBuf, 45);
    } else {
      centerText("ETH: OFFLINE", 45);
    }

    switch (currentMode) {
      case SCAN: centerText("Place finger to scan", 60); break;
      case ENROLL: centerText("Press finger to enroll", 60); break;
      case DELETE: centerText("Enter ID to delete", 60); break;
    }
  }

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

void showMessage(String title, String detail) {
  message = {true, title, detail, millis()};
  needsRefresh = true;
}

// ====== Fingerprint Functions ======
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
  Serial.println("Remove finger...");
  while (finger.getImage() != FINGERPRINT_NOFINGER);
  Serial.println("Place same finger again");

  while (finger.getImage() != FINGERPRINT_OK) {
    if (digitalRead(BTN_BACK) == LOW) return false;
  }

  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;
  if (finger.createModel() != FINGERPRINT_OK) return false;

  if (finger.storeModel(id) == FINGERPRINT_OK) {
    showMessage("ENROLLED!", "ID: " + String(id));
    return true;
  }

  return false;
}

void deleteFingerprint(uint8_t id) {
  if (finger.deleteModel(id) == FINGERPRINT_OK)
    showMessage("DELETED!", "ID: " + String(id));
  else
    showMessage("DELETE FAILED", "Not Found");
}

uint8_t promptForID(String label) {
  Serial.print("\n" + label + " MODE\nEnter ID (1-127): ");
  String input = "";
  while (true) {
    if (digitalRead(BTN_BACK) == LOW) return 0;
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') return input.toInt();
      input += c;
      Serial.print(c);
    }
  }
}

void sendLogToServer(uint16_t id, DateTime timestamp) {
  if (!wifiConnected) return;
  HTTPClient http;
  http.begin("http://192.168.2.100:3000/api/log");
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"id\":" + String(id) + ",";
  payload += "\"timestamp\":\"" + String(timestamp.year()) + "-" +
             String(timestamp.month()) + "-" +
             String(timestamp.day()) + " " +
             String(timestamp.hour()) + ":" +
             String(timestamp.minute()) + ":" +
             String(timestamp.second()) + "\"}";

  int code = http.POST(payload);
  Serial.print("POST Response: "); Serial.println(code);
  http.end();
}

void logAttendance() {
  if (millis() - lastScanTime < SCAN_DELAY) return;
  if (finger.getImage() != FINGERPRINT_OK) return;
  if (finger.image2Tz() != FINGERPRINT_OK) return;

  int p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    uint16_t foundID = finger.fingerID;
    DateTime now = rtc.now();
    showMessage("VERIFIED!", "ID: " + String(foundID));
    sendLogToServer(foundID, now);
    lastScanTime = millis(); // Prevent double scan
  } else {
    showMessage("NOT RECOGNIZED", "Try again");
    lastScanTime = millis(); // Still wait before retrying
  }
}

// ====== Button & Main Loop ======
void checkButtons() {
  if (message.active && (digitalRead(BTN_ENROLL) == LOW || digitalRead(BTN_DELETE) == LOW || digitalRead(BTN_BACK) == LOW)) {
    message.active = false;
    needsRefresh = true;
    delay(200);
    return;
  }

  if (digitalRead(BTN_ENROLL) == LOW) {
    currentMode = ENROLL; needsRefresh = true; delay(300);
  }
  if (digitalRead(BTN_DELETE) == LOW) {
    currentMode = DELETE; needsRefresh = true; delay(300);
  }
  if (digitalRead(BTN_BACK) == LOW) {
    currentMode = SCAN; needsRefresh = true; delay(300);
  }
}

void loop() {
  checkButtons();

  if (needsRefresh || millis() - lastRefresh > 100) {
    drawDisplay();
    lastRefresh = millis();
    needsRefresh = false;
  }

  switch (currentMode) {
    case SCAN: logAttendance(); break;
    case ENROLL:
      id = promptForID("ENROLL");
      if (id) enrollFingerprint(id);
      currentMode = SCAN; needsRefresh = true;
      break;
    case DELETE:
      id = promptForID("DELETE");
      if (id) deleteFingerprint(id);
      currentMode = SCAN; needsRefresh = true;
      break;
  }
}
