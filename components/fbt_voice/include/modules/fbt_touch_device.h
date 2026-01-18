#ifndef FBT_TOUCH_DEVICE_H
#define FBT_TOUCH_DEVICE_H

#include "fbt_touch_dispatcher.h"
#include <driver/i2c_master.h>
#include <esp_lcd_touch_cst816s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <functional>
#include <lvgl.h>

struct TouchParam {
    gpio_num_t gpio_sda = GPIO_NUM_NC;
    gpio_num_t gpio_scl = GPIO_NUM_NC;
    gpio_num_t gpio_rst = GPIO_NUM_NC;
    gpio_num_t gpio_int = GPIO_NUM_NC;
    uint16_t display_width = 0;
    uint16_t display_height = 0;
    int offset_x = 0;
    int offset_y = 0;
    bool mirror_x = false;
    bool mirror_y = false;
    bool swap_xy = false;
};

class FbtTouchDevice {
  public:
    FbtTouchDevice();
    ~FbtTouchDevice();

    bool Initialize(const TouchParam &param);

    // 获取LVGL输入设备指针
    lv_indev_t *GetLVGLIndev() const { return touch_indev_; }

    // 注册回调函数（可选）
    using TouchCallback = std::function<void(const TouchPoint &)>;
    void SetTouchDownCallback(TouchCallback cb) { touch_down_cb_ = cb; }
    void SetTouchUpCallback(TouchCallback cb) { touch_up_cb_ = cb; }

  private:
    // 私有成员变量
    esp_lcd_touch_handle_t touch_handle_ = nullptr;
    lv_indev_t *touch_indev_ = nullptr;
    TouchParam param_;
    TaskHandle_t touch_task_handle_ = nullptr;

    // 触摸状态
    bool last_touch_state_ = false;
    int16_t last_touch_x_ = 0;
    int16_t last_touch_y_ = 0;

    // 回调函数
    TouchCallback touch_down_cb_;
    TouchCallback touch_up_cb_;

    // 任务相关
    static void TouchTaskFunction(void *arg);

    // LVGL回调相关
    static void LVGLTouchReadCallback(lv_indev_t *indev, lv_indev_data_t *data);

    // 实际的触摸读取函数
    void InternalTouchRead(lv_indev_data_t *data);

    // 初始化LVGL输入设备
    bool InitLVGLTouchDevice();

    // 更新触摸状态
    void UpdateTouchState(bool touched, int16_t x, int16_t y);
};

#endif // FBT_TOUCH_DEVICE_H