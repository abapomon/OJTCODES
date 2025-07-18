// Full working sketch: ESP32 with FPM fingerprint match via LAN
// Uses internal R307 matching with server-provided templates

#include <ETH.h>
#include <WiFi.h>
#include <Wire.h>
#include <fpm.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "base64.h"  // You must create this header

#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    1
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN   16
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18

HardwareSerial fserial(2);
FPM finger(&fserial);
FPMSystemParams params;

const char* serverIP = "192.168.2.106";
const int serverPort = 3000;
bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_GOT_IP:
      eth_connected = true;
      Serial.print("\n✅ IP: "); Serial.println(ETH.localIP());
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
  while (!eth_connected) delay(500);

  fserial.begin(57600, SERIAL_8N1, 4, 5);
  if (!finger.begin()) {
    Serial.println("❌ Sensor not found");
    while (1);
  }
  finger.readParams(&params);
  Serial.println("✅ Fingerprint sensor ready");
}

void loop() {
  Serial.println("\n👉 Place your finger...");
  if (!captureToBuffer1()) {
    Serial.println("❌ Capture failed");
    delay(3000);
    return;
  }

  verifyAgainstServerTemplates();
  delay(5000);
}

bool captureToBuffer1() {
  FPMStatus status;
  for (int i = 0; i < 10; i++) {
    status = finger.getImage();
    if (status == FPMStatus::NOFINGER) {
      delay(300);
    } else if (status == FPMStatus::OK) {
      Serial.println("✅ Image captured");
      break;
    } else {
      Serial.printf("⚠️ getImage error: 0x%X\n", static_cast<uint16_t>(status));
      return false;
    }
  }

  status = finger.image2Tz(1);
  if (status != FPMStatus::OK) {
    Serial.printf("❌ image2Tz error: 0x%X\n", static_cast<uint16_t>(status));
    return false;
  }
  Serial.println("✅ Features extracted to Buffer 1");
  return true;
}

void verifyAgainstServerTemplates() {
  if (!eth_connected) {
    Serial.println("❌ Ethernet not connected");
    return;
  }

  HTTPClient http;
  String url = "http://" + String(serverIP) + ":" + String(serverPort) + "/api/fingerprint/templates";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.println("❌ Failed to fetch templates");
    return;
  }

  String response = http.getString();
  http.end();

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    Serial.println("❌ JSON parse error");
    return;
  }

  JsonArray templates = doc["templates"];
  for (JsonObject tpl : templates) {
    const char* id = tpl["id"];
    const char* encoded = tpl["template"];

    int decodedLen = base64_dec_len(encoded, strlen(encoded));
    uint8_t decoded[512];
    base64_decode((char*)decoded, encoded, strlen(encoded));

    FPMStatus status = finger.writeTemplateToSensor(2, decoded, 512);
    if (status != FPMStatus::OK) {
      Serial.printf("❌ Failed to load template ID %s\n", id);
      continue;
    }

    uint16_t score = 0;
    status = finger.matchTemplatePair(&score);
    if (status == FPMStatus::OK) {
      Serial.printf("✅ MATCH! ID: %s | Confidence: %u\n", id, score);
      return;
    }
  }

  Serial.println("❌ No match found");
}
