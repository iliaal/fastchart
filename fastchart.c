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
#include "php_ini.h"
#include "php_streams.h"
#include "ext/standard/info.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_smart_str.h"

#include <limits.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include "php_fastchart.h"
#include "fastchart_arginfo.h"

zend_class_entry *fastchart_chart_ce;
zend_class_entry *fastchart_line_chart_ce;
zend_class_entry *fastchart_area_chart_ce;
zend_class_entry *fastchart_bar_chart_ce;
zend_class_entry *fastchart_pie_chart_ce;
zend_class_entry *fastchart_scatter_chart_ce;
zend_class_entry *fastchart_stock_chart_ce;
zend_class_entry *fastchart_radar_chart_ce;
zend_class_entry *fastchart_bubble_chart_ce;
zend_class_entry *fastchart_surface_chart_ce;
zend_class_entry *fastchart_gauge_chart_ce;
zend_class_entry *fastchart_gantt_chart_ce;
zend_class_entry *fastchart_box_plot_ce;
zend_class_entry *fastchart_polar_chart_ce;
zend_class_entry *fastchart_contour_chart_ce;
zend_class_entry *fastchart_gd_image_ce = NULL;

/* Auto-detected default font path. Probed at MINIT, used as the
 * initial font_path on every newly-allocated chart instance. NULL
 * if no probe candidate exists on the system; users must then call
 * setFontPath() before any text-rendering chart method. */
static zend_string *fastchart_default_font_path = NULL;

/* Per-class object_handlers. Each chart class's std offset varies
 * because per-type fields shift std's position within its struct.
 * Holding a separate handlers struct per class lets the shared
 * Z_FASTCHART_OBJ_P walk back from any zend_object* via
 * obj->handlers->offset to land on the start of the user struct
 * (= the common-initial-sequence base layout) regardless of which
 * subclass we're in. */
static zend_object_handlers fastchart_line_handlers;
static zend_object_handlers fastchart_area_handlers;
static zend_object_handlers fastchart_bar_handlers;
static zend_object_handlers fastchart_pie_handlers;
static zend_object_handlers fastchart_scatter_handlers;
static zend_object_handlers fastchart_stock_handlers;
static zend_object_handlers fastchart_radar_handlers;
static zend_object_handlers fastchart_bubble_handlers;
static zend_object_handlers fastchart_surface_handlers;
static zend_object_handlers fastchart_gauge_handlers;
static zend_object_handlers fastchart_gantt_handlers;
static zend_object_handlers fastchart_boxplot_handlers;
static zend_object_handlers fastchart_polar_handlers;
static zend_object_handlers fastchart_contour_handlers;

/* Base lifecycle. Operates on the common-initial-sequence layout —
 * any fastchart_X_obj* aliases as fastchart_obj* for these reads /
 * writes since base fields share offsets across all per-type structs. */
static void fastchart_base_init_defaults(fastchart_obj *b)
{
    b->width  = FASTCHART_DEFAULT_WIDTH;
    b->height = FASTCHART_DEFAULT_HEIGHT;
    b->theme  = FASTCHART_THEME_LIGHT;
    b->title  = ZSTR_EMPTY_ALLOC();
    b->font_size = FASTCHART_DEFAULT_FONT_SIZE;

    b->bg_override = -1;
    b->plot_bg_override = -1;
    b->series_colors_n = 0;
    for (int i = 0; i < 8; i++) b->series_colors[i] = -1;

    b->strict = false;
    b->legend_position = FASTCHART_LEGEND_TOP_RIGHT;
    b->y_axis_scale = FASTCHART_SCALE_LINEAR;
    b->marker_style = -1;
    b->marker_size = -1;

    b->x_axis_title = NULL;
    b->y_axis_title = NULL;
    b->x_axis_label_angle = 0;

    b->has_y_min = false;
    b->has_y_max = false;
    b->has_y_interval = false;
    b->y_min = 0;
    b->y_max = 0;
    b->y_interval = 0;

    b->secondary_y = false;

    b->axis_color_override   = -1;
    b->grid_color_override   = -1;
    b->border_color_override = -1;
    b->text_color_override   = -1;

    b->title_font_path = NULL;
    b->axis_font_path  = NULL;
    b->label_font_path = NULL;
    b->title_font_size = 0.0;
    b->axis_font_size  = 0.0;
    b->label_font_size = 0.0;

    b->show_values = false;
    b->value_format = NULL;

    b->transparent_bg = false;
    b->bg_image_path = NULL;

    b->line_interpolation = FASTCHART_INTERP_LINEAR;

    b->has_plot_rect = false;
    b->plot_x0 = b->plot_y0 = b->plot_x1 = b->plot_y1 = 0;

    b->border_sides = FASTCHART_BORDER_ALL;

    b->x_axis_visible = true;
    b->y_axis_visible = true;
    b->y_axis_label_format = NULL;
    b->x_axis_label_format = NULL;
    b->tick_mode = FASTCHART_TICK_BOTH;
    b->bar_width_pct = 0;
    b->edge_color = -1;
    b->zero_shelf = false;
    b->x_label_stride = 1;
    b->y_axis_title2 = NULL;
    b->thumbnail_mode = false;
    b->title_color = -1;
    b->axis_label_color = -1;
    b->axis_title_color = -1;

    b->line_style = FASTCHART_LINE_SOLID;
    b->gradient_from = -1;
    b->gradient_to = -1;
    b->gradient_dir = FASTCHART_GRADIENT_VERTICAL;
    b->has_drop_shadow = false;
    b->shadow_dx = 3;
    b->shadow_dy = 3;
    b->shadow_color = 0x000000;
    b->shadow_alpha = 64;
    b->color_ramp_low = 0x1f77b4;
    b->color_ramp_high = 0xd62728;
    b->date_axis_unit = FASTCHART_DATE_DAY;
    b->date_axis_every = 0;

    b->font_path = fastchart_default_font_path
        ? zend_string_copy(fastchart_default_font_path) : NULL;

    array_init(&b->data);
    array_init(&b->config);
}

#define FASTCHART_BASE_OWNED_STR(F) \
    F(title) F(font_path) F(x_axis_title) F(y_axis_title) \
    F(title_font_path) F(axis_font_path) F(label_font_path) \
    F(value_format) F(bg_image_path) F(y_axis_label_format) \
    F(x_axis_label_format) F(y_axis_title2)

static void fastchart_base_release_owned(fastchart_obj *b)
{
#define FC_RELEASE(field) if (b->field) zend_string_release(b->field);
    FASTCHART_BASE_OWNED_STR(FC_RELEASE)
#undef FC_RELEASE
    zval_ptr_dtor(&b->data);
    zval_ptr_dtor(&b->config);
}

static void fastchart_base_addref_owned(fastchart_obj *b)
{
#define FC_ADDREF(field) if (b->field) zend_string_addref(b->field);
    FASTCHART_BASE_OWNED_STR(FC_ADDREF)
#undef FC_ADDREF
    Z_TRY_ADDREF(b->data);
    Z_TRY_ADDREF(b->config);
}

/* Per-class init / release / clone-addref helpers. No-ops for
 * classes with empty per-type segments. */
static void fastchart_line_init_extras(fastchart_line_obj *o)        { (void)o; }
static void fastchart_line_release_extras(fastchart_line_obj *o)     { (void)o; }
static void fastchart_line_addref_extras(fastchart_line_obj *o)      { (void)o; }

static void fastchart_area_init_extras(fastchart_area_obj *o)        { o->area_alpha = 64; }
static void fastchart_area_release_extras(fastchart_area_obj *o)     { (void)o; }
static void fastchart_area_addref_extras(fastchart_area_obj *o)      { (void)o; }

static void fastchart_bar_init_extras(fastchart_bar_obj *o)
{
    o->stack_mode = FASTCHART_STACK_SUM;
    o->bar_floating = false;
}
static void fastchart_bar_release_extras(fastchart_bar_obj *o)       { (void)o; }
static void fastchart_bar_addref_extras(fastchart_bar_obj *o)        { (void)o; }

static void fastchart_pie_init_extras(fastchart_pie_obj *o)
{
    o->slice_label_position = FASTCHART_LABEL_INSIDE;
    o->slice_label_format = NULL;
    o->pie_other_threshold = 0.0;
    o->pie_other_label = NULL;
}
static void fastchart_pie_release_extras(fastchart_pie_obj *o)
{
    if (o->slice_label_format) zend_string_release(o->slice_label_format);
    if (o->pie_other_label)    zend_string_release(o->pie_other_label);
}
static void fastchart_pie_addref_extras(fastchart_pie_obj *o)
{
    if (o->slice_label_format) zend_string_addref(o->slice_label_format);
    if (o->pie_other_label)    zend_string_addref(o->pie_other_label);
}

static void fastchart_scatter_init_extras(fastchart_scatter_obj *o)
{
    o->trend_line = false;
    o->trend_line_color = -1;
    o->trend_degree = 1;
}
static void fastchart_scatter_release_extras(fastchart_scatter_obj *o) { (void)o; }
static void fastchart_scatter_addref_extras(fastchart_scatter_obj *o)  { (void)o; }

static void fastchart_stock_init_extras(fastchart_stock_obj *o)        { o->candle_style = FASTCHART_STYLE_CANDLE; }
static void fastchart_stock_release_extras(fastchart_stock_obj *o)     { (void)o; }
static void fastchart_stock_addref_extras(fastchart_stock_obj *o)      { (void)o; }

static void fastchart_radar_init_extras(fastchart_radar_obj *o)
{
    o->radar_max = 0.0;
    o->radar_filled = true;
}
static void fastchart_radar_release_extras(fastchart_radar_obj *o)     { (void)o; }
static void fastchart_radar_addref_extras(fastchart_radar_obj *o)      { (void)o; }

static void fastchart_bubble_init_extras(fastchart_bubble_obj *o)      { (void)o; }
static void fastchart_bubble_release_extras(fastchart_bubble_obj *o)   { (void)o; }
static void fastchart_bubble_addref_extras(fastchart_bubble_obj *o)    { (void)o; }

static void fastchart_surface_init_extras(fastchart_surface_obj *o)
{
    o->surface_show_values = false;
    o->surface_value_format = NULL;
}
static void fastchart_surface_release_extras(fastchart_surface_obj *o)
{
    if (o->surface_value_format) zend_string_release(o->surface_value_format);
}
static void fastchart_surface_addref_extras(fastchart_surface_obj *o)
{
    if (o->surface_value_format) zend_string_addref(o->surface_value_format);
}

static void fastchart_gauge_init_extras(fastchart_gauge_obj *o)
{
    o->gauge_value = 0.0;
    o->gauge_min = 0.0;
    o->gauge_max = 100.0;
    o->gauge_value_format = NULL;
}
static void fastchart_gauge_release_extras(fastchart_gauge_obj *o)
{
    if (o->gauge_value_format) zend_string_release(o->gauge_value_format);
}
static void fastchart_gauge_addref_extras(fastchart_gauge_obj *o)
{
    if (o->gauge_value_format) zend_string_addref(o->gauge_value_format);
}

static void fastchart_gantt_init_extras(fastchart_gantt_obj *o)
{
    o->gantt_show_labels = true;
    o->gantt_has_range = false;
    o->gantt_range_start = 0;
    o->gantt_range_end = 0;
}
static void fastchart_gantt_release_extras(fastchart_gantt_obj *o)     { (void)o; }
static void fastchart_gantt_addref_extras(fastchart_gantt_obj *o)      { (void)o; }

static void fastchart_boxplot_init_extras(fastchart_boxplot_obj *o)    { o->box_width_pct = 60; }
static void fastchart_boxplot_release_extras(fastchart_boxplot_obj *o) { (void)o; }
static void fastchart_boxplot_addref_extras(fastchart_boxplot_obj *o)  { (void)o; }

