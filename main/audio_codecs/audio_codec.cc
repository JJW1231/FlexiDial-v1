#include "audio_codec.h"
#include "board.h"
#include "settings.h"

#include <esp_log.h>
#include <cstring>
#include <driver/i2s_common.h>

#define TAG "AudioCodec"

AudioCodec::AudioCodec() {
}

AudioCodec::~AudioCodec() {
}

void AudioCodec::OnInputReady(std::function<bool()> callback) {
    on_input_ready_ = callback;
}

void AudioCodec::OnOutputReady(std::function<bool()> callback) {
    on_output_ready_ = callback;
}

void AudioCodec::OutputData(std::vector<int16_t>& data) {
    Write(data.data(), data.size());
}

//  * @brief 向音频编码器输入音频数据
//  * @param data[in,out] 指向包含音频样本的缓冲区，返回时包含实际读取的数据
//  * @return 成功读取数据返回true，否则false
//  * @note 输入数据会被调整为固定帧大小 input_frame_size
bool AudioCodec::InputData(std::vector<int16_t>& data) {
    int duration = 30;
    // 计算输入帧大小：采样率/1000 * 毫秒数 * 声道数
    int input_frame_size = input_sample_rate_ / 1000 * duration * input_channels_;

    data.resize(input_frame_size);
    // 从底层设备或缓冲区读取音频数据
    int samples = Read(data.data(), data.size());
    if (samples > 0) {
        return true;
    }
    return false;
}

IRAM_ATTR bool AudioCodec::on_sent(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx) {
    auto audio_codec = (AudioCodec*)user_ctx;
    if (audio_codec->output_enabled_ && audio_codec->on_output_ready_) {
        return audio_codec->on_output_ready_();
    }
    return false;
}

IRAM_ATTR bool AudioCodec::on_recv(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx) {
    auto audio_codec = (AudioCodec*)user_ctx;
    if (audio_codec->input_enabled_ && audio_codec->on_input_ready_) {
        return audio_codec->on_input_ready_();
    }
    return false;
}

void AudioCodec::Start() {
    Settings settings("audio", false);
    output_volume_ = settings.GetInt("output_volume", output_volume_);

    // 注册音频数据回调
    i2s_event_callbacks_t rx_callbacks = {};
    rx_callbacks.on_recv = on_recv;
    i2s_channel_register_event_callback(rx_handle_, &rx_callbacks, this);

    i2s_event_callbacks_t tx_callbacks = {};
    tx_callbacks.on_sent = on_sent;
    i2s_channel_register_event_callback(tx_handle_, &tx_callbacks, this);

    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));

    EnableInput(true);
    EnableOutput(true);
}

void AudioCodec::SetOutputVolume(int volume) {
    output_volume_ = volume;
    ESP_LOGI(TAG, "Set output volume to %d", output_volume_);
    
    Settings settings("audio", true);
    settings.SetInt("output_volume", output_volume_);
}

void AudioCodec::EnableInput(bool enable) {
    if (enable == input_enabled_) {
        return;
    }
    input_enabled_ = enable;
    ESP_LOGI(TAG, "Set input enable to %s", enable ? "true" : "false");
}

void AudioCodec::EnableOutput(bool enable) {
    if (enable == output_enabled_) {
        return;
    }
    output_enabled_ = enable;
    ESP_LOGI(TAG, "Set output enable to %s", enable ? "true" : "false");
}
