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
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

int fastchart_funnel_render_to_image(fastchart_funnel_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    fastchart_begin_render((fastchart_obj *)self, &t);

    fastchart_palette pal;
    fastchart_palette_init(&t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(&t, (fastchart_obj *)self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, fastchart_target_color_to_gd(&t, pal.bg));

    /* Title reservation. */
    int top_pad = 12;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(&t, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top_pad += title_h + 10;
        }
    }

    int n = self->stage_count;
    if (n <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Funnel::draw() requires setStages() with at least one stage");
        return -1;
    }

    /* Plot region: full width minus side padding for stage labels,
     * vertical span from below the title to the canvas bottom minus
     * a small footer. */
    int side_pad = 100;     /* room for label text on either side */
    int x_left = side_pad;
    int x_right = W - side_pad;
    int y0 = top_pad;
    int y1 = H - 12;
    if (x_right <= x_left || y1 <= y0) {
        zend_throw_error(NULL,
            "FastChart\\Funnel::draw() canvas is too narrow for label margins");
        return -1;
    }

    /* Each stage is a horizontal trapezoid centred on the canvas;
     * width is proportional to its value relative to the largest
     * stage. Stage heights are equal — the value contrast lives in
     * the width, matching ChartDirector / CD-style funnels. */
    double max_v = 0;
    for (int i = 0; i < n; i++) {
        if (self->stages[i].value > max_v) max_v = self->stages[i].value;
    }
    if (max_v <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Funnel::draw() requires at least one stage with value > 0");
        return -1;
    }

    int total_h = y1 - y0;
    int stage_h = total_h / n;
    if (stage_h < 4) stage_h = 4;
    int cx = (x_left + x_right) / 2;
    int max_half = (x_right - x_left) / 2;

    const char *label_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double label_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base_size);

    for (int i = 0; i < n; i++) {
        double v_top = self->stages[i].value;
        double v_bot = (i + 1 < n) ? self->stages[i + 1].value : v_top * 0.6;
        if (v_bot < 0) v_bot = 0;
        int half_top = (int)(max_half * v_top / max_v + 0.5);
        int half_bot = (int)(max_half * v_bot / max_v + 0.5);
        int yt = y0 + i * stage_h;
        int yb = yt + stage_h - 2;

        int color;
        if (self->stages[i].color_rgb >= 0) {
            int rgb = self->stages[i].color_rgb;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        } else {
            color = pal.series[i % FASTCHART_PALETTE_SERIES_N];
        }

        gdPoint trap[4] = {
            { cx - half_top, yt },
            { cx + half_top, yt },
            { cx + half_bot, yb },
            { cx - half_bot, yb },
        };
        gdImageFilledPolygon(im, trap, 4, color);
        gdImagePolygon(im, trap, 4, fastchart_target_color_to_gd(&t, pal.border));

        /* Label: stage name on the left, optional value on the right.
         * Anchor at the trapezoid centroid Y. */
        int yc = (yt + yb) / 2 + (int)(label_size * 0.4);
        if (label_font && self->stages[i].label) {
            fastchart_text_draw(&t, label_font, label_size, pal.text,
                                x_left - 8, yc, FASTCHART_ALIGN_RIGHT,
                                self->stages[i].label, NULL, 0);
        }
        if (label_font && ((fastchart_obj *)self)->show_values) {
            /* Honour the inherited setShowValues($flag, $format)
             * format string when present; fall back to "%.0f" for
             * the unconfigured case. */
            const char *fmt = "%.0f";
            fastchart_obj *base = (fastchart_obj *)self;
            if (base->value_format && ZSTR_LEN(base->value_format) > 0) {
                fmt = ZSTR_VAL(base->value_format);
            }
            char buf[64];
            snprintf(buf, sizeof(buf), fmt, v_top);
            fastchart_text_draw(&t, label_font, label_size, pal.text,
                                x_right + 8, yc, FASTCHART_ALIGN_LEFT,
                                buf, NULL, 0);
        }
    }

    /* Title. */
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(&t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(&t, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_Funnel, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\Funnel::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_funnel_obj *self = Z_FASTCHART_FUNNEL_OBJ_P(ZEND_THIS);
    if (fastchart_funnel_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
