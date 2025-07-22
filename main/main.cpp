#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_task_wdt.h"
#include "driver/power/power.h"
#include "driver/motor/motor.h"
#include "driver/audio/audio.h"
#include "driver/audio/app_sr.h"
#include "driver/button/button.h"
#include "driver/display/display.h"
#include "driver/gyro/gyro.h"
#include "driver/hid/hid.h"
#include "driver/rgb/rgb.h"

#include "ui/events/events.h"

static const char *TAG = "Main";

// #include "driver/pn532/pn532_driver_hsu.h"
// #include "driver/pn532/pn532.h"

// // select ONLY ONE interface for the PN532
// #define PN532_MODE_I2C 0
// #define PN532_MODE_HSU 1
// #define PN532_MODE_SPI 0

// // HSU mode needs only RX/TX pins. RESET pin will be used if valid.
// #define RESET_PIN (GPIO_NUM_NC)
// #define IRQ_PIN (GPIO_NUM_NC)
// #define HSU_HOST_RX (GPIO_NUM_48)
// #define HSU_HOST_TX (GPIO_NUM_47)
// #define HSU_UART_PORT UART_NUM_1
// #define HSU_BAUD_RATE (1152000)

// void app_pn532()
// {
//     pn532_io_t pn532_io;
//     esp_err_t err;
//     ESP_ERROR_CHECK(pn532_new_driver_hsu(HSU_HOST_RX,
//                                          HSU_HOST_TX,
//                                          RESET_PIN,
//                                          IRQ_PIN,
//                                          HSU_UART_PORT,
//                                          HSU_BAUD_RATE,
//                                          &pn532_io));
//     do
//     {
//         err = pn532_init(&pn532_io);
//         if (err != ESP_OK)
//         {
//             ESP_LOGW(TAG, "初始化PN532失败");
//             pn532_release(&pn532_io);
//             vTaskDelay(1000 / portTICK_PERIOD_MS);
//         }
//     } while (err != ESP_OK);

//     ESP_LOGI(TAG, "获取固件版本");
//     uint32_t version_data = 0;
//     do
//     {
//         err = pn532_get_firmware_version(&pn532_io, &version_data);
//         if (ESP_OK != err)
//         {
//             ESP_LOGI(TAG, "Didn't find PN53x board");
//             pn532_reset(&pn532_io);
//             vTaskDelay(1000 / portTICK_PERIOD_MS);
//         }
//     } while (ESP_OK != err);

//     // Log firmware infos
//     ESP_LOGI(TAG, "发现芯片 PN5%x", (unsigned int)(version_data >> 24) & 0xFF);
//     ESP_LOGI(TAG, "固件版本. %d.%d", (int)(version_data >> 16) & 0xFF, (int)(version_data >> 8) & 0xFF);

//     ESP_LOGI(TAG, "等待ISO14443A卡 ...");
//     while (1)
//     {
//         uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // 用于存储返回UID的缓冲区
//         uint8_t uid_length;                    // UID长度（根据ISO14443A卡类型不同，长度为4或7个字节）

//         // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
//         // 'uid' will be populated with the UID, and uid_length will indicate
//         // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
//         err = pn532_read_passive_target_id(&pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);

//         if (ESP_OK == err)
//         {
//             // 显示卡片的一些基本信息
//             ESP_LOGI(TAG, "\nFound an ISO14443A card");
//             ESP_LOGI(TAG, "UID Length: %d bytes", uid_length);
//             ESP_LOGI(TAG, "UID Value:");
//             ESP_LOG_BUFFER_HEX_LEVEL(TAG, uid, uid_length, ESP_LOG_INFO);

//             err = pn532_in_list_passive_target(&pn532_io);
//             if (err != ESP_OK)
//             {
//                 ESP_LOGI(TAG, "Failed to inList passive target");
//                 continue;
//             }

//             NTAG2XX_MODEL ntag_model = NTAG2XX_UNKNOWN;
//             err = ntag2xx_get_model(&pn532_io, &ntag_model);
//             if (err != ESP_OK)
//                 continue;

//             int page_max;
//             switch (ntag_model)
//             {
//             case NTAG2XX_NTAG213:
//                 page_max = 45;
//                 ESP_LOGI(TAG, "发现NTAG213目标(或者可能是NTAG203)");
//                 break;

//             case NTAG2XX_NTAG215:
//                 page_max = 135;
//                 ESP_LOGI(TAG, "找到NTAG215目标");
//                 break;

//             case NTAG2XX_NTAG216:
//                 page_max = 231;
//                 ESP_LOGI(TAG, "found NTAG216 target");
//                 break;

//             default:
//                 ESP_LOGI(TAG, "Found unknown NTAG target!");
//                 continue;
//             }

//             for (int page = 0; page < page_max; page += 4)
//             {
//                 uint8_t buf[16];
//                 err = ntag2xx_read_page(&pn532_io, page, buf, 16);
//                 if (err == ESP_OK)
//                 {
//                     ESP_LOG_BUFFER_HEXDUMP(TAG, buf, 16, ESP_LOG_INFO);
//                 }
//                 else
//                 {
//                     ESP_LOGI(TAG, "读取页面失败 %d", page);
//                     break;
//                 }
//             }
//             vTaskDelay(1000 / portTICK_PERIOD_MS);
//         }
//     }
// }


extern "C" void app_main(void)
{
    vTaskDelay(1000);
    printf("Hello world!\n");
    ESP_LOGI(TAG, "使用前剩余堆栈大小:%lu", esp_get_free_heap_size());
    // nvs_data_init();
    power_init();
    dial_event_init();
    lvgl_display_init(1);
    button_init();
    foc_init();
    // hid_init();
    // ble_hid_init();
    // audio_init();
    // app_sr_start();
}
