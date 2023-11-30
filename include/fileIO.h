#pragma once
#include <SPIFFS.h>
String readFile(fs::FS &fs, const char * path);
void wrieFile(fs::FS &fs, const String &path, const String &data);