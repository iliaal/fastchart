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
#include "fastchart_axis.h"
#include "fastchart_text.h"
#include "fastchart_effects.h"

#include <math.h>

/* Map a gauge value v in [min, max] to an angle in degrees on a
 * 180° arc (180° = left/min, 0° = right/max in libgd's clockwise
 * coordinate system). */
static double gauge_value_to_deg(double v, double mn, double mx)
{
    if (v < mn) v = mn;
    if (v > mx) v = mx;
    double frac = (mx > mn) ? (v - mn) / (mx - mn) : 0.0;
    return 180.0 - frac * 180.0;
}

int fastchart_gauge_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

    int top = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 12;
    int cx = W / 2;
    int cy = top + (H - top) * 3 / 4;
    int radius = (W < (H - top) * 2 ? W : (H - top) * 2) / 2 - 30;
    if (radius < 40) radius = 40;
    int diameter = radius * 2;

    double mn = self->gauge_min;
    double mx = self->gauge_max;
    double v = self->gauge_value;

    /* Draw zones (or a single fill). libgd arc angles are clockwise
     * with 0° at 3-o'clock; a 180° arc covers 180° (left) to 360°
     * (right). */
    zval *zones_zv = zend_hash_str_find(Z_ARRVAL(self->config),
        "gauge_zones", sizeof("gauge_zones") - 1);
    int default_color = pal.series[0];

    if (zones_zv && Z_TYPE_P(zones_zv) == IS_ARRAY &&
        zend_hash_num_elements(Z_ARRVAL_P(zones_zv)) > 0) {
        /* Background fill (a thin ring, drawn as a fat arc). */
        gdImageFilledArc(im, cx, cy, diameter, diameter, 180, 360,
                         pal.grid, gdPie);
        zval *z;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zones_zv), z) {
            if (Z_TYPE_P(z) != IS_ARRAY) continue;
            zval *zf = zend_hash_str_find(Z_ARRVAL_P(z), "from", sizeof("from") - 1);
            zval *zt = zend_hash_str_find(Z_ARRVAL_P(z), "to",   sizeof("to") - 1);
            zval *zc = zend_hash_str_find(Z_ARRVAL_P(z), "color", sizeof("color") - 1);
            double f, t;
            if (!zf || !zt || fastchart_zval_to_double(zf, &f) != 0 ||
                fastchart_zval_to_double(zt, &t) != 0) continue;
            int color = default_color;
            if (zc && Z_TYPE_P(zc) == IS_LONG &&
                Z_LVAL_P(zc) >= 0 && Z_LVAL_P(zc) <= 0xFFFFFF) {
                long c = Z_LVAL_P(zc);
                color = gdImageColorAllocate(im,
                    (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
            }
            /* Convert from gauge values to libgd arc angles. The
             * "from" angle is the higher numeric (left side of arc);
             * libgd starts the arc at the smaller angle and sweeps
             * clockwise to the larger one. */
            double a1 = gauge_value_to_deg(f, mn, mx);
            double a2 = gauge_value_to_deg(t, mn, mx);
            int start = (int)(a2 < a1 ? a2 : a1);
            int end   = (int)(a2 < a1 ? a1 : a2);
            /* Map degrees in our 180..0 frame to libgd's 180..360 frame. */
            start += 180; end += 180;
            if (start >= 360) start -= 360;
            if (end >= 360) end -= 360;
            if (end <= start) end = start + 1;
            gdImageFilledArc(im, cx, cy, diameter, diameter,
                             start, end, color, gdPie);
        } ZEND_HASH_FOREACH_END();
    } else {
        /* Single-color sweep from min to value, with the rest in grid color. */
        gdImageFilledArc(im, cx, cy, diameter, diameter, 180, 360,
                         pal.grid, gdPie);
        double aV = gauge_value_to_deg(v, mn, mx);
        int start = 180;
        int end = 180 + (int)(180 - aV);
        if (end > start) {
            gdImageFilledArc(im, cx, cy, diameter, diameter,
                             start, end, default_color, gdPie);
        }
    }

    /* Inner cutout to make a thick ring. */
    int hole = (int)(diameter * 0.55);
    gdImageFilledEllipse(im, cx, cy, hole, hole, pal.bg);

    /* Outer arc edge in border color. */
    gdImageArc(im, cx, cy, diameter, diameter, 180, 360, pal.border);

    /* Needle. */
    double aV = gauge_value_to_deg(v, mn, mx);
    double rad = aV * M_PI / 180.0;
    int nx = cx - (int)((double)(radius - 6) * cos(rad));
    int ny = cy - (int)((double)(radius - 6) * sin(rad));
    gdImageSetThickness(im, 3);
    gdImageLine(im, cx, cy, nx, ny, pal.text);
    gdImageSetThickness(im, 1);
    gdImageFilledEllipse(im, cx, cy, 12, 12, pal.text);

    /* Center value label. */
    const char *font = fastchart_resolve_font(self, FC_FONT_LABEL);
    if (font) {
        const char *fmt = self->gauge_value_format
            ? ZSTR_VAL(self->gauge_value_format) : "%.1f";
        char buf[64];
        snprintf(buf, sizeof(buf), fmt, v);
        double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        double size = fastchart_resolve_font_size(self, FC_FONT_LABEL, base * 1.4);
        int tx = cx;
        int ty = cy + (int)(diameter * 0.35);
        fastchart_text_draw(im, font, size, pal.text, tx, ty,
                            FASTCHART_ALIGN_CENTER, buf, NULL, 0);
    }

    /* Min/max tick labels at arc ends. */
    if (font) {
        char minbuf[32], maxbuf[32];
        const char *fmt = self->gauge_value_format
            ? ZSTR_VAL(self->gauge_value_format) : "%.0f";
        snprintf(minbuf, sizeof(minbuf), fmt, mn);
        snprintf(maxbuf, sizeof(maxbuf), fmt, mx);
        double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        double size = fastchart_resolve_font_size(self, FC_FONT_LABEL, base * 0.85);
        fastchart_text_draw(im, font, size, pal.text,
                            cx - radius, cy + (int)(size * 1.5),
                            FASTCHART_ALIGN_CENTER, minbuf, NULL, 0);
        fastchart_text_draw(im, font, size, pal.text,
                            cx + radius, cy + (int)(size * 1.5),
                            FASTCHART_ALIGN_CENTER, maxbuf, NULL, 0);
    }

    /* Title. */
    fastchart_draw_floating_title(im, self, &pal, W / 2, 24);

    fastchart_draw_text_annotations(im, self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_GaugeChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) RETURN_THROWS();
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_gauge_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
