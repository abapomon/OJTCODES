//USES DEPRECATED MOBIZT FRIEBASE CLIENT VER. 4.3.7

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Replace with your network credentials
#define WIFI_SSID "Geckotech Intern ^_^"
#define WIFI_PASSWORD "Jeremiah33:3"

// Replace with your Firebase project details
#define API_KEY "AIzaSyDbaurkiboHGoywyO27pHtEjl-c_3-Woso"
#define FIREBASE_PROJECT_ID "esp8266-chat-fa268"
#define USER_EMAIL "ryan@gmail.com"
#define USER_PASSWORD "Ryan4the3"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Timer variables
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000; // 10 seconds (10000 milliseconds)

// Function prototypes
void setupWiFi();
void setupFirebase();
void sendRandomPersonData();
String randomName();
int randomAge();
String randomGender();
String randomOccupation();
float randomHeight();
float randomWeight();
String generateRandomId();
String getFirestoreTimestamp();

void setup() {
  Serial.begin(115200);
  
  setupWiFi();
  setupFirebase();
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    sendRandomPersonData();
  }
}

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void setupFirebase() {
  // Assign the API key
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void sendRandomPersonData() {
  // Generate a random document ID
  String documentPath = "people/" + generateRandomId();
  
  // Create a FirebaseJson object to hold our data
  FirebaseJson content;

  // Add random person data to the JSON object
  content.set("fields/name/stringValue", randomName());
  content.set("fields/age/integerValue", String(randomAge()));
  content.set("fields/gender/stringValue", randomGender());
  content.set("fields/occupation/stringValue", randomOccupation());
  content.set("fields/height/doubleValue", String(randomHeight()));
  content.set("fields/weight/doubleValue", String(randomWeight()));
  content.set("fields/timestamp/timestampValue", getFirestoreTimestamp());

  Serial.println("Creating a document...");
  
  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
    Serial.println("Document created successfully");
    Serial.print("Document Path: ");
    Serial.println(fbdo.payload());
  } else {
    Serial.println(fbdo.errorReason());
  }
}

String getFirestoreTimestamp() {
  // Get current time as time_t (Unix timestamp)
  time_t now = Firebase.getCurrentTime();
  
  // Convert to tm struct
  struct tm *timeinfo;
  timeinfo = localtime(&now);
  
  // Format as Firestore timestamp string (RFC 3339 format)
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
  
  return String(buffer);
}

String randomName() {
  String firstNames[] = {"Emma", "Liam", "Olivia", "Noah", "Ava", "William", "Sophia", "James", "Isabella", "Oliver"};
  String lastNames[] = {"Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis", "Rodriguez", "Martinez"};
  
  return firstNames[random(0, 10)] + " " + lastNames[random(0, 10)];
}

int randomAge() {
  return random(18, 80);
}

String randomGender() {
  return (random(0, 2) == 0) ? "Male" : "Female";
}

String randomOccupation() {
  String occupations[] = {"Doctor", "Engineer", "Teacher", "Artist", "Chef", "Developer", "Nurse", "Architect", "Writer", "Designer"};
  return occupations[random(0, 10)];
}

float randomHeight() {
  // Height in meters (1.5m to 2.0m)
  return 1.5 + (random(0, 51) / 100.0);
}

float randomWeight() {
  // Weight in kg (50kg to 100kg)
  return 50 + random(0, 51);
}

String generateRandomId() {
  String chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String id = "";
  
  for (int i = 0; i < 20; i++) {
    id += chars[random(0, chars.length())];
  }
  
  return id;
}