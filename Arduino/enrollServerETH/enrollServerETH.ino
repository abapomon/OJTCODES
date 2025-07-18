#include <ETH.h>
#include <Wire.h>
#include <Adafruit_Fingerprint.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WiFi.h>

// LAN config for WT32-ETH01 (LAN8720)
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    1
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN   16
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18

// Fingerprint (UART2: TX=GPIO5, RX=GPIO4)
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Server
const char* serverIP = "192.168.2.106";
const int serverPort = 3000;

uint8_t templateData[512];
bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_GOT_IP:
      eth_connected = true;
      Serial.print("✅ IP address: ");
      Serial.println(ETH.localIP());
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      eth_connected = false;
      Serial.println("❌ Ethernet disconnected");
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);

  Serial.println("⏳ Waiting for Ethernet...");
  while (!eth_connected) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Ethernet connected");

  mySerial.begin(57600, SERIAL_8N1, 4, 5);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("✅ Fingerprint sensor connected");
  } else {
    Serial.println("❌ Fingerprint sensor not found");
    while (1);
  }

  Serial.println("\nPress:\n[1] to verify fingerprint\n[2] to enroll fingerprint");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "1") {
      Serial.println("➡️ Starting fingerprint verification...");
      if (getFingerprintTemplate()) {
        sendToServer("/api/fingerprint/verify");
      } else {
        Serial.println("❌ Fingerprint capture failed");
      }
    } 
    else if (input == "2") {
      Serial.print("Enter ID to enroll (e.g., 1): ");
      while (!Serial.available());
      String idStr = Serial.readStringUntil('\n');
      idStr.trim();
      int id = idStr.toInt();
      Serial.println("➡️ Enrolling to ID: " + String(id));

      if (getFingerprintTemplate()) {
        String endpoint = "/api/fingerprint/store?id=" + String(id);
        sendToServer(endpoint.c_str());
      } else {
        Serial.println("❌ Fingerprint capture failed");
      }
    }

    Serial.println("\nPress:\n[1] to verify fingerprint\n[2] to enroll fingerprint");
  }
}

bool getFingerprintTemplate() {
  uint8_t p;
  Serial.println("Place finger on sensor...");

  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) {
      delay(100);
      continue;
    } else {
      Serial.println("❌ Error capturing image");
      return false;
    }
  }
  Serial.println("✅ Image captured");

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Image conversion failed");
    return false;
  }

  p = finger.getModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Failed to get model");
    return false;
  }

  uint8_t rawBuf[534];
  memset(rawBuf, 0, sizeof(rawBuf));

  uint32_t start = millis();
  int i = 0;
  while (i < 534 && (millis() - start) < 5000) {
    if (mySerial.available()) {
      rawBuf[i++] = mySerial.read();
    }
  }

  if (i < 534) {
    Serial.println("❌ Template not fully received");
    return false;
  }

  // Extract 512-byte template
  memcpy(templateData, rawBuf + 9, 256);
  memcpy(templateData + 256, rawBuf + 9 + 256 + 2 + 9, 256);

  Serial.println("✅ Template extracted");
  return true;
}

void sendToServer(const char* endpoint) {
  if (!eth_connected) {
    Serial.println("❌ Ethernet not connected");
    return;
  }

  String url = "http://" + String(serverIP) + ":" + String(serverPort) + String(endpoint);
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/octet-stream");

  int httpCode = http.POST(templateData, 512);
  String response = http.getString();

  Serial.print("📡 HTTP Code: ");
  Serial.println(httpCode);
  Serial.print("📝 Server says: ");
  Serial.println(response);

  // Parse optional JSON confidence
  if (httpCode == 200) {
    int confIndex = response.indexOf("\"confidence\":");
    if (confIndex != -1) {
      String confValue = response.substring(confIndex + 13);
      confValue.trim();
      confValue = confValue.substring(0, confValue.indexOf("}") != -1 ? confValue.indexOf("}") : confValue.length());
      Serial.println("📊 Confidence: " + confValue + "%");
    }
  }

  http.end();
}
