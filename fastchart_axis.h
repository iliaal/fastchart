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

#ifndef FASTCHART_AXIS_H
#define FASTCHART_AXIS_H

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"

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

/* Per-render entry hook. Invalidates the font-path cache and the
 * shadow-color cache (so the next resolve_font / shadow_alloc call
 * re-runs open_basedir checks against the current ini state and
 * re-allocates against the current gdImage), and stamps the chart's
 * DPI on the canvas. Every renderer must call this at draw entry,
 * BEFORE any palette / text / background work. fastchart_compute_layout
 * already calls it; non-layout renderers (gauge, radar, polar, surface,
 * contour) need to call it directly. */
void fastchart_begin_render(fastchart_obj *chart, fastchart_target_t *t);

/* Compute the plot rectangle inside the canvas after subtracting space
 * for title (top), x-axis labels (bottom), and y-axis labels (left).
 * Right margin grows when the chart has a secondary Y axis. Axis
 * titles, when set on the chart, also reserve space.
 *
 * `cat_y_labels` / `n_cat_y_labels` switch the left-margin reservation
 * from the numeric "999999" probe to the actual widest categorical
 * label width — needed for horizontal-bar layouts where the Y axis is
 * categorical and label widths are arbitrary. Pass NULL / 0 for the
 * default numeric Y axis. */
void fastchart_compute_layout(fastchart_obj *chart, fastchart_target_t *t,
                              int has_y_axis, int has_x_axis,
                              const char *const *cat_y_labels,
                              int n_cat_y_labels,
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
void fastchart_draw_frame(fastchart_target_t *t, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal);

/* Draw any horizontal plot bands registered via addHorizontalBand().
 * Z-order: behind chart data, in front of the plot background. Each
 * band is a filled, alpha-blended rectangle spanning the full plot
 * width and the [low, high] Y range, clipped to the plot rect.
 * No-op when the chart has no bands. */
void fastchart_draw_plot_bands(fastchart_target_t *t, fastchart_obj *chart,
                               const fastchart_rect *plot,
                               const fastchart_value_range *yrange,
                               const fastchart_palette *pal);

/* Vertical plot bands. Three variants for the three X-axis kinds:
 *  - categorical: x in [0, n_categories] as fractional index
 *  - xrange: x as a data value mapped through a value range
 *  - time: x as a unix timestamp mapped through (t_min, t_max)
 * Each renderer that supports vertical bands picks the right one for
 * its X axis and calls it next to fastchart_draw_plot_bands. Each
 * skips horizontal bands (i.e. only entries with is_vertical=true). */
void fastchart_draw_v_plot_bands_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             int n_categories,
                                             const fastchart_palette *pal);

/* Horizontal stripes spanning fractional Y category indices, for
 * horizontal-bar layouts where categories run top-to-bottom. Skips
 * vertical bands. */
void fastchart_draw_h_plot_bands_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             int n_categories,
                                             const fastchart_palette *pal);
void fastchart_draw_v_plot_bands_xrange(fastchart_target_t *t, fastchart_obj *chart,
                                        const fastchart_rect *plot,
                                        const fastchart_value_range *xrange,
                                        const fastchart_palette *pal);
void fastchart_draw_v_plot_bands_time(fastchart_target_t *t, fastchart_obj *chart,
                                      const fastchart_rect *plot,
                                      zend_long t_min, zend_long t_max,
                                      const fastchart_palette *pal);

/* Blit one icon (loaded from icon->path) onto im at the given pixel
 * coordinates, centered on (px, py). Respects max_w / max_h while
 * preserving the source image aspect ratio; -1 in either bound means
 * "use the source dimension as-is". Honors open_basedir restrictions
 * via php_check_open_basedir_ex; silently skips unreadable / invalid
 * images so a missing icon doesn't abort the whole render. */
void fastchart_blit_icon(fastchart_target_t *t, const fastchart_icon *icon,
                         int px, int py);

/* Draw the title above the plot area (no-op if title is empty).
 * Includes drop-shadow underneath when the chart has setDropShadow()
 * configured. */
void fastchart_draw_title(fastchart_target_t *t, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal);

/* Like fastchart_draw_title but takes the title baseline directly
 * (no plot rect). Used by charts with a non-rectangular layout
 * (radar, gauge, polar, surface, contour). Includes drop-shadow. */
void fastchart_draw_floating_title(fastchart_target_t *t, fastchart_obj *chart,
                                   const fastchart_palette *pal,
                                   int cx, int baseline);

/* Draw the Y axis: vertical line, tick marks, labels (one per tick),
 * and horizontal grid lines. */
void fastchart_draw_y_axis(fastchart_target_t *t, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           const fastchart_value_range *range);

