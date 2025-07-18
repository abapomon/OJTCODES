#include <ETH.h>
#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <fpm.h>
#include <WiFiClient.h>

// OLED on I2C (SCL=GPIO32, SDA=GPIO33)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 33, 32);

// WT32-ETH01 LAN8720 config
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    1
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN   16
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18

// Fingerprint sensor on UART2 (TX=GPIO5, RX=GPIO4)
HardwareSerial fserial(2);
FPM finger(&fserial);
FPMSystemParams params;

// TCP Server (change to your PC IP)
const char* serverIP = "192.168.2.106";
const uint16_t serverPort = 9999;

WiFiClient client;
bool eth_connected = false;
String ipString = "Getting IP...";
String oledStatus = "Booting...";

// ====================== UART & SENSOR FIXES ======================
// 1. Sensor wake-up command (for R503/FPM10A)
void wakeUpSensor() {
  const uint8_t wakeCmd[] = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  fserial.write(wakeCmd, sizeof(wakeCmd));
  delay(100);
}

// 2. Reset sensor on failure
void resetSensor() {
  fserial.end();
  delay(200);
  fserial.begin(57600, SERIAL_8N1, 4, 5);
  delay(200);
  wakeUpSensor();
}

// ====================== ETHERNET EVENTS ======================
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname("esp32-fingerprint");
      oledStatus = "ETH Started";
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      oledStatus = "ETH Connected";
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      eth_connected = true;
      ipString = ETH.localIP().toString();
      oledStatus = "IP OK";
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      eth_connected = false;
      ipString = "Disconnected";
      oledStatus = "ETH Down";
      break;
    default:
      break;
  }
}

// ====================== OLED DISPLAY ======================
void showStatus(const char* status) {
  Serial.println(status);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 10, "ETH:");
  u8g2.drawStr(40, 10, eth_connected ? "CONNECTED" : "DISCONNECTED");
  u8g2.drawStr(0, 25, "IP:");
  u8g2.drawStr(40, 25, ipString.c_str());
  u8g2.drawStr(0, 40, "STATUS:");
  u8g2.drawStr(0, 55, status);
  u8g2.sendBuffer();
}

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Wire.begin(33, 32);
  u8g2.begin();
  showStatus("Booting...");

  // Initialize Ethernet
  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);

  // Wait for IP
  while (ETH.localIP() == IPAddress(0, 0, 0, 0)) {
    showStatus("Waiting for IP...");
    delay(500);
  }

  // Initialize Fingerprint Sensor
  fserial.begin(57600, SERIAL_8N1, 4, 5);
  delay(200);
  wakeUpSensor();  // Wake up sensor first

  if (finger.begin()) {
    finger.readParams(&params);
    showStatus("Sensor OK");
    Serial.println("✅ Fingerprint sensor ready");
  } else {
    showStatus("Sensor not found");
    Serial.println("❌ Sensor not found");
    while (1);
  }
}

// ====================== MAIN LOOP ======================
void loop() {
  if (captureAndSendImageTCP()) {
    showStatus("Scan success!");
    delay(5000);  // Shorter delay after success
  } else {
    showStatus("Scan failed. Retry...");
    delay(2000);  // Shorter delay on failure
  }
}

// ====================== FIXED IMAGE CAPTURE & SEND ======================
bool captureAndSendImageTCP() {
  FPMStatus status;
  showStatus("Place finger...");
  Serial.println("👉 Place your finger");

  // 1. Retry image capture 3 times
  for (uint8_t retry = 0; retry < 3; retry++) {
    status = finger.getImage();
    if (status == FPMStatus::NOFINGER) {
      delay(200);
      continue;
    } else if (status == FPMStatus::OK) {
      break;
    } else {
      Serial.printf("⚠️ getImage failed (retry %d)\n", retry + 1);
      resetSensor();
    }
  }

  if (status != FPMStatus::OK) {
    showStatus("Capture failed");
    return false;
  }

  showStatus("Image captured");
  Serial.println("📷 Fingerprint image captured");

  // 2. Download image with retries
  for (uint8_t retry = 0; retry < 3; retry++) {
    if (finger.downloadImage() == FPMStatus::OK) break;
    if (retry == 2) {
      showStatus("Download failed");
      return false;
    }
    resetSensor();
  }

  // 3. Connect to TCP server
  showStatus("Connecting TCP...");
  if (!client.connect(serverIP, serverPort)) {
    showStatus("TCP Connect fail");
    Serial.println("❌ TCP connection failed");
    return false;
  }

  // 4. Send image data with error recovery
  showStatus("Sending image...");
  uint16_t readLen = 0;
  uint32_t totalSent = 0;
  bool complete = false;

  while (!complete) {
    bool ok = finger.readDataPacket(nullptr, &client, &readLen, &complete);
    if (!ok) {
      Serial.println("⚠️ Packet error, retrying...");
      resetSensor();
      client.stop();
      return false;
    }
    totalSent += readLen;
    delay(5);
  }

  char msg[32];
  snprintf(msg, sizeof(msg), "Sent %lu bytes", totalSent);
  showStatus(msg);
  client.stop();
  Serial.printf("✅ Sent %lu bytes to server\n", totalSent);
  return true;
}