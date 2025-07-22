#include "power.h"
#include "esp_log.h"
#include "driver/rgb/rgb.h"
static const char *TAG = "POWER";
uint8_t power_data = 0b00000000;
void power_set(bool state)
{
    gpio_set_level(POWER_PEN, state);
}
void shift_out(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        gpio_set_level(MOSI_PIN, (data >> i) & 0x01);
        gpio_set_level(SRCLK_PIN, 1);
        gpio_set_level(SRCLK_PIN, 0);
    }
    gpio_set_level(RCLK_PIN, 1);
    gpio_set_level(RCLK_PIN, 0);
}

void subjoin_power_set(bool state)
{
    if (state)
    {
        power_data |= 0b00001000; // 确保为 1
        shift_out(power_data);
    }
    else
    {
        power_data &= 0b11110111; // 确保为 0
        shift_out(power_data);
    }
}

void RGB_power_set(bool state)
{
    if (state)
    {
        power_data &= 0b11011111;
        shift_out(power_data);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        rgb_init();
    }
    else
    {
        rgb_deinit();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        power_data |= 0b00100000; // 确保第 5 位为 1
        shift_out(power_data);
    }
}
bool RGB_power_get(void)
{
    return (power_data >> 5) & 0x01;
}

void AUD_power_set(bool state)
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
bool AUD_power_get(void)
{
    return (power_data) & 0x01;
}

void power_init()
{
    gpio_config_t io_conf;
    // 禁用中断
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // 设置为输出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 位掩码
    io_conf.pin_bit_mask = (1ULL << POWER_PEN) | (1ULL << MOSI_PIN) | (1ULL << SRCLK_PIN) | (1ULL << RCLK_PIN);
    // 禁用拉上拉
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    // 禁用拉下拉
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // 配置GPIO
    gpio_config(&io_conf);

    power_set(1);
    subjoin_power_set(1);
    RGB_power_set(0);
    AUD_power_set(1);
}
