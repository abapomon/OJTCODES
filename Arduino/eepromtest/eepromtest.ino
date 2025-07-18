#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <EEPROM.h>

// === Settings ===
#define EEPROM_SIZE 1024
#define TEMPLATE_START 0  // start address in EEPROM

HardwareSerial mySerial(2);  // UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

String inputString = "";  // buffer for incoming ID
bool inputReady = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(100);
  Serial.println("🔍 Fingerprint Template Extractor with EEPROM");
  Serial.println("Enter fingerprint ID (1 to N), then press Enter:");

  EEPROM.begin(EEPROM_SIZE);  // Initialize EEPROM

  mySerial.begin(57600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("✅ Found fingerprint sensor!");
  } else {
    Serial.println("❌ Fingerprint sensor not found");
    while (1);
  }
}

void loop() {
  // Read input from Serial
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      inputReady = true;
      break;
    }
    if (isDigit(inChar)) {
      inputString += inChar;
    }
  }

  if (inputReady && inputString.length() > 0) {
    uint16_t id = inputString.toInt();
    downloadFingerprintTemplate(id);

    inputString = "";
    inputReady = false;
    Serial.println("\nEnter another ID:");
  }
}

uint8_t downloadFingerprintTemplate(uint16_t id) {
  Serial.println("------------------------------------");
  Serial.print("📥 Loading template #"); Serial.println(id);
  uint8_t p = finger.loadModel(id);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("✅ Template loaded");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("❌ Communication error");
      return p;
    default:
      Serial.print("❌ Unknown error: "); Serial.println(p);
      return p;
  }

  Serial.print("📤 Requesting template data for ID #"); Serial.println(id);
  p = finger.getModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Failed to get template");
    return p;
  }

  // Read raw template data (534 bytes)
  uint8_t bytesReceived[534];
  memset(bytesReceived, 0xFF, 534);

  uint32_t starttime = millis();
  int i = 0;
  while (i < 534 && (millis() - starttime) < 20000) {
    if (mySerial.available()) {
      bytesReceived[i++] = mySerial.read();
    }
  }
  Serial.print("📦 Bytes received: "); Serial.println(i);
  if (i != 534) {
    Serial.println("❌ Timeout or incomplete packet");
    return FINGERPRINT_PACKETRECIEVEERR;
  }

  // Extract actual 512 bytes from data packets
  uint8_t fingerTemplate[512];
  memset(fingerTemplate, 0xFF, 512);

  int uindx = 9, index = 0;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);   // packet 1
  uindx += 256 + 2 + 9;
  index += 256;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);   // packet 2

  // Save to EEPROM
  Serial.println("💾 Saving template to EEPROM...");
  for (int i = 0; i < 512; ++i) {
    EEPROM.write(TEMPLATE_START + i, fingerTemplate[i]);
  }
  EEPROM.commit();
  Serial.println("✅ Template saved to EEPROM");

  // Optional: print stored template from EEPROM
  printEEPROMTemplate();

  return p;
}

void printHex(int num, int precision) {
  char tmp[16];
  char format[8];
  sprintf(format, "%%0%dX", precision);
  sprintf(tmp, format, num);
  Serial.print(tmp);
}

void printEEPROMTemplate() {
  Serial.println("🔎 Reading EEPROM-stored template:");
  for (int i = 0; i < 512; ++i) {
    uint8_t val = EEPROM.read(TEMPLATE_START + i);
    Serial.print("0x");
    printHex(val, 2);
    Serial.print(", ");
    if ((i + 1) % 16 == 0) Serial.println();
  }
  Serial.println("✅ EEPROM read complete\n");
}
