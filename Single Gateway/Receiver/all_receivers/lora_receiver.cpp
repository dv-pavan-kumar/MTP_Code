#include "lora_receiver.h"

void lora_setup(){
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


void lora_loop() {  
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        //Serial.print("Received packet size:");
        //Serial.println(packetSize);
        uint8_t receiverAddr = LoRa.read();
        if(receiverAddr == myAddr){
            if(!receivedStart){
                int byteRead = 0;
                while (LoRa.available()) { 
                    buffer[byteRead] = LoRa.read();
                    byteRead++;
                }
                if((buffer[0] == 100) && (buffer[1] == 99) && (buffer[2] == 98) && (buffer[3] == 97)){
                    receivedStart = true; 
                }     
                return;        

            }
            if (receivedBytes == 0 && imageSize ==0){
                uint8_t sizeBuffer[4];
                for(int i=0;i<4;i++){
                    sizeBuffer[i] = LoRa.read();
                }
                imageSize = ((uint32_t)sizeBuffer[0] << 24) | ((uint32_t)sizeBuffer[1] << 16) | ((uint32_t)sizeBuffer[2] << 8) | sizeBuffer[3];
                // Serial.print("Received image size: ");
                // Serial.println(imageSize);
                if(imageSize>60000){
                    imageSize = 0;
                    receivedStart = false;
                    return;
                }
                // Serial.println(imageSize);
                for (int i = 0; i < sizeof(uint32_t); i++) {
                    Serial.write((uint8_t)(imageSize >> (i * 8))); // Send each byte of imageSize
                    delay(1); // Small delay between bytes
                }
                return;
            }

            if(!receivedDatatype){
                datatype = LoRa.read();
                // Serial.print("Received data type: ");
                Serial.write(&datatype, 1);
                // Serial.println("");
                receivedDatatype = true;
                return;
            }

            int bytesRead = 0;
            while (LoRa.available()) { 
                buffer[bytesRead] = LoRa.read();
                bytesRead++;
            }
            
            receivedBytes += bytesRead;
            // Check if all image data has been received
            if (receivedBytes >= imageSize) {
                // Image transmission complete
                bytesRead = bytesRead - (receivedBytes - imageSize);
                receivedBytes = 0;
                imageSize = 0;
                receivedDatatype = false;
                receivedStart = false;
                
            }
            delay(10);
            // Serial.println(bytesRead);
            Serial.write(buffer, bytesRead);
            delay(20);

        }
        else{
            while (LoRa.available()) {
                LoRa.read(); // Read and discard the rest of the packet
            }
        }
    }
}