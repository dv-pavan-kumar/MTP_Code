#include "audio_lora_sender.h"


void LoraSendaudio(){
  WiFiClient client1;
  // Now waiting for wifi client which is pc to connect to the sender.
  while(true){
    client1 = server.available();
    if (client1) {
      Serial.println("Client connected");
      break;
    }
  }
  // While connected to the pc, wait for the incoming data
  while (client1.connected()) {
    if (client1.available()) {
      // Receive audio size from pc and forward it to the gateway
      size_t bytesReceived = client1.readBytes(reinterpret_cast<char*>(&audioSize), sizeof(audioSize));
      if (bytesReceived != sizeof(audioSize)) {
        Serial.println("Error receiving audio size");
        return;
      }
      Serial.print("Received audio size: ");
      Serial.println(audioSize);
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
      delay(1000); 

      // Send the type of data(audio) to the gateway
      datatype = 2;
      sizeBuffer[1] = datatype;
      if (LoRa.beginPacket()) {
        LoRa.write(sizeBuffer, 2);
        LoRa.endPacket();
      }
      delay(500);
      
      // Receive audio data from pc in the form of chuncks of size 250 bytes and forward it to the gateway
      uint32_t basicchunck = 1250;
      uint32_t bytesRemaining = audioSize;
      while (bytesRemaining > 0) {
        size_t chunkSize = min(basicchunck, bytesRemaining);
        if (client1.available()){
          bytesReceived = client1.readBytes(reinterpret_cast<char*>(audio_data), chunkSize);
          if (bytesReceived != chunkSize) {
            Serial.println("Error receiving audio chunk");
            return;
          }
          bytesRemaining -= bytesReceived;
          Serial.println("-------");
          delay(10);
          Serial.write(audio_data, chunkSize);
          delay(10);
          Serial.println("-------");
          size_t Lorabasicchunck = 250;
          for (int i = 0; i < chunkSize; i += Lorabasicchunck){
            size_t currentChunkSize = Lorabasicchunck;
            if (i + Lorabasicchunck > chunkSize) {
              currentChunkSize = chunkSize - i;  // Adjust last chunk size
            }
            if (LoRa.beginPacket()) {
              LoRa.write(gatewayAddr);
              LoRa.write(audio_data+i, currentChunkSize);
              LoRa.endPacket();
            }       
            delay(tdelay);

          }
          
        }         
      }

      Serial.println("Audio received:");
      break;
    }
  }

  delay(500);
  tdelay = 0;
}



void audio_lora_sender_loop() {
  // Advertize packet to gateway every 6 seconds
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (LoRa.beginPacket()) {
      LoRa.write(gatewayAddr);
      LoRa.write(myAddr);
      LoRa.endPacket();
      Serial.println("Sent Lora adv packet");
    } else {
      Serial.println("Error sending packet");
    }
  }

  // delay(6000);
  // Check if any response from gateway
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    // Gateway sends an response, which contains time delay
    uint8_t receiverAddr = LoRa.read();
    if(receiverAddr == myAddr){
      uint8_t sizeBuffer[4];
      for(int i=0;i<4;i++){
        sizeBuffer[i] = LoRa.read();
      }
      tdelay = ((uint32_t)sizeBuffer[0] << 24) | ((uint32_t)sizeBuffer[1] << 16) | ((uint32_t)sizeBuffer[2] << 8) | sizeBuffer[3];
      Serial.printf("Received tdelay: %d\n", tdelay);
      // Now call function to capture and send image
      LoraSendaudio();
      delay(1000);
    }
  }
  //delay(5000);
}