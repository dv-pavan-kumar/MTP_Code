#include "audio_wifi_sender.h"


void audio_wifi_sender_loop() {

  if (!client.connected()) {
    // Connect to the wifi server(i.e. gateway)
    if (client.connect(serverIP, 80)) {
      Serial.println("Connected to server");

      // wait for an response(time delay) from the gateway
      while (client.connected()) {
        if (client.available()) {
          tdelay = 0;
          // Read the time delay send by the gateway
          size_t bytesReceived = client.readBytes(reinterpret_cast<char*>(&tdelay), sizeof(tdelay));
          if (bytesReceived != sizeof(tdelay)) {
            Serial.println("Error receiving delay time");
            return;
          }
          Serial.print("Received delay time of : ");
          Serial.println(tdelay);
          // Now waiting for wifi client which is pc to connect to the sender.
          WiFiClient client1;
          while(true){
            client1 = server.available();
            if (client1) {
              break;
            }
          }
          // While connected to the pc, wait for the incoming audio data
          Serial.println("Client connected");
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
              client.write((const uint8_t*)&audioSize, sizeof(uint32_t));
              delay(500);

              // Send the type of data(audio) to the gateway
              datatype = 2;
              client.write(&datatype, 1);
              delay(200);

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
                  client.write(audio_data, chunkSize);       
                  delay(tdelay);
                }         
              }

              Serial.println("Audio received:");
              break;
            }

          }
                
          break;
        }
      }

      // Close the connection
      client.stop();
      Serial.println("Connection closed");
      delay(9000);
    } else {
      Serial.println("Connection failed");
    }
    // Wait for a while before reconnecting
    delay(1000);
  }

}