#include <LoRa.h>
#include <SPI.h>
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

#define SS 12
#define RST 15
#define DIO0 16
#define SCK 14
#define MISO 13
#define MOSI 2


#define Frequency 865E6 // Hz
#define SignalBandwidth 125E3 // Hz
#define SignalBandwidthIndex 0 // 0: 125 kHz,
// 1: 250 kHz,
// 2: 500 kHz,
// 3: Reserved
#define SpreadingFactor 7 // SF7..SF12
#define CodingRate 1 // 1: 4/5,
// 2: 4/6,
// 3: 4/7,
// 4: 4/8
#define CodingRateDenominator 5
#define PreambleLength 8 // Same for Tx and Rx
#define SyncWord 0x12
#define OutputPower 14 // dBm
#define SymbolTimeout 0 // Symbols
#define FixedLengthPayload false
#define IQInversion false

uint8_t myAddr = 10;
uint8_t gatewayAddr = 50;
uint32_t tdelay = 0;


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


void captureAndSendImage(){
  Serial.println("Sending image to gateway");
  false_click();
  delay(1000);
  false_click();
  delay(1000);

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
  sizeBuffer[0] = gatewayAddr;

  // Pack size into buffer (remaining 4 bytes)
  sizeBuffer[1] = (imageSize >> 24) & 0xFF; 
  sizeBuffer[2] = (imageSize >> 16) & 0xFF;
  sizeBuffer[3] = (imageSize >> 8) & 0xFF;
  sizeBuffer[4] = imageSize & 0xFF; 
  if (LoRa.beginPacket()) {
    LoRa.write(sizeBuffer, 5);
    LoRa.endPacket();
  }
  delay(tdelay);

  size_t bytesSent = 0;
  size_t basicchunck = 250;
  while (bytesSent < fb->len) {
    size_t chunkSize = min(basicchunck, fb->len - bytesSent); // Chunk size of 250 bytes
    if (LoRa.beginPacket()) {
      LoRa.write(gatewayAddr);
      LoRa.write(fb->buf + bytesSent, chunkSize);
      LoRa.endPacket();   
      //delay(10);
      Serial.println("-------");
      delay(10);
      Serial.write(fb->buf + bytesSent, chunkSize);
      delay(10);
      Serial.println("-------");
      delay(tdelay);
      bytesSent += chunkSize;
    }
  }

  delay(500);
  tdelay = 0;
  esp_camera_fb_return(fb);

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

  LoRa.setPins(SS, RST, DIO0);
  SPI.begin(14,13,2,12); 
  SPI.setFrequency(250000);

  while (!LoRa.begin(Frequency)) {
    //Serial.println("Error initializing LoRa. Retrying...");
    delay(1000);
  }
  //Serial.print("[setup] LoRa Frequency: ");
  //Serial.println(Frequency);

  //LoRa.begin(Frequency);
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  Serial.println("Lora Initialised");
}

void loop() {
  if (LoRa.beginPacket()) {
    LoRa.write(gatewayAddr);
    LoRa.write(myAddr);
    LoRa.endPacket();
  } else {
    Serial.println("Error sending packet");
    return;
  }

  delay(800);

  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    // Read incoming data
    uint8_t receiverAddr = LoRa.read();
    if(receiverAddr == myAddr){
      uint8_t sizeBuffer[4];
      for(int i=0;i<4;i++){
        sizeBuffer[i] = LoRa.read();
      }
      tdelay = ((uint32_t)sizeBuffer[0] << 24) | ((uint32_t)sizeBuffer[1] << 16) | ((uint32_t)sizeBuffer[2] << 8) | sizeBuffer[3];
      Serial.printf("Received tdelay: %d\n", tdelay);
      captureAndSendImage();
    }
    
  }
  delay(1000);
}
