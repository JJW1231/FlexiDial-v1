#include "application.h"
#include "board.h"
#include "display.h"
#include "system_info.h"
#include "ml307_ssl_transport.h"
#include "audio_codec.h"
#include "mqtt_protocol.h"
#include "websocket_protocol.h"
#include "font_awesome_symbols.h"
#include "assets/lang_config.h"

#include <cstring>
#include <esp_log.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <arpa/inet.h>
#include <esp_app_desc.h>

#define TAG "Application"

static const char *const STATE_STRINGS[] = {
    "unknown",
    "starting",
    "configuring",
    "idle",
    "connecting",
    "listening",
    "speaking",
    "upgrading",
    "activating",
    "fatal_error",
    "invalid_state"};

Application::Application()
{
    event_group_ = xEventGroupCreate();
    background_task_ = new BackgroundTask(4096 * 8);

    esp_timer_create_args_t clock_timer_args = {
        .callback = [](void *arg)
        {
            Application *app = (Application *)arg;
            app->OnClockTimer();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "clock_timer",
        .skip_unhandled_events = true};
    esp_timer_create(&clock_timer_args, &clock_timer_handle_);
}

Application::~Application()
{
    if (clock_timer_handle_ != nullptr)
    {
        esp_timer_stop(clock_timer_handle_);
        esp_timer_delete(clock_timer_handle_);
    }
    if (background_task_ != nullptr)
    {
        delete background_task_;
    }
    vEventGroupDelete(event_group_);
}

void Application::CheckNewVersion()
{
    auto &board = Board::GetInstance();
    auto display = board.GetDisplay();
    // 检查是否有新的固件版本可用
    ota_.SetPostData(board.GetJson());

    const int MAX_RETRY = 10;
    int retry_count = 0;

    while (true)
    {
        if (!ota_.CheckVersion())
        {
            retry_count++;
            if (retry_count >= MAX_RETRY)
            {
                ESP_LOGE(TAG, "Too many retries, exit version check");
                return;
            }
            ESP_LOGW(TAG, "Check new version failed, retry in %d seconds (%d/%d)", 60, retry_count, MAX_RETRY);
            vTaskDelay(pdMS_TO_TICKS(60000));
            continue;
        }
        retry_count = 0;

        // 没有新版本，请将当前版本标记为有效
        ota_.MarkCurrentVersionValid();
        std::string message = std::string(Lang::Strings::VERSION) + ota_.GetCurrentVersion();
        display->ShowNotification(message.c_str());

        if (ota_.HasActivationCode())
        {
            // Activation code is valid
            SetDeviceState(kDeviceStateActivating);
            ShowActivationCode(); // 显示激活码信息并播放语音提示

            // Check again in 60 seconds or until the device is idle
            for (int i = 0; i < 60; ++i)
            {
                if (device_state_ == kDeviceStateIdle)
                {
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            continue;
        }

        SetDeviceState(kDeviceStateIdle);
        display->SetChatMessage("system", "");
        PlaySound(Lang::Sounds::P3_SUCCESS);
        // 如果升级或空闲，退出循环
        break;
    }
}
/**
 * 显示激活码信息
 *
 * 该函数用于显示设备激活信息，并播放对应的语音提示。
 * 主要流程包括获取激活信息、构建数字与语音的映射关系、显示激活提示并播放每个数字对应的音频。
 */
void Application::ShowActivationCode()
{
    // 获取激活信息和激活码
    auto &message = ota_.GetActivationMessage();
    auto &code = ota_.GetActivationCode();

    // 定义一个结构体，用于将数字字符映射到对应的语音文件
    struct digit_sound
    {
        char digit;                    // 数字字符
        const std::string_view &sound; // 对应的语音资源
    };

    // 初始化数字与语音的映射表
    static const std::array<digit_sound, 10> digit_sounds{{digit_sound{'0', Lang::Sounds::P3_0},
                                                           digit_sound{'1', Lang::Sounds::P3_1},
                                                           digit_sound{'2', Lang::Sounds::P3_2},
                                                           digit_sound{'3', Lang::Sounds::P3_3},
                                                           digit_sound{'4', Lang::Sounds::P3_4},
                                                           digit_sound{'5', Lang::Sounds::P3_5},
                                                           digit_sound{'6', Lang::Sounds::P3_6},
                                                           digit_sound{'7', Lang::Sounds::P3_7},
                                                           digit_sound{'8', Lang::Sounds::P3_8},
                                                           digit_sound{'9', Lang::Sounds::P3_9}}};

    // 显示激活提示信息，并播放整体的激活语音（占用较大内存）
    Alert(Lang::Strings::ACTIVATION, message.c_str(), "happy", Lang::Sounds::P3_ACTIVATION);
    vTaskDelay(pdMS_TO_TICKS(1000));       // 等待一秒确保语音播放完成
    background_task_->WaitForCompletion(); // 等待后台任务完成

    // 遍历激活码中的每一个数字字符
    for (const auto &digit : code)
    {
        // 查找对应的语音资源
        auto it = std::find_if(digit_sounds.begin(), digit_sounds.end(),
                               [digit](const digit_sound &ds)
                               { return ds.digit == digit; });

        // 如果找到对应语音，则播放它
        if (it != digit_sounds.end())
        {
            PlaySound(it->sound);
        }
    }
}
void Application::Alert(const char *status, const char *message, const char *emotion, const std::string_view &sound)
{
    ESP_LOGW(TAG, "Alert %s: %s [%s]", status, message, emotion);
    auto display = Board::GetInstance().GetDisplay();
    display->SetStatus(status);
    display->SetEmotion(emotion);
    display->SetChatMessage("system", message);
    if (!sound.empty())
    {
        PlaySound(sound);
    }
}

void Application::DismissAlert()
{
    if (device_state_ == kDeviceStateIdle)
    {
        auto display = Board::GetInstance().GetDisplay();
        display->SetStatus(Lang::Strings::STANDBY);
        display->SetEmotion("neutral");
        display->SetChatMessage("system", "");
    }
}

void Application::PlaySound(const std::string_view &sound)
{
    auto codec = Board::GetInstance().GetAudioCodec();
    codec->EnableOutput(true);
    SetDecodeSampleRate(16000);
    const char *data = sound.data();
    size_t size = sound.size();
    for (const char *p = data; p < data + size;)
    {
        auto p3 = (BinaryProtocol3 *)p;
        p += sizeof(BinaryProtocol3);

        auto payload_size = ntohs(p3->payload_size);
        std::vector<uint8_t> opus;
        opus.resize(payload_size);
        memcpy(opus.data(), p3->payload, payload_size);
        p += payload_size;

        std::lock_guard<std::mutex> lock(mutex_);
        audio_decode_queue_.emplace_back(std::move(opus));
    }
}

void Application::ToggleChatState()
{
    if (device_state_ == kDeviceStateActivating)
    {
        SetDeviceState(kDeviceStateIdle);
        return;
    }

    if (!protocol_)
    {
        ESP_LOGE(TAG, "Protocol not initialized");
        return;
    }

    if (device_state_ == kDeviceStateIdle)
    {
        Schedule([this]()
                 {
            SetDeviceState(kDeviceStateConnecting);
            if (!protocol_->OpenAudioChannel()) {
                return;
            }

            keep_listening_ = true;
            protocol_->SendStartListening(kListeningModeAutoStop);
            SetDeviceState(kDeviceStateListening); });
    }
    else if (device_state_ == kDeviceStateSpeaking)
    {
        Schedule([this]()
                 { AbortSpeaking(kAbortReasonNone); });
    }
    else if (device_state_ == kDeviceStateListening)
    {
        Schedule([this]()
                 { protocol_->CloseAudioChannel(); });
    }
}

void Application::StartListening()
{
    if (device_state_ == kDeviceStateActivating)
    {
        SetDeviceState(kDeviceStateIdle);
        return;
    }

    if (!protocol_)
    {
        ESP_LOGE(TAG, "Protocol not initialized");
        return;
    }

    keep_listening_ = false;
    if (device_state_ == kDeviceStateIdle)
    {
        Schedule([this]()
                 {
            if (!protocol_->IsAudioChannelOpened()) {
                SetDeviceState(kDeviceStateConnecting);
                if (!protocol_->OpenAudioChannel()) {
                    return;
                }
            }
            protocol_->SendStartListening(kListeningModeManualStop);
            SetDeviceState(kDeviceStateListening); });
    }
    else if (device_state_ == kDeviceStateSpeaking)
    {
        Schedule([this]()
                 {
            AbortSpeaking(kAbortReasonNone);
            protocol_->SendStartListening(kListeningModeManualStop);
            SetDeviceState(kDeviceStateListening); });
    }
}

void Application::StopListening()
{
    Schedule([this]()
             {
        if (device_state_ == kDeviceStateListening) {
            protocol_->SendStopListening();
            SetDeviceState(kDeviceStateIdle);
        } });
}

void Application::Start()
{
    auto &board = Board::GetInstance();
    SetDeviceState(kDeviceStateStarting);
    /*设置显示*/
    auto display = board.GetDisplay();
    /*设置音频编解码器*/
    auto codec = board.GetAudioCodec();
    opus_decode_sample_rate_ = codec->output_sample_rate();
    opus_decoder_ = std::make_unique<OpusDecoderWrapper>(opus_decode_sample_rate_, 1);
    opus_encoder_ = std::make_unique<OpusEncoderWrapper>(16000, 1, OPUS_FRAME_DURATION_MS);
    if (board.GetBoardType() == "ml307")
        opus_encoder_->SetComplexity(5);
    else
        opus_encoder_->SetComplexity(3);

    if (codec->input_sample_rate() != 16000)
    {
        input_resampler_.Configure(codec->input_sample_rate(), 16000);
        reference_resampler_.Configure(codec->input_sample_rate(), 16000);
    }
    codec->OnInputReady([this, codec]()
                        {
        BaseType_t higher_priority_task_woken = pdFALSE;
        xEventGroupSetBitsFromISR(event_group_, AUDIO_INPUT_READY_EVENT, &higher_priority_task_woken);
        return higher_priority_task_woken == pdTRUE; });
    codec->OnOutputReady([this]()
                         {
        BaseType_t higher_priority_task_woken = pdFALSE;
        xEventGroupSetBitsFromISR(event_group_, AUDIO_OUTPUT_READY_EVENT, &higher_priority_task_woken);
        return higher_priority_task_woken == pdTRUE; });
    codec->SetOutputVolume(100);
    codec->Start();

    /*启动主循环*/
    xTaskCreate([](void *arg)
                {
        Application* app = (Application*)arg;
        app->MainLoop();
        vTaskDelete(NULL); }, "main_loop", 4096 * 2, this, 3, nullptr);

    /* 等待网络准备好 */
    board.StartNetwork();

    // 初始化协议
    display->SetStatus(Lang::Strings::LOADING_PROTOCOL);
#ifdef CONFIG_CONNECTION_TYPE_WEBSOCKET
    protocol_ = std::make_unique<WebsocketProtocol>();
#else
    protocol_ = std::make_unique<MqttProtocol>();
#endif
    // 当网络错误发生时调用OnNetworkError，并执行特定的操作
    protocol_->OnNetworkError([this](const std::string &message)
                              {
        // 将设备状态设置为闲置状态
        SetDeviceState(kDeviceStateIdle);
        // 显示错误消息，使用悲伤的表情图标和特定的声音
        Alert(Lang::Strings::ERROR, message.c_str(), "sad", Lang::Sounds::P3_EXCLAMATION); });
    // 当有新的音频数据到达时，协议对象将调用此回调函数
    protocol_->OnIncomingAudio([this](std::vector<uint8_t> &&data)
                               {
        // 确保线程安全，使用锁保护共享资源
        std::lock_guard<std::mutex> lock(mutex_);
        // 如果当前设备状态为说话中，则将接收到的音频数据添加到解码队列中
        if (device_state_ == kDeviceStateSpeaking) {
            audio_decode_queue_.emplace_back(std::move(data));
        } });
    // 当音频通道打开时调用此回调函数
    protocol_->OnAudioChannelOpened([this, codec, &board]()
                                    {
    // 设置电源节省模式为false，确保正常运行
    board.SetPowerSaveMode(false);

    // 检查服务器采样率与设备输出采样率是否匹配
    if (protocol_->server_sample_rate() != codec->output_sample_rate()) {
        // 如果不匹配，记录警告日志
        ESP_LOGW(TAG, "服务器采样率 %d 与设备输出采样率 %d 不匹配, 重采样可能导致失真",
            protocol_->server_sample_rate(), codec->output_sample_rate());
    }

    // 设置解码采样率为服务器采样率
    SetDecodeSampleRate(protocol_->server_sample_rate()); });
    // 当音频通道关闭时，调用此回调函数
    protocol_->OnAudioChannelClosed([this, &board]()
                                    {
    // 启用电源节省模式以减少功耗
    board.SetPowerSaveMode(true);

    // 安排一个任务以处理音频通道关闭后的清理工作
    Schedule([this]() {
        // 获取显示实例
        auto display = Board::GetInstance().GetDisplay();
        // 清除聊天消息
        display->SetChatMessage("system", "");
        // 设置设备状态为闲置
        SetDeviceState(kDeviceStateIdle);
    }); });
    protocol_->OnIncomingJson([this, display](const cJSON *root)
                              {
        // Parse JSON data
        auto type = cJSON_GetObjectItem(root, "type");
        if (strcmp(type->valuestring, "tts") == 0) {
            auto state = cJSON_GetObjectItem(root, "state");
            if (strcmp(state->valuestring, "start") == 0) {
                Schedule([this]() {
                    aborted_ = false;
                    if (device_state_ == kDeviceStateIdle || device_state_ == kDeviceStateListening) {
                        SetDeviceState(kDeviceStateSpeaking);
                    }
                });
            } else if (strcmp(state->valuestring, "stop") == 0) {
                Schedule([this]() {
                    if (device_state_ == kDeviceStateSpeaking) {
                        background_task_->WaitForCompletion();
                        if (keep_listening_) {
                            protocol_->SendStartListening(kListeningModeAutoStop);
                            SetDeviceState(kDeviceStateListening);
                        } else {
                            SetDeviceState(kDeviceStateIdle);
                        }
                    }
                });
            } else if (strcmp(state->valuestring, "sentence_start") == 0) {
                auto text = cJSON_GetObjectItem(root, "text");
                if (text != NULL) {
                    ESP_LOGI(TAG, "<< %s", text->valuestring);
                    Schedule([this, display, message = std::string(text->valuestring)]() {
                        display->SetChatMessage("assistant", message.c_str());
                    });
                }
            }
        } else if (strcmp(type->valuestring, "stt") == 0) {
            auto text = cJSON_GetObjectItem(root, "text");
            if (text != NULL) {
                ESP_LOGI(TAG, ">> %s", text->valuestring);
                Schedule([this, display, message = std::string(text->valuestring)]() {
                    display->SetChatMessage("user", message.c_str());
                });
            }
        } else if (strcmp(type->valuestring, "llm") == 0) {
            auto emotion = cJSON_GetObjectItem(root, "emotion");
            if (emotion != NULL) {
                Schedule([this, display, emotion_str = std::string(emotion->valuestring)]() {
                    display->SetEmotion(emotion_str.c_str());
                });
            }
        } else if (strcmp(type->valuestring, "iot") == 0) {
            auto commands = cJSON_GetObjectItem(root, "commands");
            if (commands != NULL) {
                ESP_LOGI(TAG, "Received iot commands");
            }
        } });
    // 启动MQTT协议
    protocol_->Start();

    // 检查新固件版本或获取MQTT代理地址
    ota_.SetCheckVersionUrl(CONFIG_OTA_VERSION_URL);
    ESP_LOGI(TAG, "OTA固件信息");
    ota_.SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());
    ESP_LOGI(TAG, "Device-Id: %s", SystemInfo::GetMacAddress().c_str());
    ota_.SetHeader("Client-Id", board.GetUuid());
    ESP_LOGI(TAG, "Client-Id: %s", board.GetUuid().c_str());
    ota_.SetHeader("Accept-Language", Lang::CODE);
    auto app_desc = esp_app_get_description();
    ota_.SetHeader("User-Agent", std::string(BOARD_NAME "/") + app_desc->version);

    xTaskCreate([](void *arg)
                {
        Application* app = (Application*)arg;
        app->CheckNewVersion();
        vTaskDelete(NULL); }, "check_new_version", 4096 * 2, this, 2, nullptr);

#if CONFIG_USE_AUDIO_PROCESSOR
    audio_processor_.Initialize(codec->input_channels(), codec->input_reference());
    audio_processor_.OnOutput([this](std::vector<int16_t> &&data)
                              { background_task_->Schedule([this, data = std::move(data)]() mutable
                                                           { opus_encoder_->Encode(std::move(data), [this](std::vector<uint8_t> &&opus)
                                                                                   { Schedule([this, opus = std::move(opus)]()
                                                                                              { protocol_->SendAudio(opus); }); }); }); });
#endif

#if CONFIG_USE_WAKE_WORD_DETECT
    wake_word_detect_.Initialize(codec->input_channels(), codec->input_reference());
    wake_word_detect_.OnVadStateChange([this](bool speaking)
                                       { Schedule([this, speaking]()
                                                  {
            if (device_state_ == kDeviceStateListening) {
                if (speaking) {
                    ESP_LOGI("speaking","检测到语音活动开始");
                    voice_detected_ = true;
                } else { 
                    ESP_LOGI("speaking","检测到语音活动结束");
                    voice_detected_ = false;
                }
            } }); });

    wake_word_detect_.OnWakeWordDetected([this](const std::string &wake_word)
                                         { Schedule([this, &wake_word]()
                                                    {
            ESP_LOGI("speaking","唤醒词检测后");
            if (device_state_ == kDeviceStateIdle) {
                SetDeviceState(kDeviceStateConnecting);
                wake_word_detect_.EncodeWakeWordData();

                if (!protocol_->OpenAudioChannel()) {
                    wake_word_detect_.StartDetection();
                    return;
                }
                
                std::vector<uint8_t> opus;
                // Encode and send the wake word data to the server
                while (wake_word_detect_.GetWakeWordOpus(opus)) {
                    protocol_->SendAudio(opus);
                }
                // Set the chat state to wake word detected
                protocol_->SendWakeWordDetected(wake_word);
                ESP_LOGI("TAG", "检测到唤醒字: %s", wake_word.c_str());
                keep_listening_ = true;
                SetDeviceState(kDeviceStateIdle);
            } else if (device_state_ == kDeviceStateSpeaking) {
                AbortSpeaking(kAbortReasonWakeWordDetected);
            } else if (device_state_ == kDeviceStateActivating) {
                SetDeviceState(kDeviceStateIdle);
            }

            // Resume detection
            wake_word_detect_.StartDetection(); }); });
    wake_word_detect_.StartDetection();
#endif

    SetDeviceState(kDeviceStateIdle);
    esp_timer_start_periodic(clock_timer_handle_, 1000000);
}

void Application::OnClockTimer()
{
    clock_ticks_++;

    // Print the debug info every 10 seconds
    if (clock_ticks_ % 10 == 0)
    {
        // SystemInfo::PrintRealTimeStats(pdMS_TO_TICKS(1000));
        int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        ESP_LOGI(TAG, "Free internal: %u minimal internal: %u", free_sram, min_free_sram);

        // If we have synchronized server time, set the status to clock "HH:MM" if the device is idle
        if (ota_.HasServerTime())
        {
            if (device_state_ == kDeviceStateIdle)
            {
                Schedule([this]()
                         {
                    // Set status to clock "HH:MM"
                    time_t now = time(NULL);
                    char time_str[64];
                    strftime(time_str, sizeof(time_str), "%H:%M  ", localtime(&now));
                    Board::GetInstance().GetDisplay()->SetStatus(time_str); });
            }
        }
    }
}

void Application::Schedule(std::function<void()> callback)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        main_tasks_.push_back(std::move(callback));
    }
    xEventGroupSetBits(event_group_, SCHEDULE_EVENT);
}

