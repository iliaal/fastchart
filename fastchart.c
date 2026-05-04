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

#include "php_fastchart.h"
#include "fastchart_arginfo.h"

#include <gd.h>

/* Forward-declare the only ext/gd public API we use. ext/gd does not
 * install php_gd.h via `make install` (the file lives in php-src but
 * is not in any extension's php_HEADERS list), so a third-party
 * extension cannot `#include "ext/gd/php_gd.h"` against a system PHP
 * install. The signature has been stable on PHP-8.x; mirror it here
 * verbatim from ext/gd/php_gd.h. */
extern struct gdImageStruct *php_gd_libgdimageptr_from_zval_p(zval *zp);

zend_class_entry *fastchart_chart_ce;
zend_class_entry *fastchart_line_chart_ce;
zend_class_entry *fastchart_bar_chart_ce;
zend_class_entry *fastchart_pie_chart_ce;
zend_class_entry *fastchart_scatter_chart_ce;
zend_class_entry *fastchart_stock_chart_ce;

/* Cached at MINIT via zend_lookup_class. ext/gd defines gd_image_ce
 * as a file-static global with no PHPAPI export, so we cannot link
 * against the symbol; we resolve the class entry by name at startup
 * once ext/gd's MINIT has registered it (PHP_ADD_EXTENSION_DEP in
 * config.m4 guarantees that ordering). */
static zend_class_entry *fastchart_gd_image_ce = NULL;

static zend_object_handlers fastchart_object_handlers;

static zend_object *fastchart_create_object(zend_class_entry *ce)
{
    fastchart_obj *intern = zend_object_alloc(sizeof(fastchart_obj), ce);

    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);

    intern->width  = FASTCHART_DEFAULT_WIDTH;
    intern->height = FASTCHART_DEFAULT_HEIGHT;
    intern->theme  = 0;
    intern->title  = ZSTR_EMPTY_ALLOC();
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
    zval_ptr_dtor(&intern->data);
    zval_ptr_dtor(&intern->config);

    zend_object_std_dtor(&intern->std);
}

/* Cloning would deep-copy data/config and bump the title refcount.
 * Disabled until the v0.1 drawing logic lands so we don't expose a
 * half-cooked clone path through the abstract base. */
#define fastchart_clone_object NULL

/* ---------- shared helpers used by the concrete-class methods ---------- */

static fastchart_obj *fastchart_this(zval *this_zv)
{
    return fastchart_obj_from_zend(Z_OBJ_P(this_zv));
}

/* ---------- FastChart\Chart (abstract) ---------- */

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

    fastchart_obj *self = fastchart_this(ZEND_THIS);
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

    fastchart_obj *self = fastchart_this(ZEND_THIS);
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

    if (theme != 0 && theme != 1) {
        zend_value_error("FastChart\\Chart::setTheme() expects a THEME_* class constant");
        RETURN_THROWS();
    }

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    self->theme = theme;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* ---------- shared draw stub ----------
 *
 * Every concrete subclass's draw() routes through this for the
 * scaffold cycle: validate the GdImage, return it untouched. v0.1
 * replaces the per-class entry points with type-specific drawing
 * implementations (axis, palette, plot routines per chart family). */
static void fastchart_draw_stub(INTERNAL_FUNCTION_PARAMETERS)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    /* Cross-check that ext/gd will accept the zval: the helper returns
     * the underlying gdImagePtr or NULL for a closed/invalid GdImage.
     * Throw a clean exception now rather than letting drawing code
     * later segfault on a NULL pointer. */
    gdImagePtr im = php_gd_libgdimageptr_from_zval_p(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\Chart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }

    /* Scaffold: no drawing yet. v0.1 dispatches by class entry to the
     * per-type implementations. */
    RETURN_ZVAL(canvas_zv, 1, 0);
}

/* ---------- FastChart\LineChart ---------- */

ZEND_METHOD(FastChart_LineChart, setSeries)
{
    zval *series;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(series)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    zval_ptr_dtor(&self->data);
    ZVAL_COPY(&self->data, series);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_LineChart, draw)
{
    fastchart_draw_stub(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* ---------- FastChart\BarChart ---------- */

ZEND_METHOD(FastChart_BarChart, setSeries)
{
    zval *series;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(series)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    zval_ptr_dtor(&self->data);
    ZVAL_COPY(&self->data, series);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BarChart, setStacked)
{
    bool stacked;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(stacked)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    add_assoc_bool(&self->config, "stacked", stacked);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BarChart, draw)
{
    fastchart_draw_stub(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* ---------- FastChart\PieChart ---------- */

ZEND_METHOD(FastChart_PieChart, setSlices)
{
    zval *slices;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(slices)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    zval_ptr_dtor(&self->data);
    ZVAL_COPY(&self->data, slices);

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

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    add_assoc_double(&self->config, "donut_hole_ratio", ratio);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PieChart, draw)
{
    fastchart_draw_stub(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* ---------- FastChart\ScatterChart ---------- */

ZEND_METHOD(FastChart_ScatterChart, setPoints)
{
    zval *points;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(points)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    zval_ptr_dtor(&self->data);
    ZVAL_COPY(&self->data, points);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_ScatterChart, draw)
{
    fastchart_draw_stub(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* ---------- FastChart\StockChart ---------- */

ZEND_METHOD(FastChart_StockChart, setOhlcv)
{
    zval *ohlcv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(ohlcv)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    zval_ptr_dtor(&self->data);
    ZVAL_COPY(&self->data, ohlcv);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, setMovingAverages)
{
    zval *periods;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(periods)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_obj *self = fastchart_this(ZEND_THIS);
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

    fastchart_obj *self = fastchart_this(ZEND_THIS);
    add_assoc_bool(&self->config, "volume_pane", enabled);

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, draw)
{
    fastchart_draw_stub(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* ---------- module entry ---------- */

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

    /* Resolve \GdImage from ext/gd. PHP_ADD_EXTENSION_DEP(fastchart, gd)
     * orders ext/gd's MINIT before ours, so the class is in the table
     * by the time we look it up. We hit CG(class_table) directly with
     * the lowercased name -- zend_lookup_class() goes through the
     * autoloader chain which is not safe to invoke at MINIT, and
     * zend_string_init at MINIT would have to be persistent to avoid
     * leaking through the request-scope allocator. */
    fastchart_gd_image_ce = zend_hash_str_find_ptr(CG(class_table),
        "gdimage", sizeof("gdimage") - 1);

    if (!fastchart_gd_image_ce) {
        php_error_docref(NULL, E_CORE_ERROR,
            "fastchart: GdImage class not found; ext/gd must be loaded before fastchart");
        return FAILURE;
    }

    return SUCCESS;
}

PHP_MINFO_FUNCTION(fastchart)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "fastchart support", "enabled");
    php_info_print_table_row(2, "fastchart version", PHP_FASTCHART_VERSION);
    php_info_print_table_row(2, "Backend", "ext/gd + libgd");
    php_info_print_table_end();
}

zend_module_entry fastchart_module_entry = {
    STANDARD_MODULE_HEADER,
    "fastchart",
    NULL,                       /* function_entries */
    PHP_MINIT(fastchart),
    NULL,                       /* MSHUTDOWN */
    NULL,                       /* RINIT */
    NULL,                       /* RSHUTDOWN */
    PHP_MINFO(fastchart),
    PHP_FASTCHART_VERSION,
    STANDARD_MODULE_PROPERTIES,
};

#ifdef COMPILE_DL_FASTCHART
ZEND_GET_MODULE(fastchart)
#endif
