
#ifndef FBT_ICON_FONT_H
#define FBT_ICON_FONT_H

#include "lvgl.h"

// 声明字体变量
#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t fbt_icons_18;
extern const lv_font_t fbt_icons_24;
extern const lv_font_t fbt_icons_32;

#ifdef __cplusplus
}
#endif
// 字体指针
#define FBT_ICON_FONT_18 &fbt_icons_18
#define FBT_ICON_FONT_24 &fbt_icons_24
#define FBT_ICON_FONT_32 &fbt_icons_32

#define ICON_CLOSE "\ue60a"         // 关闭
#define ICON_WALKIE_TALKIE "\ue622" // 对讲机
#define ICON_HANG_UP "\ue9da"       // 对讲机
#define ICON_SPEAKER "\ue8c1"       // 扬声器
#define ICON_MIC "\ue8c2"           // 麦克风
#define ICON_USER_GROUP "\ue7d3"    // 用户组
#define ICON_PHONE "\ue7ca"         // 电话
#define ICON_SETTINGS "\ue7d6"      // 设置

typedef enum {
    FONT_SIZE_SMALL,
    FONT_SIZE_MODERATE,
    FONT_SIZE_BIG
} FbtIconSize;

// 根据枚举获取字体
static inline const lv_font_t *get_icon_font(FbtIconSize size) {
    switch (size) {
        case FONT_SIZE_SMALL:
            return FBT_ICON_FONT_18;
        case FONT_SIZE_MODERATE:
            return FBT_ICON_FONT_24;
        case FONT_SIZE_BIG:
            return FBT_ICON_FONT_32;
        default:
            return FBT_ICON_FONT_18;
    }
}

#endif