#include "fbt_phone_transport.h"
#include "application.h"
#include "assets/lang_config.h"
#include "audio_codec.h"
#include "board.h"

#include "fbt_event_bus.h"
#include "system_info.h"
#include <arpa/inet.h>
#include <cJSON.h>
#include <cinttypes>
#include <cstring>
#include <esp_log.h>

#define TAG "fbt_phone"

FbtPhoneTransport::FbtPhoneTransport(EventGroupHandle_t event_group, FbtAudioRepeater *audio_repeater, FbtMqttServer *mqtt_server)
    : board_(Board::GetInstance()),
      audio_codec_(board_.GetAudioCodec()),
      display_(board_.GetDisplay()),
      event_group_(event_group),
      audio_repeater_(audio_repeater),
      fbt_mqtt_(mqtt_server),
      is_running_(false),
      local_sequence_(0),
      remote_sequence_(0),
      phone_ui_(FbtUiPhone::GetInstance()) {

    mbedtls_aes_init(&aes_ctx_);

    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.Subscribe("fbt_phone", FbtEvents::START_PHONE, [this](const std::string &type, const std::string &data) {
        ESP_LOGI(TAG, "Received start_phone event from MQTT");
        this->onStartPhone(data);
    });

    event_bus.Subscribe("fbt_phone", FbtEvents::CLOSE_PHONE, [this](const std::string &type, const std::string &data) {
        ESP_LOGI(TAG, "Received CLOSE_PHONE event");
        this->close();
    });

    ESP_LOGI(TAG, "FBT audio phone constructed");

    is_running_ = true;
}

FbtPhoneTransport::~FbtPhoneTransport() {
    ClosePhone();
    vEventGroupDelete(event_group_);
    mbedtls_aes_free(&aes_ctx_);
    ESP_LOGI(TAG, "FBT audio phone destroyed");
}

void FbtPhoneTransport::Start() {
    auto &ui_ = FbtUiPhone::GetInstance();
    ui_.Initialize();
    ui_.OnAnswer([this]() {
        OnAnswer();
    });
    ui_.OnHangUp([this]() {
        OnHangUp();
    });
}

void FbtPhoneTransport::OnAudioMicrophone(std::unique_ptr<AudioStreamPacket> packet) {
    if (rtc_state_ != CALLING || !udp_) {
        return;
    }

    std::lock_guard<std::mutex> lock(channel_mutex_);

    // 准备包头（无论是否加密都需要）
    std::string nonce(session_.nonce);
    nonce[0] = 0x01; // 音频包类型
    *(uint16_t *)&nonce[2] = htons(packet->payload.size());
    *(uint32_t *)&nonce[8] = htonl(packet->timestamp);
    *(uint32_t *)&nonce[12] = htonl(++local_sequence_);

    std::string full_packet;
    full_packet.reserve(nonce.size() + packet->payload.size());

    full_packet.append(nonce);

    if (session_.key.empty()) {
        full_packet.append(reinterpret_cast<const char *>(packet->payload.data()),
                           packet->payload.size());
    } else {
        std::string encrypted_audio;
        encrypted_audio.resize(packet->payload.size());

        size_t nc_off = 0;
        uint8_t stream_block[16] = {0};

        if (mbedtls_aes_crypt_ctr(&aes_ctx_, packet->payload.size(), &nc_off,
                                  (uint8_t *)nonce.c_str(), stream_block,
                                  (uint8_t *)packet->payload.data(),
                                  (uint8_t *)encrypted_audio.data()) != 0) {
            ESP_LOGE(TAG, "Failed to encrypt audio data");
            return;
        }

        full_packet.append(encrypted_audio);
    }

    udp_->Send(full_packet);
}

void FbtPhoneTransport::OnError(const std::string &service, int error_code) {
    ESP_LOGE(TAG, "Error from service %s: %d", service.c_str(), error_code);
}

