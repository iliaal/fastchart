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

/* Marimekko: column widths proportional to per-column total,
 * segments stacked top-to-bottom within each column proportional
 * to component values. Each cell area encodes share-of-grand-total
 * directly. */
int fastchart_marimekko_render_to_target(fastchart_marimekko_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->column_count <= 0 || self->total_width <= 0.0) {
        zend_throw_error(NULL,
            "FastChart\\MarimekkoChart::draw() requires setColumns() with at least one positive segment");
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

    int plot_x0 = 20, plot_x1 = W - 20;
    int plot_y0 = top_pad + 4;
    int plot_y1 = H - 32;        /* room for column labels below */
    int avail_w = plot_x1 - plot_x0;
    int avail_h = plot_y1 - plot_y0;
    if (avail_w < 32 || avail_h < 32) {
        zend_throw_error(NULL,
            "FastChart\\MarimekkoChart::draw() canvas is too small for margins");
        return -1;
    }

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);

    /* Walk columns. Use double accumulators to spread rounding evenly. */
    double cx_acc = 0.0;
    /* Color palette is per-segment-position so the legend reads
     * consistently across columns (segment 0 = same color in every
     * column). */
    for (int c = 0; c < self->column_count; c++) {
        const fastchart_marimekko_column *col = &self->columns[c];
        double col_frac = col->total / self->total_width;
        double col_w = col_frac * avail_w;
        int x0 = plot_x0 + (int)(cx_acc + 0.5);
        cx_acc += col_w;
        int x1 = plot_x0 + (int)(cx_acc + 0.5);
        int cw = x1 - x0;
        if (cw < 1) cw = 1;
        /* Tiny visual gap between columns. */
        int gap = cw > 8 ? 1 : 0;
        x0 += gap; cw -= 2 * gap;
        if (cw < 1) cw = 1;

        /* Stack segments. */
        double sy_acc = 0.0;
        for (int s = 0; s < col->n_segments; s++) {
            const fastchart_marimekko_segment *seg = &col->segments[s];
            double seg_frac = seg->value / col->total;
            double seg_h = seg_frac * avail_h;
            int y0 = plot_y0 + (int)(sy_acc + 0.5);
            sy_acc += seg_h;
            int y1 = plot_y0 + (int)(sy_acc + 0.5);
            int sh = y1 - y0;
            if (sh < 1) sh = 1;
            int color = seg->color_rgb >= 0
                ? fastchart_target_color_rgb(t, seg->color_rgb)
                : pal.series[s % FASTCHART_PALETTE_SERIES_N];
            fastchart_target_rect(t, x0, y0, cw, sh, color, 1, 0);
            fastchart_target_rect(t, x0, y0, cw, sh, pal.border, 0, 1);

            /* Label inside segment if it fits comfortably. */
            if (font && seg->label && sh > size * 1.6 && cw > size * 4) {
                fastchart_text_draw(t, font, size, pal.text,
                                    x0 + cw / 2, y0 + sh / 2 + (int)(size * 0.4),
                                    FASTCHART_ALIGN_CENTER,
                                    seg->label, NULL, 0);
            }
        }

        /* Column label below the column. */
        if (font && col->label) {
            fastchart_text_draw(t, font, size, pal.text,
                                x0 + cw / 2, plot_y1 + (int)(size * 1.4),
                                FASTCHART_ALIGN_CENTER, col->label, NULL, 0);
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
