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

#define MAX_BOXES   64
#define MAX_OUTLIER 32

typedef struct {
    double min, q1, median, q3, max;
    double outliers[MAX_OUTLIER];
    int n_outliers;
    const char *label;
} fastchart_box;

static int read_5(HashTable *ht, double *m, double *q1, double *med,
                  double *q3, double *mx)
{
    zval *z;
    z = zend_hash_index_find(ht, 0); if (!z || fastchart_zval_to_double(z, m)   != 0) return -1;
    z = zend_hash_index_find(ht, 1); if (!z || fastchart_zval_to_double(z, q1)  != 0) return -1;
    z = zend_hash_index_find(ht, 2); if (!z || fastchart_zval_to_double(z, med) != 0) return -1;
    z = zend_hash_index_find(ht, 3); if (!z || fastchart_zval_to_double(z, q3)  != 0) return -1;
    z = zend_hash_index_find(ht, 4); if (!z || fastchart_zval_to_double(z, mx)  != 0) return -1;
    return 0;
}

static int read_dict_box(HashTable *ht, fastchart_box *out)
{
    zval *zmin = zend_hash_str_find(ht, "min", sizeof("min") - 1);
    zval *zq1  = zend_hash_str_find(ht, "q1",  sizeof("q1")  - 1);
    zval *zmed = zend_hash_str_find(ht, "median", sizeof("median") - 1);
    zval *zq3  = zend_hash_str_find(ht, "q3",  sizeof("q3")  - 1);
    zval *zmax = zend_hash_str_find(ht, "max", sizeof("max") - 1);
    if (!zmin || !zq1 || !zmed || !zq3 || !zmax) return -1;
    if (fastchart_zval_to_double(zmin, &out->min) != 0) return -1;
    if (fastchart_zval_to_double(zq1,  &out->q1)  != 0) return -1;
    if (fastchart_zval_to_double(zmed, &out->median) != 0) return -1;
    if (fastchart_zval_to_double(zq3,  &out->q3)  != 0) return -1;
    if (fastchart_zval_to_double(zmax, &out->max) != 0) return -1;

    zval *zlabel = zend_hash_str_find(ht, "label", sizeof("label") - 1);
    out->label = fastchart_label_or_null(zlabel);

    out->n_outliers = 0;
    zval *zout = zend_hash_str_find(ht, "outliers", sizeof("outliers") - 1);
    if (zout && Z_TYPE_P(zout) == IS_ARRAY) {
        zval *v;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zout), v) {
            if (out->n_outliers >= MAX_OUTLIER) break;
            double d;
            if (fastchart_zval_to_double(v, &d) == 0) {
                out->outliers[out->n_outliers++] = d;
            }
        } ZEND_HASH_FOREACH_END();
    }
    return 0;
}

