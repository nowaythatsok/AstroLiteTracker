https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#a3591e29a236e2935afd7f64ff6c22006

The fastest motor speed that can be reliably supported is about 4000 steps per second at a clock frequency of 16 MHz on Arduino such as Uno etc. Faster processors can support faster stepping speeds. However, any speed less than that down to very slow speeds (much less than one per second) are also supported, provided the run() function is called frequently enough to step the motor whenever required for the speed set. Calling setAcceleration() is expensive, since it requires a square root to be 

ESP8266 (80 or 160 MHz), 1 core 
ESP32: (160 or 240 MHz), 2 cores
https://community.platformio.org/t/setting-cpu-frequency/26760/20
https://community.platformio.org/t/esp32-board-build-f-cpu-does-nothing/5480/7
does anybody know, if setting CPU clock at runtime, is even possible on the ESP32 (like it is on ESP8266)?
No, this isn't possible on the ESP32.
