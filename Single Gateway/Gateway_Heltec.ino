#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include "BLEDevice.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// LoRa configuration parameters for LoRaWan_APP.h
#define RF_FREQUENCY                                865000000 // Frequency in Hz

#define TX_OUTPUT_POWER                             14        // Output power in dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // LoRa preamble length, Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // LoRa symbol timeout
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false     // Fixed length payload
#define LORA_IQ_INVERSION_ON                        false     // IQ inversion
#define RX_TIMEOUT_VALUE                            1000      // Receive timeout value in ms

// UUIDs for BLE service and characteristic of BLE server
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331913a"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b25a0"
//UUID for BLE characteristic of BLE server(sender devices server) to write delay time
#define CHARACTERISTIC1_UUID "beb5143e-36e1-4688-b7f5-ea07361b26a7"

// Address definitions for LoRa Addressing
uint8_t destAddr = 20;
uint8_t myAddr = 50;

bool lora_idle=true;
bool receivedFirstVar = false;
bool receivedDatatype = false;
bool wifiReceiver = false;
bool BleReceiver = false;
int16_t rssi,rxSize;
uint8_t datatype;

static RadioEvents_t RadioEvents;

// The service UUID of the remote service we are interested in.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
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

uint8_t buffer[1251]; // Buffer to hold data chunks
uint32_t image_Size = 0;
size_t bytes_Received = 0;

// WiFi credentials
const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";

// WiFi server objects
WiFiServer server1(80); // Server object for client 1
WiFiServer server2(8888); // Server object for client 2
WiFiClient client2;

// Callback function for LoRa TX done event
void OnTxDone( void )
{
  Serial.println("TX done......");
  lora_idle = true;
}

// Callback function for LoRa TX timeout event
void OnTxTimeout( void )
{
  Radio.Sleep( );
  Serial.println("TX Timeout......");
  lora_idle = true;
}

// Callback function for BLE notifications(when data is received from sender)
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, // Remote BLE characteristic that triggered the callback
  uint8_t* pData, // Data received in the notification
  size_t length, // Length of the data received
  bool isNotify) {
    // Check if this is the first packet of the data item
    if(image_Size ==0 && bytes_Received == 0){
      // Extract the data size from the received data
      image_Size = *((uint32_t*)pData);
      Serial.print("Received data size: ");
      Serial.println(image_Size);

      // Forward the data size to the appropriate receiver
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
        Serial.println("sending packet to lora");
        Radio.Send(buffer, 5); // Send dummy data first to synchronize the receiver
        delay(200);
        Radio.IrqProcess( );
        delay(50);

        // Pack the actual image size into the buffer
        buffer[1] = (image_Size >> 24) & 0xFF; 
        buffer[2] = (image_Size >> 16) & 0xFF;
        buffer[3] = (image_Size >> 8) & 0xFF;
        buffer[4] = image_Size & 0xFF; 
        
        Serial.println("sending packet to lora");
        Radio.Send(buffer, 5); // Send the actual data size over LoRa
        delay(150);        
        Radio.IrqProcess( ); // process the LoRa callback functions
      }

      bytes_Received = 0;
      return;
    }

    // Check if the data type has been received
    if(!receivedDatatype){
      // Extract the data type from the received data
      datatype = *pData;
      Serial.print("Received data type: ");
      Serial.println(datatype);
      // Forward the data type to the appropriate receiver
      if(wifiReceiver == true){
        client2.write(&datatype, 1);  // Send data type to WiFi receiver
      }
      else if(BleReceiver ==  true){
        pCharacteristic->setValue(&datatype, 1);  // Send data type to BLE receiver
        pCharacteristic->notify();
      }
      else{
        // Send data type over LoRa
        buffer[0] = destAddr; // set the first byte with the address of LoRa receiver
        buffer[1] = datatype;
        
        Serial.println("sending packet to lora");
        Radio.Send(buffer, 2);  // Send the data type
        delay(150);        
        Radio.IrqProcess( );
      }
      receivedDatatype = true;
      return;
    }

    // Copy the received data chunk to the buffer
    memcpy(buffer + 1, pData, length);
    bytes_Received += length;
    Serial.println("-------");
    Serial.write(buffer + 1, length);
    delay(10);
    Serial.println("-------");

    // Forward the received data chunk to the appropriate receiver
    if(wifiReceiver == true){
      client2.write(buffer+1, length);     // Send data chunk to WiFi receiver
    }
    else if(BleReceiver ==  true){
      pCharacteristic->setValue(buffer+1, length);  // Send data chunk to BLE receiver
      pCharacteristic->notify();
    }
    else{
      // Send received data chunk over LoRa in chunks of size 250 bytes
      size_t Lorabasicchunck = 250;
      for (int i = 0; i < length; i += Lorabasicchunck){
        size_t currentChunkSize = Lorabasicchunck;
        if (i + Lorabasicchunck > length) {
          currentChunkSize = length - i;  // Adjust last chunk size
        }
        buffer[i] = destAddr;
        Serial.println("sending packet to lora");
        Radio.Send(buffer+i, currentChunkSize+1); // Send the image data chunk
        delay(500);
        Radio.IrqProcess( );
      }
      
    } 
    
    // Check if the entire data has been received
    if (bytes_Received == image_Size) {
      Serial.println("Data received successfully");
      // Reset variables for the next data item
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
  // Callback function for BLE client when connected to a server
  void onConnect(BLEClient* pclient) {
  }

  // Callback function for BLE client when disconnected from a server
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

// Function to connect to BLE server
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

    // Connect to the remote BLE server using the advertised device
    pClient->connect(myDevice);  // Automatically handles public/private address types
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

    // Read the value of the characteristic if it is readable.
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
      myDevice = new BLEAdvertisedDevice(advertisedDevice);  // Store the advertised device
      doConnect = true;  // Set flag to initiate connection
      doScan = true;  // Set flag to continue scanning if necessary

    } // Found our server
  } // onResult
};

