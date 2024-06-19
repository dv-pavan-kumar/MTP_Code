#include <WiFi.h>
#include <SPI.h>
#include "Arduino.h"


const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";
const char *serverIP = "192.168.4.1"; // Replace with the IP address of the server ESP32
WiFiClient client;
uint8_t datatype;


void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
}

void loop() {
  if (!client.connected()) {
    // Connect to the server
    if (client.connect(serverIP, 8888)) {
      while (client.connected()) {
        if (client.available()) {
          String incoming = client.readStringUntil('\n');
          Serial.println("Received: " + incoming);
          break;
        }
      }
      
      // Close the connection
      client.stop(); 
    } 
    // Wait for a while before reconnecting
    delay(1000);
  }
}
