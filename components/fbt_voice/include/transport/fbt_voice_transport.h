#ifndef FBT_VOICE_TRANSPORT_H
#define FBT_VOICE_TRANSPORT_H

#include "display.h"

#include "fbt_audio_repeater.h"
#include "fbt_constants.h"
#include "fbt_mqtt_server.h"
#include "fbt_udp.h"
#include "fbt_ui_voice.h"
#include "protocol.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <mbedtls/aes.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

enum FbtVoiceOfferStatus {
    kStartSpeaking = 1,
    kEndSpeaking,
    kStartTuneIn,
};

class FbtVoiceTransport {

  public:
    FbtVoiceTransport(EventGroupHandle_t event_group, FbtAudioRepeater *audio_repeater, FbtMqttServer *mqtt_server);
    ~FbtVoiceTransport();

    bool Start();
    void OnAudioMicrophone(std::unique_ptr<AudioStreamPacket> packet);
    void OnSpeaking(const bool enable);
    void OnEnterVoiceRoom(const std::string &data);
    bool OnEnterVoiceMode();
    void OnExitVoiceMode();
    FbtRtcState GetRtcState() { return rtc_state_; };
    bool IsStartSpeaking() { return rtc_state_ == kSpeaking; };

  private:
    bool open_server();
    void close_server();
    void on_udp_packet(const std::string &payload);

    void on_udp_message(const std::string &payload);
    void on_remote_audio(const std::string &payload);
    void handle_answer(const std::string data);
    void start_speaking();
    void start_tune_in(const std::string &payload);
    bool start_voice();
    void end_active();
    void send_offer(FbtVoiceOfferStatus status);
    void send_ping();
    void send_udp_message(const std::string &json_str);
    void start_on_timeout(int timeout_us);
    void close_on_timeout();
    bool load_media(const std::string &json_data);

    bool is_unavailable();

    std::string
    generate_json(const std::string &type, FbtVoiceOfferStatus status);

    static void VoiceTimeoutHandler(void *arg) {
        FbtVoiceTransport *server = static_cast<FbtVoiceTransport *>(arg);
        server->end_active();
    }

  private:
    Board &board_;
    AudioCodec *audio_codec_;
    Display *display_;
    FbtAudioRepeater *audio_repeater_;
    EventGroupHandle_t event_group_;
    FbtMqttServer *fbt_mqtt_;
    FbtUiVoice &voice_ui_;

    std::unique_ptr<FbtUdp> udp_;
    std::mutex channel_mutex_;

    std::string group_id_;
    std::string device_id_;
    std::string server_addr_;
    int server_port_ = 0;

    esp_timer_handle_t voice_timeout_timer_ = nullptr;

    int audio_sample_rate_ = 16000;
    int audio_frame_duration_ = 60;
    /**
     * 缓存的音量
     */
    int cache_volume_;
    /**
     * 是否主动进入
     */
    bool is_enter_ = false;
    bool is_running_ = false;

    bool is_on_timeout_ = false;

    FbtRtcState rtc_state_ = kIdle;
};

#endif // FBT_VOICE_TRANSPORT_H