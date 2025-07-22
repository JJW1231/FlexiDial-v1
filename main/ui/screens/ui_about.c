#include "ui.h"
#include "driver/power/power.h"
#define ITEM_HEIGHT_MIN 90
#define ITEM_PAD 10
static const char *TAG = "about";
static lv_group_t *group;
static lv_timer_t *about_task; // 定时器
static int icon_index = 0;
static const foc_knob_param_t press_foc_knob_param = {0, 0, 20 * PI / 180, 1, 1, 1.1, ""}; // 按下旋转的力度
static struct                                                                              // 样式结构体，用于各种UI元素
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
static lv_obj_t *focused_obj; // 当前焦点对象
static void about_task_cb(lv_timer_t *tmr)
{
    char *sta_info_buff = (char *)malloc(60);
    switch ((uint8_t)lv_obj_get_user_data(focused_obj))
    {
    case 0:
        // sys_get_mac(sta_info_buff);
        break;
    case 1:
        // wifi_get_sta_info(sta_info_buff);
        break;
    case 2:
        // wifi_get_ap_info(sta_info_buff);
        break;
    case 3:
        // sys_get_mac(sta_info_buff);
        break;
    default:
        break;
    }
    lv_label_set_text(lv_obj_get_child(focused_obj, 1), sta_info_buff);
    free(sta_info_buff);
}
static void onFocus(lv_group_t *g)
{
    lvgl_port_lock(0);
    focused_obj = lv_group_get_focused(g);
    uint32_t current_btn_index = lv_obj_get_index(focused_obj);
    lv_obj_t *container = lv_obj_get_parent(focused_obj);
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
    lv_style_set_border_color(&style.icon_focus, lv_color_hex(0xff0000));

    lv_style_init(&style.font);
    lv_style_set_text_font(&style.font, &ui_font_SmileySansOblique16);
}
// lv_obj_t *arc_bat;
static void Item_Create(lv_obj_t *container, const char *name, const void *img_src, const char *infos)
{
    lv_obj_t *box = lv_obj_create(container);
    lv_obj_set_user_data(box, (void *)icon_index);
    icon_index++;
    lv_obj_remove_style_all(box);
    lv_obj_set_align(box, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(box, &style.def, 0);
    lv_obj_add_style(box, &style.focus, LV_STATE_FOCUSED);

    lv_obj_t *icon = lv_obj_create(box);
    lv_obj_remove_style_all(icon);
    lv_obj_set_width(icon, 70);
    lv_obj_set_align(icon, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(icon, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(icon, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_obj_set_style_pad_row(icon, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(icon, &style.icon_focus, LV_STATE_FOCUSED);

    lv_obj_t *img = lv_img_create(icon);
    lv_img_set_src(img, img_src);
    lv_obj_set_width(img, LV_SIZE_CONTENT);  /// 48
    lv_obj_set_height(img, LV_SIZE_CONTENT); /// 48

    lv_obj_t *label_name = lv_label_create(icon);
    lv_label_set_text(label_name, name);
    lv_obj_set_width(label_name, LV_SIZE_CONTENT);  /// 48
    lv_obj_set_height(label_name, LV_SIZE_CONTENT); /// 48
    lv_obj_add_style(label_name, &style.font, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label_data = lv_label_create(box);
    lv_obj_remove_style_all(label_data);
    lv_label_set_text(label_data, infos);
    lv_obj_set_width(label_data, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(label_data, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(label_data, LV_ALIGN_LEFT_MID);
    lv_obj_add_style(label_data, &style.font, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* get real max height */
    lv_obj_update_layout(label_data);
    lv_coord_t height = lv_obj_get_height(label_data);
    height = LV_MAX(height, ITEM_HEIGHT_MIN);
    lv_obj_set_height(box, height);
    lv_obj_set_height(icon, height);
}
static void Create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_width(root, 240);
    lv_obj_set_height(root, 240);
    lv_obj_set_align(root, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    icon_index = 0;
    Item_Create(root, "BLE", &ui_img_pc_png, "open ble");
    char *sta_info_buff = (char *)malloc(60);
    // wifi_get_sta_info(sta_info_buff);
    Item_Create(root, "WIFI-STA", &ui_img_customize_png, sta_info_buff);
    // wifi_get_ap_info(sta_info_buff);
    Item_Create(root, "WIFI-AP", &ui_img_customize_png, sta_info_buff);
    free(sta_info_buff);
    Item_Create(root, "POWER", &ui_img_customize_png, "FlexiDial\nv25.2.0.1\nPOWER OFF");
}
bool wifi_sta = 1;
bool wifi_ap = 0;

void ui_about_screen_event(uint8_t state)
{
    switch (state)
    {
    case DIAL_PRESS_DOWN:

        break;
    case DIAL_PRESS_UP:

        break;
    case DIAL_TURN_RIGHT:
        enc_num++;
        break;
    case DIAL_TURN_LEFT:
        enc_num--;
        break;
    case DIAL_CLICK:
        int8_t index = icon_index; // change screen1
        ESP_LOGI(TAG, "当前:%d ,状态: %d", index, state);
        switch (index)
        {
        case 0: // usb hid
            // 输出网址
            ESP_LOGI(TAG, "输出ip地址");
            break;
        case 1:
            wifi_sta = !wifi_sta;
            // wifi_init(wifi_sta + wifi_ap * 2);
            break;
        case 2:
            wifi_ap = !wifi_ap;
            // wifi_init(wifi_sta + wifi_ap * 2);
            break;
        case 3: // setting
            power_set(0);
            break;
        default:
            break;
        }
        break;
    case DIAL_DOUBLE_CLICK:
        _ui_screen_change(&ui_about, &ui_main, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_main_screen_init);
        break;
    case DIAL_LONG_PRESS_START:
        if (icon_index == 4) // 在关机界面长按设置或清零，并且重启
        {
            power_set(0);
        }
        break;
    default:
        break;
    }
}
void scr_about_unloaded_cb(lv_event_t *e)
{
    if (about_task)
        lv_timer_del(about_task);
    about_task = NULL;
    if (group)
        lv_group_del(group);
    group = NULL;
    focused_obj = NULL;
}
void scr_about_loaded_cb(lv_event_t *e)
{
    ui_state.index = UI_POWER_ABOUT;
    lv_indev_set_group(encoder_indev, group);
    foc_knob_user_set_param(press_foc_knob_param);
}

void ui_about_screen_init(void)
{
    ui_about = lv_obj_create(NULL);
    lv_obj_add_event_cb(ui_about, scr_about_loaded_cb, LV_EVENT_SCREEN_LOADED, &ui_about);
    lv_obj_add_event_cb(ui_about, scr_about_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, &ui_about);
    lv_obj_set_style_bg_img_src(ui_about, &ui_img_bg1_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_about, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    Style_Init();
    lv_obj_t *container = lv_obj_create(ui_about);
    Create(container);

    uint32_t mid_btn_index = (lv_obj_get_child_cnt(container) - 1) / 2;
    for (uint32_t i = 0; i < mid_btn_index; i++)
    {
        lv_obj_move_to_index(lv_obj_get_child(container, -1), 0);
    }
    group_init(container, icon_index);

    about_task = lv_timer_create(about_task_cb, 1000, 0);
    lv_timer_set_repeat_count(about_task, -1);
}
