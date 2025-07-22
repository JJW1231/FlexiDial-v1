#include "audio.h"
#include "esp_check.h"
#include "unity.h"
#include "audio_player.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "esp_spiffs.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
static const char *TAG = "AUDIO";
#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h"
#include "esp_check.h"
#include "es8311.h"
#include "es7210.h"
#include "app_sr.h"
#include "driver/power/power.h"
#include "app_sr.h"
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
bool record_flag = false;
uint32_t record_total_len = 0;
uint32_t file_total_len = 0;
static uint8_t *record_audio_buffer = NULL;
uint8_t *audio_rx_buffer = NULL;
audio_play_finish_cb_t audio_play_finish_cb = NULL;
static bool g_audio_chat_running = false;
es8311_handle_t es_handle;
// 初始化ES8311编解码芯片
static esp_err_t es8311_codec_init(void)
{
    const i2c_config_t es_i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 100000,
        }};
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "配置I2C失败");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0), TAG, "安装I2C驱动程序失败");
    // 初始化ES8311编解码芯片
    es_handle = es8311_create(I2C_NUM, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "创建ES8311句柄失败");
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
        .sample_frequency = EXAMPLE_SAMPLE_RATE};
    ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE, EXAMPLE_SAMPLE_RATE), TAG, "设置ES8311采样率失败");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "设置ES8311音量失败");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "配置ES8311麦克风失败");

    return ESP_OK;
}

// 初始化ES7210编解码芯片
static esp_err_t es7210_codec_init(void)
{
    es7210_dev_handle_t es7210_handle = NULL;
    es7210_i2c_config_t es7210_i2c_conf = {
        .i2c_port = I2C_NUM,
        .i2c_addr = EXAMPLE_ES7210_I2C_ADDR};
    ESP_ERROR_CHECK(es7210_new_codec(&es7210_i2c_conf, &es7210_handle));

    ESP_LOGI(TAG, "Configure ES7210 codec parameters");
    es7210_codec_config_t codec_conf = {

        .sample_rate_hz = EXAMPLE_I2S_SAMPLE_RATE,
        .mclk_ratio = EXAMPLE_I2S_MCLK_MULTIPLE,
        .i2s_format = EXAMPLE_I2S_TDM_FORMAT,
        .bit_width = (es7210_i2s_bits_t)EXAMPLE_I2S_SAMPLE_BITS,
        .mic_bias = EXAMPLE_ES7210_MIC_BIAS,
        .mic_gain = EXAMPLE_ES7210_MIC_GAIN,
        .flags = {.tdm_enable = true}};
    ESP_ERROR_CHECK(es7210_config_codec(es7210_handle, &codec_conf));
    ESP_ERROR_CHECK(es7210_config_volume(es7210_handle, EXAMPLE_ES7210_ADC_VOLUME));

    return ESP_OK;
}
// 初始化I2S驱动程序
static esp_err_t i2s_driver_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // 自动清除DMA缓冲区中的遗留数据
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle));
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;
    // 初始化I2S标准模式
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));

    return ESP_OK;
}
// 初始化音频系统
void audio_init()
{
    i2s_driver_init();
    es8311_codec_init();
    es7210_codec_init();

    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;

    FILE *fp = fopen("/spiffs/audio/powerOn.wav", "r");
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET); // 将文件指针移回文件开头
    uint8_t *data_ptr = (uint8_t *)malloc(file_size);
    size_t bytes_read = fread(data_ptr, 1, file_size, fp);
    ESP_ERROR_CHECK(i2s_channel_preload_data(tx_handle, fp, bytes_read, &bytes_write));
    // 关闭文件
    fclose(fp);

    // 启用I2S通道
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    i2s_channel_write(tx_handle, data_ptr, bytes_read, &bytes_write, portMAX_DELAY);

    // while (1)
    // {
    //     ret = i2s_channel_write(tx_handle, data_ptr, bytes_read, &bytes_write, portMAX_DELAY);
    //     if (bytes_write > 0)
    //     {
    //         ESP_LOGI(TAG, "[music]播放音乐,写入字节 %d 位.", bytes_write);
    //     }
    //     else
    //     {
    //         ESP_LOGE(TAG, "[music]音乐播放失败了。");
    //     }
    //     i2s_channel_read(rx_handle, data_ptr, bytes_read, &bytes_write, portMAX_DELAY);
    //     // //打印收到的音频数据
    //     // for (int i = 0; i < bytes_read; i++)
    //     // {
    //     //     printf("%d ", data_ptr[i]);
    //     //     vTaskDelay(1);
    //     // }
    //     // printf("\n");
    //     vTaskDelay(300 / portTICK_PERIOD_MS);
    // }
}

