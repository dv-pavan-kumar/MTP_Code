#include <WiFi.h>
#include "Arduino.h"


const char *soft_ap_ssid = "MyESP32CAM";
const char *soft_ap_password = "testpassword";
WiFiServer server(800);

const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";
const char *serverIP = "192.168.4.1"; // Replace with the IP address of the server ESP32
WiFiClient client;

uint8_t audio_data[60000];
uint32_t audioSize;


void setup() {
  Serial.begin(115200);
  delay(10);
  	
  // WiFi.mode(WIFI_MODE_APSTA); 	
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  Serial.print("ESP32 CAM IP as soft AP:");
  Serial.println(WiFi.softAPIP());

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.begin();
  Serial.println("Server started");
}


bool SendAudio(){
  if (!client.connected()) {
    // Connect to the server
    if (client.connect(serverIP, 80)) {
      Serial.println("Connected to server");

      while (client.connected()) {
        if (client.available()) {
          uint32_t tdelay = 0;
          size_t bytesReceived = client.readBytes(reinterpret_cast<char*>(&tdelay), sizeof(tdelay));
          if (bytesReceived != sizeof(tdelay)) {
            Serial.println("Error receiving delay time");
            return false;
          }
          Serial.print("Received delay time of : ");
          Serial.println(tdelay);
          
          Serial.print("Audio size is: ");
          Serial.println(audioSize);
          delay(50);

          client.write((const uint8_t*)&audioSize, sizeof(uint32_t));

          // Send data to the server
          delay(tdelay);
          // client.write(fb->buf, 250);
          // delay(50);
          // Serial.write(fb->buf, 250);
          size_t bytesSent = 0;
          size_t basicchunck = 250;
          while (bytesSent < audioSize) {
            size_t chunkSize = min(basicchunck, audioSize - bytesSent); // Chunk size of 250 bytes
            client.write(audio_data + bytesSent, chunkSize);       
            delay(50);
            Serial.println("-------");
            Serial.write(audio_data + bytesSent, chunkSize);
            Serial.println("-------");
            delay(tdelay);
            bytesSent += chunkSize;
          }

          delay(500);

          
          break;
        }
      }


      // Read the response from the server
      // Close the connection
      client.stop();
      Serial.println("Connection closed");
      delay(9000);
      return true;
    } else {
      Serial.println("Connection failed");
      return false;
    }
  }

}

void loop() {
  WiFiClient client1 = server.available();
  if (client1) {
    Serial.println("Client connected");

    // Read data from the client
    while (client1.connected()) {
      if (client1.available()) {

        size_t bytesReceived = client1.readBytes(reinterpret_cast<char*>(&audioSize), sizeof(audioSize));
        if (bytesReceived != sizeof(audioSize)) {
          Serial.println("Error receiving audio size");
          return;
        }
        Serial.print("Received audio size: ");
        Serial.println(audioSize);

        uint32_t basicchunck= 250;
        uint32_t bytesRemaining = audioSize;
        uint32_t bytesRecvd = 0;
        while (bytesRemaining > 0) {
          size_t chunkSize = min(basicchunck, bytesRemaining);
          if (client1.available()){
            bytesReceived = client1.readBytes(reinterpret_cast<char*>(audio_data+bytesRecvd), chunkSize);
            if (bytesReceived != chunkSize) {
              Serial.println("Error receiving audio chunk");
              return;
            }
            bytesRemaining -= bytesReceived;
            bytesRecvd += bytesReceived;
            delay(10);
            Serial.println("-------");
            delay(10);
            Serial.write(audio_data+bytesRecvd, chunkSize);
            delay(10);
            Serial.println("-------");
          }         
        }

        Serial.println("Audio received:");
        break;
      }
    }

    // Close the connection
    client1.stop();
    while(!SendAudio());
    Serial.println("Client disconnected");
  }
}
