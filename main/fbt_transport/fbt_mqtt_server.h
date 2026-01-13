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
    bool Start(std::string data);
    void Stop();

    // 发送消息
    bool SendText(const std::string &json_str);

    // 获取状态
    bool IsActive() const { return is_active_; }

  private:
    void OnMqttMessage(const std::string &topic, const std::string &payload);
    void HandleJsonPayload(const std::string &payload);
    void HandlePhoneCall(cJSON *root);
    void HandlePhoneClose(cJSON *root);
    // void HandlePhoneOk(cJSON *root);

    // 事件总线处理
    void HandleEventBus(const std::string &type, const std::string &json);

  private:
    std::shared_ptr<Mqtt> mqtt_;

    std::string topic_json_ = "json";

    std::string topic_voice_ = "voice";

    std::string topic_voice_ping_ = "voice-ping";
    /**
     * 当前是否活跃
     */
    bool is_active_ = false;

    // 事件总线订阅token
    void *event_subscription_ = nullptr;
};

#endif