#include "text_wifi_sender.h"

void text_wifi_sender_loop() {
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
            Serial.println("Error receiving image size");
            return;
          }
          Serial.print("Received delay time of : ");
          Serial.println(tdelay);
          // Convert the String into uint8_t type bytes to be able to send
          uint8_t* data = (uint8_t*) message.c_str();
          uint32_t length = message.length();
          Serial.print("Text size is: ");
          Serial.println(length);
          delay(50);

          // Send the size of the text to the gateway
          client.write((const uint8_t*)&length, sizeof(uint32_t));
          delay(500);

          // Send the type of data(text) to the gateway
          datatype = 3;
          client.write(&datatype, 1);
          delay(200);
      
          // Now, send the text data by dividing it into chuncks of size 250 bytes.
          size_t bytesSent = 0;
          size_t basicchunck = 1250;
          while (bytesSent < length) {
            size_t chunkSize = min(basicchunck, length - bytesSent); // Chunk size of 250 bytes
            client.write(data + bytesSent, chunkSize);       
            delay(50);
            Serial.println("-------");
            Serial.write(data + bytesSent, chunkSize);
            Serial.println("-------");
            delay(tdelay);
            bytesSent += chunkSize;
          }

          delay(500);

          
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