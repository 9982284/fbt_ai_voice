
#include "fbt_manager.h"

#include "application.h"
#include "assets/lang_config.h"
#include "audio_codec.h"
#include "fbt_audio_repeater.h"
#include "fbt_event_bus.h"
#include "fbt_http.h"
#include "fbt_mqtt_server.h"
#include "fbt_phone_transport.h"
#include "fbt_voice_transport.h"
#include <esp_log.h>
#define TAG "fbt_manager"

FbtManager &FbtManager::GetInstance() {
    static FbtManager instance;
    return instance;
}

FbtManager::FbtManager()
    : event_group_(nullptr),
      audio_service_(nullptr),
      board_(Board::GetInstance()),
      audio_codec_(board_.GetAudioCodec()),
      display_(board_.GetDisplay()) {

    ESP_LOGI(TAG, "Created");
}

FbtManager::~FbtManager() {
    Stop();
    ESP_LOGI(TAG, "Destroyed");
}

void FbtManager::Listener() {

    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.Subscribe("fbt_manager", FbtEvents::PHONE_STARTED, [this](const std::string &type, const std::string &data) {
        if (fbt_phone_) {
            ESP_LOGI(TAG, "Received phone_started  %s", data.c_str());
            event_ = kEventPhone;
        }
    });
    event_bus.Subscribe("fbt_manager", FbtEvents::PHONE_CALL_READY, [this](const std::string &type, const std::string &data) {
        ESP_LOGI(TAG, "Received phone_call_ready event");
        receive_audio_ = true;
    });
    event_bus.Subscribe("fbt_manager", FbtEvents::PHONE_CLOSED, [this](const std::string &type, const std::string &data) {
        ESP_LOGI(TAG, "Received phone_closed event");
        closeSession();
    });

    event_bus.Subscribe("fbt_manager", FbtEvents::MQTT_ENTER_INTERCOM_ROOM, [this](const std::string &type, const std::string &data) {
        if (fbt_voice_) {
            fbt_voice_->OnEnterVoiceRoom(data);
        }
        this->OnEnterIntercomMode();
    });
    event_bus.Subscribe("fbt_manager", FbtEvents::VOICE_START_ACTIVE, [this](const std::string &type, const std::string &data) {
        event_ = kEventVoice;
    });
    event_bus.Subscribe("fbt_manager", FbtEvents::VOICE_END_ACTIVE, [this](const std::string &type, const std::string &data) {
        closeSession();
    });
}

bool FbtManager::Start(EventGroupHandle_t event_, AudioService &audio_service) {
    ESP_LOGI(TAG, "Starting FBT services...");

    if (running_) {
        ESP_LOGW(TAG, "Already running");
        return true;
    }

    event_group_ = event_;
    audio_service_ = &audio_service;
    fbt_http_ = std::make_unique<FbtHttp>();
    std::string data = fbt_http_->GetConfig();
    if (data.empty()) {
        ESP_LOGE(TAG, "Failed to get config");
        return false;
    }

    Listener();

    audio_repeater_ = std::make_unique<FbtAudioRepeater>(audio_service_);
    // 创建并启动MQTT服务
    fbt_mqtt_ = std::make_unique<FbtMqttServer>();
    if (!fbt_mqtt_->Start(data)) {
        ESP_LOGE(TAG, "Failed to start MQTT server");
    }

    fbt_voice_ = std::make_unique<FbtVoiceTransport>(event_, audio_repeater_.get(), fbt_mqtt_.get());
    fbt_voice_->Start(data);

    fbt_phone_ = std::make_unique<FbtPhoneTransport>(event_, audio_repeater_.get(), fbt_mqtt_.get());

    fbt_phone_->Start();
    running_ = true;
    ESP_LOGI(TAG, "All FBT services started via event bus");
    return true;
}

void FbtManager::Stop() {
    if (!running_)
        return;

    ESP_LOGI(TAG, "Stopping FBT services...");

    if (fbt_phone_) {
        fbt_phone_->ClosePhone();
        fbt_phone_.reset();
    }

    if (fbt_mqtt_) {
        fbt_mqtt_->Stop();
        fbt_mqtt_.reset();
    }

    running_ = false;
    event_group_ = nullptr;
    audio_service_ = nullptr;

    ESP_LOGI(TAG, "All FBT services stopped");
}

bool FbtManager::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (!running_) {
        return false;
    }
    switch (event_) {
        case kEventPhone:
            fbt_phone_->OnAudioMicrophone(std::move(packet));
            break;
        case kEventVoice:
            fbt_voice_->OnAudioMicrophone(std::move(packet));
            break;
        default:
            break;
    }

    return true;
}

bool FbtManager::OnEnterIntercomMode() {
    if (event_ != kEventIdle) {
        return false;
    }
    if (!fbt_voice_) {
        return false;
    }

    if (!fbt_voice_->OnEnterVoiceMode()) {
        return false;
    }
    event_ = kEventVoice;
    return true;
}

void FbtManager::OnExitVoiceMode() {
    if (event_ != kEventVoice) {
        return;
    }
    if (!fbt_voice_) {
        return;
    }

    fbt_voice_->OnExitVoiceMode();
    closeSession();
}

void FbtManager::OnSpeaking(const bool enable) {
    if (event_ != kEventVoice) {
        return;
    }
    if (!fbt_voice_) {
        return;
    }

    fbt_voice_->OnSpeaking(enable);
}

void FbtManager::ClosePhone(const std::string &reason) {
    if (!running_ || !fbt_phone_)
        return;

    ESP_LOGI(TAG, "Service manager closing phone: %s", reason.c_str());

    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.Publish(FbtEvents::CLOSE_PHONE, reason);
}

void FbtManager::OnClickButton() {
    if (event_ == kEventIdle) {
        return;
    }
    if (fbt_phone_) {
        fbt_phone_->OnSwitchAnswer();
    }
}

bool FbtManager::IsAudioStarted() const {
    if (!fbt_voice_ || !fbt_phone_) {
        return false;
    }

    switch (event_) {
        case kEventPhone:
            return fbt_phone_ ? fbt_phone_->IsAudioStarted() : false;
        case kEventVoice:
            return fbt_voice_ ? fbt_voice_->IsStartSpeaking() : false;
        default:
            return false;
    }
}

bool FbtManager::OnHangUp() {
    if (fbt_phone_ && event_ == kEventPhone) {
        return fbt_phone_->OnHangUp();
    }
    return false;
}

bool FbtManager::OnAnswer() {
    if (fbt_phone_ && event_ == kEventPhone) {
        return fbt_phone_->OnAnswer();
    }
    return false;
}

FbtRtcState FbtManager::GetVoiceRtcState() {
    if (!fbt_voice_) {
        return kIdle;
    }
    return fbt_voice_->GetRtcState();
}

void FbtManager::closeSession() {
    if (event_ == kEventIdle) {
        return;
    }

    receive_audio_ = false;
    auto &board_ = Board::GetInstance();
    event_ = kEventIdle;
    if (event_group_) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_STOP_SESSION);
    }
    board_.SetPowerSaveMode(true);
}