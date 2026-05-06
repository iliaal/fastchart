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

#include <stdio.h>
#include <string.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

int fastchart_linear_meter_render_to_image(fastchart_linear_meter_obj *self, gdImagePtr im)
{
    fastchart_begin_render((fastchart_obj *)self, im);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

    /* Title reservation (optional). */
    int top_pad = 12;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(im, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top_pad += title_h + 10;
        }
    }

    double mn = self->meter_min;
    double mx = self->meter_max;
    double v = self->meter_value;
    if (mx <= mn) mx = mn + 1.0;
    if (v < mn) v = mn;
    if (v > mx) v = mx;

    /* Layout: orientation determines the long axis. The bar runs
     * from min->max along the long axis, with zones painted into
     * matching ranges. A small padding keeps the bar visually inset
     * from the canvas edges. */
    int bar_x0, bar_y0, bar_x1, bar_y1;
    int label_size_min = (int)(base_size * 1.2);
    if (self->meter_orientation == FASTCHART_METER_HORIZONTAL) {
        int margin_x = 60;     /* room for min/max value labels */
        int margin_y = label_size_min * 2;
        bar_x0 = margin_x;
        bar_x1 = W - margin_x;
        bar_y0 = top_pad + (H - top_pad - margin_y * 2) / 3;
        bar_y1 = bar_y0 + (H - top_pad - margin_y * 2) / 3;
        if (bar_y1 - bar_y0 < 12) bar_y1 = bar_y0 + 12;
    } else {
        int margin_x = label_size_min * 4;
        int margin_y = 30;
        bar_y0 = top_pad + margin_y;
        bar_y1 = H - margin_y;
        bar_x0 = (W - margin_x) / 2 - 12;
        bar_x1 = bar_x0 + 24;
    }

    /* Background fill (the inactive part of the bar). */
    gdImageFilledRectangle(im, bar_x0, bar_y0, bar_x1, bar_y1, pal.grid);

    /* Zone fills along the bar long axis. */
    for (int i = 0; i < self->n_zones; i++) {
        const fastchart_gauge_zone *zn = &self->zones[i];
        double t0 = (zn->from - mn) / (mx - mn);
        double t1 = (zn->to   - mn) / (mx - mn);
        if (t0 < 0) t0 = 0;
        if (t0 > 1) t0 = 1;
        if (t1 < 0) t1 = 0;
        if (t1 > 1) t1 = 1;
        if (t1 <= t0) continue;
        int color;
        if (zn->color_rgb >= 0) {
            int rgb = zn->color_rgb;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        } else {
            color = pal.series[i % FASTCHART_PALETTE_SERIES_N];
        }

        if (self->meter_orientation == FASTCHART_METER_HORIZONTAL) {
            int zx0 = bar_x0 + (int)(t0 * (bar_x1 - bar_x0));
            int zx1 = bar_x0 + (int)(t1 * (bar_x1 - bar_x0));
            gdImageFilledRectangle(im, zx0, bar_y0, zx1, bar_y1, color);
        } else {
            /* Y inverts: t=0 sits at the bottom (mn), t=1 at the top. */
            int zy1 = bar_y1 - (int)(t0 * (bar_y1 - bar_y0));
            int zy0 = bar_y1 - (int)(t1 * (bar_y1 - bar_y0));
            gdImageFilledRectangle(im, bar_x0, zy0, bar_x1, zy1, color);
        }
    }
    gdImageRectangle(im, bar_x0, bar_y0, bar_x1, bar_y1, pal.border);

    /* Pointer: a thick line + filled triangle at the value. */
    double t = (v - mn) / (mx - mn);
    if (self->meter_orientation == FASTCHART_METER_HORIZONTAL) {
        int px = bar_x0 + (int)(t * (bar_x1 - bar_x0));
        gdImageSetThickness(im, 2);
        gdImageLine(im, px, bar_y0 - 4, px, bar_y1 + 4, pal.text);
        gdImageSetThickness(im, 1);
        gdPoint tri[3] = {
            { px - 6, bar_y0 - 10 },
            { px + 6, bar_y0 - 10 },
            { px,     bar_y0 - 2  },
        };
        gdImageFilledPolygon(im, tri, 3, pal.text);
    } else {
        int py = bar_y1 - (int)(t * (bar_y1 - bar_y0));
        gdImageSetThickness(im, 2);
        gdImageLine(im, bar_x0 - 4, py, bar_x1 + 4, py, pal.text);
        gdImageSetThickness(im, 1);
        gdPoint tri[3] = {
            { bar_x1 + 10, py - 6 },
            { bar_x1 + 10, py + 6 },
            { bar_x1 + 2,  py     },
        };
        gdImageFilledPolygon(im, tri, 3, pal.text);
    }

    /* Min / max / value labels. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base_size);
    if (font) {
        char min_buf[32], max_buf[32], val_buf[64];
        const char *fmt = self->meter_value_format
            ? ZSTR_VAL(self->meter_value_format) : "%.0f";
        snprintf(min_buf, sizeof(min_buf), fmt, mn);
        snprintf(max_buf, sizeof(max_buf), fmt, mx);
        snprintf(val_buf, sizeof(val_buf), fmt, v);

        if (self->meter_orientation == FASTCHART_METER_HORIZONTAL) {
            int label_y = bar_y1 + (int)(size * 1.5);
            fastchart_text_draw(im, font, size, pal.text,
                                bar_x0, label_y, FASTCHART_ALIGN_LEFT,
                                min_buf, NULL, 0);
            fastchart_text_draw(im, font, size, pal.text,
                                bar_x1, label_y, FASTCHART_ALIGN_RIGHT,
                                max_buf, NULL, 0);
            int px = bar_x0 + (int)(t * (bar_x1 - bar_x0));
            fastchart_text_draw(im, font, size * 1.1, pal.text,
                                px, bar_y0 - 16, FASTCHART_ALIGN_CENTER,
                                val_buf, NULL, 0);
        } else {
            int label_x = bar_x0 - 8;
            fastchart_text_draw(im, font, size, pal.text,
                                label_x, bar_y1 + (int)(size * 0.4),
                                FASTCHART_ALIGN_RIGHT, min_buf, NULL, 0);
            fastchart_text_draw(im, font, size, pal.text,
                                label_x, bar_y0 + (int)(size * 0.4),
                                FASTCHART_ALIGN_RIGHT, max_buf, NULL, 0);
            int py = bar_y1 - (int)(t * (bar_y1 - bar_y0));
            fastchart_text_draw(im, font, size * 1.1, pal.text,
                                bar_x1 + 30, py + (int)(size * 0.4),
                                FASTCHART_ALIGN_LEFT, val_buf, NULL, 0);
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(im, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_LinearMeter, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\LinearMeter::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_linear_meter_obj *self = Z_FASTCHART_LINEAR_METER_OBJ_P(ZEND_THIS);
    if (fastchart_linear_meter_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
