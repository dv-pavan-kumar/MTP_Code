#include "BLEDevice.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc202-1fb5-459e-8fcc-c5c9c331915c"
#define CHARACTERISTIC_UUID "beb5484e-36e1-4688-b7f5-ea07361b26b9"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEAdvertising *pAdvertising = pServer->getAdvertising();
      pAdvertising->stop();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("Received Value: " + String(value.c_str()));
      }
    }
};

void setup() {
  Serial.begin(115200);
  delay(10);

  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE                   
                    );

  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  //BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");


}

void loop() {
  // put your main code here, to run repeatedly:
  if (!deviceConnected) {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    Serial.println("Start advertising");
    pAdvertising->start();
    delay(200); 
  }
}
