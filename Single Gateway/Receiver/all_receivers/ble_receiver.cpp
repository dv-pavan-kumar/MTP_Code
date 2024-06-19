#include "ble_receiver.h"

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    //Serial.print("Received image size: ");
    if(imageSize ==0 && bytesReceived == 0){
      imageSize = *((uint32_t*)pData);
      for (int i = 0; i < sizeof(uint32_t); i++) {
        Serial.write((uint8_t)(imageSize >> (i * 8))); // Send each byte of imageSize
        delay(1); // Small delay between bytes
      }
      bytesReceived = 0;
      return;
    }

    if(!receivedDatatype){
      datatype = *pData;
      Serial.write(&datatype, 1);
      receivedDatatype = true;
      return;
    }

    memcpy(buffer, pData, length);
    bytesReceived += length;
    Serial.write(buffer, length);
    delay(100);

    if (bytesReceived == imageSize) {
      // Reset variables for the next image
      imageSize = 0;
      bytesReceived = 0;
      receivedDatatype = false;
      pClient->disconnect();
    }

}

void MyClientCallback::onConnect(BLEClient* pclient) {

}

void MyClientCallback::onDisconnect(BLEClient* pclient) {
    connected = false;
}

bool connectToServer() {
    
    pClient  = BLEDevice::createClient();

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      pClient->disconnect();
      return false;
    }


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      pClient->disconnect();
      return false;
    }

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
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
}


void ble_setup() {
  Serial.begin(115200);
  delay(10);

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  //pBLEScan->start(5, false); 
} // End of setup.


// This is the Arduino main loop function.
void ble_loop() {

  if(!connected && doScan){
    doScan = false;
    //Serial.println("Start Scanning.");
    BLEDevice::getScan()->start(0);
  }
  if (doConnect == true) {
    connectToServer();
    doConnect = false;
  }
} // End of loop
