#include "../components/ssd1306/font8x8_basic.h"
#include "../components/ssd1306/ssd1306.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_slave.h"
#include "driver/i2c_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "hal/adc_types.h"
#include "hal/gpio_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/unistd.h>
#include <unistd.h>

SSD1306_t dev;
SSD1306_t *init_screen() {
  
  i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);

  ssd1306_init(&dev, 256, 64);

  ssd1306_clear_screen(&dev, false);
  ssd1306_contrast(&dev, 0xff);
  return &dev;
};
void screen_write_0(SSD1306_t *dev, char *text) {
  //ssd1306_clear_screen(dev, false);
  //ssd1306_contrast(dev, 0xff);
  ssd1306_display_text(dev, 2, text, strlen(text) + 1, false);
};
void screen_write(SSD1306_t *dev, char *text) {
  ssd1306_clear_screen(dev, false);
  ssd1306_contrast(dev, 0xff);
  ssd1306_display_text(dev, 4, text, strlen(text) + 1, false);
};
void screen_write_2(SSD1306_t *dev, char *text) {
  //ssd1306_clear_screen(dev, false);
  //ssd1306_contrast(dev, 0xff);
  ssd1306_display_text(dev, 5, text, strlen(text) + 1, false);
};