/* Draw the secondary Y axis on the right edge of the plot. No grid
 * lines (those would clash with the primary axis grid); just the
 * vertical line, tick marks, and labels. Charts that don't enable
 * setSecondaryYAxis() should skip this call. */
void fastchart_draw_y_axis_right(fastchart_target_t *t, fastchart_obj *chart,
                                 const fastchart_rect *plot,
                                 const fastchart_palette *pal,
                                 const fastchart_value_range *range);

/* Draw axis-title text: x_axis_title centered below the X-axis
 * labels, y_axis_title rotated 90deg on the left edge. No-op if
 * the corresponding chart field is NULL or empty. */
void fastchart_draw_axis_titles(fastchart_target_t *t, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal);

/* Draw an X axis whose positions are categorical (one tick per index
 * 0..n-1). Optional `labels` array (NULL means just draw indices); if
 * provided, `labels[i]` is rendered under tick i. */
void fastchart_draw_x_axis_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       int n_categories,
                                       const char *const *labels);

/* Draw an X axis whose positions are Unix timestamps. Picks ~5 evenly
 * spaced labels formatted YYYY-MM-DD. */
void fastchart_draw_x_axis_time(fastchart_target_t *t, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal,
                                zend_long t_min, zend_long t_max);

/* Compute the X-pixel center of category `idx` of `n` total. */
int fastchart_x_categorical_center(const fastchart_rect *plot, int idx, int n);

/* Mirror of fastchart_x_categorical_center for horizontal-bar charts:
 * returns the pixel y at the center of the i-th category slot when
 * categories run top-to-bottom along the Y axis. */
int fastchart_y_categorical_center(const fastchart_rect *plot, int idx, int n);

/* Map a data x-value to pixel x using the chart's X value range
 * (mirror of fastchart_y_to_pixel). Used by horizontal bar charts
 * where X carries the value axis instead of the category axis. */
int fastchart_x_to_pixel(double x,
                         const fastchart_value_range *range,
                         const fastchart_rect *plot);

/* User-format tick label (printf-style format string already
 * validated by the setter). Caller-supplied buffer; truncates on
 * overrun rather than overflowing. */
void fastchart_format_tick_label_user(double value, const zend_string *fmt,
                                      char *out, size_t out_n);

/* Borrow the typed category-label slots stored on the chart base into
 * a freshly ecalloc()'d const char ** of length `n`. Caller efree()s.
 * Returns NULL when no labels are set or n <= 0. */
const char **fastchart_borrow_category_labels(fastchart_obj *b, int n);

/* Numeric X axis (ticks + gridlines + labels) for charts that put the
 * value axis on X — currently only horizontal-bar. Mirror of the
 * existing fastchart_draw_y_axis. */
void fastchart_draw_x_axis_numeric(fastchart_target_t *t, fastchart_obj *chart,
                                   const fastchart_rect *plot,
                                   const fastchart_palette *pal,
                                   const fastchart_value_range *range);

/* Categorical Y axis (labels + tick marks on the left edge) for
 * charts that put the category axis on Y — currently only
 * horizontal-bar. Mirror of fastchart_draw_x_axis_categorical. */
void fastchart_draw_y_axis_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       int n_categories,
                                       const char *const *labels);

/* Compute the X-pixel for a Unix-timestamp `ts` within [t_min, t_max]. */
int fastchart_x_time_to_pixel(const fastchart_rect *plot,
                              zend_long ts, zend_long t_min, zend_long t_max);

/* Coerce a zval (long or double, with bounded long-string parsing) to
 * a double. Returns 0 on success, -1 on type mismatch. */
int fastchart_zval_to_double(zval *zv, double *out);

/* Coerce a zval to a zend_long. Returns 0 on success, -1 on type
 * mismatch / non-finite double / out-of-range double. */
int fastchart_zval_to_long(zval *zv, zend_long *out);

/* Draw a single point marker. `style` is one of FASTCHART_MARKER_*;
 * `size` is the marker's enclosing diameter in pixels. NONE renders
 * nothing. */
void fastchart_draw_marker(fastchart_target_t *t, int x, int y,
                           int style, int size, int color);

/* Font roles map the calling site to one of three setter buckets
 * (title, axis, label). Distinct X-axis vs Y-axis vs axis-title
 * roles were collapsed because the resolver and the existing setter
 * surface (setAxisFont) bucket them together; if a future API splits
 * them, restore the per-axis variants here. */
typedef enum {
    FC_FONT_TITLE,
    FC_FONT_AXIS,
    FC_FONT_LABEL,
    FC_FONT_ANNOTATION,
    FC_FONT_ROLE_COUNT
} fastchart_font_role;

const char *fastchart_resolve_font(fastchart_obj *chart,
                                   fastchart_font_role role);
double fastchart_resolve_font_size(fastchart_obj *chart,
                                   fastchart_font_role role,
                                   double base_default);