void FbtPhoneTransport::onStartPhone(const std::string &json_message) {
    ESP_LOGI(TAG, "Starting FBT audio session");
    if (rtc_state_ != IDLE) {
        return;
    }
    if (!display_) {
        return;
    }

    if (event_group_) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_START_SESSION);
    }

    if (!parse_config(json_message)) {
        ESP_LOGE(TAG, "Failed to parse configuration");
        ClosePhone();
        return;
    }

    rtc_state_ = session_.callType;
    phone_ui_.SetRtcState(rtc_state_);
    board_.SetPowerSaveMode(false);

    if (!connect_udp_server()) {
        ESP_LOGE(TAG, "Failed to connect to server");
        ClosePhone();
        return;
    }

    local_sequence_ = 0;
    remote_sequence_ = 0;

    if (audio_codec_) {
        audio_codec_->EnableInput(true);
        audio_codec_->EnableOutput(true);
    }
    auto &event_bus = FbtEventBus::GetInstance();
    char json_str[256];
    snprintf(json_str, sizeof(json_str), "{\"userType\":%d,\"name\":\"%s\"}", session_.callType, session_.name.c_str());
    event_bus.PublishAsync(FbtEvents::PHONE_STARTED, std::string(json_str));

    audio_repeater_->PlayRingtone([this]() -> bool {
        return rtc_state_ == INCOMING || rtc_state_ == OUTGOING;
    });

    phone_ui_.SetMessageValue(session_.name.c_str());
    // display_->SetChatMessage("system", session_.name.c_str());

    switch (session_.callType) {
        case OUTGOING:
            handle_call_out();
            break;
        case INCOMING:
            handle_call_in();
            break;
        default:
            // 处理其他情况
            ESP_LOGW(TAG, "未知的用户类型: %d", session_.callType);
            break;
    }

    ESP_LOGI(TAG, "FBT session started successfully, waiting for fbt_ok...");
}

void FbtPhoneTransport::OnSwitchAnswer() {
    if (!is_running_) {
        return;
    }
    switch (rtc_state_) {
        case IDLE:
            return;
        case INCOMING:
            OnAnswer();
            break;
        default:
            OnHangUp();
            break;
    }
}

bool FbtPhoneTransport::OnHangUp() {
    if (!is_running_) {
        return false;
    }
    if (rtc_state_ == IDLE) {
        return false;
    }
    ClosePhone();
    return true;
}

bool FbtPhoneTransport::OnAnswer() {
    if (!is_running_) {
        return false;
    }
    if (rtc_state_ == IDLE) {
        return false;
    }
    if (rtc_state_ == INCOMING) {
        if (!send_offer()) {
            ClosePhone();
        }
    }
    return true;
}

void FbtPhoneTransport::close() {
    if (rtc_state_ == IDLE) {
        return;
    }
    rtc_state_ = IDLE;
    session_.Clear();
    udp_.reset();
    audio_repeater_->InterruptRingtone();
    if (event_group_) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_STOP_SESSION);
    }
    phone_ui_.SetRtcState(rtc_state_);
    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.PublishAsync(FbtEvents::PHONE_CLOSED, "");
    ESP_LOGI(TAG, "FBT phone stopped");
}

void FbtPhoneTransport::ClosePhone() {
    if (!is_running_ || rtc_state_ == IDLE) {
        return;
    }
    send_bye();
    close();
    ESP_LOGI(TAG, "Send Mqtt Message ok");
}

// 解析配置
bool FbtPhoneTransport::parse_config(const std::string &json_message) {
    ESP_LOGI(TAG, "Parsing FBT configuration");

    cJSON *root = cJSON_Parse(json_message.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON message");
        return false;
    }

    // 清理配置
    session_.Clear();

    // 1. 解析session_id
    cJSON *session_id = cJSON_GetObjectItem(root, "sessionId");
    if (!cJSON_IsString(session_id)) {
        ESP_LOGE(TAG, "Missing or invalid session_id");
        cJSON_Delete(root);
        return false;
    }
    session_.session_id = session_id->valuestring;
    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (cJSON_IsString(name)) {
        session_.name = name->valuestring;
    }
    cJSON *id = cJSON_GetObjectItem(root, "deviceId");
    session_.deviceId = id->valuestring;

    // 获取userType字段
    cJSON *callType = cJSON_GetObjectItem(root, "callType");
    if (cJSON_IsNumber(callType)) {
        session_.callType = static_cast<FbtPhoneState>(callType->valueint);
    }

    // 2. 解析UDP配置
    cJSON *udp = cJSON_GetObjectItem(root, "server");
    if (!cJSON_IsObject(udp)) {
        ESP_LOGE(TAG, "Missing UDP configuration");
        cJSON_Delete(root);
        return false;
    }

    cJSON *serverAddr = cJSON_GetObjectItem(udp, "host");
    cJSON *port = cJSON_GetObjectItem(udp, "port");
    cJSON *key = cJSON_GetObjectItem(udp, "key");
    cJSON *nonce = cJSON_GetObjectItem(udp, "nonce");

    if (!cJSON_IsString(serverAddr) || !cJSON_IsNumber(port) ||
        !cJSON_IsString(nonce)) {
        ESP_LOGE(TAG, "Invalid UDP configuration fields");
        cJSON_Delete(root);
        return false;
    }

    session_.server_addr = serverAddr->valuestring;
    session_.server_port = port->valueint;
    session_.nonce = FbtConfig::Helper::DecodeHexString(nonce->valuestring);
    if (key && cJSON_IsString(key) && key->valuestring) {
        session_.key = key->valuestring;
        std::string aes_key = FbtConfig::Helper::DecodeHexString(key->valuestring);
        mbedtls_aes_setkey_enc(&aes_ctx_, (const unsigned char *)aes_key.c_str(), 128);
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "phone Config: session=%s, CallType=%d, name=%s,  server=%s:%d, sample_rate=%dHz",
             session_.session_id.c_str(), session_.callType, session_.name.c_str(), session_.server_addr.c_str(),
             session_.server_port, session_.sample_rate);

    return true;
}

