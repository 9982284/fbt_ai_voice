#include "fbt_voice_transport.h"

#include "audio_codec.h"
#include "board.h"
#include "fbt_event_bus.h"

#include "application.h"

#include "assets/lang_config.h"
#include "fbt_simple_dot.h"
#include "settings.h"
#include "system_info.h"
#include <arpa/inet.h>
#include <cJSON.h>
#include <cinttypes>
#include <cstring>
#include <esp_log.h>

#define TAG "fbt_voice"

FbtVoiceTransport::FbtVoiceTransport(EventGroupHandle_t event_group, FbtAudioRepeater *audio_repeater, FbtMqttServer *mqtt_server)
    : board_(Board::GetInstance()),
      audio_codec_(board_.GetAudioCodec()),
      display_(board_.GetDisplay()),
      audio_repeater_(audio_repeater),
      event_group_(event_group),
      fbt_mqtt_(mqtt_server)

{

    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.Subscribe("fbt_voice", FbtEvents::MQTT_VOICE_PING, [this](const std::string &type, const std::string &data) {
        send_ping();
    });

    esp_timer_create_args_t timer_args = {
        .callback = &VoiceTimeoutHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "voice_timeout"};
    esp_timer_create(&timer_args, &voice_timeout_timer_);
}

FbtVoiceTransport::~FbtVoiceTransport() {
    close_server();
}

bool FbtVoiceTransport::Start(std::string data) {
    ESP_LOGI(TAG, "FbtVoiceTransport starting...");

    // 解析 JSON
    cJSON *root = cJSON_Parse(data.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return false;
    }

    // 提取 deviceId
    cJSON *device_id_json = cJSON_GetObjectItem(root, "deviceId");
    if (device_id_json && cJSON_IsString(device_id_json)) {
        device_id_ = device_id_json->valuestring;
    }

    // 提取 voice 配置
    cJSON *voice_json = cJSON_GetObjectItem(root, "voice");
    if (!voice_json || !cJSON_IsObject(voice_json)) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Missing voice configuration");
        return false;
    }

    // 从 voice 对象中提取 groupId, host, port
    cJSON *group_id_json = cJSON_GetObjectItem(voice_json, "groupId");
    cJSON *host_json = cJSON_GetObjectItem(voice_json, "host");
    cJSON *port_json = cJSON_GetObjectItem(voice_json, "port");

    if (group_id_json && cJSON_IsString(group_id_json)) {
        group_id_ = group_id_json->valuestring;
    }

    if (host_json && cJSON_IsString(host_json)) {
        server_addr_ = host_json->valuestring;
    }

    if (port_json && cJSON_IsNumber(port_json)) {
        server_port_ = port_json->valueint;
    }

    // 释放内存
    cJSON_Delete(root);

    // 验证必要参数
    if (server_addr_.empty() || server_port_ == 0 || device_id_.empty()) {
        ESP_LOGE(TAG, "Voice transport config incomplete: addr=%s, port=%d",
                 server_addr_.c_str(), server_port_);
        return false;
    }

    // 启动服务器
    open_server();
    return true;
}

void FbtVoiceTransport::open_server() {
    // 1. 参数检查（修正逻辑错误）
    if (server_addr_.empty() || server_port_ == 0) {
        ESP_LOGE(TAG, "Server address not configured");
        return;
    }

    auto network = board_.GetNetwork();
    if (!network) {
        ESP_LOGE(TAG, "Network interface not available");
        return;
    }

    udp_ = network->CreateUdp(1);
    if (!udp_) {
        ESP_LOGE(TAG, "Failed to create UDP instance");
        return;
    }

    udp_->OnMessage([this](const std::string &data) {
        on_udp_packet(data);
    });

    if (!udp_->Connect(server_addr_, server_port_)) {
        ESP_LOGE(TAG, "Failed to connect to UDP server %s:%d",
                 server_addr_.c_str(), server_port_);
        udp_.reset();
        return;
    }

    is_running_ = true;
    send_ping();
    ESP_LOGI(TAG, "Voice server started, connected to %s:%d, room: %s",
             server_addr_.c_str(), server_port_, group_id_.c_str());
}

void FbtVoiceTransport::close_server() {
    if (!is_running_) {
        return;
    }

    udp_.reset();
    if (voice_timeout_timer_) {
        esp_timer_delete(voice_timeout_timer_);
    };

    is_running_ = false;
}

void FbtVoiceTransport::on_udp_packet(const std::string &payload) {
    if (payload.empty()) {
        return;
    }
    uint8_t packet_type = static_cast<uint8_t>(payload[0]);
    if (packet_type == 0x00) {
        on_udp_message(payload.substr(1));
    } else if (packet_type == 0x01) {
        on_remote_audio(payload.substr(1));
    } else {
        ESP_LOGW(TAG, "Unknown packet type: 0x%02X", packet_type);
    }
}

