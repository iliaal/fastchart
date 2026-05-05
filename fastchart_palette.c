/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2026 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,     |
  | that is bundled with this package in the file LICENSE, and is       |
  | available through the world-wide-web at the following url:          |
  | http://www.php.net/license/3_01.txt                                 |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_fastchart.h"
#include "fastchart_palette.h"

/* RGB tuple for a palette entry. Resolved into an allocated gd color
 * index by fastchart_palette_init below. */
typedef struct {
    int r, g, b;
} rgb_t;

/* Eight series colors per theme. Picked for legibility on the
 * matching background and for adjacent-series contrast. */
static const rgb_t LIGHT_SERIES[FASTCHART_PALETTE_SERIES_N] = {
    { 0x1f, 0x77, 0xb4 },  /* blue */
    { 0xff, 0x7f, 0x0e },  /* orange */
    { 0x2c, 0xa0, 0x2c },  /* green */
    { 0xd6, 0x27, 0x28 },  /* red */
    { 0x94, 0x67, 0xbd },  /* purple */
    { 0x8c, 0x56, 0x4b },  /* brown */
    { 0xe3, 0x77, 0xc2 },  /* pink */
    { 0x7f, 0x7f, 0x7f },  /* gray */
};

static const rgb_t DARK_SERIES[FASTCHART_PALETTE_SERIES_N] = {
    { 0x4f, 0xa3, 0xd1 },
    { 0xff, 0xa6, 0x4d },
    { 0x60, 0xc5, 0x60 },
    { 0xe5, 0x5b, 0x5b },
    { 0xb5, 0x8a, 0xd6 },
    { 0xb0, 0x80, 0x73 },
    { 0xeb, 0x9c, 0xd8 },
    { 0xb0, 0xb0, 0xb0 },
};

void fastchart_palette_init(gdImagePtr im, int theme, fastchart_palette *pal)
{
    rgb_t bg, plot_bg, axis, grid, text, border, up, down, volume;
    const rgb_t *series_src;

    if (theme == FASTCHART_THEME_DARK) {
        bg       = (rgb_t){ 0x1a, 0x1a, 0x1a };
        plot_bg  = (rgb_t){ 0x22, 0x22, 0x22 };
        axis     = (rgb_t){ 0xc8, 0xc8, 0xc8 };
        grid     = (rgb_t){ 0x40, 0x40, 0x40 };
        text     = (rgb_t){ 0xee, 0xee, 0xee };
        border   = (rgb_t){ 0x60, 0x60, 0x60 };
        up       = (rgb_t){ 0x4c, 0xc6, 0x6e };  /* candle bull */
        down     = (rgb_t){ 0xe5, 0x5b, 0x5b };  /* candle bear */
        volume   = (rgb_t){ 0x55, 0x77, 0x99 };
        series_src = DARK_SERIES;
    } else {
        bg       = (rgb_t){ 0xff, 0xff, 0xff };
        plot_bg  = (rgb_t){ 0xff, 0xff, 0xff };
        axis     = (rgb_t){ 0x33, 0x33, 0x33 };
        grid     = (rgb_t){ 0xe0, 0xe0, 0xe0 };
        text     = (rgb_t){ 0x22, 0x22, 0x22 };
        border   = (rgb_t){ 0x66, 0x66, 0x66 };
        up       = (rgb_t){ 0x2c, 0xa0, 0x2c };
        down     = (rgb_t){ 0xd6, 0x27, 0x28 };
        volume   = (rgb_t){ 0x88, 0xa0, 0xb8 };
        series_src = LIGHT_SERIES;
    }

    pal->bg      = gdImageColorAllocate(im, bg.r, bg.g, bg.b);
    pal->plot_bg = gdImageColorAllocate(im, plot_bg.r, plot_bg.g, plot_bg.b);
    pal->axis    = gdImageColorAllocate(im, axis.r, axis.g, axis.b);
    pal->grid    = gdImageColorAllocate(im, grid.r, grid.g, grid.b);
    pal->text    = gdImageColorAllocate(im, text.r, text.g, text.b);
    pal->border  = gdImageColorAllocate(im, border.r, border.g, border.b);
    pal->up      = gdImageColorAllocate(im, up.r, up.g, up.b);
    pal->down    = gdImageColorAllocate(im, down.r, down.g, down.b);
    pal->volume  = gdImageColorAllocate(im, volume.r, volume.g, volume.b);

    for (int i = 0; i < FASTCHART_PALETTE_SERIES_N; i++) {
        pal->series[i] = gdImageColorAllocate(im,
            series_src[i].r, series_src[i].g, series_src[i].b);
    }
}

void fastchart_palette_apply_overrides(gdImagePtr im,
                                        const fastchart_obj *chart,
                                        fastchart_palette *pal)
{
    if (chart->bg_override >= 0) {
        pal->bg = gdImageColorAllocate(im,
            (int)((chart->bg_override >> 16) & 0xFF),
            (int)((chart->bg_override >>  8) & 0xFF),
            (int)( chart->bg_override        & 0xFF));
        /* If only the canvas bg is overridden, mirror to plot_bg
         * so the plot area doesn't visually float on a different
         * background. setPlotBackgroundColor() unsticks them. */
        if (chart->plot_bg_override < 0) {
            pal->plot_bg = pal->bg;
        }
    }
    if (chart->plot_bg_override >= 0) {
        pal->plot_bg = gdImageColorAllocate(im,
            (int)((chart->plot_bg_override >> 16) & 0xFF),
            (int)((chart->plot_bg_override >>  8) & 0xFF),
            (int)( chart->plot_bg_override        & 0xFF));
    }
    for (int i = 0; i < chart->series_colors_n && i < FASTCHART_PALETTE_SERIES_N; i++) {
        if (chart->series_colors[i] < 0) continue;
        pal->series[i] = gdImageColorAllocate(im,
            (chart->series_colors[i] >> 16) & 0xFF,
            (chart->series_colors[i] >>  8) & 0xFF,
             chart->series_colors[i]        & 0xFF);
    }

    /* Per-element color overrides (axis line, grid lines, border,
     * text). The palette already carries theme defaults, so each
     * field stays whatever theme set unless the user overrode it. */
#define APPLY_COLOR_OVERRIDE(field_, override_) \
    do { \
        if (chart->override_ >= 0) { \
            pal->field_ = gdImageColorAllocate(im, \
                (int)((chart->override_ >> 16) & 0xFF), \
                (int)((chart->override_ >>  8) & 0xFF), \
                (int)( chart->override_        & 0xFF)); \
        } \
    } while (0)
    APPLY_COLOR_OVERRIDE(axis,   axis_color_override);
    APPLY_COLOR_OVERRIDE(grid,   grid_color_override);
    APPLY_COLOR_OVERRIDE(border, border_color_override);
    APPLY_COLOR_OVERRIDE(text,   text_color_override);
#undef APPLY_COLOR_OVERRIDE
}
