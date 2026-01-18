#include "fbt_lvgl_button.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

void FbtLvglButton::CreateBtn(const FbtButtonParam &param) {
    param_ = param;
    // do_create_btn();
    lv_async_call(initialize_async, this);
}

void FbtLvglButton::Show() {
    lv_async_call([](void *btn) { lv_obj_clear_flag((lv_obj_t *)btn, LV_OBJ_FLAG_HIDDEN); }, button_);
}

void FbtLvglButton::Hide() {
    // 不要直接调用，而是发送消息到LVGL任务

    // 使用LVGL的异步API或任务间通信
    lv_async_call([](void *btn) { lv_obj_add_flag((lv_obj_t *)btn, LV_OBJ_FLAG_HIDDEN); }, button_);
}

void FbtLvglButton::SetValue(const std::string &value) {
    if (label_) {
        lv_label_set_text(label_, value.c_str());
    }
}

void FbtLvglButton::SetIcon(const char *icon) {
    if (!label_) {
        return;
    }
    lv_label_set_text(label_, icon);
}

void FbtLvglButton::SetOffset(const int32_t *x, const int32_t *y, const lv_align_t *area) {
    if (button_) {
        // 使用指针判断
        int32_t offset_x = x ? *x : param_.offset_x;
        int32_t offset_y = y ? *y : param_.offset_y;
        lv_align_t area_value = area ? *area : param_.area;

        lv_obj_align(button_, area_value, offset_x, offset_y);

        // 更新参数
        if (x)
            param_.offset_x = *x;
        if (y)
            param_.offset_y = *y;
        if (area)
            param_.area = *area;
    }
}

void FbtLvglButton::initialize_async(void *arg) {
    FbtLvglButton *self = static_cast<FbtLvglButton *>(arg);
    self->do_create_btn();
}

void FbtLvglButton::do_create_btn() {

    lv_obj_t *parent = param_.parent ? param_.parent : lv_scr_act();
    if (!parent) {
        return;
    }

    button_ = lv_btn_create(parent);
    if (!button_) {
        return;
    }

    if (param_.bg_transparent) {
        set_bg_transparent();
    } else {
        lv_obj_set_style_bg_color(button_, lv_palette_main(param_.pg_color), LV_STATE_DEFAULT);
    }

    if (!param_.circle) {
        lv_obj_set_style_pad_hor(button_, param_.pad_hor, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_ver(button_, param_.pad_ver, LV_STATE_DEFAULT);
    }
    lv_obj_align(button_, param_.area, param_.offset_x, param_.offset_y);
    // 移除边框
    lv_obj_set_style_border_width(button_, 0, 0);
    lv_obj_set_style_border_width(button_, 0, LV_STATE_PRESSED);
    set_radius();
    // 创建标签显示文本
    label_ = lv_label_create(button_);
    if (!label_) {
        lv_obj_del(button_);
        button_ = nullptr;
        return;
    }

    if (param_.icon) {
        lv_label_set_text(label_, param_.icon);
        lv_obj_set_style_text_font(label_, get_icon_font(param_.icon_size), 0);
    } else {
        lv_label_set_text(label_, param_.text.c_str());
    }

    if (param_.text_color) {
        lv_obj_set_style_text_color(label_, lv_palette_main(param_.text_color), LV_STATE_DEFAULT);
    }
    if (param_.enable_event) {
        lv_obj_add_event_cb(button_, event_handler, LV_EVENT_ALL, this);
    }
    lv_obj_center(label_);

    if (!param_.show) {
        Hide();
    }
}

void FbtLvglButton::set_bg_transparent() {
    lv_obj_remove_style_all(button_);
    // 设置完全透明背景
    lv_obj_set_style_bg_opa(button_, LV_OPA_TRANSP, 0);                // 主背景透明
    lv_obj_set_style_bg_opa(button_, LV_OPA_TRANSP, LV_STATE_PRESSED); // 按压状态透明

    // 移除阴影
    lv_obj_set_style_shadow_width(button_, 0, 0);

    // 移除轮廓（outline）
    lv_obj_set_style_outline_width(button_, 0, 0);
}

void FbtLvglButton::set_radius() {
    if (param_.circle) {
        lv_obj_set_width(button_, param_.size);
        lv_obj_set_height(button_, param_.size);
        lv_obj_set_style_radius(button_, param_.size / 2, 0);
    } else {
        lv_obj_set_style_radius(button_, param_.radius, 0);
        lv_obj_set_width(button_, LV_SIZE_CONTENT);
        lv_obj_set_height(button_, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(button_, param_.pad_hor, 0); // 水平内边距
        lv_obj_set_style_pad_ver(button_, param_.pad_ver, 0); // 垂直内边距
    }
}

void FbtLvglButton::event_handler(lv_event_t *e) {
    void *user_data = lv_event_get_user_data(e);
    if (!user_data)
        return;

    FbtLvglButton *self = static_cast<FbtLvglButton *>(user_data);
    lv_event_code_t code = lv_event_get_code(e);

    switch (code) {
        // 按下
        case LV_EVENT_PRESSED:
            if (self->on_press_down_) {
                self->on_press_down_();
            }
            break;

        case LV_EVENT_RELEASED:
            if (self->on_press_up_) {
                self->on_press_up_();
            }
            break;

        case LV_EVENT_CLICKED:
            if (self->on_click_) {
                self->on_click_();
            }
            break;
        case LV_EVENT_PRESS_LOST:
            if (self->on_press_up_) {
                self->on_press_up_();
            }
            break;
        default:
            break;
    }
}