void FbtVoiceTransport::on_udp_message(const std::string &data) {
    cJSON *root = cJSON_Parse(data.c_str());
    if (!root) {
        return;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type)) {
        cJSON_Delete(root);
        return;
    }

    const char *msg_type = type->valuestring;

    if (strcmp(msg_type, FbtCommand::FBT_ANSWER) == 0) {
        handle_answer(data);
    }

    cJSON_Delete(root);
}

void FbtVoiceTransport::on_remote_audio(const std::string &payload) {
    // 快速检查
    if (payload.empty() || !audio_repeater_ || !is_running_ || rtc_state_ != kTuneIn) {
        return;
    }
    close_on_timeout();

    auto packet = std::make_unique<AudioStreamPacket>();
    packet->sample_rate = audio_sample_rate_;
    packet->frame_duration = audio_frame_duration_;
    packet->timestamp = 0;
    packet->payload.assign(payload.begin(), payload.end());

    // 调整payload大小并填充音频数据
    // memcpy(packet->payload.data(), payload.data(), payload.length());

    audio_repeater_->PlayStream(std::move(packet));
}

void FbtVoiceTransport::handle_answer(const std::string data) {
    // ESP_LOGI(TAG, "fbt ok Audio transmission started!");
    cJSON *root = cJSON_Parse(data.c_str());
    if (!root) {
        return;
    }

    cJSON *status = cJSON_GetObjectItem(root, "status");

    if (cJSON_IsNumber(status)) {
        FbtVoiceOfferStatus voice_status = static_cast<FbtVoiceOfferStatus>(status->valueint);
        switch (voice_status) {
            case kStartSpeaking:
                start_speaking();
                break;
            case kEndSpeaking:
                end_active();
                break;
            case kStartTuneIn:
                start_tune_in(data);
                break;
            default:
                end_active();
                break;
        }
        if (voice_status == kStartSpeaking || voice_status == kStartTuneIn) {
            if (cJSON_GetObjectItem(root, "name")) {
                std::string name = cJSON_GetObjectItem(root, "name")->valuestring;
                display_->SetChatMessage("system", name.c_str());
            }
            display_->SetEmotion("laughing");
        }
    }
    cJSON_Delete(root);
}

void FbtVoiceTransport::start_speaking() {
    rtc_state_ = kSpeaking;
    SimpleDot::Set(0);
    display_->SetStatus(Lang::Strings::FBT_VOICE_SPEAKING);
    close_on_timeout();
}

void FbtVoiceTransport::start_tune_in(const std::string &payload) {
    if (rtc_state_ == kSpeaking) {
        return;
    };
    if (!load_media(payload)) {
        end_active();
        return;
    };
    rtc_state_ = kTuneIn;
    if (!is_enter_) {
        start_voice();
        auto &event_bus = FbtEventBus::GetInstance();
        event_bus.PublishAsync(FbtEvents::VOICE_START_ACTIVE, "");
    }
    SimpleDot::Set(1);
    display_->SetStatus(Lang::Strings::FBT_VOICE_IN_SPEECH);
    start_on_timeout(60 * 3);
}

void FbtVoiceTransport::OnAudioMicrophone(std::unique_ptr<AudioStreamPacket> packet) {
    if (!udp_ || !is_running_ || rtc_state_ != kSpeaking) {
        return;
    }

    std::string payload_;
    payload_.reserve(1 + packet->payload.size()); // 预分配空间
    payload_.push_back(0x01);
    payload_.append(reinterpret_cast<const char *>(packet->payload.data()),
                    packet->payload.size());

    udp_->Send(payload_);
}
void FbtVoiceTransport::OnSpeaking(const bool enable) {
    if (enable) {
        rtc_state_ = kOffer;
    }
    send_offer(enable ? kStartSpeaking : kEndSpeaking);
    if (!enable)
        end_active();
}

bool FbtVoiceTransport::start_voice() {
    if (!display_ || !audio_codec_ || !event_group_) {
        ESP_LOGE(TAG, "Failed to enter intercom mode, device not ready");
        return false;
    }
    if (!permission_enter()) {
        return false;
    }

    xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_START_SESSION);
    audio_codec_->EnableOutput(true);
    board_.SetPowerSaveMode(false);
    if (is_enter_) {
        audio_codec_->EnableInput(true);
        display_->SetStatus(Lang::Strings::FBT_HOLD_TO_TALK);
        xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO);
    }

    return true;
}

void FbtVoiceTransport::end_active() {
    if (rtc_state_ == kIdle) {
        return;
    }
    if (!display_) {
        return;
    }
    SimpleDot::Set(2);
    display_->SetChatMessage("system", "");
    display_->SetEmotion("neutral");

    if (!is_enter_) {
        // board_.SetPowerSaveMode(true);
        auto &event_bus = FbtEventBus::GetInstance();
        event_bus.PublishAsync(FbtEvents::VOICE_END_ACTIVE, "");
    } else {
        display_->SetStatus(Lang::Strings::FBT_HOLD_TO_TALK);
    }
    rtc_state_ = kIdle;
    close_on_timeout();
}

