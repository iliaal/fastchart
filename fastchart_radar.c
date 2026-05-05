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

#define MAX_RADAR_SERIES 8
#define MAX_RADAR_AXES   32

typedef struct {
    HashTable *data;
    const char *label;
    zend_long color_override;  /* -1 = palette */
} radar_series_t;

static int collect_radar_series(zval *data_zv,
                                radar_series_t *out, int max_series,
                                int *out_count, int *out_n_axes)
{
    *out_count = 0;
    *out_n_axes = 0;
    if (Z_TYPE_P(data_zv) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(data_zv);
    if (zend_hash_num_elements(ht) == 0) return -1;

    zval *first = zend_hash_index_find(ht, 0);
    int is_multi = 0;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *data_key = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (data_key && Z_TYPE_P(data_key) == IS_ARRAY) is_multi = 1;
    }

    if (is_multi) {
        zval *s_zv;
        ZEND_HASH_FOREACH_VAL(ht, s_zv) {
            if (Z_TYPE_P(s_zv) != IS_ARRAY) continue;
            zval *d = zend_hash_str_find(Z_ARRVAL_P(s_zv), "data", sizeof("data") - 1);
            if (!d || Z_TYPE_P(d) != IS_ARRAY) continue;
            if (*out_count >= max_series) break;
            out[*out_count].data = Z_ARRVAL_P(d);
            zval *l = zend_hash_str_find(Z_ARRVAL_P(s_zv), "label", sizeof("label") - 1);
            out[*out_count].label = fastchart_label_or_null(l);
            zval *c = zend_hash_str_find(Z_ARRVAL_P(s_zv), "color", sizeof("color") - 1);
            out[*out_count].color_override = (c && Z_TYPE_P(c) == IS_LONG &&
                Z_LVAL_P(c) >= 0 && Z_LVAL_P(c) <= 0xFFFFFF) ? Z_LVAL_P(c) : -1;
            int n = (int)zend_hash_num_elements(out[*out_count].data);
            if (n > *out_n_axes) *out_n_axes = n;
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        out[0].data = ht;
        out[0].label = NULL;
        out[0].color_override = -1;
        *out_count = 1;
        *out_n_axes = (int)zend_hash_num_elements(ht);
    }
    if (*out_n_axes > MAX_RADAR_AXES) *out_n_axes = MAX_RADAR_AXES;
    return *out_n_axes >= 3 ? 0 : -1;
}

static double read_d(HashTable *ht, int i)
{
    zval *v = zend_hash_index_find(ht, i);
    if (!v) return 0.0;
    double d;
    if (fastchart_zval_to_double(v, &d) != 0) return 0.0;
    return d < 0 ? 0.0 : d;
}

int fastchart_radar_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    radar_series_t series[MAX_RADAR_SERIES];
    int n_series = 0, n_axes = 0;
    if (collect_radar_series(&self->data, series, MAX_RADAR_SERIES,
                             &n_series, &n_axes) != 0 || n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\RadarChart::draw() requires setSeries() with at least 3 axes per series");
        return -1;
    }

    /* Find global max so each axis shares the same scale. */
    double dmax = 0.0;
    for (int s = 0; s < n_series; s++) {
        for (int i = 0; i < n_axes; i++) {
            double v = read_d(series[s].data, i);
            if (v > dmax) dmax = v;
        }
    }
    if (self->radar_max > 0) dmax = self->radar_max;
    if (dmax <= 0) dmax = 1.0;

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

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
    const char *font = fastchart_resolve_font(self, FC_FONT_LABEL);
    double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(self, FC_FONT_LABEL, base);
    zval *labels_zv = zend_hash_str_find(Z_ARRVAL(self->config),
        "category_labels", sizeof("category_labels") - 1);
    if (font && labels_zv && Z_TYPE_P(labels_zv) == IS_ARRAY) {
        for (int i = 0; i < n_axes; i++) {
            zval *lv = zend_hash_index_find(Z_ARRVAL_P(labels_zv), i);
            const char *label = fastchart_label_or_null(lv);
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
    int legend_colors[MAX_RADAR_SERIES];
    const char *legend_labels[MAX_RADAR_SERIES];
    int legend_count = 0;

    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        if (series[s].color_override >= 0) {
            int rgb = (int)series[s].color_override;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }
        gdPoint poly[MAX_RADAR_AXES];
        for (int i = 0; i < n_axes; i++) {
            double v = read_d(series[s].data, i);
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
            fastchart_shadow_filled_polygon(im, self, poly, n_axes);
            gdImageFilledPolygon(im, poly, n_axes, alpha);
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
        if (series[s].label && legend_count < MAX_RADAR_SERIES) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Title at top center. */
    fastchart_draw_floating_title(im, self, &pal, W / 2, 24);

    /* Legend: reuse the standard helper with a synthetic "plot rect"
     * spanning the entire canvas so positioning works. */
    if (legend_count > 0) {
        fastchart_rect plot = { 10, title_h, W - 10, H - 10 };
        fastchart_draw_legend(im, self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(im, self, &pal);
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
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_radar_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
