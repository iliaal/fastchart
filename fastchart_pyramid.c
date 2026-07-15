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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Population pyramid: two opposing series share a common set of category
 * rows (age bands, cohorts), one bar extending left of the central axis
 * and one extending right. Category labels sit in the centre gap. */

int fastchart_pyramid_render_to_target(fastchart_pyramid_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_paint_canvas_bg(t, (fastchart_obj *)self, &pal);

    if (self->cat_count <= 0 ||
        (self->left.n <= 0 && self->right.n <= 0)) {
        zend_throw_error(NULL,
            "FastChart\\PopulationPyramid::draw() requires setCategories() and "
            "at least one of setLeftSeries() / setRightSeries()");
        return -1;
    }

    int top_pad = 16;
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

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);

    /* Centre gap sized to the widest category label. */
    int center_gap = 48;
    if (font) {
        int widest = 0;
        for (int i = 0; i < self->cat_count; i++) {
            if (!self->categories[i]) continue;
            int w = 0;
            if (fastchart_text_measure(t, font, size, self->categories[i],
                                       &w, NULL, NULL, 0) == 0 && w > widest) {
                widest = w;
            }
        }
        center_gap = widest + 16;
        if (center_gap < 32) center_gap = 32;
    }

    int legend_h = (self->left.label || self->right.label) ? (int)(size + 12) : 0;
    int plot_x0 = 16, plot_x1 = W - 16;
    int plot_y0 = top_pad + legend_h + 6, plot_y1 = H - 16;
    fastchart_apply_plot_rect((fastchart_obj *)self,
                              &plot_x0, &plot_y0, &plot_x1, &plot_y1);
    double plot_w = plot_x1 - plot_x0;
    double plot_h = plot_y1 - plot_y0;
    /* A single very wide category label could push center_gap past the
     * plot width, tripping the guard below and blanking the whole chart.
     * Clamp it to leave at least 20px for the bars (10 per side) so a
     * long-but-valid label degrades to overlap, not an empty canvas. */
    if (center_gap > plot_w - 20) {
        center_gap = (int)plot_w - 20;
        if (center_gap < 32) center_gap = 32;
    }
    if (plot_w < center_gap + 20 || plot_h < 10) return 0;

    double plot_cx = (plot_x0 + plot_x1) / 2.0;
    double cxl = plot_cx - center_gap / 2.0;   /* inner edge of left bars */
    double cxr = plot_cx + center_gap / 2.0;    /* inner edge of right bars */
    double half_w = (plot_w - center_gap) / 2.0;
    if (half_w < 1.0) half_w = 1.0;

    /* Shared value scale across both sides. Only the first cat_count
     * entries of each side are ever drawn, so values beyond that must
     * not influence the scale (an oversized trailing value would
     * otherwise collapse every visible bar). */
    double max_val = 0.0;
    for (int i = 0; i < self->left.n && i < self->cat_count; i++) {
        double v = self->left.data[i];
        if (isfinite(v) && v > max_val) max_val = v;
    }
    for (int i = 0; i < self->right.n && i < self->cat_count; i++) {
        double v = self->right.data[i];
        if (isfinite(v) && v > max_val) max_val = v;
    }
    if (max_val <= 0.0) max_val = 1.0;

    int left_color = self->left.color_rgb >= 0
        ? fastchart_target_color_rgb(t, self->left.color_rgb)
        : pal.series[0];
    int right_color = self->right.color_rgb >= 0
        ? fastchart_target_color_rgb(t, self->right.color_rgb)
        : pal.series[1 % FASTCHART_PALETTE_SERIES_N];

    double row_h = plot_h / self->cat_count;
    double bar_h = row_h * 0.7;
    if (bar_h < 1.0) bar_h = 1.0;

    for (int i = 0; i < self->cat_count; i++) {
        double y0 = plot_y0 + i * row_h + (row_h - bar_h) / 2.0;
        if (i < self->left.n && isfinite(self->left.data[i]) && self->left.data[i] > 0) {
            double w = self->left.data[i] / max_val * half_w;
            fastchart_target_rect(t, (int)(cxl - w), (int)y0, (int)w, (int)bar_h,
                                  left_color, 1, 0);
            fastchart_target_rect(t, (int)(cxl - w), (int)y0, (int)w, (int)bar_h,
                                  pal.border, 0, 1);
        }
        if (i < self->right.n && isfinite(self->right.data[i]) && self->right.data[i] > 0) {
            double w = self->right.data[i] / max_val * half_w;
            fastchart_target_rect(t, (int)cxr, (int)y0, (int)w, (int)bar_h,
                                  right_color, 1, 0);
            fastchart_target_rect(t, (int)cxr, (int)y0, (int)w, (int)bar_h,
                                  pal.border, 0, 1);
        }
        if (font && self->categories[i]) {
            fastchart_text_draw(t, font, size, pal.text,
                                (int)plot_cx, (int)(plot_y0 + i * row_h + row_h / 2.0 + size * 0.35),
                                FASTCHART_ALIGN_CENTER, self->categories[i], NULL, 0);
        }
    }

    /* Legend: left swatch top-left, right swatch top-right. */
    if (font && legend_h > 0) {
        int ly = top_pad + 2;
        int sw = (int)size;
        if (self->left.label) {
            fastchart_target_rect(t, plot_x0, ly, sw, sw, left_color, 1, 0);
            fastchart_text_draw(t, font, size, pal.text,
                                plot_x0 + sw + 5, ly + (int)(size * 0.85),
                                FASTCHART_ALIGN_LEFT, self->left.label, NULL, 0);
        }
        if (self->right.label) {
            fastchart_target_rect(t, plot_x1 - sw, ly, sw, sw, right_color, 1, 0);
            fastchart_text_draw(t, font, size, pal.text,
                                plot_x1 - sw - 5, ly + (int)(size * 0.85),
                                FASTCHART_ALIGN_RIGHT, self->right.label, NULL, 0);
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