void FbtVoiceTransport::send_offer(FbtVoiceOfferStatus status) {
    if (!permission_enter()) {
        return;
    }
    if (status == kStartSpeaking) {
        display_->SetStatus(Lang::Strings::CONNECTING);
        start_on_timeout(20);
    }
    std::string json_str = generate_json(FbtCommand::FBT_OFFER, status);
    if (json_str.empty()) {
        return;
    }
    send_udp_message(json_str);
}

void FbtVoiceTransport::send_ping() {
    if (!udp_ || group_id_.empty() || device_id_.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(channel_mutex_);
    std::string packet;
    packet.reserve(1 + device_id_.size());
    packet.push_back(0x02);
    packet.append(device_id_.c_str());
    udp_->Send(packet);
}

void FbtVoiceTransport::send_udp_message(const std::string &json_str) {
    if (!permission_enter()) {
        return;
    }
    std::lock_guard<std::mutex> lock(channel_mutex_);
    std::string packet;
    packet.reserve(1 + json_str.size());
    packet.push_back(0x00);
    packet.append(json_str);
    udp_->Send(packet);
}

void FbtVoiceTransport::start_on_timeout(int timeout_us) {
    if (voice_timeout_timer_) {
        esp_timer_stop(voice_timeout_timer_);
    }
    if (voice_timeout_timer_) {
        esp_timer_start_once(voice_timeout_timer_, 1000000 * timeout_us);
    }
    is_on_timeout_ = true;
}

void FbtVoiceTransport::close_on_timeout() {
    if (!is_on_timeout_)
        return;
    if (voice_timeout_timer_) {
        esp_timer_stop(voice_timeout_timer_);
    }
    is_on_timeout_ = false;
}

void FbtVoiceTransport::OnEnterVoiceRoom(const std::string &data) {
    cJSON *root = cJSON_Parse(data.c_str());
    cJSON *groupId = cJSON_GetObjectItem(root, "groupId");
    cJSON *deviceId = cJSON_GetObjectItem(root, "deviceId");
    if (!groupId || !cJSON_IsString(groupId) ||
        !deviceId || !cJSON_IsString(deviceId)) {
        cJSON_Delete(root);
        return;
    }
    group_id_ = groupId->valuestring;
    device_id_ = deviceId->valuestring;
    cJSON_Delete(root);
    send_ping();
}

bool FbtVoiceTransport::OnEnterVoiceMode() {
    if (!permission_enter()) {
        return false;
    }
    is_enter_ = true;
    display_->SetChatMessage("system", "");
    return start_voice();
}

void FbtVoiceTransport::OnExitVoiceMode() {
    is_enter_ = false;
    board_.SetPowerSaveMode(true);
}

std::string FbtVoiceTransport::generate_json(const std::string &type, FbtVoiceOfferStatus status) {
    if (!permission_enter()) {
        return "";
    }
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return "";
    }

    cJSON_AddStringToObject(root, "type", type.c_str());
    cJSON_AddStringToObject(root, "deviceId", device_id_.c_str());
    if (status > 0) {
        cJSON_AddNumberToObject(root, "status", status);
    }
    if (status == kStartSpeaking) {
        cJSON *audio = FbtConfig::FbtBuilder::buildAudioMedia();
        if (audio)
            cJSON_AddItemToObject(root, "audio", audio);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    std::string result;
    if (json_str) {
        result = json_str;
        free(json_str);
    }
    // ESP_LOGD(TAG, "generate json: %s", result.c_str());
    cJSON_Delete(root);
    return result;
}

bool FbtVoiceTransport::permission_enter() {
    if (!udp_) {
        return false;
    }
    if (group_id_.empty() || device_id_.empty()) {
        display_->SetChatMessage("system", "你还没有加入对讲室");
        display_->SetEmotion("crying");
        return false;
    }
    return true;
}

bool FbtVoiceTransport::load_media(const std::string &json_data) {
    cJSON *root = cJSON_Parse(json_data.c_str());
    if (!root) {
        return false;
    }
    cJSON *audio_params = cJSON_GetObjectItem(root, "audio");
    if (cJSON_IsObject(audio_params)) {
        cJSON *sample_rate = cJSON_GetObjectItem(audio_params, "sampleRate");
        cJSON *frame_duration = cJSON_GetObjectItem(audio_params, "frameDuration");
        if (cJSON_IsNumber(sample_rate))
            audio_sample_rate_ = sample_rate->valueint;
        if (cJSON_IsNumber(frame_duration))
            audio_frame_duration_ = frame_duration->valueint;
    }
    cJSON_Delete(root);
    return true;
}
