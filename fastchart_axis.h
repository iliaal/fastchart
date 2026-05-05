/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2026 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,     |
  | that is bundled with this package in the file LICENSE, and is       |
  | available through the world-wide-web at the following url:          |
  | http://www.php.net/license/3_01.txt                                 |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifndef FASTCHART_AXIS_H
#define FASTCHART_AXIS_H

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include <gd.h>

#define FASTCHART_MAX_TICKS 16

typedef struct {
    int x0, y0;     /* top-left of plot area, inclusive */
    int x1, y1;     /* bottom-right of plot area, inclusive */
} fastchart_rect;

typedef struct {
    double min;
    double max;
    double tick_step;
    int    n_ticks;
    double ticks[FASTCHART_MAX_TICKS];
} fastchart_value_range;

/* Compute the plot rectangle inside the canvas after subtracting space
 * for title (top), x-axis labels (bottom), and y-axis labels (left).
 * Right margin is a small fixed pad. */
void fastchart_compute_layout(fastchart_obj *chart, gdImagePtr im,
                              int has_y_axis, int has_x_axis,
                              fastchart_rect *out_plot);

/* "Nice" rounded value range that brackets [dmin, dmax]. Picks tick
 * step from {1, 2, 2.5, 5} × 10^N to land roughly `target_ticks` lines.
 * Always emits at least 2 ticks. */
void fastchart_value_range_compute(double dmin, double dmax,
                                   int target_ticks,
                                   fastchart_value_range *out);

/* Map a data y-value to a pixel y (gd: 0 at top). */
int fastchart_y_to_pixel(double y,
                         const fastchart_value_range *range,
                         const fastchart_rect *plot);

/* Draw the chart background, plot area, and border. */
void fastchart_draw_frame(gdImagePtr im, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal);

/* Draw the title above the plot area (no-op if title is empty). */
void fastchart_draw_title(gdImagePtr im, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal);

/* Draw the Y axis: vertical line, tick marks, labels (one per tick),
 * and horizontal grid lines. */
void fastchart_draw_y_axis(gdImagePtr im, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           const fastchart_value_range *range);

/* Draw an X axis whose positions are categorical (one tick per index
 * 0..n-1). Optional `labels` array (NULL means just draw indices); if
 * provided, `labels[i]` is rendered under tick i. */
void fastchart_draw_x_axis_categorical(gdImagePtr im, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       int n_categories,
                                       const char *const *labels);

/* Draw an X axis whose positions are Unix timestamps. Picks ~5 evenly
 * spaced labels formatted YYYY-MM-DD. */
void fastchart_draw_x_axis_time(gdImagePtr im, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal,
                                long t_min, long t_max);

/* Compute the X-pixel center of category `idx` of `n` total. */
int fastchart_x_categorical_center(const fastchart_rect *plot, int idx, int n);

/* Compute the X-pixel for a Unix-timestamp `ts` within [t_min, t_max]. */
int fastchart_x_time_to_pixel(const fastchart_rect *plot,
                              long ts, long t_min, long t_max);

/* Coerce a zval (long or double, with bounded long-string parsing) to
 * a double. Returns 0 on success, -1 on type mismatch. */
int fastchart_zval_to_double(zval *zv, double *out);

/* Coerce a zval to a long. Returns 0 on success, -1 on mismatch. */
int fastchart_zval_to_long(zval *zv, long *out);

#endif /* FASTCHART_AXIS_H */
