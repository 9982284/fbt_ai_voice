#include "fbt_audio_repeater.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#define TAG "FbtAudioRepeater"
FbtAudioRepeater::FbtAudioRepeater(AudioService *audio_service) : audio_service_(audio_service) {
    ESP_LOGI(TAG, "Fbt Audio Repeater enabled successfully ");
}

FbtAudioRepeater::~FbtAudioRepeater() {
}

bool FbtAudioRepeater::InterruptRingtone() {
    if (!audio_service_)
        return false;
    if (ringtone_task_) {
        ringtone_task_ = nullptr; // 先置空，避免重复进入
        audio_service_->FbtInterruptPlayback();
    }
    return true;
}

void FbtAudioRepeater::PlayStream(std::unique_ptr<AudioStreamPacket> packet) {
    if (!audio_service_ || !packet)
        return;
    audio_service_->PushPacketToDecodeQueue(std::move(packet));
}

void FbtAudioRepeater::PlayRingtone(std::function<bool()> callback) {
    if (!InterruptRingtone())
        return;

    auto ctx = new RingtoneContext{this, callback};

    xTaskCreate(RingtoneTask,
                "async_play",
                4096,
                ctx, // 传递上下文
                4,
                &ringtone_task_);
}

void FbtAudioRepeater::EnableDeviceAec(bool enable) {
    if (!audio_service_)
        return;
    audio_service_->EnableDeviceAec(enable);
}

void FbtAudioRepeater::RingtoneTask(void *arg) {
    std::unique_ptr<RingtoneContext> ctx(static_cast<RingtoneContext *>(arg));
    FbtAudioRepeater *self = ctx->server;
    auto &callback = ctx->callback;

    for (int i = 0; i < 3; i++) {
        vTaskDelay(pdMS_TO_TICKS(500));
        try {
            if (callback && callback()) {
                self->audio_service_->PlaySound(self->phone_ringtone_);
            } else {
                break;
            }
        } catch (...) {
            break;
        }
    }

    self->ringtone_task_ = nullptr;
    vTaskDelete(NULL);
}
