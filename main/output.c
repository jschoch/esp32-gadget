#include <stdint.h>
#include <stdio.h>
#include "stdatomic.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "output.h"
//#include "ws2812.h"
//#include "ws2812_control.h"

#define TAG "OUTPUT"

// blue
#define GPIO_CLOCK 12
// orange
#define GPIO_SERIAL 13
// brown
#define GPIO_LATCH 14
#define GPIO_BIT_SEL (BIT(GPIO_CLOCK) | BIT(GPIO_SERIAL) | BIT(GPIO_LATCH))

atomic_int state;

void set_output_value(int new_state) {
  if (new_state < 0 || new_state >= 100) {
    ESP_LOGE(TAG, "Invalid state: %d, ignorring", new_state);
    return;
  }

  atomic_store_explicit(&state, new_state, memory_order_relaxed);
}

#define RED   0xFF0000
#define GREEN 0x00FF00
#define BLUE  0x0000FF

void draw(){
  /*
  struct led_state new_state;
  new_state.leds[0] = RED;
  new_state.leds[1] = GREEN;
  new_state.leds[2] = BLUE;

  ws2812_write_leds(new_state);	
  */
}

/*
//bork
void draw2(){
	rgbVal color = makeRGBVal(100, 100, 100);
	rgbVal black = makeRGBVal(0,0,0);
	rgbVal *pixels;

	const uint8_t pixel_count = 2;
  	pixels = malloc(sizeof(rgbVal) * pixel_count);
	pixels[0] = color;
	pixels[1] = color;
	ws2812_setColors(pixel_count, pixels);
	
};
*/

void output_task(void *pvParameter) {
  ESP_LOGI(TAG, "output_task started");

  gpio_config_t io_conf = {
    .pin_bit_mask = GPIO_BIT_SEL,  // bit mask of the pins that you want to set,e.g.GPIO18/19
    .mode = GPIO_MODE_OUTPUT,  // set as output mode
    .pull_up_en = 0,  // disable pull-down mode
    .pull_down_en = 1,  // enable pull-up mode
    .intr_type = GPIO_INTR_DISABLE,  // disable interrupt
  };
  gpio_config(&io_conf);
  ESP_LOGI(TAG, "GPIO is configured");

  for (;;) {
    int value = atomic_load_explicit(&state, memory_order_relaxed);
    int digit1 = value % 10;
    int digit2 = (value / 10) % 10;

    ESP_LOGI(TAG, "output: %d %d %d", value, digit1, digit2);

    gpio_set_level(GPIO_LATCH, 0);
    gpio_set_level(GPIO_CLOCK, 0);

    for (int i = 0; i < 4; ++i) {
      gpio_set_level(GPIO_SERIAL, digit1 & 1);
      gpio_set_level(GPIO_CLOCK, 1);
      digit1 >>= 1;
      gpio_set_level(GPIO_CLOCK, 0);
    }

    for (int i = 0; i < 4; ++i) {
      gpio_set_level(GPIO_SERIAL, digit2 & 1);
      gpio_set_level(GPIO_CLOCK, 1);
      digit2 >>= 1;
      gpio_set_level(GPIO_CLOCK, 0);
    }

    gpio_set_level(GPIO_LATCH, 1);

    draw();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