// BLE server callback class
class MyServerCallbacks: public BLEServerCallbacks {
    // Server callback function called when a client connects to the BLE server
    void onConnect(BLEServer* pServer) {
      if(!is_client){
        deviceConnected = true;
      }  
    };

    // Server callback function called when a client disconnects from the BLE server
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// Function to connect to the appropriate receiver (WiFi or BLE)
void connectToReceiver(){
  //Serial.println("Connecting to receiver");
  delay(100);
  // Check if a WiFi client is available
  client2 = server2.available();
  if(client2){
    wifiReceiver = true;  // Set flag indicating WiFi receiver is connected
    Serial.println("Connected to wifi receiver");
    return;
  }

  // Start advertising for a BLE client connection
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
  delay(3000);
  // Check if a BLE client has connected
  if(deviceConnected){
    Serial.println("Connected to ble receiver");
    BleReceiver =  true; // Set flag indicating BLE receiver is connected
  }
}


// Function to send time delay to the BLE server
void sendTimeDelay(uint32_t timeDelay) {
  // Get the remote service
  BLERemoteService *pS = pClient->getService(serviceUUID);
  if (pS != nullptr) {
      // Get the remote characteristic
      BLERemoteCharacteristic *pChar = pS->getCharacteristic(CHARACTERISTIC1_UUID);
      if (pChar != nullptr) {
          // Write the time delay value to the characteristic
          pChar->writeValue((uint8_t*)&timeDelay, sizeof(uint32_t));
      }
  }
}

// Function to calculate delay for WiFi sender
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

// Function to calculate delay for BLE sender
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
    tDelayToSend = 1250;
    return tDelayToSend;
  }  
}

// Function to calculate delay for LoRa sender
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

