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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "Zend/zend_exceptions.h"

#include <math.h>
#include <sys/stat.h>

#include "php_fastchart.h"
#include "fastchart_arginfo.h"

zend_class_entry *fastchart_chart_ce;
zend_class_entry *fastchart_line_chart_ce;
zend_class_entry *fastchart_bar_chart_ce;
zend_class_entry *fastchart_pie_chart_ce;
zend_class_entry *fastchart_scatter_chart_ce;
zend_class_entry *fastchart_stock_chart_ce;
zend_class_entry *fastchart_gd_image_ce = NULL;

static zend_object_handlers fastchart_object_handlers;

/* Auto-detected default font path. Probed at MINIT, used as the
 * initial font_path on every newly-allocated chart instance. NULL
 * if no probe candidate exists on the system; users must then call
 * setFontPath() before any text-rendering chart method. */
static zend_string *fastchart_default_font_path = NULL;

/* ---------------------- ext/gd interop helper ---------------------- */

gdImagePtr fastchart_gd_image_from_zval(zval *canvas_zv)
{
    if (Z_TYPE_P(canvas_zv) != IS_OBJECT) {
        return NULL;
    }
    return php_gd_libgdimageptr_from_zval_p(canvas_zv);
}

/* --------------------- object create / free / clone ---------------- */

static zend_object *fastchart_create_object(zend_class_entry *ce)
{
    fastchart_obj *intern = zend_object_alloc(sizeof(fastchart_obj), ce);

    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);

    intern->width  = FASTCHART_DEFAULT_WIDTH;
    intern->height = FASTCHART_DEFAULT_HEIGHT;
    intern->theme  = FASTCHART_THEME_LIGHT;
    intern->title  = ZSTR_EMPTY_ALLOC();
    intern->font_size = FASTCHART_DEFAULT_FONT_SIZE;

    intern->bg_override = -1;
    intern->plot_bg_override = -1;
    intern->series_colors_n = 0;
    for (int i = 0; i < 8; i++) intern->series_colors[i] = -1;

    intern->strict = false;
    intern->legend_position = FASTCHART_LEGEND_TOP_RIGHT;
    intern->y_axis_scale = FASTCHART_SCALE_LINEAR;
    intern->marker_style = -1;  /* per-chart default applies */
    intern->marker_size = -1;

    if (fastchart_default_font_path) {
        intern->font_path = zend_string_copy(fastchart_default_font_path);
    } else {
        intern->font_path = NULL;
    }

    array_init(&intern->data);
    array_init(&intern->config);

    intern->std.handlers = &fastchart_object_handlers;
    return &intern->std;
}

static void fastchart_free_object(zend_object *object)
{
    fastchart_obj *intern = fastchart_obj_from_zend(object);

    if (intern->title) {
        zend_string_release(intern->title);
    }
    if (intern->font_path) {
        zend_string_release(intern->font_path);
    }
    zval_ptr_dtor(&intern->data);
    zval_ptr_dtor(&intern->config);

    zend_object_std_dtor(&intern->std);
}

#define fastchart_clone_object NULL

/* --------------------- font default detection ---------------------- */

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

/* ---------------- FastChart\Chart base method bodies --------------- */

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

/* ---------------- background / palette overrides ----------------- */

static void store_color_override(zend_long *slot, zend_long rgb,
                                 const char *who)
{
    if (rgb < -1 || rgb > 0xFFFFFF) {
        zend_value_error("%s expects -1 (theme default) or 0..0xFFFFFF", who);
        return;
    }
    *slot = rgb;
}

