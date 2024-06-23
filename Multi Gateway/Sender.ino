#include <LoRa.h>
#include <SPI.h> // Required for communication between ESP32CAM and LoRa module
#include <WiFi.h>
#include "BLEDevice.h"

// Pin configurations for the ESP32 to set up LoRa and SPI communication.
#define SS 12
#define RST 15
#define DIO0 16
#define SCK 14
#define MISO 13
#define MOSI 2

// LoRa communication parameters.
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


String receiverID = "RX123";   // ID of the receiver ESP32
String bestRelayID;            // ID of the best relay node
String bestRelayType;          // Connection type of the best relay node
String bestRelaySSID;          // SSID of best gateway for WiFi connection
String bestRelayPassword;      // Password of best gateway for WiFi connection
String bestRelayServiceUUID;   // Service UUID of best gateway for BLE connection
String bestRelayCharUUID;      // Characteristic UUID of best gateway for BLE connection
bool canSend = false;          // Flag to check if sending is possible

// priorityValue of lora -> 1
// priorityValue of ble -> 2
// priorityValue of wifi -> 3
uint8_t priorityValue = 0;     // Priority value to select the best relay

// WiFi client for sending data over WiFi.
WiFiClient client;

// BLE variables to manage BLE device scanning and connection.
BLEUUID serviceUUID;
BLEUUID    charUUID;

BLEClient*  pClient = NULL;
BLEScan* pBLEScan = NULL;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;


// Callback class for BLE client to handle connection and disconnection events.
class MyClientCallback : public BLEClientCallbacks {
  // Called when the BLE client is connected to a server(gateway)
  void onConnect(BLEClient* pclient) {
  }

  // Called when the BLE client is disconnected from a server(gateway)
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

// Function to connect to the BLE server.
bool connectToServer() {
  // Print the address of the device we are trying to connect to
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
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
    
    // Indicate that the connection is established
    connected = true;
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
    Serial.println(advertisedDevice.isAdvertisingService(serviceUUID));

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      // Stop scanning as we have found the desired device
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;   // Set flag to initiate connection

    } // Found our server
  } // onResult
};

 
// Function to handle received lora responses from the gateways
// The function parses the message and extracts relay information of the best gateway such as ID, type, SSID, and password.
void processResponse(String message) {
  // Assuming the message format is "RESPONSE:id,type,SSID,password"
  // Parse the message by locating the positions of commas, which separate the data fields.
  int firstCommaIndex = message.indexOf(',');
  int secondCommaIndex = message.indexOf(',', firstCommaIndex + 1);
  int thirdCommaIndex = message.indexOf(',', secondCommaIndex + 1);

  // Extracting parts of the message to retrieve relay node details.
  // The substring starts after "RESPONSE:" which is why '2' is used to skip "R:".
  String id = message.substring(2, firstCommaIndex);  // ID of the relay node
  String type = message.substring(firstCommaIndex + 1, secondCommaIndex);  // Type of connection (WiFi, BLE, LoRa)
  String SSID = message.substring(secondCommaIndex + 1, thirdCommaIndex);  // SSID for WiFi or service UUID for BLE
  String password = message.substring(thirdCommaIndex + 1);   // Password for WiFi or characteristic UUID for BLE

   // Assign the received values to global variables based on the connection type and current priority.
   // if we get a higher priority connection than the existing one, only then update the credentials
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

  // Set flag to true to indicate that a suitable relay has been found and is ready for data transmission.
  canSend = true;
}


// Attempts to send data via WiFi to a designated receiver.
// This function initiates a WiFi connection using the credentials of the best relay node determined by `processResponse`.
// It then attempts to connect to a specific IP address and port, sends a message, and disconnects.
void sendViaWiFi(){
  // Start WiFi connection using the SSID and password of the selected relay node.
  WiFi.begin(bestRelaySSID.c_str(), bestRelayPassword.c_str());
  // Wait until the WiFi connection is established.
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Attempt to connect to the server at the specified IP address and port.
  if (client.connect("192.168.4.1", 80)) {
    delay(100);
    client.println("Hello from Sender!");  // Send a text message to the server.
    delay(50);
    client.stop();  // Close the client connection.
  } else {
    Serial.println("Connection to relay via WiFi failed!");
  }

  // Disconnect from the WiFi network.
  WiFi.disconnect();
  Serial.println("Disconnected from Relay AP");
}


// This function first scans for BLE devices and tries to connect to a BLE server based on previous discovery.
// Once connected, it sends a message via BLE and then disconnects.
void sendViaBLE(){
  // Start BLE scanning for server(ble sender)
  BLEDevice::getScan()->start(6);
  // if found the desired gateway
  if (doConnect == true) {
    // connect to the server(gateway)
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Gateway");
      // Check if the characteristic of the remote BLE server is writable
      if(pRemoteCharacteristic->canWrite()) {
        // Send a message to the characteristic of the remote server.
        pRemoteCharacteristic->writeValue("Hello from Sender!", false);
      }
      delay(50);
      // Disconnect from the BLE server.
      pClient->disconnect();
      delay(1000);
    } else {
      Serial.println("We have failed to connect to the server");
    }
    // Reset the connection flag to false
    doConnect = false;
  }

}

// Send text message to gateway via LoRa
void sendViaLoRa(){
  LoRa.beginPacket();
  LoRa.print("Hello from Sender!");
  LoRa.endPacket();
  delay(1000);
  Serial.println("Sent via LoRa");
}






void setup() {
  // Start serial communication at 115200 baud rate.
  Serial.begin(115200);
  delay(10);

  // Set up LoRa communication pins.
  LoRa.setPins(SS, RST, DIO0);
  SPI.begin(14,13,2,12); 
  SPI.setFrequency(250000);

  while (!LoRa.begin(Frequency)) {
      Serial.println("Error initializing LoRa. Retrying...");
      delay(1000);
  }
  // Configure LoRa parameters for communication.
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  Serial.println("Lora Initialised");
  delay(50);

  // Initialize the BLE device
  BLEDevice::init("");
  // Set up BLE scanning with specific parameters.
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

}

void loop() {
  // Send a discovery packet via LoRa.
  LoRa.beginPacket();
  LoRa.print("D:" + receiverID);  // Include the receiver ID in the packet.
  LoRa.endPacket();
  Serial.println("Discovery request sent");

  // Wait for responses
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) { // Wait for 10 seconds to collect responses
    int packetSize = LoRa.parsePacket(); // Check for incoming packets.
    if (packetSize) { // If a packet is received.
      String incoming = "";
      while (LoRa.available()) {
        incoming += (char)LoRa.read(); // Read the packet data.
      }

      // Check if the incoming message is a response.
      if (incoming.startsWith("R:")) {
        processResponse(incoming); // Process the response
      }
    }
  }

  // After collecting responses, send data via the best available relay.
  if(canSend){
    if (bestRelayType == "WiFi") {
      sendViaWiFi(); // Send data via WiFi if it's the best relay type.
    }
    else if(bestRelayType == "BLE"){
      sendViaBLE();  // Send data via BLE if it's the best relay type.
    }
    else{
      sendViaLoRa();  // Send data via LoRa if it's the best relay type
    }
    // Reset priority after sending.
    priorityValue = 0;
    canSend = false;

  }
}
