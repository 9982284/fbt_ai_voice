#include "fbt_lvgl_progress_bar.h"

FbtLvProgressBar::~FbtLvProgressBar() {
    if (container_bg_) {
        lv_obj_del_async(container_bg_); // 异步删除，避免卡顿
    }
}

void FbtLvProgressBar::Initialize(const ProgressBarParam &param) {
    param_ = param;
    lv_async_call(initialize_async, this);
}

void FbtLvProgressBar::SetValue(int32_t value) {
    if (value < param_.min_value)
        value = param_.min_value;
    if (value > param_.max_value)
        value = param_.max_value;

    // 创建异步数据
    AsyncData *data = new AsyncData();
    data->instance = this;
    data->data.value = value;

    lv_async_call(set_value_async, data);
}

void FbtLvProgressBar::Show() {
    lv_async_call(show_async, this);
}

void FbtLvProgressBar::Hide() {
    lv_async_call(hide_async, this);
}

void FbtLvProgressBar::SetPosition(int32_t x, int32_t y) {
    AsyncData *data = new AsyncData();
    data->instance = this;
    data->data.position.x = x;
    data->data.position.y = y;

    lv_async_call(set_position_async, data);
}

void FbtLvProgressBar::SetTouchHeight(int32_t start_y, int32_t end_y) {
    // 创建数据包
    struct TouchData {
        FbtLvProgressBar *instance;
        int32_t start_y;
        int32_t end_y;
    };

    TouchData *data = new TouchData{this, start_y, end_y};

    lv_async_call([](void *arg) {
        TouchData *data = static_cast<TouchData *>(arg);
        if (data && data->instance) {
            data->instance->do_touch_delta_height(data->start_y, data->end_y);
        }
        delete data; }, data);
}

// =============== 异步回调实现 ===============
void FbtLvProgressBar::initialize_async(void *arg) {
    FbtLvProgressBar *self = static_cast<FbtLvProgressBar *>(arg);
    if (self) {
        self->do_initialize();
    }
}

void FbtLvProgressBar::set_value_async(void *arg) {
    AsyncData *data = static_cast<AsyncData *>(arg);
    if (data && data->instance) {
        data->instance->do_set_value(data->data.value);
    }
    delete data;
}

void FbtLvProgressBar::show_async(void *arg) {
    FbtLvProgressBar *self = static_cast<FbtLvProgressBar *>(arg);
    if (self) {
        self->do_show();
    }
}

void FbtLvProgressBar::hide_async(void *arg) {
    FbtLvProgressBar *self = static_cast<FbtLvProgressBar *>(arg);
    if (self) {
        self->do_hide();
    }
}

void FbtLvProgressBar::set_position_async(void *arg) {
    AsyncData *data = static_cast<AsyncData *>(arg);
    if (data && data->instance) {
        data->instance->do_set_position(data->data.position.x, data->data.position.y);
    }
    delete data;
}

