#include <LoRa.h>
#include <SPI.h> // Required for communication between ESP32CAM and LoRa module
#include "Arduino.h"
#include <WiFi.h>
#include "BLEDevice.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Pin configurations for the ESP32 to set up LoRa and SPI communication.
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
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Address definition for LoRa Addressing
uint8_t destAddr = 20;
uint8_t buffer[251];  // Buffer to hold received data

// WiFi credentials
const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";
WiFiServer server1(80); // Server object for client 1
WiFiServer server2(8888); // Server object for client 2
WiFiClient client2;

// BLE Server configuration
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool is_client = false;

// The service UUID of the remote service we are interested in.(service UUID of receiver device acting as ble server)
static BLEUUID serviceUUID("4fafc202-1fb5-459e-8fcc-c5c9c331915c");
// The characteristic of the remote service we are interested in.(characteristic UUID of receiver device)
static BLEUUID    charUUID("beb5484e-36e1-4688-b7f5-ea07361b26b9");

// BLE Client configuration
BLEClient*  pClient = NULL;
BLEScan* pBLEScan = NULL;
static boolean doConnect = false;
static boolean connected = false;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;

String SUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
String CUUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

// ID of the receiver and the type of connection between gateway and receiver
String receiverID = "RX123";
String receiverConType = "BLE";


// Function to connect to BLE server(i.e receiver)
bool connectToServer() {
  // Print the address of the device we are trying to connect to
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    is_client = true;
    
    // Create a new BLE client
    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

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
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      // Stop scanning as we have found the desired device
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;

    } // Found our server
  } // onResult
};

// BLE server callback class
class MyServerCallbacks: public BLEServerCallbacks {
    // Server callback function called when a client(gateway) connects to the BLE server(receiver)
    void onConnect(BLEServer* pServer) {
      if(!is_client){
        deviceConnected = true;
        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->stop();
      }
    };

    // Server callback function called when a client(gateway) disconnects from the BLE server(receiver)
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// Defines callbacks for BLE characteristic events, particularly focusing on write operations.
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    // Callback method that is triggered when a write operation occurs on a BLE characteristic.
    void onWrite(BLECharacteristic *pCharacteristic) {
      // Retrieve the value written to the characteristic.
      std::string value = pCharacteristic->getValue();

      // Check if the value is not empty.
      if (value.length() > 0) {
        Serial.println("Received Value: " + String(value.c_str()));
        delay(500);
        //SendtoBLERecv();
        // Check if there is an active connection where the value can be forwarded.
        if(connected){
          Serial.println("is connected");
          // Check if the remote characteristic of server(receiver) can be written to.
          if (pRemoteCharacteristic->canWrite()){
            // Write the received value to the remote characteristic.
            pRemoteCharacteristic->writeValue(value, false);
            Serial.println("Forwarded Value to the receiver");
          }
        }
      }
    }
};



// Sends a response over LoRa with the connection details for the receiver,
// depending on the type of connection expected by the receiver.
void sendResponse() {
  // Initialize an empty response string.
  String response = "";

  // Determine the response based on the type of connection the receiver expects.
  if (receiverConType == "WiFi") {
    // Format response for WiFi with SSID and password.
    response += "R:ID123,WiFi," + String(ssid) + "," + String(password);
  } else if (receiverConType == "BLE") {
    // Format response for BLE with service and characteristic UUIDs.
    response += "R:ID123,BLE," + String(SUUID) + "," + String(CUUID);
  } else {
    // Format response for LoRa (no additional data needed).
    response += "R:ID123,LoRa,,";
  }

  // Log the sending action.
  Serial.println("sending packet to lora");

  // Start a LoRa packet.
  if (LoRa.beginPacket()) {
    LoRa.write((uint8_t*)response.c_str(), response.length()); // Send response over lora
    LoRa.endPacket();
  }
  // Short delay to ensure packet sends before continuing.
  delay(500);

  // Additional actions for BLE-type responses.
  if (receiverConType == "BLE") {
    // Longer delay for BLE-specific processing.
    delay(3000);
    // Log the connection status.
    Serial.println(deviceConnected);

    // If not connected, start advertising over BLE.
    if (!deviceConnected) {
      BLEAdvertising *pAdvertising = pServer->getAdvertising();
      Serial.println("Start advertising");
      pAdvertising->start();
    }
  }
}


