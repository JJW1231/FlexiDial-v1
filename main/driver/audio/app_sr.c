/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_mn_iface.h"
#include "model_path.h"

#include "app_sr.h"
#include "web_socket.h"
#include "audio.h"
#include "ui/events/events.h"
extern QueueHandle_t Dial_Queue;
extern QueueHandle_t audioQueue;

static const char *TAG = "app_sr";

#define I2S_CHANNEL_NUM 2

static esp_afe_sr_iface_t *afe_handle = NULL;
static srmodel_list_t *models = NULL;
static bool manul_detect_flag = false;
sr_data_t *g_sr_data = NULL;

static QueueHandle_t g_audio_chat_mode_que = NULL;

static model_iface_data_t *model_data = NULL;
static const esp_mn_iface_t *multinet = NULL;
const char *cmd_phoneme[12] = {
    "guan ji",
    "guan bi kong qi jing hua qi",
    "da kai tai deng",
    "guan bi tai deng",
    "tai deng tiao liang",
    "tai deng tiao an",
    "da kai deng dai",
    "guan bi deng dai",
    "bo fang yin yue",
    "ting zhi bo fang",
    "da kai shi jian",
    "da kai ri li"};

static void audio_feed_task(void *arg)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int feed_channel = afe_handle->get_feed_channel_num(afe_data); // 3;
    ESP_LOGI(TAG, "audio_chunksize = %d, feed_channel = %d", audio_chunksize, feed_channel);
    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * feed_channel, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(audio_buffer);
    g_sr_data->afe_in_buffer = audio_buffer;

    while (true)
    {
        /* Read audio data from I2S bus */
        bsp_extra_i2s_read((char *)audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        // AFE需要3通道数据, 将第3通道（参考回路）置0
        // 如不需要AEC功能, 只需两通道mic数据即可
        for (int i = audio_chunksize - 1; i >= 0; i--)
        {
            audio_buffer[i * 3 + 0] = audio_buffer[i * 2 + 0]; // mic_l
            audio_buffer[i * 3 + 1] = audio_buffer[i * 2 + 1]; // mic_r
            audio_buffer[i * 3 + 2] = 0;                       // ref
        }

        /* Feed samples of an audio stream to the AFE_SR */
        afe_handle->feed(afe_data, audio_buffer);

        // 保存音频数据
        audio_record_save(audio_buffer, audio_chunksize);

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    afe_handle->destroy(afe_data);
    vTaskDelete(NULL);
}

static void audio_detect_task(void *arg)
{
    static vad_state_t local_state;
    static uint8_t frame_keep = 0;

    bool detect_flag = false;
    esp_afe_sr_data_t *afe_data = arg;
    while (true)
    {
        // 从AFE获取数据
        // fetch 内部可以进行 VAD 处理并检测唤醒词等动作
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL)
        {
            ESP_LOGW(TAG, "AFE Fetch Fail");
            continue;
        }

        // -------------------------------------------------------------------------------
        // 按下按键开始录音
        // if (getRecKey())
        // {
        //     if (!manul_detect_flag)
        //     {
        //         detect_flag = false;
        //         frame_keep = 0;
        //         manul_detect_flag = true;
        //         g_sr_data->afe_handle->disable_wakenet(afe_data);
        //         sr_result_t result = {
        //             .wakenet_mode = WAKENET_DETECTED,
        //             .state = ESP_MN_STATE_DETECTING,
        //             .command_id = 0x55,
        //         };
        //         app_sr_set_result(&result, 0);
        //         ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_RED) "manual detect");
        //     }
        //     continue;
        // }
        // else
        // {
        if (manul_detect_flag)
        {
            detect_flag = false;
            frame_keep = 0;
            manul_detect_flag = false;
            sr_result_t result = {
                .wakenet_mode = WAKENET_NO_DETECT,
                .state = ESP_MN_STATE_DETECTED,
                .command_id = 0x55, // 先随便用一下, 后面再改
            };
            // app_sr_set_result(&result, 0);
            xQueueSend(audioQueue, &result, 0);
            g_sr_data->afe_handle->enable_wakenet(afe_data);
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_RED) "manual detect end");
            continue;
        }
        // }
        // -------------------------------------------------------------------------------

        // 如果检测到唤醒词, 将结果通过消息队列发送给处理任务
        if (res->wakeup_state == WAKENET_DETECTED)
        {
            sr_result_t result = {
                .wakenet_mode = WAKENET_DETECTED,
                .state = ESP_MN_STATE_DETECTING,
                .command_id = 0,
            };
            // app_sr_set_result(&result, 0);
            xQueueSend(audioQueue, &result, 0);
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_PURPLE) "识别到唤醒词");
        }
        else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED)
        {
            frame_keep = 0;
            detect_flag = true;                               // 使能VAD检测
            g_sr_data->afe_handle->disable_wakenet(afe_data); // 关闭唤醒词检测
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "AFE_FETCH_CHANNEL_VERIFIED, channel index: %d\n", res->trigger_channel_id);
        }

        if (true == detect_flag)
        {

            esp_mn_state_t mn_state = ESP_MN_STATE_DETECTING;

            mn_state = multinet->detect(model_data, res->data);

            if (ESP_MN_STATE_DETECTING == mn_state)
            {
                continue;
            }
            if (ESP_MN_STATE_DETECTED == mn_state)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++)
                {
                    ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, prob: %f",
                             i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->prob[i]);
                }

                int sr_command_id = mn_result->command_id[0];
                ESP_LOGI(TAG, "Deteted command : %d", sr_command_id);
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = mn_state,
                    .command_id = sr_command_id,
                };
                xQueueSend(audioQueue, &result, 10);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }

            if (local_state != res->vad_state)
            {
                local_state = res->vad_state;
                frame_keep = 0;
            }
            else
            {
                frame_keep++;
            }

            // 如果连续50帧没有检测到人声, 则检测结束, 通知处理任务处理结果
            if ((50 == frame_keep) && (VAD_SILENCE == res->vad_state))
            {
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = ESP_MN_STATE_TIMEOUT,
                    .command_id = 0,
                };
                // app_sr_set_result(&result, 0);
                g_sr_data->afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                xQueueSend(audioQueue, &result, 0);
                ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_PURPLE) "唤醒超时");
            }
        }
    }
    afe_handle->destroy(afe_data);
    vTaskDelete(NULL);
}

