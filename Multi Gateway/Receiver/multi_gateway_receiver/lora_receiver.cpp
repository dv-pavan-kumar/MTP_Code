#include "lora_receiver.h"

void lora_setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);
  //Serial.println("[setup] Activate LoRa Sender");
  LoRa.setPins(SS, RST, DIO0);
  SPI.begin(14,13,2,12); 
  SPI.setFrequency(250000);
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

void lora_loop() {

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