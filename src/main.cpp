#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <AccelStepper.h>

#include <fileIO.h>
#include <wifi_setup.h>
#include <webserver_callbacks.h>


// Constants
#define dirPin 2
#define stepPin 3

// Globals
AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");
AccelStepper stepper = AccelStepper( AccelStepper::DRIVER, stepPin, dirPin);
unsigned long cleanupClientsTime = 0;

float stepsPerFullRotation = 200 * 4 * 4 * 120;
/*
TODO:
startup sound
tracking with sleeps
hold on/off -- with ~4 steps sec this is meaningless, maybe when tracking with sleep
*/
bool  startupTone = true;
bool  holdOn = true;
bool  sleepOn = false;
float sleepLength = 0;

bool  trackingOnCurrent = true;
bool  trackingOnTarget  = true;
bool  movingOn = false;


/***********************************************************
 * Functions
 */

void sendFullState(){
  StaticJsonDocument<400> doc;
  doc["startupTone"] = startupTone;
  doc["holdOn"]      = holdOn;
  doc["sleepOn"]     = sleepOn;
  doc["sleepLength"] = sleepLength;
  doc["nFullSteps"]  = stepsPerFullRotation;
  doc["trackerState"]  = (trackingOnTarget) ? "ON" : "OFF";
  doc["type"]  = "fullStatus";

  String ssid_wifi = readFile(SPIFFS, "/ssid_wifi.txt");
  if ( ssid_wifi != "" ) doc["ssidWifi"] = ssid_wifi;

  String serializedJSON;
  serializeJson(doc, serializedJSON);

  webSocket.textAll((char*) serializedJSON.c_str());
}

void sendTrackerState(){
  StaticJsonDocument<200> doc;
  doc["trackerState"]  = (trackingOnTarget) ? "ON" : "OFF";
  doc["type"]  = "trackerState";

  String serializedJSON;
  serializeJson(doc, serializedJSON);

  webSocket.textAll((char*) serializedJSON.c_str());
}

// Callback: receiving any WebSocket message
void onWebSocketEvent(
                      AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type, 
                      void *arg, 
                      uint8_t *payload, 
                      size_t length
                      ) {

  // Figure out the type of WebSocket event
  switch(type) {
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DATA:
    {

      StaticJsonDocument<1000> doc;
      deserializeJson(doc, payload);
      if (doc.containsKey("type")) {
        Serial.printf("[%u] Sent text: %s\n", client->id(), payload);
        Serial.printf("Message does not contain type!");
        break;
      }
      
      const char* mytype = doc["type"];
      Serial.printf("Decoded json [%u]: %s\n", client->id(), mytype);

      if ( strcmp((char *)mytype,        "trackerOn") == 0 ) {
        Serial.printf("trackerOn");

        stepper.setSpeed( stepsPerFullRotation / (24*60*60) );
        trackingOnCurrent = true;
        trackingOnTarget  = true;
        sendTrackerState();

      } else if ( strcmp((char *)mytype, "trackerOff") == 0 ) {
        Serial.printf("trackerOff");

        // // // first, if you are using runSpeed(), stop() does not do anything. 
        // // // to stop the stepper use stapler.setSpeed(0) or stop calling stapler.runSpeed()
        // // // stepper.stop();
        stepper.setSpeed( 0 );
        trackingOnTarget = false;
        sendTrackerState();

      } else if ( strcmp((char *)mytype, "stepDegrees") == 0 ) {
        Serial.printf("stepDegrees");

        trackingOnCurrent = false;
        // trackingOnTarget remains and resumes after moving if it was on
        stepper.move( stepsPerFullRotation * doc["value"] / 360.0 );
        movingOn = true;

      } else if ( strcmp((char *)mytype, "stepSteps") == 0 ) {
        Serial.printf("stepSteps");

        trackingOnCurrent = false;
        trackingOnTarget  = false;
        stepper.move( doc["value"] );
        movingOn = true;

      } else if ( strcmp((char *)mytype, "trackerSTOP") == 0 ) {
        Serial.printf("trackerSTOP");
        
        trackingOnCurrent = false;
        trackingOnTarget  = false;
        movingOn = false;

      } else if ( strcmp((char *)mytype, "settings") == 0 ) {
        Serial.printf("settings");

        startupTone          = doc["startupTone"];
        holdOn               = doc["holdOn"];
        sleepOn              = doc["sleepOn"];
        sleepLength          = doc["sleepLength"];
        stepsPerFullRotation = doc["nFullSteps"];

        if ( doc.containsKey("ssidWifi") && doc.containsKey("pwdWifi") ){
          wrieFile(SPIFFS, "/ssid_wifi.txt", doc["ssidWifi"]);
          wrieFile(SPIFFS, "/pwd_wifi.txt", doc["pwdWifi"]);
        }

        stepper.setSpeed( stepsPerFullRotation / (24*60*60) );

        sendFullState();

      } else if ( strcmp((char *)mytype, "fullState") == 0 ) {
        Serial.println("Returning full state");
        sendFullState();
      } else {
        Serial.println("Message not recognized");
      }
      break;
    }
    default:
      break;
  }
}

/***********************************************************
 * Main
 */
void setup() {
  
  // Start Serial port
  Serial.begin(115200);
  Serial.println("Hi. This is project Astro Lite Tracker.");

  // Make sure we can read the file system
  if( !SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }

  setup_wifi();
  advertise_local_address();

  String temp = readFile(SPIFFS, "/stepsPerFullRotation.txt");
  if ( temp != "" ) stepsPerFullRotation = temp.toFloat();
  stepper.setMaxSpeed(2000);
  stepper.setAcceleration(100.0);
  stepper.setSpeed( stepsPerFullRotation / (24*60*60) );
  // speed	The desired constant speed in steps per second. 
  // Positive is clockwise. Speeds of more than 1000 steps per second are unreliable. 
  // Very slow speeds may be set (eg 0.00027777 for once per hour, approximately. 
  // Speed accuracy depends on the Arduino crystal. Jitter depends on how frequently you call the runSpeed() function. 
  // The speed will be limited by the current value of setMaxSpeed()

  // On HTTP request 
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/style.css", HTTP_GET, onCSSRequest);
  server.on("/index.js", HTTP_GET, onJSRequest);
  server.on("/favicon.ico", HTTP_GET, onFavRequest);
  server.onNotFound(onPageNotFound);

  webSocket.onEvent(onWebSocketEvent);

  // Start web server
  server.addHandler(&webSocket);
  server.begin();
}

void loop() {
  if ((unsigned long)(millis() - cleanupClientsTime) >= 3000){ // handles rollover fine
    cleanupClientsTime = millis();
    webSocket.cleanupClients();
  }  

  if (trackingOnCurrent){
    stepper.runSpeed();
    if ( ! trackingOnTarget && stepper.speed() == 0.0) trackingOnCurrent = false;
  } 
  else if (movingOn){
    stepper.run();
    movingOn = abs(stepper.distanceToGo())>0; // stop when at target
  }
  else if ( trackingOnTarget ) trackingOnCurrent = true;    

}