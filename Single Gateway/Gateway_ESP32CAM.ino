#include <LoRa.h>
#include <SPI.h>
#include "Arduino.h"
#include <WiFi.h>
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


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331913a"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b25a0"
//client write
#define CHARACTERISTIC1_UUID "beb5143e-36e1-4688-b7f5-ea07361b26a7"

uint8_t destAddr = 20;
uint8_t myAddr = 50;

bool receivedFirstVar = false;
bool receivedDatatype = false;
bool wifiReceiver = false;
bool BleReceiver = false;
uint8_t datatype;

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

BLEClient*  pClient = NULL;
BLEScan* pBLEScan = NULL;
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool is_client = false;

uint8_t buffer[1251]; // Buffer to hold image chunks
uint32_t image_Size = 0;
size_t bytes_Received = 0;

const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";

WiFiServer server1(80); // Server object for client 1
WiFiServer server2(8888); // Server object for client 2
WiFiClient client2;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    //Serial.print("Received image size: ");
    if(image_Size ==0 && bytes_Received == 0){
      image_Size = *((uint32_t*)pData);
      Serial.print("Received image size: ");
      Serial.println(image_Size);

      if(wifiReceiver == true){
        client2.write((const uint8_t*)&image_Size, sizeof(uint32_t));
      }
      else if(BleReceiver ==  true){
        pCharacteristic->setValue((uint8_t*)&image_Size, 4);
        pCharacteristic->notify();
      }
      else{
        buffer[0] = destAddr;
        buffer[1] = 100;
        buffer[2] = 99;
        buffer[3] = 98;
        buffer[4] = 97;
        Serial.println("sending packet to lora");
        //Radio.Send((uint8_t*)&imageSize, sizeof(imageSize));
        if (LoRa.beginPacket()) {
          LoRa.write(buffer, 5);
          LoRa.endPacket();
        }
        delay(250);
        
        // Pack size into buffer (remaining 4 bytes)
        buffer[1] = (image_Size >> 24) & 0xFF; 
        buffer[2] = (image_Size >> 16) & 0xFF;
        buffer[3] = (image_Size >> 8) & 0xFF;
        buffer[4] = image_Size & 0xFF; 
        
        Serial.println("sending packet to lora");
        //Radio.Send((uint8_t*)&imageSize, sizeof(imageSize));
        if (LoRa.beginPacket()) {
          LoRa.write(buffer, 5);
          LoRa.endPacket();
        }
        delay(150);        
      }

      bytes_Received = 0;
      return;
    }

    if(!receivedDatatype){
      datatype = *pData;
      Serial.print("Received data type: ");
      Serial.println(datatype);
      if(wifiReceiver == true){
        client2.write(&datatype, 1);
      }
      else if(BleReceiver ==  true){
        pCharacteristic->setValue(&datatype, 1);
        pCharacteristic->notify();
      }
      else{
        buffer[0] = destAddr;
        // Pack size into buffer (remaining 4 bytes)
        buffer[1] = datatype;
        Serial.println("sending packet to lora");
        if (LoRa.beginPacket()) {
          LoRa.write(buffer, 2);
          LoRa.endPacket();
        }
        delay(150);        
      }
      receivedDatatype = true;
      return;
    }

    memcpy(buffer + 1, pData, length);
    bytes_Received += length;
    Serial.println("-------");
    Serial.write(buffer + 1, length);
    delay(10);
    Serial.println("-------");
    if(wifiReceiver == true){
      client2.write(buffer+1, length);     
    }
    else if(BleReceiver ==  true){
      pCharacteristic->setValue(buffer+1, length);
      pCharacteristic->notify();
    }
    else{
      size_t Lorabasicchunck = 250;
      for (int i = 0; i < length; i += Lorabasicchunck){
        size_t currentChunkSize = Lorabasicchunck;
        if (i + Lorabasicchunck > length) {
          currentChunkSize = length - i;  // Adjust last chunk size
        }
        buffer[i] = destAddr;
        Serial.println("sending packet to lora");
        if (LoRa.beginPacket()) {
          LoRa.write(buffer+i, currentChunkSize+1); 
          LoRa.endPacket();
        }
        delay(500);
      }
      
    } 
    

    if (bytes_Received == image_Size) {
      Serial.println("Image received successfully");
      // Reset variables for the next image
      image_Size = 0;
      bytes_Received = 0;
      wifiReceiver = false;
      BleReceiver =  false;
      receivedDatatype = false;
      pClient->disconnect();
    }
}


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
    is_client = true;
    
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

    if(pRemoteCharacteristic->canNotify()){
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
      

    connected = true;
    is_client = false;
    return true;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      if(!is_client){
        deviceConnected = true;
      }  
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


void connectToReceiver(){
  //Serial.println("Connecting to receiver");
  delay(100);
  client2 = server2.available();
  if(client2){
    wifiReceiver = true;
    Serial.println("Connected to wifi receiver");
    return;
  }

  
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
  delay(3000);
  if(deviceConnected){
    Serial.println("Connected to ble receiver");
    BleReceiver =  true;
  }
}


void sendTimeDelay(uint32_t timeDelay) {

  BLERemoteService *pS = pClient->getService(serviceUUID);
  if (pS != nullptr) {
      BLERemoteCharacteristic *pChar = pS->getCharacteristic(CHARACTERISTIC1_UUID);
      if (pChar != nullptr) {
          pChar->writeValue((uint8_t*)&timeDelay, sizeof(uint32_t));
      }
  }
}

uint32_t calcWifiDelay(){
  uint32_t tDelayToSend =0;
  if(wifiReceiver == true){
    tDelayToSend = 250;
    return tDelayToSend;
  }
  else if(BleReceiver ==  true){
    tDelayToSend = 750;
    return tDelayToSend;
  }
  else{
    tDelayToSend = 2600;
    return tDelayToSend;
  } 
}

uint32_t calcBleDelay(){
  uint32_t tDelayToSend =0;
  if(wifiReceiver == true){
    tDelayToSend = 300;
    return tDelayToSend;
  }
  else if(BleReceiver ==  true){
    tDelayToSend = 400;
    return tDelayToSend;
  }
  else{
    tDelayToSend = 1200;
    return tDelayToSend;
  }  
}

uint32_t calcLoraDelay(){
  uint32_t tDelayToSend =0;
  if(wifiReceiver == true){
    tDelayToSend = 600;
    return tDelayToSend;
  }
  else if(BleReceiver ==  true){
    tDelayToSend = 600;
    return tDelayToSend;
  }
  else{
    tDelayToSend = 1200;
    return tDelayToSend;
  }  
}

void LoRaReceived()
{
  int size = 0;
  while (LoRa.available()) { 
    buffer[size] = LoRa.read();
    size++;
  }
  uint8_t receiverAddr = buffer[0];
  if(receiverAddr == myAddr){
    if(!receivedFirstVar){
      uint8_t SenderAddr = buffer[1];
      Serial.print("Received sender addr: ");
      Serial.println(SenderAddr);
      connectToReceiver();
      uint32_t tdelay = calcLoraDelay();
      uint8_t delay_buff[5];
      delay_buff[0] = SenderAddr;
      delay_buff[1] = (tdelay >> 24) & 0xFF; 
      delay_buff[2] = (tdelay >> 16) & 0xFF;
      delay_buff[3] = (tdelay >> 8) & 0xFF;
      delay_buff[4] = tdelay & 0xFF;
      if (LoRa.beginPacket()) {
        LoRa.write(delay_buff, 5);
        LoRa.endPacket();
      }
      delay(200);
      receivedFirstVar = true;
    }
    else{
      if(bytes_Received == 0 && image_Size ==0){
        image_Size = ((uint32_t)buffer[1] << 24) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 8) | buffer[4];
        Serial.print("Received image size: ");
        Serial.println(image_Size);
        if(wifiReceiver == true){
          client2.write((const uint8_t*)&image_Size, sizeof(uint32_t));
        }
        else if(BleReceiver ==  true){
          pCharacteristic->setValue((uint8_t*)&image_Size, 4);
          pCharacteristic->notify();
        }
        else{
          buffer[0] = destAddr; 
          buffer[1] = 100;
          buffer[2] = 99;
          buffer[3] = 98;
          buffer[4] = 97;
          Serial.println("sending packet to lora");
          if (LoRa.beginPacket()) {
            LoRa.write(buffer, 5);
            LoRa.endPacket();
          }
          delay(250);

          buffer[1] = (image_Size >> 24) & 0xFF; 
          buffer[2] = (image_Size >> 16) & 0xFF;
          buffer[3] = (image_Size >> 8) & 0xFF;
          buffer[4] = image_Size & 0xFF; 
          Serial.println("sending packet to lora");
          if (LoRa.beginPacket()) {
            LoRa.write(buffer, 5);
            LoRa.endPacket();
          }
          delay(150);
        }
        return;
      }

      if(!receivedDatatype){
        datatype = buffer[1];
        Serial.print("Received data type: ");
        Serial.println(datatype);
        if(wifiReceiver == true){
          client2.write(&datatype, 1);
        }
        else if(BleReceiver ==  true){
          pCharacteristic->setValue(&datatype, 1);
          pCharacteristic->notify();
        }
        else{
          buffer[0] = destAddr;
          // Pack size into buffer (remaining 4 bytes)
          buffer[1] = datatype;
          
          Serial.println("sending packet to lora");
          //Radio.Send((uint8_t*)&imageSize, sizeof(imageSize));
          if (LoRa.beginPacket()) {
            LoRa.write(buffer, 2);
            LoRa.endPacket();
          }
          delay(150);        
        }
        receivedDatatype = true;
        return;
      }
      
      Serial.println("-------");
      delay(10);
      uint8_t var2;
      var2 = size & 0xFF;
      var2--;
      Serial.println(var2);
      Serial.write(buffer+1, var2);
      delay(10);
      Serial.println("-------");
      if(wifiReceiver == true){
        client2.write(buffer+1, var2);     
      }
      else if(BleReceiver ==  true){
        pCharacteristic->setValue(buffer+1, var2);
        pCharacteristic->notify();
      }
      else{
        buffer[0] = destAddr;
        Serial.println("sending packet to lora");
        if (LoRa.beginPacket()) {
          LoRa.write(buffer, var2 +1);
          LoRa.endPacket();
        }	
        delay(500);       
      } 

      bytes_Received += var2;
      if(bytes_Received == image_Size){
        bytes_Received = 0;
        image_Size = 0;
        wifiReceiver = false;
        BleReceiver =  false;
        receivedFirstVar = false;
        receivedDatatype = false;
      }
      
    }
  }
}




