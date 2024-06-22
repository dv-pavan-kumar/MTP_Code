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

          uint32_t imageSize;
          size_t bytesReceived = client.readBytes(reinterpret_cast<char*>(&imageSize), sizeof(imageSize));
          if (bytesReceived != sizeof(imageSize)) {
            //Serial.println("Error receiving image size");
            return;
          }
          for (int i = 0; i < sizeof(uint32_t); i++) {
            Serial.write((uint8_t)(imageSize >> (i * 8))); // Send each byte of imageSize
            delay(1); // Small delay between bytes
          }

          while (client.connected()){
            if (client.available()) {
              datatype = client.read();
              Serial.write(&datatype, 1);
              break;
            }
          }

          uint8_t buffer[1250];
          // uint32_t basicchunck= 250;
          uint32_t bytesRemaining = imageSize;
          while (bytesRemaining > 0) {
            // size_t chunkSize = min(basicchunck, bytesRemaining);
            if (client.available()){
              size_t len = client.available(); 
              bytesReceived = client.readBytes(reinterpret_cast<char*>(buffer), len);
              bytesRemaining -= bytesReceived;
              Serial.write(buffer, len);
              delay(100);
            }         
          }
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
