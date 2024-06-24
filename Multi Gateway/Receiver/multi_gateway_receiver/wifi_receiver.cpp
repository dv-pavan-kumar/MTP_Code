#include "wifi_receiver.h"

void wifi_setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
}

void wifi_loop() {
  if (!client.connected()) {
    // Connect to the server
    if (client.connect(serverIP, 8888)) {
      while (client.connected()) {
        if (client.available()) {
          String incoming = client.readStringUntil('\n');
          Serial.println("Received: " + incoming);
          break;
        }
      }
      
      // Close the connection
      client.stop(); 
    } 
    // Wait for a while before reconnecting
    delay(1000);
  }
}
