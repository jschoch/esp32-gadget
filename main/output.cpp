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
#include "FastLED.h"

#define TAG "OUTPUT"

extern CRGB leds;

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

void draw(){
	leds[0] = CRGB::White; FastLED.show(); delay(30);
	leds[0] = CRGB::Black; FastLED.show(); delay(30);
}


void output_task(void *pvParameter) {
  ESP_LOGI(TAG, "output_task started");

  ESP_LOGI(TAG, "GPIO is configured");

  for (;;) {
    /*
    int value = atomic_load_explicit(&state, memory_order_relaxed);
    int digit1 = value % 10;
    int digit2 = (value / 10) % 10;
   */

    //ESP_LOGI(TAG, "output: %d %d %d", value, digit1, digit2);


    draw();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

}
