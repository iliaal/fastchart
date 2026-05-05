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

#define MAX_POLAR_SERIES 8
#define MAX_POLAR_POINTS 512

typedef struct {
    HashTable *data;
    const char *label;
    long color_override;
} polar_series_t;

static int collect_polar_series(zval *data_zv, polar_series_t *out, int max_series,
                                int *out_count)
{
    *out_count = 0;
    if (Z_TYPE_P(data_zv) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(data_zv);
    if (zend_hash_num_elements(ht) == 0) return -1;

    /* Detect: list of [angle, r] vs list of {data: [...]}. */
    zval *first = zend_hash_index_find(ht, 0);
    int is_multi = 0;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *d = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (d && Z_TYPE_P(d) == IS_ARRAY) is_multi = 1;
    }

    if (is_multi) {
        zval *s;
        ZEND_HASH_FOREACH_VAL(ht, s) {
            if (Z_TYPE_P(s) != IS_ARRAY) continue;
            if (*out_count >= max_series) break;
            zval *d = zend_hash_str_find(Z_ARRVAL_P(s), "data", sizeof("data") - 1);
            if (!d || Z_TYPE_P(d) != IS_ARRAY) continue;
            out[*out_count].data = Z_ARRVAL_P(d);
            zval *l = zend_hash_str_find(Z_ARRVAL_P(s), "label", sizeof("label") - 1);
            out[*out_count].label = (l && Z_TYPE_P(l) == IS_STRING) ? Z_STRVAL_P(l) : NULL;
            zval *c = zend_hash_str_find(Z_ARRVAL_P(s), "color", sizeof("color") - 1);
            out[*out_count].color_override = (c && Z_TYPE_P(c) == IS_LONG &&
                Z_LVAL_P(c) >= 0 && Z_LVAL_P(c) <= 0xFFFFFF) ? Z_LVAL_P(c) : -1;
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        out[0].data = ht;
        out[0].label = NULL;
        out[0].color_override = -1;
        *out_count = 1;
    }
    return 0;
}

int fastchart_polar_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    polar_series_t series[MAX_POLAR_SERIES];
    int n_series = 0;
    if (collect_polar_series(&self->data, series, MAX_POLAR_SERIES,
                             &n_series) != 0 || n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\PolarChart::draw() requires setSeries() with non-empty data");
        return -1;
    }

    /* Find global max radius across all series. */
    double rmax = 0;
    for (int s = 0; s < n_series; s++) {
        zval *p;
        ZEND_HASH_FOREACH_VAL(series[s].data, p) {
            if (Z_TYPE_P(p) != IS_ARRAY) continue;
            zval *zr = zend_hash_index_find(Z_ARRVAL_P(p), 1);
            if (!zr) continue;
            double r;
            if (fastchart_zval_to_double(zr, &r) != 0) continue;
            if (r < 0) r = 0;
            if (r > rmax) rmax = r;
        } ZEND_HASH_FOREACH_END();
    }
    if (self->polar_max_radius > 0) rmax = self->polar_max_radius;
    if (rmax <= 0) rmax = 1.0;

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

    int top = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 8;
    int cx = W / 2;
    int cy = (top + H) / 2;
    int radius = (W < H ? W : H) / 2 - 40;
    if (radius < 40) radius = 40;

    /* Concentric grid + radial spokes every 30°. */
    const int rings = 4;
    for (int r = 1; r <= rings; r++) {
        int rr = (int)((double)radius * (double)r / (double)rings);
        gdImageEllipse(im, cx, cy, rr * 2, rr * 2, pal.grid);
    }
    for (int a = 0; a < 360; a += 30) {
        double rad = a * M_PI / 180.0;
        int tx = cx + (int)(radius * cos(rad));
        int ty = cy - (int)(radius * sin(rad));
        gdImageLine(im, cx, cy, tx, ty, pal.grid);
    }

    int legend_colors[MAX_POLAR_SERIES];
    const char *legend_labels[MAX_POLAR_SERIES];
    int legend_count = 0;

    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        if (series[s].color_override >= 0) {
            int rgb = (int)series[s].color_override;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }

        gdPoint poly[MAX_POLAR_POINTS];
        int n_pts = 0;
        zval *p;
        ZEND_HASH_FOREACH_VAL(series[s].data, p) {
            if (Z_TYPE_P(p) != IS_ARRAY || n_pts >= MAX_POLAR_POINTS) continue;
            zval *za = zend_hash_index_find(Z_ARRVAL_P(p), 0);
            zval *zr = zend_hash_index_find(Z_ARRVAL_P(p), 1);
            if (!za || !zr) continue;
            double a, r;
            if (fastchart_zval_to_double(za, &a) != 0) continue;
            if (fastchart_zval_to_double(zr, &r) != 0) continue;
            if (r < 0) r = 0;
            double rad = a * M_PI / 180.0;
            double rr = radius * r / rmax;
            poly[n_pts].x = cx + (int)(rr * cos(rad));
            poly[n_pts].y = cy - (int)(rr * sin(rad));
            n_pts++;
        } ZEND_HASH_FOREACH_END();
        if (n_pts < 2) continue;

        if (self->polar_filled && n_pts >= 3) {
            int rr = gdImageRed(im, color);
            int gg = gdImageGreen(im, color);
            int bb = gdImageBlue(im, color);
            int alpha = gdImageColorAllocateAlpha(im, rr, gg, bb, 90);
            gdImageAlphaBlending(im, 1);
            fastchart_shadow_filled_polygon(im, self, poly, n_pts);
            gdImageFilledPolygon(im, poly, n_pts, alpha);
            gdImageAlphaBlending(im, 0);
        }
        gdImageSetThickness(im, 2);
        if (self->polar_filled && n_pts >= 3) {
            gdImagePolygon(im, poly, n_pts, color);
        } else {
            for (int i = 0; i < n_pts - 1; i++) {
                gdImageLine(im, poly[i].x, poly[i].y,
                            poly[i+1].x, poly[i+1].y, color);
            }
        }
        gdImageSetThickness(im, 1);
        for (int i = 0; i < n_pts; i++) {
            fastchart_draw_marker(im, poly[i].x, poly[i].y,
                                  FASTCHART_MARKER_CIRCLE, 5, color);
        }
        if (series[s].label && legend_count < MAX_POLAR_SERIES) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Title. */
    const char *font = fastchart_resolve_font(self, "title");
    if (self->title && ZSTR_LEN(self->title) > 0 && font && !self->thumbnail_mode) {
        double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        double size = fastchart_resolve_font_size(self, "title", base * 1.4);
        int color = self->title_color >= 0 ? (int)self->title_color : pal.text;
        fastchart_shadow_text(im, self, font, size, W / 2, 24, 0.0, ZSTR_VAL(self->title));
        fastchart_text_draw(im, font, size, color, W / 2, 24,
                            FASTCHART_ALIGN_CENTER, ZSTR_VAL(self->title), NULL, 0);
    }

    if (legend_count > 0) {
        fastchart_rect plot = { 10, top, W - 10, H - 10 };
        fastchart_draw_legend(im, self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(im, self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_PolarChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) RETURN_THROWS();
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_polar_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
