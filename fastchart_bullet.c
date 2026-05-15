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

/* Bullet chart layout (Stephen Few).
 *
 *   +========================================================+
 *   | poor      | satisfactory      | good                   |   <- bands
 *   |   +------------------------+                    |      |   <- performance bar
 *   |                                       |                |   <- target tick
 *   +========================================================+
 *
 * The full-height background is partitioned by setBands(). The
 * performance bar (taller than the bands' visual half-height
 * inset, drawn in a strong color) runs from min to value. The
 * target tick is a thick vertical line spanning the full bar
 * height at the target value. */
int fastchart_bullet_render_to_target(fastchart_bullet_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    int top_pad = 12;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(t, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top_pad += title_h + 10;
        }
    }

    double mn = self->bullet_min;
    double mx = self->bullet_max;
    double v  = self->bullet_value;
    double tgt = self->bullet_target;
    if (mx <= mn) mx = mn + 1.0;
    if (v < mn) v = mn;
    if (v > mx) v = mx;

    /* Plot rect: wide horizontal band centered vertically with room
     * for value labels above (target callout) and min/max labels below. */
    int margin_x = 60;
    int bar_x0 = margin_x;
    int bar_x1 = W - margin_x;
    if (bar_x1 <= bar_x0) {
        zend_throw_error(NULL,
            "FastChart\\BulletChart::draw() canvas is too narrow "
            "for label margins (need at least 121 px wide)");
        return -1;
    }
    int band_h = 36;
    if (H - top_pad < band_h * 2 + 40) band_h = (H - top_pad - 40) / 2;
    if (band_h < 12) band_h = 12;
    int bar_y_mid = top_pad + (H - top_pad) / 2;
    int band_y0 = bar_y_mid - band_h / 2;
    int band_y1 = band_y0 + band_h;
    int perf_inset = band_h / 4;
    int perf_y0 = band_y0 + perf_inset;
    int perf_y1 = band_y1 - perf_inset;

    /* Band backdrop. */
    fastchart_target_rect(t, bar_x0, band_y0,
                          bar_x1 - bar_x0 + 1, band_h + 1,
                          pal.grid, 1, 0);
    for (int i = 0; i < self->n_bands; i++) {
        const fastchart_gauge_zone *bn = &self->bands[i];
        double t0 = (bn->from - mn) / (mx - mn);
        double t1 = (bn->to   - mn) / (mx - mn);
        if (t0 < 0) t0 = 0; if (t0 > 1) t0 = 1;
        if (t1 < 0) t1 = 0; if (t1 > 1) t1 = 1;
        if (t1 <= t0) continue;
        int color;
        if (bn->color_rgb >= 0) {
            color = fastchart_target_color_rgb(t, bn->color_rgb);
        } else {
            /* Default ramp: lighter for "low" bands, darker for "high".
             * 3 default greys cycle if more than 3 bands. */
            static const int default_greys[3] = { 0xCCCCCC, 0x999999, 0x666666 };
            color = fastchart_target_color_rgb(
                t, default_greys[i % 3]);
        }
        int zx0 = bar_x0 + (int)(t0 * (bar_x1 - bar_x0));
        int zx1 = bar_x0 + (int)(t1 * (bar_x1 - bar_x0));
        fastchart_target_rect(t, zx0, band_y0,
                              zx1 - zx0 + 1, band_h + 1,
                              color, 1, 0);
    }
    fastchart_target_rect(t, bar_x0, band_y0,
                          bar_x1 - bar_x0 + 1, band_h + 1,
                          pal.border, 0, 1);

    /* Performance bar: from min to value. Uses the first palette
     * series color (a strong saturated tone) to stand out from the
     * grey/colored bands behind. */
    double frac = (v - mn) / (mx - mn);
    int vx = bar_x0 + (int)(frac * (bar_x1 - bar_x0));
    int perf_color = pal.series[0];
    fastchart_target_rect(t, bar_x0, perf_y0,
                          vx - bar_x0 + 1, perf_y1 - perf_y0 + 1,
                          perf_color, 1, 0);
    fastchart_target_rect(t, bar_x0, perf_y0,
                          vx - bar_x0 + 1, perf_y1 - perf_y0 + 1,
                          pal.border, 0, 1);

    /* Target tick. Thick vertical line across the band, offset
     * slightly past the band edges so it's visible against the
     * performance bar fill. */
    if (isfinite(tgt) && tgt >= mn && tgt <= mx) {
        double tfrac = (tgt - mn) / (mx - mn);
        int tx = bar_x0 + (int)(tfrac * (bar_x1 - bar_x0));
        fastchart_target_line(t, tx, band_y0 - 4, tx, band_y1 + 4,
                              pal.text, 3, FASTCHART_DASH_SOLID);
    }

    /* Labels: min on the left of the bar, max on the right, value
     * floating above the performance-bar end, target value above
     * the tick. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    if (font) {
        char min_buf[32], max_buf[32], val_buf[64];
        const char *fmt = self->bullet_value_format
            ? ZSTR_VAL(self->bullet_value_format) : "%.0f";
        snprintf(min_buf, sizeof(min_buf), fmt, mn);
        snprintf(max_buf, sizeof(max_buf), fmt, mx);
        snprintf(val_buf, sizeof(val_buf), fmt, v);

        int label_y = band_y1 + (int)(size * 1.5);
        fastchart_text_draw(t, font, size, pal.text,
                            bar_x0, label_y, FASTCHART_ALIGN_LEFT,
                            min_buf, NULL, 0);
        fastchart_text_draw(t, font, size, pal.text,
                            bar_x1, label_y, FASTCHART_ALIGN_RIGHT,
                            max_buf, NULL, 0);
        fastchart_text_draw(t, font, size * 1.1, pal.text,
                            vx, band_y0 - 6, FASTCHART_ALIGN_CENTER,
                            val_buf, NULL, 0);
        if (isfinite(tgt) && tgt >= mn && tgt <= mx) {
            char tgt_buf[64];
            snprintf(tgt_buf, sizeof(tgt_buf), fmt, tgt);
            double tfrac = (tgt - mn) / (mx - mn);
            int tx = bar_x0 + (int)(tfrac * (bar_x1 - bar_x0));
            fastchart_text_draw(t, font, size, pal.text,
                                tx, band_y1 + (int)(size * 3.0),
                                FASTCHART_ALIGN_CENTER, tgt_buf, NULL, 0);
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
