#include "ui.h"
#include "ui_helpers.h"
#define TAG "UI"
lv_obj_t *ui_main;
lv_obj_t *ui_about;
lv_obj_t *ui_knob;
lv_obj_t *ui_lamp;
lv_obj_t *ui_set;
lv_obj_t *ui_dial;
lv_obj_t *ui_sleep;
lv_obj_t *ui_root;

uint8_t ui_event_flag = 0;
int16_t enc_num = 0;
int16_t enc_click = 0;

lv_indev_t *encoder_indev;
static lv_indev_drv_t indev_drv;
_ui_state ui_state;
static void encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    data->enc_diff = enc_num;
    enc_num = 0;

    if (enc_click)
    {
        data->state = LV_INDEV_STATE_PR;
        enc_click = 0;
    }
    else
        data->state = LV_INDEV_STATE_REL;
}
static void lv_port_indev_init(void)
{
    lv_indev_drv_init(&indev_drv); // 注册编码器输入设备
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read;
    // indev_drv.long_press_time = 2000;           // 按下 2s 为长按
    // indev_drv.long_press_repeat_time = 500;    // 间隔 0.5s 发送LV_EVENT_LONG_PRESSED_REPEAT 事件
    encoder_indev = lv_indev_drv_register(&indev_drv);
}
// 弹窗ui
lv_obj_t *popup; // 定义弹窗对象
static void popup_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED)
    {
        // 关闭弹窗
        lv_obj_add_flag(popup, LV_OBJ_FLAG_HIDDEN);
    }
}
void init_popup(void)
{
    // 创建弹窗
    popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(popup, 200, 150);
    lv_obj_center(popup);
    lv_obj_add_flag(popup, LV_OBJ_FLAG_HIDDEN); // 初始状态为隐藏
    // 创建背景
    lv_obj_set_style_bg_color(popup, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_border_color(popup, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(popup, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(popup, 10, LV_PART_MAIN);

    // 创建标题
    lv_obj_t *label = lv_label_create(popup);
    lv_label_set_text(label, "这是一个弹窗");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
}
void deinit_popup(void)
{
    if (popup != NULL)
    {
        lv_obj_del(popup);
        popup = NULL;
    }
}

void ui_init(bool key)
{
    lv_port_indev_init();
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_GREEN),
                                              true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    if (key)
    {
        ui_main_screen_init();
        lv_disp_load_scr(ui_main);
    }
    else
    {
        ui_root_screen_init();
        lv_disp_load_scr(ui_root);
    }
    // init_popup();
}
