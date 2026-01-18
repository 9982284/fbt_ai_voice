#include "fbt_touch_device.h"
#include "fbt_touch_dispatcher.h"
#include <algorithm>
#include <climits>
#include <esp_log.h>

static const char *TAG = "FbtTouchDevice";

// 构造函数
FbtTouchDevice::FbtTouchDevice() {
}

// 析构函数
FbtTouchDevice::~FbtTouchDevice() {
    if (touch_task_handle_) {
        vTaskDelete(touch_task_handle_);
        touch_task_handle_ = nullptr;
    }

    if (touch_indev_) {
        lv_indev_delete(touch_indev_);
        touch_indev_ = nullptr;
    }

    if (touch_handle_) {
        // 注意：需要根据实际情况释放触摸设备资源
        // esp_lcd_touch_del(touch_handle_);
        touch_handle_ = nullptr;
    }
}

// 初始化函数
bool FbtTouchDevice::Initialize(const TouchParam &param) {
    if (param.display_width == 0 || param.display_height == 0) {
        ESP_LOGE(TAG, "Invalid display dimensions");
        return false;
    }

    param_ = param;

    // 1. 初始化I2C总线
    i2c_master_bus_handle_t bus_handle = nullptr;
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = param.gpio_sda,
        .scl_io_num = param.gpio_scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_cfg, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(ret));
        return false;
    }

    // 2. 初始化I2C面板IO
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = 0x15, // CST816S的I2C地址
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags = {
            .disable_control_phase = true,
        },
        .scl_speed_hz = 100000,
    };

    ret = esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize panel IO: %s", esp_err_to_name(ret));
        i2c_del_master_bus(bus_handle);
        return false;
    }

    // 3. 初始化触摸控制器
    esp_lcd_touch_config_t touch_config = {
        .x_max = param.display_width,
        .y_max = param.display_height,
        .rst_gpio_num = param.gpio_rst,
        .int_gpio_num = param.gpio_int,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = static_cast<unsigned int>(param.swap_xy),
            .mirror_x = static_cast<unsigned int>(param.mirror_x),
            .mirror_y = static_cast<unsigned int>(param.mirror_y),
        },
    };

    ret = esp_lcd_touch_new_i2c_cst816s(io_handle, &touch_config, &touch_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CST816S: %s", esp_err_to_name(ret));
        esp_lcd_panel_io_del(io_handle);
        i2c_del_master_bus(bus_handle);
        return false;
    }

    // 4. 初始化LVGL触摸设备
    if (!InitLVGLTouchDevice()) {
        ESP_LOGE(TAG, "Failed to initialize LVGL touch device");
        return false;
    }

    // 5. 创建触摸任务
    BaseType_t task_ret = xTaskCreate(
        TouchTaskFunction,
        "touch_task",
        3072,
        this,
        3,
        &touch_task_handle_);

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create touch task");
        return false;
    }

    ESP_LOGI(TAG, "Touch device initialized successfully");
    ESP_LOGI(TAG, "Display: %dx%d, Offset: (%d,%d)",
             param.display_width, param.display_height,
             param.offset_x, param.offset_y);

    return true;
}

// LVGL触摸读取回调（必须是静态的）
void FbtTouchDevice::LVGLTouchReadCallback(lv_indev_t *indev, lv_indev_data_t *data) {
    // 从user_data获取实例指针
    FbtTouchDevice *device = static_cast<FbtTouchDevice *>(lv_indev_get_user_data(indev));
    if (device) {
        device->InternalTouchRead(data);
    } else {
        // 默认值
        data->point.x = 0;
        data->point.y = 0;
        data->state = LV_INDEV_STATE_RELEASED;
        data->continue_reading = false;
    }
}

// 实际的触摸读取函数
void FbtTouchDevice::InternalTouchRead(lv_indev_data_t *data) {
    data->point.x = last_touch_x_;
    data->point.y = last_touch_y_;
    data->state = last_touch_state_ ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->continue_reading = false;

    // 可选：调试输出
    // static uint32_t count = 0;
    // if (++count % 50 == 0) {
    //     ESP_LOGI(TAG, "Touch read: %s (%d,%d)",
    //              last_touch_state_ ? "PRESSED" : "RELEASED",
    //              last_touch_x_, last_touch_y_);
    // }
}

// 更新触摸状态
void FbtTouchDevice::UpdateTouchState(bool touched, int16_t x, int16_t y) {
    last_touch_state_ = touched;
    if (touched) {
        last_touch_x_ = x;
        last_touch_y_ = y;

        // 触发按下回调
        if (touch_down_cb_) {
            TouchPoint point(x, y, 0);
            touch_down_cb_(point);
        }
    } else {
        // 触发释放回调
        if (touch_up_cb_) {
            TouchPoint point(last_touch_x_, last_touch_y_, 0);
            touch_up_cb_(point);
        }
    }
}

