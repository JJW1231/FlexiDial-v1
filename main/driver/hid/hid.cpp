#include "hid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "esp_log.h"
#include "iot_button.h"
#include "cJSON.h"
#include "hidd_le_prf_int.h"
static const char *TAG = "hid";
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

void set_knob_battery_level(uint8_t level)
{
    set_battary_level(level);
}
// HID报告描述符
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)),
    TUD_HID_REPORT_DESC_DIAL(HID_REPORT_ID(HID_ITF_PROTOCOL_DIAL)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(HID_ITF_PROTOCOL_MEDIA)),
};
// 字串描述符
const char *hid_string_descriptor[5] = {
    // 指向字符串描述符的指针数组
    (char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "JJW",                // 1: Manufacturer
    "SmartKnob",          // 2: Product
    "123456",             // 3: Serials, should use chip ID
    "HID interface",      // 4: HID
};

/**
 * @brief配置描述符
 *
 * 这是一个简单的配置描述符，它定义了一个配置和一个HID接口
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/********* TinyUSB HID callbacks ***************/

// 当收到GET HID报告描述符请求时调用
// 应用程序返回指向描述符的指针，其内容必须存在足够长的时间才能完成传输
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// 当收到GET_REPORT控件请求时调用
// 应用程序必须填充缓冲区报告的内容并返回其长度。
// 返回0将导致堆栈暂停请求
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// 当接收到SET_REPORT控制请求时调用
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    printf("Set report :%d\n", report_id);
}

extern QueueHandle_t Dial_Queue;

void hid_init()
{
    tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
    };
    tusb_cfg.configuration_descriptor = hid_configuration_descriptor;
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
   
}

void usb_device_uninstall(void)
{
    // not use
    ESP_ERROR_CHECK(tinyusb_driver_uninstall());
}
