#include "esp_log.h"

#include "fbt_manager.h"
#include "fbt_ui_voice.h"

#define TAG "FbtUiVoice"

FbtUiVoice &FbtUiVoice::GetInstance() {
    static FbtUiVoice instance;
    return instance;
}

void FbtUiVoice::Initialize() {
#if CONFIG_USE_FBT_SCREEN_TOUCH

    FbtButtonParam param_ = {};
    param_.icon = ICON_WALKIE_TALKIE;
    param_.icon_size = FONT_SIZE_MODERATE;
    param_.bg_transparent = true;
    param_.area = LV_ALIGN_TOP_RIGHT;
    enter_btn_.CreateBtn(param_);

    param_ = {};
    param_.icon = ICON_MIC;
    param_.area = LV_ALIGN_CENTER;
    param_.pg_color = LV_PALETTE_GREEN;
    param_.show = false;
    param_.size = 80;
    param_.circle = true;
    param_.icon_size = FbtIconSize::FONT_SIZE_BIG;
    mic_btn_.CreateBtn(param_);
    on_button_callback();

#endif
}

void FbtUiVoice::ShowEnterButton() {
    if (!enter_btn_.IsInitialized()) {
        return;
    }
    enter_btn_.Show();
    is_show_enter_btn_ = true;
}

void FbtUiVoice::HideEnterButton() {
    if (!enter_btn_.IsInitialized()) {
        return;
    }
    enter_btn_.Hide();
    SetEnterState(false);
    is_show_enter_btn_ = false;
}

void FbtUiVoice::EnterIdle() {
    state_ = {
        .show_enter = true,
        .show_mic = false,
    };
    is_show_enter_btn_ = false;
    is_enter_room = false;
    lv_async_call(lvgl_thread_task, this);
}

void FbtUiVoice::SetEnterState(bool enter) {
    is_enter_room = enter;
    set_enter_state();
    if (mic_btn_.IsInitialized()) {
        if (enter) {
            mic_btn_.Show();
        } else {
            mic_btn_.Hide();
        }
    }
}

void FbtUiVoice::SetRtcState(FbtRtcState state_) {
    if (rtc_state_ == kUnavailable) {
        ShowEnterButton();
    }
    rtc_state_ = state_;
    switch (state_) {
        case kIdle:
            set_enter_state();
            break;
        case kUnavailable:
            HideEnterButton();
            break;
        default:
            break;
    }
}

void FbtUiVoice::on_button_callback() {

    enter_btn_.OnClick([this]() {
        auto &fbt_manager = FbtManager::GetInstance();
        if (!is_enter_room) {
            fbt_manager.OnEnterIntercomMode();
        } else {
            fbt_manager.OnExitVoiceMode();
        }
    });
    mic_btn_.OnPressDown([this]() {
        set_voice_speaking(true);
    });
    mic_btn_.OnPressUp([this]() {
        set_voice_speaking(false);
    });
}

void FbtUiVoice::set_enter_state() {
    if (enter_btn_.IsInitialized()) {
        const char *icon;
        if (is_enter_room) {
            icon = ICON_CLOSE;
        } else {
            icon = ICON_WALKIE_TALKIE;
        }
        enter_btn_.SetIcon(icon);
    };
}

void FbtUiVoice::set_voice_speaking(bool speaking) {
    auto &fbt_manager = FbtManager::GetInstance();
    if (!fbt_manager.IsIntercomMode()) {
        return;
    }
    fbt_manager.OnSpeaking(speaking);
}