// 初始化LVGL触摸设备
bool FbtTouchDevice::InitLVGLTouchDevice() {
    // 创建LVGL输入设备
    touch_indev_ = lv_indev_create();
    if (!touch_indev_) {
        ESP_LOGE(TAG, "Failed to create LVGL input device");
        return false;
    }

    // 设置设备类型
    lv_indev_set_type(touch_indev_, LV_INDEV_TYPE_POINTER);

    // 设置读取回调（使用静态包装函数）
    lv_indev_set_read_cb(touch_indev_, LVGLTouchReadCallback);

    // 将this指针存储到user_data中，以便在静态回调中访问
    lv_indev_set_user_data(touch_indev_, this);

    ESP_LOGI(TAG, "LVGL touch device created");
    return true;
}

// 触摸任务函数（必须是静态的）
void FbtTouchDevice::TouchTaskFunction(void *arg) {
    FbtTouchDevice *self = static_cast<FbtTouchDevice *>(arg);
    if (!self) {
        ESP_LOGE(TAG, "Invalid device instance in touch task");
        vTaskDelete(nullptr);
        return;
    }

    ESP_LOGI(TAG, "Touch task started");

    // 局部变量副本，避免频繁访问成员变量
    const int offset_x = self->param_.offset_x;
    const int offset_y = self->param_.offset_y;
    const uint16_t width = self->param_.display_width;
    const uint16_t height = self->param_.display_height;

    // 触摸状态追踪
    bool last_hardware_touch = false;
    TouchPoint last_point;

    // 主循环
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        // 50ms间隔（20Hz）
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(50));

        if (!self->touch_handle_) {
            continue;
        }

        // 读取触摸数据
        esp_err_t ret = esp_lcd_touch_read_data(self->touch_handle_);
        if (ret != ESP_OK) {
            if (last_hardware_touch) {
                // 之前是触摸状态，现在读取失败，视为释放
                self->UpdateTouchState(false, 0, 0);
                last_hardware_touch = false;
                ESP_LOGW(TAG, "Touch read error, releasing");
            }
            continue;
        }

        // 获取触摸点数据
        esp_lcd_touch_point_data_t raw_points[1]; // 只处理单点触摸
        uint8_t point_count = 0;
        ret = esp_lcd_touch_get_data(self->touch_handle_, raw_points, &point_count, 1);

        bool current_touch = (ret == ESP_OK && point_count > 0);

        if (current_touch) {
            // 处理触摸点
            int16_t raw_x = static_cast<int16_t>(raw_points[0].x);
            int16_t raw_y = static_cast<int16_t>(raw_points[0].y);

            // 应用偏移
            int32_t temp_x = static_cast<int32_t>(raw_x) + offset_x;
            int32_t temp_y = static_cast<int32_t>(raw_y) + offset_y;

            // 限制范围
            temp_x = std::max<int32_t>(0, std::min<int32_t>(temp_x, width - 1));
            temp_y = std::max<int32_t>(0, std::min<int32_t>(temp_y, height - 1));

            TouchPoint current_point;
            current_point.x = static_cast<int16_t>(temp_x);
            current_point.y = static_cast<int16_t>(temp_y);
            current_point.pressure = static_cast<uint8_t>(
                std::min<uint16_t>(raw_points[0].strength, 255));

            // 更新触摸状态
            self->UpdateTouchState(true, current_point.x, current_point.y);

            // 发送到调度器
            if (!last_hardware_touch) {
                // 按下事件
                FbtTouchDispatcher::Instance().EnqueueTouchDown(current_point);
                // ESP_LOGI(TAG, "Touch down: (%d,%d)", current_point.x, current_point.y);
            } else {
                // 移动事件（如果位置变化超过阈值）
                const int MOVE_THRESHOLD = 2;
                if (abs(current_point.x - last_point.x) > MOVE_THRESHOLD ||
                    abs(current_point.y - last_point.y) > MOVE_THRESHOLD) {
                    FbtTouchDispatcher::Instance().EnqueueTouchMove(last_point, current_point);
                }
            }

            last_point = current_point;
            last_hardware_touch = true;

        } else if (last_hardware_touch) {
            // 触摸释放
            self->UpdateTouchState(false, last_point.x, last_point.y);
            FbtTouchDispatcher::Instance().EnqueueTouchUp(last_point);
            // ESP_LOGI(TAG, "Touch up: (%d,%d)", last_point.x, last_point.y);
            last_hardware_touch = false;
        }

        // 调试输出（可选）
        // static uint32_t loop_count = 0;
        // if (++loop_count % 20 == 0) {
        //     ESP_LOGI(TAG, "Touch task running, state: %s",
        //              current_touch ? "TOUCHED" : "RELEASED");
        // }
    }
}