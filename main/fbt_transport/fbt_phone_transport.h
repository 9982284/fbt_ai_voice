#ifndef FBT_PHONE_TRANSPORT_H
#define FBT_PHONE_TRANSPORT_H

#include "display.h"

#include "fbt_audio_repeater.h"
#include "fbt_config.h"
#include "fbt_constants.h"
#include "fbt_mqtt_server.h"

#include "protocol.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <mbedtls/aes.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Udp;
class AudioCodec;

class FbtPhoneTransport {
  public:
    FbtPhoneTransport(EventGroupHandle_t event_group, FbtAudioRepeater *audio_repeater, FbtMqttServer *mqtt_server);
    ~FbtPhoneTransport();

    void OnAudioMicrophone(std::unique_ptr<AudioStreamPacket> packet);
    void OnError(const std::string &service, int error_code);
    void onStartPhone(const std::string &json_str);
    void OnSwitchAnswer();
    /**
     * 挂断电话
     */
    bool OnHangUp();
    /**
     * 接听电话
     */
    bool OnAnswer();

    void ClosePhone();

    bool IsRunning() const { return is_running_; }
    bool IsAudioStarted() const { return rtc_state_ == kActive; }

  private:
    bool parse_config(const std::string &json_message);
    bool connect_udp_server();
    bool send_udp_message(const std::string &type);
    bool send_offer();
    bool send_bye();
    void close();

    // 呼出模式
    void handle_call_out();
    // 呼入模式
    void handle_call_in();

    void on_udp_packet(const std::string &data);
    void handle_json(const std::string &json_data);
    void on_remote_audio(const std::string &data);
    void start_call();
    std::string generate_json(const std::string &type);

    bool load_media(const std::string &json_data);

  private:
    // 网络组件
    std::unique_ptr<Udp> udp_;
    std::mutex channel_mutex_;
    mbedtls_aes_context aes_ctx_;

    // 硬件组件
    Board &board_;
    AudioCodec *audio_codec_;
    Display *display_;
    // 事件组
    EventGroupHandle_t event_group_;
    FbtAudioRepeater *audio_repeater_;
    FbtMqttServer *fbt_mqtt_;

    // 配置和状态
    FbtStruct::PhoneSession session_;

    // 状态标志
    bool is_running_;
    FbtRtcState rtc_state_ = kIdle;

    // 序列号
    uint32_t local_sequence_;
    uint32_t remote_sequence_;
};
#endif // FBT_PHONE_TRANSPORT_H