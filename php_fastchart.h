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

/* Forward-declare the only ext/gd public API we use (also declared in
 * fastchart.c at the call site). Mirrored verbatim from
 * ext/gd/php_gd.h since that header is not installed via make install. */
extern struct gdImageStruct *php_gd_libgdimageptr_from_zval_p(zval *zp);

/* Pull the underlying gdImagePtr out of the caller-supplied canvas
 * zval. NULL on failure; the caller throws. */
gdImagePtr fastchart_gd_image_from_zval(zval *canvas_zv);

#endif /* PHP_FASTCHART_H */
