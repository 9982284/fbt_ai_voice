#include "fbt_lvgl_label.h"

void FbtLvglLabel::CreateLabel(const FbtLabelParam &param) {
    lv_obj_t *parent = param.parent ? param.parent : lv_scr_act();
    if (!parent) {
        return;
    }
    context_ = lv_label_create(parent);
    lv_obj_align(context_, param.area, param.offset_x, param.offset_y);
    if (!param.text.empty()) {
        lv_label_set_text(context_, param.text.c_str());
    }
    if (param.text_color) {
        lv_obj_set_style_text_color(context_, lv_color_hex(param.text_color), LV_STATE_DEFAULT);
        // lv_obj_set_style_text_color(context_, lv_palette_main(param.text_color), LV_STATE_DEFAULT);
    }
}

void FbtLvglLabel::SetValue(std::string value) {
    if (context_) {
        pending_text_ = value; // 缓存到成员变量
        if (context_) {
            lv_async_call(update_text_task, this);
        }
    }
}
