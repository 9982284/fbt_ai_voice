// fbt_event_bus.h
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <esp_log.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifndef FBT_EVENT_BUS_TAG
#define FBT_EVENT_BUS_TAG "fbt_event_bus"
#endif

class FbtEventBus {
  public:
    using EventHandler = std::function<void(const std::string &type, const std::string &data)>;

    static FbtEventBus &GetInstance() {
        static FbtEventBus instance;
        return instance;
    }

    // 订阅事件

    void Subscribe(const std::string &subscriber, const std::string &event_type, EventHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.push_back({subscriber, event_type, handler});
        ESP_LOGI(FBT_EVENT_BUS_TAG, "%s subscribed to %s", subscriber.c_str(), event_type.c_str());
    }

    // 发布事件
    void Publish(const std::string &event_type, const std::string &data) {
        std::lock_guard<std::mutex> lock(mutex_);

        // ESP_LOGI(FBT_EVENT_BUS_TAG, "Event published: %s", event_type.c_str());

        for (auto &[subscriber, type, handler] : handlers_) {
            if (type == event_type || type == "*") { // * 表示通配符
                if (handler) {
                    try {
                        handler(event_type, data);
                    } catch (const std::exception &e) {
                        ESP_LOGE(FBT_EVENT_BUS_TAG, "Handler %s error: %s", subscriber.c_str(), e.what());
                    }
                }
            }
        }
    }
    void PublishAsync(const std::string &event_type, const std::string &data) {
        if (xTaskCreate(
                [](void *param) {
                    auto *args = static_cast<std::pair<std::string, std::string> *>(param);
                    auto &event_bus = FbtEventBus::GetInstance();
                    event_bus.Publish(args->first, args->second);
                    delete args;
                    vTaskDelete(NULL);
                },
                "fbt_evt_publish",
                4096,
                new std::pair<std::string, std::string>(event_type, data),
                1,
                nullptr) != pdPASS) {
            ESP_LOGE(FBT_EVENT_BUS_TAG, "创建异步发布任务失败");
            // 降级为同步发布
            Publish(event_type, data);
        }
    }

  private:
    struct HandlerInfo {
        std::string subscriber;
        std::string event_type;
        EventHandler handler;
    };

    std::mutex mutex_;
    std::vector<HandlerInfo> handlers_;
};