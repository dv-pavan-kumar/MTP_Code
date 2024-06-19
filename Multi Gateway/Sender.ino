#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include "BLEDevice.h"

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

String receiverID = "RX123";   // ID of the receiver ESP32
String bestRelayID;            // ID of the best relay node
String bestRelayType;          // Connection type of the best relay node
String bestRelaySSID;
String bestRelayPassword; 
String bestRelayServiceUUID;
String bestRelayCharUUID; 
bool canSend = false;
uint8_t priorityValue = 0;

WiFiClient client;


BLEUUID serviceUUID;
BLEUUID    charUUID;

BLEClient*  pClient = NULL;
BLEScan* pBLEScan = NULL;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;



class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      // std::string value = pRemoteCharacteristic->readValue();
      // Serial.print("The characteristic value was: ");
      // Serial.println(value.c_str());
    }
      
    connected = true;
    return true;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    Serial.println(advertisedDevice.isAdvertisingService(serviceUUID));

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;

    } // Found our server
  } // onResult
};


// Function to handle received responses
void processResponse(String message) {
  // Assuming the message format is "RESPONSE:id,type,SSID,password"
  int firstCommaIndex = message.indexOf(',');
  int secondCommaIndex = message.indexOf(',', firstCommaIndex + 1);
  int thirdCommaIndex = message.indexOf(',', secondCommaIndex + 1);

  String id = message.substring(2, firstCommaIndex);
  String type = message.substring(firstCommaIndex + 1, secondCommaIndex);
  String SSID = message.substring(secondCommaIndex + 1, thirdCommaIndex);
  String password = message.substring(thirdCommaIndex + 1);

  if (type == "WiFi" && (priorityValue <3)){
    bestRelayID = id;
    bestRelayType = type;
    bestRelaySSID = SSID;
    bestRelayPassword = password;
    priorityValue = 3;
  }
  if (type == "BLE" && (priorityValue <2)){
    bestRelayID = id;
    bestRelayType = type;
    serviceUUID = BLEUUID(SSID.c_str());
    charUUID = BLEUUID(password.c_str());
    // bestRelayServiceUUID = SSID;
    // bestRelayCharUUID = password;
    priorityValue = 2;
  }
  if (type == "LoRa" && (priorityValue <1)){
    bestRelayID = id;
    bestRelayType = type;
    priorityValue = 1;
  }
  
  canSend = true;
}


void sendViaWiFi(){
  WiFi.begin(bestRelaySSID.c_str(), bestRelayPassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  if (client.connect("192.168.4.1", 80)) {
    delay(100);
    client.println("Hello from Sender!");
    delay(50);
    client.stop();
  } else {
    Serial.println("Connection to relay via WiFi failed!");
  }

  WiFi.disconnect();
  Serial.println("Disconnected from Relay AP");
}

void sendViaBLE(){
  BLEDevice::getScan()->start(6);
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Gateway");
      if(pRemoteCharacteristic->canWrite()) {
        pRemoteCharacteristic->writeValue("Hello from Sender!", false);
      }
      delay(50);
      pClient->disconnect();
      delay(1000);
    } else {
      Serial.println("We have failed to connect to the server");
    }
    doConnect = false;
  }

}

void sendViaLoRa(){
  LoRa.beginPacket();
  LoRa.print("Hello from Sender!");
  LoRa.endPacket();
  delay(1000);
  Serial.println("Sent via LoRa");
}






void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);

  LoRa.setPins(SS, RST, DIO0);
  SPI.begin(14,13,2,12); 
  SPI.setFrequency(250000);

  while (!LoRa.begin(Frequency)) {
      Serial.println("Error initializing LoRa. Retrying...");
      delay(1000);
  }
  // Setting lora parameters to be able to communicate to heltec lora
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  Serial.println("Lora Initialised");
  delay(50);

  BLEDevice::init("");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

}

void loop() {
  LoRa.beginPacket();
  LoRa.print("D:" + receiverID);
  LoRa.endPacket();
  Serial.println("Discovery request sent");

  // Wait for responses
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) { // Wait for 10 seconds to collect responses
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String incoming = "";
      while (LoRa.available()) {
        incoming += (char)LoRa.read();
      }

      // Process the incoming message
      if (incoming.startsWith("R:")) {
        processResponse(incoming);
      }
    }
  }
  
  if(canSend){
    if (bestRelayType == "WiFi") {
      sendViaWiFi();
    }
    else if(bestRelayType == "BLE"){
      Serial.println(bestRelayServiceUUID);
      Serial.println(bestRelayCharUUID);
      sendViaBLE();

    }
    else{
      sendViaLoRa();
    }
    priorityValue = 0;
    canSend = false;

  }

}
