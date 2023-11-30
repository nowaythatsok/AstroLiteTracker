#pragma once
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <read_file.h>

void setup_wifi();
void advertise_local_address();