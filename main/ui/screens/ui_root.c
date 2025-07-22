#include "ui.h"
#include "driver/motor/motor.h"
static lv_group_t *group;
static lv_timer_t *root_task; // 定时器
static lv_style_t font_t;
void ui_root_screen_event(uint8_t state)
{
    switch (state)
    {
    case DIAL_TURN_RIGHT:
    case DIAL_TURN_LEFT:
    case DIAL_CLICK:
        break;
    default:
        break;
    }
}
static void scr_ui_root_unloaded_cb(lv_event_t *e)
{
    if (root_task)
    {
        lv_timer_del(root_task);
        root_task = NULL;
    }
    if (group)
    {
        lv_group_del(group);
        group = NULL;
    }
}
static void scr_ui_root_loaded_cb(lv_event_t *e)
{
    ui_state.index = UI_ROOT;
    lv_indev_set_group(encoder_indev, group);
    foc_knob_set_param(MOTOR_UNBOUND_COARSE_DETENTS);
}
static void root_task_cb(lv_timer_t *tmr)
{
    char full_infos[20] = {"欢迎\n灵活旋钮"};
    lv_label_set_text(lv_obj_get_child(ui_root, 0), full_infos);
}
void ui_root_screen_init(void)
{
    if (group == NULL)
        group = lv_group_create();
    ui_root = lv_obj_create(NULL);
    lv_obj_add_event_cb(ui_root, scr_ui_root_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_root, scr_ui_root_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_obj_set_style_bg_img_src(ui_root, &ui_img_bg1_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_root, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    // lv_obj_t *asdasd = lv_label_create(ui_root);
    // lv_label_set_long_mode(asdasd, LV_LABEL_LONG_SCROLL_CIRCULAR); /*Circular scroll*/
    // char full_infos[20] = {"欢迎\n灵活旋钮"};
    // lv_label_set_text(asdasd, full_infos);
    // lv_obj_align(asdasd, LV_ALIGN_CENTER, 0, -10);
    // lv_style_init(&font_t);
    // lv_style_set_text_font(&font_t, &ui_font_root32);
    // lv_obj_add_style(asdasd, &font_t, LV_PART_MAIN | LV_STATE_DEFAULT);
    // root_task = lv_timer_create(root_task_cb, 1000, 0);
    // lv_timer_set_repeat_count(root_task, -1);
}
