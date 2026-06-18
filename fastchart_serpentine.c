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

/* Serpentine timeline: events are laid out in reading rows that reverse
 * direction each line (boustrophedon), so the connecting path snakes
 * back and forth and a long sequence fits a compact rectangle. The snake
 * geometry falls out of the alternating per-row x assignment — connecting
 * consecutive event centres in order yields the U-turns for free. */

int fastchart_serpentine_render_to_target(fastchart_serpentine_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->event_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\SerpentineTimeline::draw() requires setEvents()");
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

    int n = self->event_count;
    int plot_x0 = 24, plot_x1 = W - 24;
    int plot_y0 = top_pad + 12, plot_y1 = H - 16;
    double plot_w = plot_x1 - plot_x0;
    double plot_h = plot_y1 - plot_y0;
    if (plot_w < 20 || plot_h < 20) return 0;

    int per_row = (int)self->per_row;
    if (per_row <= 0) {
        /* Aim for roughly square cells. */
        double guess = round(sqrt((double)n * plot_w / plot_h));
        per_row = (int)guess;
        if (per_row < 1) per_row = 1;
        if (per_row > n) per_row = n;
    }
    if (per_row > n) per_row = n;
    int rows = (n + per_row - 1) / per_row;

    double step_x = plot_w / per_row;
    double row_h = plot_h / rows;

    double *ex = ecalloc(n, sizeof(double));
    double *ey = ecalloc(n, sizeof(double));
    for (int i = 0; i < n; i++) {
        int row = i / per_row;
        int col = i % per_row;
        int c = (row & 1) ? (per_row - 1 - col) : col;   /* reverse odd rows */
        ex[i] = plot_x0 + (c + 0.5) * step_x;
        ey[i] = plot_y0 + (row + 0.5) * row_h;
    }

    /* Connecting path (behind markers). */
    for (int i = 0; i + 1 < n; i++) {
        fastchart_target_line(t, (int)ex[i], (int)ey[i],
                              (int)ex[i + 1], (int)ey[i + 1],
                              pal.grid, 3, FASTCHART_DASH_SOLID);
    }

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    int r = 9;

    for (int i = 0; i < n; i++) {
        int color = self->events[i].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->events[i].color_rgb)
            : pal.series[i % FASTCHART_PALETTE_SERIES_N];
        fastchart_target_ellipse(t, (int)ex[i], (int)ey[i], r, r, color, 1, 0);
        fastchart_target_ellipse(t, (int)ex[i], (int)ey[i], r, r, pal.border, 0, 1);

        if (font) {
            /* Date above the marker, label below — keeps both clear of
             * the connecting path that runs through the marker centre. */
            if (self->events[i].date) {
                fastchart_text_draw(t, font, size * 0.85, pal.text,
                                    (int)ex[i], (int)(ey[i] - r - 4),
                                    FASTCHART_ALIGN_CENTER,
                                    self->events[i].date, NULL, 0);
            }
            if (self->events[i].label) {
                fastchart_text_draw(t, font, size, pal.text,
                                    (int)ex[i], (int)(ey[i] + r + (int)size),
                                    FASTCHART_ALIGN_CENTER,
                                    self->events[i].label, NULL, 0);
            }
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    efree(ex);
    efree(ey);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
