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

#define PHP_FASTCHART_VERSION "0.1.0-dev"

extern zend_module_entry fastchart_module_entry;
#define phpext_fastchart_ptr &fastchart_module_entry

#ifdef PHP_WIN32
#define PHP_FASTCHART_API __declspec(dllexport)
#else
#define PHP_FASTCHART_API
#endif

#include "php.h"

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
extern zend_class_entry *fastchart_bar_chart_ce;
extern zend_class_entry *fastchart_pie_chart_ce;
extern zend_class_entry *fastchart_scatter_chart_ce;
extern zend_class_entry *fastchart_stock_chart_ce;

/* Single object struct shared across all five concrete chart classes.
 * The shape suits the scaffold, not the v0.1 ship target -- once the
 * per-type drawing logic lands, each chart class will get its own
 * struct (and its own create_object handler) so type-specific state
 * is statically typed in C instead of stored in a generic zval. */
typedef struct _fastchart_obj {
    zend_long width;
    zend_long height;
    zend_long theme;
    zend_string *title;
    /* Holds the array passed to the most recent type-specific setter
     * (setSeries / setSlices / setPoints / setOhlcv). Empty array
     * sentinel after construction; replaced wholesale on each setter
     * call. */
    zval data;
    /* Holds per-type configuration that doesn't fit the data zval:
     * BarChart::setStacked, PieChart::setDonutHoleRatio,
     * StockChart::setMovingAverages + setVolumePane. Keyed by string
     * so each subclass writes the keys it cares about. */
    zval config;
    zend_object std;
} fastchart_obj;

static inline fastchart_obj *fastchart_obj_from_zend(zend_object *obj) {
    return (fastchart_obj *)((char *)(obj) - XtOffsetOf(fastchart_obj, std));
}

#define Z_FASTCHART_OBJ_P(zv) fastchart_obj_from_zend(Z_OBJ_P(zv))

/* Default canvas size. Any value the user sets via setSize() replaces
 * these on the instance; draw() consults the instance fields, never
 * these macros directly. */
#define FASTCHART_DEFAULT_WIDTH  800
#define FASTCHART_DEFAULT_HEIGHT 600

#endif /* PHP_FASTCHART_H */
