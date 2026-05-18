/*******************************************************************************
 * Size: 16 px
 * Bpp: 4
 * Opts: --bpp 4 --size 16 --font components/ui/fonts/DS-DIGIT.TTF --range 0x41,0x4D,0x50 --format lvgl --output
 * components/ui/fonts/lv_font_ds_digital_16.c --no-compress
 ******************************************************************************/

#include "lvgl.h"

#ifndef LV_FONT_DS_DIGITAL_16
#define LV_FONT_DS_DIGITAL_16 1
#endif

#if LV_FONT_DS_DIGITAL_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0041 "A" */
    0x3, 0x9f, 0xff, 0xf9, 0x10, 0x6c, 0x68, 0x88, 0xf0, 0x8, 0xf0, 0x0, 0x9e, 0x0, 0x9c, 0x22, 0x28, 0xd0, 0x1, 0x9f,
    0xff, 0xa1, 0x0, 0xb7, 0x55, 0x59, 0x80, 0xe, 0x90, 0x0, 0xf8, 0x0, 0xf8, 0x0, 0xf, 0x70, 0x1e, 0x20, 0x0, 0xb6,
    0x1, 0x10, 0x0, 0x0, 0x20,

    /* U+004D "M" */
    0x3, 0x9f, 0xff, 0xf8, 0x10, 0x6c, 0x68, 0x87, 0xf0, 0x7, 0xf0, 0xc6, 0x9f, 0x0, 0x9e, 0x1f, 0x7a, 0xd0, 0x4, 0x32,
    0xf5, 0x26, 0x0, 0x62, 0x2f, 0x33, 0x50, 0xd, 0xa0, 0x10, 0xe9, 0x0, 0xf9, 0x0, 0xf, 0x80, 0xf, 0x50, 0x0, 0xe6,
    0x1, 0x60, 0x0, 0x3, 0x40,

    /* U+0050 "P" */
    0x3, 0x9f, 0xff, 0xc0, 0x0, 0x6c, 0x68, 0x88, 0xb0, 0x8, 0xf0, 0x0, 0x9f, 0x0, 0x9c, 0x22, 0x28, 0xd0, 0x1, 0x9f,
    0xff, 0xa1, 0x0, 0xb7, 0x55, 0x51, 0x0, 0xe, 0x90, 0x0, 0x0, 0x0, 0xf8, 0x0, 0x0, 0x0, 0x1e, 0x20, 0x0, 0x0, 0x1,
    0x10, 0x0, 0x0, 0x0};

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 130, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 130, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 130, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {0x0, 0xc, 0xf};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] = {{.range_start = 65,
                                                .range_length = 16,
                                                .glyph_id_start = 1,
                                                .unicode_list = unicode_list_0,
                                                .glyph_id_ofs_list = NULL,
                                                .list_length = 3,
                                                .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY}};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static lv_font_fmt_txt_glyph_cache_t cache;
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
    .bpp = 4,
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
const lv_font_t lv_font_ds_digital_16 = {
#else
lv_font_t lv_font_ds_digital_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt, /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt, /*Function pointer to get glyph's bitmap*/
    .line_height = 10,                              /*The maximum line height required by the font*/
    .base_line = 0,                                 /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc, /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};

#endif /*#if LV_FONT_DS_DIGITAL_16*/
