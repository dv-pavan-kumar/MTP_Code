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

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic1 = NULL;
bool deviceConnected = false;
uint32_t timeDelay = 0;
bool messageSendingEnabled = false;

String message = "ffd8ffe000104a46494600010101004800480000ffdb004300281c1e231e19282321232d2b28303c64413c37373c7b585d4964918099968f808c8aa0b4e6c3a0aadaad8a8cc8ffcbdaeef5ffffff9bc1fffffffaffe6fdfff8ffdb0043012b2d2d3c353c76414176f8a58ca5f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8f8ffc0001108014001e003012200021101031101ffc4001a000003010101010000000000000000000000010203040506ffc4002d1000020201040104020104030101000000000102112103123141510413226132718105234291143352a1c1ffc400160101010100000000000000000000000000000102ffc400161101010100000000000000000000000000001101ffda000c03010002110311003f00f51c530f6e3e0c56f83e6d0e1acee9ac046bedc7c06c8f80534fb294932836a0da86004ed41b5140413b506d4501466e0bc09e947c1a00197b51f00f4a3e0d44c0e4d5d35e0b868c6b82a4ae66a9520335a515d0fdb5e0b0033f6e3e07b1782c008d883622c4046d43da50054ed42da50daaa088a0a28402a0a18013414300150a8a00268286040a85450805414302aa40644e6a2bec8878b0a0d28375297237f93414a85430015050c00542a28404d0514004505140066e09f285edaf0680066f4e3e097a5178a350039dfa742ff8eba3a44076d216d8f8c94a912e37c0184ed3237cd3b4ed23a9413e4c670daf8e482b4bd4eec4957866ca69e3b38b635276b1e4a727b71ca2a3b6ec679cbd638ba92c9d5a7af09f1228dc42bb1d80009ba56c87272e3802dc9221ea6692150895436c7b9d0ae84f8143f71d95bcc45904742927d8ce58c9df26b19946a21292630800000436f810000862000000010c44000000080000431000018eaeb287c5658556a6a6d54b9274e172dd2cbf0441a734eceb8a495be406928c4c7fc99ab78314ee4c0a0000100c40021880000004000002188018008000040775205904ed0f001c704c96e45524269f280ce5a6ebca3270f08e8dcc55d81e57abd2929377839e3a928b54cf4bd6c7e2f3fc1e63480eef4feba9a8cde0ed8eb465c34cf092a67a1fd3a0e52949dd2e00ee772045a414412f340cada4c828a5e4863698f694409a65d09819b41c2b29a2580e32cfd9b4657fb39d7052935411d004c65ba3632a1881b1580c400c0004160000220602b00000000105996aeaec4bcb016aeb2861727376dbe58b326db1d3b0ad74239afb3b924b07368429afb3a24b204d7266f12a46927b518b95480b012c8c00400002000000001000008000004310008000ed4d15488487d9068a380a27ae4137e6c0251544a693a6529794274d9473fabd253d372ed1e4ca3d1ee4b3068f235a2937920c29a783d6fe9d9f4cbcde4f2acf5ffa6d3f4df76074ed0a2e80a21a1517420276d724b7e0a7f64f2c2a681c4ae06066e24491b5641c40e6ab0768d671f067f4c04a7edcedf0cd94e2f8660d6e8b4616d46d7411ded8b75ac1c0bd549603fe5679a2a3b94fc8ece097a869d594bd4d45ae483b6c5671c3d4daa93fd30ff9293dade42bb37601bc2a3923ea55f3c82f509369bec23aaf160a472cb5f6b69bc3ca2e1aa9a6ef006ee5f225cf2e8c5ea273e47092dcefa0ab94f6a76724e4e72b674496f95f467edd26fa02628baca1c61f1b08aca0ae8d04d49596de4ce2f6cebe8b5c8412ca31945ff06f2ca21b5b1a7c81317828ce0b345a0000000100000818000008218802c2900000086203b2ca54d1128fd502abc9156e2eb024f2352f03dcbb083f925af039462f2991b5d5a014a4d7479feae34eeb93d14d3fd98fa9d38ca16d640f1ddd9eb7f4a7fda923cc9429b3bffa5eedf25d50c1e9a062194264b6537833b0093a64b60c9e58552c9490a28be006903a16586d088924cca70ac9bb8912881ceb1233db53941f0cd6527c5113af7632be7903975f45c7557fe59ccff33d5d483945aed1c9aba5b69aef20639491152e99d3b71464b9c9065f246b28a9c1792b6ad8eb94528a6ad20ac5c1c436bb5f66b2cc7eec2296cfb4c512e2e4a3f45e96117a7cd09c76cb1e4227cc9150794fc9515764d5455790378da6ca9a6f0108fc5365c7e49c979a03350757d025d9b460dcaab05ca29a492e0a3193a945f92926f2b8235156a25e0b4e980de44ee8a6af227f8818b4d64a8c93c14d6e8b30fc65680dc40b800062189b000000010000310c00400000200223d26932762f036c96d9152e0d662c94f3f243949f81294a5828bda9ac31538f4195d07b8b890052646a414a3468dc64b9217c70f280f3fd4e96cfb5d19fa5d67a3ae9f4f0cf475f4a3a916e2f3e0f2a71719535415eec649ab4071fa2d5dda49768ea522a062781dda2640431c50bb2d608aaba0e5824524502453690899308adc999c96192d8b7010d5996a25be26cf0ac8715279e42b7c269f94736b42dfe8def15e08e488e6d48a5a4a5461b6d33af5d7c2ba30846d9061a799532e2f639046349bfb14bf16c29c3e53cf92b516d9ba1452c3f244db7a8c0d20d4752fa68ae246106dcabc1acdb4d3411a24f763b4447f16becad29252b6439536d7928df77c52f05e94bfb34bc9c919d4ff65e8ea6d5407769ea5c5b45a6945f9e8c7412f6827369aae8a09c38f21548b8fc92f24be40d21981324a8519d2da3e80c64e9fd1308ee7b8bd4fc78c8a0b003018980985000000080000000400c000040000007a0ff0044b0725d488b7fb32a76d0b04dbe196a36034e496096ef12894d38fd90e7d38bb02251a7f16cb8b7d8269feca4ddd328aa8c91cbeafd35adc8eadbe0749aa60797e9a7ed6ad4b867a4964e2f55a5edcad70757a79fb9a2bb6b0305f02653a336eca89cd951648e3c915b22a85128a84c968b64be00c658c0a3c953568ce32f8b4f94451ab7175d13aaee71dbda34d4f9e8eeed1cd1bdf1cfe206fa6f73656da9212ffb64bca295e2c232d554d59cea2a136afb3a751db329abc810a2ada309d462cdf525b64999c9a926ab922a347e4bf9c035fdd63d15b6716cd1afeedf498186dab922a76e2d84dd6a4a3d32e2adfd504637b781dfc59acf4d29a6b8329e217e58528e3517843dd96d193fc9bbe4b8ac67a2a3d0d195d4453b94e93eccbd26a7ce727d236d27727fec0dbf1544590e5294a914a0df2c01bf92656e44ed0697051136db6c70cc426b090d61000bb06c0218864b0a003a00000100c400002188004311076ed4f363a4bfc502a1d2e88a96a3e0aa5586524271fa025bf285ce474fa25a7d803591a74b28552fe0a4ac0a8c93e062515fc94065ab1df0716ace4d3dda13aba4ceefd1328292a92021cd3245283d39574359284ca8f21580e00d62cadc64995928bbb42b1116d004978327f93c7eca94974c956e79e08145d466ba1c209db1ed6d515a69a54c808c7e69f7545b5690eab246ecc9782a33794fca33d474b8e8d751a4ad2e4cd2dcb206135ba29af24c63f2a36d28b51945f9c0e51ad4ba0a892a84bfd99a962d9acfb5d511283d880c7515cafa2f465fdbb7ca09afed2f366595c046ce76a2feccb59f0979b14b0813df406524fa346fff00a56cb85ae449795902b4decd296d7cf27568c94126b38395aaab5c9519d351bec0edd3ab6d8e4ef86649dc135c149aaa02e2aa37628ab764fcbf81ac705049db10300000000620060002008043105000000160200b00003d04fc89ce3e478138a7d195253f00e74f22a8f192947f9017b885ee45f91ca0bc12ad01569f191ae7c0a3fc1aaa6809ab0c8daf034542c12cba26499029437429982752dace98b235346327b9624510c86376b0c96555e9f26b7661a66bf8ac014f8339b6ba2e32b1cab6e4839676e4bc33551fedc4ca32ad469fe2cde3f8a440a2bfd94d66c4d05e4a81bc5f46134e33dd78b36d46d2c70108a9469811b7fd33371cfda357bb4dbaca1aa794073c5b726fc3369470138aab5d969394620736a7e0bc8a517f048d35570bc30d5b5b6480e592dab3e44e26dad1f8a7f64a56dfd018c636da974428b7a94b18367f19bbec992a9297d05283a8afa66ba9151ac2c98c634ce9925271974d04653d34e9986a46a76b847524d2c91a995b5017094568c6298e0f34610c4b26b069203a2d25448a391940080000417600003001086002131b1580860958c0400040806203d0db8c12d48ba62645428b5c9542717e432bb02ad133dac761f16b90222cd635e4cedac22a360680c4ac69b2a25cab92653a2dd3e89daaea8a2633b29a6d09e8aff1c11ed6a7fe881ca2fb4434945b2bdbd55dd8dc24e34d144e83dd0b7e4bd4584c9d38ec545356802293c6ec8daa12c7286e488a9714d657f24a838b4d32dbb4251086";


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



void setup() {
  Serial.begin(115200);
  delay(10);

  
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
  
  if (deviceConnected) {
    Serial.println("Connected to client");

    while(deviceConnected){
      if(messageSendingEnabled){
        uint8_t* data = (uint8_t*) message.c_str();
        uint32_t length = message.length();
        Serial.print("Text size is: ");
        Serial.println(length);
        
        pCharacteristic->setValue((uint8_t*)&length, 4);
        pCharacteristic->notify();
        delay(timeDelay);

        size_t bytesSent = 0;
        size_t basicchunck = 250;
        while (bytesSent < length) {
          size_t chunkSize = min(basicchunck, length - bytesSent); // Chunk size of 250 bytes
          pCharacteristic->setValue(data + bytesSent, chunkSize);
          pCharacteristic->notify();       
          delay(50);
          Serial.println("-------");
          Serial.write(data + bytesSent, chunkSize);
          Serial.println("-------");
          delay(timeDelay);
          bytesSent += chunkSize;
        }

        delay(500);

        messageSendingEnabled = false;
        break;
      }
    }

    
  }
  // disconnecting
  if (!deviceConnected) {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    Serial.println("start advertising");
    pAdvertising->start();
    delay(200); 
  }
}