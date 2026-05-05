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

#ifndef PHP_FASTCHART_H
#define PHP_FASTCHART_H

#include "php.h"
#include <gd.h>

#define PHP_FASTCHART_VERSION "0.1.0"

extern zend_module_entry fastchart_module_entry;
#define phpext_fastchart_ptr &fastchart_module_entry

#ifdef PHP_WIN32
#define PHP_FASTCHART_API __declspec(dllexport)
#else
#define PHP_FASTCHART_API
#endif

/* PHP 8.3 compat shim for zend_register_internal_class_with_flags
 * (added in 8.4). gen_stub.php emits the 8.4+ variant for any
 * abstract/final class declaration, but we still target 8.3. */
#if PHP_VERSION_ID < 80400
static inline zend_class_entry *zend_register_internal_class_with_flags(
    zend_class_entry *class_entry,
    zend_class_entry *parent_ce,
    uint32_t flags)
{
    zend_class_entry *registered = zend_register_internal_class_ex(class_entry, parent_ce);
    registered->ce_flags |= flags;
    return registered;
}
#endif

extern zend_class_entry *fastchart_chart_ce;
extern zend_class_entry *fastchart_line_chart_ce;
extern zend_class_entry *fastchart_area_chart_ce;
extern zend_class_entry *fastchart_bar_chart_ce;
extern zend_class_entry *fastchart_pie_chart_ce;
extern zend_class_entry *fastchart_scatter_chart_ce;
extern zend_class_entry *fastchart_stock_chart_ce;
extern zend_class_entry *fastchart_radar_chart_ce;
extern zend_class_entry *fastchart_bubble_chart_ce;
extern zend_class_entry *fastchart_surface_chart_ce;
extern zend_class_entry *fastchart_gauge_chart_ce;
extern zend_class_entry *fastchart_gantt_chart_ce;
extern zend_class_entry *fastchart_box_plot_ce;
extern zend_class_entry *fastchart_polar_chart_ce;
extern zend_class_entry *fastchart_contour_chart_ce;

/* Cached \GdImage class entry, resolved at MINIT via direct
 * CG(class_table) lookup. ext/gd defines gd_image_ce file-static
 * with no PHPAPI export so we cannot link against it; the autoload
 * path of zend_lookup_class is unsafe at MINIT. */
extern zend_class_entry *fastchart_gd_image_ce;

/* Per-class object layout. Every chart subclass owns its own struct
 * laid out as { FASTCHART_BASE_FIELDS, <per-type fields>, zend_object std }
 * so per-class create/free/clone handlers can size and initialize the
 * exact memory their class needs. The shared FASTCHART_BASE_FIELDS
 * macro ensures every per-type struct presents the same field layout
 * at offset 0; that common-initial-sequence lets base setters and
 * shared helpers cast any per-type pointer to fastchart_obj* and
 * touch base fields by their natural names without the per-type-
 * setters needing a base accessor. The std member sits at the end of
 * each per-type struct, and each class registers its own
 * zend_object_handlers with offset = XtOffsetOf(class_struct, std)
 * so Z_FASTCHART_OBJ_P below lands on the start of the user struct
 * (= the base layout) regardless of which subclass we're in. */
#define FASTCHART_BASE_FIELDS \
    zend_long width; \
    zend_long height; \
    zend_long theme; \
    zend_string *title; \
    zend_string *font_path; \
    double font_size; \
    zend_long bg_override; \
    zend_long plot_bg_override; \
    int series_colors_n; \
    int series_colors[8]; \
    bool strict; \
    zend_long legend_position; \
    zend_long y_axis_scale; \
    zend_long marker_style; \
    zend_long marker_size; \
    zend_string *x_axis_title; \
    zend_string *y_axis_title; \
    zend_long x_axis_label_angle; \
    bool has_y_min; \
    bool has_y_max; \
    bool has_y_interval; \
    double y_min; \
    double y_max; \
    double y_interval; \
    bool secondary_y; \
    zend_long axis_color_override; \
    zend_long grid_color_override; \
    zend_long border_color_override; \
    zend_long text_color_override; \
    zend_string *title_font_path; \
    zend_string *axis_font_path; \
    zend_string *label_font_path; \
    double title_font_size; \
    double axis_font_size; \
    double label_font_size; \
    bool show_values; \
    zend_string *value_format; \
    bool transparent_bg; \
    zend_string *bg_image_path; \
    zend_long line_interpolation; \
    bool has_plot_rect; \
    int plot_x0; \
    int plot_y0; \
    int plot_x1; \
    int plot_y1; \
    zend_long border_sides; \
    bool x_axis_visible; \
    bool y_axis_visible; \
    zend_string *y_axis_label_format; \
    zend_string *x_axis_label_format; \
    zend_long tick_mode; \
    zend_long bar_width_pct; \
    zend_long edge_color; \
    bool zero_shelf; \
    zend_long x_label_stride; \
    zend_string *y_axis_title2; \
    bool thumbnail_mode; \
    zend_long title_color; \
    zend_long axis_label_color; \
    zend_long axis_title_color; \
    zend_long line_style; \
    zend_long gradient_from; \
    zend_long gradient_to; \
    zend_long gradient_dir; \
    bool has_drop_shadow; \
    zend_long shadow_dx; \
    zend_long shadow_dy; \
    zend_long shadow_color; \
    zend_long shadow_alpha; \
    zend_long color_ramp_low; \
    zend_long color_ramp_high; \
    zend_long date_axis_unit; \
    zend_long date_axis_every; \
    zval data; \
    zval config;

