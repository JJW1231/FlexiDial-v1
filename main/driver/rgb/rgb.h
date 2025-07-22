#pragma once

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define LED_STRIP_BLINK_GPIO GPIO_NUM_0
#define LED_STRIP_LED_NUMBERS 15
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)
#define EXAMPLE_CHASE_SPEED_MS 20
#define MAX_Brightness 55
    void rgb_init();
    void rgb_deinit();
#ifdef __cplusplus
}
#endif