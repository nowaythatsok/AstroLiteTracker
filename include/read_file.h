#pragma once
#include <SPIFFS.h>
String readFile(fs::FS &fs, const char * path);