/* Base view type. fastchart_obj* is what base setters and shared
 * helpers receive. It deliberately omits zend_object std — concrete
 * instances always belong to one of the per-type structs below, and
 * the embedded std lives at the end of those. */
typedef struct _fastchart_obj { FASTCHART_BASE_FIELDS } fastchart_obj;

/* Shared series shape for the cartesian chart families (Line, Area,
 * Bar). Each series carries a parsed double array (NaN marks a gap),
 * an optional malloc'd label, optional per-point color overrides
 * (resolved at render time by gdImageColorAllocate), and for the bar
 * case an optional values_max array that turns the entries into
 * floating [min, max] ranges. */
typedef struct {
    char *label;          /* malloc'd, NUL-terminated; NULL = no label */
    double *values;       /* malloc'd, len entries; NaN = data gap */
    double *values_max;   /* malloc'd OR NULL; set on floating-bar series */
    zend_long *point_colors; /* malloc'd OR NULL; -1 = use series default */
    int len;
    bool right_axis;
} fastchart_series_t;

#define FASTCHART_MAX_SERIES 8

/* PieChart slice. label is owned (malloc'd, NUL-terminated) when the
 * caller supplied one; NULL means "use idx_label as a numeric
 * fallback". value is always > 0 (non-positive slices skip parsing).
 * color_rgb < 0 means "use the next palette color". */
typedef struct {
    char *label;
    char  idx_label[16];
    double value;
    int color_rgb;
} fastchart_pie_slice;

#define FASTCHART_MAX_SLICES 32

/* ScatterChart point. series_idx selects the per-series palette
 * color when color_rgb is -1; href and tooltip are owned strings used
 * by getImageMap to emit clickable areas. */
typedef struct {
    double x;
    double y;
    int series_idx;       /* 0..MAX_SCATTER_SERIES-1 */
    int color_rgb;        /* -1 = use palette */
    char *href;           /* owned, may be NULL */
    char *tooltip;        /* owned, may be NULL */
} fastchart_scatter_point;

#define FASTCHART_MAX_SCATTER_POINTS 4096
#define FASTCHART_MAX_SCATTER_SERIES 8

/* BubbleChart entry. */
typedef struct {
    double x;
    double y;
    double size;
    int color_rgb;       /* -1 = use palette */
} fastchart_bubble_point;

#define FASTCHART_MAX_BUBBLE_POINTS 4096

/* BoxPlot entry. five-number summary plus optional outliers. */
typedef struct {
    char *label;          /* owned */
    double min, q1, median, q3, max;
    double *outliers;     /* malloc'd OR NULL */
    int outlier_count;
} fastchart_boxplot_entry;

#define FASTCHART_MAX_BOXPLOT_ENTRIES 32

/* PolarChart series: list of (angle_deg, radius) points plus optional
 * label / color override. */
typedef struct {
    double *angles;       /* malloc'd, len entries */
    double *radii;        /* malloc'd, len entries */
    int len;
    char *label;          /* owned */
    int color_rgb;        /* -1 = use palette */
} fastchart_polar_series;

#define FASTCHART_MAX_POLAR_SERIES 8

/* RadarChart series: parallel array of values per axis. */
typedef struct {
    double *values;       /* malloc'd, len entries */
    int len;
    char *label;          /* owned */
    int color_rgb;        /* -1 = use palette */
} fastchart_radar_series;

#define FASTCHART_MAX_RADAR_SERIES 8

/* GanttChart task / milestone. */
typedef struct {
    char *name;           /* owned */
    zend_long start;
    zend_long end;        /* same as start for milestones */
    int color_rgb;        /* -1 = use palette */
    bool is_milestone;
    int *deps;            /* malloc'd OR NULL; indices into the task array */
    int n_deps;
} fastchart_gantt_task;