// 连接到UDP服务器
bool FbtPhoneTransport::connect_udp_server() {
    // 创建UDP实例
    udp_ = CreateFbtUdp(3);

    if (!udp_) {
        ESP_LOGE(TAG, "Failed to create UDP instance");
        return false;
    }

    // 设置数据接收回调
    udp_->OnMessage([this](const std::string &data) {
        on_udp_packet(data);
    });

    // 连接到服务器
    if (!udp_->Connect(session_.server_addr, session_.server_port)) {
        ESP_LOGE(TAG, "Failed to connect to UDP server %s:%d",
                 session_.server_addr.c_str(), session_.server_port);
        udp_.reset();
        return false;
    }

    ESP_LOGI(TAG, "Connected to UDP server %s:%d", session_.server_addr.c_str(),
             session_.server_port);
    return true;
}

bool FbtPhoneTransport::send_offer() {
    if (!send_udp_message(FbtCommand::FBT_OFFER)) {
        return false;
    }

    return true;
}

void FbtPhoneTransport::send_bye() {
    std::string json_str = generate_json(FbtCommand::PHONE_BYE);
    if (json_str.empty()) {
        ESP_LOGE(TAG, "生成 JSON 失败，返回空字符串");
        return;
    }
    fbt_mqtt_->SendText(json_str);
}

// 发送控制消息
bool FbtPhoneTransport::send_udp_message(const std::string &type) {
    if (!udp_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(channel_mutex_);

    // 准备nonce数据
    std::string nonce(session_.nonce);
    nonce[0] = 0x00;
    std::string json_str = generate_json(type);
    if (json_str.empty())
        return false;

    std::string packet;
    size_t total_size = nonce.size() + json_str.size();
    packet.reserve(total_size);
    packet.append(nonce);
    packet.append(json_str);

    bool result = udp_->SendMust(packet) > 0;
    return result;
}

void FbtPhoneTransport::handle_call_out() {
    if (!send_offer()) {
        ESP_LOGE(TAG, "Failed to send phone ack");
        ClosePhone();
        return;
    }
    // phone_ui_.SetTypeValue(Lang::Strings::FBT_PHONE_RINGING);
}

void FbtPhoneTransport::handle_call_in() {
    // phone_ui_.SetTypeValue(Lang::Strings::FBT_PHONE_AUDIO_IN);
}

// 处理接收到的数据
void FbtPhoneTransport::on_udp_packet(const std::string &data) {
    if (data.empty()) {
        return;
    }

    uint8_t packet_type = static_cast<uint8_t>(data[0]);

    if (packet_type == 0x00) {
        handle_json(data.substr(1));
    } else if (packet_type == 0x01) {
        on_remote_audio(data);
    } else {
        ESP_LOGW(TAG, "Unknown packet type: 0x%02X", packet_type);
    }
}

void FbtPhoneTransport::start_call() {
    rtc_state_ = CONNECTING;
    phone_ui_.SetRtcState(rtc_state_);
    if (!audio_repeater_->InterruptRingtone()) {
        close();
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    if (event_group_) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO);
    }
    // 再次确认打开了麦克风跟杨声器
    if (audio_codec_) {
        audio_codec_->EnableInput(true);
        audio_codec_->EnableOutput(true);
    }
    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.PublishAsync(FbtEvents::PHONE_CALL_READY, "");
    rtc_state_ = CALLING;
    phone_ui_.SetRtcState(rtc_state_);
}

