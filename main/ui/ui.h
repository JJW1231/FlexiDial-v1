#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include "lvgl.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_app_desc.h"
#include "ui_helpers.h"
#include "events/events.h"
#include "driver/motor/motor.h"
#include "driver/display/display.h"

    enum
    {
        UI_NULL,
        UI_MENU_INTERFACE, /*!<主菜单选择不同的界面 */
        UI_POWER_ABOUT,    /*!<电源控制 */
        UI_SURFACE_KNOB,   /*!<Surface dial控制界面*/
        UI_LAMP,           /*!<台灯控制界面*/
        UI_SET,            /*!<旋钮设置*/
        UI_FLEXIDIAL,      /*!<灵活旋钮 */
        UI_SLEEP,
        UI_ROOT,
    };
    typedef struct
    {
        uint8_t index; // current ui screen index
    } _ui_state;
    struct DIAL_STA_DATA
    {
        uint8_t hid_data[7];
    };
    struct UI_HID_ICON_INFO
    {
        const void *img_src;
        const char *name;
        const char *left_info;
        const char *mid_info;
        const char *right_info;
    };
    typedef struct
    {
        uint8_t icon_id;    // 图标的id
        uint8_t hid_id;     // hid设备id
        uint16_t img_angle; // 图标的角度
        foc_knob_param_t param_list;
        struct DIAL_STA_DATA dial_sta[10];
        struct UI_HID_ICON_INFO icon;
    } UI_HID_INFO;
    extern _ui_state ui_state;
    extern int16_t enc_num;
    extern int16_t enc_click;

    extern lv_indev_t *encoder_indev;

    extern lv_obj_t *popup;
    extern lv_obj_t *ui_main; // main screen
    void ui_main_screen_event(uint8_t state);
    void ui_main_screen_init(void);

    extern lv_obj_t *ui_about;
    void ui_about_screen_event(uint8_t state);
    void ui_about_screen_init(void);

    extern lv_obj_t *ui_knob;
    void ui_knob_screen_event(uint8_t state);
    void ui_knob_screen_init(void);

    extern lv_obj_t *ui_lamp;
    void ui_lamp_screen_event(uint8_t state);
    void ui_lamp_screen_init(void);

    extern lv_obj_t *ui_set;
    void ui_set_screen_event(uint8_t state);
    void ui_set_screen_init(void);

    extern lv_obj_t *ui_dial;
    void ui_dial_screen_event(uint8_t state);
    void ui_dial_screen_init(void);

    extern lv_obj_t *ui_sleep;
    void ui_sleep_screen_event(uint8_t state);
    void ui_sleep_screen_init(void);

    extern lv_obj_t *ui_root;
    void ui_root_screen_event(uint8_t state);
    void ui_root_screen_init(void);

    LV_IMG_DECLARE(ui_lamp_icon); // 台灯图标 48*48

    LV_IMG_DECLARE(ui_img_bg1_png);        // assets\bg1.png
    LV_IMG_DECLARE(ui_img_pc_png);         // assets\pc.png
    LV_IMG_DECLARE(ui_img_set_png);        // assets\set.png
    LV_IMG_DECLARE(ui_img_power_png);      // assets\power.png
    LV_IMG_DECLARE(ui_img_copy_png);       // assets\copy.png
    LV_IMG_DECLARE(ui_img_left_right_png); // assets\left_right.png
    LV_IMG_DECLARE(ui_img_bg_dial_png);
    LV_IMG_DECLARE(ui_img_dial_png);                // assets\dial.png
    LV_IMG_DECLARE(ui_img_up_dowm_png);             // assets\up_dowm.png
    LV_IMG_DECLARE(ui_img_wheel_png);               // assets\wheel.png
    LV_IMG_DECLARE(ui_img_key_left_right_png);      // assets\key_left_right.png
    LV_IMG_DECLARE(ui_img_volume_png);              // assets\volume.png
    LV_IMG_DECLARE(ui_img_pointer_png);             // assets\pointer.png
    LV_IMG_DECLARE(ui_img_bg_anime_png);            // assets\bg_anime.png
    LV_IMG_DECLARE(ui_img_customize_png);           // assets\customize.png
    LV_IMG_DECLARE(ui_img_screenrotation_png);      // assets\ScreenRotation.png
    LV_IMG_DECLARE(ui_img_wifilogo_png);            // assets\WifiLogo.png
    LV_IMG_DECLARE(ui_img_light_png);               // assets\light.png
    LV_IMG_DECLARE(ui_img_screenlocklandscape_png); // assets\ScreenLockLandscape.png
    LV_IMG_DECLARE(ui_img_sleep_png);               // assets\sleep.png
    LV_IMG_DECLARE(ui_img_ble_png);                 // assets\ble.png
    LV_IMG_DECLARE(ui_img_home_page_png);           // assets\home_page.png

    LV_FONT_DECLARE(ui_font_SmileySansOblique16);
    LV_FONT_DECLARE(ui_font_SmileySansOblique20);
    LV_FONT_DECLARE(ui_font_china22);
    LV_FONT_DECLARE(ui_font_time_num64);
    LV_FONT_DECLARE(ui_font_day24);
    // LV_FONT_DECLARE(ui_font_root32);
    // LV_FONT_DECLARE(font_puhui_20_4);
    void ui_init(bool key);
    void init_popup(void);
    void deinit_popup(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif
