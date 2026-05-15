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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Pareto chart: bar series (left Y axis, absolute values) + a
 * cumulative-percentage line overlay (right Y axis, 0..100%).
 * Caller controls bar order — the rendering does NOT re-sort. */
int fastchart_pareto_render_to_target(fastchart_pareto_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->bar_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\ParetoChart::draw() requires setBars() with at least one bar");
        return -1;
    }

    /* Cumulative totals; total is the grand sum used for percentages. */
    double total = 0.0;
    for (int i = 0; i < self->bar_count; i++) {
        total += self->bars[i].value;
    }
    if (total <= 0.0) {
        zend_throw_error(NULL,
            "FastChart\\ParetoChart::draw() requires at least one positive bar");
        return -1;
    }
    double y_max = 0.0;
    for (int i = 0; i < self->bar_count; i++) {
        if (self->bars[i].value > y_max) y_max = self->bars[i].value;
    }
    fastchart_value_range yr;
    fastchart_value_range_compute(0, y_max, 5, &yr);
    double y_axis_max = yr.max > 0 ? yr.max : y_max;

    /* Plot rect: leave room on left for bar-axis ticks, right for
     * the 0..100% axis. */
    int top_pad = 16;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(t, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top_pad += title_h + 8;
        }
    }
    int plot_x0 = 60;
    int plot_x1 = W - 60;
    int plot_y0 = top_pad;
    int plot_y1 = H - 48;
    if (plot_x1 <= plot_x0 || plot_y1 <= plot_y0) {
        zend_throw_error(NULL,
            "FastChart\\ParetoChart::draw() canvas is too small for margins");
        return -1;
    }

    /* Plot background + frame. */
    fastchart_target_rect(t, plot_x0, plot_y0,
                          plot_x1 - plot_x0 + 1, plot_y1 - plot_y0 + 1,
                          pal.plot_bg, 1, 0);

    /* Left-axis ticks. 5 evenly-spaced including 0 and max. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base_size);
    const char *fmt = self->value_label_format
        ? ZSTR_VAL(self->value_label_format) : "%.0f";
    for (int k = 0; k <= 5; k++) {
        double v = y_axis_max * k / 5.0;
        int y = plot_y1 - (int)((double)k / 5.0 * (plot_y1 - plot_y0));
        fastchart_target_line(t, plot_x0, y, plot_x1, y,
                              pal.grid, 1, FASTCHART_DASH_SOLID);
        if (font) {
            char buf[32];
            snprintf(buf, sizeof(buf), fmt, v);
            fastchart_text_draw(t, font, size, pal.text,
                                plot_x0 - 6, y + (int)(size * 0.4),
                                FASTCHART_ALIGN_RIGHT, buf, NULL, 0);
        }
    }
    /* Right-axis ticks at 0/25/50/75/100%. */
    for (int k = 0; k <= 4; k++) {
        double pct = k * 25.0;
        int y = plot_y1 - (int)(pct / 100.0 * (plot_y1 - plot_y0));
        if (font) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d%%", (int)pct);
            fastchart_text_draw(t, font, size, pal.text,
                                plot_x1 + 6, y + (int)(size * 0.4),
                                FASTCHART_ALIGN_LEFT, buf, NULL, 0);
        }
    }
    fastchart_target_rect(t, plot_x0, plot_y0,
                          plot_x1 - plot_x0 + 1, plot_y1 - plot_y0 + 1,
                          pal.border, 0, 1);

    /* Bars. Equal-width slots, small gap between bars. */
    int slot_w = (plot_x1 - plot_x0) / self->bar_count;
    int gap = slot_w / 8;
    if (gap < 1) gap = 1;
    int bar_w = slot_w - 2 * gap;
    if (bar_w < 1) bar_w = 1;
    for (int i = 0; i < self->bar_count; i++) {
        int bx = plot_x0 + i * slot_w + gap;
        double frac = self->bars[i].value / y_axis_max;
        if (frac < 0) frac = 0;
        if (frac > 1) frac = 1;
        int bh = (int)(frac * (plot_y1 - plot_y0));
        int by = plot_y1 - bh;
        int color = self->bars[i].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->bars[i].color_rgb)
            : pal.series[i % FASTCHART_PALETTE_SERIES_N];
        fastchart_target_rect(t, bx, by, bar_w, bh, color, 1, 0);
        fastchart_target_rect(t, bx, by, bar_w, bh, pal.border, 0, 1);

        if (font && self->bars[i].label) {
            fastchart_text_draw(t, font, size, pal.text,
                                bx + bar_w / 2, plot_y1 + (int)(size * 1.4),
                                FASTCHART_ALIGN_CENTER,
                                self->bars[i].label, NULL, 0);
        }
        if (font && ((fastchart_obj *)self)->show_values) {
            char buf[32];
            snprintf(buf, sizeof(buf), fmt, self->bars[i].value);
            fastchart_text_draw(t, font, size * 0.9, pal.text,
                                bx + bar_w / 2, by - 4,
                                FASTCHART_ALIGN_CENTER, buf, NULL, 0);
        }
    }

    /* Cumulative line + markers, anchored at the right edge of each
     * bar slot (so the first point sits at the first bar's right
     * edge and 100% lands on the last bar's right edge). */
    int line_color = self->line_color >= 0
        ? fastchart_target_color_rgb(t, self->line_color)
        : pal.series[1 % FASTCHART_PALETTE_SERIES_N];
    double cum = 0.0;
    int prev_x = 0, prev_y = 0;
    bool have_prev = false;
    for (int i = 0; i < self->bar_count; i++) {
        cum += self->bars[i].value;
        double pct = cum / total;
        if (pct > 1.0) pct = 1.0;
        int px = plot_x0 + i * slot_w + slot_w - gap;
        int py = plot_y1 - (int)(pct * (plot_y1 - plot_y0));
        if (have_prev) {
            fastchart_target_line(t, prev_x, prev_y, px, py,
                                  line_color, 2, FASTCHART_DASH_SOLID);
        }
        /* Circle marker. */
        fastchart_target_ellipse(t, px, py, 6, 6, line_color, 1, 0);
        fastchart_target_ellipse(t, px, py, 6, 6, pal.border, 0, 1);
        prev_x = px; prev_y = py;
        have_prev = true;
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