// 处理控制消息
void FbtPhoneTransport::handle_json(const std::string &json_data) {
    cJSON *root = cJSON_Parse(json_data.c_str());
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
        if (!load_media(json_data)) {
            cJSON_Delete(root);
            ClosePhone();
            return;
        }
        start_call();
        ESP_LOGI(TAG, "fbt ok Audio transmission started!");
    } else if (strcmp(msg_type, FbtCommand::PHONE_BYE) == 0) {
        ESP_LOGI(TAG, "fbt Server ended session");
        ClosePhone();
    }

    cJSON_Delete(root);
}

// 处理音频包
void FbtPhoneTransport::on_remote_audio(const std::string &data) {
    if (data.size() < session_.nonce.size()) {
        ESP_LOGE(TAG, "Packet too small: %zu < %zu", data.size(), session_.nonce.size());
        return;
    }

    // 解析包头（无论是否加密都需要）
    if (data[0] != 0x01) {
        ESP_LOGE(TAG, "Invalid audio packet type: 0x%02X", data[0]);
        return;
    }

    // 从包头中提取信息
    uint16_t payload_size = ntohs(*(uint16_t *)&data[2]);
    uint32_t timestamp = ntohl(*(uint32_t *)&data[8]);
    uint32_t sequence = ntohl(*(uint32_t *)&data[12]);

    // 检查序列号
    if (sequence < remote_sequence_) {
        return;
    }

    // 验证包大小
    size_t expected_size = session_.nonce.size() + payload_size;
    if (data.size() != expected_size) {
        /* ESP_LOGE(TAG, "Packet size mismatch: got %zu, expected %zu",
                 data.size(), expected_size); */
        return;
    }

    // 创建音频包
    auto packet = std::make_unique<AudioStreamPacket>();
    packet->sample_rate = session_.sample_rate;
    packet->frame_duration = session_.frame_duration;
    packet->timestamp = timestamp;
    packet->payload.resize(payload_size);

    // 提取音频数据部分
    const uint8_t *audio_data = reinterpret_cast<const uint8_t *>(data.data()) + session_.nonce.size();

    // 检查是否需要解密
    if (session_.key.empty()) {
        memcpy(packet->payload.data(), audio_data, payload_size);
    } else {
        // 有key，需要解密
        size_t nc_off = 0;
        uint8_t stream_block[16] = {0};

        int ret = mbedtls_aes_crypt_ctr(&aes_ctx_, payload_size, &nc_off, (uint8_t *)data.data(), stream_block, audio_data, packet->payload.data());

        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to decrypt audio data, ret: %d", ret);
            return;
        }
    }

    // 更新序列号
    remote_sequence_ = sequence;

    audio_repeater_->PlayStream(std::move(packet));
}

std::string FbtPhoneTransport::generate_json(const std::string &type) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return "";
    }

    cJSON_AddStringToObject(root, "type", type.c_str());
    cJSON_AddStringToObject(root, "deviceId", session_.deviceId.c_str());
    cJSON_AddStringToObject(root, "sessionId", session_.session_id.c_str());

    if (type == FbtCommand::FBT_OFFER) {
        cJSON *audio = FbtConfig::FbtBuilder::buildAudioMedia();
        if (audio)
            cJSON_AddItemToObject(root, "audio", audio);
    }

    // 提取需要的信息
    char *json_str = cJSON_PrintUnformatted(root);
    std::string result;
    if (json_str) {
        result = json_str;
        free(json_str); // 释放cJSON分配的内存
    }
    ESP_LOGD(TAG, "generate json: %s", result.c_str());
    cJSON_Delete(root); // 释放cJSON对象
    return result;
}

bool FbtPhoneTransport::load_media(const std::string &json_data) {
    cJSON *root = cJSON_Parse(json_data.c_str());
    if (!root) {
        return false;
    }
    cJSON *audio_params = cJSON_GetObjectItem(root, "audio");
    if (cJSON_IsObject(audio_params)) {
        cJSON *sample_rate = cJSON_GetObjectItem(audio_params, "sampleRate");
        cJSON *frame_duration = cJSON_GetObjectItem(audio_params, "frameDuration");
        if (cJSON_IsNumber(sample_rate))
            session_.sample_rate = sample_rate->valueint;
        if (cJSON_IsNumber(frame_duration))
            session_.frame_duration = frame_duration->valueint;
    }
    cJSON_Delete(root);
    return true;
}
