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
    int    log_scale;       /* 0 linear, 1 base-10 log */
    double ticks[FASTCHART_MAX_TICKS];
} fastchart_value_range;

/* Compute the plot rectangle inside the canvas after subtracting space
 * for title (top), x-axis labels (bottom), and y-axis labels (left).
 * Right margin grows when the chart has a secondary Y axis. Axis
 * titles, when set on the chart, also reserve space. */
void fastchart_compute_layout(fastchart_obj *chart, gdImagePtr im,
                              int has_y_axis, int has_x_axis,
                              fastchart_rect *out_plot);

/* "Nice" rounded value range that brackets [dmin, dmax]. Picks tick
 * step from {1, 2, 2.5, 5} × 10^N to land roughly `target_ticks` lines.
 * Always emits at least 2 ticks. */
void fastchart_value_range_compute(double dmin, double dmax,
                                   int target_ticks,
                                   fastchart_value_range *out);

/* Apply the chart's setYAxisRange() overrides on top of an
 * already-computed `out`. If the user forced min/max, ticks are
 * regenerated using the (also optional) forced interval. No-op if
 * the chart has no overrides. */
void fastchart_value_range_apply_override(const fastchart_obj *chart,
                                          fastchart_value_range *out);

/* Log10 value range; ticks at powers of ten that bracket the data.
 * Both `dmin` and `dmax` must be strictly positive. Returns 0 on
 * success, -1 if dmin <= 0 (caller throws). */
int fastchart_value_range_compute_log(double dmin, double dmax,
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

/* Draw the secondary Y axis on the right edge of the plot. No grid
 * lines (those would clash with the primary axis grid); just the
 * vertical line, tick marks, and labels. Charts that don't enable
 * setSecondaryYAxis() should skip this call. */
void fastchart_draw_y_axis_right(gdImagePtr im, fastchart_obj *chart,
                                 const fastchart_rect *plot,
                                 const fastchart_palette *pal,
                                 const fastchart_value_range *range);

/* Draw axis-title text: x_axis_title centered below the X-axis
 * labels, y_axis_title rotated 90deg on the left edge. No-op if
 * the corresponding chart field is NULL or empty. */
void fastchart_draw_axis_titles(gdImagePtr im, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal);

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

/* Draw a single point marker. `style` is one of FASTCHART_MARKER_*;
 * `size` is the marker's enclosing diameter in pixels. NONE renders
 * nothing. */
void fastchart_draw_marker(gdImagePtr im, int x, int y,
                           int style, int size, int color);

/* Draw a legend in the position the chart's `legend_position`
 * field selects (one of FASTCHART_LEGEND_*). One row per series
 * with a color swatch and a label. `colors[i]` and `labels[i]`
 * are paired. A label may be NULL, in which case the row is
 * skipped. The legend has an opaque background so it overdraws
 * data underneath -- callers place this last. No-op if
 * n_entries < 1, the chart has no font, or position == LEGEND_NONE. */
void fastchart_draw_legend(gdImagePtr im, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           int n_entries,
                           const int *colors,
                           const char *const *labels);

/* Draw any horizontal-line annotations stored on the chart. The
 * y-mapping uses the supplied value_range. Out-of-range
 * annotations are silently clamped to the plot bounds. */
void fastchart_draw_h_annotations(gdImagePtr im, fastchart_obj *chart,
                                  const fastchart_rect *plot,
                                  const fastchart_palette *pal,
                                  const fastchart_value_range *yrange);

/* Vertical annotation drawing variants -- one per X-coordinate
 * system. `position` in the annotation is interpreted per the
 * chart's X axis. */
void fastchart_draw_v_annotations_categorical(gdImagePtr im, fastchart_obj *chart,
                                              const fastchart_rect *plot,
                                              const fastchart_palette *pal,
                                              int n_categories);

void fastchart_draw_v_annotations_continuous(gdImagePtr im, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             const fastchart_palette *pal,
                                             const fastchart_value_range *xrange);

void fastchart_draw_v_annotations_time(gdImagePtr im, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       long t_min, long t_max);

#endif /* FASTCHART_AXIS_H */
