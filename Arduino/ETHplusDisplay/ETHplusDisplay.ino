#include <ETH.h>
#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>

// OLED on I2C (GPIO32 = SCL, GPIO33 = SDA)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, 32, 33);

// WT32-ETH01-S1 Ethernet config
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    1
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN   16
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18

bool eth_connected = false;
String ipString = "Getting IP...";

// Handle Ethernet Events
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH IP: ");
      Serial.println(ETH.localIP());
      eth_connected = true;
      ipString = ETH.localIP().toString();
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      ipString = "Disconnected";
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      ipString = "Stopped";
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Start OLED display
  Wire.begin(33, 32);  // SDA, SCL (reversed because U8g2 expects that order)
  u8g2.begin();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 20, "Starting ETH...");
  u8g2.sendBuffer();

  WiFi.onEvent(WiFiEvent);  // Register Ethernet event handler

  // Start Ethernet
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN,
            ETH_POWER_PIN, ETH_CLK_MODE);
}

void drawDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(0, 20, "Ethernet Status:");
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 40, eth_connected ? "CONNECTED" : "DISCONNECTED");
  u8g2.drawStr(0, 55, ipString.c_str());
  u8g2.sendBuffer();
}

void loop() {
  drawDisplay();
  delay(1000);
}
