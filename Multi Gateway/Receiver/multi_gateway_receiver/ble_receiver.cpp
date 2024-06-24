#include "ble_receiver.h"

void MyServerCallbacks::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->stop();
}

void MyServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
}

void CharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
    Serial.println("Received Value: " + String(value.c_str()));
    }
}


void ble_setup() {
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

void ble_loop() {
  // put your main code here, to run repeatedly:
  if (!deviceConnected) {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    Serial.println("Start advertising");
    pAdvertising->start();
    delay(300); 
  }
}