#define FASTCHART_MAX_GANTT_TASKS 64

/* SurfaceChart / ContourChart 2D grid. Stored row-major. */
typedef struct {
    double *cells;        /* malloc'd, rows*cols, NaN = missing */
    int rows;
    int cols;
} fastchart_grid;

/* Per-type structs. Each adds the class-specific fields (or none)
 * plus the zend_object std at the end. */
typedef struct {
    FASTCHART_BASE_FIELDS
    fastchart_series_t series[FASTCHART_MAX_SERIES];
    int n_series;
    int max_len;
    zend_object std;
} fastchart_line_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    zend_long area_alpha;
    fastchart_series_t series[FASTCHART_MAX_SERIES];
    int n_series;
    int max_len;
    zend_object std;
} fastchart_area_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    zend_long stack_mode;
    bool bar_floating;
    fastchart_series_t series[FASTCHART_MAX_SERIES];
    int n_series;
    int max_len;
    zend_object std;
} fastchart_bar_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    zend_long slice_label_position;
    zend_string *slice_label_format;
    double pie_other_threshold;
    zend_string *pie_other_label;
    fastchart_pie_slice *slices;       /* owned */
    int slice_count;
    double total;
    double donut_hole_ratio;
    zend_long *explode;                /* owned, parallel to slices; 0 = no offset */
    int explode_count;
    zend_object std;
} fastchart_pie_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    bool trend_line;
    zend_long trend_line_color;
    zend_long trend_degree;
    fastchart_scatter_point *points;        /* owned */
    int point_count;
    char *series_labels[FASTCHART_MAX_SCATTER_SERIES]; /* owned */
    int n_series;
    /* error_bars storage shared with LineChart's config zval for now;
     * a future migration would land here as well. */
    zend_object std;
} fastchart_scatter_obj;

/* StockChart parsed-data shapes. setOhlcv() parses user rows into a
 * fastchart_candle array, sorts by timestamp, and stores it on the
 * stock obj. setMovingAverages / setVolumePane / setVolumeColors /
 * addIndicatorPane similarly store typed C state instead of stuffing
 * into the generic config zval. */
typedef struct {
    zend_long ts;
    double open;
    double high;
    double low;
    double close;
    double volume;
    int has_volume;
} fastchart_candle;

typedef struct {
    char *name;            /* malloc'd, NUL-terminated; NULL = empty slot */
    double *values;        /* malloc'd; non-numeric input becomes NaN */
    int value_count;
    bool has_color;
    int color_rgb;
    bool has_reference;
    double reference;
    bool has_min;
    double min;
    bool has_max;
    double max;
} fastchart_indicator_pane;

#define FASTCHART_MAX_CANDLES         4096
#define FASTCHART_MAX_SMA             8
#define FASTCHART_MAX_INDICATOR_PANES 3

typedef struct {
    FASTCHART_BASE_FIELDS
    zend_long candle_style;
    fastchart_candle *candles;          /* malloc'd, owned */
    int candle_count;
    bool any_volume;
    bool volume_pane;
    int *volume_colors;                 /* malloc'd, parallel to candles up to volume_colors_count; -1 = use up/down default */
    int volume_colors_count;
    int sma_periods[FASTCHART_MAX_SMA];
    int sma_count;
    fastchart_indicator_pane indicator_panes[FASTCHART_MAX_INDICATOR_PANES];
    int indicator_pane_count;
    zend_object std;
} fastchart_stock_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    double radar_max;
    bool radar_filled;
    fastchart_radar_series series[FASTCHART_MAX_RADAR_SERIES];
    int n_series;
    char **categories;                 /* malloc'd, n_categories entries; each owned */
    int n_categories;
    zend_object std;
} fastchart_radar_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    fastchart_bubble_point *points;    /* owned */
    int point_count;
    zend_object std;
} fastchart_bubble_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    bool surface_show_values;
    zend_string *surface_value_format;
    fastchart_grid grid;
    zend_object std;
} fastchart_surface_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    double gauge_value;
    double gauge_min;
    double gauge_max;
    zend_string *gauge_value_format;
    zend_object std;
} fastchart_gauge_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    bool gantt_show_labels;
    bool gantt_has_range;
    zend_long gantt_range_start;
    zend_long gantt_range_end;
    fastchart_gantt_task *tasks;       /* owned */
    int task_count;
    zend_object std;
} fastchart_gantt_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    zend_long box_width_pct;
    fastchart_boxplot_entry *entries;  /* owned */
    int entry_count;
    zend_object std;
} fastchart_boxplot_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    double polar_max_radius;
    bool polar_filled;
    fastchart_polar_series series[FASTCHART_MAX_POLAR_SERIES];
    int n_series;
    zend_object std;
} fastchart_polar_obj;

