#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

HardwareSerial mySerial(2);  // UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

String inputString = "";
bool inputReady = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(100);
  Serial.println("📤 Fingerprint template extractor");
  Serial.println("Enter fingerprint ID (e.g., 1, 4) and press Enter:");

  mySerial.begin(57600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("✅ Found fingerprint sensor!");
  } else {
    Serial.println("❌ Did not find fingerprint sensor :(");
    while (1);
  }
}

void loop() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      inputReady = true;
      break;
    }
    if (isDigit(inChar)) inputString += inChar;
  }

  if (inputReady && inputString.length() > 0) {
    uint16_t id = inputString.toInt();
    flushSerial(); // important before communication
    downloadFingerprintTemplate(id);

    inputString = "";
    inputReady = false;
    Serial.println("\nEnter another ID:");
  }
}

void flushSerial() {
  while (mySerial.available()) mySerial.read();
}

uint8_t downloadFingerprintTemplate(uint16_t id) {
  Serial.println("------------------------------------");
  Serial.print("Attempting to load #"); Serial.println(id);

  uint8_t p = finger.loadModel(id);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("✅ Template loaded");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("❌ Communication error");
      return p;
    default:
      Serial.print("❌ Error: "); Serial.println(p);
      return p;
  }

  Serial.print("Transferring template for ID "); Serial.println(id);
  p = finger.getModel();
  if (p != FINGERPRINT_OK) {
    Serial.print("❌ getModel error: "); Serial.println(p);
    return p;
  }

  uint8_t bytesReceived[534];
  memset(bytesReceived, 0xFF, sizeof(bytesReceived));

  uint32_t start = millis();
  int i = 0;
  while (i < 534 && (millis() - start) < 2000) {
    if (mySerial.available()) {
      bytesReceived[i++] = mySerial.read();
    }
  }
  Serial.print(i); Serial.println(" bytes read.");

  if (i < 534) {
    Serial.println("❌ Incomplete template read");
    return FINGERPRINT_PACKETRECIEVEERR;
  }

  Serial.println("✅ Decoding template...");

  uint8_t fingerTemplate[512];
  memset(fingerTemplate, 0xFF, sizeof(fingerTemplate));

  int uindx = 9, index = 0;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);
  uindx += 256 + 2 + 9;
  index += 256;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);

  for (int i = 0; i < 512; ++i) {
    Serial.print("0x");
    printHex(fingerTemplate[i], 2);
    Serial.print(", ");
    if ((i + 1) % 16 == 0) Serial.println();
  }

  Serial.println("\n✅ Done.\n");

  delay(500); // allow buffer to clear
  flushSerial(); // clean up

  return p;
}

void printHex(int num, int precision) {
  char tmp[16];
  sprintf(tmp, "%0*X", precision, num);
  Serial.print(tmp);
}
