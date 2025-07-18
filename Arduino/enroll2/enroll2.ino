#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

HardwareSerial mySerial(2);  // UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

void setup()
{
  Serial.begin(115200);
  while (!Serial);  // Wait for USB Serial
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");

  // Begin UART2 on GPIO 16 (RX), 17 (TX)
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop()
{
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  id = readnumber();
  if (id == 0) return;

  Serial.print("Enrolling ID #");
  Serial.println(id);

  while (! getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) { Serial.print("."); continue; }
    else if (p == FINGERPRINT_PACKETRECIEVEERR) { Serial.println("Communication error"); continue; }
    else if (p == FINGERPRINT_IMAGEFAIL) { Serial.println("Imaging error"); continue; }
    else if (p != FINGERPRINT_OK) { Serial.println("Unknown error"); continue; }
    Serial.println("Image taken");
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Image conversion failed"); return p;
  }
  Serial.println("Image converted");

  Serial.println("Remove finger");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("Place same finger again");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) { Serial.print("."); continue; }
    else if (p == FINGERPRINT_PACKETRECIEVEERR) { Serial.println("Communication error"); continue; }
    else if (p == FINGERPRINT_IMAGEFAIL) { Serial.println("Imaging error"); continue; }
    else { Serial.println("Unknown error"); continue; }
  }
  Serial.println("Image taken");

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Second image conversion failed"); return p;
  }
  Serial.println("Image converted");

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Fingerprints did not match or error"); return p;
  }
  Serial.println("Prints matched!");

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) Serial.println("Stored!");
  else Serial.println("Failed to store!");

  return true;
}
