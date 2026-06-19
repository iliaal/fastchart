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

/* Pictogram / pictorial fraction: a grid of unit icons where a value's
 * share of a total is shown by filling that fraction of the icons left
 * to right. The boundary icon is partially filled via a clip rect so
 * fractional values read precisely. */

static void picto_draw_icon(fastchart_target_t *t, double x, double y,
                            double w, double h, zend_long shape, int color)
{
    double cx = x + w / 2.0;
    switch (shape) {
        case FASTCHART_PICTO_SHAPE_CIRCLE:
            fastchart_target_ellipse(t, (int)cx, (int)(y + h / 2.0),
                                     (int)(w / 2.0), (int)(h / 2.0), color, 1, 0);
            break;
        case FASTCHART_PICTO_SHAPE_PERSON: {
            double hr = w * 0.18;
            fastchart_target_ellipse(t, (int)cx, (int)(y + h * 0.22),
                                     (int)hr, (int)hr, color, 1, 0);
            fastchart_point_t body[4] = {
                { (int)(cx - w * 0.22), (int)(y + h * 0.42) },
                { (int)(cx + w * 0.22), (int)(y + h * 0.42) },
                { (int)(cx + w * 0.30), (int)(y + h * 0.95) },
                { (int)(cx - w * 0.30), (int)(y + h * 0.95) },
            };
            fastchart_target_polygon(t, body, 4, color, 1, 0);
            break;
        }
        case FASTCHART_PICTO_SHAPE_SQUARE:
        default:
            fastchart_target_rect(t, (int)x, (int)y, (int)w, (int)h, color, 1, 0);
            break;
    }
}

int fastchart_pictogram_render_to_target(fastchart_pictogram_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->total <= 0.0) {
        zend_throw_error(NULL,
            "FastChart\\Pictogram::draw() requires setTotal() with a positive total");
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

    int count = (int)self->icon_count;
    if (count < 1) count = 1;
    double frac = self->value / self->total;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;
    double filled_units = frac * count;
    int full = (int)floor(filled_units);
    double partial = filled_units - full;

    int cols = (int)self->columns;
    if (cols <= 0) cols = count < 10 ? count : 10;
    if (cols > count) cols = count;
    int rows = (count + cols - 1) / cols;

    int plot_x0 = 16, plot_x1 = W - 16;
    int plot_y0 = top_pad + 8, plot_y1 = H - 16;
    double cell_w = (double)(plot_x1 - plot_x0) / cols;
    double cell_h = (double)(plot_y1 - plot_y0) / rows;
    if (cell_w < 2.0 || cell_h < 2.0) return 0;
    double icon = (cell_w < cell_h ? cell_w : cell_h) * 0.8;
    double pad_x = (cell_w - icon) / 2.0;
    double pad_y = (cell_h - icon) / 2.0;

    int fill_color = self->fill_color_rgb >= 0
        ? fastchart_target_color_rgb(t, self->fill_color_rgb)
        : pal.series[0];
    int empty_color = self->empty_color_rgb >= 0
        ? fastchart_target_color_rgb(t, self->empty_color_rgb)
        : fastchart_target_color(t, 220, 220, 220, 255);

    for (int i = 0; i < count; i++) {
        int r = i / cols, c = i % cols;
        double x = plot_x0 + c * cell_w + pad_x;
        double y = plot_y0 + r * cell_h + pad_y;

        if (i < full) {
            /* Fully filled: draw directly, no clip needed. */
            picto_draw_icon(t, x, y, icon, icon, self->shape, fill_color);
        } else {
            picto_draw_icon(t, x, y, icon, icon, self->shape, empty_color);
            /* Only the boundary icon needs a fractional clip. */
            if (i == full && partial > 0.0) {
                double fw = icon * partial;
                if (fw > 0.5) {
                    fastchart_target_clip_push(t, (int)x, (int)y,
                                               (int)ceil(fw), (int)icon);
                    picto_draw_icon(t, x, y, icon, icon, self->shape, fill_color);
                    fastchart_target_clip_pop(t);
                }
            }
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
