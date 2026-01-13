#ifndef FBT_SIMPLE_DOT_H
#define FBT_SIMPLE_DOT_H

#include <lvgl.h>

#include "fbt_config.h"

class SimpleDot {
  private:
    static int desired_state_;
    static int actual_state_;
    static lv_obj_t *dot_;

  public:
    //
    /**
     * 设置颜色
     *
     * @param mode 0:红色 1 蓝色 2 隐藏
     */
    static void Set(int mode) {
        if (!FbtConfig::Config::show_voice_dot)
            return;
        desired_state_ = mode;

        if (!dot_) {
            lv_async_call(InitializeDot, nullptr);
        } else {
            lv_async_call(ApplyStateAsync, nullptr);
        }
    }

  private:
    // 异步初始化函数
    static void InitializeDot(void *) {
        // 创建圆点
        dot_ = lv_obj_create(lv_scr_act());
        if (!dot_)
            return;

        // 1. 大小和形状
        lv_obj_set_size(dot_, 15, 15);
        lv_obj_set_style_radius(dot_, LV_RADIUS_CIRCLE, 0);

        // 2. 边框和阴影
        lv_obj_set_style_border_width(dot_, 1, 0);
        lv_obj_set_style_border_color(dot_, lv_color_white(), 0);
        lv_obj_set_style_border_opa(dot_, LV_OPA_50, 0);

        // 3. 阴影效果（让圆点有立体感）

        // lv_obj_set_style_shadow_width(dot_, 5, 0);
        //  lv_obj_set_style_shadow_spread(dot_, 3, 0);

        lv_obj_set_style_shadow_ofs_x(dot_, 0, 0);
        lv_obj_set_style_shadow_ofs_y(dot_, 2, 0);

        // 4. 位置
        lv_obj_align(dot_, LV_ALIGN_TOP_LEFT, 15, 30);
        lv_obj_move_foreground(dot_);

        // 初始状态
        lv_obj_set_style_bg_opa(dot_, LV_OPA_TRANSP, 0);
        lv_obj_add_flag(dot_, LV_OBJ_FLAG_HIDDEN);
        actual_state_ = -1;

        ApplyState();
    }

    // 异步应用状态
    static void ApplyStateAsync(void *) {
        ApplyState();
    }

    // 实际应用状态
    static void ApplyState() {
        if (!dot_ || desired_state_ == actual_state_) {
            return;
        }

        if (desired_state_ == 0) { // 红色（美化）
            lv_obj_clear_flag(dot_, LV_OBJ_FLAG_HIDDEN);

            // 红色渐变（从亮红到深红）
            lv_obj_set_style_bg_color(dot_, lv_color_make(255, 80, 80), 0);      // 亮红色
            lv_obj_set_style_bg_grad_color(dot_, lv_color_make(200, 30, 30), 0); // 深红色
            lv_obj_set_style_bg_grad_dir(dot_, LV_GRAD_DIR_VER, 0);
            lv_obj_set_style_bg_opa(dot_, LV_OPA_COVER, 0);

            // lv_obj_set_style_shadow_color(dot_, lv_color_make(255, 100, 100), 0);

            actual_state_ = 0;

        } else if (desired_state_ == 1) { // 蓝色（美化）
            lv_obj_clear_flag(dot_, LV_OBJ_FLAG_HIDDEN);

            // 蓝色渐变（从天蓝到深蓝）
            lv_obj_set_style_bg_color(dot_, lv_color_make(100, 180, 255), 0);     // 天蓝色
            lv_obj_set_style_bg_grad_color(dot_, lv_color_make(30, 100, 200), 0); // 深蓝色
            lv_obj_set_style_bg_grad_dir(dot_, LV_GRAD_DIR_VER, 0);
            lv_obj_set_style_bg_opa(dot_, LV_OPA_COVER, 0);

            // 蓝色阴影
            // lv_obj_set_style_shadow_color(dot_, lv_color_make(100, 150, 255), 0);

            actual_state_ = 1;

        } else { // 隐藏
            // lv_obj_set_style_bg_opa(dot_, LV_OPA_TRANSP, 0);
            lv_obj_add_flag(dot_, LV_OBJ_FLAG_HIDDEN);
            actual_state_ = -1;
        }
    }
};

// 静态成员初始化
int SimpleDot::desired_state_ = -1;
int SimpleDot::actual_state_ = -1;
lv_obj_t *SimpleDot::dot_ = nullptr;

#endif // FBT_SIMPLE_DOT_H