#pragma once
#include <stdio.h>
#include "esp_hidd_prf_api.h"
#include "tinyusb.h"
#include <class/hid/hid_device.h>

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        HID_ITF_PROTOCOL_DIAL = 3,
        HID_ITF_PROTOCOL_MEDIA = 4,
    };
    void set_knob_battery_level(uint8_t level);
#define TUD_HID_REPORT_DESC_DIAL(...)                                      \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                \
        HID_USAGE(0x0e),                                                   \
        HID_COLLECTION(HID_COLLECTION_APPLICATION), /* Report ID if any */ \
        __VA_ARGS__                                                        \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DIGITIZER),                          \
        HID_USAGE(0x21),                                                   \
        HID_COLLECTION(HID_COLLECTION_PHYSICAL),                           \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                             \
        HID_USAGE(1),                                                      \
        HID_REPORT_COUNT(1),                                               \
        HID_REPORT_SIZE(1),                                                \
        HID_LOGICAL_MIN(0),                                                \
        HID_LOGICAL_MAX(1),                                                \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                 \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                            \
        HID_USAGE(HID_USAGE_DESKTOP_DIAL),                                 \
        HID_REPORT_COUNT(1),                                               \
        HID_REPORT_SIZE(15),                                               \
        HID_UNIT_EXPONENT(15),                                             \
        HID_UNIT(0x14),                                                    \
        HID_PHYSICAL_MIN_N(-3600, 2),                                      \
        HID_PHYSICAL_MAX_N(3600, 2),                                       \
        HID_LOGICAL_MIN_N(-3600, 2),                                       \
        HID_LOGICAL_MAX_N(3600, 2),                                        \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_RELATIVE),                 \
        HID_COLLECTION_END,                                                \
        HID_COLLECTION_END

// Consumer Control Report Descriptor Template
#define TUD_HID_REPORT_DESC_CONSUMER(...)                                  \
    HID_USAGE_PAGE(HID_USAGE_PAGE_CONSUMER),                               \
        HID_USAGE(HID_USAGE_CONSUMER_CONTROL),                             \
        HID_COLLECTION(HID_COLLECTION_APPLICATION), /* Report ID if any */ \
        __VA_ARGS__                                                        \
            HID_LOGICAL_MIN(0x00),                                         \
        HID_LOGICAL_MAX_N(0x03FF, 2),                                      \
        HID_USAGE_MIN(0x00),                                               \
        HID_USAGE_MAX_N(0x03FF, 2),                                        \
        HID_REPORT_COUNT(1),                                               \
        HID_REPORT_SIZE(16),                                               \
        HID_INPUT(HID_DATA | HID_ARRAY | HID_ABSOLUTE),                    \
        HID_COLLECTION_END

    // HID Report Map characteristic value
    // Keyboard report descriptor (using format for Boot interface descriptor)
    static const uint8_t hidReportMap[] = {
        TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
        TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)),
        TUD_HID_REPORT_DESC_DIAL(HID_REPORT_ID(HID_ITF_PROTOCOL_DIAL)),
        TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(HID_ITF_PROTOCOL_MEDIA)),
    };

    typedef enum
    {
        HID_NULL,
        HID_KEYBOARD_Clock,
        HID_KEYBOARD_Press, // 一起输出
        HID_MEDIA_Clock,
        HID_MEDIA_Press,
        KBWrite,      // 依次输出
        KGame,        // 游戏键盘
        KMouse_move,  // 鼠标指针移到
        KMouse_wheel, // 竖滚动条移动
        KMouse_pan,   // 横滚动条移动
        KMouse_press, // 鼠标按钮
        KDial,        // 旋钮默认功能
    } KEYOUT_TYPE;
    void hid_init();

    typedef struct
    {
        uint8_t hid_id;
        uint8_t state;
        uint8_t hid_data[7];
    } Command_HID;
#ifdef __cplusplus
}
#endif