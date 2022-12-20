// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "time.h"

//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP);

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "UGA_Visitors_WiFi"
#define WIFI_PASSWORD ""

AsyncWebServer server(80);

#define API_KEY "AIzaSyAVSZIbJleD8ZO1fIKI4_SojjMacPwPBQk"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL ""
#define USER_PASSWORD ""

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://rpi-dataset-default-rtdb.firebaseio.com/"


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;
String formattedDate;

String tempPath = "/temperature";
String presPath = "/pressure";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

Adafruit_BMP085 bmp;
float temperature;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1800       /* Time ESP32 will go to sleep (in seconds) */ 


void setup(void){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Running");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  Wire.begin(32,33); 
  if (!bmp.begin()) {
	Serial.println("Could not find a valid BMP085/BMP180 sensor, check wiring!");
	while (1) {}
  }

  configTime(0, 0, ntpServer);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);

  // Update database path
  //databasePath = "/UsersData/" + uid + "/readings";
  databasePath = "/frank_arroyo";

  // Initialize a NTPClient to get time
  //timeClient.begin();
  //timeClient.setTimeOffset(3600);
  while (Firebase.ready()){
    timestamp = getTime();
    Serial.println ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);
    json.set(tempPath.c_str(), String(bmp.readTemperature()));
    json.set(presPath.c_str(), String(bmp.readPressure()));
    json.set(timePath, String(timestamp));
    json.set("IPAddress", String(WiFi.localIP()));

    //Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str();
    Serial.println("Data Saved ");
    //delay(30000);
    break;
    
  }
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop(){
  
}


