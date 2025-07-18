#include <ETH.h>
#include <WiFi.h>
#include <WebServer.h>

// WT32-ETH01-S1 Config
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    1
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN   16
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18

WebServer server(80);

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
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      break;
    default:
      break;
  }
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <html>
    <body>
      <h1>ESP32 WebServer Test</h1>
      <div id="txtRandomData">Unknown</div>
      <input type="button" value="random" onclick="getData()">
      <script>
        function getData() {
          fetch('/getRandomData')
            .then(response => response.text())
            .then(data => {
              document.getElementById('txtRandomData').innerText = data;
            });
        }
      </script>
    </body>
    </html>
  )rawliteral");
}

void handleAjax() {
  server.send(200, "text/plain", "Random data: " + String(random(10000)));
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  WiFi.onEvent(WiFiEvent);

  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);

  server.on("/", handleRoot);
  server.on("/getRandomData", HTTP_GET, handleAjax);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
