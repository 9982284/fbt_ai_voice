#ifndef FBT_UI_VOLUME_CONTROL_H
#define FBT_UI_VOLUME_CONTROL_H

#include "audio_codec.h"
#include "fbt_lvgl_button.h"
#include "fbt_lvgl_progress_bar.h"
#include <cstdint>
#include <lvgl.h>

struct VolumeControlTouch {
    int32_t min_x = 0;
    int32_t max_x = 0;
    int32_t min_y = 0;
    int32_t max_y = 0;
};

struct VolumeControlParam {
    int32_t position_x;
    int32_t position_y;
    int32_t width;
    int32_t height;
    VolumeControlTouch touch_area;
};

class FbtUiVolumeControl {

  public:
    // 获取单例实例
    static FbtUiVolumeControl &GetInstance();

    void Initialize(VolumeControlParam param);

    void SetValue(int32_t value);

    void SetTouchDeltaY(int32_t start_y, int32_t end_y);
    /**
     * 设置触摸范围X轴
     */
    void SetTouchScope(int32_t min_x, int32_t max_x, int32_t min_y, int32_t max_y);
    /**
     * 模型显示
     * 当前按下的座标
     */
    bool TouchShow(int32_t touch_x, int32_t touch_y);

    // 隐藏
    void Hide();

    // 获取是否已初始化
    bool IsInitialized() const { return is_initialized_; }

    // 获取是否显示中
    bool IsShowing() const { return is_show_; }

  private:
    static FbtUiVolumeControl *instance_; // 单例实例指针
    AudioCodec *audio_codec_;

    int32_t touch_min_x_;
    int32_t touch_max_x_;
    int32_t touch_min_y_;
    int32_t touch_max_y_;

    bool is_initialized_;
    bool is_show_;

    VolumeControlParam param_;

    FbtLvProgressBar progress_bar_;

    void on_touch_event();

    // 私有构造函数
    FbtUiVolumeControl() {};

    // 删除拷贝构造函数和赋值运算符
    FbtUiVolumeControl(const FbtUiVolumeControl &) = delete;
    FbtUiVolumeControl &operator=(const FbtUiVolumeControl &) = delete;
};

#endif // FBT_UI_VOLUME_CONTROL_H