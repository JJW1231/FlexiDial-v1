#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "iot_button.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define SW_BUTTON GPIO_NUM_38

    void button_init();

#ifdef __cplusplus
}
#endif