#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "foc_knob.h"
#include "foc_knob_default.h"

#define PHASE_U_GPIO GPIO_NUM_2
#define PHASE_V_GPIO GPIO_NUM_1
#define PHASE_W_GPIO GPIO_NUM_3
#define MOTOR_PP 7
#define MT6701_SPI_HOST SPI3_HOST
#define MT6701_SPI_SCLK_GPIO GPIO_NUM_40
#define MT6701_SPI_MISO_GPIO GPIO_NUM_41
#define MT6701_SPI_MOSI_GPIO GPIO_NUM_NC
#define MT6701_SPI_CS_GPIO GPIO_NUM_39
#define USING_MCPWM 0

#if !USING_MCPWM
#define LEDC_CHAN_0 1
#define LEDC_CHAN_1 2
#define LEDC_CHAN_2 3
#endif
    float get_motor_shaft_velocity(void);
    float get_motor_shaft_angle(void);
    void foc_knob_set_param(uint8_t mode);
    void foc_knob_user_set_param(foc_knob_param_t param);
    void foc_init(void);
    int32_t get_motor_position();  // 获取当前电机位置
    void set_lock_flag(bool flag); // 设置休眠标志
#ifdef __cplusplus
}
#endif