static void fastchart_polar_init_extras(fastchart_polar_obj *o)
{
    o->polar_max_radius = 0.0;
    o->polar_filled = true;
}
static void fastchart_polar_release_extras(fastchart_polar_obj *o)     { (void)o; }
static void fastchart_polar_addref_extras(fastchart_polar_obj *o)      { (void)o; }

static void fastchart_contour_init_extras(fastchart_contour_obj *o)    { o->contour_filled = true; }
static void fastchart_contour_release_extras(fastchart_contour_obj *o) { (void)o; }
static void fastchart_contour_addref_extras(fastchart_contour_obj *o)  { (void)o; }

/* Generates the create / free / clone trio for one chart class.
 * The handlers struct must already exist in static scope; MINIT
 * memcpy's std_object_handlers into it and sets offset / dtor. */
#define FASTCHART_DEFINE_LIFECYCLE(name, T)                                      \
    static zend_object *fastchart_##name##_create_object(zend_class_entry *ce)   \
    {                                                                            \
        T *intern = zend_object_alloc(sizeof(T), ce);                            \
        zend_object_std_init(&intern->std, ce);                                  \
        object_properties_init(&intern->std, ce);                                \
        fastchart_base_init_defaults((fastchart_obj *)intern);                   \
        fastchart_##name##_init_extras(intern);                                  \
        intern->std.handlers = &fastchart_##name##_handlers;                     \
        return &intern->std;                                                     \
    }                                                                            \
    static void fastchart_##name##_free_object(zend_object *object)              \
    {                                                                            \
        T *intern = (T *)((char *)object - fastchart_##name##_handlers.offset);  \
        fastchart_base_release_owned((fastchart_obj *)intern);                   \
        fastchart_##name##_release_extras(intern);                               \
        zend_object_std_dtor(&intern->std);                                      \
    }                                                                            \
    static zend_object *fastchart_##name##_clone_object(zend_object *src_obj)    \
    {                                                                            \
        T *src = (T *)((char *)src_obj - fastchart_##name##_handlers.offset);    \
        zend_object *dst_obj = fastchart_##name##_create_object(src_obj->ce);    \
        T *dst = (T *)((char *)dst_obj - fastchart_##name##_handlers.offset);    \
        fastchart_base_release_owned((fastchart_obj *)dst);                      \
        fastchart_##name##_release_extras(dst);                                  \
        memcpy(dst, src, offsetof(T, std));                                      \
        fastchart_base_addref_owned((fastchart_obj *)dst);                       \
        fastchart_##name##_addref_extras(dst);                                   \
        zend_objects_clone_members(&dst->std, &src->std);                        \
        return &dst->std;                                                        \
    }

FASTCHART_DEFINE_LIFECYCLE(line,    fastchart_line_obj)
FASTCHART_DEFINE_LIFECYCLE(area,    fastchart_area_obj)
FASTCHART_DEFINE_LIFECYCLE(bar,     fastchart_bar_obj)
FASTCHART_DEFINE_LIFECYCLE(pie,     fastchart_pie_obj)
FASTCHART_DEFINE_LIFECYCLE(scatter, fastchart_scatter_obj)
FASTCHART_DEFINE_LIFECYCLE(stock,   fastchart_stock_obj)
FASTCHART_DEFINE_LIFECYCLE(radar,   fastchart_radar_obj)
FASTCHART_DEFINE_LIFECYCLE(bubble,  fastchart_bubble_obj)
FASTCHART_DEFINE_LIFECYCLE(surface, fastchart_surface_obj)
FASTCHART_DEFINE_LIFECYCLE(gauge,   fastchart_gauge_obj)
FASTCHART_DEFINE_LIFECYCLE(gantt,   fastchart_gantt_obj)
FASTCHART_DEFINE_LIFECYCLE(boxplot, fastchart_boxplot_obj)
FASTCHART_DEFINE_LIFECYCLE(polar,   fastchart_polar_obj)
FASTCHART_DEFINE_LIFECYCLE(contour, fastchart_contour_obj)

/* Common locations for a sans-serif TTF that ships by default on the
 * platforms PIE supports. Probed in order; the first existing path
 * becomes the auto-detected default. setFontPath() overrides per
 * instance. The list is intentionally short -- adding a path here is
 * an ABI-stable choice the extension makes about which install layout
 * to assume.
 *
 *   Debian / Ubuntu:    /usr/share/fonts/truetype/dejavu/...
 *   Fedora / RHEL:      /usr/share/fonts/dejavu-sans-fonts/...
 *   Arch:               /usr/share/fonts/TTF/...
 *   Alpine:             /usr/share/fonts/TTF/...
 *   macOS:              /Library/Fonts/... (system) or /System/Library/Fonts/... */
static const char *DEFAULT_FONT_CANDIDATES[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf",
    "/Library/Fonts/Arial.ttf",
    "/System/Library/Fonts/Helvetica.ttc",
    NULL,
};

static zend_string *fastchart_probe_default_font(void)
{
    struct stat st;
    for (int i = 0; DEFAULT_FONT_CANDIDATES[i]; i++) {
        if (stat(DEFAULT_FONT_CANDIDATES[i], &st) == 0 && S_ISREG(st.st_mode)) {
            return zend_string_init(DEFAULT_FONT_CANDIDATES[i],
                                    strlen(DEFAULT_FONT_CANDIDATES[i]),
                                    1 /* persistent */);
        }
    }
    return NULL;
}

ZEND_METHOD(FastChart_Chart, __construct)
{
    zend_long width = 0, height = 0;
    bool width_is_null = true, height_is_null = true;

    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(width, width_is_null)
        Z_PARAM_LONG_OR_NULL(height, height_is_null)
    ZEND_PARSE_PARAMETERS_END();

    /* If only one of width/height is given, treat as user error
     * rather than auto-defaulting one and not the other -- better
     * to surface the asymmetry than silently produce a chart of
     * unexpected dimensions. */
    if (width_is_null != height_is_null) {
        zend_value_error("FastChart\\Chart::__construct() requires both width and height, or neither");
        RETURN_THROWS();
    }
    if (width_is_null) {
        return; /* keep create_object defaults */
    }

    if (width <= 0 || height <= 0) {
        zend_value_error("FastChart\\Chart::__construct() requires positive dimensions");
        RETURN_THROWS();
    }
    if (width > 65535 || height > 65535) {
        zend_value_error("FastChart\\Chart::__construct() dimensions must fit in 16 bits");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->width  = width;
    self->height = height;
}

ZEND_METHOD(FastChart_Chart, version)
{
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_STRING(PHP_FASTCHART_VERSION);
}

ZEND_METHOD(FastChart_Chart, setSize)
{
    zend_long width, height;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(width)
        Z_PARAM_LONG(height)
    ZEND_PARSE_PARAMETERS_END();

    if (width <= 0 || height <= 0) {
        zend_value_error("FastChart\\Chart::setSize() requires positive dimensions");
        RETURN_THROWS();
    }
    if (width > 65535 || height > 65535) {
        zend_value_error("FastChart\\Chart::setSize() dimensions must fit in 16 bits");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->width  = width;
    self->height = height;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setTitle)
{
    zend_string *title;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(title)
    ZEND_PARSE_PARAMETERS_END();

    /* Reject embedded NUL: gdImageStringFT silently truncates at \0
     * while the stored zend_string keeps its full length, so
     * consumers see one length and the rendered image shows another. */
    if (memchr(ZSTR_VAL(title), 0, ZSTR_LEN(title)) != NULL) {
        zend_value_error("FastChart\\Chart::setTitle() title contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->title) {
        zend_string_release(self->title);
    }
    self->title = zend_string_copy(title);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setTheme)
{
    zend_long theme;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(theme)
    ZEND_PARSE_PARAMETERS_END();

    if (theme != FASTCHART_THEME_LIGHT && theme != FASTCHART_THEME_DARK) {
        zend_value_error("FastChart\\Chart::setTheme() expects a THEME_* class constant");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->theme = theme;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setFontPath)
{
    zend_string *path;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(path)
    ZEND_PARSE_PARAMETERS_END();

    if (ZSTR_LEN(path) == 0) {
        zend_value_error("FastChart\\Chart::setFontPath() requires a non-empty path");
        RETURN_THROWS();
    }
    /* Embedded NUL gates: this path will travel into FreeType's stat()
     * via libgd; reject before it gets there so we don't open a path
     * the user cannot have intended. */
    if (memchr(ZSTR_VAL(path), 0, ZSTR_LEN(path)) != NULL) {
        zend_value_error("FastChart\\Chart::setFontPath() path contains an embedded NUL");
        RETURN_THROWS();
    }
    if (php_check_open_basedir(ZSTR_VAL(path))) {
        /* php_check_open_basedir already raised a warning. Don't throw,
         * just no-op the setter so callers under open_basedir are not
         * forced to wrap every set in a try/catch. */
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->font_path) {
        zend_string_release(self->font_path);
    }
    self->font_path = zend_string_copy(path);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setFontSize)
{
    double size;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_DOUBLE(size)
    ZEND_PARSE_PARAMETERS_END();

    if (!(size >= 1.0 && size <= 200.0)) {
        zend_value_error("FastChart\\Chart::setFontSize() expects a value in [1.0, 200.0]");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->font_size = size;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setCategoryLabels)
{
    zval *labels;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(labels)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    /* Stash a refcounted copy on config so the per-type draw can
     * find it under a known key. Replacing on each call keeps the
     * setter idempotent. */
    zval labels_copy;
    ZVAL_COPY(&labels_copy, labels);
    add_assoc_zval(&self->config, "category_labels", &labels_copy);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setBackgroundColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(rgb)
    ZEND_PARSE_PARAMETERS_END();
    if (rgb < -1 || rgb > 0xFFFFFF) {
        zend_value_error("FastChart\\Chart::setBackgroundColor() expects -1 (theme default) or 0..0xFFFFFF");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->bg_override = rgb;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setPlotBackgroundColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(rgb)
    ZEND_PARSE_PARAMETERS_END();
    if (rgb < -1 || rgb > 0xFFFFFF) {
        zend_value_error("FastChart\\Chart::setPlotBackgroundColor() expects -1 (theme default) or 0..0xFFFFFF");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->plot_bg_override = rgb;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setSeriesColors)
{
    zval *colors;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(colors)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(colors);

    int n = 0;
    int parsed[8];
    zval *v;
    ZEND_HASH_FOREACH_VAL(ht, v) {
        if (n >= 8) break;
        if (Z_TYPE_P(v) != IS_LONG) {
            zend_type_error("FastChart\\Chart::setSeriesColors() expects a list of 24-bit RGB ints");
            RETURN_THROWS();
        }
        zend_long c = Z_LVAL_P(v);
        if (c < 0 || c > 0xFFFFFF) {
            zend_value_error("FastChart\\Chart::setSeriesColors() entry out of range; expected 0..0xFFFFFF");
            RETURN_THROWS();
        }
        parsed[n++] = (int)c;
    } ZEND_HASH_FOREACH_END();

    self->series_colors_n = n;
    for (int i = 0; i < n; i++) self->series_colors[i] = parsed[i];

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setLegendPosition)
{
    zend_long pos;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(pos)
    ZEND_PARSE_PARAMETERS_END();

    if (pos < FASTCHART_LEGEND_NONE || pos > FASTCHART_LEGEND_BOTTOM_LEFT) {
        zend_value_error("FastChart\\Chart::setLegendPosition() expects a LEGEND_* class constant");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->legend_position = pos;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setYAxisScale)
{
    zend_long scale;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(scale)
    ZEND_PARSE_PARAMETERS_END();

    if (scale != FASTCHART_SCALE_LINEAR && scale != FASTCHART_SCALE_LOG) {
        zend_value_error("FastChart\\Chart::setYAxisScale() expects a SCALE_* class constant");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->y_axis_scale = scale;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Field-assignment bool setter: stash a bool param into a struct
 * field. Defined here so every bool setter in this file (including
 * the early setStrict) can use it. */
#define FASTCHART_BOOL_SETTER(class_, name_, field_) \
    ZEND_METHOD(class_, name_) \
    { \
        bool v; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_BOOL(v) \
        ZEND_PARSE_PARAMETERS_END(); \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        self->field_ = v; \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

/* Same shape as FASTCHART_BOOL_SETTER but lets the caller pick the
 * per-type accessor (Z_FASTCHART_BAR_OBJ_P, Z_FASTCHART_RADAR_OBJ_P,
 * etc) for setters that write to a per-type struct field. */
#define FASTCHART_BOOL_SETTER_AS(class_, name_, accessor_, field_) \
    ZEND_METHOD(class_, name_) \
    { \
        bool v; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_BOOL(v) \
        ZEND_PARSE_PARAMETERS_END(); \
        accessor_(ZEND_THIS)->field_ = v; \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_BOOL_SETTER(FastChart_Chart, setStrict, strict)

/* Reject non-finite (NaN, Inf, -Inf) doubles at the setter boundary.
 * Several public setters bound their value with comparison operators
 * (`x >= lo && x <= hi`, `x < 0`, `min >= max`) which all return false
 * for NaN, leaving the value to ride through to (int)cast — undefined
 * behavior under C / Annex F. fastchart_zval_to_double already gates
 * doubles arriving via array iteration; this helper covers the
 * Z_PARAM_DOUBLE entry path. */
static int fastchart_reject_non_finite(double v, const char *where)
{
    if (!isfinite(v)) {
        zend_value_error("%s expects a finite numeric value", where);
        return -1;
    }
    return 0;
}

/* Shared annotation pusher. `kind` is "h" or "v"; storage shape on
 * config is a list of dicts {kind, value, label?, color?}. */
static void push_annotation(fastchart_obj *self, const char *kind,
                            double value, zend_string *label,
                            zend_long has_color, zend_long color)
{
    zval *list_zv = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "annotations", sizeof("annotations") - 1);
    if (!list_zv || Z_TYPE_P(list_zv) != IS_ARRAY) {
        zval list;
        array_init(&list);
        list_zv = zend_hash_str_update(Z_ARRVAL(self->config),
            "annotations", sizeof("annotations") - 1, &list);
    }

    zval entry;
    array_init(&entry);
    add_assoc_string(&entry, "kind", (char *)kind);
    add_assoc_double(&entry, "value", value);
    if (label) {
        add_assoc_str(&entry, "label", zend_string_copy(label));
    }
    if (has_color) {
        add_assoc_long(&entry, "color", color);
    }
    add_next_index_zval(list_zv, &entry);
}

ZEND_METHOD(FastChart_Chart, addHorizontalLine)
{
    double value;
    zend_string *label = NULL;
    zend_long color = 0;
    bool color_is_null = true;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_DOUBLE(value)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(label)
        Z_PARAM_LONG_OR_NULL(color, color_is_null)
    ZEND_PARSE_PARAMETERS_END();

    if (fastchart_reject_non_finite(value, "FastChart\\Chart::addHorizontalLine()") != 0) {
        RETURN_THROWS();
    }
    if (!color_is_null && (color < 0 || color > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::addHorizontalLine() color out of range; expected 0..0xFFFFFF");
        RETURN_THROWS();
    }
    if (label && memchr(ZSTR_VAL(label), 0, ZSTR_LEN(label)) != NULL) {
        zend_value_error("FastChart\\Chart::addHorizontalLine() label contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    push_annotation(self, "h", value, label, !color_is_null, color);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, addVerticalLine)
{
    double position;
    zend_string *label = NULL;
    zend_long color = 0;
    bool color_is_null = true;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_DOUBLE(position)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(label)
        Z_PARAM_LONG_OR_NULL(color, color_is_null)
    ZEND_PARSE_PARAMETERS_END();

    if (fastchart_reject_non_finite(position, "FastChart\\Chart::addVerticalLine()") != 0) {
        RETURN_THROWS();
    }
    if (!color_is_null && (color < 0 || color > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::addVerticalLine() color out of range; expected 0..0xFFFFFF");
        RETURN_THROWS();
    }
    if (label && memchr(ZSTR_VAL(label), 0, ZSTR_LEN(label)) != NULL) {
        zend_value_error("FastChart\\Chart::addVerticalLine() label contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    push_annotation(self, "v", position, label, !color_is_null, color);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

#define FASTCHART_MARKER_SETTERS(class_) \
    ZEND_METHOD(class_, setMarkerStyle) \
    { \
        zend_long style; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_LONG(style) \
        ZEND_PARSE_PARAMETERS_END(); \
        if (style < FASTCHART_MARKER_NONE || style > FASTCHART_MARKER_PLUS) { \
            zend_value_error("setMarkerStyle() expects a MARKER_* class constant"); \
            RETURN_THROWS(); \
        } \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        self->marker_style = style; \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    } \
    ZEND_METHOD(class_, setMarkerSize) \
    { \
        zend_long size; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_LONG(size) \
        ZEND_PARSE_PARAMETERS_END(); \
        if (size < 1 || size > 32) { \
            zend_value_error("setMarkerSize() expects a value in [1, 32]"); \
            RETURN_THROWS(); \
        } \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        self->marker_size = size; \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_MARKER_SETTERS(FastChart_LineChart)
FASTCHART_MARKER_SETTERS(FastChart_ScatterChart)

#define FASTCHART_COLOR_OVERRIDE_SETTER(name_, field_) \
    ZEND_METHOD(FastChart_Chart, name_) \
    { \
        zend_long rgb; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_LONG(rgb) \
        ZEND_PARSE_PARAMETERS_END(); \
        if (rgb < -1 || rgb > 0xFFFFFF) { \
            zend_value_error("FastChart\\Chart::" #name_ "() expects -1 or 0..0xFFFFFF"); \
            RETURN_THROWS(); \
        } \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        self->field_ = rgb; \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_COLOR_OVERRIDE_SETTER(setAxisColor,   axis_color_override)
FASTCHART_COLOR_OVERRIDE_SETTER(setGridColor,   grid_color_override)
FASTCHART_COLOR_OVERRIDE_SETTER(setBorderColor, border_color_override)
FASTCHART_COLOR_OVERRIDE_SETTER(setTextColor,   text_color_override)

#define FASTCHART_FONT_OVERRIDE_SETTER(name_, path_field_, size_field_) \
    ZEND_METHOD(FastChart_Chart, name_) \
    { \
        zend_string *path = NULL; \
        double size = 0.0; \
        bool size_is_null = true; \
        ZEND_PARSE_PARAMETERS_START(0, 2) \
            Z_PARAM_OPTIONAL \
            Z_PARAM_STR_OR_NULL(path) \
            Z_PARAM_DOUBLE_OR_NULL(size, size_is_null) \
        ZEND_PARSE_PARAMETERS_END(); \
        if (path && memchr(ZSTR_VAL(path), 0, ZSTR_LEN(path)) != NULL) { \
            zend_value_error("FastChart\\Chart::" #name_ "() path contains an embedded NUL"); \
            RETURN_THROWS(); \
        } \
        if (path && php_check_open_basedir(ZSTR_VAL(path))) { \
            RETURN_THROWS(); \
        } \
        if (!size_is_null && !(size >= 1.0 && size <= 200.0)) { \
            zend_value_error("FastChart\\Chart::" #name_ "() size must be in [1.0, 200.0]"); \
            RETURN_THROWS(); \
        } \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        if (path) { \
            if (self->path_field_) zend_string_release(self->path_field_); \
            self->path_field_ = ZSTR_LEN(path) == 0 ? NULL : zend_string_copy(path); \
        } \
        if (!size_is_null) { \
            self->size_field_ = size; \
        } \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_FONT_OVERRIDE_SETTER(setTitleFont, title_font_path, title_font_size)
FASTCHART_FONT_OVERRIDE_SETTER(setAxisFont,  axis_font_path,  axis_font_size)
FASTCHART_FONT_OVERRIDE_SETTER(setLabelFont, label_font_path, label_font_size)

/* Validate a user-supplied sprintf format string for safe use against
 * a single double argument. Allows literal text plus exactly one
 * conversion directive of the f/F/e/E/g/G family (with optional
 * flags / width / precision). Rejects %s (wrong-arg-type crash),
 * %n (write-where), %d/%i/%x/etc. (wrong-type), embedded NULs, and
 * format strings with zero or more than one numeric conversion.
 *
 * Returns 0 on success, -1 with an exception thrown on failure.
 * `where` is the method name shown in the error. Empty format
 * strings (length 0) are treated as the "clear / use default"
 * sentinel and accepted without further checks. */
static int fastchart_validate_double_format(const zend_string *fmt, const char *where)
{
    if (!fmt || ZSTR_LEN(fmt) == 0) return 0;

    const char *p = ZSTR_VAL(fmt);
    size_t len = ZSTR_LEN(fmt);
    if (memchr(p, 0, len) != NULL) {
        zend_value_error("FastChart\\Chart::%s() format contains an embedded NUL", where);
        return -1;
    }

    int n_conversions = 0;
    for (size_t i = 0; i < len; i++) {
        if (p[i] != '%') continue;
        if (i + 1 < len && p[i + 1] == '%') {  /* literal %% */
            i++;
            continue;
        }
        i++;  /* skip the % */
        /* Skip flags */
        while (i < len && (p[i] == '-' || p[i] == '+' || p[i] == ' ' ||
                            p[i] == '#' || p[i] == '0' || p[i] == '\'')) i++;
        /* Skip width */
        while (i < len && p[i] >= '0' && p[i] <= '9') i++;
        /* Skip precision */
        if (i < len && p[i] == '.') {
            i++;
            while (i < len && p[i] >= '0' && p[i] <= '9') i++;
        }
        /* Reject length modifiers (l, ll, h, etc. -- they imply
         * non-double arg types). */
        if (i < len && (p[i] == 'l' || p[i] == 'L' || p[i] == 'h' ||
                        p[i] == 'j' || p[i] == 'z' || p[i] == 't')) {
            zend_value_error("FastChart\\Chart::%s() length modifiers are not allowed in format strings", where);
            return -1;
        }
        /* Conversion specifier must be one of the double family. */
        if (i >= len) {
            zend_value_error("FastChart\\Chart::%s() format ends with an incomplete conversion", where);
            return -1;
        }
        char c = p[i];
        if (c != 'f' && c != 'F' && c != 'e' && c != 'E' && c != 'g' && c != 'G') {
            zend_value_error("FastChart\\Chart::%s() format must use one numeric conversion (f/F/e/E/g/G), got '%%%c'", where, c);
            return -1;
        }
        n_conversions++;
        if (n_conversions > 1) {
            zend_value_error("FastChart\\Chart::%s() format must contain exactly one numeric conversion", where);
            return -1;
        }
    }

    if (n_conversions == 0) {
        zend_value_error("FastChart\\Chart::%s() format must contain exactly one numeric conversion", where);
        return -1;
    }
    return 0;
}

ZEND_METHOD(FastChart_Chart, setShowValues)
{
    bool show;
    zend_string *fmt = NULL;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_BOOL(show)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(fmt)
    ZEND_PARSE_PARAMETERS_END();

    if (fmt && ZSTR_LEN(fmt) > 0) {
        if (fastchart_validate_double_format(fmt, "setShowValues") != 0) {
            RETURN_THROWS();
        }
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->show_values = show;
    if (fmt) {
        if (self->value_format) zend_string_release(self->value_format);
        self->value_format = ZSTR_LEN(fmt) == 0 ? NULL : zend_string_copy(fmt);
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER(FastChart_Chart, setTransparentBackground, transparent_bg)

ZEND_METHOD(FastChart_Chart, setBackgroundImage)
{
    zend_string *path;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(path)
    ZEND_PARSE_PARAMETERS_END();

    if (ZSTR_LEN(path) > 0) {
        if (memchr(ZSTR_VAL(path), 0, ZSTR_LEN(path)) != NULL) {
            zend_value_error("FastChart\\Chart::setBackgroundImage() path contains an embedded NUL");
            RETURN_THROWS();
        }
        if (php_check_open_basedir(ZSTR_VAL(path))) {
            RETURN_THROWS();
        }
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->bg_image_path) zend_string_release(self->bg_image_path);
    self->bg_image_path = ZSTR_LEN(path) == 0 ? NULL : zend_string_copy(path);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setLineInterpolation)
{
    zend_long mode;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(mode)
    ZEND_PARSE_PARAMETERS_END();
    if (mode != FASTCHART_INTERP_LINEAR && mode != FASTCHART_INTERP_SMOOTH) {
        zend_value_error("FastChart\\Chart::setLineInterpolation() expects INTERP_LINEAR or INTERP_SMOOTH");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->line_interpolation = mode;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setPlotRect)
{
    zend_long x0, y0, x1, y1;
    ZEND_PARSE_PARAMETERS_START(4, 4)
        Z_PARAM_LONG(x0)
        Z_PARAM_LONG(y0)
        Z_PARAM_LONG(x1)
        Z_PARAM_LONG(y1)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    /* Validate ranges before any subtraction so PHP_INT_MIN /
     * PHP_INT_MAX inputs can't trip signed-overflow UB on x1 - x0. */
    if (x0 < 0 || y0 < 0 || x1 > 65535 || y1 > 65535) {
        zend_value_error("FastChart\\Chart::setPlotRect() coordinates must fit within a 16-bit canvas");
        RETURN_THROWS();
    }
    /* Negative width or height (post-validation) reverts to auto-layout. */
    if (x1 <= x0 || y1 <= y0) {
        self->has_plot_rect = false;
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    self->has_plot_rect = true;
    self->plot_x0 = (int)x0;
    self->plot_y0 = (int)y0;
    self->plot_x1 = (int)x1;
    self->plot_y1 = (int)y1;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setBorderSides)
{
    zend_long sides;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(sides)
    ZEND_PARSE_PARAMETERS_END();
    if (sides < 0 || sides > FASTCHART_BORDER_ALL) {
        zend_value_error("FastChart\\Chart::setBorderSides() expects a bitwise OR of BORDER_* constants in [0, 15]");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->border_sides = sides;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER(FastChart_Chart, setXAxisVisible, x_axis_visible)
FASTCHART_BOOL_SETTER(FastChart_Chart, setYAxisVisible, y_axis_visible)

#define FASTCHART_LABEL_FORMAT_SETTER(name_, field_) \
    ZEND_METHOD(FastChart_Chart, name_) \
    { \
        zend_string *fmt; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_STR(fmt) \
        ZEND_PARSE_PARAMETERS_END(); \
        if (ZSTR_LEN(fmt) > 0 && \
            fastchart_validate_double_format(fmt, #name_) != 0) { \
            RETURN_THROWS(); \
        } \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        if (self->field_) zend_string_release(self->field_); \
        self->field_ = ZSTR_LEN(fmt) == 0 ? NULL : zend_string_copy(fmt); \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_LABEL_FORMAT_SETTER(setYAxisLabelFormat, y_axis_label_format)
FASTCHART_LABEL_FORMAT_SETTER(setXAxisLabelFormat, x_axis_label_format)

ZEND_METHOD(FastChart_Chart, setTickMode)
{
    zend_long m;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(m)
    ZEND_PARSE_PARAMETERS_END();
    if (m < 0 || m > FASTCHART_TICK_BOTH) {
        zend_value_error("FastChart\\Chart::setTickMode() expects TICK_NONE | TICK_LABELS | TICK_POINTS | TICK_BOTH");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->tick_mode = m;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setBarWidth)
{
    zend_long pct;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(pct)
    ZEND_PARSE_PARAMETERS_END();
    if (pct < 1 || pct > 100) {
        zend_value_error("FastChart\\Chart::setBarWidth() expects a percent in [1, 100]");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->bar_width_pct = pct;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setEdgeColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(rgb)
    ZEND_PARSE_PARAMETERS_END();
    if (rgb < -1 || rgb > 0xFFFFFF) {
        zend_value_error("FastChart\\Chart::setEdgeColor() expects -1 or 0..0xFFFFFF");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->edge_color = rgb;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER(FastChart_Chart, setZeroShelf, zero_shelf)

ZEND_METHOD(FastChart_Chart, setXLabelStride)
{
    zend_long n;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(n)
    ZEND_PARSE_PARAMETERS_END();
    if (n < 1 || n > 1000) {
        zend_value_error("FastChart\\Chart::setXLabelStride() expects a stride in [1, 1000]");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->x_label_stride = n;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setSecondaryYAxisTitle)
{
    zend_string *t;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(t)
    ZEND_PARSE_PARAMETERS_END();
    if (memchr(ZSTR_VAL(t), 0, ZSTR_LEN(t)) != NULL) {
        zend_value_error("FastChart\\Chart::setSecondaryYAxisTitle() text contains an embedded NUL");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->y_axis_title2) zend_string_release(self->y_axis_title2);
    self->y_axis_title2 = ZSTR_LEN(t) == 0 ? NULL : zend_string_copy(t);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER(FastChart_Chart, setThumbnailMode, thumbnail_mode)

FASTCHART_COLOR_OVERRIDE_SETTER(setTitleColor,     title_color)
FASTCHART_COLOR_OVERRIDE_SETTER(setAxisLabelColor, axis_label_color)
FASTCHART_COLOR_OVERRIDE_SETTER(setAxisTitleColor, axis_title_color)

/* color_ramp_low/high lives on the shared object struct so we can
 * accept setColorRamp on any Chart subclass. Only the chart families
 * that paint a continuous numeric range (SurfaceChart, ContourChart)
 * actually read it; setting it on a LineChart is a harmless no-op. */
ZEND_METHOD(FastChart_Chart, setColorRamp)
{
    zend_long lo, hi;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(lo)
        Z_PARAM_LONG(hi)
    ZEND_PARSE_PARAMETERS_END();
    if (lo < 0 || lo > 0xFFFFFF || hi < 0 || hi > 0xFFFFFF) {
        zend_value_error("FastChart\\Chart::setColorRamp() colors must be 0..0xFFFFFF");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->color_ramp_low = lo;
    self->color_ramp_high = hi;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, addTextAnnotation)
{
    zend_string *text;
    zend_long x, y;
    zend_long color = -1;
    bool color_is_null = true;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STR(text)
        Z_PARAM_LONG(x)
        Z_PARAM_LONG(y)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(color, color_is_null)
    ZEND_PARSE_PARAMETERS_END();

    if (!color_is_null && (color < 0 || color > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::addTextAnnotation() color must be 0..0xFFFFFF or null");
        RETURN_THROWS();
    }
    if (x < INT_MIN || x > INT_MAX || y < INT_MIN || y > INT_MAX) {
        zend_value_error("FastChart\\Chart::addTextAnnotation() x and y must fit in a 32-bit int");
        RETURN_THROWS();
    }
    if (memchr(ZSTR_VAL(text), 0, ZSTR_LEN(text)) != NULL) {
        zend_value_error("FastChart\\Chart::addTextAnnotation() text contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval *list_zv = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "text_annotations", sizeof("text_annotations") - 1);
    if (!list_zv || Z_TYPE_P(list_zv) != IS_ARRAY) {
        zval list;
        array_init(&list);
        list_zv = zend_hash_str_update(Z_ARRVAL(self->config),
            "text_annotations", sizeof("text_annotations") - 1, &list);
    }

    zval entry;
    array_init(&entry);
    add_assoc_str(&entry, "text", zend_string_copy(text));
    add_assoc_long(&entry, "x", x);
    add_assoc_long(&entry, "y", y);
    if (!color_is_null) add_assoc_long(&entry, "color", color);
    add_next_index_zval(list_zv, &entry);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BarChart, setStackMode)
{
    zend_long m;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(m)
    ZEND_PARSE_PARAMETERS_END();
    if (m < 0 || m > FASTCHART_STACK_LAYER) {
        zend_value_error("FastChart\\BarChart::setStackMode() expects STACK_SUM | STACK_BESIDE | STACK_LAYER");
        RETURN_THROWS();
    }
    fastchart_bar_obj *self = Z_FASTCHART_BAR_OBJ_P(ZEND_THIS);
    self->stack_mode = m;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER_AS(FastChart_BarChart, setFloating, Z_FASTCHART_BAR_OBJ_P, bar_floating)

ZEND_METHOD(FastChart_Chart, setLineStyle)
{
    zend_long s;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(s)
    ZEND_PARSE_PARAMETERS_END();
    if (s < FASTCHART_LINE_SOLID || s > FASTCHART_LINE_DOTTED) {
        zend_value_error("FastChart\\Chart::setLineStyle() expects LINE_SOLID | LINE_DASHED | LINE_DOTTED");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->line_style = s;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setGradientFill)
{
    zend_long from, to = -1, dir = FASTCHART_GRADIENT_VERTICAL;
    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_LONG(from)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(to)
        Z_PARAM_LONG(dir)
    ZEND_PARSE_PARAMETERS_END();
    if (from != -1 && (from < 0 || from > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::setGradientFill() $from must be -1 or 0..0xFFFFFF");
        RETURN_THROWS();
    }
    if (to != -1 && (to < 0 || to > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::setGradientFill() $to must be -1 or 0..0xFFFFFF");
        RETURN_THROWS();
    }
    if (dir != FASTCHART_GRADIENT_VERTICAL && dir != FASTCHART_GRADIENT_HORIZONTAL) {
        zend_value_error("FastChart\\Chart::setGradientFill() $direction must be GRADIENT_VERTICAL or GRADIENT_HORIZONTAL");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->gradient_from = from;
    self->gradient_to = (to == -1 && from != -1) ? from : to;
    self->gradient_dir = dir;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setDropShadow)
{
    zend_long dx, dy;
    zend_long color = 0x000000;
    bool color_is_null = true;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_LONG(dx)
        Z_PARAM_LONG(dy)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(color, color_is_null)
    ZEND_PARSE_PARAMETERS_END();

    if (dx < -50 || dx > 50 || dy < -50 || dy > 50) {
        zend_value_error("FastChart\\Chart::setDropShadow() offsets must be in [-50, 50]");
        RETURN_THROWS();
    }
    if (!color_is_null && (color < 0 || color > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::setDropShadow() color must be 0..0xFFFFFF");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->shadow_dx = dx;
    self->shadow_dy = dy;
    if (!color_is_null) self->shadow_color = color;
    self->has_drop_shadow = (dx != 0 || dy != 0);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setShadowAlpha)
{
    zend_long alpha;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(alpha)
    ZEND_PARSE_PARAMETERS_END();
    if (alpha < 0 || alpha > 127) {
        zend_value_error("FastChart\\Chart::setShadowAlpha() expects 0..127 (libgd alpha)");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->shadow_alpha = alpha;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_ScatterChart, setTrendLine)
{
    bool en;
    zend_long color = -1;
    zend_long degree = 1;
    bool color_is_null = true;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_BOOL(en)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(color, color_is_null)
        Z_PARAM_LONG(degree)
    ZEND_PARSE_PARAMETERS_END();

    if (!color_is_null && (color < 0 || color > 0xFFFFFF)) {
        zend_value_error("FastChart\\ScatterChart::setTrendLine() color must be 0..0xFFFFFF");
        RETURN_THROWS();
    }
    if (degree < 1 || degree > 3) {
        zend_value_error("FastChart\\ScatterChart::setTrendLine() degree must be in [1, 3]");
        RETURN_THROWS();
    }
    fastchart_scatter_obj *self = Z_FASTCHART_SCATTER_OBJ_P(ZEND_THIS);
    self->trend_line = en;
    self->trend_line_color = color_is_null ? -1 : color;
    self->trend_degree = degree;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

#define FASTCHART_ERROR_BARS_SETTER(class_) \
    ZEND_METHOD(class_, setErrorBars) \
    { \
        zval *errs; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_ARRAY(errs) \
        ZEND_PARSE_PARAMETERS_END(); \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        zval copy; \
        ZVAL_COPY(&copy, errs); \
        zend_hash_str_update(Z_ARRVAL(self->config), "error_bars", \
                             sizeof("error_bars") - 1, &copy); \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_ERROR_BARS_SETTER(FastChart_LineChart)
FASTCHART_ERROR_BARS_SETTER(FastChart_ScatterChart)

ZEND_METHOD(FastChart_RadarChart, setMaxValue)
{
    double m;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_DOUBLE(m)
    ZEND_PARSE_PARAMETERS_END();
    if (fastchart_reject_non_finite(m, "FastChart\\RadarChart::setMaxValue()") != 0) {
        RETURN_THROWS();
    }
    if (m < 0.0) {
        zend_value_error("FastChart\\RadarChart::setMaxValue() must be non-negative");
        RETURN_THROWS();
    }
    fastchart_radar_obj *self = Z_FASTCHART_RADAR_OBJ_P(ZEND_THIS);
    self->radar_max = m;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER_AS(FastChart_RadarChart, setFilled, Z_FASTCHART_RADAR_OBJ_P, radar_filled)


ZEND_METHOD(FastChart_SurfaceChart, setShowCellValues)
{
    bool show;
    zend_string *fmt = NULL;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_BOOL(show)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(fmt)
    ZEND_PARSE_PARAMETERS_END();
    if (fmt && ZSTR_LEN(fmt) > 0 &&
        fastchart_validate_double_format(fmt, "setShowCellValues") != 0) {
        RETURN_THROWS();
    }
    fastchart_surface_obj *self = Z_FASTCHART_SURFACE_OBJ_P(ZEND_THIS);
    self->surface_show_values = show;
    if (fmt) {
        if (self->surface_value_format) zend_string_release(self->surface_value_format);
        self->surface_value_format = ZSTR_LEN(fmt) == 0 ? NULL : zend_string_copy(fmt);
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_GaugeChart, setValue)
{
    double v;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_DOUBLE(v)
    ZEND_PARSE_PARAMETERS_END();
    if (fastchart_reject_non_finite(v, "FastChart\\GaugeChart::setValue()") != 0) {
        RETURN_THROWS();
    }
    fastchart_gauge_obj *self = Z_FASTCHART_GAUGE_OBJ_P(ZEND_THIS);
    self->gauge_value = v;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_GaugeChart, setRange)
{
    double mn, mx;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_DOUBLE(mn)
        Z_PARAM_DOUBLE(mx)
    ZEND_PARSE_PARAMETERS_END();
    if (fastchart_reject_non_finite(mn, "FastChart\\GaugeChart::setRange()") != 0 ||
        fastchart_reject_non_finite(mx, "FastChart\\GaugeChart::setRange()") != 0) {
        RETURN_THROWS();
    }
    if (mn >= mx) {
        zend_value_error("FastChart\\GaugeChart::setRange() requires min < max");
        RETURN_THROWS();
    }
    fastchart_gauge_obj *self = Z_FASTCHART_GAUGE_OBJ_P(ZEND_THIS);
    self->gauge_min = mn;
    self->gauge_max = mx;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_GaugeChart, setZones)
{
    zval *zones;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(zones)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval copy;
    ZVAL_COPY(&copy, zones);
    zend_hash_str_update(Z_ARRVAL(self->config), "gauge_zones",
                         sizeof("gauge_zones") - 1, &copy);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_GaugeChart, setValueFormat)
{
    zend_string *fmt;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(fmt)
    ZEND_PARSE_PARAMETERS_END();
    if (ZSTR_LEN(fmt) > 0 &&
        fastchart_validate_double_format(fmt, "GaugeChart::setValueFormat") != 0) {
        RETURN_THROWS();
    }
    fastchart_gauge_obj *self = Z_FASTCHART_GAUGE_OBJ_P(ZEND_THIS);
    if (self->gauge_value_format) zend_string_release(self->gauge_value_format);
    self->gauge_value_format = ZSTR_LEN(fmt) == 0 ? NULL : zend_string_copy(fmt);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setDateAxisStride)
{
    zend_long unit, every = 1;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_LONG(unit)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(every)
    ZEND_PARSE_PARAMETERS_END();
    if (unit < FASTCHART_DATE_DAY || unit > FASTCHART_DATE_YEAR) {
        zend_value_error("FastChart\\Chart::setDateAxisStride() unit must be a DATE_* constant");
        RETURN_THROWS();
    }
    if (every < 0 || every > 1000) {
        zend_value_error("FastChart\\Chart::setDateAxisStride() every must be in [0, 1000] (0 = auto)");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->date_axis_unit = unit;
    self->date_axis_every = every;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Walk a flat list of points (Scatter/Bubble shape) and emit
 * <area> entries for those that carry an 'href' / 'tooltip' key.
 * The chart must have been render()'d already so the chart's
 * cached pixel positions are present in self->config["areas"]. */
ZEND_METHOD(FastChart_ScatterChart, getImageMap)
{
    zend_string *name;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval *areas_zv = zend_hash_str_find(Z_ARRVAL(self->config),
                                        "image_map_areas",
                                        sizeof("image_map_areas") - 1);
    if (!areas_zv || Z_TYPE_P(areas_zv) != IS_ARRAY ||
        zend_hash_num_elements(Z_ARRVAL_P(areas_zv)) == 0) {
        RETURN_EMPTY_STRING();
    }

    smart_str out = {0};

    /* Map name: HTML5 imagemap names allow letters, digits, '-' and '_'.
     * Anything else gets stripped to prevent attribute-injection
     * through a crafted name. If sanitization leaves zero characters
     * (e.g. caller passed "!@#"), fall back to the default so we
     * never emit name="" — which would silently break any
     * <img usemap="#..."> referencing the chart. */
    smart_str_appends(&out, "<map name=\"");
    const char *map_name = "fastchart";
    size_t map_name_len = sizeof("fastchart") - 1;
    if (ZEND_NUM_ARGS() >= 1 && name && ZSTR_LEN(name) > 0) {
        map_name = ZSTR_VAL(name);
        map_name_len = ZSTR_LEN(name);
    }
    size_t before = ZSTR_LEN(out.s ? out.s : ZSTR_EMPTY_ALLOC());
    for (size_t i = 0; i < map_name_len; i++) {
        char c = map_name[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_') {
            smart_str_appendc(&out, c);
        }
    }
    smart_str_0(&out);
    if (ZSTR_LEN(out.s) == before) {
        smart_str_appendl(&out, "fastchart", sizeof("fastchart") - 1);
    }
    smart_str_appends(&out, "\">");

    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(areas_zv), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *zx = zend_hash_str_find(Z_ARRVAL_P(entry), "x", sizeof("x") - 1);
        zval *zy = zend_hash_str_find(Z_ARRVAL_P(entry), "y", sizeof("y") - 1);
        zval *zr = zend_hash_str_find(Z_ARRVAL_P(entry), "r", sizeof("r") - 1);
        zval *zh = zend_hash_str_find(Z_ARRVAL_P(entry), "href", sizeof("href") - 1);
        zval *zt = zend_hash_str_find(Z_ARRVAL_P(entry), "title", sizeof("title") - 1);
        if (!zx || !zy || !zr || !zh) continue;
        if (Z_TYPE_P(zh) != IS_STRING) continue;

        /* Scheme allowlist: dangerous URL schemes (javascript:, data:,
         * vbscript:) are rejected. Relative paths, fragments, and
         * mailto: are allowed alongside http(s). Reject the whole
         * <area> entry on a bad scheme rather than emit a sanitized
         * one -- callers can audit their input. Embedded NUL also
         * drops the entry: HTML5 parsers replace \0 with U+FFFD,
         * but C-string consumers truncate, so the visible href and
         * the actual link diverge. Same convention as the format-
         * string validator. */
        const char *href_str = Z_STRVAL_P(zh);
        size_t href_len = Z_STRLEN_P(zh);
        if (memchr(href_str, 0, href_len) != NULL) continue;
        bool href_ok = false;
        if (href_len == 0) {
            href_ok = true;
        } else if (href_str[0] == '/' || href_str[0] == '#') {
            /* Single leading '/' covers absolute paths (/foo) and the
             * legacy protocol-relative form (//host/path), which the
             * browser resolves against the page scheme. The XSS surface
             * is the same as an explicit https:// since the scheme is
             * inherited, not chosen by the link. */
            href_ok = true;
        } else if (zend_binary_strncasecmp(href_str, href_len, "http://",  7,  7) == 0 ||
                   zend_binary_strncasecmp(href_str, href_len, "https://", 8,  8) == 0 ||
                   zend_binary_strncasecmp(href_str, href_len, "mailto:",  7,  7) == 0) {
            href_ok = true;
        }
        if (!href_ok) continue;
        if (zt && Z_TYPE_P(zt) == IS_STRING &&
            memchr(Z_STRVAL_P(zt), 0, Z_STRLEN_P(zt)) != NULL) {
            /* Tooltip with embedded NUL: drop the title attribute,
             * keep the link. Same divergence rationale as href. */
            zt = NULL;
        }

        char buf[256];
        int n = snprintf(buf, sizeof(buf),
            "<area shape=\"circle\" coords=\"" ZEND_LONG_FMT "," ZEND_LONG_FMT "," ZEND_LONG_FMT "\" href=\"",
            Z_LVAL_P(zx), Z_LVAL_P(zy), Z_LVAL_P(zr));
        if (n < 0) n = 0;
        if ((size_t)n > sizeof(buf)) n = (int)sizeof(buf);
        smart_str_appendl(&out, buf, (size_t)n);

        /* HTML-escape the href value: &, <, >, " each become their
         * entity form. Single quotes don't need escaping inside a
         * double-quoted attribute. */
        for (size_t i = 0; i < href_len; i++) {
            char c = href_str[i];
            switch (c) {
                case '&':  smart_str_appends(&out, "&amp;");  break;
                case '<':  smart_str_appends(&out, "&lt;");   break;
                case '>':  smart_str_appends(&out, "&gt;");   break;
                case '"':  smart_str_appends(&out, "&quot;"); break;
                default:   smart_str_appendc(&out, c);
            }
        }
        smart_str_appends(&out, "\"");

        if (zt && Z_TYPE_P(zt) == IS_STRING) {
            smart_str_appends(&out, " title=\"");
            const char *t_str = Z_STRVAL_P(zt);
            size_t t_len = Z_STRLEN_P(zt);
            for (size_t i = 0; i < t_len; i++) {
                char c = t_str[i];
                switch (c) {
                    case '&':  smart_str_appends(&out, "&amp;");  break;
                    case '<':  smart_str_appends(&out, "&lt;");   break;
                    case '>':  smart_str_appends(&out, "&gt;");   break;
                    case '"':  smart_str_appends(&out, "&quot;"); break;
                    default:   smart_str_appendc(&out, c);
                }
            }
            smart_str_appends(&out, "\"");
        }
        smart_str_appends(&out, ">");
    } ZEND_HASH_FOREACH_END();

    smart_str_appends(&out, "</map>");
    smart_str_0(&out);
    RETURN_STR(out.s);
}

ZEND_METHOD(FastChart_GanttChart, setTimeRange)
{
    zend_long start = 0, end = 0;
    bool start_is_null = true, end_is_null = true;
    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(start, start_is_null)
        Z_PARAM_LONG_OR_NULL(end, end_is_null)
    ZEND_PARSE_PARAMETERS_END();
    /* Validate before storing so the comparison runs against the
     * full-precision zend_long values, not after a narrowing cast. */
    bool has_range = !(start_is_null && end_is_null);
    if (has_range && end <= start) {
        zend_value_error("FastChart\\GanttChart::setTimeRange() requires start < end");
        RETURN_THROWS();
    }
    fastchart_gantt_obj *self = Z_FASTCHART_GANTT_OBJ_P(ZEND_THIS);
    self->gantt_has_range = has_range;
    self->gantt_range_start = start_is_null ? 0 : start;
    self->gantt_range_end   = end_is_null   ? 0 : end;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER_AS(FastChart_GanttChart, setShowTaskLabels, Z_FASTCHART_GANTT_OBJ_P, gantt_show_labels)

ZEND_METHOD(FastChart_BoxPlot, setBoxWidth)
{
    zend_long pct;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(pct)
    ZEND_PARSE_PARAMETERS_END();
    if (pct < 1 || pct > 100) {
        zend_value_error("FastChart\\BoxPlot::setBoxWidth() expects a percent in [1, 100]");
        RETURN_THROWS();
    }
    fastchart_boxplot_obj *self = Z_FASTCHART_BOXPLOT_OBJ_P(ZEND_THIS);
    self->box_width_pct = pct;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PolarChart, setMaxRadius)
{
    double m;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_DOUBLE(m)
    ZEND_PARSE_PARAMETERS_END();
    if (fastchart_reject_non_finite(m, "FastChart\\PolarChart::setMaxRadius()") != 0) {
        RETURN_THROWS();
    }
    if (m < 0.0) {
        zend_value_error("FastChart\\PolarChart::setMaxRadius() must be non-negative");
        RETURN_THROWS();
    }
    fastchart_polar_obj *self = Z_FASTCHART_POLAR_OBJ_P(ZEND_THIS);
    self->polar_max_radius = m;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER_AS(FastChart_PolarChart, setFilled, Z_FASTCHART_POLAR_OBJ_P, polar_filled)

ZEND_METHOD(FastChart_ContourChart, setLevels)
{
    zval *levels;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(levels)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval copy;
    ZVAL_COPY(&copy, levels);
    zend_hash_str_update(Z_ARRVAL(self->config), "contour_levels",
                         sizeof("contour_levels") - 1, &copy);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER_AS(FastChart_ContourChart, setFilled, Z_FASTCHART_CONTOUR_OBJ_P, contour_filled)

ZEND_METHOD(FastChart_Chart, addOverlaySeries)
{
    zend_string *type;
    zval *values;
    HashTable *opts = NULL;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(type)
        Z_PARAM_ARRAY(values)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END();

    if (!zend_string_equals_literal(type, "line") &&
        !zend_string_equals_literal(type, "area")) {
        zend_value_error("FastChart\\Chart::addOverlaySeries() type must be \"line\" or \"area\"");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval *list_zv = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "overlays", sizeof("overlays") - 1);
    if (!list_zv || Z_TYPE_P(list_zv) != IS_ARRAY) {
        zval list;
        array_init(&list);
        list_zv = zend_hash_str_update(Z_ARRVAL(self->config),
            "overlays", sizeof("overlays") - 1, &list);
    }

    zval entry;
    array_init(&entry);
    add_assoc_str(&entry, "type", zend_string_copy(type));
    zval values_copy;
    ZVAL_COPY(&values_copy, values);
    add_assoc_zval(&entry, "values", &values_copy);

    if (opts) {
        zval *opt;
        opt = zend_hash_str_find(opts, "color", sizeof("color") - 1);
        if (opt && Z_TYPE_P(opt) == IS_LONG) {
            zend_long c = Z_LVAL_P(opt);
            if (c >= 0 && c <= 0xFFFFFF) add_assoc_long(&entry, "color", c);
        }
        opt = zend_hash_str_find(opts, "label", sizeof("label") - 1);
        if (opt && Z_TYPE_P(opt) == IS_STRING) {
            add_assoc_str(&entry, "label", zend_string_copy(Z_STR_P(opt)));
        }
        opt = zend_hash_str_find(opts, "thickness", sizeof("thickness") - 1);
        if (opt && Z_TYPE_P(opt) == IS_LONG) {
            zend_long t = Z_LVAL_P(opt);
            if (t >= 1 && t <= 16) add_assoc_long(&entry, "thickness", t);
        }
        opt = zend_hash_str_find(opts, "axis", sizeof("axis") - 1);
        if (opt && Z_TYPE_P(opt) == IS_STRING) {
            add_assoc_str(&entry, "axis", zend_string_copy(Z_STR_P(opt)));
        }
    }

    add_next_index_zval(list_zv, &entry);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PieChart, setOtherThreshold)
{
    double percent;
    zend_string *label = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_DOUBLE(percent)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(label)
    ZEND_PARSE_PARAMETERS_END();

    if (fastchart_reject_non_finite(percent, "FastChart\\PieChart::setOtherThreshold()") != 0) {
        RETURN_THROWS();
    }
    if (percent < 0.0 || percent >= 100.0) {
        zend_value_error("FastChart\\PieChart::setOtherThreshold() expects a percentage in [0.0, 100.0)");
        RETURN_THROWS();
    }
    if (label && memchr(ZSTR_VAL(label), 0, ZSTR_LEN(label)) != NULL) {
        zend_value_error("FastChart\\PieChart::setOtherThreshold() label contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_pie_obj *self = Z_FASTCHART_PIE_OBJ_P(ZEND_THIS);
    self->pie_other_threshold = percent;
    if (self->pie_other_label) zend_string_release(self->pie_other_label);
    self->pie_other_label = label ? zend_string_copy(label) : NULL;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, setVolumeColors)
{
    zval *colors;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(colors)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval colors_copy;
    ZVAL_COPY(&colors_copy, colors);
    add_assoc_zval(&self->config, "volume_colors", &colors_copy);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setXAxisTitle)
{
    zend_string *txt;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(txt)
    ZEND_PARSE_PARAMETERS_END();
    if (memchr(ZSTR_VAL(txt), 0, ZSTR_LEN(txt)) != NULL) {
        zend_value_error("FastChart\\Chart::setXAxisTitle() text contains an embedded NUL");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->x_axis_title) zend_string_release(self->x_axis_title);
    self->x_axis_title = ZSTR_LEN(txt) == 0 ? NULL : zend_string_copy(txt);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setYAxisTitle)
{
    zend_string *txt;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(txt)
    ZEND_PARSE_PARAMETERS_END();
    if (memchr(ZSTR_VAL(txt), 0, ZSTR_LEN(txt)) != NULL) {
        zend_value_error("FastChart\\Chart::setYAxisTitle() text contains an embedded NUL");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->y_axis_title) zend_string_release(self->y_axis_title);
    self->y_axis_title = ZSTR_LEN(txt) == 0 ? NULL : zend_string_copy(txt);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setXAxisLabelAngle)
{
    zend_long deg;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(deg)
    ZEND_PARSE_PARAMETERS_END();

    if (deg != 0 && deg != 45 && deg != 90) {
        zend_value_error("FastChart\\Chart::setXAxisLabelAngle() expects 0, 45, or 90 degrees");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->x_axis_label_angle = deg;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setYAxisRange)
{
    double y_min = 0, y_max = 0, y_int = 0;
    bool y_min_is_null = true, y_max_is_null = true, y_int_is_null = true;

    ZEND_PARSE_PARAMETERS_START(0, 3)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE_OR_NULL(y_min, y_min_is_null)
        Z_PARAM_DOUBLE_OR_NULL(y_max, y_max_is_null)
        Z_PARAM_DOUBLE_OR_NULL(y_int, y_int_is_null)
    ZEND_PARSE_PARAMETERS_END();

    if ((!y_min_is_null && fastchart_reject_non_finite(y_min, "FastChart\\Chart::setYAxisRange()") != 0) ||
        (!y_max_is_null && fastchart_reject_non_finite(y_max, "FastChart\\Chart::setYAxisRange()") != 0) ||
        (!y_int_is_null && fastchart_reject_non_finite(y_int, "FastChart\\Chart::setYAxisRange()") != 0)) {
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);

    if (!y_min_is_null && !y_max_is_null && y_min >= y_max) {
        zend_value_error("FastChart\\Chart::setYAxisRange() min must be < max");
        RETURN_THROWS();
    }
    if (!y_int_is_null && y_int <= 0) {
        zend_value_error("FastChart\\Chart::setYAxisRange() interval must be > 0");
        RETURN_THROWS();
    }

    self->has_y_min      = !y_min_is_null;
    self->has_y_max      = !y_max_is_null;
    self->has_y_interval = !y_int_is_null;
    self->y_min          = y_min;
    self->y_max          = y_max;
    self->y_interval     = y_int;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER(FastChart_Chart, setSecondaryYAxis, secondary_y)

ZEND_METHOD(FastChart_PieChart, setExplode)
{
    zval *offsets;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(offsets)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval offsets_copy;
    ZVAL_COPY(&offsets_copy, offsets);
    add_assoc_zval(&self->config, "explode", &offsets_copy);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PieChart, setSliceLabelPosition)
{
    zend_long pos;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(pos)
    ZEND_PARSE_PARAMETERS_END();
    if (pos < FASTCHART_LABEL_NONE || pos > FASTCHART_LABEL_RIGHT) {
        zend_value_error("FastChart\\PieChart::setSliceLabelPosition() expects a LABEL_* class constant");
        RETURN_THROWS();
    }
    fastchart_pie_obj *self = Z_FASTCHART_PIE_OBJ_P(ZEND_THIS);
    self->slice_label_position = pos;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PieChart, setSliceLabelFormat)
{
    zend_string *fmt;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(fmt)
    ZEND_PARSE_PARAMETERS_END();
    if (ZSTR_LEN(fmt) > 0 &&
        fastchart_validate_double_format(fmt, "PieChart::setSliceLabelFormat") != 0) {
        RETURN_THROWS();
    }
    fastchart_pie_obj *self = Z_FASTCHART_PIE_OBJ_P(ZEND_THIS);
    if (self->slice_label_format) zend_string_release(self->slice_label_format);
    self->slice_label_format = ZSTR_LEN(fmt) == 0 ? NULL : zend_string_copy(fmt);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, setCandleStyle)
{
    zend_long style;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(style)
    ZEND_PARSE_PARAMETERS_END();
    if (style < FASTCHART_STYLE_CANDLE || style > FASTCHART_STYLE_VECTOR) {
        zend_value_error("FastChart\\StockChart::setCandleStyle() expects a STYLE_* class constant");
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    self->candle_style = style;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_AreaChart, setSeries)
{
    zval *series;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(series)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval_ptr_dtor(&self->data);
    ZVAL_COPY(&self->data, series);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_AreaChart, setStacked)
{
    bool stacked;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(stacked)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    add_assoc_bool(&self->config, "stacked", stacked);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_AreaChart, setFillOpacity)
{
    zend_long alpha;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(alpha)
    ZEND_PARSE_PARAMETERS_END();
    if (alpha < 0 || alpha > 127) {
        zend_value_error("FastChart\\AreaChart::setFillOpacity() expects a value in [0, 127]");
        RETURN_THROWS();
    }
    fastchart_area_obj *self = Z_FASTCHART_AREA_OBJ_P(ZEND_THIS);
    self->area_alpha = alpha;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Dispatch by class entry. Six concrete subclasses; a single
 * if/else chain is fine -- this is per-render-call, not per-pixel.
 * Returns 0 on success with a PHP exception possibly pending; -1
 * if we hit an unknown class entry (defensive; should not happen
 * because the abstract base is uninstantiable). */
/* Dispatch by class entry. self points at the start of the per-type
 * struct; we cast to the specific type for each renderer. The cast
 * is safe because Z_FASTCHART_OBJ_P landed on the start of whatever
 * subclass the user actually instantiated. */
static int dispatch_render(fastchart_obj *self, zend_class_entry *ce, gdImagePtr im)
{
    if (ce == fastchart_line_chart_ce)    return fastchart_line_render_to_image((fastchart_line_obj *)self, im);
    if (ce == fastchart_area_chart_ce)    return fastchart_area_render_to_image((fastchart_area_obj *)self, im);
    if (ce == fastchart_bar_chart_ce)     return fastchart_bar_render_to_image((fastchart_bar_obj *)self, im);
    if (ce == fastchart_pie_chart_ce)     return fastchart_pie_render_to_image((fastchart_pie_obj *)self, im);
    if (ce == fastchart_scatter_chart_ce) return fastchart_scatter_render_to_image((fastchart_scatter_obj *)self, im);
    if (ce == fastchart_stock_chart_ce)   return fastchart_stock_render_to_image((fastchart_stock_obj *)self, im);
    if (ce == fastchart_radar_chart_ce)   return fastchart_radar_render_to_image((fastchart_radar_obj *)self, im);
    if (ce == fastchart_bubble_chart_ce)  return fastchart_bubble_render_to_image((fastchart_bubble_obj *)self, im);
    if (ce == fastchart_surface_chart_ce) return fastchart_surface_render_to_image((fastchart_surface_obj *)self, im);
    if (ce == fastchart_gauge_chart_ce)   return fastchart_gauge_render_to_image((fastchart_gauge_obj *)self, im);
    if (ce == fastchart_gantt_chart_ce)   return fastchart_gantt_render_to_image((fastchart_gantt_obj *)self, im);
    if (ce == fastchart_box_plot_ce)      return fastchart_boxplot_render_to_image((fastchart_boxplot_obj *)self, im);
    if (ce == fastchart_polar_chart_ce)   return fastchart_polar_render_to_image((fastchart_polar_obj *)self, im);
    if (ce == fastchart_contour_chart_ce) return fastchart_contour_render_to_image((fastchart_contour_obj *)self, im);
    zend_throw_error(NULL, "FastChart: render dispatch found unknown class entry");
    return -1;
}

/* Encode a rendered gdImagePtr in the requested format. Returns
 * malloc'd bytes via *out_bytes / *out_sz; caller gdFree's. NULL
 * out_bytes on failure (caller throws). `format`: 0 PNG, 1 JPEG,
 * 2 WebP, 3 GIF, 4 AVIF. */
static int encode_image(gdImagePtr im, int format, int quality,
                        void **out_bytes, int *out_sz)
{
    *out_bytes = NULL;
    *out_sz = 0;
    switch (format) {
        case 0: *out_bytes = gdImagePngPtr(im, out_sz); break;
        case 1: *out_bytes = gdImageJpegPtr(im, out_sz, quality); break;
        case 2: *out_bytes = gdImageWebpPtrEx(im, out_sz, quality); break;
        case 3: *out_bytes = gdImageGifPtr(im, out_sz); break;
        case 4:
#ifdef HAVE_GD_AVIF
            *out_bytes = gdImageAvifPtrEx(im, out_sz, quality, -1);
#else
            zend_throw_exception(zend_ce_exception,
                "FastChart: libgd was built without AVIF support", 0);
            return -1;
#endif
            break;
        default:
            return -1;
    }
    return (*out_bytes && *out_sz > 0) ? 0 : -1;
}

static void render_to_string_helper(INTERNAL_FUNCTION_PARAMETERS, int format, zend_long quality)
{
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);

    if (self->width <= 0 || self->height <= 0) {
        zend_throw_error(NULL, "FastChart: invalid canvas size; setSize() first");
        RETURN_THROWS();
    }

    gdImagePtr im = gdImageCreateTrueColor((int)self->width, (int)self->height);
    if (!im) {
        zend_throw_error(NULL, "FastChart: gdImageCreateTrueColor() returned NULL");
        RETURN_THROWS();
    }

    if (dispatch_render(self, Z_OBJCE_P(ZEND_THIS), im) != 0 || EG(exception)) {
        gdImageDestroy(im);
        RETURN_THROWS();
    }

    void *bytes = NULL;
    int sz = 0;
    if (encode_image(im, format, (int)quality, &bytes, &sz) != 0) {
        if (bytes) gdFree(bytes);
        gdImageDestroy(im);
        if (!EG(exception)) {
            zend_throw_error(NULL, "FastChart: gd encoder produced no output");
        }
        RETURN_THROWS();
    }

    zend_string *out = zend_string_init((const char *)bytes, (size_t)sz, 0);
    gdFree(bytes);
    gdImageDestroy(im);
    RETURN_STR(out);
}

ZEND_METHOD(FastChart_Chart, renderPng)
{
    ZEND_PARSE_PARAMETERS_NONE();
    render_to_string_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, 0);
}

ZEND_METHOD(FastChart_Chart, renderJpeg)
{
    zend_long quality = 90;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();

    if (quality < 1 || quality > 100) {
        zend_value_error("FastChart\\Chart::renderJpeg() quality must be in [1, 100]");
        RETURN_THROWS();
    }
    render_to_string_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1, quality);
}

ZEND_METHOD(FastChart_Chart, renderWebp)
{
    zend_long quality = 90;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();

    if (quality < 0 || quality > 100) {
        zend_value_error("FastChart\\Chart::renderWebp() quality must be in [0, 100]");
        RETURN_THROWS();
    }
    render_to_string_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2, quality);
}

ZEND_METHOD(FastChart_Chart, renderGif)
{
    ZEND_PARSE_PARAMETERS_NONE();
    render_to_string_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, 3, 0);
}

ZEND_METHOD(FastChart_Chart, renderAvif)
{
    zend_long quality = 60;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();

    if (quality < 0 || quality > 100) {
        zend_value_error("FastChart\\Chart::renderAvif() quality must be in [0, 100]");
        RETURN_THROWS();
    }
    render_to_string_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, 4, quality);
}

/* --------------------- renderToFile -------------------------------
 *
 * Path extension picks the format. Honors open_basedir. Writes via
 * the Zend stream layer so wrapper paths (file://, sftp://) work
 * within open_basedir constraints. */
static int format_from_path(const char *path, size_t len)
{
    /* Walk back to find the last '.'; bounded to avoid scanning a
     * megabyte of pathological input. */
    if (len == 0 || len > 4096) return -1;
    const char *dot = NULL;
    for (size_t i = len; i > 0; i--) {
        if (path[i - 1] == '.') { dot = &path[i - 1]; break; }
        if (path[i - 1] == '/' || path[i - 1] == '\\') break;
    }
    if (!dot) return -1;
    const char *ext = dot + 1;
    size_t ext_len = strlen(ext);
    /* ASCII-only fold: strcasecmp() honors the C locale, so a
     * tr_TR.UTF-8 process would map "I" to "ı" and miss "JPG". */
    if (zend_binary_strcasecmp(ext, ext_len, "png",  3) == 0) return 0;
    if (zend_binary_strcasecmp(ext, ext_len, "jpg",  3) == 0) return 1;
    if (zend_binary_strcasecmp(ext, ext_len, "jpeg", 4) == 0) return 1;
    if (zend_binary_strcasecmp(ext, ext_len, "webp", 4) == 0) return 2;
    if (zend_binary_strcasecmp(ext, ext_len, "gif",  3) == 0) return 3;
    if (zend_binary_strcasecmp(ext, ext_len, "avif", 4) == 0) return 4;
    return -1;
}

ZEND_METHOD(FastChart_Chart, renderToFile)
{
    zend_string *path;
    zend_long quality = 90;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_PATH_STR(path)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();

    int format = format_from_path(ZSTR_VAL(path), ZSTR_LEN(path));
    if (format < 0) {
        zend_value_error("FastChart\\Chart::renderToFile() could not infer format from extension; expected .png/.jpg/.jpeg/.webp/.gif/.avif");
        RETURN_THROWS();
    }
    if (quality < 1 || quality > 100) {
        zend_value_error("FastChart\\Chart::renderToFile() quality must be in [1, 100]");
        RETURN_THROWS();
    }
    if (php_check_open_basedir(ZSTR_VAL(path))) {
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->width <= 0 || self->height <= 0) {
        zend_throw_error(NULL, "FastChart: invalid canvas size; setSize() first");
        RETURN_THROWS();
    }

    gdImagePtr im = gdImageCreateTrueColor((int)self->width, (int)self->height);
    if (!im) {
        zend_throw_error(NULL, "FastChart: gdImageCreateTrueColor() returned NULL");
        RETURN_THROWS();
    }

    if (dispatch_render(self, Z_OBJCE_P(ZEND_THIS), im) != 0 || EG(exception)) {
        gdImageDestroy(im);
        RETURN_THROWS();
    }

    void *bytes = NULL;
    int sz = 0;
    if (encode_image(im, format, (int)quality, &bytes, &sz) != 0) {
        if (bytes) gdFree(bytes);
        gdImageDestroy(im);
        if (!EG(exception)) {
            zend_throw_error(NULL, "FastChart: gd encoder produced no output");
        }
        RETURN_THROWS();
    }
    gdImageDestroy(im);

    php_stream *stream = php_stream_open_wrapper(ZSTR_VAL(path), "wb",
        REPORT_ERRORS, NULL);
    if (!stream) {
        gdFree(bytes);
        RETURN_THROWS();
    }

    ssize_t written = php_stream_write(stream, (const char *)bytes, (size_t)sz);
    php_stream_close(stream);
    gdFree(bytes);

    if (written < 0) {
        zend_throw_error(NULL, "FastChart: write to %s failed", ZSTR_VAL(path));
        RETURN_THROWS();
    }
    RETURN_LONG((zend_long)written);
}

/* ---------------- per-class setSeries family --------------------
 *
 * gen_stub emits a separate ZEND_METHOD entry per class even though
 * the bodies are identical, so we cannot collapse them into one
 * shared ZEND_FUNCTION. They all just stash the array on `data`. */

#define FASTCHART_SETTER_ARRAY(class_, method_, slot_) \
    ZEND_METHOD(class_, method_) \
    { \
        zval *arr; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_ARRAY(arr) \
        ZEND_PARSE_PARAMETERS_END(); \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        zval_ptr_dtor(&self->slot_); \
        ZVAL_COPY(&self->slot_, arr); \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_SETTER_ARRAY(FastChart_LineChart,    setSeries, data)
FASTCHART_SETTER_ARRAY(FastChart_BarChart,     setSeries, data)
FASTCHART_SETTER_ARRAY(FastChart_PieChart,     setSlices, data)
FASTCHART_SETTER_ARRAY(FastChart_ScatterChart, setPoints, data)
FASTCHART_SETTER_ARRAY(FastChart_StockChart,   setOhlcv,  data)
FASTCHART_SETTER_ARRAY(FastChart_RadarChart,   setSeries, data)
FASTCHART_SETTER_ARRAY(FastChart_BubbleChart,  setPoints, data)
FASTCHART_SETTER_ARRAY(FastChart_SurfaceChart, setGrid,   data)
FASTCHART_SETTER_ARRAY(FastChart_GanttChart,   setTasks,  data)
FASTCHART_SETTER_ARRAY(FastChart_BoxPlot,      setBoxes,  data)
FASTCHART_SETTER_ARRAY(FastChart_PolarChart,   setSeries, data)
FASTCHART_SETTER_ARRAY(FastChart_ContourChart, setGrid,   data)

ZEND_METHOD(FastChart_BarChart, setStacked)
{
    bool stacked;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(stacked)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    add_assoc_bool(&self->config, "stacked", stacked);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PieChart, setDonutHoleRatio)
{
    double ratio;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_DOUBLE(ratio)
    ZEND_PARSE_PARAMETERS_END();

    if (fastchart_reject_non_finite(ratio, "FastChart\\PieChart::setDonutHoleRatio()") != 0) {
        RETURN_THROWS();
    }
    if (ratio < 0.0 || ratio >= 1.0) {
        zend_value_error("FastChart\\PieChart::setDonutHoleRatio() expects a value in [0.0, 1.0)");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    add_assoc_double(&self->config, "donut_hole_ratio", ratio);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, setMovingAverages)
{
    zval *periods;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(periods)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    zval periods_copy;
    ZVAL_COPY(&periods_copy, periods);
    add_assoc_zval(&self->config, "moving_averages", &periods_copy);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, setVolumePane)
{
    bool enabled;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(enabled)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    add_assoc_bool(&self->config, "volume_pane", enabled);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, addIndicatorPane)
{
    zend_string *name;
    zval *values;
    HashTable *opts = NULL;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(name)
        Z_PARAM_ARRAY(values)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END();

    if (ZSTR_LEN(name) == 0) {
        zend_value_error("FastChart\\StockChart::addIndicatorPane() requires a non-empty name");
        RETURN_THROWS();
    }
    if (memchr(ZSTR_VAL(name), 0, ZSTR_LEN(name)) != NULL) {
        zend_value_error("FastChart\\StockChart::addIndicatorPane() name contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);

    /* Indicator panes live as a list under config[indicator_panes].
     * Each entry: {name, values: [...], color?, reference?, min?, max?}.
     * Cap at 3 panes so the price+volume+indicator stack stays
     * legible on a typical 600-700px tall canvas. */
    zval *list_zv = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "indicator_panes",
                                       sizeof("indicator_panes") - 1);
    if (!list_zv || Z_TYPE_P(list_zv) != IS_ARRAY) {
        zval list;
        array_init(&list);
        list_zv = zend_hash_str_update(Z_ARRVAL(self->config),
            "indicator_panes", sizeof("indicator_panes") - 1, &list);
    }
    if (zend_hash_num_elements(Z_ARRVAL_P(list_zv)) >= 3) {
        zend_value_error("FastChart\\StockChart::addIndicatorPane() supports at most 3 panes");
        RETURN_THROWS();
    }

    zval entry;
    array_init(&entry);
    add_assoc_str(&entry, "name", zend_string_copy(name));

    zval values_copy;
    ZVAL_COPY(&values_copy, values);
    add_assoc_zval(&entry, "values", &values_copy);

    if (opts) {
        zval *opt;
        opt = zend_hash_str_find(opts, "color", sizeof("color") - 1);
        if (opt && Z_TYPE_P(opt) == IS_LONG) {
            zend_long c = Z_LVAL_P(opt);
            if (c >= 0 && c <= 0xFFFFFF) {
                add_assoc_long(&entry, "color", c);
            }
        }
        opt = zend_hash_str_find(opts, "reference", sizeof("reference") - 1);
        if (opt) {
            double d;
            if (Z_TYPE_P(opt) == IS_DOUBLE) d = Z_DVAL_P(opt);
            else if (Z_TYPE_P(opt) == IS_LONG) d = (double)Z_LVAL_P(opt);
            else d = NAN;
            if (!isnan(d)) add_assoc_double(&entry, "reference", d);
        }
        opt = zend_hash_str_find(opts, "min", sizeof("min") - 1);
        if (opt) {
            double d;
            if (Z_TYPE_P(opt) == IS_DOUBLE) d = Z_DVAL_P(opt);
            else if (Z_TYPE_P(opt) == IS_LONG) d = (double)Z_LVAL_P(opt);
            else d = NAN;
            if (!isnan(d)) add_assoc_double(&entry, "min", d);
        }
        opt = zend_hash_str_find(opts, "max", sizeof("max") - 1);
        if (opt) {
            double d;
            if (Z_TYPE_P(opt) == IS_DOUBLE) d = Z_DVAL_P(opt);
            else if (Z_TYPE_P(opt) == IS_LONG) d = (double)Z_LVAL_P(opt);
            else d = NAN;
            if (!isnan(d)) add_assoc_double(&entry, "max", d);
        }
    }

    add_next_index_zval(list_zv, &entry);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

PHP_MINIT_FUNCTION(fastchart)
{
    /* Per-class handlers init. Each handlers struct gets its own
     * std offset matching its per-type struct layout, plus the
     * class's free / clone hooks. */
#define FASTCHART_INIT_HANDLERS(name, T) do {                                            \
        memcpy(&fastchart_##name##_handlers, &std_object_handlers,                       \
               sizeof(zend_object_handlers));                                            \
        fastchart_##name##_handlers.offset    = XtOffsetOf(T, std);                      \
        fastchart_##name##_handlers.free_obj  = fastchart_##name##_free_object;          \
        fastchart_##name##_handlers.clone_obj = fastchart_##name##_clone_object;         \
        fastchart_##name##_handlers.dtor_obj  = zend_objects_destroy_object;             \
    } while (0)
    FASTCHART_INIT_HANDLERS(line,    fastchart_line_obj);
    FASTCHART_INIT_HANDLERS(area,    fastchart_area_obj);
    FASTCHART_INIT_HANDLERS(bar,     fastchart_bar_obj);
    FASTCHART_INIT_HANDLERS(pie,     fastchart_pie_obj);
    FASTCHART_INIT_HANDLERS(scatter, fastchart_scatter_obj);
    FASTCHART_INIT_HANDLERS(stock,   fastchart_stock_obj);
    FASTCHART_INIT_HANDLERS(radar,   fastchart_radar_obj);
    FASTCHART_INIT_HANDLERS(bubble,  fastchart_bubble_obj);
    FASTCHART_INIT_HANDLERS(surface, fastchart_surface_obj);
    FASTCHART_INIT_HANDLERS(gauge,   fastchart_gauge_obj);
    FASTCHART_INIT_HANDLERS(gantt,   fastchart_gantt_obj);
    FASTCHART_INIT_HANDLERS(boxplot, fastchart_boxplot_obj);
    FASTCHART_INIT_HANDLERS(polar,   fastchart_polar_obj);
    FASTCHART_INIT_HANDLERS(contour, fastchart_contour_obj);
#undef FASTCHART_INIT_HANDLERS

    /* Abstract base class: never instantiated directly, so it does
     * not need its own create_object. The per-class create handlers
     * below take over for every concrete subclass. */
    fastchart_chart_ce = register_class_FastChart_Chart();

    fastchart_line_chart_ce    = register_class_FastChart_LineChart(fastchart_chart_ce);
    fastchart_line_chart_ce->create_object = fastchart_line_create_object;

    fastchart_area_chart_ce    = register_class_FastChart_AreaChart(fastchart_chart_ce);
    fastchart_area_chart_ce->create_object = fastchart_area_create_object;

    fastchart_bar_chart_ce     = register_class_FastChart_BarChart(fastchart_chart_ce);
    fastchart_bar_chart_ce->create_object = fastchart_bar_create_object;

    fastchart_pie_chart_ce     = register_class_FastChart_PieChart(fastchart_chart_ce);
    fastchart_pie_chart_ce->create_object = fastchart_pie_create_object;

    fastchart_scatter_chart_ce = register_class_FastChart_ScatterChart(fastchart_chart_ce);
    fastchart_scatter_chart_ce->create_object = fastchart_scatter_create_object;

    fastchart_stock_chart_ce   = register_class_FastChart_StockChart(fastchart_chart_ce);
    fastchart_stock_chart_ce->create_object = fastchart_stock_create_object;

    fastchart_radar_chart_ce   = register_class_FastChart_RadarChart(fastchart_chart_ce);
    fastchart_radar_chart_ce->create_object = fastchart_radar_create_object;

    fastchart_bubble_chart_ce  = register_class_FastChart_BubbleChart(fastchart_chart_ce);
    fastchart_bubble_chart_ce->create_object = fastchart_bubble_create_object;

    fastchart_surface_chart_ce = register_class_FastChart_SurfaceChart(fastchart_chart_ce);
    fastchart_surface_chart_ce->create_object = fastchart_surface_create_object;

    fastchart_gauge_chart_ce   = register_class_FastChart_GaugeChart(fastchart_chart_ce);
    fastchart_gauge_chart_ce->create_object = fastchart_gauge_create_object;

    fastchart_gantt_chart_ce   = register_class_FastChart_GanttChart(fastchart_chart_ce);
    fastchart_gantt_chart_ce->create_object = fastchart_gantt_create_object;

    fastchart_box_plot_ce      = register_class_FastChart_BoxPlot(fastchart_chart_ce);
    fastchart_box_plot_ce->create_object = fastchart_boxplot_create_object;

    fastchart_polar_chart_ce   = register_class_FastChart_PolarChart(fastchart_chart_ce);
    fastchart_polar_chart_ce->create_object = fastchart_polar_create_object;

    fastchart_contour_chart_ce = register_class_FastChart_ContourChart(fastchart_chart_ce);
    fastchart_contour_chart_ce->create_object = fastchart_contour_create_object;

    fastchart_gd_image_ce = zend_hash_str_find_ptr(CG(class_table),
        "gdimage", sizeof("gdimage") - 1);

    if (!fastchart_gd_image_ce) {
        php_error_docref(NULL, E_CORE_ERROR,
            "fastchart: GdImage class not found; ext/gd must be loaded before fastchart");
        return FAILURE;
    }

    fastchart_default_font_path = fastchart_probe_default_font();
    /* A NULL probe result is not fatal -- users can still call
     * setFontPath() per-instance. The text helpers no-op on NULL. */

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(fastchart)
{
    if (fastchart_default_font_path) {
        /* Allocated via zend_string_init(..., persistent=1) in
         * fastchart_probe_default_font; the persistent variant of
         * release is what pairs with that. */
        zend_string_release_ex(fastchart_default_font_path, /*persistent=*/1);
        fastchart_default_font_path = NULL;
    }
    return SUCCESS;
}

PHP_MINFO_FUNCTION(fastchart)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "fastchart support", "enabled");
    php_info_print_table_row(2, "fastchart version", PHP_FASTCHART_VERSION);
    php_info_print_table_row(2, "Backend", "ext/gd + libgd + FreeType");
    php_info_print_table_row(2, "Default font",
        fastchart_default_font_path
            ? ZSTR_VAL(fastchart_default_font_path)
            : "(not auto-detected, setFontPath() required)");
    php_info_print_table_end();
}

zend_module_entry fastchart_module_entry = {
    STANDARD_MODULE_HEADER,
    "fastchart",
    NULL,                       /* function_entries */
    PHP_MINIT(fastchart),
    PHP_MSHUTDOWN(fastchart),
    NULL,                       /* RINIT */
    NULL,                       /* RSHUTDOWN */
    PHP_MINFO(fastchart),
    PHP_FASTCHART_VERSION,
    STANDARD_MODULE_PROPERTIES,
};

#ifdef COMPILE_DL_FASTCHART
ZEND_GET_MODULE(fastchart)
#endif