void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to Wi-Fi
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Start the server
  server1.begin();
  server2.begin();
  delay(50);

  LoRa.setPins(SS, RST, DIO0);
  SPI.begin(14,13,2,12); 
  SPI.setFrequency(250000);

  while (!LoRa.begin(Frequency)) {
      Serial.println("Error initializing LoRa. Retrying...");
      delay(1000);
  }

  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  Serial.println("LoRa Initialized");
  delay(50);

  BLEDevice::init("");

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

  // Start the service
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
}



void loop() {
  if (receivedFirstVar) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      LoRaReceived();
    }
  }
  else{
    WiFiClient client1 = server1.available();
    //Serial.println("checking");
    if (client1) {
      Serial.println("Wifi Sender connected");
      connectToReceiver();
      uint32_t timeDelayToSend = calcWifiDelay();
      client1.write((const uint8_t*)&timeDelayToSend, sizeof(uint32_t));
      delay(50);

      // Read data from the client
      while (client1.connected()) {
        if (client1.available()) {
          uint32_t imageSize;
          size_t bytesReceived = client1.readBytes(reinterpret_cast<char*>(&imageSize), sizeof(imageSize));
          
          Serial.print("Received image size: ");
          Serial.println(imageSize);
          if(wifiReceiver == true){
            client2.write((const uint8_t*)&imageSize, sizeof(uint32_t));
          }
          else if(BleReceiver ==  true){
            pCharacteristic->setValue((uint8_t*)&imageSize, 4);
            pCharacteristic->notify();
          }
          else{
            buffer[0] = destAddr;
            buffer[1] = 100;
            buffer[2] = 99;
            buffer[3] = 98;
            buffer[4] = 97;
            Serial.println("sending packet to lora");
            //Radio.Send((uint8_t*)&imageSize, sizeof(imageSize));
            if (LoRa.beginPacket()) {
              LoRa.write(buffer, 5);
              LoRa.endPacket();
            }
            delay(250);

            // Pack size into buffer (remaining 4 bytes)
            buffer[1] = (imageSize >> 24) & 0xFF; 
            buffer[2] = (imageSize >> 16) & 0xFF;
            buffer[3] = (imageSize >> 8) & 0xFF;
            buffer[4] = imageSize & 0xFF; 
            
            Serial.println("sending packet to lora");
            if (LoRa.beginPacket()) {
              LoRa.write(buffer, 5);
              LoRa.endPacket();
            }
            delay(150);
          }

          while (client1.connected()){
            if (client1.available()){
              datatype = client1.read();
              Serial.print("Received data type: ");
              Serial.println(datatype);
              if(wifiReceiver == true){
                client2.write(&datatype, 1);
              }
              else if(BleReceiver ==  true){
                pCharacteristic->setValue(&datatype, 1);
                pCharacteristic->notify();
              }
              else{
                buffer[0] = destAddr;
                buffer[1] = datatype; 
                Serial.println("sending packet to lora");
                if (LoRa.beginPacket()) {
                  LoRa.write(buffer, 2);
                  LoRa.endPacket();
                }
                delay(150);
              }
              break;
            }
          }         
          
          //uint32_t basicchunck= 1250;
          uint32_t bytesRemaining = imageSize;
          while (bytesRemaining > 0) {
            //size_t chunkSize = min(basicchunck, bytesRemaining);
            if (client1.available()){ 
              size_t len = client1.available();         
              bytesReceived = client1.readBytes(reinterpret_cast<char*>(buffer+1), len);
              if (bytesReceived != len) {
                Serial.println("Error receiving image chunk");
                return;
              }
              bytesRemaining -= bytesReceived;
              Serial.println("-------");
              Serial.write(buffer+1, len);
              delay(10);
              Serial.println("-------");
              delay(10);
              if(wifiReceiver == true){
                client2.write(buffer+1, len);     
              }
              else if(BleReceiver ==  true){
                size_t chunkSize = 500;
                for (int i = 0; i < len; i += chunkSize){
                  size_t currentChunkSize = chunkSize;
                  if (i + chunkSize > len) {
                    currentChunkSize = len - i;  // Adjust last chunk size
                  }
                  pCharacteristic->setValue(buffer+1+i, currentChunkSize);
                  pCharacteristic->notify();
                  delay(100);
                }
              }
              else{
                size_t chunkSize = 250;
                for (int i = 0; i < len; i += chunkSize){
                  size_t currentChunkSize = chunkSize;
                  if (i + chunkSize > len) {
                    currentChunkSize = len - i;  // Adjust last chunk size
                  }
                  buffer[i] = destAddr;
                  Serial.println("sending packet to lora");
                  if (LoRa.beginPacket()) {
                    LoRa.write(buffer+i, currentChunkSize+1);
                    LoRa.endPacket();
                  }
                  delay(500);                             
                }
             
              } 
            }         
          }
          Serial.println("Image received:");
          wifiReceiver = false;
          BleReceiver =  false;
          break;
        }
      }

      // Close the connection
      client1.stop();
      Serial.println("Client disconnected");
    }
    
    BLEDevice::getScan()->start(1);
    if (doConnect == true) {
      if (connectToServer()) {
        Serial.println("We are now connected to the BLE Sender.");
        connectToReceiver();
        uint32_t timeDelayToSend = calcBleDelay();
        sendTimeDelay(timeDelayToSend);
      } else {
        Serial.println("We have failed to connect to the server");
      }
      doConnect = false;
    }

    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      LoRaReceived();
    }

  }  
}