// Callback function for LoRa RX done event
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
  // Store received RSSI and size
  rssi=rssi;
  rxSize=size;
  // Copy the received payload to the buffer
  memcpy(buffer, payload, size );
  // Extract the receiver address from the buffer
  uint8_t receiverAddr = buffer[0];
  // Check if the received message is addressed to this device
  if(receiverAddr == myAddr){
    // If it's the first variable received
    if(!receivedFirstVar){
      // Extract the sender address from the buffer
      uint8_t SenderAddr = buffer[1];
      Serial.print("Received sender addr: ");
      Serial.println(SenderAddr);
      // Connect to the appropriate receiver
      connectToReceiver();
      // Calculate the LoRa delay and send it back to the sender
      uint32_t tdelay = calcLoraDelay();
      uint8_t delay_buff[5];
      delay_buff[0] = SenderAddr;
      delay_buff[1] = (tdelay >> 24) & 0xFF; 
      delay_buff[2] = (tdelay >> 16) & 0xFF;
      delay_buff[3] = (tdelay >> 8) & 0xFF;
      delay_buff[4] = tdelay & 0xFF;
      Radio.Send(delay_buff, 5);
      delay(200);
      Radio.IrqProcess( );
      receivedFirstVar = true;
    }
    else{
      // If the data size and received bytes are zero, extract the data size
      if(bytes_Received == 0 && image_Size ==0){
        image_Size = ((uint32_t)buffer[1] << 24) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 8) | buffer[4];
        Serial.print("Received data size: ");
        Serial.println(image_Size);

        // Forward the data size to the appropriate receiver
        if(wifiReceiver == true){
          client2.write((const uint8_t*)&image_Size, sizeof(uint32_t)); // Send the data size to WiFi receiver
        }
        else if(BleReceiver ==  true){
          pCharacteristic->setValue((uint8_t*)&image_Size, 4); // Send the data size to BLE receiver
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
          Radio.Send(buffer, 5); // Send dummy data first to synchronize the receiver
          delay(200);
          Radio.IrqProcess( );
          delay(50);

          // Pack the actual image size into the buffer
          buffer[1] = (image_Size >> 24) & 0xFF; 
          buffer[2] = (image_Size >> 16) & 0xFF;
          buffer[3] = (image_Size >> 8) & 0xFF;
          buffer[4] = image_Size & 0xFF; 
          Serial.println("sending packet to lora");         
          Radio.Send(buffer, 5); // Send the data size over LoRa
          delay(150);
          Radio.IrqProcess( );
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
          // Pack size into buffer (remaining 4 bytes)
          buffer[1] = datatype;
          
          Serial.println("sending packet to lora");
          //Radio.Send((uint8_t*)&imageSize, sizeof(imageSize));
          Radio.Send(buffer, 2);
          delay(150);        
          Radio.IrqProcess( );
        }
        receivedDatatype = true;
        return;
      }
      
      // Handle the received data chunk
      Serial.println("-------");
      delay(10);
      uint8_t var2;
      var2 = size & 0xFF;
      var2--;
      Serial.println(var2);
      Serial.write(buffer+1, var2);
      delay(10);
      Serial.println("-------");

      // Forward the received data chunk to the appropriate receiver
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
        Radio.Send(buffer, var2 +1); // Send the data chunk over LoRa	
        delay(500);       
        Radio.IrqProcess( );
      } 

      bytes_Received += var2;
      // Check if the entire image has been received
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
  // Put the LoRa radio to sleep and set the idle flag
  Radio.Sleep( );
  lora_idle = true;
}



void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud
  delay(10);

  // Set Up Wi-Fi as an Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  IPAddress IP = WiFi.softAPIP();  // Get the IP address of the Access Point
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Start the WiFi servers
  server1.begin();  // Start server1 on port 80
  server2.begin();  // Start server2 on port 8888
  delay(50);

  Mcu.begin();  // Initialize the MCU

  // Set the LoRa event callbacks
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;
  // Initialize the LoRa radio with the configured events
  Radio.Init( &RadioEvents );

  // Configure the LoRa radio settings
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 
  Serial.println("LoRa Initialized");
  delay(50);

  BLEDevice::init("");  // Initialize the BLE device

  pServer = BLEDevice::createServer();  // Create a BLE server
  pServer->setCallbacks(new MyServerCallbacks());  // Set the server callbacks

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
  // have detected a new device.  Specify that we want active scanning and start the
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
}



void loop() {
  if (receivedFirstVar) {
    if (lora_idle) {
      lora_idle = false;
      Serial.println("into RX mode");
      Radio.Rx(0);
    }
    Radio.IrqProcess();
  }
  else{
    // Check for available WiFi client on server1
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
            Radio.Send(buffer, 5);
            delay(200);
            Radio.IrqProcess( );
            delay(50);

            // Pack size into buffer (remaining 4 bytes)
            buffer[1] = (imageSize >> 24) & 0xFF; 
            buffer[2] = (imageSize >> 16) & 0xFF;
            buffer[3] = (imageSize >> 8) & 0xFF;
            buffer[4] = imageSize & 0xFF; 
            
            Serial.println("sending packet to lora");
            //Radio.Send((uint8_t*)&imageSize, sizeof(imageSize));
            Radio.Send(buffer, 5);
            delay(150);
            Radio.IrqProcess( );
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
                //Radio.Send((uint8_t*)&imageSize, sizeof(imageSize));
                Radio.Send(buffer, 2);
                delay(150);
                Radio.IrqProcess( );
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
                  Radio.Send(buffer+i, currentChunkSize+1); //send the package out	
                  delay(500);            
                  Radio.IrqProcess( );
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
    
    // Start BLE scanning
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

    // If LoRa is idle, switch to RX mode
    if (lora_idle) {
      lora_idle = false;
      Serial.println("into RX mode");
      // Put radio into RX mode infinetely.
      Radio.Rx(0);
    }
    // Process any pending LoRa interrupts
    Radio.IrqProcess();

  }  
}
