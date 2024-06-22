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


#define Frequency 865E6 // Frequency in Hz
#define SignalBandwidth 125E3 // Hz
#define SignalBandwidthIndex 0 // [0: 125 kHz,
                              // 1: 250 kHz,
                              // 2: 500 kHz,
                              // 3: Reserved]
#define SpreadingFactor 7 // [SF7..SF12]
#define CodingRate 1 // [1: 4/5,
                    // 2: 4/6,
                    // 3: 4/7,
                    // 4: 4/8]
#define CodingRateDenominator 5
#define PreambleLength 8 // LoRa preamble length, Same for Tx and Rx
#define SyncWord 0x12    // LoRa Sync Word, Same for Tx and Rx
#define OutputPower 14 // Output power in dBm
#define SymbolTimeout 0 // LoRa symbol timeout is the maximum number of symbol periods that the LoRa transceiver should wait for a valid reception before timing out
#define FixedLengthPayload false // Weather the LoRa transceiver will use fixed payload length.
#define IQInversion false   // The LoRa transceiver will use IQ inversion on the signal or not.

// UUIDs for BLE service and characteristic of BLE server
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331913a"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b25a0"
//UUID for BLE characteristic of BLE server(sender devices server) to write delay time
#define CHARACTERISTIC1_UUID "beb5143e-36e1-4688-b7f5-ea07361b26a7"

// Address definitions for LoRa Addressing
uint8_t destAddr = 20;
uint8_t myAddr = 50;

bool receivedFirstVar = false;
bool receivedDatatype = false;
bool wifiReceiver = false;
bool BleReceiver = false;
uint8_t datatype;

// The service UUID of the remote service we are interested in.(service UUID of sender device acting as ble server)
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.(characteristic UUID of sender device)
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

// BLE configuration
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

// WiFi credentials
const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";

// WiFi server objects
WiFiServer server1(80); // Server object for client 1
WiFiServer server2(8888); // Server object for client 2
WiFiClient client2;