void FbtLvProgressBar::do_initialize() {
    if (container_bg_)
        return; // 已经初始化过了

    lv_obj_t *parent = lv_scr_act();
    if (!parent)
        return;

    // 1. 创建背景
    container_bg_ = lv_obj_create(parent);
    if (!container_bg_)
        return;
    int32_t padding = 8;       // 统一的边距
    int32_t label_height = 25; // 底部标签区域高度

    lv_obj_set_size(container_bg_, param_.width, param_.height);
    lv_obj_set_pos(container_bg_, param_.position_x, param_.position_y);
    lv_obj_set_style_bg_color(container_bg_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(container_bg_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container_bg_, param_.width / 2, 0);
    lv_obj_set_style_border_width(container_bg_, 1, 0);
    lv_obj_set_style_border_color(container_bg_, lv_color_hex(0x555555), 0);
    lv_obj_set_style_pad_all(container_bg_, 0, 0);
    lv_obj_set_style_pad_left(container_bg_, 0, 0);
    lv_obj_set_style_pad_right(container_bg_, 0, 0);
    lv_obj_set_style_pad_top(container_bg_, 0, 0);
    lv_obj_set_style_pad_bottom(container_bg_, 0, 0);
    lv_obj_clear_flag(container_bg_, LV_OBJ_FLAG_SCROLLABLE);

    // 2. 创建进度条
    lv_obj_t *bar_bg = lv_obj_create(container_bg_);
    if (!bar_bg)
        return;
    bar_bg_width_ = param_.width - padding * 2;
    bar_bg_height_ = param_.height - padding * 2 - label_height;
    bar_radius_ = bar_bg_width_ / 2;

    lv_obj_set_size(bar_bg, bar_bg_width_, bar_bg_height_);
    // 对齐到容器顶部，水平居中
    lv_obj_align(bar_bg, LV_ALIGN_TOP_MID, 0, 8);
    // 背景样式：深灰色，全圆角
    lv_obj_set_style_bg_color(bar_bg, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(bar_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar_bg, bar_radius_, 0); // 全圆角
    lv_obj_set_style_border_width(bar_bg, 0, 0);
    // 关键：移除所有内边距
    lv_obj_set_style_pad_all(bar_bg, 0, 0);
    lv_obj_set_style_pad_left(bar_bg, 0, 0);
    lv_obj_set_style_pad_right(bar_bg, 0, 0);
    lv_obj_set_style_pad_top(bar_bg, 0, 0);
    lv_obj_set_style_pad_bottom(bar_bg, 0, 0);

    bar_obj_ = lv_obj_create(bar_bg);
    if (!bar_obj_)
        return;
    lv_obj_set_size(bar_obj_, bar_bg_width_, 0);

    // 对齐到背景底部中心
    lv_obj_align(bar_obj_, LV_ALIGN_BOTTOM_MID, 0, 0);

    // 进度条样式
    lv_obj_set_style_bg_opa(bar_obj_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_obj_, 0, 0);
    // 关键：移除所有内边距
    lv_obj_set_style_pad_all(bar_obj_, 0, 0);

    // 设置圆角（根据实际高度动态调整）
    if (0 < bar_bg_width_) {
        lv_obj_set_style_radius(bar_obj_, bar_radius_, 0);
    }

    value_label_ = lv_label_create(container_bg_);
    if (value_label_) {
        lv_obj_set_style_text_color(value_label_, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(value_label_, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(value_label_, LV_ALIGN_BOTTOM_MID, 0, -5);
    }
    if (!param_.default_show) {
        do_hide();
    } else {
        do_show();
    }

    update_display();
}

void FbtLvProgressBar::do_touch_delta_height(int32_t start_y, int32_t end_y) {
    int32_t delta_y = end_y - start_y;

    // 计算步数
    const int32_t step_pixels = param_.step_pixels; // 每2像素变化一个单位
    int32_t steps = delta_y / step_pixels;

    // 根据方向调整值
    int32_t new_value = param_.current_value;
    if (steps > 0) {
        // 向下滑动，减少音量
        new_value -= steps;
    } else if (steps < 0) {
        // 向上滑动，增加音量
        new_value += (-steps); // steps是负数，所以取反
    }

    // 限制范围
    if (new_value < 0)
        new_value = 0;
    if (new_value > 100)
        new_value = 100;
    do_set_value(new_value);
    are_changes_value_ = true;
}

void FbtLvProgressBar::do_set_value(int32_t value) {
    if (!bar_obj_ || !container_bg_)
        return;

    param_.current_value = value;
    update_display();
}

void FbtLvProgressBar::do_show() {
    if (container_bg_) {
        lv_obj_clear_flag(container_bg_, LV_OBJ_FLAG_HIDDEN);
        is_showing_ = true;
        lv_obj_move_foreground(container_bg_); // 移到最上层
    }
}

void FbtLvProgressBar::do_hide() {
    if (container_bg_) {
        lv_obj_add_flag(container_bg_, LV_OBJ_FLAG_HIDDEN);
        is_showing_ = false;
        are_changes_value_ = false;
    }
}

void FbtLvProgressBar::do_set_position(int32_t x, int32_t y) {
    if (!container_bg_)
        return;

    param_.position_x = x;
    param_.position_y = y;

    lv_obj_set_pos(container_bg_, x, y);
}

void FbtLvProgressBar::update_display() {
    // 更新进度条高度（从底部向上增长）
    if (bar_obj_) {
        // 计算百分比
        float percent = (float)(param_.current_value) / 100;

        // 计算进度条高度
        int32_t bar_height = (int32_t)(bar_bg_height_ * percent);

        // 设置高度
        lv_obj_set_size(bar_obj_, bar_bg_width_, bar_height);
        // 重新对齐到底部
        lv_obj_align(bar_obj_, LV_ALIGN_BOTTOM_MID, 0, 0);

        // 如果高度为0，隐藏圆角
        if (bar_height == 0) {
            lv_obj_set_style_radius(bar_obj_, 0, 0);
        } else {
            if (bar_height < bar_bg_width_) {
                // 高度小于宽度时，使用高度的一半作为圆角
                lv_obj_set_style_radius(bar_obj_, bar_height / 2, 0);
            } else {
                // 否则使用全圆角
                lv_obj_set_style_radius(bar_obj_, bar_radius_, 0);
            }
        }
    }

    // 更新数值标签
    if (value_label_) {
        lv_label_set_text_fmt(value_label_, "%ld", param_.current_value);
    }

    update_bar_color();
}

// 更新进度条颜色
void FbtLvProgressBar::update_bar_color() {
    if (!bar_obj_)
        return;

    lv_color_t color;
    if (param_.current_value == 0) {
        color = lv_color_hex(0x808080); // 灰色（静音）
    } else if (param_.current_value < 30) {
        color = lv_color_hex(0x4CAF50); // 绿色（低音量）
    } else if (param_.current_value < 70) {
        color = lv_color_hex(0x2196F3); // 蓝色（中音量）
    } else {
        color = lv_color_hex(0xFF5722); // 橙色（高音量）
    }

    lv_obj_set_style_bg_color(bar_obj_, color, 0);
}