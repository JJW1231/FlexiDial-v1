#pragma once

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "time.h"
#include "math.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define POWER_PEN GPIO_NUM_21
#define BAT_PIN GPIO_NUM_10

#define MOSI_PIN GPIO_NUM_11
#define SRCLK_PIN GPIO_NUM_13
#define RCLK_PIN GPIO_NUM_12
    void power_init();
    void power_set(bool state);
    void RGB_power_set(bool state);
    bool RGB_power_get(void);
    void AUD_power_set(bool state);
    bool AUD_power_get(void);

#ifdef __cplusplus
}
#endif