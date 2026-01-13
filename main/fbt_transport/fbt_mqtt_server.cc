// fbt_mqtt_server.cpp
#include "fbt_mqtt_server.h"
#include "application.h"
#include "assets/lang_config.h"
#include "board.h"
#include "cJSON.h"
#include "fbt_constants.h"
#include "fbt_event_bus.h"
#include "settings.h"
#include <arpa/inet.h>
#include <cstring>
#include <esp_log.h>
#define TAG "fbt_mqtt"

FbtMqttServer::FbtMqttServer() : is_active_(false) {

    auto &event_bus = FbtEventBus::GetInstance();

    event_bus.Subscribe("MQTT_Server", FbtEvents::SEND_TO_SERVER, [this](const std::string &type, const std::string &data) {
        ESP_LOGI(TAG, "Subscribe mqtt to server : %s", data.c_str());
        this->SendText(data);
    });
}

FbtMqttServer::~FbtMqttServer() {
    Stop();
}

bool FbtMqttServer::Start(std::string data) {
    ESP_LOGI(TAG, "fbt Mqtt starting...");

    // 如果已经启动，先停止
    if (is_active_) {
        ESP_LOGW(TAG, "MQTT already active, restarting...");
        Stop();
    }

    // 解析传入的 JSON 数据
    cJSON *root = cJSON_Parse(data.c_str());
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return false; // 修正：需要返回 false
    }

    // 从 data 参数中获取 mqtt 配置
    cJSON *fbt_mqtt = cJSON_GetObjectItem(root, "mqtt");
    if (!cJSON_IsObject(fbt_mqtt)) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Invalid mqtt configuration in JSON");
        return false;
    }

    // 从 JSON 中提取配置项
    cJSON *server_host_json = cJSON_GetObjectItem(fbt_mqtt, "host");
    cJSON *server_port_json = cJSON_GetObjectItem(fbt_mqtt, "port");
    cJSON *client_id_json = cJSON_GetObjectItem(fbt_mqtt, "clientId");
    cJSON *username_json = cJSON_GetObjectItem(fbt_mqtt, "username");
    cJSON *password_json = cJSON_GetObjectItem(fbt_mqtt, "password");
    // cJSON *keepalive_interval_json = cJSON_GetObjectItem(fbt_mqtt, "keepalive");
    cJSON *topics_json = cJSON_GetObjectItem(fbt_mqtt, "topics");

    // 验证必要参数
    if (!server_host_json || !cJSON_IsString(server_host_json)) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "MQTT host is missing or invalid");
        return false;
    }

    if (!topics_json || !cJSON_IsArray(topics_json)) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Topics configuration is missing or invalid");
        return false;
    }

    // 提取字符串值
    std::string server_host = server_host_json->valuestring;
    std::string client_id = "";
    std::string username = "";
    std::string password = "";

    // 设置默认值
    int server_port = 1883;
    int keepalive_interval = 60;

    // 检查并解析可选参数
    if (server_port_json && cJSON_IsNumber(server_port_json)) {
        server_port = server_port_json->valueint;
    }

    if (client_id_json && cJSON_IsString(client_id_json)) {
        client_id = client_id_json->valuestring;
    }

    if (username_json && cJSON_IsString(username_json)) {
        username = username_json->valuestring;
    }

    if (password_json && cJSON_IsString(password_json)) {
        password = password_json->valuestring;
    }

    /*   if (keepalive_interval_json && cJSON_IsNumber(keepalive_interval_json)) {
          keepalive_interval = keepalive_interval_json->valueint;
      } */

    // 验证必要参数
    if (server_host.empty()) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "fbt Mqtt not configured: host is empty");
        return false;
    }

    // 释放 JSON 内存
    cJSON_Delete(root);

    if (topic_json_.empty()) {
        ESP_LOGE(TAG, "Cannot get topics: JSON is empty");
        return false;
    }

    // 创建 MQTT 客户端并连接
    auto network = Board::GetInstance().GetNetwork();
    mqtt_ = network->CreateMqtt(2);
    mqtt_->SetKeepAlive(keepalive_interval);

    // 设置回调函数
    mqtt_->OnDisconnected([this]() {
        ESP_LOGI(TAG, "Disconnected from endpoint");
        is_active_ = false;
    });

    mqtt_->OnMessage([this](const std::string &topic, const std::string &payload) {
        ESP_LOGI(TAG, "Received MQTT: topic=%s, len=%zu", topic.c_str(), payload.length());
        this->OnMqttMessage(topic, payload);
    });

    mqtt_->OnConnected([this]() {
        ESP_LOGI(TAG, "Connected");
        if (!is_active_) {
            is_active_ = true;
            HandleEventBus(FbtEvents::MQTT_VOICE_PING, "");
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

void FbtMqttServer::OnMqttMessage(const std::string &topic, const std::string &payload) {
    if (topic == topic_json_) {
        HandleJsonPayload(payload);
    } else if (topic == topic_voice_) {
        HandleEventBus(FbtEvents::MQTT_VOICE_PAYLOAD, payload);
    } else if (topic == topic_voice_ping_) {
        HandleEventBus(FbtEvents::MQTT_VOICE_PING, payload);
    } else {
        ESP_LOGW(TAG, "未知主题: %s", topic.c_str());
    }
}

void FbtMqttServer::HandleJsonPayload(const std::string &payload) {
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
        HandleEventBus(FbtEvents::START_PHONE, payload);
        // HandlePhoneCall(root);
    } else if (strcmp(type_str, FbtCommand::PHONE_BYE) == 0) {
        HandleEventBus(FbtEvents::CLOSE_PHONE, payload);
        // HandlePhoneClose(root);
    } else if (strcmp(type_str, FbtCommand::ENTER_INTERCOM_ROOM) == 0) {
        HandleEventBus(FbtEvents::MQTT_ENTER_INTERCOM_ROOM, payload);
    } else {
        ESP_LOGW(TAG, "Unknown message type: %s", type_str);
    }

    cJSON_Delete(root);
}

void FbtMqttServer::HandlePhoneCall(cJSON *root) {
    ESP_LOGI(TAG, "Handling phone call event");

    // 提取需要的信息
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str)
        return;

    HandleEventBus(FbtEvents::START_PHONE, json_str);

    cJSON_free(json_str);
}

void FbtMqttServer::HandlePhoneClose(cJSON *root) {
    ESP_LOGI(TAG, "Handling phone close event");

    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str)
        return;

    HandleEventBus(FbtEvents::CLOSE_PHONE, json_str);
    cJSON_free(json_str);
}

void FbtMqttServer::HandleEventBus(const std::string &type, const std::string &json) {
    if (!is_active_ || !mqtt_) {
        ESP_LOGW(TAG, "MQTT not active, cannot send");
        return;
    }
    auto &event_bus = FbtEventBus::GetInstance();
    event_bus.PublishAsync(type, json);
}

bool FbtMqttServer::SendText(const std::string &json_str) {
    if (!is_active_ || !mqtt_) {
        ESP_LOGW(TAG, "Cannot send, MQTT not active");
        return false;
    }

    if (topic_json_.empty()) {
        ESP_LOGE(TAG, "Publish topic not set");
        return false;
    }

    ESP_LOGI(TAG, "Sending to server: %s", json_str.c_str());

    if (!mqtt_->Publish(topic_json_, json_str)) {
        ESP_LOGE(TAG, "Failed to publish message");
        return false;
    }
    ESP_LOGI(TAG, "send mqtt message ok");
    return true;
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