typedef struct {
    FASTCHART_BASE_FIELDS
    bool contour_filled;
    fastchart_grid grid;
    double *levels;                    /* owned, level_count entries */
    int level_count;
    zend_object std;
} fastchart_contour_obj;

/* Walk back from zend_object* to the start of the containing per-type
 * struct using each class's handlers->offset. Cast to fastchart_obj*
 * is the common-initial-sequence access — base fields land at the
 * same offsets in every per-type struct. */
static inline fastchart_obj *fastchart_obj_from_zend(zend_object *obj) {
    return (fastchart_obj *)((char *)(obj) - obj->handlers->offset);
}

#define Z_FASTCHART_OBJ_P(zv)         fastchart_obj_from_zend(Z_OBJ_P(zv))
#define Z_FASTCHART_LINE_OBJ_P(zv)    ((fastchart_line_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_AREA_OBJ_P(zv)    ((fastchart_area_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_BAR_OBJ_P(zv)     ((fastchart_bar_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_PIE_OBJ_P(zv)     ((fastchart_pie_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_SCATTER_OBJ_P(zv) ((fastchart_scatter_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_STOCK_OBJ_P(zv)   ((fastchart_stock_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_RADAR_OBJ_P(zv)   ((fastchart_radar_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_BUBBLE_OBJ_P(zv)  ((fastchart_bubble_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_SURFACE_OBJ_P(zv) ((fastchart_surface_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_GAUGE_OBJ_P(zv)   ((fastchart_gauge_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_GANTT_OBJ_P(zv)   ((fastchart_gantt_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_BOXPLOT_OBJ_P(zv) ((fastchart_boxplot_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_POLAR_OBJ_P(zv)   ((fastchart_polar_obj *)Z_FASTCHART_OBJ_P(zv))
#define Z_FASTCHART_CONTOUR_OBJ_P(zv) ((fastchart_contour_obj *)Z_FASTCHART_OBJ_P(zv))

#define FASTCHART_DEFAULT_WIDTH      800
#define FASTCHART_DEFAULT_HEIGHT     600
#define FASTCHART_DEFAULT_FONT_SIZE  10.0

#define FASTCHART_THEME_LIGHT 0
#define FASTCHART_THEME_DARK  1

/* Marker styles. Match the const ints in fastchart.stub.php. */
#define FASTCHART_MARKER_NONE    0
#define FASTCHART_MARKER_CIRCLE  1
#define FASTCHART_MARKER_SQUARE  2
#define FASTCHART_MARKER_DIAMOND 3
#define FASTCHART_MARKER_CROSS   4
#define FASTCHART_MARKER_PLUS    5

/* Legend placement. */
#define FASTCHART_LEGEND_NONE         0
#define FASTCHART_LEGEND_TOP_RIGHT    1
#define FASTCHART_LEGEND_TOP_LEFT     2
#define FASTCHART_LEGEND_BOTTOM_RIGHT 3
#define FASTCHART_LEGEND_BOTTOM_LEFT  4

/* Y-axis scale. */
#define FASTCHART_SCALE_LINEAR 0
#define FASTCHART_SCALE_LOG    1

/* PieChart label position. */
#define FASTCHART_LABEL_NONE    0
#define FASTCHART_LABEL_INSIDE  1
#define FASTCHART_LABEL_OUTSIDE 2

/* StockChart OHLC presentation style. */
#define FASTCHART_STYLE_CANDLE  0
#define FASTCHART_STYLE_BAR     1
#define FASTCHART_STYLE_DIAMOND 2
#define FASTCHART_STYLE_I_CAP   3
#define FASTCHART_STYLE_HOLLOW  4
#define FASTCHART_STYLE_VOLUME  5
#define FASTCHART_STYLE_VECTOR  6

/* Border side bitmask. */
#define FASTCHART_BORDER_NONE   0
#define FASTCHART_BORDER_LEFT   1
#define FASTCHART_BORDER_RIGHT  2
#define FASTCHART_BORDER_TOP    4
#define FASTCHART_BORDER_BOTTOM 8
#define FASTCHART_BORDER_ALL    15

/* Line interpolation. */
#define FASTCHART_INTERP_LINEAR 0
#define FASTCHART_INTERP_SMOOTH 1

/* Tick mode bitmask: bit0 = labels, bit1 = points. */
#define FASTCHART_TICK_NONE   0
#define FASTCHART_TICK_LABELS 1
#define FASTCHART_TICK_POINTS 2
#define FASTCHART_TICK_BOTH   3

