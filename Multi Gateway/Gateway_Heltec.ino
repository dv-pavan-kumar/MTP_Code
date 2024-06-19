#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include "BLEDevice.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


#define RF_FREQUENCY                                865000000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

bool lora_idle=true;
int16_t rssi,rxSize;

static RadioEvents_t RadioEvents;
uint8_t destAddr = 20;
uint8_t buffer[251];

const char *ssid = "HELTEC-ESP32-AP";
const char *password = "123456789";
WiFiServer server1(80); // Server object for client 1
WiFiServer server2(8888); // Server object for client 2
WiFiClient client2;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool is_client = false;

static BLEUUID serviceUUID("4fafc202-1fb5-459e-8fcc-c5c9c331915c");
static BLEUUID    charUUID("beb5484e-36e1-4688-b7f5-ea07361b26b9");

BLEClient*  pClient = NULL;
BLEScan* pBLEScan = NULL;
static boolean doConnect = false;
static boolean connected = false;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;

String SUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
String CUUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

String receiverID = "RX123";
String receiverConType = "BLE";

void OnTxDone( void )
{
  Serial.println("TX done......");
  lora_idle = true;
}

void OnTxTimeout( void )
{
  Radio.Sleep( );
  Serial.println("TX Timeout......");
  lora_idle = true;
}


bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    is_client = true;
    
    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

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
    is_client = false;
    return true;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;

    } // Found our server
  } // onResult
};


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      if(!is_client){
        deviceConnected = true;
        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->stop();
      }
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
        delay(500);
        //SendtoBLERecv();
        if(connected){
          Serial.println("is connected");
          if (pRemoteCharacteristic->canWrite()){
            pRemoteCharacteristic->writeValue(value, false);
            Serial.println("Forwarded Value to the receiver");
          }
        }
      }
    }
};

void sendResponse() {
  // Send response with connection details
  String response = "";
  if(receiverConType == "WiFi"){
    response += "R:ID123,WiFi," + String(ssid) + "," + String(password);
  }
  else if(receiverConType == "BLE"){
    response += "R:ID123,BLE," + String(SUUID) + "," + String(CUUID);
  }
  else{
    response += "R:ID123,LoRa,," ;
  }
  Serial.println("sending packet to lora");
  Radio.Send((uint8_t*)response.c_str(), response.length());
  delay(500);
  Radio.IrqProcess( );
  if(receiverConType == "BLE"){
    delay(3000);
    Serial.println(deviceConnected);
    if (!deviceConnected) {
      BLEAdvertising *pAdvertising = pServer->getAdvertising();
      Serial.println("Start advertising");
      pAdvertising->start(); 
    }
  }
}


void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
  rssi=rssi;
  rxSize=size;
  String incoming = "";
  for (int i = 0; i < size; i++) {
    incoming += (char)payload[i];
  }
  // Serial.println("received lora packet");
  // Now handle the incoming data as needed
  if (incoming.startsWith("D:")) {
    String received_id = incoming.substring(2, size);
    if(received_id == receiverID){
      sendResponse();
    }
  } else {
    Serial.print("Received from lora: ");
    Serial.println(incoming);
    memcpy(buffer+1, payload, size );
    buffer[0] = destAddr;
    delay(50);
    Serial.println("sending packet to lora");
    Radio.Send(buffer, size+1);
    delay(500);
    Radio.IrqProcess( );
  }
  
  Radio.Sleep( );
  lora_idle = true;
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

  Mcu.begin();

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;
  
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 
  Serial.println("LoRa Initialized");
  delay(50);

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

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  BLEDevice::getScan()->start(5);
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Receiver");
    } else {
      Serial.println("We have failed to connect to the receiver");
    }
    doConnect = false;
  }
}


void loop() {
  WiFiClient client1 = server1.available();
  //Serial.println("checking");
  if (client1) {
    Serial.println("Wifi Sender connected");
    client2 = server2.available();
    while (client1.connected()) {
      if (client1.available()) {
        String incoming = client1.readStringUntil('\n');
        client2.println(incoming);
        delay(10);
        break;
      }
    }
    client1.stop();
    Serial.println("Wifi Sender disconnected");
  } 

  if (lora_idle) {
    lora_idle = false;
    Radio.Rx(0);
  }
  Radio.IrqProcess();

}
