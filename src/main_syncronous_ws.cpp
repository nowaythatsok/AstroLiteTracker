// https://shawnhymel.com/1882/how-to-create-a-web-server-with-websockets-using-an-esp32-in-arduino/

#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include <read_file.h>
#include <wifi_setup.h>
#include <webserver_callbacks.h>

// Constants
const int http_port = 80;
const int ws_port = 1337;

// Globals
AsyncWebServer server(http_port);
WebSocketsServer webSocket = WebSocketsServer(ws_port);
char msg_buf[200];

/***********************************************************
 * Functions
 */

// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num,
                      WStype_t type,
                      uint8_t * payload,
                      size_t length) {

  // Figure out the type of WebSocket event
  switch(type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("INFO: [%u] Disconnected!\n", client_num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("INFO: [%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;

    // Handle text messages from client
    case WStype_TEXT:{

      // Print out raw message
      Serial.printf("[%u] Received text: %s\n", client_num, payload);

      StaticJsonDocument<1000> doc;
      deserializeJson(doc, payload);
      const char* mytype = doc["type"];
      Serial.printf("Decoded json [%u]: %s\n", client_num, mytype);

      if ( strcmp((char *)mytype, "trackerOn") == 0 ) {
        Serial.printf("trackerOn");
      } else if ( strcmp((char *)mytype, "fullState") == 0 ) {
        Serial.println("Returning full state");
        //webSocket.sendTXT(client_num, msg_buf);
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

  // On HTTP request 
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/style.css", HTTP_GET, onCSSRequest);
  server.on("/index.js", HTTP_GET, onJSRequest);
  server.on("/favicon.ico", HTTP_GET, onFavRequest);
  server.onNotFound(onPageNotFound);

  // Start web server
  server.begin();

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  
}

void loop() {
  // Look for and handle WebSocket data
  webSocket.loop();
}