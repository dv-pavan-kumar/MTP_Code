#ifndef common_h
#define common_h

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include "Arduino.h"
#include "BLEDevice.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


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

#define SERVICE_UUID        "4fafc202-1fb5-459e-8fcc-c5c9c331915c"
#define CHARACTERISTIC_UUID "beb5484e-36e1-4688-b7f5-ea07361b26b9"

extern uint8_t myAddr;

extern const char *ssid;
extern const char *password;
extern const char *serverIP; // Replace with the IP address of the server ESP32
extern WiFiClient client;

extern BLEServer* pServer;
extern BLECharacteristic* pCharacteristic;
extern bool deviceConnected;


#endif