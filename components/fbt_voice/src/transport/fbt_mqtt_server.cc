// fbt_mqtt_server.cpp
#include "fbt_mqtt_server.h"
#include "application.h"
#include "assets/lang_config.h"
#include "board.h"
#include "cJSON.h"
#include "fbt_constants.h"
#include "fbt_event_bus.h"
#include "ml307_board.h"
#include "settings.h"
#include <arpa/inet.h>
#include <at_uart.h>
#include <cstring>
#include <esp_log.h>

#define TAG "fbt_mqtt"

FbtMqttServer::FbtMqttServer()
    : is_active_(false) {

    auto &event_bus = FbtEventBus::GetInstance();

    event_bus.Subscribe("MQTT_Server", FbtEvents::SEND_TO_SERVER, [this](const std::string &type, const std::string &data) {
        ESP_LOGI(TAG, "Subscribe mqtt to server : %s", data.c_str());
        this->SendText(data);
    });
}

FbtMqttServer::~FbtMqttServer() {
    Stop();
}

bool FbtMqttServer::Start() {
    ESP_LOGI(TAG, "fbt Mqtt starting...");

    // 如果已经启动，先停止
    if (is_active_) {
        ESP_LOGW(TAG, "MQTT already active, restarting...");
        Stop();
    }

    Settings settings("fbt_mqtt", true);

    std::string server_host = settings.GetString("host");
    int server_port = settings.GetInt("port");
    std::string client_id = settings.GetString("clientId");
    std::string username = settings.GetString("username");
    std::string password = settings.GetString("password");

    if (server_host.empty() || !server_port) {
        ESP_LOGI(TAG, "Failed to connect to endpoint");
        return false;
    }

    int keepalive_interval = 180;
    // 创建 MQTT 客户端并连接
    auto network = Board::GetInstance().GetNetwork();
    mqtt_ = network->CreateMqtt(mqtt_id_);
    mqtt_->SetKeepAlive(keepalive_interval);

    // 设置回调函数
    mqtt_->OnDisconnected([this]() {
        ESP_LOGI(TAG, "Disconnected from endpoint");
        is_active_ = false;
    });

    mqtt_->OnMessage([this](const std::string &topic, const std::string &payload) {
        ESP_LOGI(TAG, "Received MQTT: topic=%s, len=%d", topic.c_str(), payload.length());
        if (!is_active_) {
            is_active_ = true;
        }
        this->on_mqtt_message(topic, payload);
    });

    mqtt_->OnConnected([this]() {
        ESP_LOGI(TAG, "Connected");
        if (!is_active_) {
            is_active_ = true;
            handle_event_bus(FbtEvents::MQTT_VOICE_PING, "");
        }
        if (Board::GetInstance().GetBoardType() == "ML307") {
            if (auto *at_modem = dynamic_cast<AtModem *>(Board::GetInstance().GetNetwork())) {
                at_modem->GetAtUart()->SendCommand("AT+MQTTCFG=\"pingreq\"," + std::to_string(mqtt_id_) + ",50");
            }
        }
    });

    // 连接 MQTT 服务器
    if (!mqtt_->Connect(server_host, server_port, client_id, username, password)) {
        ESP_LOGE(TAG, "Failed to connect to endpoint");
        is_active_ = false;
        return false;
    } else {
        ESP_LOGI(TAG, "MQTT server started successfully");
    }

    return true;
}

void FbtMqttServer::on_mqtt_message(const std::string &topic, const std::string &payload) {
    if (topic == topic_json_) {
        handle_json_payload(payload);
    } else if (topic == topic_voice_) {
        handle_event_bus(FbtEvents::MQTT_VOICE_PAYLOAD, payload);
    } else if (topic == topic_voice_ping_) {
        handle_event_bus(FbtEvents::MQTT_VOICE_PING, payload);
    } else {
        ESP_LOGW(TAG, "未知主题: %s", topic.c_str());
    }
}

void FbtMqttServer::handle_json_payload(const std::string &payload) {
    cJSON *root = cJSON_Parse(payload.c_str());
    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }
    ESP_LOGI(TAG, "mqtt message: %s", payload.c_str());
    // 提取消息类型
    cJSON *type_item = cJSON_GetObjectItem(root, "type");
    if (!type_item || !cJSON_IsString(type_item)) {
        ESP_LOGE(TAG, "Missing 'type' field in JSON");
        cJSON_Delete(root);
        return;
    }

    const char *type_str = type_item->valuestring;
    ESP_LOGI(TAG, "Processing message type: %s", type_str);

    // 分发处理
    if (strcmp(type_str, FbtCommand::PHONE_CALL) == 0) {
        handle_event_bus(FbtEvents::START_PHONE, payload);
        // HandlePhoneCall(root);
    } else if (strcmp(type_str, FbtCommand::PHONE_BYE) == 0) {
        handle_event_bus(FbtEvents::CLOSE_PHONE, payload);
        // HandlePhoneClose(root);
    } else if (strcmp(type_str, FbtCommand::ENTER_INTERCOM_ROOM) == 0) {
        handle_event_bus(FbtEvents::MQTT_ENTER_INTERCOM_ROOM, payload);
    } else {
        ESP_LOGW(TAG, "Unknown message type: %s", type_str);
    }

    cJSON_Delete(root);
}

void FbtMqttServer::async_send_text(void *arg) {
    AsyncSendJson *self = static_cast<AsyncSendJson *>(arg);
    if (self && self->instance) {
        for (int i = 0; i < 5; i++) {
            if (self->instance->mqtt_->Publish(topic_json_, self->message, 1)) {
                break;
            } else {
                ESP_LOGE(TAG, "Failed to publish message %d", i);
            };
        }
    }
    delete self;
}

void FbtMqttServer::handle_event_bus(const std::string &type, const std::string &json) {
    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.PublishAsync(type, json);
}

void FbtMqttServer::SendText(const std::string &json_str) {
    if (!is_active_ || !mqtt_) {
        ESP_LOGW(TAG, "Cannot send, MQTT not active");
        return;
    }

    if (topic_json_.empty()) {
        ESP_LOGE(TAG, "Publish topic not set");
        return;
    }

    ESP_LOGI(TAG, "Sending to server: %s", json_str.c_str());

    if (!mqtt_->Publish(topic_json_, json_str, 1)) {
        ESP_LOGE(TAG, "Failed to publish message");
        return;
    }
    ESP_LOGI(TAG, "send mqtt message ok");
    return;
}

void FbtMqttServer::Stop() {
    if (!is_active_)
        return;

    ESP_LOGI(TAG, "Stopping MQTT server");

    is_active_ = false;
    if (mqtt_) {
        mqtt_->Disconnect();
        mqtt_.reset();
    }

    ESP_LOGI(TAG, "MQTT server stopped");
}