ZEND_METHOD(FastChart_Chart, setBackgroundColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(rgb)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    store_color_override(&self->bg_override, rgb,
        "FastChart\\Chart::setBackgroundColor()");
    if (EG(exception)) RETURN_THROWS();
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, setPlotBackgroundColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(rgb)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    store_color_override(&self->plot_bg_override, rgb,
        "FastChart\\Chart::setPlotBackgroundColor()");
    if (EG(exception)) RETURN_THROWS();
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

/* ---------------- legend / scale / strict / annotations ---------- */

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

ZEND_METHOD(FastChart_Chart, setStrict)
{
    bool strict;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(strict)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->strict = strict;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
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
    bool label_is_null = true;
    zend_long color = 0;
    bool color_is_null = true;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_DOUBLE(value)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(label)
        Z_PARAM_LONG_OR_NULL(color, color_is_null)
    ZEND_PARSE_PARAMETERS_END();
    label_is_null = (label == NULL);

    if (!color_is_null && (color < 0 || color > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::addHorizontalLine() color out of range; expected 0..0xFFFFFF");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    push_annotation(self, "h", value,
                    label_is_null ? NULL : label,
                    !color_is_null, color);
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

    if (!color_is_null && (color < 0 || color > 0xFFFFFF)) {
        zend_value_error("FastChart\\Chart::addVerticalLine() color out of range; expected 0..0xFFFFFF");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    push_annotation(self, "v", position,
                    label == NULL ? NULL : label,
                    !color_is_null, color);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* ---------------- LineChart / ScatterChart marker setters -------- */

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

/* ----------------- renderPng / renderJpeg / renderWebp ----------- */

/* Dispatch by class entry. Five concrete subclasses; a single
 * if/else chain is fine -- this is per-render-call, not per-pixel.
 * Returns 0 on success with a PHP exception possibly pending; -1
 * if we hit an unknown class entry (defensive; should not happen
 * because the abstract base is uninstantiable). */
static int dispatch_render(fastchart_obj *self, zend_class_entry *ce, gdImagePtr im)
{
    if (ce == fastchart_line_chart_ce)    return fastchart_line_render_to_image(self, im);
    if (ce == fastchart_bar_chart_ce)     return fastchart_bar_render_to_image(self, im);
    if (ce == fastchart_pie_chart_ce)     return fastchart_pie_render_to_image(self, im);
    if (ce == fastchart_scatter_chart_ce) return fastchart_scatter_render_to_image(self, im);
    if (ce == fastchart_stock_chart_ce)   return fastchart_stock_render_to_image(self, im);
    zend_throw_error(NULL, "FastChart: render dispatch found unknown class entry");
    return -1;
}

/* `format`: 0 = PNG, 1 = JPEG, 2 = WebP. */
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

    int sz = 0;
    void *bytes = NULL;
    if (format == 0) {
        bytes = gdImagePngPtr(im, &sz);
    } else if (format == 1) {
        bytes = gdImageJpegPtr(im, &sz, (int)quality);
    } else {
        bytes = gdImageWebpPtrEx(im, &sz, (int)quality);
    }

    if (!bytes || sz <= 0) {
        if (bytes) gdFree(bytes);
        gdImageDestroy(im);
        zend_throw_error(NULL, "FastChart: gd encoder produced no output");
        RETURN_THROWS();
    }

    /* gdImage*Ptr buffers are gdMalloc'd; copy into a Zend string,
     * then gdFree the libgd buffer. */
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

/* ---------------- per-class setSeries family --------------------
 *
 * gen_stub emits a separate ZEND_METHOD entry per class even though
 * the bodies are identical, so we cannot collapse them into one
 * shared ZEND_FUNCTION. They all just stash the array on `data`. */

#define FASTCHART_SETTER_ARRAY(class_, method_, slot_, name_) \
    ZEND_METHOD(class_, method_) \
    { \
        zval *arr; \
        ZEND_PARSE_PARAMETERS_START(1, 1) \
            Z_PARAM_ARRAY(arr) \
        ZEND_PARSE_PARAMETERS_END(); \
        fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS); \
        zval_ptr_dtor(&self->slot_); \
        ZVAL_COPY(&self->slot_, arr); \
        (void)name_; \
        RETURN_ZVAL(ZEND_THIS, 1, 0); \
    }

FASTCHART_SETTER_ARRAY(FastChart_LineChart,    setSeries, data, "setSeries")
FASTCHART_SETTER_ARRAY(FastChart_BarChart,     setSeries, data, "setSeries")
FASTCHART_SETTER_ARRAY(FastChart_PieChart,     setSlices, data, "setSlices")
FASTCHART_SETTER_ARRAY(FastChart_ScatterChart, setPoints, data, "setPoints")
FASTCHART_SETTER_ARRAY(FastChart_StockChart,   setOhlcv,  data, "setOhlcv")

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

/* --------------------------- module entry ------------------------- */

PHP_MINIT_FUNCTION(fastchart)
{
    memcpy(&fastchart_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
    fastchart_object_handlers.offset    = XtOffsetOf(fastchart_obj, std);
    fastchart_object_handlers.free_obj  = fastchart_free_object;
    fastchart_object_handlers.clone_obj = fastchart_clone_object;

    fastchart_chart_ce = register_class_FastChart_Chart();
    fastchart_chart_ce->create_object = fastchart_create_object;

    fastchart_line_chart_ce    = register_class_FastChart_LineChart(fastchart_chart_ce);
    fastchart_line_chart_ce->create_object = fastchart_create_object;

    fastchart_bar_chart_ce     = register_class_FastChart_BarChart(fastchart_chart_ce);
    fastchart_bar_chart_ce->create_object = fastchart_create_object;

    fastchart_pie_chart_ce     = register_class_FastChart_PieChart(fastchart_chart_ce);
    fastchart_pie_chart_ce->create_object = fastchart_create_object;

    fastchart_scatter_chart_ce = register_class_FastChart_ScatterChart(fastchart_chart_ce);
    fastchart_scatter_chart_ce->create_object = fastchart_create_object;

    fastchart_stock_chart_ce   = register_class_FastChart_StockChart(fastchart_chart_ce);
    fastchart_stock_chart_ce->create_object = fastchart_create_object;

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
        zend_string_release(fastchart_default_font_path);
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
