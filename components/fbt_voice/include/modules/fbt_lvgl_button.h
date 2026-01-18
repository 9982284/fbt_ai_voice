#ifndef FBT_LVGL_BUTTON_H
#define FBT_LVGL_BUTTON_H

#include "fbt_icon_font.h"
#include <functional>
#include <lvgl.h>
#include <memory>
#include <string>

struct FbtButtonParam {
    /**
     * 父级元素
     */
    lv_obj_t *parent = nullptr;
    /**
     * 按钮的大小,圆形时有效
     */
    int32_t size;
    /**
     * 水平内边距
     */
    int32_t pad_hor = 20;
    /**
     * 垂直内边距
     */
    int32_t pad_ver = 10;
    /**
     * icon字符
     */
    const char *icon;
    /**
     * icon 大小
     */
    FbtIconSize icon_size;
    /**
     * 是否圆形
     */
    bool circle;
    /**
     * 背景色
     */
    lv_palette_t pg_color = LV_PALETTE_LIGHT_BLUE;
    /**
     * 对齐方式
     */
    lv_align_t area = LV_ALIGN_DEFAULT;
    /**
     * 相对对齐x轴偏移
     */
    int32_t offset_x = 0;
    /**
     * 相对对齐y轴偏移
     */
    int32_t offset_y = 0;

    int32_t radius = 0;
    /**
     * 按钮的内容,暂时不支持跟icon同时存在
     */
    std::string text;

    lv_palette_t text_color;
    /**
     * 纯透明背景
     */
    bool bg_transparent;
    /**
     * 默认是否显示
     */
    bool show = true;
    /**
     * 启用监听事件
     */
    bool enable_event = true;
};
class FbtLvglButton {

  public:
    FbtLvglButton() {};
    ~FbtLvglButton() {};

    // 设置点击回调
    void OnClick(std::function<void()> callback) { on_click_ = std::move(callback); };
    void OnPressDown(std::function<void()> callback) { on_press_down_ = std::move(callback); };
    void OnPressUp(std::function<void()> callback) { on_press_up_ = std::move(callback); };

    void CreateBtn(const FbtButtonParam &param);
    void Show();
    void Hide();
    void SetValue(const std::string &value);
    void SetIcon(const char *icon);
    bool IsInitialized() const { return button_ != nullptr; }

    void SetOffset(const int32_t *x = nullptr, const int32_t *y = nullptr, const lv_align_t *area = nullptr);
    // 获取按钮对象（可选）
    lv_obj_t *GetButtonObject() const { return button_; }
    lv_obj_t *GetLabelObject() const { return label_; }

  private:
    // 异步回调（静态）
    static void initialize_async(void *arg);

    void do_create_btn();

    void set_bg_transparent();

    void set_radius();

    static void event_handler(lv_event_t *e);

  private:
    FbtButtonParam param_;
    lv_obj_t *button_ = nullptr;
    lv_obj_t *label_ = nullptr;

    // 点击回调
    std::function<void()> on_click_;
    std::function<void()> on_press_down_;
    std::function<void()> on_press_up_;

    // 用于事件回调的用户数据
    struct EventUserData {
        FbtLvglButton *button_ptr;
    };
    std::unique_ptr<EventUserData> event_user_data_;
};
#endif // FBT_LVGL_BUTTON_H