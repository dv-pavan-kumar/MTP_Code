#include "text_ble_sender.h"

void text_ble_sender_loop() {
  // If BLE client(i.e. gateway) is connected 
  if (deviceConnected) {
    Serial.println("Connected to client");

    // Wait for reply(time delay) from gateway 
    while(deviceConnected){
      if(messageSendingEnabled){
        // Convert the String into uint8_t bytes to be able to send
        uint8_t* data = (uint8_t*) message.c_str();
        uint32_t length = message.length();
        Serial.print("Text size is: ");
        Serial.println(length);
        // Send the size of the text to the gateway
        pCharacteristic->setValue((uint8_t*)&length, 4);
        pCharacteristic->notify();
        delay(500);

        // Send the type of data(text) to the gateway
        datatype = 3;
        pCharacteristic->setValue(&datatype, 1);
        pCharacteristic->notify();
        delay(250);

        // Now, send the text data by dividing it into chuncks of size 250 bytes.
        size_t bytesSent = 0;
        size_t basicchunck = 500;
        while (bytesSent < length) {
          size_t chunkSize = min(basicchunck, length - bytesSent); // Chunk size of 250 bytes
          pCharacteristic->setValue(data + bytesSent, chunkSize);
          pCharacteristic->notify();       
          delay(timeDelay);
          bytesSent += chunkSize;
        }

        delay(500);

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