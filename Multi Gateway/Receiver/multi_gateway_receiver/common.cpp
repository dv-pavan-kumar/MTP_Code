#include "common.h"

uint8_t myAddr = 20;

const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";
const char *serverIP = "192.168.4.1"; // Replace with the IP address of the server ESP32
WiFiClient client;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;