// The Main Loop controls the chat state and websocket connection
// If other tasks need to access the websocket or chat state,
// they should use Schedule to call this function
void Application::MainLoop()
{
    while (true)
    {
        auto bits = xEventGroupWaitBits(event_group_,
                                        SCHEDULE_EVENT | AUDIO_INPUT_READY_EVENT | AUDIO_OUTPUT_READY_EVENT,
                                        pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & AUDIO_INPUT_READY_EVENT)
        {
            InputAudio();
        }
        if (bits & AUDIO_OUTPUT_READY_EVENT)
        {
            OutputAudio();
        }
        if (bits & SCHEDULE_EVENT)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            std::list<std::function<void()>> tasks = std::move(main_tasks_);
            lock.unlock();
            for (auto &task : tasks)
            {
                task();
            }
        }
    }
}

void Application::ResetDecoder()
{
    std::lock_guard<std::mutex> lock(mutex_);
    opus_decoder_->ResetState();
    audio_decode_queue_.clear();
    last_output_time_ = std::chrono::steady_clock::now();
}

void Application::OutputAudio()
{
    auto now = std::chrono::steady_clock::now();
    auto codec = Board::GetInstance().GetAudioCodec();
    const int max_silence_seconds = 10;

    std::unique_lock<std::mutex> lock(mutex_);
    if (audio_decode_queue_.empty())
    {
        // Disable the output if there is no audio data for a long time
        if (device_state_ == kDeviceStateIdle)
        {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_output_time_).count();
            if (duration > max_silence_seconds)
            {
                codec->EnableOutput(false);
            }
        }
        return;
    }

    if (device_state_ == kDeviceStateListening)
    {
        audio_decode_queue_.clear();
        return;
    }

    last_output_time_ = now;
    auto opus = std::move(audio_decode_queue_.front());
    audio_decode_queue_.pop_front();
    lock.unlock();

    background_task_->Schedule([this, codec, opus = std::move(opus)]() mutable
                               {
        if (aborted_) {
            return;
        }

        std::vector<int16_t> pcm;
        if (!opus_decoder_->Decode(std::move(opus), pcm)) {
            return;
        }

        // Resample if the sample rate is different
        if (opus_decode_sample_rate_ != codec->output_sample_rate()) {
            int target_size = output_resampler_.GetOutputSamples(pcm.size());
            std::vector<int16_t> resampled(target_size);
            output_resampler_.Process(pcm.data(), pcm.size(), resampled.data());
            pcm = std::move(resampled);
        }
        
        codec->OutputData(pcm); });
}

