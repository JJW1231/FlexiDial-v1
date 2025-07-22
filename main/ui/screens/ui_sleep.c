#include "ui.h"
#include "driver/motor/motor.h"
static lv_group_t *group;
static lv_timer_t *sleep_task; // 定时器
static lv_style_t font_t;
static lv_style_t font_t1;
void ui_sleep_screen_event(uint8_t state)
{
    switch (state)
    {
    case DIAL_TURN_RIGHT:
    case DIAL_TURN_LEFT:
    case DIAL_CLICK:
        _ui_screen_change(&ui_sleep, &ui_main, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_main_screen_init);
        break;
    default:
        break;
    }
}
static void scr_sleep_unloaded_cb(lv_event_t *e)
{
    if (sleep_task)
    {
        lv_timer_del(sleep_task);
        sleep_task = NULL;
    }
    if (group)
    {
        lv_group_del(group);
        group = NULL;
    }
}
static void scr_sleep_loaded_cb(lv_event_t *e)
{
    ui_state.index = UI_SLEEP;
    lv_indev_set_group(encoder_indev, group);
    foc_knob_set_param(MOTOR_UNBOUND_COARSE_DETENTS);
}
static void sleep_task_cb(lv_timer_t *tmr)
{
    char full_infos[10] = {"12:34"};
    char *week = (char *)malloc(30);
    // https_get_time(full_infos, week);
    lv_label_set_text(lv_obj_get_child(ui_sleep, 0), full_infos);
    lv_label_set_text(lv_obj_get_child(ui_sleep, 1), week);
    free(week);
}
void ui_sleep_screen_init(void)
{
    if (group == NULL)
        group = lv_group_create();
    ui_sleep = lv_obj_create(NULL);
    lv_obj_add_event_cb(ui_sleep, scr_sleep_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_sleep, scr_sleep_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_obj_set_style_bg_img_src(ui_sleep, &ui_img_bg1_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_sleep, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    lv_obj_t *label2 = lv_label_create(ui_sleep);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR); /*Circular scroll*/
    char full_infos[10] = {"12:34"};
    char *weekss = (char *)malloc(30);
    // https_get_time(full_infos, weekss);
    lv_label_set_text(label2, full_infos);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *label3 = lv_label_create(ui_sleep);
    lv_label_set_long_mode(label3, LV_LABEL_LONG_SCROLL_CIRCULAR); /*Circular scroll*/
    lv_label_set_text(label3, weekss);
    lv_obj_align(label3, LV_ALIGN_CENTER, 0, 34);
    free(weekss);
    lv_style_init(&font_t);
    lv_style_set_text_font(&font_t, &ui_font_time_num64);
    lv_obj_add_style(label2, &font_t, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_style_init(&font_t1);
    lv_style_set_text_font(&font_t1, &ui_font_day24);
    lv_style_set_text_color(&font_t1, lv_color_hex(0x00FFCC)); // 设置字体颜色为绿色
    lv_obj_add_style(label3, &font_t1, LV_PART_MAIN | LV_STATE_DEFAULT);
    sleep_task = lv_timer_create(sleep_task_cb, 1000, 0);
    lv_timer_set_repeat_count(sleep_task, -1);
}
