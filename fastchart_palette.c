/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2025-2026, Ilia Alshanetsky                            |
  | Copyright (c) 2025-2026, Advanced Internet Designs Inc.              |
  +----------------------------------------------------------------------+
  | This source file is subject to the BSD 3-Clause license that is      |
  | bundled with this package in the file LICENSE.                       |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"

/* RGB tuple for a palette entry. Resolved into a target color handle
 * by fastchart_palette_init below. */
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

void fastchart_palette_init(fastchart_target_t *t, int theme, fastchart_palette *pal)
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

    pal->bg      = fastchart_target_color(t, bg.r, bg.g, bg.b, 0xFF);
    pal->plot_bg = fastchart_target_color(t, plot_bg.r, plot_bg.g, plot_bg.b, 0xFF);
    pal->axis    = fastchart_target_color(t, axis.r, axis.g, axis.b, 0xFF);
    pal->grid    = fastchart_target_color(t, grid.r, grid.g, grid.b, 0xFF);
    pal->text    = fastchart_target_color(t, text.r, text.g, text.b, 0xFF);
    pal->border  = fastchart_target_color(t, border.r, border.g, border.b, 0xFF);
    pal->up      = fastchart_target_color(t, up.r, up.g, up.b, 0xFF);
    pal->down    = fastchart_target_color(t, down.r, down.g, down.b, 0xFF);
    pal->volume  = fastchart_target_color(t, volume.r, volume.g, volume.b, 0xFF);

    for (int i = 0; i < FASTCHART_PALETTE_SERIES_N; i++) {
        pal->series[i] = fastchart_target_color(t,
            series_src[i].r, series_src[i].g, series_src[i].b, 0xFF);
    }
}

void fastchart_palette_apply_overrides(fastchart_target_t *t,
                                        const fastchart_obj *chart,
                                        fastchart_palette *pal)
{
    if (chart->bg_override >= 0) {
        pal->bg = fastchart_target_color_rgb(t, (int)chart->bg_override);
        /* If only the canvas bg is overridden, mirror to plot_bg
         * so the plot area doesn't visually float on a different
         * background. setPlotBackgroundColor() unsticks them. */
        if (chart->plot_bg_override < 0) {
            pal->plot_bg = pal->bg;
        }
    }
    if (chart->plot_bg_override >= 0) {
        pal->plot_bg = fastchart_target_color_rgb(t, (int)chart->plot_bg_override);
    }
    for (int i = 0; i < chart->series_colors_n && i < FASTCHART_PALETTE_SERIES_N; i++) {
        if (chart->series_colors[i] < 0) continue;
        pal->series[i] = fastchart_target_color_rgb(t, (int)chart->series_colors[i]);
    }

    /* Per-element color overrides (axis line, grid lines, border,
     * text). The palette already carries theme defaults, so each
     * field stays whatever theme set unless the user overrode it. */
#define APPLY_COLOR_OVERRIDE(field_, override_) \
    do { \
        if (chart->override_ >= 0) { \
            pal->field_ = fastchart_target_color_rgb(t, (int)chart->override_); \
        } \
    } while (0)
    APPLY_COLOR_OVERRIDE(axis,   axis_color_override);
    APPLY_COLOR_OVERRIDE(grid,   grid_color_override);
    APPLY_COLOR_OVERRIDE(border, border_color_override);
    APPLY_COLOR_OVERRIDE(text,   text_color_override);
#undef APPLY_COLOR_OVERRIDE
}