int fastchart_boxplot_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    HashTable *data = Z_TYPE(self->data) == IS_ARRAY ? Z_ARRVAL(self->data) : NULL;
    if (!data || zend_hash_num_elements(data) == 0) {
        zend_throw_error(NULL,
            "FastChart\\BoxPlot::draw() requires setBoxes() with non-empty data");
        return -1;
    }

    fastchart_box boxes[MAX_BOXES];
    int n = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(data, entry) {
        if (n >= MAX_BOXES) break;
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        boxes[n].n_outliers = 0;
        boxes[n].label = NULL;
        HashTable *eh = Z_ARRVAL_P(entry);
        if (zend_hash_str_find(eh, "min", sizeof("min") - 1)) {
            if (read_dict_box(eh, &boxes[n]) != 0) continue;
        } else {
            if (read_5(eh, &boxes[n].min, &boxes[n].q1, &boxes[n].median,
                       &boxes[n].q3, &boxes[n].max) != 0) continue;
        }
        n++;
    } ZEND_HASH_FOREACH_END();

    if (n == 0) {
        zend_throw_error(NULL,
            "FastChart\\BoxPlot::draw() found no valid box entries");
        return -1;
    }

    /* Find global Y range, including outliers. */
    double dmin = boxes[0].min, dmax = boxes[0].max;
    for (int i = 0; i < n; i++) {
        if (boxes[i].min < dmin) dmin = boxes[i].min;
        if (boxes[i].max > dmax) dmax = boxes[i].max;
        for (int k = 0; k < boxes[i].n_outliers; k++) {
            if (boxes[i].outliers[k] < dmin) dmin = boxes[i].outliers[k];
            if (boxes[i].outliers[k] > dmax) dmax = boxes[i].outliers[k];
        }
    }

    fastchart_value_range range;
    fastchart_value_range_compute(dmin, dmax, 6, &range);
    fastchart_value_range_apply_override(self, &range);

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &plot, &pal, &range);

    /* Use category labels if supplied, else fall back to per-box label
     * fields, else integer indices. */
    const char **labels = ecalloc(n, sizeof(const char *));
    zval *cat_zv = zend_hash_str_find(Z_ARRVAL(self->config),
        "category_labels", sizeof("category_labels") - 1);
    for (int i = 0; i < n; i++) {
        if (cat_zv && Z_TYPE_P(cat_zv) == IS_ARRAY) {
            zval *lv = zend_hash_index_find(Z_ARRVAL_P(cat_zv), i);
            labels[i] = fastchart_label_or_null(lv);
        }
        if (!labels[i] && boxes[i].label) labels[i] = boxes[i].label;
    }
    fastchart_draw_x_axis_categorical(im, self, &plot, &pal, n, labels);
    fastchart_draw_axis_titles(im, self, &plot, &pal);
    efree(labels);

    int slot_w = (plot.x1 - plot.x0) / n;
    int box_pct = (int)self->box_width_pct;
    if (box_pct <= 0) box_pct = 60;
    int box_w = slot_w * box_pct / 100;
    if (box_w < 4) box_w = 4;

    for (int i = 0; i < n; i++) {
        int cx = fastchart_x_categorical_center(&plot, i, n);
        int x0 = cx - box_w / 2;
        int x1 = cx + box_w / 2;

        int y_min = fastchart_y_to_pixel(boxes[i].min,    &range, &plot);
        int y_q1  = fastchart_y_to_pixel(boxes[i].q1,     &range, &plot);
        int y_med = fastchart_y_to_pixel(boxes[i].median, &range, &plot);
        int y_q3  = fastchart_y_to_pixel(boxes[i].q3,     &range, &plot);
        int y_max = fastchart_y_to_pixel(boxes[i].max,    &range, &plot);

        int color = pal.series[i % FASTCHART_PALETTE_SERIES_N];
        int r = gdImageRed(im, color);
        int g = gdImageGreen(im, color);
        int b = gdImageBlue(im, color);
        int alpha = gdImageColorAllocateAlpha(im, r, g, b, 64);

        /* Whiskers (vertical lines from min->q1, q3->max) with caps. */
        gdImageLine(im, cx, y_min, cx, y_q1, pal.axis);
        gdImageLine(im, cx, y_q3,  cx, y_max, pal.axis);
        gdImageLine(im, cx - box_w / 4, y_min, cx + box_w / 4, y_min, pal.axis);
        gdImageLine(im, cx - box_w / 4, y_max, cx + box_w / 4, y_max, pal.axis);

        /* Q1..Q3 box. */
        fastchart_shadow_filled_rectangle(im, self, x0, y_q3, x1, y_q1);
        gdImageAlphaBlending(im, 1);
        gdImageFilledRectangle(im, x0, y_q3, x1, y_q1, alpha);
        gdImageAlphaBlending(im, 0);
        gdImageRectangle(im, x0, y_q3, x1, y_q1,
                         self->edge_color >= 0 ? (int)self->edge_color : pal.axis);

        /* Median line. */
        gdImageSetThickness(im, 2);
        gdImageLine(im, x0, y_med, x1, y_med,
                    self->edge_color >= 0 ? (int)self->edge_color : pal.axis);
        gdImageSetThickness(im, 1);

        /* Outliers as small open circles. */
        for (int k = 0; k < boxes[i].n_outliers; k++) {
            int oy = fastchart_y_to_pixel(boxes[i].outliers[k], &range, &plot);
            gdImageEllipse(im, cx, oy, 4, 4, color);
        }
    }

    fastchart_draw_text_annotations(im, self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_BoxPlot, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\BoxPlot::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_boxplot_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
