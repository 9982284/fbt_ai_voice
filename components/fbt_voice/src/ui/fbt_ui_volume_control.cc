#include "fbt_ui_volume_control.h"

#if CONFIG_USE_FBT_SCREEN_TOUCH
#include "fbt_touch_dispatcher.h"
#endif
#include <esp_log.h>
#include <settings.h>

#define TAG "FbtUiVolumeControl"

FbtUiVolumeControl *FbtUiVolumeControl::instance_ = nullptr;

FbtUiVolumeControl &FbtUiVolumeControl::GetInstance() {
    if (instance_ == nullptr) {
        static FbtUiVolumeControl instance;
        instance_ = &instance;
    }
    return *instance_;
}

void FbtUiVolumeControl::Initialize(VolumeControlParam param) {
    param_ = param;

    ProgressBarParam pb_param;
    pb_param.position_x = param.position_x;
    pb_param.position_y = param.position_y;
    pb_param.width = param.width;
    pb_param.height = param.height;
    pb_param.default_show = false;
    Settings settings("audio", false);
    pb_param.current_value = settings.GetInt("output_volume", pb_param.current_value);
    progress_bar_.Initialize(pb_param);
    on_touch_event();
    is_initialized_ = true;

    touch_min_x_ = param.touch_area.min_x;
    touch_max_x_ = param.touch_area.max_x;
    touch_min_y_ = param.touch_area.min_y;
    touch_max_y_ = param.touch_area.max_y;
}

void FbtUiVolumeControl::SetValue(int32_t value) {
    progress_bar_.SetValue(value);
}

void FbtUiVolumeControl::SetTouchDeltaY(int32_t start_y, int32_t end_y) {
    if (!is_show_) {
        return;
    }
    progress_bar_.SetTouchHeight(start_y, end_y);
}

void FbtUiVolumeControl::SetTouchScope(int32_t min_x, int32_t max_x, int32_t min_y, int32_t max_y) {
    touch_min_x_ = min_x;
    touch_max_x_ = max_x;
    touch_min_y_ = min_y;
    touch_max_y_ = max_y;
}

bool FbtUiVolumeControl::TouchShow(int32_t touch_x, int32_t touch_y) {
    if (is_show_ || !is_initialized_) {
        return false;
    }
    if (!audio_codec_) {
        audio_codec_ = Board::GetInstance().GetAudioCodec();
    }

    if (!audio_codec_) {
        return false;
    }

    if (touch_x && (touch_min_x_ != touch_max_x_) && (touch_y && (touch_min_y_ != touch_max_y_))) {
        if (touch_x < touch_min_x_ || touch_x > touch_max_x_) {
            return false;
        }
        if (touch_y < touch_min_y_ || touch_y > touch_max_y_) {
            return false;
        }
    }

    progress_bar_.Show();
    is_show_ = true;
    return true;
}

// 隐藏
void FbtUiVolumeControl::Hide() {
    if (!is_show_ || !is_initialized_) {
        return;
    }
    progress_bar_.Hide();
    is_show_ = false;
    if (progress_bar_.IsAreChangesValue() && audio_codec_) {
        audio_codec_->SetOutputVolume(progress_bar_.GetValue());
    }
}

void FbtUiVolumeControl::on_touch_event() {
#if CONFIG_USE_FBT_SCREEN_TOUCH
    FbtUiVolumeControl &self = FbtUiVolumeControl::GetInstance();
    FbtTouchDispatcher::Instance().AddListener(
        // 按下回调
        [&self](const TouchPoint &point) -> bool {
            if (self.TouchShow(point.x, point.y)) {
                return true;
            }
            return false;
        },
        // 松开回调
        [&self](const TouchPoint &point) {
            self.Hide();
        },
        // 移动回调
        [&self](const TouchPoint &last, const TouchPoint &point) {
            self.SetTouchDeltaY(last.y, point.y);
        });
#endif
}
