#pragma once

#include "driver/gpio.h"

class Power
{
public:
    Power(gpio_num_t power_gpio, gpio_num_t mosi_gpio, gpio_num_t sck_gpio, gpio_num_t clk_gpio);
    // ~Power();
    void power_set(bool state);
    void subjoin_power_set(bool state);
    void RGB_power_set(bool state);
    bool RGB_power_get(void);
    void AUD_power_set(bool state);
    bool AUD_power_get(void);

private:
    gpio_num_t power_gpio_;
    gpio_num_t mosi_gpio_;
    gpio_num_t sck_gpio_;
    gpio_num_t clk_gpio_;
    uint8_t power_data;
    void shift_out(uint8_t data);
};