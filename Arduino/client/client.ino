#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

HardwareSerial mySerial(2);  // RX=5, TX=17
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const char* ssid = "abapomons";
const char* password = "abapomon";
const char* serverIP = "192.168.52.133";  // Your PC IP
const int serverPort = 3000;              // Default Next.js port

uint8_t templateData[512];

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  mySerial.begin(57600, SERIAL_8N1, 5, 17);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("✅ Fingerprint sensor connected");
  } else {
    Serial.println("❌ Fingerprint sensor not found");
    while (1);
  }

  Serial.println("\nPress:");
  Serial.println("[1] to verify fingerprint");
  Serial.println("[2] to enroll fingerprint");
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
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
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

  // Parse JSON response to extract confidence
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

