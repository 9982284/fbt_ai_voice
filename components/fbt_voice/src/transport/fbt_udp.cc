#include "fbt_udp.h"

#include <esp_log.h>

#define TAG "fbt_udp"

// 创建函数实现
std::unique_ptr<FbtUdp> CreateFbtUdp(int connect_id, int max_retries) {
    auto network = Board::GetInstance().GetNetwork();
    if (!network) {
        ESP_LOGE(TAG, "Network interface not available");
        return nullptr;
    }
    auto udp = network->CreateUdp(connect_id);
    if (!udp)
        return nullptr;

    return std::make_unique<FbtUdp>(std::move(udp), max_retries);
}

FbtUdp::FbtUdp(std::unique_ptr<Udp> udp, int max_retries)
    : udp_(std::move(udp)), max_retries_(max_retries) {
    // 准备ACK包
    /* ack_packet_.push_back(static_cast<char>(0x05));

    // 创建二进制信号量
    ack_semaphore_ = xSemaphoreCreateBinary();
    task_running_ = true;
    // 创建高优先级任务（优先级比接收任务高）
    xTaskCreate(high_priority_ack_task, "ack_high", 2048, this, 10, &high_priority_task_); */
}

FbtUdp::~FbtUdp() {
    if (timeout_timer_) {
        esp_timer_stop(timeout_timer_);
        esp_timer_delete(timeout_timer_);
        timeout_timer_ = nullptr;
    }
    /* task_running_ = false;

    // 唤醒任务让其退出
    if (ack_semaphore_) {
        xSemaphoreGive(ack_semaphore_);
        vTaskDelay(pdMS_TO_TICKS(50)); // 给时间退出
    }

    if (high_priority_task_) {
        vTaskDelete(high_priority_task_); // 确保删除
        high_priority_task_ = nullptr;
    }

    if (ack_semaphore_) {
        vSemaphoreDelete(ack_semaphore_);
        ack_semaphore_ = nullptr;
    } */
    ESP_LOGI(TAG, "FbtUdp destroyed");
}

bool FbtUdp::Connect(const std::string &host, int port) {
    bool result = udp_->Connect(host, port);
    connected_ = udp_->connected();
    return result;
}

void FbtUdp::Disconnect() {
    udp_->Disconnect();
    connected_ = false;
}

int FbtUdp::Send(const std::string &data) {
    return udp_->Send(data);
}

int FbtUdp::GetLastError() {
    return udp_->GetLastError();
}

void FbtUdp::OnMessage(std::function<void(const std::string &data)> callback) {
    // 保存回调
    message_callback_ = std::move(callback);

    udp_->OnMessage([this](const std::string &data) {
        uint8_t packet_type = static_cast<uint8_t>(data[0]);
        if (packet_type == 0x05) {
            stop_timer();
            if (!payload_.empty()) {
                payload_.clear();
            }
            retries_ = 0;
        } else {
            if (message_callback_) {
                message_callback_(data);
            }
            if (packet_type == 0x00) {
                send_ack();
            }
        }
    });
}

bool FbtUdp::SendMust(const std::string &payload) {
    if (!connected_) {
        ESP_LOGE(TAG, "SendMust failed: not connected");
        return false;
    }

    payload_ = payload;
    retries_ = 0;
    start_timer();
    return Send(payload);
}

void FbtUdp::resend_send() {
    if (!payload_.empty()) {
        if (retries_ >= max_retries_) {
            payload_.clear();
            ESP_LOGE(TAG, "Message sending failed, exceeding retry attempts");
            return;
        }

        Send(payload_);
        retries_++;
        start_timer();
    }
}

void FbtUdp::start_timer() {
    if (!timeout_timer_) {
        // 第一次创建
        esp_timer_create_args_t timer_args = {
            .callback = &VoiceTimeoutHandler,
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "fbt_udp_timeout"};
        esp_timer_create(&timer_args, &timeout_timer_);
    }
    start_timeout_timer = true;
    // 启动定时器（如果是已经存在的就重新启动）
    esp_timer_start_once(timeout_timer_, 2000000); // 3秒
}

void FbtUdp::stop_timer() {
    if (timeout_timer_ && start_timeout_timer) {
        esp_timer_stop(timeout_timer_);
    }
}

void FbtUdp::send_ack() {
    // xSemaphoreGiveFromISR(ack_semaphore_, NULL);
    static TaskHandle_t ack_task = nullptr;
    xTaskCreate([](void *arg) {
        FbtUdp *self = static_cast<FbtUdp *>(arg);
        if (self->udp_) {
            std::string packet(1, static_cast<char>(0x05));
            self->udp_->Send(packet);
        }
        vTaskDelete(NULL); }, "ack_once", 1536, this, 1, &ack_task);
}
