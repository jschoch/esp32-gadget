#include <stdint.h>
#include <stdio.h>
//#include "stdatomic.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "output.h"
//#include "ws2812.h"
//#include "ws2812_control.h"
#include "Arduino.h"
#include "leds.h"
#include "FastLED.h"
#include "freertos/queue.h"

#define TAG "OUTPUT"

extern CRGB leds;
extern xQueueHandle timer_queue;

void draw(int msg){

	if (msg == 0){
	  for(int i=0; i < NUM_LEDS; i++) {
		leds[i] = CRGB::Red;
		FastLED.show();
        	delay(50);
	  }
	}else{
	  for(int i=0; i < NUM_LEDS; i++) {
                leds[i] = CRGB::Blue;
                FastLED.show();
                delay(50);
		}

	}

	/*
	if(i == 0){	
	  leds[0] = CRGB::White; FastLED.show(); delay(30);
	  leds[0] = CRGB::Black; FastLED.show(); delay(30);
	  leds[1] = CRGB::Red; FastLED.show(); delay(30);
	} else{
	  leds[1] = CRGB::White; FastLED.show(); delay(30);
          leds[1] = CRGB::Black; FastLED.show(); delay(30);
	}
	*/
}


extern "C" {
/*
atomic_int state;

void set_output_value(int new_state) {
  if (new_state < 0 || new_state >= 100) {
    ESP_LOGE(TAG, "Invalid state: %d, ignorring", new_state);
    return;
  }

  atomic_store_explicit(&state, new_state, memory_order_relaxed);
}
*/


#define RED   0xFF0000
#define GREEN 0x00FF00
#define BLUE  0x0000FF


void output_task(void *pvParameter) {
  ESP_LOGI(TAG, "output_task started");

  ESP_LOGI(TAG, "GPIO is configured");

  int bork;

  int state = 0;

  for (;;) {

    if(xQueueReceive(timer_queue,&bork,1500/portTICK_RATE_MS)!=pdTRUE) { 
		ESP_LOGI(TAG,"no queue %d",state);
		draw(state);
	}else{
		ESP_LOGI(TAG,"got msg, %d",bork);
		draw(bork);
		if(bork == 1){
			state = 1;
		}else{
			state = 0;
		}
	}


    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

}
