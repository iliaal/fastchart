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
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"
#include "fastchart_effects.h"

#define MAX_BOXES   64
#define MAX_OUTLIER 32

int fastchart_boxplot_render_to_image(fastchart_boxplot_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    if (self->entry_count == 0) {
        zend_throw_error(NULL,
            "FastChart\\BoxPlot::draw() requires setBoxes() with non-empty data");
        return -1;
    }
    fastchart_boxplot_entry *boxes = self->entries;
    int n = self->entry_count;

    double dmin = boxes[0].min, dmax = boxes[0].max;
    for (int i = 0; i < n; i++) {
        if (boxes[i].min < dmin) dmin = boxes[i].min;
        if (boxes[i].max > dmax) dmax = boxes[i].max;
        for (int k = 0; k < boxes[i].outlier_count; k++) {
            if (boxes[i].outliers[k] < dmin) dmin = boxes[i].outliers[k];
            if (boxes[i].outliers[k] > dmax) dmax = boxes[i].outliers[k];
        }
    }

    fastchart_value_range range;
    fastchart_value_range_compute(dmin, dmax, 6, &range);
    fastchart_value_range_apply_override((fastchart_obj *)self, &range);

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, &t, 1, 1, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(&t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(&t, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(&t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(&t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(&t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_plot_bands(&t, (fastchart_obj *)self, &plot, &range, &pal);
    fastchart_draw_v_plot_bands_categorical(&t, (fastchart_obj *)self, &plot,
                                            n, &pal);

    /* Use category labels if supplied, else fall back to per-box label
     * fields, else integer indices. */
    const char **labels = ecalloc(n, sizeof(const char *));
    fastchart_obj *base = (fastchart_obj *)self;
    for (int i = 0; i < n; i++) {
        if (base->category_labels && i < base->n_category_labels) {
            labels[i] = base->category_labels[i];
        }
        if (!labels[i] && boxes[i].label) labels[i] = boxes[i].label;
    }
    fastchart_draw_x_axis_categorical(&t, (fastchart_obj *)self, &plot, &pal, n, labels);
    fastchart_draw_axis_titles(&t, (fastchart_obj *)self, &plot, &pal);
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
        gdImageLine(im, cx, y_min, cx, y_q1, fastchart_target_color_to_gd(&t, pal.axis));
        gdImageLine(im, cx, y_q3,  cx, y_max, fastchart_target_color_to_gd(&t, pal.axis));
        gdImageLine(im, cx - box_w / 4, y_min, cx + box_w / 4, y_min, fastchart_target_color_to_gd(&t, pal.axis));
        gdImageLine(im, cx - box_w / 4, y_max, cx + box_w / 4, y_max, fastchart_target_color_to_gd(&t, pal.axis));

        /* Q1..Q3 box. */
        fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y_q3, x1, y_q1);
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
        for (int k = 0; k < boxes[i].outlier_count; k++) {
            int oy = fastchart_y_to_pixel(boxes[i].outliers[k], &range, &plot);
            gdImageEllipse(im, cx, oy, 4, 4, color);
        }
    }

    fastchart_draw_h_annotations(&t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(&t, (fastchart_obj *)self, &plot, &pal, n);
    fastchart_draw_overlays_categorical(&t, (fastchart_obj *)self, &plot, &pal,
                                         &range, NULL, n);
    fastchart_draw_text_annotations(&t, (fastchart_obj *)self, &pal);

    if (self->icons && self->n_icons > 0 && n > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_x = n > 1 ? (ic->x + 0.5) / (double)n : 0.5;
            int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(ic->y, &range, &plot);
            fastchart_blit_icon(&t, ic, px, py);
        }
    }
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
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_boxplot_obj *self = Z_FASTCHART_BOXPLOT_OBJ_P(ZEND_THIS);
    if (fastchart_boxplot_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
