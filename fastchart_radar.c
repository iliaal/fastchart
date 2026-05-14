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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

#include <math.h>

/* Match the public setter cap so accepted axes render end-to-end.
 * 128 gdPoint = 1 KB on the stack. The previous 32 silently dropped
 * axes 33..N from charts whose series were built within the
 * advertised FASTCHART_MAX_RADAR_VALUES limit. */
#define MAX_RADAR_AXES   FASTCHART_MAX_RADAR_VALUES

/* Read a typed-series cell, treating out-of-range as 0. */
static inline double radar_read_d(const fastchart_radar_series *s, int i)
{
    if (i >= s->len) return 0.0;
    double d = s->values[i];
    return (d < 0 || isnan(d)) ? 0.0 : d;
}

int fastchart_radar_render_to_target(fastchart_radar_obj *self, fastchart_target_t *t)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\RadarChart::draw() requires setSeries() with at least 3 axes per series");
        return -1;
    }
    fastchart_radar_series *series = self->series;
    int n_series = self->n_series;

    /* n_axes = max series len. Each axis is one entry index. */
    int n_axes = 0;
    for (int s = 0; s < n_series; s++) {
        if (series[s].len > n_axes) n_axes = series[s].len;
    }
    if (n_axes > MAX_RADAR_AXES) n_axes = MAX_RADAR_AXES;
    if (n_axes < 3) {
        zend_throw_error(NULL,
            "FastChart\\RadarChart::draw() requires setSeries() with at least 3 axes per series");
        return -1;
    }

    double dmax = 0.0;
    for (int s = 0; s < n_series; s++) {
        for (int i = 0; i < n_axes; i++) {
            double v = radar_read_d(&series[s], i);
            if (v > dmax) dmax = v;
        }
    }
    if (self->radar_max > 0) dmax = self->radar_max;
    if (dmax <= 0) dmax = 1.0;

    /* Per-render entry: invalidate the font cache (so a runtime
     * open_basedir narrowing between draws is honored) and stamp DPI
     * on the canvas. Must come BEFORE any palette / text work. */
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    /* Reserve top space for title. */
    int title_h = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 8;
    int cx = W / 2;
    int cy = (title_h + H) / 2;
    int radius = (W < H ? W : H) / 2 - 60;
    if (radius < 40) radius = 40;

    /* Pre-compute the per-axis sin/cos table once. The grid loop,
     * spoke loop, label loop, and series polygon loop all share
     * these N angles, so without the cache each axis paid for 4
     * cos+sin evaluations per render. */
    double cos_a[MAX_RADAR_AXES];
    double sin_a[MAX_RADAR_AXES];
    for (int i = 0; i < n_axes; i++) {
        double angle = -M_PI / 2.0 + 2 * M_PI * i / n_axes;
        cos_a[i] = cos(angle);
        sin_a[i] = sin(angle);
    }

    /* Concentric grid: 4 rings + axis spokes. */
    const int rings = 4;
    for (int r = 1; r <= rings; r++) {
        double rr = (double)radius * (double)r / (double)rings;
        gdPoint poly[MAX_RADAR_AXES];
        for (int i = 0; i < n_axes; i++) {
            poly[i].x = cx + (int)(rr * cos_a[i]);
            poly[i].y = cy + (int)(rr * sin_a[i]);
        }
        fastchart_target_polygon(t, poly, n_axes, pal.grid, 0, 1);
    }
    /* Spokes from center to each axis tip. */
    for (int i = 0; i < n_axes; i++) {
        int tx = cx + (int)(radius * cos_a[i]);
        int ty = cy + (int)(radius * sin_a[i]);
        fastchart_target_line(t, cx, cy, tx, ty, pal.axis, 1, FASTCHART_DASH_SOLID);
    }

    /* Axis labels via setCategoryLabels. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base);
    fastchart_obj *chart_base = (fastchart_obj *)self;
    if (font && chart_base->category_labels) {
        for (int i = 0; i < n_axes; i++) {
            const char *label = (i < chart_base->n_category_labels)
                ? chart_base->category_labels[i] : NULL;
            if (!label) continue;
            int lx = cx + (int)((radius + 16) * cos_a[i]);
            int ly = cy + (int)((radius + 16) * sin_a[i]);
            fastchart_align align;
            if (cos_a[i] > 0.3) align = FASTCHART_ALIGN_LEFT;
            else if (cos_a[i] < -0.3) align = FASTCHART_ALIGN_RIGHT;
            else align = FASTCHART_ALIGN_CENTER;
            fastchart_text_draw(t, font, size, pal.text,
                                lx, ly + (int)(size * 0.35),
                                align, label, NULL, 0);
        }
    }

    /* One polygon per series. */
    int legend_colors[FASTCHART_MAX_RADAR_SERIES];
    const char *legend_labels[FASTCHART_MAX_RADAR_SERIES];
    int legend_count = 0;

    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        if (series[s].color_rgb >= 0) {
            color = fastchart_target_color_rgb(t, (int)series[s].color_rgb);
        }
        gdPoint poly[MAX_RADAR_AXES];
        for (int i = 0; i < n_axes; i++) {
            double v = radar_read_d(&series[s], i);
            double rr = (double)radius * v / dmax;
            poly[i].x = cx + (int)(rr * cos_a[i]);
            poly[i].y = cy + (int)(rr * sin_a[i]);
        }
        if (self->radar_filled) {
            uint32_t rgba = fastchart_target_color_to_rgba(t, color);
            int rr = (rgba >> 16) & 0xFF;
            int gg = (rgba >>  8) & 0xFF;
            int bb =  rgba        & 0xFF;
            /* gd_alpha 90 → byte 255 - 90*2 = 75. */
            int alpha = fastchart_target_color(t, rr, gg, bb, 75);
            fastchart_target_polygon(t, poly, n_axes, alpha, 1, 0);
        }
        fastchart_target_polygon(t, poly, n_axes, color, 0, 2);
        /* Markers at vertices. */
        for (int i = 0; i < n_axes; i++) {
            fastchart_draw_marker(t, poly[i].x, poly[i].y,
                                  FASTCHART_MARKER_CIRCLE, 5, color);
        }
        if (series[s].label && legend_count < FASTCHART_MAX_RADAR_SERIES) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Title at top center. */
    fastchart_draw_floating_title(t, (fastchart_obj *)self, &pal, W / 2, 24);

    /* Legend: reuse the standard helper with a synthetic "plot rect"
     * spanning the entire canvas so positioning works. */
    if (legend_count > 0) {
        fastchart_rect plot = { 10, title_h, W - 10, H - 10 };
        fastchart_draw_legend(t, (fastchart_obj *)self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
