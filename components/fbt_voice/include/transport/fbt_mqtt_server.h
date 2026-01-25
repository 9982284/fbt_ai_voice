#ifndef FBT_MQTT_SERVER_H
#define FBT_MQTT_SERVER_H

#pragma once
#include <cJSON.h>
#include <functional>
#include <memory>
#include <mqtt.h>
#include <string>

class FbtMqttServer {
  public:
    FbtMqttServer();
    ~FbtMqttServer();

    // 启动停止
    bool Start();
    void Stop();

    // 发送消息
    void SendText(const std::string &json_str);

    // 获取状态
    bool IsActive() const { return is_active_; }

  private:
    void on_mqtt_message(const std::string &topic, const std::string &payload);
    void handle_json_payload(const std::string &payload);

    // void HandlePhoneOk(cJSON *root);

    // 事件总线处理
    void handle_event_bus(const std::string &type, const std::string &json);

    void async_send_text(void *arg);

  private:
    std::shared_ptr<Mqtt> mqtt_;

    int mqtt_id_ = 2;

    std::string topic_json_ = "json";

    std::string topic_voice_ = "voice";

    std::string topic_voice_ping_ = "voice-ping";
    /**
     * 当前是否活跃
     */
    bool is_active_ = false;

    // 事件总线订阅token
    void *event_subscription_ = nullptr;

    // 异步参数结构
    struct AsyncSendJson {
        FbtMqttServer *instance;
        std::string message;
    };
};

#endif