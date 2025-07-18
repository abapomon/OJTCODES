#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

// Button pins
#define BTN_ENROLL 12
#define BTN_DELETE 14
#define BTN_BACK   27

// Fingerprint sensor on Serial2 (GPIO 16 RX, 17 TX)
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

enum Mode { SCAN, ENROLL, DELETE };
Mode currentMode = SCAN;
uint8_t id;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(100);

  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  // Setup buttons
  pinMode(BTN_ENROLL, INPUT_PULLUP);
  pinMode(BTN_DELETE, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  Serial.println("\n🏢 Company Fingerprint Attendance System");

  if (finger.verifyPassword()) {
    Serial.println("✅ Fingerprint sensor detected.");
  } else {
    Serial.println("❌ Sensor not found. Check wiring.");
    while (1);
  }

  finger.getTemplateCount();
  Serial.print("🧠 Stored fingerprints: ");
  Serial.println(finger.templateCount);
}

void loop() {
  checkButtons();

  switch (currentMode) {
    case SCAN:
      logAttendance();
      break;

    case ENROLL:
      Serial.println("\n🔐 ENROLL MODE");
      Serial.println("Enter ID to enroll (1–127). Press BACK to cancel:");
      id = readnumberWithCancel();
      if (id == 0) {
        Serial.println("❎ Enroll cancelled.");
      } else if (id > 127) {
        Serial.println("⚠️ Invalid ID.");
      } else {
        enrollFingerprint(id);
      }
      currentMode = SCAN;
      break;

    case DELETE:
      Serial.println("\n🧹 DELETE MODE");
      Serial.println("Enter ID to delete (1–127). Press BACK to cancel:");
      id = readnumberWithCancel();
      if (id == 0) {
        Serial.println("❎ Delete cancelled.");
      } else {
        deleteFingerprint(id);
      }
      currentMode = SCAN;
      break;
  }
}

// ===================== INPUT HANDLING =====================

void checkButtons() {
  if (digitalRead(BTN_ENROLL) == LOW) {
    currentMode = ENROLL;
    delay(500);
  }
  if (digitalRead(BTN_DELETE) == LOW) {
    currentMode = DELETE;
    delay(500);
  }
  if (digitalRead(BTN_BACK) == LOW) {
    currentMode = SCAN;
    Serial.println("🔙 Back to scan mode");
    delay(500);
  }
}

uint8_t readnumberWithCancel() {
  String input = "";
  while (true) {
    if (digitalRead(BTN_BACK) == LOW) {
      Serial.println("\n🔙 Back button pressed. Cancelling...");
      delay(500);
      return 0;
    }

    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          return input.toInt();
        }
      } else {
        input += c;
        Serial.print(c);
      }
    }
  }
}

// ===================== ENROLL =====================

bool enrollFingerprint(uint8_t id) {
  int p = -1;
  Serial.println("➡️ Place your finger");

  while (p != FINGERPRINT_OK) {
    if (digitalRead(BTN_BACK) == LOW) {
      Serial.println("\n🔙 Cancelled.");
      delay(500);
      return false;
    }

    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p != FINGERPRINT_OK) {
      Serial.println("❌ Image error");
      return false;
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return false;

  Serial.println("🆗 Remove finger...");
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("➡️ Place same finger again");
  while (finger.getImage() != FINGERPRINT_OK) {
    if (digitalRead(BTN_BACK) == LOW) {
      Serial.println("\n🔙 Cancelled.");
      delay(500);
      return false;
    }
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return false;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Fingerprints did not match");
    return false;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.print("✅ Stored ID ");
    Serial.println(id);
    return true;
  } else {
    Serial.println("❌ Store failed");
    return false;
  }
}

// ===================== SCAN =====================

void logAttendance() {
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) return;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("✅ Match found!");
    Serial.print("👤 ID: "); Serial.println(finger.fingerID);
    Serial.print("📊 Confidence: "); Serial.println(finger.confidence);
    delay(2000);  // Debounce delay to prevent double scan
  } else {
    Serial.println("❌ Finger not recognized");
    delay(1000);
  }
}

// ===================== DELETE =====================

void deleteFingerprint(uint8_t id) {
  int p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.print("🗑️ Deleted ID ");
    Serial.println(id);
  } else {
    Serial.println("❌ Failed to delete (maybe ID doesn’t exist)");
  }
}
