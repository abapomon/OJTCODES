#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "Geckotech Intern ^_^";
const char* password = "Jeremiah33:3";

ESP8266WebServer server(80);
const int triggerPin = 5; // D1 = GPIO5

// Static IP configuration
IPAddress local_IP(192, 168, 2, 123);      // desired static IP
IPAddress gateway(192, 168, 2, 1);         // your router's IP
IPAddress subnet(255, 255, 255, 0);        // subnet mask
IPAddress primaryDNS(8, 8, 8, 8);          // optional
IPAddress secondaryDNS(8, 8, 4, 4);        // optional

void handleTrigger() {
  digitalWrite(triggerPin, HIGH);
  delay(1000);
  digitalWrite(triggerPin, LOW);
  server.sendHeader("Access-Control-Allow-Origin", "*");  // 👈 this is critical
  server.send(200, "text/plain", "Triggered");
}
void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
  server.send(204); // No Content
}

void setup() {
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  Serial.begin(115200);

  // Apply the static IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
  Serial.print("Assigned IP: ");
  Serial.println(WiFi.localIP());
  server.on("/trigger", HTTP_GET, handleTrigger);
  server.on("/trigger", HTTP_OPTIONS, handleOptions); 
  server.on("/trigger", handleTrigger);
  server.begin();
  Serial.println("HTTP server started");
  
}

void loop() {
  server.handleClient();
}
