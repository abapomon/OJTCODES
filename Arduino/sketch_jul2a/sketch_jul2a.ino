#include <Wire.h>
#include <U8g2lib.h>
#include <RTClib.h>
#include <WiFi.h>

// 📟 OLED Display I2C (SCL = GPIO32, SDA = GPIO33)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0, /* reset=*/ U8X8_PIN_NONE, 32, 33
);

// 🕒 RTC
RTC_DS3231 rtc;

// 🌐 Wi-Fi Credentials
const char* ssid = "Geckotech Intern ^_^";
const char* password = "Jeremiah33:3";

// 🦖 Dino Animation
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
int currentFrame = 0;
unsigned long lastFrameTime = 0;
unsigned long lastBlink = 0;
bool showColon = true;
int dinoX = 0;

// Wi-Fi status
bool wifiConnected = false;

int dinoY = 40;        // Ground level
bool jumping = false;
int jumpOffset = 0;
int jumpSpeed = -3;

void setup() {

  Serial.begin(115200);
  Serial.println(WiFi.localIP());


  Wire.begin(33, 32);
  u8g2.begin();
  rtc.begin();
  WiFi.begin(ssid, password);

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Optional first time
}

void loop() {
  u8g2.clearBuffer();

  // ⏱ Get RTC time
  DateTime now = rtc.now();

  bool isNight = now.hour() >= 18 || now.hour() < 6;

  // 📶 Check Wi-Fi connection
  wifiConnected = WiFi.status() == WL_CONNECTED;

  

  // 🏷 Header
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(0, 10);
  u8g2.print("WT32-ETH01");

  char timeBuf[9];
  sprintf(timeBuf, "%02d%c%02d%c%02d",
    now.hour(),
    showColon ? ':' : ' ',
    now.minute(),
    showColon ? ':' : ' ',
    now.second());
  int timeW = u8g2.getStrWidth(timeBuf);
  u8g2.setCursor(128 - timeW, 10);
  u8g2.print(timeBuf);

  // 🗓 Date
  char dateBuf[20];
  sprintf(dateBuf, "Jul %d, %d", now.day(), now.year());
  centerText(dateBuf, 25);

  // 💬 Status
  u8g2.setFont(u8g2_font_6x10_tf);
  centerText(wifiConnected ? "WiFi Connected!" : "Waiting for WiFi...", 42);

  // 🌵 Draw ground
  u8g2.drawHLine(0, 63, 128); // ground line

  // Draw cactus (simple lines)
  if (wifiConnected) {
    u8g2.drawLine(100, 63, 100, 55);
    u8g2.drawLine(102, 63, 102, 58);
    u8g2.drawLine(104, 63, 104, 56);
  }

  // 🦖 Dino logic
  if (wifiConnected) {
    drawRunningDino();
  } else {
    drawDeadDino();
  }

  u8g2.sendBuffer();

  // 🔄 Colon blink
  if (millis() - lastBlink > 500) {
    showColon = !showColon;
    lastBlink = millis();
  }

  if (isNight) drawNightSky();

  delay(30);
}

// ─────────────────────────────────────────────
// Center text helper
void centerText(const char* txt, int y) {
  int w = u8g2.getStrWidth(txt);
  u8g2.setCursor((128 - w) / 2, y);
  u8g2.print(txt);
}

// ─────────────────────────────────────────────
// Running dino that moves horizontally
void drawRunningDino() {
  if (millis() - lastFrameTime > 150) {
    currentFrame = (currentFrame + 1) % numFrames;
    lastFrameTime = millis();
    dinoX += 4;
    if (dinoX > 128) dinoX = -16;
  }
  u8g2.drawXBMP(dinoX, dinoY + jumpOffset, 24, 24, dinoFrames[currentFrame]);
  Serial.println("Dino Frame: " + String(currentFrame) + ", X: " + String(dinoX));
}

// Dead dino in center
void drawDeadDino() {
  u8g2.drawXBMP((128 - 16) / 2, 47, 16, 16, deadDino);
}

void updateJump() {
  if (jumping) {
    jumpOffset += jumpSpeed;
    jumpSpeed += 1; // gravity
    if (jumpOffset >= 0) {
      jumpOffset = 0;
      jumpSpeed = -3;
      jumping = false;
    }
  }
}

void drawNightSky() {
  u8g2.drawPixel(10, 5);
  u8g2.drawPixel(50, 10);
  u8g2.drawPixel(80, 3);
  u8g2.drawPixel(100, 7);
  // clouds or more stars here
}

