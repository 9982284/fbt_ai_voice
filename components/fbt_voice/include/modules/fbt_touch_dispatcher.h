#pragma once
#include <algorithm>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <functional>
#include <vector>

// 触摸点结构
struct TouchPoint {
    int16_t x;
    int16_t y;
    uint8_t pressure;
};

// 简单的锁包装器
class MutexLock {
  public:
    explicit MutexLock(SemaphoreHandle_t mutex) : mutex_(mutex) {
        if (mutex_)
            xSemaphoreTake(mutex_, portMAX_DELAY);
    }

    ~MutexLock() {
        if (mutex_)
            xSemaphoreGive(mutex_);
    }

  private:
    SemaphoreHandle_t mutex_;
};

// 触摸分发器
class FbtTouchDispatcher {
  public:
    static FbtTouchDispatcher &Instance() {
        static FbtTouchDispatcher instance;
        return instance;
    }

    // 初始化
    bool Init();

    // 注册UI监听器
    int AddListener(std::function<bool(const TouchPoint &)> on_down,
                    std::function<void(const TouchPoint &)> on_up = nullptr,
                    std::function<void(const TouchPoint &, const TouchPoint &)> on_move = nullptr);

    // 移除监听器
    void RemoveListener(int listener_id);

    // 触摸事件入队
    bool EnqueueTouchDown(const TouchPoint &point);

    bool EnqueueTouchUp(const TouchPoint &point);

    bool EnqueueTouchMove(const TouchPoint &last, const TouchPoint &current);

    // 调试信息
    size_t GetListenerCount() const {
        MutexLock lock(listener_mutex_);
        return listeners_.size();
    }

    size_t GetQueueSize() const {
        return uxQueueMessagesWaiting(event_queue_);
    }

    // 清理资源
    void Deinit();

  private:
    FbtTouchDispatcher() : next_listener_id_(1) {}

    ~FbtTouchDispatcher() {
        Deinit();
    }

    // 禁止拷贝
    FbtTouchDispatcher(const FbtTouchDispatcher &) = delete;
    FbtTouchDispatcher &operator=(const FbtTouchDispatcher &) = delete;

    enum EventType {
        EVENT_DOWN,
        EVENT_UP,
        EVENT_MOVE
    };

    struct TouchEvent {
        EventType type;
        TouchPoint point1;
        TouchPoint point2;
    };

    struct TouchListener {
        int id;
        std::function<bool(const TouchPoint &)> on_down;
        std::function<void(const TouchPoint &)> on_up;
        std::function<void(const TouchPoint &, const TouchPoint &)> on_move;
        bool active;
    };

    // 工作线程
    static void worker_task(void *arg) {
        FbtTouchDispatcher *self = static_cast<FbtTouchDispatcher *>(arg);
        TouchEvent event;

        while (1) {
            if (xQueueReceive(self->event_queue_, &event, portMAX_DELAY) == pdTRUE) {
                self->process_event(event);
            }
        }
    }

    // 处理单个事件
    void process_event(const TouchEvent &event);

    void process_touch_down(const TouchPoint &point);

    void process_touch_up(const TouchPoint &point);

    void process_touch_move(const TouchPoint &last, const TouchPoint &current);

    // 成员变量
    std::vector<TouchListener> listeners_;
    SemaphoreHandle_t listener_mutex_ = nullptr;
    QueueHandle_t event_queue_ = nullptr;
    TaskHandle_t worker_task_ = nullptr;
    int next_listener_id_;
    bool initialized_ = false;
};
