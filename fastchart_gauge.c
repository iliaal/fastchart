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

int fastchart_gauge_render_to_image(fastchart_gauge_obj *self, gdImagePtr im)
{
    /* Per-render entry: invalidate the font cache (so a runtime
     * open_basedir narrowing between draws is honored) and stamp DPI
     * on the canvas. Must come BEFORE any palette / text work. */
    fastchart_begin_render((fastchart_obj *)self, im);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

    /* Reserve title height proportional to font size, not a hardcoded
     * 32px constant. At larger canvas + larger font (1200x800 with
     * setFontSize(22)), 32px clipped the ascender against the canvas
     * top. Measure the title's actual height; fall back to scaling
     * from the base font size if measurement fails. */
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    int title_baseline = 12;
    int top = 12;
    if (self->title && ZSTR_LEN(self->title) > 0) {
        const char *tfont = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
        int th = (int)(title_size * 1.0);  /* fallback */
        if (tfont) {
            int measured_h = 0;
            if (fastchart_text_measure(im, tfont, title_size, ZSTR_VAL(self->title),
                                       NULL, &measured_h, NULL, 0) == 0) {
                th = measured_h;
            }
        }
        title_baseline = th + 10;
        top = title_baseline + 12;
    }

    /* Compute the gauge geometry from the available area. The body is
     * a top-half semicircle of radius `radius`; the bottom-tick labels
     * (0% / 100%) sit just under the cy line and need ~size*1.5 below.
     * Vertically centre the body+labels block inside [top, H]. */
    int label_pad = (int)(base_size * 1.5) + 8;
    int avail_h = H - top - label_pad;
    int avail_w = W - 60;  /* 30px side padding for 0%/100% labels */
    int radius = (avail_w / 2 < avail_h ? avail_w / 2 : avail_h);
    if (radius < 40) radius = 40;
    int diameter = radius * 2;
    int cx = W / 2;
    /* Centre the (radius + label_pad) block vertically in [top, H]:
     *   block_top  = top + (avail_h_total - block_h) / 2
     *   cy         = block_top + radius
     *   avail_h_total = H - top, block_h = radius + label_pad
     */
    int block_h = radius + label_pad;
    int cy = top + ((H - top) - block_h) / 2 + radius;

    double mn = self->gauge_min;
    double mx = self->gauge_max;
    double v = self->gauge_value;

    /* Draw zones (or a single fill). libgd arc angles are clockwise
     * with 0° at 3-o'clock; a 180° arc covers 180° (left) to 360°
     * (right). Zones are pre-parsed into typed C state by setZones. */
    int default_color = pal.series[0];

    if (self->zones && self->n_zones > 0) {
        /* Background fill (a thin ring, drawn as a fat arc). */
        fastchart_filled_wedge_aa(im, cx, cy, diameter, 180, 360, pal.grid);
        for (int i = 0; i < self->n_zones; i++) {
            const fastchart_gauge_zone *zn = &self->zones[i];
            int color = default_color;
            if (zn->color_rgb >= 0) {
                int c = zn->color_rgb;
                color = gdImageColorAllocate(im,
                    (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
            }
            /* Map gauge values directly into libgd's arc-angle frame:
             *   value=min  -> 180° (left edge of upper half)
             *   value=max  -> 360° (right edge of upper half)
             * Increasing value sweeps CCW through the top. The old
             * code routed via gauge_value_to_deg [180..0] plus a
             * +=180 / %360 dance, which collapsed the leftmost zone
             * to a 1° sliver because value=0 wrapped 360 -> 0. */
            double frac_a = (zn->from - mn) / (mx - mn);
            double frac_b = (zn->to   - mn) / (mx - mn);
            if (frac_a < 0) frac_a = 0;
            if (frac_a > 1) frac_a = 1;
            if (frac_b < 0) frac_b = 0;
            if (frac_b > 1) frac_b = 1;
            int start = (int)(180 + frac_a * 180);
            int end   = (int)(180 + frac_b * 180);
            if (start > end) { int t = start; start = end; end = t; }
            if (end <= start) continue;  /* empty zone, skip */
            fastchart_filled_wedge_aa(im, cx, cy, diameter, start, end, color);
        }
    } else {
        /* Single-color sweep from min to value, with the rest in grid color. */
        fastchart_filled_wedge_aa(im, cx, cy, diameter, 180, 360, pal.grid);
        double aV = gauge_value_to_deg(v, mn, mx);
        int start = 180;
        int end = 180 + (int)(180 - aV);
        if (end > start) {
            fastchart_filled_wedge_aa(im, cx, cy, diameter, start, end, default_color);
        }
    }

    /* Inner cutout to make a thick ring. */
    int hole = (int)(diameter * 0.55);
    gdImageFilledEllipse(im, cx, cy, hole, hole, pal.bg);

    /* Outer arc edge in border color. */
    gdImageArc(im, cx, cy, diameter, diameter, 180, 360, pal.border);

    /* Needle. gauge_value_to_deg returns 180 (value=min) to 0 (value=max),
     * which is the standard math angle (CCW from +x axis): 180=left,
     * 90=up, 0=right. Screen y points down, so we negate the sine. */
    double aV = gauge_value_to_deg(v, mn, mx);
    double rad = aV * M_PI / 180.0;
    int nx = cx + (int)((double)(radius - 6) * cos(rad));
    int ny = cy - (int)((double)(radius - 6) * sin(rad));
    /* Needle thickness scales with gauge size — visible on a 1200x800
     * canvas, not dominant on a 480x320 one. libgd has no proper
     * thick-AA primitive, so paint the body thick (no AA) then
     * overdraw a 1px AA spine to soften diagonals. */
    double needle_thickness = (double)diameter / 200.0 + 2.0;
    if (needle_thickness < 3.0) needle_thickness = 3.0;
    gdImageSetThickness(im, (int)needle_thickness);
    gdImageLine(im, cx, cy, nx, ny, pal.text);
    gdImageSetThickness(im, 1);
    gdImageSetAntiAliased(im, pal.text);
    gdImageLine(im, cx, cy, nx, ny, gdAntiAliased);

    /* Hub: scale with diameter so it doesn't look tiny on a large canvas. */
    int hub = diameter / 60;
    if (hub < 8) hub = 8;
    gdImageFilledEllipse(im, cx, cy, hub, hub, pal.text);

    /* Center value label. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    if (font) {
        const char *fmt = self->gauge_value_format
            ? ZSTR_VAL(self->gauge_value_format) : "%.1f";
        char buf[64];
        snprintf(buf, sizeof(buf), fmt, v);
        double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base * 1.4);
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
        double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base * 0.85);
        fastchart_text_draw(im, font, size, pal.text,
                            cx - radius, cy + (int)(size * 1.5),
                            FASTCHART_ALIGN_CENTER, minbuf, NULL, 0);
        fastchart_text_draw(im, font, size, pal.text,
                            cx + radius, cy + (int)(size * 1.5),
                            FASTCHART_ALIGN_CENTER, maxbuf, NULL, 0);
    }

    /* Title. Baseline scales with the title font size so the ascender
     * stays inside the canvas at any canvas/font scale. */
    fastchart_draw_floating_title(im, (fastchart_obj *)self, &pal, W / 2, title_baseline);

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_GaugeChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\GaugeChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_gauge_obj *self = Z_FASTCHART_GAUGE_OBJ_P(ZEND_THIS);
    if (fastchart_gauge_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
