#include "ui.h"
#define ITEM_HEIGHT_MIN 90
#define ITEM_PAD 10
static const char *TAG = "dial";
static lv_group_t *group;
static int icon_index = 0;
static struct // 样式结构体，用于各种UI元素
{
    lv_style_t def; // 默认
    lv_style_t focus;
    lv_style_t icon_def;
    lv_style_t icon_focus;
    lv_style_t name;
    lv_style_t info;
    lv_style_t data;
    lv_style_t font;
} style;
static void onFocus(lv_group_t *g)
{
    lvgl_port_lock(0);

    lv_obj_t *icon = lv_group_get_focused(g);
    uint32_t current_btn_index = lv_obj_get_index(icon);
    lv_obj_t *container = lv_obj_get_parent(icon);
    uint32_t icon_num = lv_obj_get_child_cnt(container);
    uint32_t mid_btn_index = (icon_num - 1) / 2;
    if (current_btn_index > mid_btn_index)
    {
        icon_index++;
        if (icon_index > icon_num - 1)
            icon_index = 0;
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_btn_index - 1), LV_ANIM_OFF);
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_btn_index), LV_ANIM_ON);
        lv_obj_move_to_index(lv_obj_get_child(container, 0), -1);
    }
    else if (current_btn_index < mid_btn_index)
    {
        icon_index--;
        if (icon_index < 0)
        {
            icon_index = icon_num - 1;
        }
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_btn_index + 1), LV_ANIM_OFF);
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_btn_index), LV_ANIM_ON);
        lv_obj_move_to_index(lv_obj_get_child(container, -1), 0);
    }
    uint8_t cnt = lv_obj_get_child_cnt(container);
    for (uint8_t i = 0; i < cnt; i++)
    {
        lv_obj_t *obt = lv_obj_get_child(container, i);
        lv_obj_clear_state(lv_obj_get_child(obt, 0), LV_STATE_FOCUSED);
        // 远离中心一个图标以上的图标隐藏
        if (i + 1 < mid_btn_index)
        {
            lv_obj_add_flag(obt, LV_OBJ_FLAG_HIDDEN);
        }
        else if (i > mid_btn_index + 1)
        {
            lv_obj_add_flag(obt, LV_OBJ_FLAG_HIDDEN);
        }
        else
            lv_obj_clear_flag(obt, LV_OBJ_FLAG_HIDDEN);
        if (i == mid_btn_index)
        {
            lv_obj_add_state(lv_obj_get_child(obt, 0), LV_STATE_FOCUSED); /// States
        }
    }

    lvgl_port_unlock();
}
static void group_init(lv_obj_t *container, uint8_t id)
{
    icon_index = 0;
    group = lv_group_create();
    uint8_t cnt = lv_obj_get_child_cnt(container);
    for (uint8_t i = 0; i < cnt; i++)
    {
        lv_group_add_obj(group, lv_obj_get_child(container, i));
    }
    lv_group_set_focus_cb(group, onFocus);
    uint32_t mid_btn_index = (lv_obj_get_child_cnt(container) - 1) / 2;
    lv_group_focus_obj(lv_obj_get_child(container, mid_btn_index));
    for (uint8_t i = 0; i < id; i++)
    {
        lv_group_focus_obj(lv_obj_get_child(container, -3));
    }
}
static void Style_Init()
{
    lv_style_init(&style.def);
    lv_style_set_width(&style.def, 70);

    lv_style_init(&style.focus);
    lv_style_set_width(&style.focus, 220);
    lv_style_set_pad_column(&style.focus, ITEM_PAD);

    static const lv_style_prop_t style_prop[] =
        {
            LV_STYLE_WIDTH,
            LV_STYLE_PROP_INV};

    static lv_style_transition_dsc_t trans;
    lv_style_transition_dsc_init(
        &trans,
        style_prop,
        lv_anim_path_overshoot,
        200,
        0,
        NULL);
    lv_style_set_transition(&style.focus, &trans);
    lv_style_set_transition(&style.def, &trans);

    lv_style_init(&style.icon_focus);
    lv_style_set_border_side(&style.icon_focus, LV_BORDER_SIDE_RIGHT);
    lv_style_set_border_width(&style.icon_focus, 2);
    lv_style_set_border_color(&style.icon_focus, lv_color_hex(0x000000));

    lv_style_init(&style.font);
    lv_style_set_text_font(&style.font, &ui_font_SmileySansOblique16);
}

void ui_dial_screen_event(uint8_t state)
{
    switch (state)
    {
    case DIAL_PRESS_DOWN:
        break;
    case DIAL_PRESS_UP:
        break;
    case DIAL_TURN_RIGHT:
        break;
    case DIAL_TURN_LEFT:
        break;
    case DIAL_DOUBLE_CLICK:
        _ui_screen_change(&ui_dial,&ui_main, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_main_screen_init);
        break;
    default:
        break;
    }
}
void scr_dial_unloaded_cb(lv_event_t *e)
{
    lv_obj_t **var = lv_event_get_user_data(e);
    lv_group_del(group);
    lv_obj_del(*var);
    (*var) = NULL;
}
void scr_dial_loaded_cb(lv_event_t *e)
{
    ui_state.index = UI_SURFACE_KNOB;
    // lv_indev_set_group(encoder_indev, group);
    foc_knob_set_param(MOTOR_UNBOUND_FINE_DETENTS);
}

void ui_dial_screen_init(void)
{
    ui_dial = lv_obj_create(NULL);
    lv_obj_add_event_cb(ui_dial, scr_dial_loaded_cb, LV_EVENT_SCREEN_LOADED, &ui_dial);
    // lv_obj_add_event_cb(ui_dial, scr_dial_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, &ui_dial);
    lv_obj_set_style_bg_img_src(ui_dial, &ui_img_bg_dial_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_clear_flag(ui_dial, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    // Style_Init();
}
