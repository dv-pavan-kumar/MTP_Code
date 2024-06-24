#include "image_lora_sender.h"


void captureAndSendImage(){
  Serial.println("Sending image to gateway");
  // Take two false clicks to stabilize camera
  false_click();
  delay(1000);
  false_click();
  delay(1000);

  // Capture image, which is stored in frame buffer(fb)
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.print("Captured image size: ");
  uint32_t imageSize = fb->len;
  Serial.println(imageSize);
  delay(50);
  uint8_t sizeBuffer[5];
  // Set the first byte of with the gateway address for addressing in lora
  sizeBuffer[0] = gatewayAddr;
  // Pack size into buffer (remaining 4 bytes)
  sizeBuffer[1] = (imageSize >> 24) & 0xFF; 
  sizeBuffer[2] = (imageSize >> 16) & 0xFF;
  sizeBuffer[3] = (imageSize >> 8) & 0xFF;
  sizeBuffer[4] = imageSize & 0xFF; 
  
  // Send image size to the gateway
  if (LoRa.beginPacket()) {
    LoRa.write(sizeBuffer, 5);
    LoRa.endPacket();
  }
  delay(1000);

  // Send the type of data(image) to the gateway
  datatype = 1;
  sizeBuffer[1] = datatype;
  if (LoRa.beginPacket()) {
    LoRa.write(sizeBuffer, 2);
    LoRa.endPacket();
  }
  delay(tdelay);

  // Now, send the data from frame buffer by dividing into chuncks of size 250 bytes.
  size_t bytesSent = 0;
  size_t basicchunck = 250;
  while (bytesSent < fb->len) {
    size_t chunkSize = min(basicchunck, fb->len - bytesSent); // Chunk size of 250 bytes
    if (LoRa.beginPacket()) {
      // Set the first byte of every packet with the gateway address
      LoRa.write(gatewayAddr);
      LoRa.write(fb->buf + bytesSent, chunkSize);
      LoRa.endPacket();
      delay(tdelay);
      bytesSent += chunkSize;
    }
  }

  delay(500);
  tdelay = 0;
  esp_camera_fb_return(fb);
}



void image_lora_sender_loop() {
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
  
  
  //delay(3500);
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
      captureAndSendImage();
      delay(1000);
    }
    else{
      while (LoRa.available()) {
        LoRa.read(); // Read and discard the rest of the packet
      }
    }
    
  }
}