/* BarChart stack mode. */
#define FASTCHART_STACK_SUM    0
#define FASTCHART_STACK_BESIDE 1
#define FASTCHART_STACK_LAYER  2

/* Pie label position extends LABEL_INSIDE/OUTSIDE/NONE. */
#define FASTCHART_LABEL_LEFT  3
#define FASTCHART_LABEL_RIGHT 4

/* Line dash styles. */
#define FASTCHART_LINE_SOLID  0
#define FASTCHART_LINE_DASHED 1
#define FASTCHART_LINE_DOTTED 2

/* Gradient direction. */
#define FASTCHART_GRADIENT_VERTICAL   0
#define FASTCHART_GRADIENT_HORIZONTAL 1

/* Calendar-aware date-axis stride units. */
#define FASTCHART_DATE_DAY     0
#define FASTCHART_DATE_WEEK    1
#define FASTCHART_DATE_MONTH   2
#define FASTCHART_DATE_QUARTER 3
#define FASTCHART_DATE_YEAR    4

/* Forward-declare the only ext/gd public API we use (also declared in
 * fastchart.c at the call site). Mirrored verbatim from
 * ext/gd/php_gd.h since that header is not installed via make install. */
extern struct gdImageStruct *php_gd_libgdimageptr_from_zval_p(zval *zp);

/* Pull the underlying gdImagePtr out of the caller-supplied canvas
 * zval. NULL on failure; the caller throws. The IS_OBJECT pre-check
 * is redundant against ext/gd's own type validation but avoids a
 * call into ext/gd when the value is obviously wrong (e.g. NULL
 * after a Z_PARAM_OBJECT_OF_CLASS that the caller forgot to
 * RETURN_THROWS on). */
static inline gdImagePtr fastchart_gd_image_from_zval(zval *canvas_zv)
{
    if (Z_TYPE_P(canvas_zv) != IS_OBJECT) return NULL;
    return (gdImagePtr)php_gd_libgdimageptr_from_zval_p(canvas_zv);
}

/* Read a string label from an array-shaped setter, dropping the
 * value if it carries an embedded NUL. Public scalar setters reject
 * embedded NUL with ValueError; per-element strings inside arrays
 * (series labels, slice labels, gantt task names, category labels,
 * overlay labels, etc.) take the silent-drop path because rejecting
 * them with an exception would force every chart-type setter to
 * walk the whole input array up front. The render-vs-stored
 * divergence (gdImageStringFT truncates at \0, PHP keeps the full
 * length) is what we're guarding against. */
static inline const char *fastchart_label_or_null(const zval *zv)
{
    if (!zv || Z_TYPE_P(zv) != IS_STRING) return NULL;
    if (memchr(Z_STRVAL_P(zv), 0, Z_STRLEN_P(zv)) != NULL) return NULL;
    return Z_STRVAL_P(zv);
}

/* Per-chart drawing helpers extracted from each ZEND_METHOD draw()
 * so the renderPng/Jpeg/Webp shortcuts can reuse them without
 * routing through ext/gd's zval extraction. Each returns 0 on
 * success, -1 on a draw-time error (a PHP exception is already
 * pending). */
int fastchart_line_render_to_image(fastchart_line_obj *self, gdImagePtr im);
int fastchart_area_render_to_image(fastchart_area_obj *self, gdImagePtr im);
int fastchart_bar_render_to_image(fastchart_bar_obj *self, gdImagePtr im);
int fastchart_pie_render_to_image(fastchart_pie_obj *self, gdImagePtr im);
int fastchart_scatter_render_to_image(fastchart_scatter_obj *self, gdImagePtr im);
int fastchart_stock_render_to_image(fastchart_stock_obj *self, gdImagePtr im);
int fastchart_radar_render_to_image(fastchart_radar_obj *self, gdImagePtr im);
int fastchart_bubble_render_to_image(fastchart_bubble_obj *self, gdImagePtr im);
int fastchart_surface_render_to_image(fastchart_surface_obj *self, gdImagePtr im);
int fastchart_gauge_render_to_image(fastchart_gauge_obj *self, gdImagePtr im);
int fastchart_gantt_render_to_image(fastchart_gantt_obj *self, gdImagePtr im);
int fastchart_boxplot_render_to_image(fastchart_boxplot_obj *self, gdImagePtr im);
int fastchart_polar_render_to_image(fastchart_polar_obj *self, gdImagePtr im);
int fastchart_contour_render_to_image(fastchart_contour_obj *self, gdImagePtr im);

#endif /* PHP_FASTCHART_H */
