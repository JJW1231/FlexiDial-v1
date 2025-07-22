#include "motor.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_simplefoc.h"
#include "ui/events/events.h"
#define TAG "motor"
static foc_knob_handle_t foc_knob_handle = NULL;
static bool lock_flag = 0;
static bool motor_shake = false;
int8_t press_rotation = 0;
/*update motor parameters based on hardware design*/
BLDCDriver3PWM driver = BLDCDriver3PWM(PHASE_U_GPIO, PHASE_V_GPIO, PHASE_W_GPIO);
BLDCMotor motor = BLDCMotor(MOTOR_PP);
MT6701 mt6701 = MT6701(MT6701_SPI_HOST, MT6701_SPI_SCLK_GPIO, MT6701_SPI_MISO_GPIO, MT6701_SPI_MOSI_GPIO, MT6701_SPI_CS_GPIO);

extern QueueHandle_t Dial_Queue;
extern bool button_press_flag;
int32_t current_position = 0;
static time_t last_time;
int32_t get_motor_position() // 获取当前电机位置
{
    return current_position;
}
static void motor_event_cb(void *arg, void *data)
{
    // foc_knob_state_t state;
    // foc_knob_get_state(arg, &state);
    // current_position = state.position;
    // ESP_LOGI(TAG, "motor shaft angle: %ld", state.position);
    time(&last_time);
    lock_flag = 0;
    uint8_t send_state = (int)data;
    if (button_press_flag)
        send_state += 4;
    xQueueSend(Dial_Queue, &send_state, 0);
}
void motor_init(void)
{
    mt6701.init();
    motor.linkSensor(&mt6701);
    driver.voltage_power_supply = 6;
    driver.voltage_limit = 6;
#if USING_MCPWM
    driver.init(0);
#else
    driver.init({LEDC_CHAN_0, LEDC_CHAN_1, LEDC_CHAN_2});
#endif
    motor.linkDriver(&driver);
    motor.foc_modulation = SpaceVectorPWM;
    motor.controller = MotionControlType::torque;
    motor.PID_velocity.P = 4;
    motor.PID_velocity.I = 0;
    motor.PID_velocity.D = 0.04;
    motor.PID_velocity.output_ramp = 10000;
    motor.PID_velocity.limit = 10;

    motor.init();    // initialize motor
    motor.initFOC(); // align sensor and start FOC
    // printf("Motor motor.zero_electric_angle %f\n", motor.zero_electric_angle);
}
float motor_shake_func(float strength, int delay_cnt)
{
    static int time_cnt = 0;
    if (time_cnt < delay_cnt)
    {
        time_cnt++;
        return strength;
    }
    else if (time_cnt < 2 * delay_cnt)
    {
        time_cnt++;
        return -strength;
    }
    else
    {
        time_cnt = 0;
        motor_shake = false;
        return 0;
    }
}
void set_lock_flag(bool flag)
{
    lock_flag = flag;
}
static void motor_task(void *arg)
{
    static float torque = 0;

    static float last_angle = 0;
    static uint16_t lock_time = 30;
    time_t now_time;
    time(&now_time);
    last_time = now_time;
    while (1)
    {
        motor.loopFOC();
        if (motor_shake)
        {
            torque = motor_shake_func(4, 4);
        }
        else
        {
            torque = foc_knob_run(foc_knob_handle, motor.shaft_velocity, motor.shaft_angle);
        }
        motor.move(torque);

        if (!lock_flag && lock_time)
        {
            time(&now_time);
            if (difftime(now_time, last_time) > lock_time)
            {
                last_angle = get_motor_shaft_angle();
                uint8_t send_state = DIAL_SLEEP_TIME;
                lock_flag = 1;
                last_time = now_time;
                xQueueSend(Dial_Queue, &send_state, 0);
                // ESP_LOGI("DIAL", "进入休眠");
            }
        }

        vTaskDelay(2);
    }
}
float get_motor_shaft_angle(void)
{
    return motor.shaft_angle;
}
float get_motor_shaft_velocity(void)
{
    return motor.shaft_velocity;
}
static float motor_pid_cb(float P, float D, float limit, float error)
{
    motor.PID_velocity.limit = limit;
    motor.PID_velocity.P = P;
    motor.PID_velocity.D = D;
    return motor.PID_velocity(error);
}
void foc_init()
{
    motor_init();

    foc_knob_config_t cfg = {
        .param_lists = default_foc_knob_param_lst,
        .param_list_num = MOTOR_MAX_MODES,
        .max_torque_out_limit = 5,
        .max_torque = 5,
        .pid_cb = motor_pid_cb,
    };
    foc_knob_handle = foc_knob_create(&cfg);
    foc_knob_change_mode(foc_knob_handle, 0);
    foc_knob_register_cb(foc_knob_handle, FOC_KNOB_INC, motor_event_cb, (void *)DIAL_TURN_RIGHT);
    foc_knob_register_cb(foc_knob_handle, FOC_KNOB_DEC, motor_event_cb, (void *)DIAL_TURN_LEFT);
    foc_knob_register_cb(foc_knob_handle, FOC_KNOB_H_LIM, motor_event_cb, (void *)DIAL_RIGH_LIM);
    foc_knob_register_cb(foc_knob_handle, FOC_KNOB_L_LIM, motor_event_cb, (void *)DIAL_LEFT_LIM);
    xTaskCreatePinnedToCore(motor_task, "motor_task", 4096, NULL, 22, NULL, 1);
}

void foc_knob_set_param(uint8_t mode)
{
    foc_knob_change_mode(foc_knob_handle, mode);
}
void foc_knob_user_set_param(foc_knob_param_t param)
{
    foc_knob_set_param_list(foc_knob_handle, param);
}
