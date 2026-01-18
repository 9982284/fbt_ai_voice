#include "fbt_ui_phone.h"
#include "assets.h"

#include "board.h"
#if CONFIG_USE_FBT_SCREEN_TOUCH
#include "fbt_lvgl_image.h"
#endif
#include <esp_log.h>

#include <lang_config.h>
#include <stdio.h>

#define TAG "FbtBgManager"

FbtUiPhone &FbtUiPhone::GetInstance() {
    static FbtUiPhone instance;
    return instance;
}

void FbtUiPhone::Initialize() {
    display_ = Board::GetInstance().GetDisplay();

#if CONFIG_USE_FBT_SCREEN_TOUCH
    lv_async_call(initialize_async, this);
#endif
}

// 显示/隐藏方法
void FbtUiPhone::Show(bool show_answer) {
    if (is_show_) {
        return;
    }
    is_show_ = true;
    if (context_) {
        lv_async_call([](void *obj) {
            lv_obj_clear_flag((lv_obj_t *)obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground((lv_obj_t *)obj); }, context_);
        do_show_answer(show_answer);
    }
}

void FbtUiPhone::Hide() {
    if (!is_show_) {
        return;
    }
    is_show_ = false;
    if (context_) {
        lv_async_call([](void *btn) { lv_obj_add_flag((lv_obj_t *)btn, LV_OBJ_FLAG_HIDDEN); }, context_);
    }
}

void FbtUiPhone::do_answer() {
    if (on_answer_) {
        on_answer_();
    }
    do_show_answer(false);
}

void FbtUiPhone::do_show_answer(bool show) {
    if (answer_btn_.IsInitialized() && hang_up_btn_.IsInitialized()) {
        int32_t x_pos = 0;
        lv_align_t align_all = LV_ALIGN_BOTTOM_MID;
        if (!show) {
            answer_btn_.Hide();
            hang_up_btn_.SetOffset(&x_pos, nullptr, &align_all);
        } else {
            answer_btn_.Show();
            x_pos = -50;
            align_all = LV_ALIGN_BOTTOM_RIGHT;
            hang_up_btn_.SetOffset(&x_pos, nullptr, &align_all);
        }
    }
}

void FbtUiPhone::add_button() {
    if (!context_) {
        return;
    }
    FbtButtonParam param = {};
    param.area = LV_ALIGN_BOTTOM_LEFT;
    param.parent = context_;
    param.offset_y = -20;
    param.offset_x = 50;
    param.circle = true;
    param.icon = ICON_PHONE;
    param.size = 60;
    param.pg_color = LV_PALETTE_GREEN;
    param.icon_size = FONT_SIZE_SMALL;
    answer_btn_.CreateBtn(param);
    param.area = LV_ALIGN_BOTTOM_RIGHT;
    param.offset_x = -50;
    param.pg_color = LV_PALETTE_RED;
    param.icon = ICON_HANG_UP;
    hang_up_btn_.CreateBtn(param);
}

void FbtUiPhone::SetTypeValue(std::string value) {
    if (type_label_.IsInitialized()) {
        type_label_.SetValue(value);
    } else {
        display_->SetStatus(value.c_str());
    }
}

void FbtUiPhone::SetMessageValue(std::string value) {
    if (message_label_.IsInitialized()) {
        message_label_.SetValue(value);
        display_->SetChatMessage("system", "");
    } else {
        display_->SetChatMessage("fbt", value.c_str());
    }
}

void FbtUiPhone::SetRtcState(FbtPhoneState state) {
    std::string status;
    bool show_answer = false;
    switch (state) {
        case IDLE:
            break;
        case CONNECTING:
            status = Lang::Strings::CONNECTING;
            break;
        case INCOMING:
            status = Lang::Strings::FBT_PHONE_AUDIO_IN;
            bg_color_ = BgColors::COLOR_INCOMING;
            show_answer = true;
            break;
        case OUTGOING:
            status = Lang::Strings::FBT_PHONE_RINGING;
            bg_color_ = BgColors::COLOR_OUTGOING;
            break;
        case CALLING:
            status = Lang::Strings::FBT_PHONE_STARTED;
            bg_color_ = BgColors::COLOR_CALLING;
            break;
        default:
            bg_color_ = BgColors::COLOR_CALLING;
            break;
    }

    if (!status.empty()) {
        SetTypeValue(status);
        Show(show_answer);
        set_bg_color();
    } else {
        Hide();
    }
}

void FbtUiPhone::set_bg_color() {
    if (context_) {
        // lv_obj_set_style_bg_color(context_, lv_color_hex(bg_color_), 0);
        lv_async_call([](void *arg) {
        FbtUiPhone *self = static_cast<FbtUiPhone *>(arg);
        if (self->context_) {
            lv_obj_set_style_bg_color(self->context_, lv_color_hex(self->bg_color_), 0);
        } }, this);
    }
}

void FbtUiPhone::initialize_async(void *arg) {
    FbtUiPhone *self = static_cast<FbtUiPhone *>(arg);
    self->do_init();
}

void FbtUiPhone::do_init() {
    context_ = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(context_);

    lv_obj_set_size(context_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(context_, 0, 0);
    lv_obj_set_style_bg_opa(context_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(context_, 0, 0);
    lv_obj_set_style_pad_all(context_, 0, 0);
    // lv_obj_set_style_bg_color(context_, lv_color_hex(BgColors::COLOR_DIALING), 0);

    /* FbtImageParam param = {};
    param.file_name = "fbt_phone_bg_1_jpg";
    param.parent = context_;
    if (!image_.CreateJpg(param)) {
        return;
    }; */
    add_button();
    FbtLabelParam labelParam = {};
    labelParam.parent = context_;
    labelParam.offset_y = -20;
    labelParam.text_color = 0xffffff;
    message_label_.CreateLabel(labelParam);
    labelParam.area = LV_ALIGN_TOP_MID;
    labelParam.offset_y = 20;
    labelParam.text_color = 0xffffff;
    type_label_.CreateLabel(labelParam);

    on_button_callback();
    Hide();
    ESP_LOGI(TAG, "初始化完成");
}

void FbtUiPhone::on_button_callback() {
    answer_btn_.OnClick([this]() {
        do_answer();
    });
    hang_up_btn_.OnClick([this]() {
        Hide();
        if (on_hang_up_) {
            on_hang_up_();
        }
    });
}
