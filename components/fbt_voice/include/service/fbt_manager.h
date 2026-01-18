
#pragma once
#include "audio_service.h"
#include "display.h"
#include "esp_timer.h"
#include "fbt_constants.h"
#include "fbt_ui_phone.h"
#include "protocol.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <memory>
#include <string>

// 前向声明
class AudioService;
struct EventGroupDef_t;
typedef struct EventGroupDef_t *EventGroupHandle_t;

class FbtAudioRepeater;
class FbtHttp;
class FbtMqttServer;
class FbtPhoneTransport;
class FbtVoiceTransport;

/**
 * 全局服务管理器
 */
class FbtManager {
  public:
    static FbtManager &GetInstance();

    // 启动所有服务
    bool Start(EventGroupHandle_t event_group, AudioService &audio_service);

    // 停止所有服务
    void Stop();

    // 发送音频数据（直接调用Phone）
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet);

    bool OnEnterIntercomMode();

    void OnExitVoiceMode();

    bool IsIntercomMode() { return event_ == kEventVoice; }

    void OnSpeaking(const bool enable);

    // 关闭电话（直接调用Phone）
    void ClosePhone(const std::string &reason);
    /**
     * 按钮单击事件
     */
    void OnClickButton();
    // 状态查询
    bool IsAudioStarted() const;
    bool IsRunning() const { return running_; }

    /**
     * 挂断电话
     */
    bool OnHangUp();
    /**
     * 接听电话
     */
    bool OnAnswer();

    FbtRtcState GetVoiceRtcState();

  private:
    FbtManager();
    ~FbtManager();

    FbtManager(const FbtManager &) = delete;
    FbtManager &operator=(const FbtManager &) = delete;

  private:
    std::unique_ptr<FbtAudioRepeater> audio_repeater_;
    std::unique_ptr<FbtHttp> fbt_http_;
    std::unique_ptr<FbtMqttServer> fbt_mqtt_;
    std::unique_ptr<FbtPhoneTransport> fbt_phone_;
    std::unique_ptr<FbtVoiceTransport> fbt_voice_;

    EventGroupHandle_t event_group_ = nullptr;
    AudioService *audio_service_ = nullptr;
    Board &board_;
    AudioCodec *audio_codec_;
    Display *display_;
    bool running_ = false;
    void Listener();

    std::string phone_name_;
    /**
     * 正在进行的事件
     */
    FbtServerState event_ = kEventIdle;

    bool receive_audio_ = false;

    bool enable_timeout_ = false;

    void closeSession();
};