void Application::InputAudio()
{
    auto codec = Board::GetInstance().GetAudioCodec();
    std::vector<int16_t> data;
    if (!codec->InputData(data))
    {
        return;
    }

    if (codec->input_sample_rate() != 16000)
    {
        if (codec->input_channels() == 2)
        {
            auto mic_channel = std::vector<int16_t>(data.size() / 2);
            auto reference_channel = std::vector<int16_t>(data.size() / 2);
            for (size_t i = 0, j = 0; i < mic_channel.size(); ++i, j += 2)
            {
                mic_channel[i] = data[j];
                reference_channel[i] = data[j + 1];
            }
            auto resampled_mic = std::vector<int16_t>(input_resampler_.GetOutputSamples(mic_channel.size()));
            auto resampled_reference = std::vector<int16_t>(reference_resampler_.GetOutputSamples(reference_channel.size()));
            input_resampler_.Process(mic_channel.data(), mic_channel.size(), resampled_mic.data());
            reference_resampler_.Process(reference_channel.data(), reference_channel.size(), resampled_reference.data());
            data.resize(resampled_mic.size() + resampled_reference.size());
            for (size_t i = 0, j = 0; i < resampled_mic.size(); ++i, j += 2)
            {
                data[j] = resampled_mic[i];
                data[j + 1] = resampled_reference[i];
            }
        }
        else
        {
            auto resampled = std::vector<int16_t>(input_resampler_.GetOutputSamples(data.size()));
            input_resampler_.Process(data.data(), data.size(), resampled.data());
            data = std::move(resampled);
        }
    }

#if CONFIG_USE_WAKE_WORD_DETECT
    if (wake_word_detect_.IsDetectionRunning())
    {
        wake_word_detect_.Feed(data);
    }
#endif
#if CONFIG_USE_AUDIO_PROCESSOR
    if (audio_processor_.IsRunning())
    {
        audio_processor_.Input(data);
    }
#else
    if (device_state_ == kDeviceStateListening)
    {
        background_task_->Schedule([this, data = std::move(data)]() mutable
                                   { opus_encoder_->Encode(std::move(data), [this](std::vector<uint8_t> &&opus)
                                                           { Schedule([this, opus = std::move(opus)]()
                                                                      { protocol_->SendAudio(opus); }); }); });
    }
#endif
}

