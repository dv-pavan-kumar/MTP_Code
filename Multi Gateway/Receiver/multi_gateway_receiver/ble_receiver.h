#ifndef ble_receiver_h
#define ble_receiver_h

#include "common.h"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
};

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic);
};

void ble_setup();
void ble_loop();

#endif