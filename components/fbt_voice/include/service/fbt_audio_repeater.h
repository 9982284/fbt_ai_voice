
#ifndef FBT_AUDIO_REPEATER_H
#define FBT_AUDIO_REPEATER_H
#include "assets/lang_config.h"
#include "audio_codec.h"
#include "audio_service.h"
#include "protocol.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class FbtAudioRepeater {

  public:
    FbtAudioRepeater(AudioService *audio_service);
    ~FbtAudioRepeater();

    /**
     * 中断铃声
     */
    bool InterruptRingtone();
    /**
     * 播放网络流
     */
    void PlayStream(std::unique_ptr<AudioStreamPacket> packet);
    /**
     * 播放铃声
     */
    void PlayRingtone(std::function<bool()> callback);

    void EnableDeviceAec(bool enable);

  private:
    AudioService *audio_service_ = nullptr;
    /**
     * 电话铃声
     */
    const std::string_view phone_ringtone_ = Lang::Sounds::OGG_PHONE_IN;

    std::function<bool()> ringtone_callback_;
    TaskHandle_t ringtone_task_ = nullptr;
    std::mutex ringtone_mutex_;

    static void RingtoneTask(void *arg);

    struct RingtoneContext {
        FbtAudioRepeater *server;
        std::function<bool()> callback;
    };
};

#endif // FBT_AUDIO_REPEATER_H