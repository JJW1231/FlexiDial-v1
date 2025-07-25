#include "../ui.h"
static const uint8_t ICON_CNT = 6;
static const char *TAG = "ui_lamp";
static lv_group_t *group;
static int icon_index = 0;
static lv_timer_t *timer_task;

struct UI_SETTING_ICON_INFO
{
    const void *img_src;
    const char *name;
    const char *mid_info;
};
typedef struct
{
    uint8_t current_position;
    uint8_t icon_id;    // 图标的id
    uint16_t img_angle; // 图标的角度
    foc_knob_param_t param_list;
    struct UI_SETTING_ICON_INFO icon;
} UI_SETTINGICON_INFO;
UI_SETTINGICON_INFO ui_icon[6];
static struct
{
    lv_style_t unfocused_0; // 在焦点旁边 半透明
    lv_style_t unfocused_1; // 远离焦点 低透明度
    lv_style_t unfocused_2; // 远离焦点 低透明度
    lv_style_t focus;
    lv_style_t font16;
    lv_style_t font20;
} style;
static struct
{
    lv_obj_t *name;
    lv_obj_t *mid;
    lv_obj_t *arc;
    lv_obj_t *roller;
    lv_obj_t *wifi_roller;
    lv_obj_t *bottom_label;
} ui_label;

