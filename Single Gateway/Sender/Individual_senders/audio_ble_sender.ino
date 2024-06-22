#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Arduino.h"



// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC1_UUID "beb5143e-36e1-4688-b7f5-ea07361b26a7"

const char *soft_ap_ssid = "MyESP32CAM";
const char *soft_ap_password = "testpassword";
WiFiServer server(800);

uint8_t audio_data[60000];
uint32_t audioSize;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic1 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t timeDelay = 0;
bool messageSendingEnabled = false;



class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic1) {
        std::string value = pCharacteristic1->getValue();
        if (value.length() == sizeof(uint32_t)) {
            memcpy(&timeDelay, value.data(), sizeof(uint32_t));
            messageSendingEnabled = true;
        }
    }
};


bool receiveAudio(){
  WiFiClient client1 = server.available();
  if (client1) {
    Serial.println("Client connected");

    // Read data from the client
    while (client1.connected()) {
      if (client1.available()) {

        size_t bytesReceived = client1.readBytes(reinterpret_cast<char*>(&audioSize), sizeof(audioSize));
        if (bytesReceived != sizeof(audioSize)) {
          Serial.println("Error receiving audio size");
          return false;
        }
        Serial.print("Received audio size: ");
        Serial.println(audioSize);

        uint32_t basicchunck= 250;
        uint32_t bytesRemaining = audioSize;
        uint32_t bytesRecvd = 0;
        while (bytesRemaining > 0) {
          size_t chunkSize = min(basicchunck, bytesRemaining);
          if (client1.available()){
            bytesReceived = client1.readBytes(reinterpret_cast<char*>(audio_data+bytesRecvd), chunkSize);
            if (bytesReceived != chunkSize) {
              Serial.println("Error receiving audio chunk");
              return false;
            }
            bytesRemaining -= bytesReceived;
            bytesRecvd += bytesReceived;
            delay(10);
            Serial.println("-------");
            delay(10);
            Serial.write(audio_data+bytesRecvd, chunkSize);
            delay(10);
            Serial.println("-------");
          }         
        }

        Serial.println("Audio received:");
        break;
      }
    }

    // Close the connection
    client1.stop();
    Serial.println("Client disconnected");
    return true;
  }
  return false;
}



void setup() {
  Serial.begin(115200);
  delay(10);

  //WiFi.mode(WIFI_MODE_APSTA); 	
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  Serial.print("ESP32 CAM IP as soft AP:");
  Serial.println(WiFi.softAPIP());

  server.begin();
  Serial.println("Server started");

  
  // Create the BLE Device
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
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic1 = pService->createCharacteristic(
                      CHARACTERISTIC1_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  
                    );

  pCharacteristic1->setCallbacks(new CharacteristicCallbacks());
  pCharacteristic1->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }
  // notify changed value
  if (deviceConnected) {
    Serial.println("Connected to ble client");

    while(deviceConnected){
      if(messageSendingEnabled){
        Serial.println("Waiting for audio");
        while(!receiveAudio());
        Serial.print("Audio size is: ");
        Serial.println(audioSize);
        
        pCharacteristic->setValue((uint8_t*)&audioSize, 4);
        pCharacteristic->notify();
        delay(500);

        size_t bytesSent = 0;
        size_t basicchunck = 250;
        while (bytesSent < audioSize) {
          size_t chunkSize = min(basicchunck, audioSize - bytesSent); // Chunk size of 250 bytes
          pCharacteristic->setValue(audio_data + bytesSent, chunkSize);
          pCharacteristic->notify();       
          delay(50);
          Serial.println("-------");
          Serial.write(audio_data + bytesSent, chunkSize);
          Serial.println("-------");
          delay(800);
          bytesSent += chunkSize;
        }

        delay(500);

        messageSendingEnabled = false;
        break;
      }
    }

    
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(300); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
}