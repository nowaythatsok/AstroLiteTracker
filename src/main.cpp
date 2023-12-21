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
#define STEPS 200
#define MICROSTEPS 16
#define GEARBOX 4*4
#define CLEANUPMILLIS 3000
#define dirPin 12 //2
#define stepPin 14 //4

#define SIDERALHOURS 23.9344696
#define SOLARHOURS 24 //synodic day 
#define LUNARHOURS 24*(1 + 1/27.3)

// Globals
AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");
AccelStepper stepper = AccelStepper( AccelStepper::DRIVER, stepPin, dirPin);
unsigned long cleanupClientsTime = 0;

float stepsPerFullRotation = STEPS * MICROSTEPS * GEARBOX * 120;
/*
TODO:
startup sound
tracking with sleeps
hold on/off -- with ~4 steps sec this is meaningless, maybe when tracking with sleep
when not working, maybe the motor power should be disconnected
disableOutputs(), enableOutputs() IF the enable pin is defined
setEnablePin (uint8_t enablePin=0xff)
microstep pins?
safety timer?
safety angle limits?
*/
bool  startupTone = true;
bool  holdOn = true;
bool  sleepOn = false;
float sleepLength = 0;

bool  trackingOnCurrent = true;
bool  trackingOnTarget  = true;
bool  movingOn = false;

long loopCounter1 = 0;
long loopCounter2 = 0;

char* trackingMode = "sidereal";


/***********************************************************
 * Functions
 */


void setSpeed(){
  if      (strcmp(trackingMode,  "sidereal") == 0) stepper.setSpeed( stepsPerFullRotation / (SIDERALHOURS*60*60) );
  else if (strcmp(trackingMode,  "lunar") == 0)    stepper.setSpeed( stepsPerFullRotation / (LUNARHOURS*60*60) );
  else if (strcmp(trackingMode,  "solar") == 0)    stepper.setSpeed( stepsPerFullRotation / (SOLARHOURS*60*60) );
  else Serial.printf("Unrecognized trackingMode: %s", trackingMode);
}

void sendFullState(){
  StaticJsonDocument<500> doc;
  doc["startupTone"] = startupTone;
  doc["holdOn"]      = holdOn;
  doc["sleepOn"]     = sleepOn;
  doc["sleepLength"] = sleepLength;
  doc["nFullSteps"]  = stepsPerFullRotation;
  doc["trackerState"]  = (trackingOnTarget) ? "ON" : "OFF";
  doc["trackingMode"]  = trackingMode;
  doc["type"]  = "fullState";

  String ssid_wifi = readFile(SPIFFS, "/ssid_wifi.txt");
  if ( ssid_wifi != "" ) doc["ssidWifi"] = ssid_wifi;

  String serializedJSON;
  serializeJson(doc, serializedJSON);

  webSocket.textAll((char*) serializedJSON.c_str());
}

void sendTrackerState(){
  StaticJsonDocument<200> doc;
  doc["trackerState"]  = (trackingOnTarget) ? "ON" : "OFF";
  doc["trackingMode"]  = trackingMode;
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
      Serial.println("Sending a full state");
      sendFullState();
      break;
    case WS_EVT_DATA:
    {

      StaticJsonDocument<1000> doc;
      deserializeJson(doc, payload, length);
      if (! doc.containsKey("type")) {
        Serial.printf("[%u] Sent text: %s\n", client->id(), String(payload, length).c_str() );
        Serial.printf("Claimed length: %u\n", length);
        Serial.println("Message does not contain type!");
        break;
      }
      
      const char* mytype = doc["type"];
      Serial.printf("Decoded json [%u]: %s\n", client->id(), mytype);

      if ( strcmp((char *)mytype,        "trackerOn") == 0 ) {
        Serial.printf("trackerOn");

        trackingMode  = (char *)doc["trackingMode"];
        setSpeed();
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
        stepper.move( long(stepsPerFullRotation * float(doc["value"]) / 360.0) );
        movingOn = true;

      } else if ( strcmp((char *)mytype, "stepSteps") == 0 ) {
        Serial.printf("stepSteps");

        trackingOnCurrent = false;
        trackingOnTarget  = false;
        stepper.move( long(doc["value"]) );
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
        trackingMode  = (char *)doc["trackingMode"];
        setSpeed();

        if ( doc.containsKey("ssidWifi") && doc.containsKey("pwdWifi") ){
          wrieFile(SPIFFS, "/ssid_wifi.txt", doc["ssidWifi"]);
          wrieFile(SPIFFS, "/pwd_wifi.txt", doc["pwdWifi"]);
        }

        wrieFile(SPIFFS, "/stepsPerFullRotation.txt", String(stepsPerFullRotation));

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
  uint32_t f = getCpuFrequencyMhz();
  Serial.printf("Freq: %i. This should be 240 MHz or as high as can be.\n", f);

  // Make sure we can read the file system
  if( !SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }

  setup_wifi();
  advertise_local_address();

  String temp = readFile(SPIFFS, "/stepsPerFullRotation.txt");
  if ( temp != "" ) stepsPerFullRotation = temp.toFloat();
  stepper.setMaxSpeed( STEPS * MICROSTEPS * GEARBOX * 2 );
  stepper.setAcceleration( STEPS * MICROSTEPS * GEARBOX );
  setSpeed();

  // for testing:
  stepper.setSpeed( 10000 );
  // speed	The desired constant speed in steps per second. 
  // Positive is clockwise. Speeds of more than 1000 steps per second are unreliable. 
  // Very slow speeds may be set (eg 0.00027777 for once per hour, approximately. 
  // Speed accuracy depends on the Arduino crystal. Jitter depends on how frequently you call the runSpeed() function. 
  // The speed will be limited by the current value of setMaxSpeed()

  // On HTTP request 
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/style.css", HTTP_GET, onCSSRequest);
  server.on("/radio.css", HTTP_GET, onCSS2Request);
  server.on("/index.js", HTTP_GET, onJSRequest);
  server.on("/favicon.ico", HTTP_GET, onFavRequest);
  server.onNotFound(onPageNotFound);

  webSocket.onEvent(onWebSocketEvent);

  // Start web server
  server.addHandler(&webSocket);
  server.begin();
}

void loop() {
  loopCounter1 ++;

  if ((unsigned long)(millis() - cleanupClientsTime) >= CLEANUPMILLIS){ // handles rollover fine
    
    webSocket.cleanupClients();
    Serial.printf("Loop frequency: %i kHz and %i kHz\n", loopCounter1/CLEANUPMILLIS, loopCounter2/CLEANUPMILLIS);// 3 seconds!

    loopCounter1 = 0;
    loopCounter2 = 0;
    cleanupClientsTime = millis();
  }  

  loopCounter2 ++;

  if (trackingOnCurrent){
    stepper.runSpeed();
    if ( ! trackingOnTarget && stepper.speed() == 0.0) trackingOnCurrent = false;
  } 
  else if (movingOn){
    for (int i = 0; i<MICROSTEPS; i++) stepper.run();
    /*
    RTOS is very greedy and runs other threads on CORE1 before and after the loop. 
    This can make it difficult to approach the max available speed. 
    By default @240 MHz an almost empty loop runs only at 100 kHz.
    By implementing an inner loop we can get more executions per second at 
    a minor loss of frequency for the maintenance tasks. 
    */
    movingOn = abs(stepper.distanceToGo())>0; // stop when at target
  }
  else if ( trackingOnTarget ) trackingOnCurrent = true;   
} 
