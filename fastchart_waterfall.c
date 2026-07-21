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

/* Default colors when the user hasn't called setRiseColor / setFallColor /
 * setTotalColor. These match the conventional accounting-statement palette
 * (green for positive deltas, red for negative, blue for totals). */
#define WF_DEFAULT_RISE   0x4FB286
#define WF_DEFAULT_FALL   0xE34A6F
#define WF_DEFAULT_TOTAL  0x2E86DE

int fastchart_waterfall_render_to_target(fastchart_waterfall_obj *self, fastchart_target_t *t)
{
    if (self->bar_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Waterfall::draw() requires setBars() with at least one bar");
        return -1;
    }

    /* Compute running cumulative + value range for the Y axis.
     * For a TOTAL bar, the value is treated as the absolute height
     * from zero (the bar represents the cumulative-to-date snapshot
     * at that step, drawn from baseline 0). For a DELTA bar, the
     * bar runs from cum to cum+value. */
    int n = self->bar_count;
    double *bar_lo = ecalloc((size_t)n, sizeof(double));
    double *bar_hi = ecalloc((size_t)n, sizeof(double));
    double cum = 0;
    double y_min = 0, y_max = 0;
    for (int i = 0; i < n; i++) {
        if (self->bars[i].kind == FASTCHART_WF_TOTAL) {
            /* A TOTAL bar is anchored to zero but keeps its sign: a
             * net-loss subtotal draws from the negative value up to the
             * baseline (below zero), matching how waterfall charts show
             * a negative cumulative total. The signed span feeds the
             * y-range scan below so the axis includes the negative
             * region; cum carries the signed value forward. */
            double val = self->bars[i].value;
            bar_lo[i] = val < 0 ? val : 0;
            bar_hi[i] = val > 0 ? val : 0;
            cum = val;
        } else {
            double next = cum + self->bars[i].value;
            bar_lo[i] = cum < next ? cum : next;
            bar_hi[i] = cum > next ? cum : next;
            cum = next;
        }
        if (bar_lo[i] < y_min) y_min = bar_lo[i];
        if (bar_hi[i] > y_max) y_max = bar_hi[i];
    }
    if (y_max <= y_min) y_max = y_min + 1.0;

    fastchart_value_range range;
    fastchart_value_range_compute(y_min, y_max, 6, &range);
	if (fastchart_value_range_apply_override((fastchart_obj *)self,
			&range) != 0) {
		efree(bar_lo);
		efree(bar_hi);
		return -1;
	}

    fastchart_rect plot;
    /* Borrow the bar labels for the categorical x-axis margin so
     * long stage names don't clip on the bottom. */
    const char **labels = ecalloc((size_t)n, sizeof(const char *));
    for (int i = 0; i < n; i++) labels[i] = self->bars[i].label;
    /* Expose the bar labels to the layout's x-axis margin reservation so a
     * rotated long label (setXAxisLabelAngle) reserves enough bottom margin
     * instead of overflowing the canvas. The labels come from setBars(), not
     * setCategoryLabels(), so compute_layout can't see them otherwise.
     * draw_x_axis_categorical() below receives the labels explicitly;
     * category_labels is borrowed only for the measurement, then restored so
     * nothing else (or the dtor) treats the borrowed array as owned. */
    char **saved_cat = ((fastchart_obj *)self)->category_labels;
    int saved_cat_n = ((fastchart_obj *)self)->n_category_labels;
    ((fastchart_obj *)self)->category_labels = (char **)labels;
    ((fastchart_obj *)self)->n_category_labels = n;
    fastchart_compute_layout((fastchart_obj *)self, t, 1, 1, NULL, 0, &plot);
    ((fastchart_obj *)self)->category_labels = saved_cat;
    ((fastchart_obj *)self)->n_category_labels = saved_cat_n;

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_plot_bands(t, (fastchart_obj *)self, &plot, &range, &pal);
    fastchart_draw_v_plot_bands_categorical(t, (fastchart_obj *)self, &plot,
                                            n, &pal);
    fastchart_draw_x_axis_categorical(t, (fastchart_obj *)self, &plot, &pal, n, labels);
    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

    int rise_rgb  = self->rise_color  >= 0 ? self->rise_color  : WF_DEFAULT_RISE;
    int fall_rgb  = self->fall_color  >= 0 ? self->fall_color  : WF_DEFAULT_FALL;
    int total_rgb = self->total_color >= 0 ? self->total_color : WF_DEFAULT_TOTAL;
    int rise_c  = fastchart_target_color_rgb(t, rise_rgb);
    int fall_c  = fastchart_target_color_rgb(t, fall_rgb);
    int total_c = fastchart_target_color_rgb(t, total_rgb);

    int slot_w = (plot.x1 - plot.x0) / n;
    /* Too many bars for the plot width collapses slot_w to 0, stacking
     * every bar at the left edge. Reject instead of emitting a misleading
     * chart, matching ParetoChart / Funnel. */
    if (slot_w < 1) {
        efree(bar_lo);
        efree(bar_hi);
        efree((void *)labels);
        zend_value_error(
            "FastChart\\Waterfall::draw() canvas is too narrow for the number of bars");
        return -1;
    }
    int bar_pad = slot_w / 6;
    if (bar_pad < 1) bar_pad = 1;
    int bar_w = slot_w - 2 * bar_pad;
    if (bar_w < 1) bar_w = 1;

    for (int i = 0; i < n; i++) {
        int slot_cx = fastchart_x_categorical_center(&plot, i, n);
        int x0 = slot_cx - bar_w / 2;
        int x1 = x0 + bar_w - 1;
        int y_top = fastchart_y_to_pixel(bar_hi[i], &range, &plot);
        int y_bot = fastchart_y_to_pixel(bar_lo[i], &range, &plot);

        int color;
        if (self->bars[i].kind == FASTCHART_WF_TOTAL) {
            color = total_c;
        } else if (self->bars[i].value >= 0) {
            color = rise_c;
        } else {
            color = fall_c;
        }
        fastchart_target_rect(t, x0, y_top, x1 - x0 + 1, y_bot - y_top + 1, color, 1, 0);
        fastchart_target_rect(t, x0, y_top, x1 - x0 + 1, y_bot - y_top + 1, pal.border, 0, 1);

        /* Connector line from this bar's right edge to the next
         * bar's left at the running cumulative; gives the chart its
         * characteristic "stair-step" look. Skip after a TOTAL bar
         * since the next bar restarts the cumulative. */
        if (i + 1 < n && self->bars[i].kind != FASTCHART_WF_TOTAL) {
            int y_conn = fastchart_y_to_pixel(
                self->bars[i].value >= 0 ? bar_hi[i] : bar_lo[i],
                &range, &plot);
            int next_slot_cx = fastchart_x_categorical_center(&plot, i + 1, n);
            int x_next0 = next_slot_cx - bar_w / 2;
            fastchart_target_line(t, x1 + 1, y_conn, x_next0 - 1, y_conn,
                                  pal.grid, 1, FASTCHART_DASH_SOLID);
        }
    }

    fastchart_draw_h_annotations(t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(t, (fastchart_obj *)self, &plot, &pal, n);

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    efree(bar_lo);
    efree(bar_hi);
    efree((void *)labels);
    return 0;
}