// Callback function for BLE notifications(when data is received from sender through ble)
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,  // Remote BLE characteristic that triggered the callback
  uint8_t* pData,  // Data received in the notification
  size_t length,   // Length of the data received
  bool isNotify) {
    // Check if this is the first packet of the data item
    if(image_Size ==0 && bytes_Received == 0){
      // Extract the data size from the received data
      image_Size = *((uint32_t*)pData);
      Serial.print("Received image size: ");
      Serial.println(image_Size);

      // Forward the data size to the appropriate receiver, it can be either WiFi or BLE or LoRa
      if(wifiReceiver == true){
        // Send the data size to the WiFi receiver
        client2.write((const uint8_t*)&image_Size, sizeof(uint32_t));
      }
      else if(BleReceiver ==  true){
        // Send the data size to the BLE receiver
        pCharacteristic->setValue((uint8_t*)&image_Size, 4);
        pCharacteristic->notify();
      }
      else{
        buffer[0] = destAddr; // set the first byte with the address of LoRa receiver
        buffer[1] = 100;
        buffer[2] = 99;
        buffer[3] = 98;
        buffer[4] = 97;

        if (LoRa.beginPacket()) {
          LoRa.write(buffer, 5);  // Send a start packet first, to synchronize the receiver incase of lora packet loss
          LoRa.endPacket();
        }
        delay(250);
        
        // Pack the actual image size into the buffer
        buffer[1] = (image_Size >> 24) & 0xFF; 
        buffer[2] = (image_Size >> 16) & 0xFF;
        buffer[3] = (image_Size >> 8) & 0xFF;
        buffer[4] = image_Size & 0xFF; 
        
        if (LoRa.beginPacket()) {
          LoRa.write(buffer, 5);  // Send the actual data size over LoRa
          LoRa.endPacket();
        }
        delay(150);        
      }

      bytes_Received = 0;
      return;
    }

    // Check if the data type has been received
    if(!receivedDatatype){
      // Extract the data type(1 byte) from the received data
      datatype = *pData;
      Serial.print("Received data type: ");
      Serial.println(datatype);
      // Forward the data type to the appropriate receiver, it can be either WiFi or BLE or LoRa
      if(wifiReceiver == true){
        client2.write(&datatype, 1);  // Send data type to WiFi receiver
      }
      else if(BleReceiver ==  true){
        pCharacteristic->setValue(&datatype, 1);  // Send data type to BLE receiver
        pCharacteristic->notify();
      }
      else{
        // Send data type over LoRa
        buffer[0] = destAddr;  // set the first byte with the address of LoRa receiver
        buffer[1] = datatype;
        if (LoRa.beginPacket()) {
          LoRa.write(buffer, 2);  // Send the data type
          LoRa.endPacket();
        }
        delay(150);        
      }
      receivedDatatype = true;
      return;
    }

    // Copy the received data chunk to the buffer
    memcpy(buffer + 1, pData, length);
    bytes_Received += length;
    // Forward the received data chunk to the appropriate receiver, it can be either WiFi or BLE or LoRa
    if(wifiReceiver == true){
      client2.write(buffer+1, length);  // Send data chunk to WiFi receiver
    }
    else if(BleReceiver ==  true){
      pCharacteristic->setValue(buffer+1, length);  // Send data chunk to BLE receiver
      pCharacteristic->notify();
    }
    else{
      // We can receive 500 bytes of datathrough ble, but can't send it all at a time over LoRa.
      // So, divide received data into 250 bytes chunks and send over LoRa  
      size_t Lorabasicchunck = 250;
      for (int i = 0; i < length; i += Lorabasicchunck){
        size_t currentChunkSize = Lorabasicchunck;
        if (i + Lorabasicchunck > length) {
          currentChunkSize = length - i;  // Adjust last chunk size
        }
        buffer[i] = destAddr;
        Serial.println("sending packet to lora");
        if (LoRa.beginPacket()) {
          LoRa.write(buffer+i, currentChunkSize+1);  // Send data chunk to LoRa receiver
          LoRa.endPacket();
        }
        delay(500);
      }
      
    } 
    
    // Check if the entire data has been received and reset the variables.
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

// BLE client callback class
class MyClientCallback : public BLEClientCallbacks {
  // Callback function for BLE client when connected to a server(sender)
  void onConnect(BLEClient* pclient) {
  }
  // Callback function for BLE client when disconnected from a server(sender)
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

// Function to connect to BLE server(i.e sender)
bool connectToServer() {
    // Print the address of the device we are trying to connect to
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    is_client = true;
    
    // Create a new BLE client
    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");
    // Set callback functions to handle events from the BLE client
    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      // If the service is not found, print an error message and disconnect
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      // If the characteristic is not found, print an error message and disconnect
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

    // Register for notifications if the characteristic supports notifications
    if(pRemoteCharacteristic->canNotify()){
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
      
    // Indicate that the connection is established
    connected = true;
    is_client = false;
    return true;
}

// BLE advertised device callback class
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      // Stop scanning as we have found the desired device
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;  // Set flag to initiate connection
      doScan = true;  // Set flag to continue scanning if necessary

    } // Found our server
  } // onResult
};

