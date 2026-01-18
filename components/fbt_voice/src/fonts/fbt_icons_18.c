/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font ./iconfont.ttf -r 0xe000-0xf8ff --size 18 --bpp 1 --format lvgl -o fbt_icons_18.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef FBT_ICONS_18
#define FBT_ICONS_18 1
#endif

#if FBT_ICONS_18

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+E60A "" */
    0x1, 0xc0, 0x0, 0xe0, 0x0, 0x70, 0x0, 0x38,
    0x3, 0x9c, 0xe3, 0xce, 0x79, 0xc7, 0x1d, 0xe3,
    0x8f, 0xe1, 0xc3, 0xf0, 0xe1, 0xf8, 0x70, 0xfc,
    0x0, 0x7e, 0x0, 0x3b, 0x80, 0x38, 0xe0, 0x38,
    0x78, 0x3c, 0xf, 0xf8, 0x3, 0xf8, 0x0,

    /* U+E622 "" */
    0x0, 0xe0, 0x40, 0x30, 0x82, 0x11, 0x83, 0x23,
    0x55, 0x2f, 0xf6, 0x7f, 0xf0, 0xe0, 0x60, 0xc0,
    0xc1, 0x81, 0x83, 0xff, 0x7, 0xfe, 0x7, 0xf8,
    0xf, 0xf0, 0x1f, 0xe0, 0x3f, 0xc0, 0x7f, 0x80,
    0x7e, 0x0,

    /* U+E7CA "" */
    0xf0, 0x0, 0xf0, 0x0, 0xf8, 0x0, 0xf8, 0x0,
    0x78, 0x0, 0x78, 0x0, 0x70, 0x0, 0x30, 0x0,
    0x38, 0x0, 0x1c, 0x0, 0xe, 0x0, 0x7, 0x1c,
    0x3, 0xbf, 0x1, 0xff, 0x0, 0x7f, 0x0, 0x1f,

    /* U+E7D3 "" */
    0x7, 0xb8, 0x3, 0xf3, 0x0, 0xfc, 0xe0, 0x7f,
    0xb8, 0xf, 0xce, 0x3, 0xf3, 0x0, 0x7b, 0x80,
    0x0, 0x0, 0xf, 0xdf, 0x1f, 0xfe, 0x6f, 0xff,
    0xcf, 0xff, 0xf3, 0xff, 0xfc, 0xff, 0xff, 0x30,

    /* U+E7D6 "" */
    0x1, 0xe0, 0x0, 0x78, 0x0, 0x3f, 0x1, 0xff,
    0xfe, 0x7f, 0xff, 0xbf, 0x87, 0xf7, 0xe1, 0xf8,
    0xf0, 0x3c, 0x3c, 0xf, 0xf, 0x3, 0xc7, 0xe1,
    0xfb, 0xf8, 0x7f, 0x7f, 0xff, 0x8f, 0xff, 0xc0,
    0x3f, 0x0, 0x7, 0x80, 0x1, 0xe0, 0x0,

    /* U+E8C1 "" */
    0x0, 0x0, 0xb, 0x80, 0x63, 0x3, 0x82, 0x1e,
    0xcb, 0xf9, 0x9f, 0xe2, 0x7f, 0x89, 0xfe, 0x64,
    0x7b, 0x20, 0xe0, 0x81, 0x8c, 0x2, 0xe0, 0x0,
    0x0,

    /* U+E8C2 "" */
    0xe, 0x3, 0xe0, 0x7c, 0xf, 0x81, 0xf0, 0x3e,
    0x7, 0xc0, 0xf8, 0x9f, 0x31, 0xc7, 0x1, 0xb0,
    0x63, 0xf8, 0x8, 0x1, 0x0, 0x20,

    /* U+E9DA "" */
    0x7, 0xf8, 0xf, 0xff, 0xc7, 0xff, 0xfb, 0xf0,
    0x3f, 0xf8, 0x7, 0xfe, 0x1, 0xfc, 0x0, 0xc
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 288, .box_w = 17, .box_h = 18, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 39, .adv_w = 288, .box_w = 15, .box_h = 18, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 73, .adv_w = 288, .box_w = 16, .box_h = 16, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 105, .adv_w = 288, .box_w = 18, .box_h = 14, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 137, .adv_w = 288, .box_w = 18, .box_h = 17, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 176, .adv_w = 288, .box_w = 14, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 288, .box_w = 11, .box_h = 16, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 223, .adv_w = 288, .box_w = 18, .box_h = 7, .ofs_x = 0, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x18, 0x1c0, 0x1c9, 0x1cc, 0x2b7, 0x2b8, 0x3d0
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 58890, .range_length = 977, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 8, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t fbt_icons_18 = {
#else
lv_font_t fbt_icons_18 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 18,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if FBT_ICONS_18*/

