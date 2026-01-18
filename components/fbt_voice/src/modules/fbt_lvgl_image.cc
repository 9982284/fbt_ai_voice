#include "fbt_lvgl_image.h"
#include "jpeg_to_image.h"
#include <esp_log.h>
#include <stdio.h>

#define TAG "FbtLvglImage"

bool FbtLvglImage::CreateJpg(const FbtImageParam &param) {
    param_ = param;
    const uint8_t *jpg_start = nullptr;
    const uint8_t *jpg_end = nullptr;

// 3. 查找图片
#define IMAGE_ENTRY(name)                   \
    if (param.file_name == #name) {         \
        jpg_start = _binary_##name##_start; \
        jpg_end = _binary_##name##_end;     \
    } else
    IMAGE_LIST
    /* 最后一个else处理未找到的情况 */
    {
        ESP_LOGE(TAG, "未知图片: %s", param.file_name.c_str());
        return false;
    }
#undef IMAGE_ENTRY

    size_t jpeg_size = jpg_end - jpg_start;
    if (jpeg_size <= 100) {
        ESP_LOGE(TAG, "图片大小无效: %d字节", jpeg_size);
        return false;
    }

    uint8_t *jpeg_data = (uint8_t *)jpg_start;

    esp_err_t ret = jpeg_to_image(jpeg_data, jpeg_size,
                                  &rgb_data_, &rgb_size_,
                                  &rgb_width_, &rgb_height_, &rgb_stride_);

    if (ret != ESP_OK || !rgb_data_) {
        ESP_LOGE(TAG, "JPEG 解码失败: %d", ret);
        return false;
    }

    // lv_async_call(initialize_async, this);
    do_init();
    return true;
}

void FbtLvglImage::initialize_async(void *arg) {
    FbtLvglImage *self = static_cast<FbtLvglImage *>(arg);
    self->do_init();
}

void FbtLvglImage::do_init() {
    lv_obj_t *parent = param_.parent ? param_.parent : lv_scr_act();
    if (!parent) {
        return;
    }

    // 1. 直接创建图片作为覆盖层
    context_ = lv_img_create(parent);

    // 2. 设置图片数据
    static lv_img_dsc_t img_dsc;
    img_dsc.header.w = rgb_width_;
    img_dsc.header.h = rgb_height_;
    img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    img_dsc.data_size = rgb_size_;
    img_dsc.data = rgb_data_;

    lv_img_set_src(context_, &img_dsc);

    // 3. 设置为全屏
    lv_obj_set_size(context_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(context_, 0, 0);
    lv_obj_set_style_img_recolor_opa(context_, LV_OPA_TRANSP, 0); // 无重着色

    // 4. 禁用所有交互
    lv_obj_clear_flag(context_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(context_, LV_OBJ_FLAG_SCROLLABLE);

    // 5. 添加半透明黑色遮罩
    if (param_.enable_overlay) {
        lv_obj_t *overlay_ = lv_obj_create(context_);
        lv_obj_set_size(overlay_, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(overlay_, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(overlay_, param_.overlay_alpha, 0);
        lv_obj_set_style_border_width(overlay_, 0, 0);
        lv_obj_set_style_pad_all(overlay_, 0, 0);
        lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_CLICKABLE);
    }

    is_show = param_.show;
    // 7. 初始隐藏
    if (!is_show) {
        lv_async_call([](void *obj) { lv_obj_add_flag((lv_obj_t *)obj, LV_OBJ_FLAG_HIDDEN); }, context_);
    }

    // 8. 设置删除回调
    lv_obj_add_event_cb(context_, free_image_data, LV_EVENT_DELETE, rgb_data_);
    ESP_LOGI(TAG, "图片加完成");
}

// 释放内存回调
void FbtLvglImage::free_image_data(lv_event_t *e) {
    uint8_t *data = (uint8_t *)lv_event_get_user_data(e);
    if (data) {
        free(data);
        ESP_LOGI("FbtUiPhone", "释放图片内存");
    }
}

void FbtLvglImage::Show() {
    if (is_show) {
        return;
    }
    if (context_) {
        is_show = true;
        lv_obj_clear_flag(context_, LV_OBJ_FLAG_HIDDEN);
    }
}

void FbtLvglImage::Hide() {
    if (!is_show) {
        return;
    }
    if (context_) {
        is_show = false;
        lv_async_call([](void *obj) { lv_obj_add_flag((lv_obj_t *)obj, LV_OBJ_FLAG_HIDDEN); }, context_);
    }
}