static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;

esp_err_t bsp_extra_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = i2s_channel_read(rx_handle, audio_buffer, len, bytes_read, portMAX_DELAY);
    // ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    *bytes_read = len;
    return ret;
}

esp_err_t bsp_extra_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = i2s_channel_write(tx_handle, audio_buffer, len, bytes_written, portMAX_DELAY);
    // ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

void audio_record_save(int16_t *audio_buffer, int audio_chunksize)
{
#if DEBUG_SAVE_PCM
    if (record_flag)
    {
        uint16_t *record_buff = (uint16_t *)(record_audio_buffer + sizeof(wav_header_t));
        record_buff += record_total_len;
        for (int i = 0; i < (audio_chunksize - 1); i++)
        {
            if (record_total_len < (MAX_FILE_SIZE - sizeof(wav_header_t)) / 2)
            {
#if PCM_ONE_CHANNEL
                record_buff[i * 1 + 0] = audio_buffer[i * 3 + 0];
                record_total_len += 1;
#else
                record_buff[i * 2 + 0] = audio_buffer[i * 3 + 0];
                record_buff[i * 2 + 1] = audio_buffer[i * 3 + 1];
                record_total_len += 2;
#endif
            }
        }
    }
#endif
}

// 播放唤醒词音频数据
void play_audio(const char *__restrict _name)
{
    size_t bytes_write = 0;
    FILE *fp = fopen(_name, "r");
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET); // 将文件指针移回文件开头
    uint8_t *data_ptr = (uint8_t *)malloc(file_size);
    size_t bytes_read = fread(data_ptr, 1, file_size, fp);
    fclose(fp);
    i2s_channel_write(tx_handle, data_ptr, bytes_read, &bytes_write, portMAX_DELAY);
    free(data_ptr);
}
QueueHandle_t audioQueue = NULL;

// 处理sr（语音识别）结果的任务函数
void sr_handler_task(void *pvParam)
{
    sr_result_t result;
    while (true)
    {
        if (xQueueReceive(audioQueue, &result, portMAX_DELAY) == pdTRUE)
        {

            ESP_LOGI(TAG, "cmd:%d, wakemode:%d,state:%d", result.command_id, result.wakenet_mode, result.state);

            switch (result.state)
            {
            case ESP_MN_STATE_DETECTING:
                play_audio("/spiffs/audio/echo_cn_wake.wav");
                break;
            case ESP_MN_STATE_TIMEOUT:
                play_audio("/spiffs/audio/echo_cn_end.wav");
                break;
            case ESP_MN_STATE_DETECTED:
                ESP_LOGI(TAG, "检测到命令词");
                switch (result.command_id)
                {
                case 0:
                    play_audio("/spiffs/audio/powerOff.wav");
                    vTaskDelay(40);
                    power_set(0);
                    break;
                case 6:
                    RGB_power_set(1);
                    break;
                case 7:
                    RGB_power_set(0);
                    break;
                default:
                    break;
                }
                play_audio("/spiffs/audio/echo_cn_ok.wav");
                break;
            default:
                break;
            }
        }
    }
    vTaskDelete(NULL);
}
enum
{
    GBK_TASK_IDLE = 0,
    GBK_HEX_TO_NUMPAD,
    GBK_NUMPAD_PRESSED,
    GBK_NUMPAD_RELEASE,
    GBK_ALTKEY_PRESSED,
    GBK_ALTKEY_RELEASE,
};
#define MAX_BUFFER_SIZE (1024 * 4)
static unsigned int *gbk_hex_buffer = NULL;
static int gbk_hex_count = 0;
static int gbk_hex_index = 0;
static uint8_t gbk_hid_buffer[10] = {0};
static int gbk_hid_count = 0;
static int gbk_hid_index = 0;
static uint8_t gbk_hid_state = GBK_TASK_IDLE;

