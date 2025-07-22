#include "button.h"
#include "ui/events/events.h"
extern QueueHandle_t Dial_Queue;
bool button_press_flag = 0;
static const char *TAG = "button";

static void button_event_cb(void *arg, void *data)
{
    // iot_button_print_event((button_handle_t)arg);
    // ESP_LOGI(TAG, "button event data: %d", (int)data);
    uint8_t send_state = (int)data;
    if (send_state <= DIAL_PRESS_DOWN)
        button_press_flag = send_state;
    xQueueSend(Dial_Queue, &send_state, 0);
}

void button_init()
{
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = SW_BUTTON,
            .active_level = 0,
        },
    };
    button_handle_t btn = iot_button_create(&btn_cfg);
    assert(btn);
    esp_err_t err = iot_button_register_cb(btn, BUTTON_PRESS_DOWN, button_event_cb, (void *)DIAL_PRESS_DOWN);
    err |= iot_button_register_cb(btn, BUTTON_PRESS_UP, button_event_cb, (void *)DIAL_PRESS_UP);
    err |= iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, button_event_cb, (void *)DIAL_CLICK);
    err |= iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK, button_event_cb, (void *)DIAL_DOUBLE_CLICK);
    err |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, button_event_cb, (void *)DIAL_LONG_PRESS_START);
    // err |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_HOLD, button_event_cb, (void *)DIAL_LONG_PRESS_HOLD);
    err |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_UP, button_event_cb, (void *)DIAL_LONG_PRESS_UP);
    ESP_ERROR_CHECK(err);
}