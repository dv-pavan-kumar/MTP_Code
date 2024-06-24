#include "image_ble_sender.h"


void image_ble_sender_loop() {
  
  // If BLE client(i.e. gateway) is connected 
  if (deviceConnected) {
    Serial.println("Connected to client");
    // Take two false clicks to stabilize camera
    false_click();
    delay(1000);
    false_click();
    delay(1000);

    // Wait for reply(time delay) from gateway 
    while(deviceConnected){
      if(messageSendingEnabled){
        // Capture image, which is stored in frame buffer(fb)
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("Camera capture failed");
          return;
        }
        Serial.print("Captured image size: ");
        Serial.println(fb->len);
        // Send image size to the gateway
        uint32_t imageSize = fb->len;
        pCharacteristic->setValue((uint8_t*)&imageSize, 4);
        pCharacteristic->notify();
        delay(500);

        // Send the type of data(image) to the gateway
        datatype = 1;
        pCharacteristic->setValue(&datatype, 1);
        pCharacteristic->notify();
        delay(250);

        // Now, send the data from frame buffer by dividing into chuncks of size 250 bytes.
        size_t bytesSent = 0;
        size_t basicchunck = 500;
        while (bytesSent < fb->len) {
          size_t chunkSize = min(basicchunck, fb->len - bytesSent); // Chunk size of 250 bytes
          pCharacteristic->setValue(fb->buf + bytesSent, chunkSize);
          pCharacteristic->notify();       
          delay(timeDelay);
          bytesSent += chunkSize;
        }

        delay(500);

        // free the frame buffer
        esp_camera_fb_return(fb);
        messageSendingEnabled = false;
        break;
      }
    }
  
  }
  // If BLE client is not connected continuously start advertising after every 200ms.
  if (!deviceConnected) {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    // Start Advertising BLE Server
    pAdvertising->start();
    delay(200); 
  }
}