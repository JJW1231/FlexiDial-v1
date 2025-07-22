#include "RGB.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/power/power.h"
static const char *TAG = "RGB";
TaskHandle_t rgb_task_handle = NULL;
led_strip_handle_t led_strip;
static uint8_t led_strip_pixels[LED_STRIP_LED_NUMBERS * 3];

led_strip_handle_t configure_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,                     
        .max_leds = LED_STRIP_LED_NUMBERS,                          
        .led_model = LED_MODEL_WS2812,                               
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, 
    };

    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT,       
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .flags = {
            .with_dma = false, 
        }
#endif
    };

    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    return led_strip;
}

/**
 * @brief 将HSV色彩转换为RGB色彩
 */
static void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void rgb_task(void *arg)
{

    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    // ESP_LOGI("rgb_task", "灯光运行");
    while (1)
    {
        // 从绿到红
        for (int i = 0; i < MAX_Brightness; i++)
        {
            red = i;
            green = MAX_Brightness - i;
            blue = 0;
            for (int j = 0; j < LED_STRIP_LED_NUMBERS; j++)
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, red, green, blue));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        // 从红到蓝
        for (int i = 0; i < MAX_Brightness; i++)
        {
            red = MAX_Brightness - i;
            green = 0;
            blue = i;
            for (int j = 0; j < LED_STRIP_LED_NUMBERS; j++)
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, red, green, blue));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        // 从蓝到绿
        for (int i = 0; i < MAX_Brightness; i++)
        {
            red = 0;
            green = i;
            blue = MAX_Brightness - i;
            for (int j = 0; j < LED_STRIP_LED_NUMBERS; j++)
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, red, green, blue));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
    }
    vTaskDelete(NULL);
}

bool rgb_fist_init = 1;
void rgb_init()
{
    if (rgb_fist_init)
    {
        led_strip = configure_led();
        for (int j = 0; j < LED_STRIP_LED_NUMBERS; j++)
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, 0, 0, 0));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        rgb_fist_init = 0;
    }

    if (rgb_task_handle == NULL)
        xTaskCreate(rgb_task, "rgb_task", 4096, NULL, 17, &rgb_task_handle);
}
void rgb_deinit()
{
    if (rgb_task_handle != NULL)
    {
        for (int j = 0; j < LED_STRIP_LED_NUMBERS; j++)
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, 0, 0, 0));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        vTaskDelete(rgb_task_handle);
        rgb_task_handle = NULL;
    }
}