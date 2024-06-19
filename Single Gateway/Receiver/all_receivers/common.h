#ifndef common_h
#define common_h

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include "Arduino.h"
#include "BLEDevice.h"

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

extern const char *ssid;
extern const char *password;
extern const char *serverIP; // Replace with the IP address of the server ESP32
extern WiFiClient client;

// The remote service we wish to connect to.
extern BLEUUID serviceUUID;
// The characteristic of the remote service we are interested in.
extern BLEUUID    charUUID;

extern BLEClient*  pClient;
extern boolean doConnect;
extern boolean connected;
extern boolean doScan;
extern BLERemoteCharacteristic* pRemoteCharacteristic;
extern BLEAdvertisedDevice* myDevice;

extern uint8_t buffer[1250];
extern uint32_t imageSize;
extern uint32_t receivedBytes;
extern size_t bytesReceived;
extern uint8_t myAddr;
extern bool receivedDatatype;
extern bool receivedStart;
extern uint8_t datatype;

#endif