#pragma once 
#include <ESPAsyncWebServer.h>

// Callback: send homepage
void onIndexRequest(AsyncWebServerRequest *request);
void onCSSRequest(AsyncWebServerRequest *request);
void onCSS2Request(AsyncWebServerRequest *request);
void onJSRequest(AsyncWebServerRequest *request);
void onPageNotFound(AsyncWebServerRequest *request);
void onFavRequest(AsyncWebServerRequest *request);