/**
 * @brief  Convert a UTF-8 string to GBK string , GBK string is must big enough to contain the result.
 * @param  * utf: UTF-8 string pointer for converting.
 * @param  * GBK: GBK string pointer for store the result.
 * @retval Number of character is converted.
 */
int utf82gbk(char *gb, int gb_size, char *utf)
{
    if (gb == NULL || utf == NULL || gb_size == 0)
    {
        return -1;
    }

    int outputSize = 0; // 记录转换后的Unicode字符串的字节数
    int remaining_size = gb_size;
    unsigned short unicode;

    while (*utf)
    {
        if (remaining_size < 2)
        {
            return -2; // 缓冲区不足
        }

        if (*utf > 0x00 && *utf < 0x80) // 处理单字节UTF8字符（英文字母、数字）
        {
            *gb = *utf;
            utf++;
            gb++;
            remaining_size -= 1;
            outputSize += 1;
        }
        else if (((*utf) & 0xE0) == 0xC0) // 处理双字节UTF8字符
        {
            unsigned short high = *utf;
            utf++;
            unsigned short low = *utf;
            utf++;
            if ((low & 0xC0) != 0x80) // 检查是否为合法的UTF8字符表示
            {
                return -1; // 如果不是则报错
            }
            unicode = ((high & 0x1F) << 6) + (low & 0x3F); // 取出high的低5位与low的低6位，组合成unicode字符的低8位
            // *(unsigned short *)gb = getgb(unicode);        // get gbk from unicode
            gb += 2;
            remaining_size -= 2;
            outputSize += 2;
        }
        else if (((*utf) & 0xF0) == 0xE0) // 处理三字节UTF8字符
        {
            unsigned short high = *utf;
            utf++;
            unsigned short middle = *utf;
            utf++;
            unsigned short low = *utf;
            utf++;
            if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80))
            {
                return -1;
            }
            unicode = ((high & 0x0F) << 12) + ((middle & 0x3F) << 6) + (low & 0x3F); // 取出high的低4位，middle的低6位和low的低6位，组合成unicode
            // *(unsigned short *)gb = getgb(unicode);                                  // get gbk from unicode
            gb += 2;
            remaining_size -= 2;
            outputSize += 2;
        }
        else // 对于其他字节数的UTF8字符不进行处理
        {
            return -1;
        }
    }

    // gbk字符串后面，有1个\0
    if (remaining_size < 1)
    {
        return -2; // 缓冲区不足
    }
    *gb = '\0';
    gb++;
    return outputSize;
}

