#ifndef FBT_UI_VOICE_H
#define FBT_UI_VOICE_H

#include "fbt_lvgl_button.h"
#include <string>

class FbtUiVoice {

  public:
    static FbtUiVoice &GetInstance();

    void Initialize();

    void ShowEnterButton();
    void HideEnterButton();
    void EnterIdle();
    void SetEnterState(bool enter);
    void SetRtcState(FbtRtcState state_);

  private:
    FbtUiVoice() = default;
    ~FbtUiVoice() = default;

    struct VoiceState {
        bool show_enter;
        bool show_mic;
    };

    void on_button_callback();
    void set_enter_state();
    void set_voice_speaking(bool speaking);

    static void lvgl_thread_task(void *arg) {
        FbtUiVoice *self = static_cast<FbtUiVoice *>(arg);
        if (self->enter_btn_.IsInitialized()) {
            if (self->state_.show_enter) {
                self->enter_btn_.Show();
            } else {
                self->enter_btn_.Hide();
            }
        }
        if (self->mic_btn_.IsInitialized()) {
            if (self->state_.show_mic) {
                self->mic_btn_.Show();
            } else {
                self->mic_btn_.Hide();
            }
        }
        self->set_enter_state();
    }

    VoiceState state_;

    FbtLvglButton enter_btn_;

    FbtLvglButton mic_btn_;

    bool is_show_enter_btn_ = false;
    bool is_enter_room = false;

    FbtRtcState rtc_state_ = kIdle;
};

#endif // FBT_UI_VOICE_H