static foc_knob_param_t lamp_foc_knob_param_lst[] = {
    [0] = {101, 0, 3 * PI / 180, 1, 1, 1.1, ""}, // 台灯亮度
    [1] = {181, 0, 2 * PI / 180, 1, 1, 1.1, ""}, // 台灯色温
    [2] = {101, 0, 5 * PI / 180, 1, 1, 1.1, ""}, // 背光亮度
    [3] = {101, 0, 5 * PI / 180, 1, 1, 1.1, ""}, // RGB-H
    [4] = {5, 0, 30 * PI / 180, 1, 1, 1.1, ""},  // RGB_MODE
    [5] = {2, 0, 30 * PI / 180, 1, 1, 1.1, ""},  // wifi开关
};
static const foc_knob_param_t press_foc_knob_param = {0, 0, 20 * PI / 180, 1, 1, 1.1, ""}; // 按下旋转的力度
static void ui_icon_info_init()
{
    uint8_t id;
    id = 0;
    ui_icon[id].icon_id = id;
    ui_icon[id].param_list = lamp_foc_knob_param_lst[id];

    ui_icon[id].icon.img_src = &ui_img_light_png;
    ui_icon[id].icon.name = "亮度";
    ui_icon[id].current_position = 40;
    ui_icon[id].param_list.position = ui_icon[id].current_position-1;

    id = 1;
    ui_icon[id].icon_id = id;
    ui_icon[id].param_list = lamp_foc_knob_param_lst[id];

    ui_icon[id].icon.img_src = &ui_img_screenlocklandscape_png;
    ui_icon[id].icon.name = "色温";
    ui_icon[id].current_position = 90;
    ui_icon[id].param_list.position = ui_icon[id].current_position-1;

    id = 2;
    ui_icon[id].icon_id = id;
    ui_icon[id].param_list = lamp_foc_knob_param_lst[id];

    ui_icon[id].icon.img_src = &ui_img_sleep_png;
    ui_icon[id].icon.name = "RGB亮度";
    ui_icon[id].current_position = 1;
    ui_icon[id].param_list.position = ui_icon[id].current_position;

    id = 3;
    ui_icon[id].icon_id = id;
    ui_icon[id].param_list = lamp_foc_knob_param_lst[id];

    ui_icon[id].icon.img_src = &ui_img_home_page_png;
    ui_icon[id].icon.name = "RGB-H";
    ui_icon[id].current_position = 1;
    ui_icon[id].param_list.position = ui_icon[id].current_position;

    id = 4;
    ui_icon[id].icon_id = id;
    ui_icon[id].param_list = lamp_foc_knob_param_lst[id];

    ui_icon[id].icon.img_src = &ui_img_screenrotation_png;
    ui_icon[id].icon.name = "灯带模式";
    ui_icon[id].current_position = 1;
    ui_icon[id].param_list.position = ui_icon[id].current_position;

    id = 5;
    ui_icon[id].icon_id = id;
    ui_icon[id].param_list = lamp_foc_knob_param_lst[id];

    ui_icon[id].icon.img_src = &ui_img_wifilogo_png;
    ui_icon[id].icon.name = "模式切换";
    ui_icon[id].current_position = 1;
    ui_icon[id].param_list.position = ui_icon[id].current_position;
}
void ui_lamp_screen_event(uint8_t state)
{
    switch (icon_index)
    {
    case 0:
        switch (state) // 台灯亮度
        {
        case DIAL_TURN_RIGHT:
            if (ui_icon[icon_index].current_position < 100)
                ui_icon[icon_index].current_position++;
            // espnow_report_smartlamp(LAMP_CMD_BRIGHTNESS_SET, ui_icon[icon_index].current_position*2, 0, NULL);
            break;
        case DIAL_TURN_LEFT:
            if (ui_icon[icon_index].current_position > 0)
                ui_icon[icon_index].current_position--;
            // espnow_report_smartlamp(LAMP_CMD_BRIGHTNESS_SET, ui_icon[icon_index].current_position*2, 0, NULL);
            break;
        }
        break;
    case 1:
        switch (state) // 台灯色温
        {
        case DIAL_TURN_RIGHT:
            if (ui_icon[icon_index].current_position < 180)
                ui_icon[icon_index].current_position++;
            // espnow_report_smartlamp(LAMP_CMD_COLORT_SET, ui_icon[icon_index].current_position * 2, 0, NULL);
            break;
        case DIAL_TURN_LEFT:
            if (ui_icon[icon_index].current_position > 0)
                ui_icon[icon_index].current_position--;
            // espnow_report_smartlamp(LAMP_CMD_COLORT_SET, ui_icon[icon_index].current_position * 2, 0, NULL);
            break;
        }
        break;
    case 2:
        switch (state) //
        {
        case DIAL_TURN_RIGHT:
            if (ui_icon[icon_index].current_position < 180)
                ui_icon[icon_index].current_position++;
            break;
        case DIAL_TURN_LEFT:
            if (ui_icon[icon_index].current_position > 0)
                ui_icon[icon_index].current_position--;
            break;
        }
        break;
    case 3:
        switch (state) //
        {
        case DIAL_TURN_RIGHT:
            if (ui_icon[icon_index].current_position < 180)
                ui_icon[icon_index].current_position++;
            break;
        case DIAL_TURN_LEFT:
            if (ui_icon[icon_index].current_position > 0)
                ui_icon[icon_index].current_position--;
            break;
        }
        break;
    case 4:
        switch (state) // 灯光模式
        {
        case DIAL_TURN_RIGHT:
            if (ui_icon[icon_index].current_position < 4)
                ui_icon[icon_index].current_position++;
            // espnow_report_smartlamp(LAMP_CMD_LIGHT_MODE_SET, ui_icon[icon_index].current_position, 0, NULL);
            break;
        case DIAL_TURN_LEFT:
            if (ui_icon[icon_index].current_position > 0)
                ui_icon[icon_index].current_position--;
            // espnow_report_smartlamp(LAMP_CMD_LIGHT_MODE_SET, ui_icon[icon_index].current_position, 0, NULL);
            break;
        }
        break;
    case 5:
        switch (state) // 模式切换
        {
        case DIAL_TURN_RIGHT:
            if (ui_icon[icon_index].current_position < 1)
                ui_icon[icon_index].current_position++;
            // espnow_report_smartlamp(LAMP_CMD_MODE_SET, ui_icon[icon_index].current_position, 0, NULL);
            break;
        case DIAL_TURN_LEFT:
            if (ui_icon[icon_index].current_position > 0)
                ui_icon[icon_index].current_position--;
            // espnow_report_smartlamp(LAMP_CMD_MODE_SET, ui_icon[icon_index].current_position, 0, NULL);
            break;
        }
        break;
    }
    ui_icon[icon_index].param_list.position = ui_icon[icon_index].current_position;
    switch (state)
    {
    case DIAL_PRESS_UP:
        foc_knob_user_set_param(ui_icon[icon_index].param_list);
        break;
    case DIAL_PRESS_DOWN:
        foc_knob_user_set_param(press_foc_knob_param);
        break;
    case DIAL_TURN_RIGHT:
        break;
    case DIAL_TURN_LEFT:
        break;
    case DIAL_CLICK:
        break;
    case DIAL_DOUBLE_CLICK:
        _ui_screen_change(&ui_lamp,&ui_main, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_main_screen_init);
        break;
    case DIAL_PRESS_RIGHT:
        enc_num++;
        break;
    case DIAL_PRESS_LEFT:
        enc_num--;
        break;
    default:
        break;
    }
}
static lv_obj_t *container_create(lv_obj_t *screen)
{
    lv_obj_t *ui_Container = lv_obj_create(screen);
    lv_obj_remove_style_all(ui_Container);
    lv_obj_set_width(ui_Container, 220);
    lv_obj_set_height(ui_Container, 115);
    lv_obj_set_x(ui_Container, 0);
    lv_obj_set_y(ui_Container, 74);
    lv_obj_set_align(ui_Container, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_Container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_Container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(ui_Container, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_obj_set_style_radius(ui_Container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Container, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Container, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui_Container, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui_Container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui_Container, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui_Container, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Container, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Container, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Container, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_Container, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_Container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_Container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_Container, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_Container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return ui_Container;
}
/**
 * @brief on focus set different information/icon
 *
 */
static void setl_label_info(uint8_t index)
{
    lv_label_set_text(ui_label.name, ui_icon[index].icon.name);
    lv_label_set_text(ui_label.mid, ui_icon[index].icon.mid_info);
    // 聚焦icon 显示对应lv_obj_t
    switch (index)
    {
    case 0: // 台灯亮度
        lv_obj_add_flag(ui_label.wifi_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.bottom_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.mid, LV_OBJ_FLAG_HIDDEN);
        lv_arc_set_value(ui_label.arc, ui_icon[index].current_position);
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[index].current_position);
        ui_icon[index].param_list.position = ui_icon[index].current_position - 1;
        break;
    case 1: // 台灯色温
        lv_obj_add_flag(ui_label.wifi_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.bottom_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.mid, LV_OBJ_FLAG_HIDDEN);
        lv_arc_set_value(ui_label.arc, ui_icon[index].current_position);
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[index].current_position);
        ui_icon[index].param_list.position = ui_icon[index].current_position - 1;
        break;
    case 2: // 背光亮度
        lv_obj_add_flag(ui_label.wifi_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.bottom_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.mid, LV_OBJ_FLAG_HIDDEN);
        lv_arc_set_value(ui_label.arc, ui_icon[index].current_position);
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[index].current_position);
        ui_icon[index].param_list.position = ui_icon[index].current_position - 1;
        break;
    case 3: // RGB-H
        lv_obj_add_flag(ui_label.wifi_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.bottom_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_label.mid, LV_OBJ_FLAG_HIDDEN);
        lv_arc_set_value(ui_label.arc, ui_icon[index].current_position);
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[index].current_position);
        ui_icon[index].param_list.position = ui_icon[index].current_position - 1;
        break;
    case 4: // 灯光模式
        lv_obj_clear_flag(ui_label.roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.mid, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.wifi_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.bottom_label, LV_OBJ_FLAG_HIDDEN);
        lv_roller_set_options(ui_label.roller, "Mode 0\nMode 1\nMode 2\nMode 3\nMode 4", LV_ROLLER_MODE_NORMAL);
        lv_roller_set_selected(ui_label.roller, ui_icon[index].current_position, LV_ANIM_ON);
        ui_icon[index].param_list.position = ui_icon[index].current_position;
        break;
    case 5: // 台灯模式
        lv_obj_clear_flag(ui_label.wifi_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.bottom_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.arc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_label.mid, LV_OBJ_FLAG_HIDDEN);
        lv_roller_set_options(ui_label.wifi_roller, "LAMP\nRGB", LV_ROLLER_MODE_NORMAL);
        lv_roller_set_selected(ui_label.wifi_roller, ui_icon[index].current_position, LV_ANIM_ON);
        ui_icon[index].param_list.position = ui_icon[index].current_position;
        break;
    }
    foc_knob_user_set_param(ui_icon[index].param_list);
}
static void Style_Init()
{
    lv_style_init(&style.focus);
    lv_style_set_radius(&style.focus, 15);
    lv_style_set_bg_color(&style.focus, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&style.focus, 50);
    lv_style_set_bg_grad_color(&style.focus, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_main_stop(&style.focus, 150);
    lv_style_set_bg_grad_stop(&style.focus, 255);
    lv_style_set_bg_grad_dir(&style.focus, LV_GRAD_DIR_VER);

    lv_style_init(&style.unfocused_0);
    lv_style_set_opa(&style.unfocused_0, 150);
    // lv_style_set_transform_zoom(&style.unfocused_0,220);
    lv_style_set_translate_x(&style.unfocused_0, 0);
    lv_style_set_translate_y(&style.unfocused_0, 15);

    lv_style_init(&style.unfocused_1);
    lv_style_set_opa(&style.unfocused_1, 100);
    // lv_style_set_transform_zoom(&style.unfocused_1, 150);
    lv_style_set_translate_x(&style.unfocused_1, 15);
    lv_style_set_translate_y(&style.unfocused_1, 25);

    lv_style_init(&style.unfocused_2);
    lv_style_set_opa(&style.unfocused_2, 100);
    // lv_style_set_transform_zoom(&style.unfocused_2, 150);
    lv_style_set_translate_x(&style.unfocused_2, -15);
    lv_style_set_translate_y(&style.unfocused_2, 25);

    static const lv_style_prop_t style_prop[] =
        {
            LV_STYLE_TRANSLATE_X,
            // LV_STYLE_TRANSLATE_Y,
            // LV_STYLE_TRANSFORM_ZOOM,
            LV_STYLE_PROP_INV};

    static lv_style_transition_dsc_t trans;
    lv_style_transition_dsc_init(
        &trans,
        style_prop,
        lv_anim_path_bounce,
        200,
        0,
        NULL);
    lv_style_set_transition(&style.focus, &trans);
    lv_style_set_transition(&style.unfocused_0, &trans);
    lv_style_set_transition(&style.unfocused_1, &trans);
    lv_style_set_transition(&style.unfocused_2, &trans);

    lv_style_init(&style.font20);
    lv_style_set_text_font(&style.font20, &ui_font_SmileySansOblique20);
    lv_style_init(&style.font16);
    lv_style_set_text_font(&style.font16, &ui_font_SmileySansOblique16);
}
static lv_obj_t *icon_create(lv_obj_t *container, const void *img_src, uint16_t img_angle)
{
    lv_obj_t *ui_Image = lv_img_create(container);

    lv_img_set_src(ui_Image, img_src);
    lv_obj_remove_style_all(ui_Image);
    lv_obj_clear_flag(ui_Image, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_Image, &style.unfocused_0, LV_STATE_USER_1);
    lv_obj_add_style(ui_Image, &style.unfocused_1, LV_STATE_USER_2);
    lv_obj_add_style(ui_Image, &style.unfocused_2, LV_STATE_USER_3);
    lv_obj_add_style(ui_Image, &style.focus, LV_STATE_FOCUSED);
    lv_img_set_angle(ui_Image, img_angle);
    return ui_Image;
}
static void onFocus(lv_group_t *g)
{
    lvgl_port_lock(0);
    lv_obj_t *icon = lv_group_get_focused(g);
    uint32_t current_btn_index = lv_obj_get_index(icon);
    // printf("current_btn_index:%ld\n", current_btn_index);
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
        lv_obj_clear_state(obt, LV_STATE_USER_1 | LV_STATE_USER_2 | LV_STATE_USER_3);
        // 远离中心两个图标以上的图标隐藏
        if (i + 2 < mid_btn_index)
        {
            lv_obj_add_flag(obt, LV_OBJ_FLAG_HIDDEN);
        }
        else if (i > mid_btn_index + 2)
        {
            lv_obj_add_flag(obt, LV_OBJ_FLAG_HIDDEN);
        }
        else
            lv_obj_clear_flag(obt, LV_OBJ_FLAG_HIDDEN);
    }
    // 设置中间焦点左右两边图标样式，实现越远越小，越远越透明的效果,多个if以防icon数量不到5个
    if (cnt > 1)
    {
        lv_img_set_zoom(lv_obj_get_child(container, mid_btn_index), 256);                  /// ZOOM
        lv_obj_add_state(lv_obj_get_child(container, mid_btn_index - 1), LV_STATE_USER_1); /// States
        lv_img_set_zoom(lv_obj_get_child(container, mid_btn_index - 1), 220);              /// ZOOM
    }
    if (cnt > 2)
    {
        lv_obj_add_state(lv_obj_get_child(container, mid_btn_index + 1), LV_STATE_USER_1); /// States
        lv_img_set_zoom(lv_obj_get_child(container, mid_btn_index + 1), 220);              /// ZOOM
    }
    if (cnt > 3)
    {
        lv_obj_add_state(lv_obj_get_child(container, mid_btn_index - 2), LV_STATE_USER_2); /// States
        lv_img_set_zoom(lv_obj_get_child(container, mid_btn_index - 2), 150);              /// ZOOM
    }
    if (cnt > 4)
    {
        lv_obj_add_state(lv_obj_get_child(container, mid_btn_index + 2), LV_STATE_USER_3); /// States
        lv_img_set_zoom(lv_obj_get_child(container, mid_btn_index + 2), 150);              /// ZOOM
    }
    setl_label_info(icon_index);

    /* Task unlock */
    lvgl_port_unlock();
}
static void group_init(lv_obj_t *container, int8_t id)
{
    icon_index = 0;
    group = lv_group_create();
    uint8_t cnt = lv_obj_get_child_cnt(container);
    // printf("%ld\n", cnt);
    for (uint8_t i = 0; i < cnt; i++)
    {
        lv_group_add_obj(group, lv_obj_get_child(container, i));
    }
    lv_group_set_focus_cb(group, onFocus);
    uint32_t mid_btn_index = (lv_obj_get_child_cnt(container) - 1) / 2;
    for (uint8_t i = 0; i < id; i++)
    {
        lv_group_focus_obj(lv_obj_get_child(container, -1));
    }
    lv_group_focus_obj(lv_obj_get_child(container, mid_btn_index));
}
static void Create(lv_obj_t *root)
{
    ui_label.name = lv_label_create(root);
    lv_obj_set_width(ui_label.name, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_label.name, LV_SIZE_CONTENT); /// 1
    lv_obj_set_x(ui_label.name, 0);
    lv_obj_set_y(ui_label.name, 93);
    lv_obj_set_align(ui_label.name, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_label.name, &style.font20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_label.mid = lv_label_create(root);
    lv_obj_set_width(ui_label.mid, LV_SIZE_CONTENT);  /// 73
    lv_obj_set_height(ui_label.mid, LV_SIZE_CONTENT); /// 1
    lv_obj_set_y(ui_label.mid, -30);
    lv_obj_set_align(ui_label.mid, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_label.mid, &style.font16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_label.arc = lv_arc_create(root);
    lv_obj_set_width(ui_label.arc, 220);
    lv_obj_set_height(ui_label.arc, 220);
    lv_obj_set_align(ui_label.arc, LV_ALIGN_CENTER);
    // lv_arc_set_value(ui_label.arc,50);
    lv_arc_set_bg_angles(ui_label.arc, 180, 0);
    lv_obj_set_style_arc_width(ui_label.arc, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(ui_label.arc, lv_color_hex(0x1A50BB), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_label.arc, 100, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_label.arc, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_opa(ui_label.arc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_label.roller = lv_roller_create(root);
    lv_obj_set_height(ui_label.roller, 100);
    lv_obj_set_width(ui_label.roller, LV_SIZE_CONTENT); /// 1
    lv_obj_set_x(ui_label.roller, 0);
    lv_obj_set_y(ui_label.roller, -45);
    lv_obj_set_align(ui_label.roller, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_label.roller, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_label.roller, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_label.roller, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_label.roller, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_label.roller, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_label.roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label.roller, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_label.roller, lv_color_hex(0x005BBB), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_label.roller, 100, LV_PART_SELECTED | LV_STATE_DEFAULT);

    ui_label.wifi_roller = lv_roller_create(root);
    lv_obj_set_height(ui_label.wifi_roller, 50);
    lv_obj_set_width(ui_label.wifi_roller, LV_SIZE_CONTENT); /// 1
    lv_obj_set_x(ui_label.wifi_roller, 0);
    lv_obj_set_y(ui_label.wifi_roller, -60);
    lv_obj_set_align(ui_label.wifi_roller, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_label.wifi_roller, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_label.wifi_roller, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_label.wifi_roller, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_label.wifi_roller, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_label.wifi_roller, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_label.wifi_roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label.wifi_roller, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_label.wifi_roller, lv_color_hex(0x005BBB), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_label.wifi_roller, 100, LV_PART_SELECTED | LV_STATE_DEFAULT);

    ui_label.bottom_label = lv_label_create(root);
    lv_obj_set_width(ui_label.bottom_label, LV_SIZE_CONTENT);  /// 73
    lv_obj_set_height(ui_label.bottom_label, LV_SIZE_CONTENT); /// 1
    lv_obj_set_y(ui_label.bottom_label, 0);
    lv_obj_set_align(ui_label.bottom_label, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_label.bottom_label, &style.font16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_flag(ui_label.roller, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_label.arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_label.mid, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_label.bottom_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_label.wifi_roller, LV_OBJ_FLAG_HIDDEN);
}

static void scr_Screen_lamp_unloaded_cb(lv_event_t *e)
{
    lv_obj_t **var = lv_event_get_user_data(e);

    // 检查 timer_task 是否有效
    if (timer_task)
    {
        lv_timer_del(timer_task);
        timer_task = NULL; // 置空，避免悬空指针
    }
    // // 检查 group 是否有效
    // if (group)
    // {
    //     lv_group_del(group);
    //     group = NULL; // 置空，避免悬空指针
    // }

    // // 检查 var 是否有效
    // if (*var)
    // {
    //     lv_obj_del(*var);
    //     (*var) = NULL; // 置空，避免悬空指针
    // }
}
static void scr_Screen_lamp_loaded_cb(lv_event_t *e)
{
    ui_state.index = UI_LAMP;
    lv_indev_set_group(encoder_indev, group);
    foc_knob_user_set_param(ui_icon[icon_index].param_list);
}
static void task_timer_cb()
{
    switch (icon_index)
    {
    case 0:
        lv_arc_set_value(ui_label.arc, ui_icon[icon_index].current_position);
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[icon_index].current_position);
        break;
    case 1:
        lv_arc_set_value(ui_label.arc, (int16_t)(ui_icon[icon_index].current_position / 1.8));
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[icon_index].current_position * 2);
        break;
    case 2:
        lv_arc_set_value(ui_label.arc, ui_icon[icon_index].current_position);
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[icon_index].current_position);
        break;
    case 3:
        lv_arc_set_value(ui_label.arc, ui_icon[icon_index].current_position);
        lv_label_set_text_fmt(ui_label.mid, "%d", ui_icon[icon_index].current_position);
        break;
    case 4:
        lv_roller_set_selected(ui_label.roller, ui_icon[icon_index].current_position, LV_ANIM_ON);
        break;
    case 5:
        lv_roller_set_selected(ui_label.wifi_roller, ui_icon[icon_index].current_position, LV_ANIM_ON);
        break;
    default:
        break;
    }
}
void ui_lamp_screen_init(void)
{
    ui_lamp = lv_obj_create(NULL);
    ui_icon_info_init();
    lv_obj_add_event_cb(ui_lamp, scr_Screen_lamp_loaded_cb, LV_EVENT_SCREEN_LOADED, &ui_lamp);
    lv_obj_add_event_cb(ui_lamp, scr_Screen_lamp_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, &ui_lamp);
    lv_obj_clear_flag(ui_lamp, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_obj_set_style_bg_img_src(ui_lamp, &ui_img_bg1_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    Style_Init();
    Create(ui_lamp);
    setl_label_info(icon_index);
    lv_obj_t *container = container_create(ui_lamp);
    for (uint8_t i = 0; i < ICON_CNT; i++)
    {
        icon_create(container, ui_icon[i].icon.img_src, ui_icon[i].img_angle);
    }
    uint32_t mid_btn_index = (lv_obj_get_child_cnt(container) - 1) / 2;
    for (uint32_t i = 0; i < mid_btn_index; i++)
    {
        lv_obj_move_to_index(lv_obj_get_child(container, -1), 0);
    }
    group_init(container, icon_index);
    timer_task = lv_timer_create(task_timer_cb, 50, 0);
    lv_timer_set_repeat_count(timer_task, -1);
}
