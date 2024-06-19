#include "text_lora_sender.h"


void SendText(){
  Serial.println("Sending text to gateway");
  // Convert the String into uint8_t type bytes to be able to send
  uint8_t* data = (uint8_t*) message.c_str();
  uint32_t length = message.length();
  Serial.print("Text size is: ");
  Serial.println(length);
  delay(50);

  uint8_t sizeBuffer[5];
  sizeBuffer[0] = gatewayAddr;
  // Pack size into buffer (remaining 4 bytes)
  sizeBuffer[1] = (length >> 24) & 0xFF; 
  sizeBuffer[2] = (length >> 16) & 0xFF;
  sizeBuffer[3] = (length >> 8) & 0xFF;
  sizeBuffer[4] = length & 0xFF; 
  // Send the size of the text to the gateway
  if (LoRa.beginPacket()) {
    LoRa.write(sizeBuffer, 5);
    LoRa.endPacket();
  }
  delay(1000);

  // Send the type of data(text) to the gateway
  datatype = 3;
  sizeBuffer[1] = datatype;
  if (LoRa.beginPacket()) {
    LoRa.write(sizeBuffer, 2);
    LoRa.endPacket();
  }
  delay(tdelay);

  // Now, send the text data by dividing it into chuncks of size 250 bytes.
  size_t bytesSent = 0;
  size_t basicchunck = 250;
  while (bytesSent < length) {
    size_t chunkSize = min(basicchunck, length - bytesSent); // Chunk size of 250 bytes
    if (LoRa.beginPacket()) {
      LoRa.write(gatewayAddr);
      LoRa.write(data + bytesSent, chunkSize);
      LoRa.endPacket();  
      Serial.println("-------");
      delay(10);
      Serial.write(data + bytesSent, chunkSize);
      delay(10);
      Serial.println("-------");
      delay(tdelay);
      bytesSent += chunkSize;
    }
  }

  delay(500);
  tdelay = 0;

}



void text_lora_sender_loop() {
  // Advertize packet to gateway
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
    uint8_t receiverAddr = LoRa.read(); // LoRa.read(), reads the available data byte by byte.
    if(receiverAddr == myAddr){
      uint8_t sizeBuffer[4];
      for(int i=0;i<4;i++){
        sizeBuffer[i] = LoRa.read();
      }
      tdelay = ((uint32_t)sizeBuffer[0] << 24) | ((uint32_t)sizeBuffer[1] << 16) | ((uint32_t)sizeBuffer[2] << 8) | sizeBuffer[3];
      Serial.printf("Received tdelay: %d\n", tdelay);
      // Now call function to capture and send image
      SendText();
      delay(1000);
    } 
  }
  //delay(1000);
}