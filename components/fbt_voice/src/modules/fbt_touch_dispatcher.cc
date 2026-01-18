#include "fbt_touch_dispatcher.h"

bool FbtTouchDispatcher::Init() {
    if (initialized_)
        return true;

    // 创建事件队列
    event_queue_ = xQueueCreate(8, sizeof(TouchEvent));
    if (!event_queue_)
        return false;

    // 创建互斥锁
    listener_mutex_ = xSemaphoreCreateMutex();
    if (!listener_mutex_) {
        vQueueDelete(event_queue_);
        return false;
    }

    // 创建工作线程
    if (xTaskCreate(worker_task, "touch_worker", 2048, this, 2, &worker_task_) != pdPASS) {
        vSemaphoreDelete(listener_mutex_);
        vQueueDelete(event_queue_);
        return false;
    }

    initialized_ = true;
    return true;
}
int FbtTouchDispatcher::AddListener(std::function<bool(const TouchPoint &)> on_down, std::function<void(const TouchPoint &)> on_up, std::function<void(const TouchPoint &, const TouchPoint &)> on_move) {
    MutexLock lock(listener_mutex_);

    int id = next_listener_id_++;
    listeners_.push_back({id,
                          std::move(on_down),
                          std::move(on_up),
                          std::move(on_move),
                          true});

    return id;
}

void FbtTouchDispatcher::RemoveListener(int listener_id) {
    MutexLock lock(listener_mutex_);

    for (auto &listener : listeners_) {
        if (listener.id == listener_id) {
            listener.active = false;
            break;
        }
    }
}

bool FbtTouchDispatcher::EnqueueTouchDown(const TouchPoint &point) {
    if (!initialized_)
        return false;

    TouchEvent event;
    event.type = EVENT_DOWN;
    event.point1 = point;

    return xQueueSend(event_queue_, &event, 0) == pdTRUE;
}

bool FbtTouchDispatcher::EnqueueTouchUp(const TouchPoint &point) {
    if (!initialized_)
        return false;

    TouchEvent event;
    event.type = EVENT_UP;
    event.point1 = point;

    return xQueueSend(event_queue_, &event, 0) == pdTRUE;
}

bool FbtTouchDispatcher::EnqueueTouchMove(const TouchPoint &last, const TouchPoint &current) {
    if (!initialized_)
        return false;

    TouchEvent event;
    event.type = EVENT_MOVE;
    event.point1 = last;
    event.point2 = current;

    return xQueueSend(event_queue_, &event, 0) == pdTRUE;
}

void FbtTouchDispatcher::Deinit() {
    if (worker_task_) {
        vTaskDelete(worker_task_);
        worker_task_ = nullptr;
    }
    if (event_queue_) {
        vQueueDelete(event_queue_);
        event_queue_ = nullptr;
    }
    if (listener_mutex_) {
        vSemaphoreDelete(listener_mutex_);
        listener_mutex_ = nullptr;
    }
    listeners_.clear();
    initialized_ = false;
}
void FbtTouchDispatcher::process_event(const TouchEvent &event) {
    MutexLock lock(listener_mutex_);

    // 清理失效的监听器
    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
                       [](const TouchListener &l) { return !l.active; }),
        listeners_.end());

    // 分发事件
    switch (event.type) {
        case EVENT_DOWN:
            process_touch_down(event.point1);
            break;
        case EVENT_UP:
            process_touch_up(event.point1);
            break;
        case EVENT_MOVE:
            process_touch_move(event.point1, event.point2);
            break;
    }
}

void FbtTouchDispatcher::process_touch_down(const TouchPoint &point) {

    for (auto &listener : listeners_) {

        if (listener.active && listener.on_down) {
            bool handled = listener.on_down(point);
            if (handled) {
                break;
            }
        }
    }
}

void FbtTouchDispatcher::process_touch_up(const TouchPoint &point) {
    for (auto &listener : listeners_) {
        if (listener.active && listener.on_up) {
            listener.on_up(point);
        }
    }
}

void FbtTouchDispatcher::process_touch_move(const TouchPoint &last, const TouchPoint &current) {
    for (auto &listener : listeners_) {
        if (listener.active && listener.on_move) {
            listener.on_move(last, current);
        }
    }
}