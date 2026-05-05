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

#define MAX_RADAR_AXES   32

/* Read a typed-series cell, treating out-of-range as 0. */
static inline double radar_read_d(const fastchart_radar_series *s, int i)
{
    if (i >= s->len) return 0.0;
    double d = s->values[i];
    return (d < 0 || isnan(d)) ? 0.0 : d;
}

int fastchart_radar_render_to_image(fastchart_radar_obj *self, gdImagePtr im)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\RadarChart::draw() requires setSeries() with at least 3 axes per series");
        return -1;
    }
    fastchart_radar_series *series = self->series;
    int n_series = self->n_series;

    /* n_axes = max series len. Each axis is one entry index. */
    int n_axes = 0;
    for (int s = 0; s < n_series; s++) {
        if (series[s].len > n_axes) n_axes = series[s].len;
    }
    if (n_axes > MAX_RADAR_AXES) n_axes = MAX_RADAR_AXES;
    if (n_axes < 3) {
        zend_throw_error(NULL,
            "FastChart\\RadarChart::draw() requires setSeries() with at least 3 axes per series");
        return -1;
    }

    double dmax = 0.0;
    for (int s = 0; s < n_series; s++) {
        for (int i = 0; i < n_axes; i++) {
            double v = radar_read_d(&series[s], i);
            if (v > dmax) dmax = v;
        }
    }
    if (self->radar_max > 0) dmax = self->radar_max;
    if (dmax <= 0) dmax = 1.0;

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    /* Stamp DPI on the canvas — feeds gdImage's resolution
     * metadata + FreeType hinting via fastchart_text_draw's
     * gdImageStringFTEx call. Renderers that go through
     * fastchart_compute_layout already get this; this one
     * does not, so the call is local. */
    if (((fastchart_obj *)self)->dpi > 0) {
        gdImageSetResolution(im, (unsigned int)((fastchart_obj *)self)->dpi,
                              (unsigned int)((fastchart_obj *)self)->dpi);
    }

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

    /* Reserve top space for title. */
    int title_h = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 8;
    int cx = W / 2;
    int cy = (title_h + H) / 2;
    int radius = (W < H ? W : H) / 2 - 60;
    if (radius < 40) radius = 40;

    /* Pre-compute the per-axis sin/cos table once. The grid loop,
     * spoke loop, label loop, and series polygon loop all share
     * these N angles, so without the cache each axis paid for 4
     * cos+sin evaluations per render. */
    double cos_a[MAX_RADAR_AXES];
    double sin_a[MAX_RADAR_AXES];
    for (int i = 0; i < n_axes; i++) {
        double angle = -M_PI / 2.0 + 2 * M_PI * i / n_axes;
        cos_a[i] = cos(angle);
        sin_a[i] = sin(angle);
    }

    /* Concentric grid: 4 rings + axis spokes. */
    const int rings = 4;
    for (int r = 1; r <= rings; r++) {
        double rr = (double)radius * (double)r / (double)rings;
        gdPoint poly[MAX_RADAR_AXES];
        for (int i = 0; i < n_axes; i++) {
            poly[i].x = cx + (int)(rr * cos_a[i]);
            poly[i].y = cy + (int)(rr * sin_a[i]);
        }
        gdImagePolygon(im, poly, n_axes, pal.grid);
    }
    /* Spokes from center to each axis tip. */
    for (int i = 0; i < n_axes; i++) {
        int tx = cx + (int)(radius * cos_a[i]);
        int ty = cy + (int)(radius * sin_a[i]);
        gdImageLine(im, cx, cy, tx, ty, pal.axis);
    }

    /* Axis labels via setCategoryLabels. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base);
    fastchart_obj *chart_base = (fastchart_obj *)self;
    if (font && chart_base->category_labels) {
        for (int i = 0; i < n_axes; i++) {
            const char *label = (i < chart_base->n_category_labels)
                ? chart_base->category_labels[i] : NULL;
            if (!label) continue;
            int lx = cx + (int)((radius + 16) * cos_a[i]);
            int ly = cy + (int)((radius + 16) * sin_a[i]);
            fastchart_align align;
            if (cos_a[i] > 0.3) align = FASTCHART_ALIGN_LEFT;
            else if (cos_a[i] < -0.3) align = FASTCHART_ALIGN_RIGHT;
            else align = FASTCHART_ALIGN_CENTER;
            fastchart_text_draw(im, font, size, pal.text,
                                lx, ly + (int)(size * 0.35),
                                align, label, NULL, 0);
        }
    }

    /* One polygon per series. */
    int legend_colors[FASTCHART_MAX_RADAR_SERIES];
    const char *legend_labels[FASTCHART_MAX_RADAR_SERIES];
    int legend_count = 0;

    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        if (series[s].color_rgb >= 0) {
            int rgb = (int)series[s].color_rgb;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }
        gdPoint poly[MAX_RADAR_AXES];
        for (int i = 0; i < n_axes; i++) {
            double v = radar_read_d(&series[s], i);
            double rr = (double)radius * v / dmax;
            poly[i].x = cx + (int)(rr * cos_a[i]);
            poly[i].y = cy + (int)(rr * sin_a[i]);
        }
        if (self->radar_filled) {
            int r = (color >> 0); /* gd palette index, not raw RGB */
            (void)r;
            int rr = gdImageRed(im, color);
            int gg = gdImageGreen(im, color);
            int bb = gdImageBlue(im, color);
            int alpha = gdImageColorAllocateAlpha(im, rr, gg, bb, 90);
            gdImageAlphaBlending(im, 1);
            fastchart_shadow_filled_polygon(im, (fastchart_obj *)self, poly, n_axes);
            fastchart_filled_polygon_aa(im, poly, n_axes, alpha);
            gdImageAlphaBlending(im, 0);
        }
        gdImageSetThickness(im, 2);
        gdImagePolygon(im, poly, n_axes, color);
        gdImageSetThickness(im, 1);
        /* Markers at vertices. */
        for (int i = 0; i < n_axes; i++) {
            fastchart_draw_marker(im, poly[i].x, poly[i].y,
                                  FASTCHART_MARKER_CIRCLE, 5, color);
        }
        if (series[s].label && legend_count < FASTCHART_MAX_RADAR_SERIES) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Title at top center. */
    fastchart_draw_floating_title(im, (fastchart_obj *)self, &pal, W / 2, 24);

    /* Legend: reuse the standard helper with a synthetic "plot rect"
     * spanning the entire canvas so positioning works. */
    if (legend_count > 0) {
        fastchart_rect plot = { 10, title_h, W - 10, H - 10 };
        fastchart_draw_legend(im, (fastchart_obj *)self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_RadarChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\RadarChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_radar_obj *self = Z_FASTCHART_RADAR_OBJ_P(ZEND_THIS);
    if (fastchart_radar_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
