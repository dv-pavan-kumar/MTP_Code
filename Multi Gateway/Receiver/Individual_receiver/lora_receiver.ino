#include <SPI.h>
#include <LoRa.h>

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


uint8_t buffer[250];
uint32_t imageSize = 0;
uint32_t receivedBytes = 0;
uint8_t myAddr = 20;
bool receivedDatatype = false;
uint8_t datatype;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);
  //Serial.println("[setup] Activate LoRa Sender");
  while (!LoRa.begin(Frequency)) {
    //Serial.println("Error initializing LoRa. Retrying...");
    delay(1000);
  }
  //Serial.print("[setup] LoRa Frequency: ");
  //Serial.println(Frequency);

  //LoRa.begin(Frequency);
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();

  //Serial.println("[setup] LoRa initialisation complete");

}

void loop() {

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    //Serial.print("Received packet size:");
    //Serial.println(packetSize);
    uint8_t receiverAddr = LoRa.read();
    if(receiverAddr == myAddr){
      String incoming = "";
      while (LoRa.available()) {
        incoming += (char)LoRa.read(); // Read and discard the rest of the packet
      }
      Serial.println(incoming);
      

    }
    else{
      while (LoRa.available()) {
        LoRa.read(); // Read and discard the rest of the packet
      }
    }
  }
}