// BLE server callback class
class MyServerCallbacks: public BLEServerCallbacks {
    // Server callback function called when a client(receiver) connects to the BLE server
    void onConnect(BLEServer* pServer) {
      if(!is_client){
        deviceConnected = true;
      }  
    };
    // Server callback function called when a client(receiver) disconnects from the BLE server
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// Function to connect to the appropriate receiver (WiFi or BLE, if both not present then LoRa is taken by default)
void connectToReceiver(){
  delay(100);
  // Check if a WiFi client is available
  client2 = server2.available();
  if(client2){
    wifiReceiver = true;  // Set flag indicating WiFi receiver is connected
    Serial.println("Connected to wifi receiver");
    return;
  }

  // Start advertising so as to connect to a BLE client(receiver)
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
  delay(3000);
  // Check if a BLE client has connected
  if(deviceConnected){
    Serial.println("Connected to ble receiver");
    BleReceiver =  true;  // Set flag indicating BLE receiver is connected
  }
}

// Function to send time delay calculated to the BLE server(sender)
void sendTimeDelay(uint32_t timeDelay) {
  // Get the remote service of the BLE server(sender)
  BLERemoteService *pS = pClient->getService(serviceUUID);
  if (pS != nullptr) {
    // Get the remote characteristic of the BLE server(sender)
      BLERemoteCharacteristic *pChar = pS->getCharacteristic(CHARACTERISTIC1_UUID);
      if (pChar != nullptr) {
        // Write the time delay value to the characteristic
          pChar->writeValue((uint8_t*)&timeDelay, sizeof(uint32_t));
      }
  }
}

// Function to find delay time when the sender device is connected to the gateway through WiFi
uint32_t calcWifiDelay(){
  uint32_t tDelayToSend =0;
  // If the receiver is WiFi
  if(wifiReceiver == true){
    tDelayToSend = 250;
    return tDelayToSend;
  }
  // Else if the receiver is BLE
  else if(BleReceiver ==  true){
    tDelayToSend = 750;
    return tDelayToSend;
  }
  // Else the receiver will be LoRa
  else{
    tDelayToSend = 2600;
    return tDelayToSend;
  } 
}

// Function to find delay time when the sender device is connected to the gateway through BLE
uint32_t calcBleDelay(){
  uint32_t tDelayToSend =0;
  // If the receiver is WiFi
  if(wifiReceiver == true){
    tDelayToSend = 300;
    return tDelayToSend;
  }
  // Else if the receiver is BLE
  else if(BleReceiver ==  true){
    tDelayToSend = 400;
    return tDelayToSend;
  }
  // Else the receiver will be LoRa
  else{
    tDelayToSend = 1200;
    return tDelayToSend;
  }  
}

// Function to find delay time when the sender device is connected to the gateway through LoRa
uint32_t calcLoraDelay(){
  uint32_t tDelayToSend =0;
  // If the receiver is WiFi
  if(wifiReceiver == true){
    tDelayToSend = 600;
    return tDelayToSend;
  }
  // Else if the receiver is BLE
  else if(BleReceiver ==  true){
    tDelayToSend = 600;
    return tDelayToSend;
  }
  // Else the receiver will be LoRa
  else{
    tDelayToSend = 1200;
    return tDelayToSend;
  }  
}

// Function called when we receive a lora packet
void LoRaReceived()
{
  // copy the entire data into buffer
  int size = 0;
  while (LoRa.available()) { 
    buffer[size] = LoRa.read();
    size++;
  }
  // Extract the receiver address from the buffer
  uint8_t receiverAddr = buffer[0];
  // Check if the received message is addressed to this device
  if(receiverAddr == myAddr){
    // If we have not received start packet
    if(!receivedFirstVar){
      // Extract the sender address from the buffer
      uint8_t SenderAddr = buffer[1];
      Serial.print("Received sender addr: ");
      Serial.println(SenderAddr);
      // Connect to the appropriate receiver
      connectToReceiver();
      // Calculate the delay time and send it back to the sender
      uint32_t tdelay = calcLoraDelay();
      uint8_t delay_buff[5];
      delay_buff[0] = SenderAddr;  // set the first byte with the address of LoRa sender
      // Pack the delay time into the buffer
      delay_buff[1] = (tdelay >> 24) & 0xFF; 
      delay_buff[2] = (tdelay >> 16) & 0xFF;
      delay_buff[3] = (tdelay >> 8) & 0xFF;
      delay_buff[4] = tdelay & 0xFF;
      if (LoRa.beginPacket()) {
        LoRa.write(delay_buff, 5); // Send the delay time over LoRa
        LoRa.endPacket();
      }
      delay(200);
      receivedFirstVar = true;
    }
    else{
      // If the data size and received bytes are zero, extract the data size
      if(bytes_Received == 0 && image_Size ==0){
        image_Size = ((uint32_t)buffer[1] << 24) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 8) | buffer[4];
        Serial.print("Received image size: ");
        Serial.println(image_Size);

        // Forward the data size to the appropriate receiver, it can be either WiFi or BLE or LoRa
        if(wifiReceiver == true){
          client2.write((const uint8_t*)&image_Size, sizeof(uint32_t));  // Send the data size to WiFi receiver
        }
        else if(BleReceiver ==  true){
          pCharacteristic->setValue((uint8_t*)&image_Size, 4);  // Send the data size to BLE receiver
          pCharacteristic->notify();
        }
        else{
          // Send the image size over LoRa
          buffer[0] = destAddr;  // set the first byte with the address of LoRa receiver
          buffer[1] = 100;
          buffer[2] = 99;
          buffer[3] = 98;
          buffer[4] = 97;
          Serial.println("sending packet to lora");
          if (LoRa.beginPacket()) {
            LoRa.write(buffer, 5); // Send a start packet first, to synchronize the receiver incase of lora packet loss
            LoRa.endPacket();
          }
          delay(250);
          
          // Pack the actual data size into the buffer
          buffer[1] = (image_Size >> 24) & 0xFF; 
          buffer[2] = (image_Size >> 16) & 0xFF;
          buffer[3] = (image_Size >> 8) & 0xFF;
          buffer[4] = image_Size & 0xFF; 
          
          if (LoRa.beginPacket()) { 
            LoRa.write(buffer, 5); // Send the data size over LoRa
            LoRa.endPacket();
          }
          delay(150);
        }
        return;
      }

      // If the data type has not been received, extract the data type
      if(!receivedDatatype){
        datatype = buffer[1];
        Serial.print("Received data type: ");
        Serial.println(datatype);
        // Forward the data type to the appropriate receiver
        if(wifiReceiver == true){
          client2.write(&datatype, 1);
        }
        else if(BleReceiver ==  true){
          pCharacteristic->setValue(&datatype, 1);
          pCharacteristic->notify();
        }
        else{
          buffer[0] = destAddr;
          // Pack data type into buffer
          buffer[1] = datatype;
          
          if (LoRa.beginPacket()) {
            LoRa.write(buffer, 2);
            LoRa.endPacket();
          }
          delay(150);        
        }
        receivedDatatype = true;
        return;
      }
      
      // Handle the received data chunk
      uint8_t var2; // this variable is used represent the number of bytes of data to be sent
      var2 = size & 0xFF;
      var2--; // we have to decrease by one as the first byte is the address and we don't send it in BLE or wifi
      // Forward the received data chunk to the appropriate receiver
      if(wifiReceiver == true){
        // we will not send the 1st byte as it is address, so we will send from "buffer+1"
        client2.write(buffer+1, var2);     
      }
      else if(BleReceiver ==  true){
        // we will not send the 1st byte as it is address, so we will send from "buffer+1"
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
      // Check if the entire data has been received
      if(bytes_Received == image_Size){
        // Reset the variables
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
  Serial.begin(115200); // Initialize serial communication at 115200 baud
  delay(10);

  // Set Up Wi-Fi as an Access Point(to connect to sender and receiver)
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  IPAddress IP = WiFi.softAPIP();  // Get the IP address of the Access Point
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Start the WiFi servers
  server1.begin(); // Start server1 on port 80
  server2.begin(); // Start server2 on port 8888
  delay(50);

  // Set up LoRa
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

  BLEDevice::init(""); // Initialize the BLE device

  pServer = BLEDevice::createServer(); // Create a BLE server
  pServer->setCallbacks(new MyServerCallbacks()); // Set the server callbacks

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

  // Configure BLE advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the scan
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
}



void loop() {
  // If connected to the LoRa sender, then only listen to incoming lora packets untill all the data is received.
  if (receivedFirstVar) {
    int packetSize = LoRa.parsePacket();
    // If received a lora packet
    if (packetSize) {
      LoRaReceived();
    }
  }
  // If not connected to lora sender
  else{
    // Check for available WiFi client(sender) on server1
    WiFiClient client1 = server1.available();
    if (client1) {
      Serial.println("Wifi Sender connected");
      // Connect to the appropriate receiver
      connectToReceiver();
      // Calculate and send the WiFi delay to the sender
      uint32_t timeDelayToSend = calcWifiDelay();
      client1.write((const uint8_t*)&timeDelayToSend, sizeof(uint32_t));
      delay(50);

      // Read data from the client
      while (client1.connected()) {
        if (client1.available()) {
          uint32_t imageSize;
          // read the data size from the client
          size_t bytesReceived = client1.readBytes(reinterpret_cast<char*>(&imageSize), sizeof(imageSize));
          
          Serial.print("Received image size: ");
          Serial.println(imageSize);

          // Forward the data size to the appropriate receiver
          if(wifiReceiver == true){
            client2.write((const uint8_t*)&imageSize, sizeof(uint32_t)); // Send the data size to WiFi receiver
          }
          else if(BleReceiver ==  true){
            pCharacteristic->setValue((uint8_t*)&imageSize, 4);  // Send the data size to BLE receiver
            pCharacteristic->notify();
          }
          else{
            // Send the data size over LoRa
            buffer[0] = destAddr;  // set the first byte with the address of LoRa receiver
            buffer[1] = 100;
            buffer[2] = 99;
            buffer[3] = 98;
            buffer[4] = 97;
            
            if (LoRa.beginPacket()) {
              LoRa.write(buffer, 5);  // Send a start packet first, to synchronize the receiver incase of lora packet loss
              LoRa.endPacket();
            }
            delay(250);

            // Pack data size into buffer (remaining 4 bytes)
            buffer[1] = (imageSize >> 24) & 0xFF; 
            buffer[2] = (imageSize >> 16) & 0xFF;
            buffer[3] = (imageSize >> 8) & 0xFF;
            buffer[4] = imageSize & 0xFF; 
            
            Serial.println("sending packet to lora");
            if (LoRa.beginPacket()) {
              LoRa.write(buffer, 5); // Send the data size over LoRa
              LoRa.endPacket();
            }
            delay(150);
          }

          // continuously check untill we receive data
          while (client1.connected()){
            if (client1.available()){
              // read the data type
              datatype = client1.read();
              Serial.print("Received data type: ");
              Serial.println(datatype);

              // Forward the data type to the appropriate receiver
              if(wifiReceiver == true){
                client2.write(&datatype, 1);  // Send the data type to WiFi receiver
              }
              else if(BleReceiver ==  true){
                pCharacteristic->setValue(&datatype, 1);  // Send the data type to ble receiver
                pCharacteristic->notify();
              }
              else{
                buffer[0] = destAddr;
                buffer[1] = datatype; 
                Serial.println("sending packet to lora");
                if (LoRa.beginPacket()) {
                  LoRa.write(buffer, 2);  // Send the data type to lora receiver
                  LoRa.endPacket();
                }
                delay(150);
              }
              break;
            }
          }         
          
          // receive data chunks
          // The basic chunck size is 1250 bytes for wifi
          uint32_t bytesRemaining = imageSize; // number of bytes of data is left to read
          while (bytesRemaining > 0) {
            // If data is available to read
            if (client1.available()){ 
              size_t len = client1.available(); 
              // Read the data chunk to the buffer        
              bytesReceived = client1.readBytes(reinterpret_cast<char*>(buffer+1), len);
              if (bytesReceived != len) {
                Serial.println("Error receiving image chunk");
                return;
              }
              bytesRemaining -= bytesReceived;

              // Forward the received data chunk to the appropriate receiver
              if(wifiReceiver == true){
                client2.write(buffer+1, len);  // forward the chunk as is it to the wifi receiver
              }
              else if(BleReceiver ==  true){
                // We can receive 1250 bytes of datathrough wifi, but can't send it all at a time over ble or LoRa.
                // So, divide received data into 500 bytes chunks and send over ble
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
                // We can receive 1250 bytes of datathrough wifi, but can't send it all at a time over ble or LoRa.
                // So, divide received data into 500 bytes chunks and send over ble
                size_t chunkSize = 250;
                for (int i = 0; i < len; i += chunkSize){
                  size_t currentChunkSize = chunkSize;
                  if (i + chunkSize > len) {
                    currentChunkSize = len - i;  // Adjust last chunk size
                  }
                  buffer[i] = destAddr;          
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

      // Close the connection to the sender
      client1.stop();
      Serial.println("Client disconnected");
    }
    // Start BLE scanning for server(ble sender)
    BLEDevice::getScan()->start(1);
    // if found the desired sender
    if (doConnect == true) {
      // connect to the sender
      if (connectToServer()) {
        Serial.println("We are now connected to the BLE Sender.");
        // Connect to the appropriate receiver
        connectToReceiver();
        // Calculate and send the ble delay to the sender
        uint32_t timeDelayToSend = calcBleDelay();
        sendTimeDelay(timeDelayToSend);
      } else {
        Serial.println("We have failed to connect to the server");
      }
      doConnect = false;
    }
    
    // check if lora packet is received
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      LoRaReceived();
    }

  }  
}
