#include "common.h"
#include "audio_ble_sender.h"
#include "audio_lora_sender.h"
#include "audio_wifi_sender.h"
#include "image_ble_sender.h"
#include "image_lora_sender.h"
#include "image_wifi_sender.h"
#include "text_ble_sender.h"
#include "text_lora_sender.h"
#include "text_wifi_sender.h"

uint8_t technology = 1;
uint8_t type_of_data  = 1;

void setup() {
  // Call common setup function
  setupCommon();
}

void loop() {
  if(technology == 1){
    if(type_of_data  == 1){
      image_wifi_sender_loop();
    }
    else if(type_of_data  == 2){
      audio_wifi_sender_loop();
    }
    else{
      text_wifi_sender_loop();
    }
  }
  else if(technology == 2){
    if(type_of_data  == 1){
      image_ble_sender_loop();
    }
    else if(type_of_data  == 2){
      audio_ble_sender_loop();
    }
    else{
      text_ble_sender_loop();
    }
  }
  else if(technology == 3){
    if(type_of_data  == 1){
      image_lora_sender_loop();
    }
    else if(type_of_data  == 2){
      audio_lora_sender_loop();
    }
    else{
      text_lora_sender_loop();
    }
  }
}