
#ifndef FBT_UI_PHONE_H
#define FBT_UI_PHONE_H

#include "display.h"
#include "fbt_lvgl_button.h"
#include "fbt_lvgl_image.h"
#include "fbt_lvgl_label.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <fbt_constants.h>
#include <lvgl.h>

class FbtUiPhone {
  public:
    typedef enum {
        /**
         * 通话中(默认)
         */
        COLOR_CALLING = 0X000,
        /**
         * 来电话
         */
        COLOR_INCOMING = 0X1A365D,
        /**
         * 拔号
         */
        COLOR_OUTGOING = 0x141210
    } BgColors;
    static FbtUiPhone &GetInstance();

    void Initialize(); // 初始化（调用一次）
    //                 // 隐藏弹窗
    void add_button();

    void SetTypeValue(std::string value);
    void SetMessageValue(std::string value);

    void SetRtcState(FbtPhoneState state);

    void OnAnswer(std::function<void()> callback) { on_answer_ = std::move(callback); };
    void OnHangUp(std::function<void()> callback) { on_hang_up_ = std::move(callback); };

  private:
    FbtUiPhone() = default;
    ~FbtUiPhone() = default;

    static void initialize_async(void *arg);
    void Show(bool show_answer); // 显示弹窗
    void Hide();

    void set_bg_color();

    void do_init();

    void do_answer();

    void do_show_answer(bool show);

    void on_button_callback();

    Display *display_;

    std::function<void()> on_answer_;

    std::function<void()> on_hang_up_;

    lv_obj_t *context_ = nullptr;

    FbtLvglImage image_;

    FbtLvglLabel type_label_;

    FbtLvglLabel message_label_;

    FbtLvglButton answer_btn_;

    FbtLvglButton hang_up_btn_;

    FbtRtcState state_;

    BgColors bg_color_ = BgColors::COLOR_CALLING;

    bool is_show_ = true;
};

#endif // FBT_UI_PHONE_H