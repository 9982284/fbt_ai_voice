#ifndef FBT_LVGL_LABEL_H
#define FBT_LVGL_LABEL_H

#include <esp_log.h>
#include <functional>
#include <lvgl.h>
#include <memory>
#include <string>

struct FbtLabelParam {
    /**
     * 父级元素
     */
    lv_obj_t *parent = nullptr;

    std::string text = "text";

    uint32_t text_color;
    /**
     * 对齐方式
     */
    lv_align_t area = LV_ALIGN_CENTER;
    /**
     * 相对对齐x轴偏移
     */
    int32_t offset_x = 0;
    /**
     * 相对对齐y轴偏移
     */
    int32_t offset_y = 0;
};

class FbtLvglLabel {

  public:
    FbtLvglLabel() {};
    ~FbtLvglLabel() {
        if (IsInitialized()) {
            lv_obj_delete(context_);
        }
    };

    void CreateLabel(const FbtLabelParam &param);

    void SetValue(std::string value);
    bool IsInitialized() const { return context_ != nullptr; }
    lv_obj_t *GetContext() const { return context_; }

  private:
    lv_obj_t *context_ = nullptr;
    std::string pending_text_;
    static void update_text_task(void *arg) {
        FbtLvglLabel *self = static_cast<FbtLvglLabel *>(arg);
        if (self && self->context_) {
            lv_label_set_text(self->context_, self->pending_text_.c_str());
        }
    }
};

#endif // FBT_LVGL_LABEL_H