void Application::AbortSpeaking(AbortReason reason)
{
    ESP_LOGI(TAG, "Abort speaking");
    aborted_ = true;
    protocol_->SendAbortSpeaking(reason);
}

void Application::SetDeviceState(DeviceState state)
{
    if (device_state_ == state)
    {
        return;
    }

    clock_ticks_ = 0;
    auto previous_state = device_state_;
    device_state_ = state;
    ESP_LOGI(TAG, "STATE: %s", STATE_STRINGS[device_state_]);
    // The state is changed, wait for all background tasks to finish
    background_task_->WaitForCompletion();

    auto &board = Board::GetInstance();
    auto codec = board.GetAudioCodec();
    auto display = board.GetDisplay();
    switch (state)
    {
    case kDeviceStateUnknown:
    case kDeviceStateIdle:
        display->SetStatus(Lang::Strings::STANDBY);
        display->SetEmotion("neutral");
#if CONFIG_USE_AUDIO_PROCESSOR
        audio_processor_.Stop();
#endif
        break;
    case kDeviceStateConnecting:
        display->SetStatus(Lang::Strings::CONNECTING);
        display->SetEmotion("neutral");
        display->SetChatMessage("system", "");
        break;
    case kDeviceStateListening:
        display->SetStatus(Lang::Strings::LISTENING);
        display->SetEmotion("neutral");
        ResetDecoder();
        opus_encoder_->ResetState();
#if CONFIG_USE_AUDIO_PROCESSOR
        audio_processor_.Start();
#endif
        UpdateIotStates();
        if (previous_state == kDeviceStateSpeaking)
        {
            // FIXME: Wait for the speaker to empty the buffer
            vTaskDelay(pdMS_TO_TICKS(120));
        }
        break;
    case kDeviceStateSpeaking:
        display->SetStatus(Lang::Strings::SPEAKING);
        ResetDecoder();
        codec->EnableOutput(true);
#if CONFIG_USE_AUDIO_PROCESSOR
        audio_processor_.Stop();
#endif
        break;
    default:
        // Do nothing
        break;
    }
}

void Application::SetDecodeSampleRate(int sample_rate)
{
    if (opus_decode_sample_rate_ == sample_rate)
    {
        return;
    }

    opus_decode_sample_rate_ = sample_rate;
    opus_decoder_.reset();
    opus_decoder_ = std::make_unique<OpusDecoderWrapper>(opus_decode_sample_rate_, 1);

    auto codec = Board::GetInstance().GetAudioCodec();
    if (opus_decode_sample_rate_ != codec->output_sample_rate())
    {
        ESP_LOGI(TAG, "Resampling audio from %d to %d", opus_decode_sample_rate_, codec->output_sample_rate());
        output_resampler_.Configure(opus_decode_sample_rate_, codec->output_sample_rate());
    }
}

void Application::UpdateIotStates()
{
}

void Application::Reboot()
{
    ESP_LOGI(TAG, "Rebooting...");
    esp_restart();
}

