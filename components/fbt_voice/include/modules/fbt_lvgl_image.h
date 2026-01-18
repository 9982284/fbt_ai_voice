#ifndef FBT_LVGL_IMAGE_H
#define FBT_LVGL_IMAGE_H

#include <functional>
#include <lvgl.h>
#include <memory>
#include <string>

#define IMAGE_LIST                  \
    IMAGE_ENTRY(fbt_phone_bg_1_jpg) \
    IMAGE_ENTRY(fbt_phone_bg_2_jpg) \
    IMAGE_ENTRY(fbt_phone_bg_3_jpg)

// 2. 声明所有符号
#define IMAGE_ENTRY(name)                          \
    extern const uint8_t _binary_##name##_start[]; \
    extern const uint8_t _binary_##name##_end[];
IMAGE_LIST
#undef IMAGE_ENTRY

struct FbtImageParam {
    /**
     * 父级元素
     */
    lv_obj_t *parent = nullptr;
    /**
     * 文件名
     */
    std::string file_name;
    /**
     * 是否全屏显示
     */
    bool full_screen = true;
    /**
     * 是否启用黑色遮罩层
     */
    bool enable_overlay = false;

    /**
     * 遮罩层透明度 (0-255, 值越小越透明)
     * 例如：128表示50%黑色遮罩
     */
    lv_opa_t overlay_alpha = LV_OPA_50; // 默认50%透明度
    /**
     * 初始化时是否显示
     */
    bool show = true;
};

class FbtLvglImage {

  public:
    FbtLvglImage(/* args */) {};
    ~FbtLvglImage() {
        if (IsInitialized()) {
            lv_obj_delete(context_);
        }
    };
    bool CreateJpg(const FbtImageParam &param);
    void Show();
    void Hide();
    bool IsInitialized() const { return context_ != nullptr; }

    lv_obj_t *GetContext() const { return context_; }

  private:
    lv_obj_t *context_ = nullptr;

    FbtImageParam param_;

    uint8_t *rgb_data_;

    size_t rgb_width_ = 0;

    size_t rgb_height_ = 0;

    size_t rgb_size_ = 0;

    size_t rgb_stride_ = 0;
    static void initialize_async(void *arg);

    void do_init();

    static void free_image_data(lv_event_t *e);

    bool is_show = true;
};

#endif