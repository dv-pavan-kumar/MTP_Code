#include "common.h"

const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";
const char *serverIP = "192.168.4.1"; // Replace with the IP address of the server ESP32
WiFiClient client;

// The remote service we wish to connect to.
BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331913a");
// The characteristic of the remote service we are interested in.
BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b25a0");

BLEClient*  pClient = NULL;
boolean doConnect = false;
boolean connected = false;
boolean doScan = true;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;

uint8_t buffer[1250];
uint32_t imageSize = 0;
uint32_t receivedBytes = 0;
size_t bytesReceived = 0;
uint8_t myAddr = 20;
bool receivedDatatype = false;
bool receivedStart = false;
uint8_t datatype;