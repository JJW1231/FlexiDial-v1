#include "events.h"
#include "ui.h"

QueueHandle_t Dial_Queue = NULL;
static const char *TAG = "ui/events";
void dial_event_task(void *param)
{
    uint8_t state;
    uint8_t tick_num = 0;
    lv_obj_t *aa;
    while (1)
    {
        // ESP_LOGI(TAG, "dial_event_task最小空闲堆栈空间:%lu", uxTaskGetStackHighWaterMark2(NULL));
        if (xQueueReceive(Dial_Queue, &state, portMAX_DELAY) == pdTRUE)
        {
            // if (DIAL_WAKENET_DETECTED)
            // {
            //     lv_obj_clear_flag(popup, LV_OBJ_FLAG_HIDDEN);
            // }
            // else
            // {
            //     lv_obj_add_flag(popup, LV_OBJ_FLAG_HIDDEN);
            // }

            // ESP_LOGI(TAG, "mune:%d,state:%d", ui_state.index, state);
            switch (ui_state.index)
            {
            case UI_MENU_INTERFACE:
                ui_main_screen_event(state);
                break;
            case UI_POWER_ABOUT:
                ui_about_screen_event(state);
                break;
            case UI_SURFACE_KNOB:
                ui_knob_screen_event(state);
                break;
            case UI_LAMP:
                ui_lamp_screen_event(state);
                break;
            case UI_SET:
                ui_set_screen_event(state);
                break;
            case UI_SLEEP:
                // ui_sleep_screen_event(state);
                break;
            default:
                break;
            }
        }
    }
}
void dial_event_init()
{
    Dial_Queue = xQueueCreate(10, sizeof(uint8_t)); /* 消息队列的长度 * 消息的大小 */
    if (Dial_Queue != NULL)                         // 判断队列是否创建成功
        xTaskCreatePinnedToCore(dial_event_task, "dial_event_task", 4096, NULL, 20, NULL, 0);
}