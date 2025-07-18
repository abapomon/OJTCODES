#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE       2048        // Enough for 4 templates
#define TEMPLATE_SIZE     512         // One fingerprint template = 512 bytes
#define MAX_USERS         2           // You can increase this based on EEPROM size
#define TEMPLATE_ADDR(i)  (i * TEMPLATE_SIZE)

const char* ssid = "abapomons";
const char* password = "abapomon";

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }

  Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());

  // Endpoint to save template for a specific user (e.g., /enroll?id=0)
  server.on("/enroll", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!request->hasParam("id")) {
        request->send(400, "text/plain", "❌ Missing user ID");
        return;
      }

      int id = request->getParam("id")->value().toInt();
      if (id < 0 || id >= MAX_USERS) {
        request->send(400, "text/plain", "❌ Invalid ID");
        return;
      }

      if (len != TEMPLATE_SIZE) {
        request->send(400, "text/plain", "❌ Invalid template size");
        return;
      }

      int addr = TEMPLATE_ADDR(id);
      for (int i = 0; i < TEMPLATE_SIZE; i++) {
        EEPROM.write(addr + i, data[i]);
      }
      EEPROM.commit();

      Serial.printf("📥 Template stored for ID %d\n", id);
      request->send(200, "text/plain", "✅ Template saved");
    }
  );

  // Endpoint to verify a fingerprint template
  server.on("/verify", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (len != TEMPLATE_SIZE) {
        request->send(400, "text/plain", "❌ Invalid template size");
        return;
      }

      // Compare against all stored templates
      for (int id = 0; id < MAX_USERS; id++) {
        bool match = true;
        int addr = TEMPLATE_ADDR(id);

        for (int i = 0; i < TEMPLATE_SIZE; i++) {
          uint8_t stored = EEPROM.read(addr + i);
          if (stored != data[i]) {
            match = false;
            break;
          }
        }

        if (match) {
          String msg = "✅ Match! ID: " + String(id);
          Serial.println(msg);
          request->send(200, "text/plain", msg);
          return;
        }
      }

      Serial.println("❌ No match found");
      request->send(200, "text/plain", "❌ No match found");
    }
  );

  // Optional: Check what's saved in EEPROM (for debugging)
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    String out = "Saved templates (first 16 bytes each):\n";
    for (int id = 0; id < MAX_USERS; id++) {
      out += "ID " + String(id) + ": ";
      for (int i = 0; i < 16; i++) {
        uint8_t val = EEPROM.read(TEMPLATE_ADDR(id) + i);
        char hex[6];
        sprintf(hex, "%02X ", val);
        out += String(hex);
      }
      out += "\n";
    }
    request->send(200, "text/plain", out);
  });

  server.begin();
  Serial.println("🌐 Async HTTP server started");
}

void loop() {
  // Nothing needed for AsyncWebServer
}
