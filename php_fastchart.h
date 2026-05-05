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

/* Cached \GdImage class entry, resolved at MINIT via direct
 * CG(class_table) lookup. ext/gd defines gd_image_ce file-static
 * with no PHPAPI export so we cannot link against it; the autoload
 * path of zend_lookup_class is unsafe at MINIT. */
extern zend_class_entry *fastchart_gd_image_ce;

/* Shared object struct, one shape across all five chart subclasses.
 * Type-specific state is parked in the `data` and `config` zvals
 * during the setter calls; each draw() implementation reads back
 * what it expects from those slots. */
typedef struct _fastchart_obj {
    zend_long width;
    zend_long height;
    zend_long theme;
    zend_string *title;
    zend_string *font_path;
    double font_size;

    /* Per-instance palette overrides. -1 means "use theme default"
     * for color slots. series_colors_n is the count of overrides
     * (0..8); series_colors[i] is the override RGB for series i. */
    zend_long bg_override;
    zend_long plot_bg_override;
    int series_colors_n;
    int series_colors[8];

    /* Strict mode: when set, non-numeric data raises \TypeError on
     * draw() instead of being silently skipped. */
    bool strict;

    /* Legend placement, one of FASTCHART_LEGEND_*. */
    zend_long legend_position;

    /* Y-axis scale: 0 linear, 1 log10. */
    zend_long y_axis_scale;

    /* LineChart / ScatterChart marker overrides. -1 means default. */
    zend_long marker_style;   /* FASTCHART_MARKER_* */
    zend_long marker_size;    /* pixels */

    /* Axis titles. NULL when unset. */
    zend_string *x_axis_title;
    zend_string *y_axis_title;

    /* X-axis label rotation in degrees: 0, 45, or 90. */
    zend_long x_axis_label_angle;

    /* Forced Y-axis range. has_y_min / has_y_max / has_y_interval
     * track whether the value is meaningful (since 0.0 is a legal
     * forced bound). */
    bool has_y_min;
    bool has_y_max;
    bool has_y_interval;
    double y_min;
    double y_max;
    double y_interval;

    /* Secondary Y axis on the right side of the plot. */
    bool secondary_y;

    /* StockChart candle presentation style. */
    zend_long candle_style;

    /* PieChart label rendering mode + sprintf format string. */
    zend_long slice_label_position;
    zend_string *slice_label_format;

    /* AreaChart fill alpha (0..127, libgd convention). */
    zend_long area_alpha;

    /* Per-element color overrides. -1 = use theme palette. */
    zend_long axis_color_override;
    zend_long grid_color_override;
    zend_long border_color_override;
    zend_long text_color_override;

    /* Per-element font overrides. NULL path / 0.0 size = default. */
    zend_string *title_font_path;
    zend_string *axis_font_path;
    zend_string *label_font_path;
    double title_font_size;
    double axis_font_size;
    double label_font_size;

    /* Numeric value labels next to data points. */
    bool show_values;
    zend_string *value_format;

    /* PieChart "Other" slice aggregation threshold (percent of total). */
    double pie_other_threshold;
    zend_string *pie_other_label;

    /* Background features. */
    bool transparent_bg;
    zend_string *bg_image_path;

    /* Line interpolation mode (FASTCHART_INTERP_*). */
    zend_long line_interpolation;

    /* Hard plot rectangle. has_plot_rect = false means auto-layout. */
    bool has_plot_rect;
    int plot_x0, plot_y0, plot_x1, plot_y1;

    /* Border-side bitmask (FASTCHART_BORDER_*). Default ALL. */
    zend_long border_sides;

    /* Axis visibility. Both default true. */
    bool x_axis_visible;
    bool y_axis_visible;

    /* sprintf format strings for tick labels. NULL = auto-format. */
    zend_string *y_axis_label_format;
    zend_string *x_axis_label_format;

    /* Tick mode (FASTCHART_TICK_*). Default BOTH. */
    zend_long tick_mode;

    /* Bar fill width as percent of slot (1..100). 0 = use default. */
    zend_long bar_width_pct;

    /* Edge color for filled shapes. -1 = no outline. */
    zend_long edge_color;

    /* Render a horizontal shelf at y=0 when range crosses zero. */
    bool zero_shelf;

    /* X-axis label stride: render every Nth label. 1 = all. */
    zend_long x_label_stride;

    /* Secondary Y axis title. */
    zend_string *y_axis_title2;

    /* Thumbnail mode: shrink fonts and elide labels. */
    bool thumbnail_mode;

    /* BarChart stack mode (FASTCHART_STACK_*). Default SUM. */
    zend_long stack_mode;

    /* BarChart floating bars: each series entry is [min, max]. */
    bool bar_floating;

    /* Per-element text colors. -1 = fall through to text_color_override. */
    zend_long title_color;
    zend_long axis_label_color;
    zend_long axis_title_color;

    /* Per-axis font overrides. NULL path / 0 size = inherit. */
    zend_string *x_axis_font_path;
    zend_string *y_axis_font_path;
    zend_string *x_axis_title_font_path;
    zend_string *y_axis_title_font_path;
    zend_string *annotation_font_path;
    double x_axis_font_size;
    double y_axis_font_size;
    double x_axis_title_font_size;
    double y_axis_title_font_size;
    double annotation_font_size;

    zval data;
    zval config;
    zend_object std;
} fastchart_obj;

static inline fastchart_obj *fastchart_obj_from_zend(zend_object *obj) {
    return (fastchart_obj *)((char *)(obj) - XtOffsetOf(fastchart_obj, std));
}

#define Z_FASTCHART_OBJ_P(zv) fastchart_obj_from_zend(Z_OBJ_P(zv))

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

/* Forward-declare the only ext/gd public API we use (also declared in
 * fastchart.c at the call site). Mirrored verbatim from
 * ext/gd/php_gd.h since that header is not installed via make install. */
extern struct gdImageStruct *php_gd_libgdimageptr_from_zval_p(zval *zp);

/* Pull the underlying gdImagePtr out of the caller-supplied canvas
 * zval. NULL on failure; the caller throws. */
gdImagePtr fastchart_gd_image_from_zval(zval *canvas_zv);

/* Per-chart drawing helpers extracted from each ZEND_METHOD draw()
 * so the renderPng/Jpeg/Webp shortcuts can reuse them without
 * routing through ext/gd's zval extraction. Each returns 0 on
 * success, -1 on a draw-time error (a PHP exception is already
 * pending). */
int fastchart_line_render_to_image(fastchart_obj *self, gdImagePtr im);
int fastchart_area_render_to_image(fastchart_obj *self, gdImagePtr im);
int fastchart_bar_render_to_image(fastchart_obj *self, gdImagePtr im);
int fastchart_pie_render_to_image(fastchart_obj *self, gdImagePtr im);
int fastchart_scatter_render_to_image(fastchart_obj *self, gdImagePtr im);
int fastchart_stock_render_to_image(fastchart_obj *self, gdImagePtr im);

#endif /* PHP_FASTCHART_H */
