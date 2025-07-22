/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SR_CONTINUE_DET 1

typedef struct {
    wakenet_state_t     wakenet_mode;
    esp_mn_state_t      state;
    int                 command_id;
} sr_result_t;
typedef enum
{
    SR_LANG_EN,
    SR_LANG_CN,
    SR_LANG_MAX,
} sr_language_t;
typedef struct
{
    sr_language_t lang;
#if 1
    model_iface_data_t *model_data;
    const esp_mn_iface_t *multinet;
    const esp_afe_sr_iface_t *afe_handle;
    esp_afe_sr_data_t *afe_data;
#endif
    int16_t *afe_in_buffer;
    int16_t *afe_out_buffer;
    uint8_t cmd_num;
    TaskHandle_t feed_task;
    TaskHandle_t detect_task;
    TaskHandle_t handle_task;
    TaskHandle_t chat_task;
    QueueHandle_t result_que;
    EventGroupHandle_t event_group;
    FILE *fp;
    bool b_record_en;
} sr_data_t;

typedef enum
{
    AUDIO_CHAT_MODE_GPT = 0, // GPT聊天
    AUDIO_CHAT_MODE_ASR,     // 语音转文字
    AUDIO_CHAT_MODE_TTS,     // 文字转语音
} audio_chat_mode_t;

esp_err_t app_sr_start(void);
esp_err_t app_sr_stop(void);
BaseType_t app_sr_get_chat_mode(audio_chat_mode_t *chat_mode, TickType_t xTicksToWait);
esp_err_t app_sr_set_chat_mode(audio_chat_mode_t *chat_mode, TickType_t xTicksToWait);

#ifdef __cplusplus
}
#endif