#include <wifi_setup.h>

void setup_wifi(){

  //try to connect to known wifi network
  String ssid_wifi = readFile(SPIFFS, "/ssid_wifi.txt");
  String pwd_wifi  = readFile(SPIFFS, "/pwd_wifi.txt");

  if ( ssid_wifi!="" && pwd_wifi!="" ){
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_wifi, pwd_wifi);
    for (int i=0; i<20; i++) {
      if (WiFi.status() == WL_CONNECTED) return;
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }
    Serial.println("Could not find WiFi network, creating AP.");
  }
  else Serial.println("Could not find WiFi credentials, creating AP.");

  // Start access point
  String ssid_ap = readFile(SPIFFS, "/ssid_ap.txt");
  String pwd_ap  = readFile(SPIFFS, "/pwd_ap.txt");

  if ( ssid_wifi=="" && pwd_wifi=="" ){
    Serial.println("Could not find AP credentials, exiting.");
    esp_deep_sleep_start();
  }
 
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap, pwd_ap);
  // Print our IP address
  Serial.println();
  Serial.println("AP running");
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());

}

void advertise_local_address(){

  String local_addr  = readFile(SPIFFS, "/local_addr.txt");
  if (local_addr == "") local_addr = "esp32";

    // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp32.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(local_addr)) {
      Serial.println("Error setting up MDNS responder!");
      while(1) {
          delay(1000);
      }
  }
  Serial.println("mDNS responder started: " + local_addr + ".local");
}