typedef struct {
    int x, y;
    bool valid;   /* false marks a gap (NaN data point) */
} fastchart_pt;

/* Draw a polyline through `pts` honoring the chart's
 * line_interpolation setting. Linear uses gdImageLine for each
 * consecutive valid pair; smooth uses Catmull-Rom with ~10 sub-
 * segments per interval. Gaps (valid=false) break the polyline. */
void fastchart_draw_polyline(fastchart_target_t *t, fastchart_obj *chart,
                             const fastchart_pt *pts, int n,
                             int color, int thickness, bool antialiased);

/* Draw a numeric value label above (x, y) -- typically used by the
 * setShowValues() rendering path. Picks the value font + size and
 * applies the chart's value_format (default "%g"). No-op if the
 * chart has no font or show_values is off. `value` is the raw
 * datum, NaN values render nothing. */
void fastchart_draw_value_label(fastchart_target_t *t, fastchart_obj *chart,
                                const fastchart_palette *pal,
                                int x, int y, double value);

/* Walk overlay series stored on chart->config["overlays"] and
 * draw each as a line or area on top of the existing plot, using
 * the supplied yrange for left-axis overlays and yrange_right (may
 * be NULL) for right-axis ones. Caller passes the categorical-
 * X-axis cell count so position math stays consistent. */
/* Overlay rendering for horizontal-bar layouts (value axis is X,
 * category axis is Y). Mirror of fastchart_draw_overlays_categorical
 * with the axes swapped. */
void fastchart_draw_overlays_horizontal_bar(fastchart_target_t *t, fastchart_obj *chart,
                                            const fastchart_rect *plot,
                                            const fastchart_palette *pal,
                                            const fastchart_value_range *xrange,
                                            int n_categories);

void fastchart_draw_overlays_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                          const fastchart_rect *plot,
                                          const fastchart_palette *pal,
                                          const fastchart_value_range *yrange,
                                          const fastchart_value_range *yrange_right,
                                          int n_categories);

/* Time-axis variant for StockChart overlays. */
void fastchart_draw_overlays_time(fastchart_target_t *t, fastchart_obj *chart,
                                  const fastchart_rect *plot,
                                  const fastchart_palette *pal,
                                  const fastchart_value_range *yrange,
                                  zend_long t_min, zend_long t_max,
                                  zend_long *timestamps, int n_candles);

/* Draw a legend in the position the chart's `legend_position`
 * field selects (one of FASTCHART_LEGEND_*). One row per series
 * with a color swatch and a label. `colors[i]` and `labels[i]`
 * are paired. A label may be NULL, in which case the row is
 * skipped. The legend has an opaque background so it overdraws
 * data underneath -- callers place this last. No-op if
 * n_entries < 1, the chart has no font, or position == LEGEND_NONE. */
void fastchart_draw_legend(fastchart_target_t *t, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           int n_entries,
                           const int *colors,
                           const char *const *labels);

/* Draw any horizontal-line annotations stored on the chart. The
 * y-mapping uses the supplied value_range. Out-of-range
 * annotations are silently clamped to the plot bounds. */
void fastchart_draw_h_annotations(fastchart_target_t *t, fastchart_obj *chart,
                                  const fastchart_rect *plot,
                                  const fastchart_palette *pal,
                                  const fastchart_value_range *yrange);

/* Vertical annotation drawing variants -- one per X-coordinate
 * system. `position` in the annotation is interpreted per the
 * chart's X axis. */
/* Annotation rendering for horizontal-bar layouts. Walks the shared
 * annotation list once and draws "h" entries (value-axis) as vertical
 * lines through xrange, and "v" entries (category-axis) as horizontal
 * lines through fractional category indices. */
void fastchart_draw_horizontal_bar_annotations(fastchart_target_t *t, fastchart_obj *chart,
                                               const fastchart_rect *plot,
                                               const fastchart_palette *pal,
                                               const fastchart_value_range *xrange,
                                               int n_categories);

void fastchart_draw_v_annotations_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                              const fastchart_rect *plot,
                                              const fastchart_palette *pal,
                                              int n_categories);

void fastchart_draw_v_annotations_continuous(fastchart_target_t *t, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             const fastchart_palette *pal,
                                             const fastchart_value_range *xrange);

void fastchart_draw_v_annotations_time(fastchart_target_t *t, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       zend_long t_min, zend_long t_max);

/* Walk chart->config["text_annotations"] and render each free-floating
 * label at the stored canvas pixel coordinates. Per-entry color
 * defaults to pal->text. No-op if there are no annotations. */
void fastchart_draw_text_annotations(fastchart_target_t *t, fastchart_obj *chart,
                                     const fastchart_palette *pal);

#endif /* FASTCHART_AXIS_H */
