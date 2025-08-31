#ifndef _AUDIO_CODEC_H
#define _AUDIO_CODEC_H

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <driver/i2s_std.h>

#include <vector>
#include <string>
#include <functional>

#include "board.h"

class AudioCodec
{
public:
    AudioCodec();
    virtual ~AudioCodec();

    /**
     * @brief 设置音频编解码器的输出音量
     * @param volume 输出音量的值
     */
    virtual void SetOutputVolume(int volume);

    /**
     * @brief 启用或禁用音频编解码器的输入
     * @param enable 如果为true，启用输入；如果为false，禁用输入
     */
    virtual void EnableInput(bool enable);

    /**
     * @brief 启用或禁用音频编解码器的输出
     * @param enable 如果为true，启用输出；如果为false，禁用输出
     */
    virtual void EnableOutput(bool enable);

    /**
     * @brief 启动音频编解码器
     */
    void Start();

    /**
     * @brief 将音频数据输出到编解码器
     * @param data 包含要输出的音频数据的向量
     */
    void OutputData(std::vector<int16_t> &data);

    /**
     * @brief 从编解码器输入音频数据
     * @param data 用于存储输入音频数据的向量
     * @return 如果成功读取数据返回true，否则返回false
     */
    bool InputData(std::vector<int16_t> &data);

    /**
     * @brief 设置输出数据准备就绪的回调函数
     * @param callback 当输出数据准备就绪时调用的回调函数
     */
    void OnOutputReady(std::function<bool()> callback);

    /**
     * @brief 设置输入数据准备就绪的回调函数
     * @param callback 当输入数据准备就绪时调用的回调函数
     */
    void OnInputReady(std::function<bool()> callback);

    /**
     * @brief 获取音频编解码器是否支持全双工模式
     * @return 如果支持全双工模式返回true，否则返回false
     */
    inline bool duplex() const { return duplex_; }

    /**
     * @brief 获取音频编解码器是否使用输入参考
     * @return 如果使用输入参考返回true，否则返回false
     */
    inline bool input_reference() const { return input_reference_; }

    /**
     * @brief 获取音频编解码器的输入采样率
     * @return 输入采样率的值
     */
    inline int input_sample_rate() const { return input_sample_rate_; }

    /**
     * @brief 获取音频编解码器的输出采样率
     * @return 输出采样率的值
     */
    inline int output_sample_rate() const { return output_sample_rate_; }

    /**
     * @brief 获取音频编解码器的输入通道数
     * @return 输入通道数的值
     */
    inline int input_channels() const { return input_channels_; }

    /**
     * @brief 获取音频编解码器的输出通道数
     * @return 输出通道数的值
     */
    inline int output_channels() const { return output_channels_; }

    /**
     * @brief 获取音频编解码器的输出音量
     * @return 输出音量的值
     */
    inline int output_volume() const { return output_volume_; }

private:
    std::function<bool()> on_input_ready_;
    std::function<bool()> on_output_ready_;

    IRAM_ATTR static bool on_recv(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx);
    IRAM_ATTR static bool on_sent(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx);

protected: // 受保护类型
    i2s_chan_handle_t tx_handle_ = nullptr;
    i2s_chan_handle_t rx_handle_ = nullptr;

    bool duplex_ = false;
    bool input_reference_ = false;
    bool input_enabled_ = false;
    bool output_enabled_ = false;
    int input_sample_rate_ = 0;
    int output_sample_rate_ = 0;
    int input_channels_ = 1;
    int output_channels_ = 1;
    int output_volume_ = 70;

    virtual int Read(int16_t *dest, int samples) = 0;
    virtual int Write(const int16_t *data, int samples) = 0;
};

#endif // _AUDIO_CODEC_H
