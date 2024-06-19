#ifndef common_h
#define common_h

#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
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

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC1_UUID "beb5143e-36e1-4688-b7f5-ea07361b26a7"

extern uint8_t myAddr;
extern uint8_t gatewayAddr;
extern uint32_t tdelay;
extern unsigned long previousMillis;  // will store last time the LoRa packet was sent
extern const long interval;
extern unsigned long currentMillis;


extern const char *soft_ap_ssid;
extern const char *soft_ap_password;
extern WiFiServer server;

extern const char *ssid;
extern const char *password;
extern const char *serverIP; // Replace with the IP address of the server ESP32
extern WiFiClient client;


extern BLEServer* pServer;
extern BLECharacteristic* pCharacteristic;
extern BLECharacteristic* pCharacteristic1;
extern bool deviceConnected;
extern uint32_t timeDelay;
extern bool messageSendingEnabled;

extern String message;

extern uint8_t audio_data[1500];
extern uint32_t audioSize;
extern uint8_t datatype;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic);
};

void false_click();

void setupCommon();


#endif
