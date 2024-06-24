#include "audio_ble_sender.h"



void audio_ble_sender_loop() {
  // If BLE client(i.e. gateway) is connected
  if (deviceConnected) {
    Serial.println("Connected to ble client");

    // Wait for reply(time delay) from gateway 
    while(deviceConnected){
      if(messageSendingEnabled){
        Serial.println("Waiting for audio");
        // Now waiting for wifi client which is pc to send audio.
        WiFiClient client1;
        while(true){
          client1 = server.available();
          if (client1) {
            break;
          }
        }
        Serial.println("Audio Client connected");
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
            pCharacteristic->setValue((uint8_t*)&audioSize, 4);
            pCharacteristic->notify();
            delay(500);
            // Send the type of data(audio) to the gateway
            datatype = 2;
            pCharacteristic->setValue(&datatype, 1);
            pCharacteristic->notify();
            delay(250);

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
                size_t BLEbasicchunck = 500;
                bytesRemaining -= bytesReceived;
              
                for (int i = 0; i < chunkSize; i += BLEbasicchunck){
                  size_t currentChunkSize = BLEbasicchunck;
                  if (i + BLEbasicchunck > chunkSize) {
                    currentChunkSize = chunkSize - i;  // Adjust last chunk size
                  }
                  pCharacteristic->setValue(audio_data + i, currentChunkSize);
                  pCharacteristic->notify();
                  delay(timeDelay);
                }
                        
                // delay(timeDelay);
              }         
            }

            Serial.println("Audio received and sent");
            break;

          }

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