#define CHAT_TASK_STACK_SIZE (10 * 1024)
static StaticTask_t xChatTaskBuffer;
static StackType_t *xChatTaskStack;
esp_err_t app_sr_start(void)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(NULL == g_sr_data, ESP_ERR_INVALID_STATE, TAG, "SR already running");

    g_sr_data = heap_caps_calloc(1, sizeof(sr_data_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(NULL != g_sr_data, ESP_ERR_NO_MEM, TAG, "Failed create sr data");

    g_sr_data->result_que = xQueueCreate(3, sizeof(sr_result_t));
    ESP_GOTO_ON_FALSE(NULL != g_sr_data->result_que, ESP_ERR_NO_MEM, err, TAG, "Failed create result queue");

    // g_audio_chat_mode_que = xQueueCreate(1, sizeof(audio_chat_mode_t));
    // ESP_GOTO_ON_FALSE(NULL != g_audio_chat_mode_que, ESP_ERR_NO_MEM, err, TAG, "Failed create audio chat mode queue");

    BaseType_t ret_val;
    models = esp_srmodel_init("model");

    afe_config_t *afe_config = afe_config_init("MMR", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_handle = esp_afe_handle_from_config(afe_config);
    afe_config->wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    afe_config->aec_init = false; // aec不使能：BSS/NS 算法处理在 feed() 中进行

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    g_sr_data->afe_handle = afe_handle;
    g_sr_data->afe_data = afe_data;

    g_sr_data->lang = SR_LANG_MAX;
    char *mn_name = esp_srmodel_filter(models, ESP_MN_CHINESE, NULL);
    if (NULL == mn_name)
    {
        ESP_LOGE(TAG, "No multinet model found");
        return ESP_FAIL;
    }

    multinet = esp_mn_handle_from_name(mn_name);
    model_data = multinet->create(mn_name, 5760);
    ESP_LOGI(TAG, "load multinet:%s", mn_name);

    esp_mn_commands_clear();

    for (int i = 0; i < sizeof(cmd_phoneme) / sizeof(cmd_phoneme[0]); i++)
    {
        esp_mn_commands_add(i, (char *)cmd_phoneme[i]);
    }

    esp_mn_commands_update();
    esp_mn_commands_print();
    multinet->print_active_speech_commands(model_data);

    // 采集音频数据
    ret_val = xTaskCreatePinnedToCore(&audio_feed_task, "Feed Task", 6 * 1024, (void *)afe_data, 5, &g_sr_data->feed_task, 0);
    ESP_GOTO_ON_FALSE(pdPASS == ret_val, ESP_FAIL, err, TAG, "Failed create audio feed task");
    ESP_LOGI(TAG, "音频采集任务启动成功");
    // 音频检测
    ret_val = xTaskCreatePinnedToCore(&audio_detect_task, "Detect Task", 6 * 1024, (void *)afe_data, 5, &g_sr_data->detect_task, 1);
    ESP_GOTO_ON_FALSE(pdPASS == ret_val, ESP_FAIL, err, TAG, "Failed create audio detect task");
    ESP_LOGI(TAG, "音频检测任务启动成功");
    // 语音处理任务
    audioQueue = xQueueCreate(4, sizeof(sr_result_t));
    xTaskCreatePinnedToCore(&sr_handler_task, "SR Handler Task", 3 * 1024, NULL, 4, NULL, 1);
    ESP_LOGI(TAG, "语音处理任务启动成功");
    return ESP_OK;
err:
    app_sr_stop();
    return ret;
}

esp_err_t app_sr_stop(void)
{
    ESP_RETURN_ON_FALSE(NULL != g_sr_data, ESP_ERR_INVALID_STATE, TAG, "SR is not running");

    if (g_sr_data->result_que)
    {
        vQueueDelete(g_sr_data->result_que);
        g_sr_data->result_que = NULL;
    }

    if (g_sr_data->fp)
    {
        fclose(g_sr_data->fp);
        g_sr_data->fp = NULL;
    }

    if (g_sr_data->model_data)
    {
        g_sr_data->multinet->destroy(g_sr_data->model_data);
    }

    if (g_sr_data->afe_data)
    {
        g_sr_data->afe_handle->destroy(g_sr_data->afe_data);
    }

    if (g_sr_data->afe_in_buffer)
    {
        heap_caps_free(g_sr_data->afe_in_buffer);
    }

    if (g_sr_data->afe_out_buffer)
    {
        heap_caps_free(g_sr_data->afe_out_buffer);
    }

    heap_caps_free(g_sr_data);
    g_sr_data = NULL;
    return ESP_OK;
}

BaseType_t app_sr_get_chat_mode(audio_chat_mode_t *chat_mode, TickType_t xTicksToWait)
{
    ESP_RETURN_ON_FALSE(NULL != g_audio_chat_mode_que, ESP_ERR_INVALID_STATE, TAG, "SR is not running");
    return xQueueReceive(g_audio_chat_mode_que, chat_mode, xTicksToWait);
}

esp_err_t app_sr_set_chat_mode(audio_chat_mode_t *chat_mode, TickType_t xTicksToWait)
{
    ESP_RETURN_ON_FALSE(NULL != g_audio_chat_mode_que, ESP_ERR_INVALID_STATE, TAG, "SR is not running");
    xQueueSend(g_audio_chat_mode_que, chat_mode, xTicksToWait);
    return ESP_OK;
}
