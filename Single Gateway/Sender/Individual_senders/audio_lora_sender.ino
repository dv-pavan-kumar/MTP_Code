#include <WiFi.h>
#include <LoRa.h>
#include <SPI.h>
#include "Arduino.h"


#define SS 12
#define RST 15
#define DIO0 16
#define SCK 14
#define MISO 13
#define MOSI 2


#define Frequency 865E6 // Hz
#define SignalBandwidth 125E3 // Hz
#define SignalBandwidthIndex 0 // 0: 125 kHz,
// 1: 250 kHz,
// 2: 500 kHz,
// 3: Reserved
#define SpreadingFactor 7 // SF7..SF12
#define CodingRate 1 // 1: 4/5,
// 2: 4/6,
// 3: 4/7,
// 4: 4/8
#define CodingRateDenominator 5
#define PreambleLength 8 // Same for Tx and Rx
#define SyncWord 0x12
#define OutputPower 14 // dBm
#define SymbolTimeout 0 // Symbols
#define FixedLengthPayload false
#define IQInversion false

uint8_t myAddr = 10;
uint8_t gatewayAddr = 50;
uint32_t tdelay = 0;

const char *soft_ap_ssid = "MyESP32CAM";
const char *soft_ap_password = "testpassword";
WiFiServer server(800);


uint8_t audio_data[60000];
uint32_t audioSize;



bool receiveAudio(){
  WiFiClient client1 = server.available();
  if (client1) {
    Serial.println("Client connected");

    // Read data from the client
    while (client1.connected()) {
      if (client1.available()) {

        size_t bytesReceived = client1.readBytes(reinterpret_cast<char*>(&audioSize), sizeof(audioSize));
        if (bytesReceived != sizeof(audioSize)) {
          Serial.println("Error receiving audio size");
          return false;
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
              return false;
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
    Serial.println("Client disconnected");
    return true;
  }
  return false;
}


void captureAndSendaudio(){
  Serial.println("Receiving audio");
  while(!receiveAudio());
  Serial.println("Sending audio to gateway");

  Serial.print("Audio size is: ");
  Serial.println(audioSize);
  delay(50);
  uint8_t sizeBuffer[5];
  sizeBuffer[0] = gatewayAddr;

  // Pack size into buffer (remaining 4 bytes)
  sizeBuffer[1] = (audioSize >> 24) & 0xFF; 
  sizeBuffer[2] = (audioSize >> 16) & 0xFF;
  sizeBuffer[3] = (audioSize >> 8) & 0xFF;
  sizeBuffer[4] = audioSize & 0xFF; 
  if (LoRa.beginPacket()) {
    LoRa.write(sizeBuffer, 5);
    LoRa.endPacket();
  }
  delay(tdelay);

  size_t bytesSent = 0;
  size_t basicchunck = 250;
  while (bytesSent < audioSize) {
    size_t chunkSize = min(basicchunck, audioSize - bytesSent); // Chunk size of 250 bytes
    if (LoRa.beginPacket()) {
      LoRa.write(gatewayAddr);
      LoRa.write(audio_data + bytesSent, chunkSize);
      LoRa.endPacket();
      Serial.println("-------");
      delay(10);
      Serial.write(audio_data + bytesSent, chunkSize);
      delay(10);
      Serial.println("-------");
      delay(tdelay);
      bytesSent += chunkSize;
    }
  }

  delay(500);
  tdelay = 0;
}


void setup() {
  Serial.begin(115200);
  delay(10);

  //WiFi.mode(WIFI_MODE_APSTA); 	
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  Serial.print("ESP32 CAM IP as soft AP:");
  Serial.println(WiFi.softAPIP());

  server.begin();
  Serial.println("Server started");




  LoRa.setPins(SS, RST, DIO0);
  SPI.begin(14,13,2,12); 
  SPI.setFrequency(250000);

  while (!LoRa.begin(Frequency)) {
    //Serial.println("Error initializing LoRa. Retrying...");
    delay(1000);
  }
  //Serial.print("[setup] LoRa Frequency: ");
  //Serial.println(Frequency);

  //LoRa.begin(Frequency);
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  Serial.println("Lora Initialised");
}

void loop() {
  if (LoRa.beginPacket()) {
    LoRa.write(gatewayAddr);
    LoRa.write(myAddr);
    LoRa.endPacket();
  } else {
    Serial.println("Error sending packet");
    return;
  }

  delay(800);

  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    // Read incoming data
    uint8_t receiverAddr = LoRa.read();
    if(receiverAddr == myAddr){
      uint8_t sizeBuffer[4];
      for(int i=0;i<4;i++){
        sizeBuffer[i] = LoRa.read();
      }
      tdelay = ((uint32_t)sizeBuffer[0] << 24) | ((uint32_t)sizeBuffer[1] << 16) | ((uint32_t)sizeBuffer[2] << 8) | sizeBuffer[3];
      Serial.printf("Received tdelay: %d\n", tdelay);
      captureAndSendaudio();
    }  
  }
  delay(1000);
}
