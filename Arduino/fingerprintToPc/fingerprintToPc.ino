#include <HardwareSerial.h>
#include <fpm.h>

HardwareSerial fserial(2);  // UART2: RX=16, TX=17 (adjust if needed)
FPM finger(&fserial);
FPMSystemParams params;

void setup() {
  Serial.begin(57600);  // USB Serial to PC
  fserial.begin(57600, SERIAL_8N1, 5, 17);  // Sensor UART: RX=5, TX=17

  Serial.println("🔍 Starting FPM fingerprint image example");

  if (finger.begin()) {
    Serial.println("✅ Sensor connected");
    finger.readParams(&params);
    Serial.print("Capacity: "); Serial.println(params.capacity);
  } else {
    Serial.println("❌ Sensor not found");
    while (true);
  }
}

void loop() {
  captureImageToPC();
  delay(5000);
}

void captureImageToPC() {
  Serial.println("\n👉 Place your finger on the sensor...");

  while (finger.getImage() != FPMStatus::OK) {
    delay(100);
  }
  Serial.println("✅ Image taken");

  if (finger.downloadImage() != FPMStatus::OK) {
    Serial.println("❌ Failed to download image");
    return;
  }

  Serial.write(0xAA);  // Start marker for sync

  uint16_t readLen = 0;
  uint32_t totalRead = 0;
  bool readComplete = false;

  while (!readComplete) {
    bool ok = finger.readDataPacket(NULL, &Serial, &readLen, &readComplete);
    if (!ok) {
      Serial.println("❌ Failed to read packet");
      return;
    }
    totalRead += readLen;
  }

  Serial.println();
  Serial.print("📦 Total bytes sent: ");
  Serial.println(totalRead);
}
