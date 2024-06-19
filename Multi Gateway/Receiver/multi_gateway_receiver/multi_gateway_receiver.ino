#include "common.h"
#include "lora_receiver.h"
#include "ble_receiver.h"
#include "wifi_receiver.h"


uint8_t technology = 1;

void setup() {
  if(technology  == 1){
    wifi_setup();
  }
  else if(technology  == 2){
    ble_setup();
  }
  else if(technology  == 3){
    lora_setup();
  }
}

void loop() {
  if(technology  == 1){
    wifi_loop();
  }
  else if(technology  == 2){
    ble_loop();
  }
  else if(technology  == 3){
    lora_loop();
  }
}