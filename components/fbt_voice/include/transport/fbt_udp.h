#ifndef FBT_UDP_H
#define FBT_UDP_H

#include "board.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class FbtUdp : public Udp {
  private:
    std::unique_ptr<Udp> udp_;
    int max_retries_;
    int retries_ = 0;
    std::string payload_;
    esp_timer_handle_t timeout_timer_ = nullptr;
    bool start_timeout_timer = false;

    void resend_send();

    void start_timer();
    void stop_timer();

    void send_ack();

    static void VoiceTimeoutHandler(void *arg) {
        FbtUdp *server = static_cast<FbtUdp *>(arg);
        server->resend_send();
    }

    TaskHandle_t high_priority_task_ = nullptr;
    SemaphoreHandle_t ack_semaphore_ = nullptr;
    std::string ack_packet_;
    bool task_running_ = false;

    /* static void high_priority_ack_task(void *arg) {
        FbtUdp *self = static_cast<FbtUdp *>(arg);

        while (self->task_running_) {
            // 等待信号量
            if (xSemaphoreTake(self->ack_semaphore_, portMAX_DELAY) == pdTRUE) {
                if (self->udp_) {
                    self->udp_->Send(self->ack_packet_);
                } else {
                    return;
                }
            }
        }
        // 任务正常退出
        vTaskDelete(NULL);
    } */

  public:
    FbtUdp(std::unique_ptr<Udp> udp, int max_retries = 3);
    ~FbtUdp();

    // 实现Udp的所有纯虚函数
    bool Connect(const std::string &host, int port) override;
    void Disconnect() override;
    int Send(const std::string &data) override;
    int GetLastError() override;
    void OnMessage(std::function<void(const std::string &data)> callback) override;
    /**
     * 发送必达消息
     */
    bool SendMust(const std::string &payload);
};

// 创建函数
std::unique_ptr<FbtUdp> CreateFbtUdp(int connect_id, int max_retries = 3);

#endif // FBT_UDP_H