void gbkStrToHex(char *gbk_str, int gbk_str_len)
{
    gbk_hex_index = 0;
    gbk_hex_count = 0;

    int tmpGBKSize = MAX_BUFFER_SIZE * sizeof(char);
    char *tmpGBK = (char *)heap_caps_malloc((const int)tmpGBKSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(tmpGBK);
    memset(tmpGBK, '\0', MAX_BUFFER_SIZE * sizeof(char));

    int tempUTF8Size = gbk_str_len + 1;
    char *tempUTF8 = (char *)heap_caps_malloc((const int)tempUTF8Size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(tempUTF8);
    memset(tempUTF8, '\0', tempUTF8Size);
    sprintf(tempUTF8, "%s", gbk_str);
    int gbk_len = utf82gbk(tmpGBK, tmpGBKSize, tempUTF8);
    free(tempUTF8);

    if (gbk_hex_buffer == NULL)
    {
        gbk_hex_buffer = (unsigned int *)heap_caps_malloc(MAX_BUFFER_SIZE * sizeof(unsigned int), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    assert(gbk_hex_buffer);

    memset(gbk_hex_buffer, 0, sizeof(gbk_hex_buffer));
    for (int i = 0; i < gbk_len; i++)
    {
        if (i >= tmpGBKSize)
        {
            break;
        }

        if (gbk_hex_count >= MAX_BUFFER_SIZE)
        {
            break;
        }

        int c1 = tmpGBK[i];
        if (c1 > 0x7F)
        {
            // 双字节字符
            if ((i + 1) >= tmpGBKSize)
            {
                break;
            }

            int c2 = tmpGBK[i + 1];
            int gbk_hex = (c1 << 8) | c2;
            gbk_hex_buffer[gbk_hex_count++] = gbk_hex;
            printf("%04X ", gbk_hex);
            i++;
        }
        else
        {
            // 单字节字符（ASCII）
            gbk_hex_buffer[gbk_hex_count++] = c1;
            printf("%02X ", c1);
        }
    }
    printf("\r\n");
    free(tmpGBK);
    gbk_hid_state = GBK_HEX_TO_NUMPAD;
}

// static uint8_t *record_audio_buffer = NULL;
// uint8_t *audio_rx_buffer = NULL;

// static void audio_player_cb(audio_player_cb_ctx_t *ctx)
// {
//     switch (ctx->audio_event)
//     {
//     case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
//         ESP_LOGI(TAG, "Player IDLE");
//         bsp_codec_set_fs(16000, 16, 2);
//         if (audio_play_finish_cb)
//         {
//             audio_play_finish_cb();
//         }
//         break;
//     case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
//         ESP_LOGI(TAG, "Player NEXT");
//         break;
//     case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
//         ESP_LOGI(TAG, "Player PLAYING");
//         break;
//     case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
//         ESP_LOGI(TAG, "Player PAUSE");
//         break;
//     case AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN:
//         ESP_LOGI(TAG, "Player SHUTDOWN");
//         break;
//     default:
//         break;
//     }
// }

// void audio_record_init()
// {
// #if DEBUG_SAVE_PCM
//     // 分配录音缓存
//     record_audio_buffer = heap_caps_calloc(1, RECORD_FILE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
//     assert(record_audio_buffer);
//     printf("successfully created record_audio_buffer with a size: %zu\r\n", RECORD_FILE_SIZE);
//     // 分配语音合成缓存
//     audio_rx_buffer = heap_caps_calloc(1, MAX_FILE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
//     assert(audio_rx_buffer);
//     printf("successfully created audio_rx_buffer with a size: %zu\r\n", MAX_FILE_SIZE);
// #endif

//     if (record_audio_buffer == NULL || audio_rx_buffer == NULL)
//     {
//         printf("Error: Failed to allocate memory for buffers\r\n");
//         return;
//     }

//     file_iterator_instance_t *file_iterator = file_iterator_new(BSP_SPIFFS_MOUNT_POINT);
//     assert(file_iterator != NULL);

//     audio_player_config_t config = {
//         .mute_fn = audio_mute_function,
//         .clk_set_fn = audio_codec_set_fs,
//         .write_fn = bsp_i2s_write,
//         .priority = 5,
//     };
//     ESP_ERROR_CHECK(audio_player_new(config));
//     audio_player_callback_register(audio_player_cb, NULL);
//     ESP_LOGI(TAG, "audio_record_init -----> Audio player initialized");
// }