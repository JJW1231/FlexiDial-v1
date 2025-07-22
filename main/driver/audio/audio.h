#pragma once

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define DEBUG_SAVE_PCM      (1)
#define PCM_ONE_CHANNEL     (1)
#define RECORD_FILE_SIZE    (1 * 1024 * 1024)
#define MAX_FILE_SIZE       (1 * 1024 * 1024)
/* Example configurations */
#define EXAMPLE_RECV_BUF_SIZE (2400)
#define EXAMPLE_SAMPLE_RATE (16000)
#define EXAMPLE_MCLK_MULTIPLE (I2S_MCLK_MULTIPLE_384) // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
// #define EXAMPLE_VOICE_VOLUME 94
#define EXAMPLE_VOICE_VOLUME 80

/* I2C port and GPIOs */
#define I2C_NUM (I2C_NUM_0)
#define I2C_SCL_IO (GPIO_NUM_42)
#define I2C_SDA_IO (GPIO_NUM_45)

/* I2S port and GPIOs */
#define I2S_NUM (I2S_NUM_0)
#define I2S_MCK_IO (GPIO_NUM_8)
#define I2S_BCK_IO (GPIO_NUM_16)
#define I2S_WS_IO (GPIO_NUM_15)
#define I2S_DO_IO (GPIO_NUM_7)
#define I2S_DI_IO (GPIO_NUM_14)

/* I2S configurations */
#define EXAMPLE_I2S_TDM_FORMAT     (ES7210_I2S_FMT_I2S)
#define EXAMPLE_I2S_CHAN_NUM       (4)
#define EXAMPLE_I2S_SAMPLE_RATE    (48000)
#define EXAMPLE_I2S_MCLK_MULTIPLE  (I2S_MCLK_MULTIPLE_256)
#define EXAMPLE_I2S_SAMPLE_BITS    (I2S_DATA_BIT_WIDTH_16BIT)
#define EXAMPLE_I2S_TDM_SLOT_MASK  (I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3)

/* ES7210 configurations */
#define EXAMPLE_ES7210_I2C_ADDR    (0x40)
#define EXAMPLE_ES7210_I2C_CLK     (100000)
#define EXAMPLE_ES7210_MIC_GAIN    (ES7210_MIC_GAIN_30DB)
#define EXAMPLE_ES7210_MIC_BIAS    (ES7210_MIC_BIAS_2V87)
#define EXAMPLE_ES7210_ADC_VOLUME  (0)

#define NEED_DELETE    BIT0
#define FEED_DELETED   BIT1
#define DETECT_DELETED BIT2
#define HANDLE_DELETED BIT3


typedef struct {
    // The "RIFF" chunk descriptor
    uint8_t ChunkID[4];// Indicates the file as "RIFF" file
    int32_t ChunkSize;// The total size of the entire file, excluding the "RIFF" and the header itself, which is the file size minus 8 bytes.
    uint8_t Format[4];// File format header indicating a "WAVE" file.
    // The "fmt" sub-chunk
    uint8_t Subchunk1ID[4];// Format identifier for the "fmt" sub-chunk.
    int32_t Subchunk1Size;// The length of the fmt sub-chunk (subchunk1) excluding the Subchunk1 ID and Subchunk1 Size fields. It is typically 16, but a value greater than 16 indicates the presence of an extended area. Optional values for the length include 16, 18, 20, 40, etc.
    int16_t AudioFormat;// Audio encoding format, which represents the compression format. A value of 0x01 indicates PCM format, which is uncompressed. Please refer to table 3 for more details.
    int16_t NumChannels;// Number of audio channels
    int32_t SampleRate;// Sample rate, for example, "44100" represents a sampling rate of 44100 Hz.
    int32_t ByteRate;// Bit rate: Sample rate x bit depth x number of channels / 8. For example, the bit rate for a stereo (2 channels) audio with a sample rate of 44.1 kHz and 16-bit depth would be 176400 bits per second.
    int16_t BlockAlign;// Memory size occupied by one sample: Bit depth x number of channels / 8.
    int16_t BitsPerSample;//Sample depth, also known as bit depth.
    // The "data" sub-chunk
    uint8_t Subchunk2ID[4];// Total length of the audio data, which is the file size minus the length of the WAV file header.
    int32_t Subchunk2Size;// Length of the data section, referring to the size of the audio data excluding the header.
} wav_header_t;
typedef void (*audio_play_finish_cb_t)(void);
extern uint8_t *audio_rx_buffer;
void audio_init();
void sr_handler_task(void *pvParam);
esp_err_t bsp_extra_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);
esp_err_t bsp_extra_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);
void audio_record_save(int16_t *audio_buffer, int audio_chunksize);


#ifdef __cplusplus
}
#endif