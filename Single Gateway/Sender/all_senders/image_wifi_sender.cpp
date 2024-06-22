#include "image_wifi_sender.h"


void image_wifi_sender_loop() {
  if (!client.connected()) {
    // Connect to the wifi server(i.e. gateway)
    if (client.connect(serverIP, 80)) {
      Serial.println("Connected to server");
      // Take two false clicks to stabilize camera
      false_click();
      delay(1000);
      false_click();
      delay(1000);

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

          // Capture image, which is stored in frame buffer(fb)
          camera_fb_t *fb = esp_camera_fb_get();
          if (!fb) {
            Serial.println("Camera capture failed");
            return;
          }
          Serial.print("Captured image size: ");
          Serial.println(fb->len);
          delay(50);

          // Send image size to the gateway
          uint32_t imageSize = fb->len;
          client.write((const uint8_t*)&imageSize, sizeof(uint32_t));
          delay(500);

          // Send the type of data(image) to the gateway
          datatype = 1;
          client.write(&datatype, 1);
          delay(200);
          
          // Now, send the data from frame buffer by dividing into chuncks of size 1250 bytes.
          size_t bytesSent = 0;
          size_t basicchunck = 1250;
          while (bytesSent < fb->len) {
            size_t chunkSize = min(basicchunck, fb->len - bytesSent); // Chunk size of 1250 bytes
            client.write(fb->buf + bytesSent, chunkSize);       
            delay(50);
            Serial.println("-------");
            Serial.write(fb->buf + bytesSent, chunkSize);
            Serial.println("-------");
            delay(tdelay);
            bytesSent += chunkSize;
          }

          delay(500);

          // free the frame buffer
          esp_camera_fb_return(fb);
          
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
