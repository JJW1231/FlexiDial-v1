#include "power.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <esp_log.h>
// #include "rgb/rgb.h"
#define TAG "Power"

void Power::shift_out(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        gpio_set_level(mosi_gpio_, (data >> i) & 0x01);
        gpio_set_level(sck_gpio_, 1);
        gpio_set_level(sck_gpio_, 0);
    }
    gpio_set_level(clk_gpio_, 1);
    gpio_set_level(clk_gpio_, 0);
}

Power::Power(gpio_num_t power_gpio, gpio_num_t mosi_gpio, gpio_num_t sck_gpio, gpio_num_t clk_gpio)
    : power_gpio_(power_gpio), mosi_gpio_(mosi_gpio), sck_gpio_(sck_gpio), clk_gpio_(clk_gpio)
{
    power_data = 0b00000000;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;                                                                           // 禁用中断
    io_conf.mode = GPIO_MODE_OUTPUT;                                                                                 // 设置为输出模式
    io_conf.pin_bit_mask = (1ULL << power_gpio_) | (1ULL << mosi_gpio_) | (1ULL << sck_gpio_) | (1ULL << clk_gpio_); // 位掩码
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf); // 配置GPIO

    power_set(1);
    subjoin_power_set(1);
    RGB_power_set(1);
    AUD_power_set(1);
}

// Power::~Power() {}

void Power::power_set(bool state)
{
    gpio_set_level(power_gpio_, state);
}

void Power::subjoin_power_set(bool state)
{
    if (state)
        power_data |= 0b00001000; // 确保为 1
    else
        power_data &= 0b11110111; // 确保为 0
    shift_out(power_data);
}

void Power::RGB_power_set(bool state)
{
    if (state)
    {
        power_data &= 0b11011111;
        shift_out(power_data);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // rgb_init();
    }
    else
    {
        // rgb_deinit();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        power_data |= 0b00100000; // 确保第 5 位为 1
        shift_out(power_data);
    }
}
bool Power::RGB_power_get(void)
{
    return (power_data >> 5) & 0x01;
}

void Power::AUD_power_set(bool state)
{
    if (state)
    {
        power_data &= 0b11111110;
        shift_out(power_data);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // audio_init();
    }
    else
    {
        // audio_deinit();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        power_data |= 0b00000001;
        shift_out(power_data);
    }
}
bool Power::AUD_power_get(void)
{
    return (power_data) & 0x01;
}
