#ifndef FBT_LVGL_PROGRESS_BAR_H
#define FBT_LVGL_PROGRESS_BAR_H

#include <lvgl.h>

struct ProgressBarParam {
    int32_t position_x;        // 位置
    int32_t position_y;        // 位置
    int32_t width;             // 进度条宽度
    int32_t height;            // 时度条高度
    int32_t min_value = 0;     // 最小值
    int32_t max_value = 100;   // 最大值
    int32_t current_value = 0; // 当前值
    int32_t step_pixels = 2;   // 每2像素变化一个单位
    bool default_show = true;  // 默认显示与隐藏状态 true 显示 false 隐藏
};

class FbtLvProgressBar {
  public:
    FbtLvProgressBar() {};
    ~FbtLvProgressBar();

    // 异步操作
    void Initialize(const ProgressBarParam &param);
    void SetValue(int32_t value);
    void Show();
    void Hide();
    void SetPosition(int32_t x, int32_t y);
    void SetTouchHeight(int32_t start_y, int32_t end_y);
    bool IsAreChangesValue() const { return are_changes_value_; }

    // 同步获取
    int32_t GetValue() const { return param_.current_value; }
    lv_obj_t *GetObject() const { return bar_obj_; }
    bool IsInitialized() const { return bar_obj_ != nullptr; }

  private:
    // 异步回调（静态）
    static void initialize_async(void *arg);
    static void set_value_async(void *arg);
    static void show_async(void *arg);
    static void hide_async(void *arg);
    static void set_position_async(void *arg);

    // 实际执行（非静态）

    void do_initialize();
    void do_touch_delta_height(int32_t start_y, int32_t end_y);
    void do_set_value(int32_t value);
    void do_show();
    void do_hide();
    void do_set_position(int32_t x, int32_t y);
    void update_display();
    void update_bar_color();

    // 异步参数结构
    struct AsyncData {
        FbtLvProgressBar *instance;
        union {
            int32_t value; // for SetValue
            struct {       // for SetPosition
                int32_t x;
                int32_t y;
            } position;
        } data;
    };

  private:
    ProgressBarParam param_;
    lv_obj_t *bar_obj_ = nullptr;
    lv_obj_t *container_bg_ = nullptr;
    lv_obj_t *value_label_; // 数值标签

    int32_t bar_bg_width_;
    int32_t bar_bg_height_;
    int32_t bar_radius_;
    bool is_showing_ = false;
    // 是否有更改数值
    bool are_changes_value_ = false;
};
#endif // FBT_LVGL_PROGRESS_BAR_H