// Handles incoming packets received over LoRa.
void LoRaReceived() {
  // Variable to track the size of the received packet.
  int size = 0;

  // Read all available data from LoRa into a buffer.
  while (LoRa.available()) {
    buffer[size + 1] = LoRa.read();
    size++;
  }

  // Convert the buffer to a String for easier manipulation.
  String incoming = "";
  for (int i = 1; i <= size; i++) {
    incoming += (char)buffer[i];
  }

  // Check if the packet contains a discovery request.
  if (incoming.startsWith("D:")) {
    // Extract the receiver ID from the packet.
    String received_id = incoming.substring(2, size);

    // If the received ID matches the expected receiver ID, send a response.
    if (received_id == receiverID) {
      sendResponse();
    }
  } else {
    // For other packets, log the received data.
    Serial.print("Received from lora: ");
    Serial.println(incoming);

    // Prepare to forward the received packet.
    buffer[0] = destAddr;
    delay(50);
    Serial.println("sending packet to lora");

    // Send the packet.
    if (LoRa.beginPacket()) {
      LoRa.write(buffer, size + 1);
      LoRa.endPacket();
    }
    delay(500);
  }
}



void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate.
  delay(10);

  // Set up WiFi access point with specified SSID and password
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  IPAddress IP = WiFi.softAPIP();  // Get and print the IP address of the soft access point
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Start WiFi servers on specified ports.
  server1.begin();
  server2.begin();
  delay(50);

  // Initialize LoRa with specified pins and settings.
  LoRa.setPins(SS, RST, DIO0);
  SPI.begin(14,13,2,12); 
  SPI.setFrequency(250000);

  while (!LoRa.begin(Frequency)) {
      Serial.println("Error initializing LoRa. Retrying...");
      delay(1000);
  }
  // Set LoRa communication parameters.
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  Serial.println("LoRa Initialized");
  delay(50);

  // Initialize BLE with a custom name.
  BLEDevice::init("H");

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

  // Retrieve the advertising object to configure BLE advertising parameters.
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  
  // Get the BLE scanning object to configure device scanning settings.
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  // Start scanning for BLE devices
  BLEDevice::getScan()->start(5);
  // if found the desired receiver
  if (doConnect == true) {
    // connect to the receiver
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Receiver");
    } else {
      Serial.println("We have failed to connect to the receiver");
    }
    doConnect = false;
  }
}


void loop() {
  // Check for incoming clients(sender) on the first WiFi server.
  WiFiClient client1 = server1.available();

  // Check if a client has connected to the first WiFi server.
  if (client1) {
    // Log the connection of a new WiFi sender.
    Serial.println("WiFi Sender connected");

    // Check for availability of client(receiver) on the second WiFi server and prepare to relay messages.
    client2 = server2.available();

    // Keep the connection open as long as the client is connected.
    while (client1.connected()) {
      // Check if there is data available from the client.
      if (client1.available()) {
        // Read the incoming data from the client until a newline is encountered.
        String incoming = client1.readStringUntil('\n');

        // Send the incoming data to the second WiFi client/server.
        client2.println(incoming);
        delay(10);

        // Exit the loop after sending data to handle next connection.
        break;
      }
    }

    // Disconnect the client after handling the data transfer.
    client1.stop();
    Serial.println("WiFi Sender disconnected");
  }

  // Check if there is a LoRa packet to process.
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Handle the incoming LoRa packet.
    LoRaReceived();
  }
}