#include <WiFi.h>
#include "esp_camera.h"
#include "Arduino.h"

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";
const char *serverIP = "192.168.4.1"; // Replace with the IP address of the server ESP32
WiFiClient client;


void false_click(){
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.print("Captured image size: ");
  Serial.println(fb->len);
  esp_camera_fb_return(fb);
  delay(300);
}


void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Frame parameters
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize camera with the above configuration
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  if (!client.connected()) {
    // Connect to the server
    if (client.connect(serverIP, 80)) {
      Serial.println("Connected to server");
      false_click();
      delay(1000);
      false_click();
      delay(1000);

      while (client.connected()) {
        if (client.available()) {
          uint32_t tdelay = 0;
          size_t bytesReceived = client.readBytes(reinterpret_cast<char*>(&tdelay), sizeof(tdelay));
          if (bytesReceived != sizeof(tdelay)) {
            Serial.println("Error receiving image size");
            return;
          }
          Serial.print("Received delay time of : ");
          Serial.println(tdelay);
          camera_fb_t *fb = esp_camera_fb_get();
          if (!fb) {
            Serial.println("Camera capture failed");
            return;
          }
          Serial.print("Captured image size: ");
          Serial.println(fb->len);
          delay(50);

          uint32_t imageSize = fb->len;
          client.write((const uint8_t*)&imageSize, sizeof(uint32_t));

          // Send data to the server
          delay(tdelay);
          // client.write(fb->buf, 250);
          // delay(50);
          // Serial.write(fb->buf, 250);
          size_t bytesSent = 0;
          size_t basicchunck = 250;
          while (bytesSent < fb->len) {
            size_t chunkSize = min(basicchunck, fb->len - bytesSent); // Chunk size of 250 bytes
            client.write(fb->buf + bytesSent, chunkSize);       
            delay(50);
            Serial.println("-------");
            Serial.write(fb->buf + bytesSent, chunkSize);
            Serial.println("-------");
            delay(tdelay);
            bytesSent += chunkSize;
          }

          delay(500);


          esp_camera_fb_return(fb);
          
          break;
        }
      }


      // Read the response from the server
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
