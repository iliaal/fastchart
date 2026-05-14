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
#include "fastchart_render_helpers.h"
#include "fastchart_target.h"
#include "fastchart_svg.h"
#include "fastchart_axis.h"

/* gen_stub.php on PHP 8.4+ emits the 6-arg ZEND_RAW_FENTRY form (the
 * abstract draw() method on Chart). PHP 8.3's macro takes 4 args, and
 * ZEND_ME there expands into the 4-arg form. Redefine variadically so
 * either call shape works -- the extra trailing args (frameless infos,
 * doc comment) just get dropped on 8.3. */
#if PHP_VERSION_ID < 80400
# undef ZEND_RAW_FENTRY
# define ZEND_RAW_FENTRY(zend_name, name, arg_info, flags, ...) \
	{ zend_name, name, arg_info, (uint32_t) (sizeof(arg_info)/sizeof(struct _zend_internal_arg_info)-1), flags },
#endif

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
zend_class_entry *fastchart_treemap_ce;
zend_class_entry *fastchart_funnel_ce;
zend_class_entry *fastchart_waterfall_ce;
zend_class_entry *fastchart_heatmap_ce;
zend_class_entry *fastchart_linear_meter_ce;
zend_class_entry *fastchart_symbol_ce;
zend_class_entry *fastchart_barcode_ce;
zend_class_entry *fastchart_code128_ce;
zend_class_entry *fastchart_qrcode_ce;
zend_class_entry *fastchart_gd_image_ce = NULL;

/* Auto-detected default font path. Probed at MINIT, used as the
 * initial font_path on every newly-allocated chart instance. NULL
 * if no probe candidate exists on the system; users must then call
 * setFontPath() before any text-rendering chart method. Non-static
 * so the Symbol family (fastchart_code128.c) can share the same
 * fallback for show_text rendering without duplicating the probe
 * chain. */
zend_string *fastchart_default_font_path = NULL;

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
static zend_object_handlers fastchart_treemap_handlers;
static zend_object_handlers fastchart_funnel_handlers;
static zend_object_handlers fastchart_waterfall_handlers;
static zend_object_handlers fastchart_heatmap_handlers;
static zend_object_handlers fastchart_linear_meter_handlers;

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
    /* Default DPI matches libgd's own default and the web-screen
     * convention. Affects PNG/JPEG metadata and FreeType glyph
     * hinting via gdImageStringFTEx. setDpi() overrides. */
    b->dpi = 96;

    b->font_path = fastchart_default_font_path
        ? zend_string_copy(fastchart_default_font_path) : NULL;

    b->category_labels = NULL;
    b->n_category_labels = 0;

    b->plot_bands = NULL;
    b->n_plot_bands = 0;

    b->icons = NULL;
    b->n_icons = 0;

    for (int i = 0; i < 4; i++) b->font_cache_path[i] = NULL;
    b->font_cache_valid = false;
    b->shadow_color_handle = -1;
    b->shadow_color_valid = false;

    array_init(&b->config);
}

static void fastchart_icons_free(fastchart_obj *b)
{
    if (!b->icons) return;
    for (int i = 0; i < b->n_icons; i++) {
        if (b->icons[i].path) efree(b->icons[i].path);
    }
    efree(b->icons);
    b->icons = NULL;
    b->n_icons = 0;
}

static void fastchart_plot_bands_free(fastchart_obj *b)
{
    if (!b->plot_bands) return;
    for (int i = 0; i < b->n_plot_bands; i++) {
        if (b->plot_bands[i].label) efree(b->plot_bands[i].label);
    }
    efree(b->plot_bands);
    b->plot_bands = NULL;
    b->n_plot_bands = 0;
}

static void fastchart_category_labels_free(fastchart_obj *b)
{
    if (!b->category_labels) return;
    for (int i = 0; i < b->n_category_labels; i++) {
        if (b->category_labels[i]) efree(b->category_labels[i]);
    }
    efree(b->category_labels);
    b->category_labels = NULL;
    b->n_category_labels = 0;
}

/* Renderer helper: borrow the category-label slots into a freshly
 * allocated const char ** sized to `n` slots so callers can pass it
 * straight to the categorical-axis drawer. Out-of-range slots are
 * filled with NULL. Returns NULL when no labels are set or n <= 0. */
const char **fastchart_borrow_category_labels(fastchart_obj *b, int n)
{
    if (n <= 0 || !b->category_labels) return NULL;
    const char **out = ecalloc((size_t)n, sizeof(const char *));
    for (int i = 0; i < n; i++) {
        out[i] = (i < b->n_category_labels) ? b->category_labels[i] : NULL;
    }
    return out;
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
    fastchart_category_labels_free(b);
    fastchart_plot_bands_free(b);
    fastchart_icons_free(b);
    zval_ptr_dtor(&b->config);
}

static void fastchart_base_addref_owned(fastchart_obj *b)
{
#define FC_ADDREF(field) if (b->field) zend_string_addref(b->field);
    FASTCHART_BASE_OWNED_STR(FC_ADDREF)
#undef FC_ADDREF
    /* Deep-copy the category-label string array so the clone owns
     * its own slots. */
    if (b->category_labels && b->n_category_labels > 0) {
        char **copy = ecalloc((size_t)b->n_category_labels, sizeof(char *));
        for (int i = 0; i < b->n_category_labels; i++) {
            const char *src = b->category_labels[i];
            if (!src) { copy[i] = NULL; continue; }
            size_t len = strlen(src);
            copy[i] = emalloc(len + 1);
            memcpy(copy[i], src, len + 1);
        }
        b->category_labels = copy;
    }
    if (b->plot_bands && b->n_plot_bands > 0) {
        size_t bytes = (size_t)b->n_plot_bands * sizeof(fastchart_plot_band);
        fastchart_plot_band *copy = emalloc(bytes);
        memcpy(copy, b->plot_bands, bytes);
        for (int i = 0; i < b->n_plot_bands; i++) {
            if (copy[i].label) {
                size_t len = strlen(copy[i].label);
                char *l = emalloc(len + 1);
                memcpy(l, copy[i].label, len + 1);
                copy[i].label = l;
            }
        }
        b->plot_bands = copy;
    }
    if (b->icons && b->n_icons > 0) {
        size_t bytes = (size_t)b->n_icons * sizeof(fastchart_icon);
        fastchart_icon *copy = emalloc(bytes);
        memcpy(copy, b->icons, bytes);
        for (int i = 0; i < b->n_icons; i++) {
            if (copy[i].path) {
                size_t len = strlen(copy[i].path);
                char *p = emalloc(len + 1);
                memcpy(p, copy[i].path, len + 1);
                copy[i].path = p;
            }
        }
        b->icons = copy;
    }
    Z_TRY_ADDREF(b->config);
}

/* Shared series array helpers. The Line / Area / Bar per-type
 * structs each carry a fixed-size fastchart_series_t array; these
 * helpers manage the malloc'd label / values / values_max /
 * point_colors slots inside each entry. */
static void fastchart_series_array_init(fastchart_series_t *arr, int max)
{
    for (int i = 0; i < max; i++) {
        arr[i].label = NULL;
        arr[i].values = NULL;
        arr[i].values_max = NULL;
        arr[i].point_colors = NULL;
        arr[i].len = 0;
        arr[i].right_axis = false;
    }
}
static void fastchart_series_array_release(fastchart_series_t *arr, int n)
{
    for (int i = 0; i < n; i++) {
        if (arr[i].label)        efree(arr[i].label);
        if (arr[i].values)       efree(arr[i].values);
        if (arr[i].values_max)   efree(arr[i].values_max);
        if (arr[i].point_colors) efree(arr[i].point_colors);
        arr[i].label = NULL;
        arr[i].values = NULL;
        arr[i].values_max = NULL;
        arr[i].point_colors = NULL;
        arr[i].len = 0;
    }
}
static void fastchart_series_array_addref(fastchart_series_t *arr, int n)
{
    for (int i = 0; i < n; i++) {
        if (arr[i].label) {
            size_t len = strlen(arr[i].label);
            char *copy = emalloc(len + 1);
            memcpy(copy, arr[i].label, len + 1);
            arr[i].label = copy;
        }
        int slot_len = arr[i].len;
        if (arr[i].values && slot_len > 0) {
            size_t bytes = (size_t)slot_len * sizeof(double);
            double *copy = emalloc(bytes);
            memcpy(copy, arr[i].values, bytes);
            arr[i].values = copy;
        }
        if (arr[i].values_max && slot_len > 0) {
            size_t bytes = (size_t)slot_len * sizeof(double);
            double *copy = emalloc(bytes);
            memcpy(copy, arr[i].values_max, bytes);
            arr[i].values_max = copy;
        }
        if (arr[i].point_colors && slot_len > 0) {
            size_t bytes = (size_t)slot_len * sizeof(zend_long);
            zend_long *copy = emalloc(bytes);
            memcpy(copy, arr[i].point_colors, bytes);
            arr[i].point_colors = copy;
        }
    }
}

/* Parse one user-facing series array into a typed slot. flags picks
 * which optional fields to read: bit0 colors, bit1 right_axis,
 * bit2 floating-bar [min,max] pairs. Returns 0 on success, -1 if
 * the input wasn't a usable series shape. */
#define FC_SERIES_F_COLORS    0x1
#define FC_SERIES_F_RIGHTAXIS 0x2
#define FC_SERIES_F_FLOATING  0x4
#define FC_SERIES_F_STRICT    0x8   /* error on non-numeric / non-null cells */

static void fastchart_series_dup_label(fastchart_series_t *out, const char *src)
{
    if (!src) { out->label = NULL; return; }
    size_t len = strlen(src);
    out->label = emalloc(len + 1);
    memcpy(out->label, src, len + 1);
}

static int fastchart_parse_series(zval *series_zv, fastchart_series_t *out, int flags)
{
    if (Z_TYPE_P(series_zv) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(series_zv);
    HashTable *data_ht = NULL;
    HashTable *colors_ht = NULL;
    bool right_axis = false;
    const char *label = NULL;

    /* Allow either { 'data' => [...], ... } or a flat numeric list. */
    zval *data_key = zend_hash_str_find(ht, "data", sizeof("data") - 1);
    if (data_key && Z_TYPE_P(data_key) == IS_ARRAY) {
        data_ht = Z_ARRVAL_P(data_key);
        zval *label_zv = zend_hash_str_find(ht, "label", sizeof("label") - 1);
        label = fastchart_label_or_null(label_zv);
        if (flags & FC_SERIES_F_COLORS) {
            zval *colors_zv = zend_hash_str_find(ht, "colors", sizeof("colors") - 1);
            if (colors_zv && Z_TYPE_P(colors_zv) == IS_ARRAY) {
                colors_ht = Z_ARRVAL_P(colors_zv);
            }
        }
        if (flags & FC_SERIES_F_RIGHTAXIS) {
            zval *axis_zv = zend_hash_str_find(ht, "axis", sizeof("axis") - 1);
            /* zend_string_equals_literal is length-aware: rejects
             * "right\0junk" that strcmp would accept. */
            right_axis = (axis_zv && Z_TYPE_P(axis_zv) == IS_STRING &&
                          zend_string_equals_literal(Z_STR_P(axis_zv), "right"));
        }
    } else {
        data_ht = ht;
    }

    uint32_t un = zend_hash_num_elements(data_ht);
    if (un > FASTCHART_MAX_POINTS_PER_SERIES) {
        zend_value_error("FastChart series accepts at most %d points per series; got %u",
                         FASTCHART_MAX_POINTS_PER_SERIES, un);
        return -1;
    }
    int n = (int)un;
    out->len = n;
    out->right_axis = right_axis;
    fastchart_series_dup_label(out, label);
    out->values = NULL;
    out->values_max = NULL;
    out->point_colors = NULL;
    if (n == 0) return 0;

    if (flags & FC_SERIES_F_FLOATING) {
        out->values = emalloc((size_t)n * sizeof(double));
        out->values_max = emalloc((size_t)n * sizeof(double));
        for (int i = 0; i < n; i++) {
            zval *v = zend_hash_index_find(data_ht, i);
            double lo = NAN, hi = NAN;
            if (v && Z_TYPE_P(v) == IS_ARRAY) {
                HashTable *pair = Z_ARRVAL_P(v);
                zval *zlo = zend_hash_index_find(pair, 0);
                zval *zhi = zend_hash_index_find(pair, 1);
                if (zlo && zhi) {
                    double l, h;
                    if (fastchart_zval_to_double(zlo, &l) == 0 &&
                        fastchart_zval_to_double(zhi, &h) == 0) {
                        if (l > h) { double t = l; l = h; h = t; }
                        lo = l; hi = h;
                    }
                }
            }
            out->values[i] = lo;
            out->values_max[i] = hi;
        }
    } else {
        out->values = emalloc((size_t)n * sizeof(double));
        for (int i = 0; i < n; i++) {
            zval *v = zend_hash_index_find(data_ht, i);
            double d;
            if (!v || Z_TYPE_P(v) == IS_NULL) {
                out->values[i] = NAN;
            } else if (fastchart_zval_to_double(v, &d) == 0) {
                out->values[i] = d;
            } else if (flags & FC_SERIES_F_STRICT) {
                zend_type_error("FastChart strict mode: series data must be numeric or null");
                /* Release the partial state we already allocated so
                 * the caller doesn't have to worry about half-built
                 * slots. label was emalloc'd in dup_label above. */
                if (out->label) { efree(out->label); out->label = NULL; }
                efree(out->values);
                out->values = NULL;
                out->len = 0;
                return -1;
            } else {
                out->values[i] = NAN;
            }
        }
    }

    if (colors_ht) {
        out->point_colors = emalloc((size_t)n * sizeof(zend_long));
        for (int i = 0; i < n; i++) {
            zval *cv = zend_hash_index_find(colors_ht, i);
            zend_long c = -1;
            if (cv && Z_TYPE_P(cv) == IS_LONG) c = Z_LVAL_P(cv);
            out->point_colors[i] = (c >= 0 && c <= 0xFFFFFF) ? c : -1;
        }
    }
    return 0;
}

/* Parse the user setSeries() input into self->series[]. Accepts
 * either a flat numeric list (single series) or a list of
 * series-dicts. Returns 0 on success, -1 on shape error. Caller
 * already cleared any previously-parsed state via the array
 * release helper. */
static int fastchart_collect_series_into(zval *arr, fastchart_series_t *out,
                                         int *out_n, int *out_max_len, int flags)
{
    *out_n = 0;
    *out_max_len = 0;
    if (Z_TYPE_P(arr) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(arr);
    int n = (int)zend_hash_num_elements(ht);
    if (n == 0) return 0;

    /* Multi-series detection: first element is an array with a 'data'
     * key. Single-series fallback: the input is itself the values. */
    zval *first = zend_hash_index_find(ht, 0);
    bool is_multi = false;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *dk = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (dk && Z_TYPE_P(dk) == IS_ARRAY) is_multi = true;
    }

    if (is_multi) {
        zval *series_zv;
        ZEND_HASH_FOREACH_VAL(ht, series_zv) {
            if (*out_n >= FASTCHART_MAX_SERIES) break;
            if (fastchart_parse_series(series_zv, &out[*out_n], flags) != 0) {
                /* Strict-mode TypeError leaves an exception pending;
                 * propagate it instead of silently skipping. */
                if (EG(exception)) return -1;
                continue;
            }
            if (out[*out_n].len > *out_max_len) *out_max_len = out[*out_n].len;
            (*out_n)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        /* Single flat numeric series. Wrap in a fake series_zv arg. */
        if (fastchart_parse_series(arr, &out[0], flags) != 0) return -1;
        *out_n = 1;
        *out_max_len = out[0].len;
    }
    return 0;
}

/* Per-class init / release / clone-addref helpers. */
static void fastchart_line_init_extras(fastchart_line_obj *o)
{
    fastchart_series_array_init(o->series, FASTCHART_MAX_SERIES);
    o->n_series = 0;
    o->max_len = 0;
    o->err_lo = NULL;
    o->err_hi = NULL;
    o->err_n  = 0;
}
static void fastchart_line_release_extras(fastchart_line_obj *o)
{
    fastchart_series_array_release(o->series, o->n_series);
    o->n_series = 0;
    o->max_len = 0;
    if (o->err_lo) efree(o->err_lo);
    if (o->err_hi) efree(o->err_hi);
    o->err_lo = NULL;
    o->err_hi = NULL;
    o->err_n  = 0;
}
static void fastchart_line_addref_extras(fastchart_line_obj *o)
{
    fastchart_series_array_addref(o->series, o->n_series);
    if (o->err_lo && o->err_n > 0) {
        size_t bytes = (size_t)o->err_n * sizeof(double);
        double *l = emalloc(bytes); memcpy(l, o->err_lo, bytes); o->err_lo = l;
        double *h = emalloc(bytes); memcpy(h, o->err_hi, bytes); o->err_hi = h;
    }
}

static void fastchart_area_init_extras(fastchart_area_obj *o)
{
    o->area_alpha = 64;
    o->stacked = false;
    fastchart_series_array_init(o->series, FASTCHART_MAX_SERIES);
    o->n_series = 0;
    o->max_len = 0;
}
static void fastchart_area_release_extras(fastchart_area_obj *o)
{
    fastchart_series_array_release(o->series, o->n_series);
    o->n_series = 0;
    o->max_len = 0;
}
static void fastchart_area_addref_extras(fastchart_area_obj *o)
{
    fastchart_series_array_addref(o->series, o->n_series);
}

static void fastchart_bar_init_extras(fastchart_bar_obj *o)
{
    o->stack_mode = FASTCHART_STACK_SUM;
    o->bar_floating = false;
    o->stacked = false;
    o->bar_orientation = FASTCHART_BAR_VERTICAL;
    fastchart_series_array_init(o->series, FASTCHART_MAX_SERIES);
    o->n_series = 0;
    o->max_len = 0;
}
static void fastchart_bar_release_extras(fastchart_bar_obj *o)
{
    fastchart_series_array_release(o->series, o->n_series);
    o->n_series = 0;
    o->max_len = 0;
}
static void fastchart_bar_addref_extras(fastchart_bar_obj *o)
{
    fastchart_series_array_addref(o->series, o->n_series);
}

/* dup/release helpers shared across the typed-storage classes. */
static char *fc_strdup_opt(const char *src)
{
    if (!src) return NULL;
    size_t len = strlen(src);
    char *copy = emalloc(len + 1);
    memcpy(copy, src, len + 1);
    return copy;
}
static void fc_efree_opt(void *p) { if (p) efree(p); }

static void fastchart_pie_init_extras(fastchart_pie_obj *o)
{
    o->slice_label_position = FASTCHART_LABEL_INSIDE;
    o->slice_label_format = NULL;
    o->pie_other_threshold = 0.0;
    o->pie_other_label = NULL;
    o->slices = NULL;
    o->slice_count = 0;
    o->total = 0.0;
    o->donut_hole_ratio = 0.0;
    o->explode = NULL;
    o->explode_count = 0;
}
static void fastchart_pie_release_extras(fastchart_pie_obj *o)
{
    if (o->slice_label_format) zend_string_release(o->slice_label_format);
    if (o->pie_other_label)    zend_string_release(o->pie_other_label);
    if (o->slices) {
        for (int i = 0; i < o->slice_count; i++) {
            if (o->slices[i].label) efree(o->slices[i].label);
        }
        efree(o->slices);
        o->slices = NULL;
    }
    if (o->explode) { efree(o->explode); o->explode = NULL; }
    o->slice_count = 0;
    o->explode_count = 0;
}
static void fastchart_pie_addref_extras(fastchart_pie_obj *o)
{
    if (o->slice_label_format) zend_string_addref(o->slice_label_format);
    if (o->pie_other_label)    zend_string_addref(o->pie_other_label);
    if (o->slices && o->slice_count > 0) {
        size_t bytes = (size_t)o->slice_count * sizeof(fastchart_pie_slice);
        fastchart_pie_slice *copy = emalloc(bytes);
        memcpy(copy, o->slices, bytes);
        for (int i = 0; i < o->slice_count; i++) {
            copy[i].label = fc_strdup_opt(o->slices[i].label);
        }
        o->slices = copy;
    } else {
        o->slices = NULL;
    }
    if (o->explode && o->explode_count > 0) {
        size_t bytes = (size_t)o->explode_count * sizeof(zend_long);
        zend_long *copy = emalloc(bytes);
        memcpy(copy, o->explode, bytes);
        o->explode = copy;
    } else {
        o->explode = NULL;
    }
}

static void fastchart_scatter_init_extras(fastchart_scatter_obj *o)
{
    o->trend_line = false;
    o->trend_line_color = -1;
    o->trend_degree = 1;
    o->points = NULL;
    o->point_count = 0;
    o->n_series = 0;
    for (int i = 0; i < FASTCHART_MAX_SCATTER_SERIES; i++) {
        o->series_labels[i] = NULL;
    }
    o->err_lo = NULL;
    o->err_hi = NULL;
    o->err_n  = 0;
    o->image_map_areas = NULL;
    o->n_image_map_areas = 0;
}
static void fastchart_scatter_release_extras(fastchart_scatter_obj *o)
{
    if (o->points) {
        for (int i = 0; i < o->point_count; i++) {
            if (o->points[i].href) efree(o->points[i].href);
            if (o->points[i].tooltip) efree(o->points[i].tooltip);
        }
        efree(o->points);
        o->points = NULL;
    }
    o->point_count = 0;
    for (int i = 0; i < FASTCHART_MAX_SCATTER_SERIES; i++) {
        if (o->series_labels[i]) {
            efree(o->series_labels[i]);
            o->series_labels[i] = NULL;
        }
    }
    o->n_series = 0;
    if (o->err_lo) efree(o->err_lo);
    if (o->err_hi) efree(o->err_hi);
    o->err_lo = NULL;
    o->err_hi = NULL;
    o->err_n  = 0;
    /* Image-map areas borrow href/tooltip from points[i]; only the
     * areas array itself is owned. */
    if (o->image_map_areas) efree(o->image_map_areas);
    o->image_map_areas = NULL;
    o->n_image_map_areas = 0;
}
static void fastchart_scatter_addref_extras(fastchart_scatter_obj *o)
{
    if (o->points && o->point_count > 0) {
        size_t bytes = (size_t)o->point_count * sizeof(fastchart_scatter_point);
        fastchart_scatter_point *copy = emalloc(bytes);
        memcpy(copy, o->points, bytes);
        for (int i = 0; i < o->point_count; i++) {
            copy[i].href = fc_strdup_opt(o->points[i].href);
            copy[i].tooltip = fc_strdup_opt(o->points[i].tooltip);
        }
        o->points = copy;
    } else {
        o->points = NULL;
    }
    for (int i = 0; i < o->n_series; i++) {
        o->series_labels[i] = fc_strdup_opt(o->series_labels[i]);
    }
    if (o->err_lo && o->err_n > 0) {
        size_t bytes = (size_t)o->err_n * sizeof(double);
        double *l = emalloc(bytes); memcpy(l, o->err_lo, bytes); o->err_lo = l;
        double *h = emalloc(bytes); memcpy(h, o->err_hi, bytes); o->err_hi = h;
    }
    /* Image-map areas are a render artifact — clear on the clone so
     * the cloned chart starts with no cached areas. The next draw on
     * the clone will repopulate them with fresh href/tooltip pointers
     * borrowed from the cloned points[]. Carrying over the original
     * areas would leave stale pointers into the original points array
     * (which is no longer reachable after this addref). */
    o->image_map_areas = NULL;
    o->n_image_map_areas = 0;
}

static void fastchart_stock_init_extras(fastchart_stock_obj *o)
{
    o->candle_style = FASTCHART_STYLE_CANDLE;
    o->candles = NULL;
    o->candle_count = 0;
    o->any_volume = false;
    o->volume_pane = false;
    o->volume_colors = NULL;
    o->volume_colors_count = 0;
    o->sma_count = 0;
    for (int i = 0; i < FASTCHART_MAX_SMA; i++) o->sma_types[i] = FASTCHART_MA_SMA;
    o->indicator_pane_count = 0;
    for (int i = 0; i < FASTCHART_MAX_INDICATOR_PANES; i++) {
        o->indicator_panes[i].name = NULL;
        o->indicator_panes[i].values = NULL;
        o->indicator_panes[i].value_count = 0;
        o->indicator_panes[i].values2 = NULL;
        o->indicator_panes[i].values3 = NULL;
        o->indicator_panes[i].color2_rgb = -1;
        o->indicator_panes[i].color3_rgb = -1;
        o->indicator_panes[i].histogram_third = false;
    }
    o->overlay_count = 0;
    for (int i = 0; i < FASTCHART_MAX_PRICE_OVERLAYS; i++) {
        o->overlays[i].a = NULL;
        o->overlays[i].b = NULL;
        o->overlays[i].c = NULL;
        o->overlays[i].n = 0;
    }
}
static void fastchart_stock_release_extras(fastchart_stock_obj *o)
{
    if (o->candles)        { efree(o->candles);        o->candles = NULL; }
    if (o->volume_colors)  { efree(o->volume_colors);  o->volume_colors = NULL; }
    for (int i = 0; i < o->indicator_pane_count; i++) {
        if (o->indicator_panes[i].name)    efree(o->indicator_panes[i].name);
        if (o->indicator_panes[i].values)  efree(o->indicator_panes[i].values);
        if (o->indicator_panes[i].values2) efree(o->indicator_panes[i].values2);
        if (o->indicator_panes[i].values3) efree(o->indicator_panes[i].values3);
        o->indicator_panes[i].name = NULL;
        o->indicator_panes[i].values = NULL;
        o->indicator_panes[i].values2 = NULL;
        o->indicator_panes[i].values3 = NULL;
    }
    for (int i = 0; i < o->overlay_count; i++) {
        if (o->overlays[i].a) efree(o->overlays[i].a);
        if (o->overlays[i].b) efree(o->overlays[i].b);
        if (o->overlays[i].c) efree(o->overlays[i].c);
        o->overlays[i].a = NULL;
        o->overlays[i].b = NULL;
        o->overlays[i].c = NULL;
    }
    o->candle_count = 0;
    o->volume_colors_count = 0;
    o->indicator_pane_count = 0;
    o->overlay_count = 0;
    o->sma_count = 0;
}
/* Clone deep-copies the malloc'd candle / volume_color / indicator-
 * pane storage. After fastchart_DEFINE_LIFECYCLE's memcpy of the POD
 * region, dst points at src's allocations; this function replaces
 * those with fresh copies so source and clone don't alias. */
static void fastchart_stock_addref_extras(fastchart_stock_obj *o)
{
    if (o->candles && o->candle_count > 0) {
        size_t bytes = (size_t)o->candle_count * sizeof(fastchart_candle);
        fastchart_candle *copy = emalloc(bytes);
        memcpy(copy, o->candles, bytes);
        o->candles = copy;
    } else {
        o->candles = NULL;
    }
    if (o->volume_colors && o->volume_colors_count > 0) {
        size_t bytes = (size_t)o->volume_colors_count * sizeof(int);
        int *copy = emalloc(bytes);
        memcpy(copy, o->volume_colors, bytes);
        o->volume_colors = copy;
    } else {
        o->volume_colors = NULL;
    }
    for (int i = 0; i < o->indicator_pane_count; i++) {
        fastchart_indicator_pane *p = &o->indicator_panes[i];
        if (p->name) {
            size_t name_len = strlen(p->name);
            char *copy = emalloc(name_len + 1);
            memcpy(copy, p->name, name_len + 1);
            p->name = copy;
        }
        size_t vbytes = (size_t)p->value_count * sizeof(double);
        if (p->values && p->value_count > 0) {
            double *copy = emalloc(vbytes);
            memcpy(copy, p->values, vbytes);
            p->values = copy;
        } else { p->values = NULL; }
        if (p->values2 && p->value_count > 0) {
            double *copy = emalloc(vbytes);
            memcpy(copy, p->values2, vbytes);
            p->values2 = copy;
        } else { p->values2 = NULL; }
        if (p->values3 && p->value_count > 0) {
            double *copy = emalloc(vbytes);
            memcpy(copy, p->values3, vbytes);
            p->values3 = copy;
        } else { p->values3 = NULL; }
    }
    for (int i = 0; i < o->overlay_count; i++) {
        fastchart_price_overlay *ov = &o->overlays[i];
        size_t bytes = (size_t)ov->n * sizeof(double);
        if (ov->a && ov->n > 0) {
            double *copy = emalloc(bytes);
            memcpy(copy, ov->a, bytes);
            ov->a = copy;
        } else { ov->a = NULL; }
        if (ov->b && ov->n > 0) {
            double *copy = emalloc(bytes);
            memcpy(copy, ov->b, bytes);
            ov->b = copy;
        } else { ov->b = NULL; }
        if (ov->c && ov->n > 0) {
            double *copy = emalloc(bytes);
            memcpy(copy, ov->c, bytes);
            ov->c = copy;
        } else { ov->c = NULL; }
    }
}

static void fastchart_radar_init_extras(fastchart_radar_obj *o)
{
    o->radar_max = 0.0;
    o->radar_filled = true;
    o->n_series = 0;
    for (int i = 0; i < FASTCHART_MAX_RADAR_SERIES; i++) {
        o->series[i].values = NULL;
        o->series[i].len = 0;
        o->series[i].label = NULL;
        o->series[i].color_rgb = -1;
    }
    o->categories = NULL;
    o->n_categories = 0;
}
static void fastchart_radar_release_extras(fastchart_radar_obj *o)
{
    for (int i = 0; i < o->n_series; i++) {
        fc_efree_opt(o->series[i].values);
        fc_efree_opt(o->series[i].label);
        o->series[i].values = NULL;
        o->series[i].label = NULL;
        o->series[i].len = 0;
    }
    o->n_series = 0;
    if (o->categories) {
        for (int i = 0; i < o->n_categories; i++) fc_efree_opt(o->categories[i]);
        efree(o->categories);
        o->categories = NULL;
    }
    o->n_categories = 0;
}
static void fastchart_radar_addref_extras(fastchart_radar_obj *o)
{
    for (int i = 0; i < o->n_series; i++) {
        if (o->series[i].values && o->series[i].len > 0) {
            size_t bytes = (size_t)o->series[i].len * sizeof(double);
            double *copy = emalloc(bytes);
            memcpy(copy, o->series[i].values, bytes);
            o->series[i].values = copy;
        }
        o->series[i].label = fc_strdup_opt(o->series[i].label);
    }
    if (o->categories && o->n_categories > 0) {
        char **copy = emalloc((size_t)o->n_categories * sizeof(char *));
        for (int i = 0; i < o->n_categories; i++) {
            copy[i] = fc_strdup_opt(o->categories[i]);
        }
        o->categories = copy;
    }
}

static void fastchart_bubble_init_extras(fastchart_bubble_obj *o)
{
    o->points = NULL;
    o->point_count = 0;
}
static void fastchart_bubble_release_extras(fastchart_bubble_obj *o)
{
    if (o->points) { efree(o->points); o->points = NULL; }
    o->point_count = 0;
}
static void fastchart_bubble_addref_extras(fastchart_bubble_obj *o)
{
    if (o->points && o->point_count > 0) {
        size_t bytes = (size_t)o->point_count * sizeof(fastchart_bubble_point);
        fastchart_bubble_point *copy = emalloc(bytes);
        memcpy(copy, o->points, bytes);
        o->points = copy;
    } else {
        o->points = NULL;
    }
}

static void fastchart_surface_init_extras(fastchart_surface_obj *o)
{
    o->surface_show_values = false;
    o->surface_value_format = NULL;
    o->grid.cells = NULL;
    o->grid.rows = 0;
    o->grid.cols = 0;
}
static void fastchart_surface_release_extras(fastchart_surface_obj *o)
{
    if (o->surface_value_format) zend_string_release(o->surface_value_format);
    if (o->grid.cells) { efree(o->grid.cells); o->grid.cells = NULL; }
    o->grid.rows = 0;
    o->grid.cols = 0;
}
static void fastchart_surface_addref_extras(fastchart_surface_obj *o)
{
    if (o->surface_value_format) zend_string_addref(o->surface_value_format);
    if (o->grid.cells && o->grid.rows > 0 && o->grid.cols > 0) {
        size_t bytes = (size_t)o->grid.rows * (size_t)o->grid.cols * sizeof(double);
        double *copy = emalloc(bytes);
        memcpy(copy, o->grid.cells, bytes);
        o->grid.cells = copy;
    } else {
        o->grid.cells = NULL;
    }
}

static void fastchart_gauge_init_extras(fastchart_gauge_obj *o)
{
    o->gauge_value = 0.0;
    o->gauge_min = 0.0;
    o->gauge_max = 100.0;
    o->gauge_value_format = NULL;
    o->zones = NULL;
    o->n_zones = 0;
}
static void fastchart_gauge_release_extras(fastchart_gauge_obj *o)
{
    if (o->gauge_value_format) zend_string_release(o->gauge_value_format);
    if (o->zones) efree(o->zones);
    o->zones = NULL;
    o->n_zones = 0;
}
static void fastchart_gauge_addref_extras(fastchart_gauge_obj *o)
{
    if (o->gauge_value_format) zend_string_addref(o->gauge_value_format);
    if (o->zones && o->n_zones > 0) {
        size_t bytes = (size_t)o->n_zones * sizeof(fastchart_gauge_zone);
        fastchart_gauge_zone *copy = emalloc(bytes);
        memcpy(copy, o->zones, bytes);
        o->zones = copy;
    }
}

static void fastchart_gantt_init_extras(fastchart_gantt_obj *o)
{
    o->gantt_show_labels = true;
    o->gantt_has_range = false;
    o->gantt_range_start = 0;
    o->gantt_range_end = 0;
    o->tasks = NULL;
    o->task_count = 0;
}
static void fastchart_gantt_release_extras(fastchart_gantt_obj *o)
{
    if (o->tasks) {
        for (int i = 0; i < o->task_count; i++) {
            fc_efree_opt(o->tasks[i].name);
            fc_efree_opt(o->tasks[i].deps);
        }
        efree(o->tasks);
        o->tasks = NULL;
    }
    o->task_count = 0;
}
static void fastchart_gantt_addref_extras(fastchart_gantt_obj *o)
{
    if (o->tasks && o->task_count > 0) {
        size_t bytes = (size_t)o->task_count * sizeof(fastchart_gantt_task);
        fastchart_gantt_task *copy = emalloc(bytes);
        memcpy(copy, o->tasks, bytes);
        for (int i = 0; i < o->task_count; i++) {
            copy[i].name = fc_strdup_opt(o->tasks[i].name);
            if (o->tasks[i].deps && o->tasks[i].n_deps > 0) {
                size_t db = (size_t)o->tasks[i].n_deps * sizeof(int);
                int *dcopy = emalloc(db);
                memcpy(dcopy, o->tasks[i].deps, db);
                copy[i].deps = dcopy;
            } else {
                copy[i].deps = NULL;
            }
        }
        o->tasks = copy;
    } else {
        o->tasks = NULL;
    }
}

static void fastchart_boxplot_init_extras(fastchart_boxplot_obj *o)
{
    o->box_width_pct = 60;
    o->entries = NULL;
    o->entry_count = 0;
}
static void fastchart_boxplot_release_extras(fastchart_boxplot_obj *o)
{
    if (o->entries) {
        for (int i = 0; i < o->entry_count; i++) {
            fc_efree_opt(o->entries[i].label);
            fc_efree_opt(o->entries[i].outliers);
        }
        efree(o->entries);
        o->entries = NULL;
    }
    o->entry_count = 0;
}
static void fastchart_boxplot_addref_extras(fastchart_boxplot_obj *o)
{
    if (o->entries && o->entry_count > 0) {
        size_t bytes = (size_t)o->entry_count * sizeof(fastchart_boxplot_entry);
        fastchart_boxplot_entry *copy = emalloc(bytes);
        memcpy(copy, o->entries, bytes);
        for (int i = 0; i < o->entry_count; i++) {
            copy[i].label = fc_strdup_opt(o->entries[i].label);
            if (o->entries[i].outliers && o->entries[i].outlier_count > 0) {
                size_t obytes = (size_t)o->entries[i].outlier_count * sizeof(double);
                double *ocopy = emalloc(obytes);
                memcpy(ocopy, o->entries[i].outliers, obytes);
                copy[i].outliers = ocopy;
            } else {
                copy[i].outliers = NULL;
            }
        }
        o->entries = copy;
    } else {
        o->entries = NULL;
    }
}

static void fastchart_polar_init_extras(fastchart_polar_obj *o)
{
    o->polar_max_radius = 0.0;
    o->polar_filled = true;
    o->polar_style = FASTCHART_POLAR_STYLE_LINE;
    o->n_series = 0;
    for (int i = 0; i < FASTCHART_MAX_POLAR_SERIES; i++) {
        o->series[i].angles = NULL;
        o->series[i].radii = NULL;
        o->series[i].len = 0;
        o->series[i].label = NULL;
        o->series[i].color_rgb = -1;
    }
}
static void fastchart_polar_release_extras(fastchart_polar_obj *o)
{
    for (int i = 0; i < o->n_series; i++) {
        fc_efree_opt(o->series[i].angles);
        fc_efree_opt(o->series[i].radii);
        fc_efree_opt(o->series[i].label);
        o->series[i].angles = NULL;
        o->series[i].radii = NULL;
        o->series[i].label = NULL;
        o->series[i].len = 0;
    }
    o->n_series = 0;
}
static void fastchart_polar_addref_extras(fastchart_polar_obj *o)
{
    for (int i = 0; i < o->n_series; i++) {
        int len = o->series[i].len;
        if (o->series[i].angles && len > 0) {
            size_t bytes = (size_t)len * sizeof(double);
            double *copy = emalloc(bytes);
            memcpy(copy, o->series[i].angles, bytes);
            o->series[i].angles = copy;
        }
        if (o->series[i].radii && len > 0) {
            size_t bytes = (size_t)len * sizeof(double);
            double *copy = emalloc(bytes);
            memcpy(copy, o->series[i].radii, bytes);
            o->series[i].radii = copy;
        }
        o->series[i].label = fc_strdup_opt(o->series[i].label);
    }
}

static void fastchart_contour_init_extras(fastchart_contour_obj *o)
{
    o->contour_filled = true;
    o->grid.cells = NULL;
    o->grid.rows = 0;
    o->grid.cols = 0;
    o->levels = NULL;
    o->level_count = 0;
}
static void fastchart_contour_release_extras(fastchart_contour_obj *o)
{
    if (o->grid.cells) { efree(o->grid.cells); o->grid.cells = NULL; }
    if (o->levels)     { efree(o->levels);     o->levels = NULL; }
    o->grid.rows = 0;
    o->grid.cols = 0;
    o->level_count = 0;
}
static void fastchart_contour_addref_extras(fastchart_contour_obj *o)
{
    if (o->grid.cells && o->grid.rows > 0 && o->grid.cols > 0) {
        size_t bytes = (size_t)o->grid.rows * (size_t)o->grid.cols * sizeof(double);
        double *copy = emalloc(bytes);
        memcpy(copy, o->grid.cells, bytes);
        o->grid.cells = copy;
    } else {
        o->grid.cells = NULL;
    }
    if (o->levels && o->level_count > 0) {
        size_t bytes = (size_t)o->level_count * sizeof(double);
        double *copy = emalloc(bytes);
        memcpy(copy, o->levels, bytes);
        o->levels = copy;
    } else {
        o->levels = NULL;
    }
}

static void fastchart_treemap_init_extras(fastchart_treemap_obj *o)
{
    o->items = NULL;
    o->item_count = 0;
    o->show_labels = true;
}
static void fastchart_treemap_release_extras(fastchart_treemap_obj *o)
{
    if (o->items) {
        for (int i = 0; i < o->item_count; i++) {
            if (o->items[i].label) efree(o->items[i].label);
        }
        efree(o->items);
        o->items = NULL;
    }
    o->item_count = 0;
}
static void fastchart_treemap_addref_extras(fastchart_treemap_obj *o)
{
    if (o->items && o->item_count > 0) {
        size_t bytes = (size_t)o->item_count * sizeof(fastchart_treemap_item);
        fastchart_treemap_item *copy = emalloc(bytes);
        memcpy(copy, o->items, bytes);
        for (int i = 0; i < o->item_count; i++) {
            copy[i].label = fc_strdup_opt(o->items[i].label);
        }
        o->items = copy;
    } else {
        o->items = NULL;
    }
}

static void fastchart_funnel_init_extras(fastchart_funnel_obj *o)
{
    o->stages = NULL;
    o->stage_count = 0;
    /* Override the base default (false) — funnels typically want
     * the per-stage value rendered next to the label. */
    ((fastchart_obj *)o)->show_values = true;
}
static void fastchart_funnel_release_extras(fastchart_funnel_obj *o)
{
    if (o->stages) {
        for (int i = 0; i < o->stage_count; i++) {
            if (o->stages[i].label) efree(o->stages[i].label);
        }
        efree(o->stages);
        o->stages = NULL;
    }
    o->stage_count = 0;
}
static void fastchart_funnel_addref_extras(fastchart_funnel_obj *o)
{
    if (o->stages && o->stage_count > 0) {
        size_t bytes = (size_t)o->stage_count * sizeof(fastchart_funnel_stage);
        fastchart_funnel_stage *copy = emalloc(bytes);
        memcpy(copy, o->stages, bytes);
        for (int i = 0; i < o->stage_count; i++) {
            copy[i].label = fc_strdup_opt(o->stages[i].label);
        }
        o->stages = copy;
    } else {
        o->stages = NULL;
    }
}

static void fastchart_waterfall_init_extras(fastchart_waterfall_obj *o)
{
    o->bars = NULL;
    o->bar_count = 0;
    o->rise_color = -1;
    o->fall_color = -1;
    o->total_color = -1;
}
static void fastchart_waterfall_release_extras(fastchart_waterfall_obj *o)
{
    if (o->bars) {
        for (int i = 0; i < o->bar_count; i++) {
            if (o->bars[i].label) efree(o->bars[i].label);
        }
        efree(o->bars);
        o->bars = NULL;
    }
    o->bar_count = 0;
}
static void fastchart_waterfall_addref_extras(fastchart_waterfall_obj *o)
{
    if (o->bars && o->bar_count > 0) {
        size_t bytes = (size_t)o->bar_count * sizeof(fastchart_waterfall_bar);
        fastchart_waterfall_bar *copy = emalloc(bytes);
        memcpy(copy, o->bars, bytes);
        for (int i = 0; i < o->bar_count; i++) {
            copy[i].label = fc_strdup_opt(o->bars[i].label);
        }
        o->bars = copy;
    } else {
        o->bars = NULL;
    }
}

static void fastchart_heatmap_init_extras(fastchart_heatmap_obj *o)
{
    o->grid.cells = NULL;
    o->grid.rows = 0;
    o->grid.cols = 0;
    o->color_low_rgb = -1;
    o->color_high_rgb = -1;
    /* show_values + value_format are base fields; the base init
     * already sets them to false / NULL. */
}
static void fastchart_heatmap_release_extras(fastchart_heatmap_obj *o)
{
    if (o->grid.cells) { efree(o->grid.cells); o->grid.cells = NULL; }
    /* base release frees value_format. */
    o->grid.rows = 0;
    o->grid.cols = 0;
}
static void fastchart_heatmap_addref_extras(fastchart_heatmap_obj *o)
{
    if (o->grid.cells && o->grid.rows > 0 && o->grid.cols > 0) {
        size_t bytes = (size_t)o->grid.rows * (size_t)o->grid.cols * sizeof(double);
        double *copy = emalloc(bytes);
        memcpy(copy, o->grid.cells, bytes);
        o->grid.cells = copy;
    } else {
        o->grid.cells = NULL;
    }
    /* base addref bumps value_format refcount. */
}

static void fastchart_linear_meter_init_extras(fastchart_linear_meter_obj *o)
{
    o->meter_value = 0.0;
    o->meter_min = 0.0;
    o->meter_max = 100.0;
    o->meter_orientation = FASTCHART_METER_HORIZONTAL;
    o->n_zones = 0;
    o->meter_value_format = NULL;
}
static void fastchart_linear_meter_release_extras(fastchart_linear_meter_obj *o)
{
    if (o->meter_value_format) zend_string_release(o->meter_value_format);
}
static void fastchart_linear_meter_addref_extras(fastchart_linear_meter_obj *o)
{
    if (o->meter_value_format) zend_string_addref(o->meter_value_format);
}

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
FASTCHART_DEFINE_LIFECYCLE(treemap, fastchart_treemap_obj)
FASTCHART_DEFINE_LIFECYCLE(funnel,  fastchart_funnel_obj)
FASTCHART_DEFINE_LIFECYCLE(waterfall, fastchart_waterfall_obj)
FASTCHART_DEFINE_LIFECYCLE(heatmap, fastchart_heatmap_obj)
FASTCHART_DEFINE_LIFECYCLE(linear_meter, fastchart_linear_meter_obj)

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
static const char *FASTCHART_DEFAULT_FONT_CANDIDATES[] = {
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
    for (int i = 0; FASTCHART_DEFAULT_FONT_CANDIDATES[i]; i++) {
        if (stat(FASTCHART_DEFAULT_FONT_CANDIDATES[i], &st) == 0 && S_ISREG(st.st_mode)) {
            return zend_string_init(FASTCHART_DEFAULT_FONT_CANDIDATES[i],
                                    strlen(FASTCHART_DEFAULT_FONT_CANDIDATES[i]),
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
    HashTable *ht = Z_ARRVAL_P(labels);
    uint32_t n = zend_hash_num_elements(ht);
    if (n > FASTCHART_MAX_CATEGORY_LABELS) n = FASTCHART_MAX_CATEGORY_LABELS;

    fastchart_category_labels_free(self);
    if (n == 0) {
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }

    /* Materialize each label into an owned char* slot. Non-string
     * cells become NULL so renderers can skip them; embedded NUL is
     * not allowed. */
    char **slots = ecalloc((size_t)n, sizeof(char *));
    uint32_t idx = 0;
    zval *lv;
    ZEND_HASH_FOREACH_VAL(ht, lv) {
        if (idx >= n) break;
        if (Z_TYPE_P(lv) == IS_STRING) {
            zend_string *zs = Z_STR_P(lv);
            const char *src = ZSTR_VAL(zs);
            size_t len = ZSTR_LEN(zs);
            if (memchr(src, '\0', len) == NULL) {
                slots[idx] = emalloc(len + 1);
                memcpy(slots[idx], src, len);
                slots[idx][len] = '\0';
            }
        } else if (Z_TYPE_P(lv) == IS_LONG) {
            char buf[32];
            int blen = snprintf(buf, sizeof(buf), ZEND_LONG_FMT, Z_LVAL_P(lv));
            if (blen > 0 && blen < (int)sizeof(buf)) {
                slots[idx] = emalloc((size_t)blen + 1);
                memcpy(slots[idx], buf, (size_t)blen + 1);
            }
        } else if (Z_TYPE_P(lv) == IS_DOUBLE) {
            char buf[64];
            int blen = snprintf(buf, sizeof(buf), "%g", Z_DVAL_P(lv));
            if (blen > 0 && blen < (int)sizeof(buf)) {
                slots[idx] = emalloc((size_t)blen + 1);
                memcpy(slots[idx], buf, (size_t)blen + 1);
            }
        }
        idx++;
    } ZEND_HASH_FOREACH_END();

    self->category_labels = slots;
    self->n_category_labels = (int)n;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Validate that `var_` is a 24-bit RGB int (0..0xFFFFFF), throwing a
 * ValueError that names `method_str_` if not. The _OR_DEFAULT variant
 * additionally accepts -1 as the "use the theme default" sentinel.
 * `method_str_` is the qualified method name as a string literal so
 * message wording stays consistent across ~13 setters that previously
 * each spelled the same range and boundary slightly differently. */
#define FASTCHART_VALIDATE_RGB(var_, method_str_) do { \
    if ((var_) < 0 || (var_) > 0xFFFFFF) { \
        zend_value_error(method_str_ "() expects a 24-bit RGB int (0..0xFFFFFF)"); \
        RETURN_THROWS(); \
    } \
} while (0)

#define FASTCHART_VALIDATE_RGB_OR_DEFAULT(var_, method_str_) do { \
    if ((var_) < -1 || (var_) > 0xFFFFFF) { \
        zend_value_error(method_str_ "() expects -1 (theme default) or a 24-bit RGB int (0..0xFFFFFF)"); \
        RETURN_THROWS(); \
    } \
} while (0)

ZEND_METHOD(FastChart_Chart, setBackgroundColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(rgb)
    ZEND_PARSE_PARAMETERS_END();
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(rgb, "FastChart\\Chart::setBackgroundColor");
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
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(rgb, "FastChart\\Chart::setPlotBackgroundColor");
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
        FASTCHART_VALIDATE_RGB(c, "FastChart\\Chart::setSeriesColors");
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
    if (!color_is_null) {
        FASTCHART_VALIDATE_RGB(color, "FastChart\\Chart::addHorizontalLine");
    }
    if (label && memchr(ZSTR_VAL(label), 0, ZSTR_LEN(label)) != NULL) {
        zend_value_error("FastChart\\Chart::addHorizontalLine() label contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    push_annotation(self, "h", value, label, !color_is_null, color);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, addHorizontalBand)
{
    double low, high;
    zend_long color;
    zend_long alpha = 64;
    zend_string *label = NULL;

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_DOUBLE(low)
        Z_PARAM_DOUBLE(high)
        Z_PARAM_LONG(color)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(alpha)
        Z_PARAM_STR_OR_NULL(label)
    ZEND_PARSE_PARAMETERS_END();

    if (fastchart_reject_non_finite(low, "FastChart\\Chart::addHorizontalBand()") != 0 ||
        fastchart_reject_non_finite(high, "FastChart\\Chart::addHorizontalBand()") != 0) {
        RETURN_THROWS();
    }
    FASTCHART_VALIDATE_RGB(color, "FastChart\\Chart::addHorizontalBand");
    if (alpha < 0 || alpha > 127) {
        zend_value_error("FastChart\\Chart::addHorizontalBand() alpha out of range; expected 0..127");
        RETURN_THROWS();
    }
    if (label && memchr(ZSTR_VAL(label), 0, ZSTR_LEN(label)) != NULL) {
        zend_value_error("FastChart\\Chart::addHorizontalBand() label contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->n_plot_bands >= FASTCHART_MAX_BANDS) {
        zend_value_error("FastChart\\Chart::addHorizontalBand() supports at most %d bands",
                         FASTCHART_MAX_BANDS);
        RETURN_THROWS();
    }

    /* Grow the array one slot at a time. erealloc is fine here since
     * the cap is small (16) and bands are added at chart-config time,
     * not in a hot loop. */
    size_t new_bytes = (size_t)(self->n_plot_bands + 1) * sizeof(fastchart_plot_band);
    self->plot_bands = self->plot_bands
        ? erealloc(self->plot_bands, new_bytes)
        : emalloc(new_bytes);

    fastchart_plot_band *band = &self->plot_bands[self->n_plot_bands];
    band->low = low < high ? low : high;
    band->high = low < high ? high : low;
    band->color_rgb = (int)color;
    band->alpha = (int)alpha;
    band->label = NULL;
    band->is_vertical = false;
    if (label) {
        size_t len = ZSTR_LEN(label);
        band->label = emalloc(len + 1);
        memcpy(band->label, ZSTR_VAL(label), len);
        band->label[len] = '\0';
    }
    self->n_plot_bands++;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, addVerticalBand)
{
    double low, high;
    zend_long color;
    zend_long alpha = 64;
    zend_string *label = NULL;

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_DOUBLE(low)
        Z_PARAM_DOUBLE(high)
        Z_PARAM_LONG(color)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(alpha)
        Z_PARAM_STR_OR_NULL(label)
    ZEND_PARSE_PARAMETERS_END();

    if (fastchart_reject_non_finite(low, "FastChart\\Chart::addVerticalBand()") != 0 ||
        fastchart_reject_non_finite(high, "FastChart\\Chart::addVerticalBand()") != 0) {
        RETURN_THROWS();
    }
    FASTCHART_VALIDATE_RGB(color, "FastChart\\Chart::addVerticalBand");
    if (alpha < 0 || alpha > 127) {
        zend_value_error("FastChart\\Chart::addVerticalBand() alpha out of range; expected 0..127");
        RETURN_THROWS();
    }
    if (label && memchr(ZSTR_VAL(label), 0, ZSTR_LEN(label)) != NULL) {
        zend_value_error("FastChart\\Chart::addVerticalBand() label contains an embedded NUL");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->n_plot_bands >= FASTCHART_MAX_BANDS) {
        zend_value_error("FastChart\\Chart::addVerticalBand() supports at most %d bands",
                         FASTCHART_MAX_BANDS);
        RETURN_THROWS();
    }

    size_t new_bytes = (size_t)(self->n_plot_bands + 1) * sizeof(fastchart_plot_band);
    self->plot_bands = self->plot_bands
        ? erealloc(self->plot_bands, new_bytes)
        : emalloc(new_bytes);

    fastchart_plot_band *band = &self->plot_bands[self->n_plot_bands];
    band->low = low < high ? low : high;
    band->high = low < high ? high : low;
    band->color_rgb = (int)color;
    band->alpha = (int)alpha;
    band->label = NULL;
    band->is_vertical = true;
    if (label) {
        size_t len = ZSTR_LEN(label);
        band->label = emalloc(len + 1);
        memcpy(band->label, ZSTR_VAL(label), len);
        band->label[len] = '\0';
    }
    self->n_plot_bands++;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Chart, addIconAt)
{
    double x, y;
    zend_string *path;
    zend_long max_w = -1, max_h = -1;

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_DOUBLE(x)
        Z_PARAM_DOUBLE(y)
        Z_PARAM_STR(path)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(max_w)
        Z_PARAM_LONG(max_h)
    ZEND_PARSE_PARAMETERS_END();

    if (fastchart_reject_non_finite(x, "FastChart\\Chart::addIconAt()") != 0 ||
        fastchart_reject_non_finite(y, "FastChart\\Chart::addIconAt()") != 0) {
        RETURN_THROWS();
    }
    if (ZSTR_LEN(path) == 0 ||
        memchr(ZSTR_VAL(path), 0, ZSTR_LEN(path)) != NULL) {
        zend_value_error("FastChart\\Chart::addIconAt() path must be non-empty and NUL-free");
        RETURN_THROWS();
    }
    if ((max_w != -1 && (max_w < 1 || max_w > 4096)) ||
        (max_h != -1 && (max_h < 1 || max_h > 4096))) {
        zend_value_error("FastChart\\Chart::addIconAt() max width / height must be -1 or in [1, 4096]");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->n_icons >= FASTCHART_MAX_ICONS) {
        zend_value_error("FastChart\\Chart::addIconAt() supports at most %d icons",
                         FASTCHART_MAX_ICONS);
        RETURN_THROWS();
    }

    size_t new_bytes = (size_t)(self->n_icons + 1) * sizeof(fastchart_icon);
    self->icons = self->icons
        ? erealloc(self->icons, new_bytes)
        : emalloc(new_bytes);

    fastchart_icon *icon = &self->icons[self->n_icons];
    icon->x = x;
    icon->y = y;
    icon->max_w = (int)max_w;
    icon->max_h = (int)max_h;
    size_t plen = ZSTR_LEN(path);
    icon->path = emalloc(plen + 1);
    memcpy(icon->path, ZSTR_VAL(path), plen);
    icon->path[plen] = '\0';
    self->n_icons++;

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
    if (!color_is_null) {
        FASTCHART_VALIDATE_RGB(color, "FastChart\\Chart::addVerticalLine");
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
        FASTCHART_VALIDATE_RGB_OR_DEFAULT(rgb, "FastChart\\Chart::" #name_); \
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
            /* php_check_open_basedir emits E_WARNING but does not set \
             * EG(exception). Without an explicit throw, RETURN_THROWS \
             * asserts under debug builds. */ \
            if (!EG(exception)) { \
                zend_throw_error(NULL, \
                    "FastChart\\Chart::" #name_ "() open_basedir restriction " \
                    "prevents access to %s", ZSTR_VAL(path)); \
            } \
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
        /* Cap width and precision at three digits (max 999). libc's
         * printf(3) honors the width by padding the output buffer; a
         * format like "%500000000f" forces snprintf into a multi-second
         * loop that translates straight into request CPU when chart
         * labels render. Three digits is more than enough for any real
         * label (max chart canvas is ~16K px wide). */
        int wdigits = 0;
        while (i < len && p[i] >= '0' && p[i] <= '9') {
            if (++wdigits > 3) {
                zend_value_error("FastChart\\Chart::%s() format width is capped at three digits", where);
                return -1;
            }
            i++;
        }
        if (i < len && p[i] == '.') {
            i++;
            int pdigits = 0;
            while (i < len && p[i] >= '0' && p[i] <= '9') {
                if (++pdigits > 3) {
                    zend_value_error("FastChart\\Chart::%s() format precision is capped at three digits", where);
                    return -1;
                }
                i++;
            }
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
            /* php_check_open_basedir emits E_WARNING but does not set
             * EG(exception); throw explicitly so RETURN_THROWS does not
             * assert under debug builds. */
            if (!EG(exception)) {
                zend_throw_error(NULL,
                    "FastChart\\Chart::setBackgroundImage() open_basedir "
                    "restriction prevents access to %s", ZSTR_VAL(path));
            }
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
    if (mode != FASTCHART_INTERP_LINEAR && mode != FASTCHART_INTERP_SMOOTH &&
        mode != FASTCHART_INTERP_STEP_AFTER && mode != FASTCHART_INTERP_STEP_BEFORE) {
        zend_value_error("FastChart\\Chart::setLineInterpolation() expects INTERP_LINEAR, INTERP_SMOOTH, INTERP_STEP_AFTER or INTERP_STEP_BEFORE");
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
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(rgb, "FastChart\\Chart::setEdgeColor");
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
    FASTCHART_VALIDATE_RGB(lo, "FastChart\\Chart::setColorRamp");
    FASTCHART_VALIDATE_RGB(hi, "FastChart\\Chart::setColorRamp");
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

    if (!color_is_null) {
        FASTCHART_VALIDATE_RGB(color, "FastChart\\Chart::addTextAnnotation");
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
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(from, "FastChart\\Chart::setGradientFill");
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(to,   "FastChart\\Chart::setGradientFill");
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
    if (!color_is_null) {
        FASTCHART_VALIDATE_RGB(color, "FastChart\\Chart::setDropShadow");
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

    if (!color_is_null) {
        FASTCHART_VALIDATE_RGB(color, "FastChart\\ScatterChart::setTrendLine");
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

/* Parse a setErrorBars input array into two parallel double arrays
 * (lo / hi as positive magnitudes). Each entry is one of:
 *   - non-numeric / negative scalar -> NaN slot (no error bar)
 *   - non-negative scalar M         -> lo = hi = M  (symmetric)
 *   - [lo, hi] array                -> as-is, with negative values
 *                                      coerced to NaN (no error bar)
 * Returns 0 on success; never fails for shape — bad cells silently
 * become NaN slots. Caller frees out_lo / out_hi via efree(). */
static int fastchart_parse_error_bars(zval *errs, uint32_t cap,
                                      double **out_lo,
                                      double **out_hi, int *out_n)
{
    HashTable *ht = Z_ARRVAL_P(errs);
    uint32_t n = zend_hash_num_elements(ht);
    if (n == 0) {
        *out_lo = NULL;
        *out_hi = NULL;
        *out_n = 0;
        return 0;
    }
    /* Cap is per-chart-type: line series cap at FASTCHART_MAX_POINTS_PER_SERIES
     * (2048), scatter at FASTCHART_MAX_SCATTER_POINTS (4096). Extra entries
     * beyond the cap simply have no data point to attach to. */
    if (n > cap) n = cap;
    double *lo = emalloc((size_t)n * sizeof(double));
    double *hi = emalloc((size_t)n * sizeof(double));

    uint32_t idx = 0;
    zval *ev;
    ZEND_HASH_FOREACH_VAL(ht, ev) {
        if (idx >= n) break;
        double l = NAN, h = NAN;
        if (Z_TYPE_P(ev) == IS_ARRAY) {
            zval *zlo = zend_hash_index_find(Z_ARRVAL_P(ev), 0);
            zval *zhi = zend_hash_index_find(Z_ARRVAL_P(ev), 1);
            double dl = NAN, dh = NAN;
            if (zlo && fastchart_zval_to_double(zlo, &dl) == 0 && dl >= 0) l = dl;
            if (zhi && fastchart_zval_to_double(zhi, &dh) == 0 && dh >= 0) h = dh;
        } else {
            double m;
            if (fastchart_zval_to_double(ev, &m) == 0 && m >= 0) {
                l = m;
                h = m;
            }
        }
        lo[idx] = l;
        hi[idx] = h;
        idx++;
    } ZEND_HASH_FOREACH_END();

    *out_lo = lo;
    *out_hi = hi;
    *out_n = (int)n;
    return 0;
}

ZEND_METHOD(FastChart_LineChart, setErrorBars)
{
    zval *errs;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(errs)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_line_obj *self = Z_FASTCHART_LINE_OBJ_P(ZEND_THIS);
    if (self->err_lo) efree(self->err_lo);
    if (self->err_hi) efree(self->err_hi);
    self->err_lo = NULL;
    self->err_hi = NULL;
    self->err_n = 0;
    fastchart_parse_error_bars(errs, FASTCHART_MAX_POINTS_PER_SERIES,
                               &self->err_lo, &self->err_hi, &self->err_n);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_ScatterChart, setErrorBars)
{
    zval *errs;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(errs)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_scatter_obj *self = Z_FASTCHART_SCATTER_OBJ_P(ZEND_THIS);
    if (self->err_lo) efree(self->err_lo);
    if (self->err_hi) efree(self->err_hi);
    self->err_lo = NULL;
    self->err_hi = NULL;
    self->err_n = 0;
    fastchart_parse_error_bars(errs, FASTCHART_MAX_SCATTER_POINTS,
                               &self->err_lo, &self->err_hi, &self->err_n);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Parse one [x, y] (or longer) tuple plus optional 'href' / 'tooltip'
 * / 'color' assoc keys into a typed scatter point. Returns 0 on
 * success, -1 on shape error. */
static int fastchart_parse_scatter_point(zval *pair, fastchart_scatter_point *out,
                                         int series_idx)
{
    if (Z_TYPE_P(pair) != IS_ARRAY) return -1;
    HashTable *p = Z_ARRVAL_P(pair);
    zval *zx = zend_hash_index_find(p, 0);
    zval *zy = zend_hash_index_find(p, 1);
    if (!zx || !zy) return -1;
    double x, y;
    if (fastchart_zval_to_double(zx, &x) != 0) return -1;
    if (fastchart_zval_to_double(zy, &y) != 0) return -1;
    out->x = x;
    out->y = y;
    out->series_idx = series_idx;
    out->color_rgb = -1;
    out->href = NULL;
    out->tooltip = NULL;

    zval *zh = zend_hash_str_find(p, "href", sizeof("href") - 1);
    if (zh && Z_TYPE_P(zh) == IS_STRING) {
        if (memchr(Z_STRVAL_P(zh), 0, Z_STRLEN_P(zh)) == NULL) {
            size_t len = Z_STRLEN_P(zh);
            out->href = emalloc(len + 1);
            memcpy(out->href, Z_STRVAL_P(zh), len + 1);
        }
    }
    zval *zt = zend_hash_str_find(p, "tooltip", sizeof("tooltip") - 1);
    if (zt && Z_TYPE_P(zt) == IS_STRING) {
        if (memchr(Z_STRVAL_P(zt), 0, Z_STRLEN_P(zt)) == NULL) {
            size_t len = Z_STRLEN_P(zt);
            out->tooltip = emalloc(len + 1);
            memcpy(out->tooltip, Z_STRVAL_P(zt), len + 1);
        }
    }
    zval *zc = zend_hash_str_find(p, "color", sizeof("color") - 1);
    if (zc && Z_TYPE_P(zc) == IS_LONG) {
        zend_long c = Z_LVAL_P(zc);
        if (c >= 0 && c <= 0xFFFFFF) out->color_rgb = (int)c;
    }
    return 0;
}

ZEND_METHOD(FastChart_ScatterChart, setPoints)
{
    zval *data_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(data_zv)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_scatter_obj *self = Z_FASTCHART_SCATTER_OBJ_P(ZEND_THIS);
    /* Drop any existing parsed state. */
    if (self->points) {
        for (int i = 0; i < self->point_count; i++) {
            if (self->points[i].href) efree(self->points[i].href);
            if (self->points[i].tooltip) efree(self->points[i].tooltip);
        }
        efree(self->points);
        self->points = NULL;
    }
    self->point_count = 0;
    for (int i = 0; i < FASTCHART_MAX_SCATTER_SERIES; i++) {
        if (self->series_labels[i]) {
            efree(self->series_labels[i]);
            self->series_labels[i] = NULL;
        }
    }
    self->n_series = 0;

    HashTable *ht = Z_ARRVAL_P(data_zv);
    int n_input = (int)zend_hash_num_elements(ht);
    if (n_input == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    /* Detect multi-series: first element is dict with 'data' key. */
    zval *first = zend_hash_index_find(ht, 0);
    bool is_multi = false;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *dk = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (dk && Z_TYPE_P(dk) == IS_ARRAY) is_multi = true;
    }

    /* Two-pass: first count points so we can size the output once. */
    int total_pts = 0;
    if (is_multi) {
        zval *series_zv;
        int s = 0;
        ZEND_HASH_FOREACH_VAL(ht, series_zv) {
            if (s >= FASTCHART_MAX_SCATTER_SERIES) break;
            if (Z_TYPE_P(series_zv) != IS_ARRAY) continue;
            zval *dk = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                          "data", sizeof("data") - 1);
            if (!dk || Z_TYPE_P(dk) != IS_ARRAY) continue;
            total_pts += (int)zend_hash_num_elements(Z_ARRVAL_P(dk));
            s++;
        } ZEND_HASH_FOREACH_END();
    } else {
        total_pts = n_input;
    }
    if (total_pts > FASTCHART_MAX_SCATTER_POINTS) total_pts = FASTCHART_MAX_SCATTER_POINTS;
    if (total_pts == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    self->points = ecalloc((size_t)total_pts, sizeof(fastchart_scatter_point));
    int slot = 0;

    if (is_multi) {
        zval *series_zv;
        int s = 0;
        ZEND_HASH_FOREACH_VAL(ht, series_zv) {
            if (s >= FASTCHART_MAX_SCATTER_SERIES) break;
            if (Z_TYPE_P(series_zv) != IS_ARRAY) continue;
            zval *dk = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                          "data", sizeof("data") - 1);
            if (!dk || Z_TYPE_P(dk) != IS_ARRAY) continue;

            zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                "label", sizeof("label") - 1);
            const char *label = fastchart_label_or_null(label_zv);
            self->series_labels[s] = fc_strdup_opt(label);

            zval *pair;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(dk), pair) {
                if (slot >= total_pts) break;
                if (fastchart_parse_scatter_point(pair, &self->points[slot], s) == 0) slot++;
            } ZEND_HASH_FOREACH_END();
            s++;
        } ZEND_HASH_FOREACH_END();
        self->n_series = s;
    } else {
        zval *pair;
        ZEND_HASH_FOREACH_VAL(ht, pair) {
            if (slot >= total_pts) break;
            if (fastchart_parse_scatter_point(pair, &self->points[slot], 0) == 0) slot++;
        } ZEND_HASH_FOREACH_END();
        self->n_series = 1;
    }

    self->point_count = slot;
    if (slot == 0) {
        efree(self->points);
        self->points = NULL;
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

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

    fastchart_gauge_obj *self = Z_FASTCHART_GAUGE_OBJ_P(ZEND_THIS);

    /* Replace any existing zones. Each user-facing entry is an assoc
     * array { from: float, to: float, color?: int }. Bad-shape entries
     * are silently dropped (matches the prior config-zval behavior).
     * Up to 16 zones; further entries are ignored. */
    if (self->zones) efree(self->zones);
    self->zones = NULL;
    self->n_zones = 0;

    HashTable *ht = Z_ARRVAL_P(zones);
    uint32_t total = zend_hash_num_elements(ht);
    if (total == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    fastchart_gauge_zone *out = ecalloc(
        total > FASTCHART_MAX_GAUGE_ZONES ? FASTCHART_MAX_GAUGE_ZONES : total,
        sizeof(fastchart_gauge_zone));
    int n = 0;
    zval *z;
    ZEND_HASH_FOREACH_VAL(ht, z) {
        if (n >= FASTCHART_MAX_GAUGE_ZONES) break;
        if (Z_TYPE_P(z) != IS_ARRAY) continue;
        zval *zf = zend_hash_str_find(Z_ARRVAL_P(z), "from", sizeof("from") - 1);
        zval *zt = zend_hash_str_find(Z_ARRVAL_P(z), "to",   sizeof("to")   - 1);
        zval *zc = zend_hash_str_find(Z_ARRVAL_P(z), "color", sizeof("color") - 1);
        double f, t;
        if (!zf || !zt) continue;
        if (fastchart_zval_to_double(zf, &f) != 0) continue;
        if (fastchart_zval_to_double(zt, &t) != 0) continue;
        int color_rgb = -1;
        if (zc && Z_TYPE_P(zc) == IS_LONG &&
            Z_LVAL_P(zc) >= 0 && Z_LVAL_P(zc) <= 0xFFFFFF) {
            color_rgb = (int)Z_LVAL_P(zc);
        }
        out[n].from = f;
        out[n].to = t;
        out[n].color_rgb = color_rgb;
        n++;
    } ZEND_HASH_FOREACH_END();

    if (n == 0) {
        efree(out);
    } else {
        self->zones = out;
        self->n_zones = n;
    }
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

ZEND_METHOD(FastChart_Chart, setDpi)
{
    zend_long dpi;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(dpi)
    ZEND_PARSE_PARAMETERS_END();
    /* Cap range at sane bounds. libgd / FreeType happily accept
     * arbitrary values, but anything outside [24, 1200] is either a
     * bug or a typo (24 dpi = e-paper teletypes; 1200 dpi = pro
     * print). */
    if (dpi < 24 || dpi > 1200) {
        zend_value_error("FastChart\\Chart::setDpi() must be in [24, 1200]");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    self->dpi = dpi;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Emit a HTML <map> for the scatter chart's clickable points. Reads
 * the typed image_map_areas array populated by the renderer; chart
 * must have been draw()'d at least once for any output. */
ZEND_METHOD(FastChart_ScatterChart, getImageMap)
{
    zend_string *name;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_scatter_obj *self = Z_FASTCHART_SCATTER_OBJ_P(ZEND_THIS);
    if (!self->image_map_areas || self->n_image_map_areas == 0) {
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

    for (int idx = 0; idx < self->n_image_map_areas; idx++) {
        const fastchart_image_map_area *a = &self->image_map_areas[idx];
        const char *href_str = a->href;
        if (!href_str) continue;
        size_t href_len = strlen(href_str);

        /* Scheme allowlist: dangerous URL schemes (javascript:, data:,
         * vbscript:) are rejected. Relative paths, fragments, and
         * mailto: are allowed alongside http(s). Reject the whole
         * <area> entry on a bad scheme rather than emit a sanitized
         * one -- callers can audit their input. Embedded NUL was
         * already dropped by the setter; href_str is NUL-clean. */
        bool href_ok = false;
        if (href_len == 0) {
            href_ok = true;
        } else if (href_str[0] == '/' || href_str[0] == '#') {
            href_ok = true;
        } else if (zend_binary_strncasecmp(href_str, href_len, "http://",  7,  7) == 0 ||
                   zend_binary_strncasecmp(href_str, href_len, "https://", 8,  8) == 0 ||
                   zend_binary_strncasecmp(href_str, href_len, "mailto:",  7,  7) == 0) {
            href_ok = true;
        }
        if (!href_ok) continue;

        char buf[256];
        int n_chars = snprintf(buf, sizeof(buf),
            "<area shape=\"circle\" coords=\"%d,%d,%d\" href=\"",
            a->x, a->y, a->r);
        if (n_chars < 0) n_chars = 0;
        if ((size_t)n_chars > sizeof(buf)) n_chars = (int)sizeof(buf);
        smart_str_appendl(&out, buf, (size_t)n_chars);

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

        if (a->tooltip) {
            smart_str_appends(&out, " title=\"");
            const char *t_str = a->tooltip;
            size_t t_len = strlen(t_str);
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
    }

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

ZEND_METHOD(FastChart_PolarChart, setStyle)
{
    zend_long style;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(style)
    ZEND_PARSE_PARAMETERS_END();
    if (style != FASTCHART_POLAR_STYLE_LINE && style != FASTCHART_POLAR_STYLE_ROSE) {
        zend_value_error(
            "FastChart\\PolarChart::setStyle() expects STYLE_LINE or STYLE_ROSE");
        RETURN_THROWS();
    }
    fastchart_polar_obj *self = Z_FASTCHART_POLAR_OBJ_P(ZEND_THIS);
    self->polar_style = (int)style;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_ContourChart, setLevels)
{
    zval *levels;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(levels)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_contour_obj *self = Z_FASTCHART_CONTOUR_OBJ_P(ZEND_THIS);
    if (self->levels) { efree(self->levels); self->levels = NULL; }
    self->level_count = 0;
    HashTable *ht = Z_ARRVAL_P(levels);
    uint32_t un = zend_hash_num_elements(ht);
    if (un == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);
    if (un > FASTCHART_MAX_LEVELS) un = FASTCHART_MAX_LEVELS;
    int n = (int)un;
    self->levels = ecalloc((size_t)n, sizeof(double));
    int k = 0;
    zval *v;
    ZEND_HASH_FOREACH_VAL(ht, v) {
        if (k >= n) break;
        double d;
        if (fastchart_zval_to_double(v, &d) == 0 && isfinite(d)) {
            self->levels[k++] = d;
        }
    } ZEND_HASH_FOREACH_END();
    self->level_count = k;
    if (k == 0) {
        efree(self->levels);
        self->levels = NULL;
    }
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

/* Parse one OHLCV row into out. Required indices 0=ts,1=o,2=h,3=l,4=c
 * with optional 5=v. Returns 0 on success, -1 if the row is too short
 * or any required cell is non-numeric / non-finite. */
static int fastchart_parse_candle(zval *row, fastchart_candle *out)
{
    if (Z_TYPE_P(row) != IS_ARRAY) return -1;
    HashTable *r = Z_ARRVAL_P(row);
    if (zend_hash_num_elements(r) < 5) return -1;
    zval *zts = zend_hash_index_find(r, 0);
    zval *zo  = zend_hash_index_find(r, 1);
    zval *zh  = zend_hash_index_find(r, 2);
    zval *zl  = zend_hash_index_find(r, 3);
    zval *zc  = zend_hash_index_find(r, 4);
    if (!zts || !zo || !zh || !zl || !zc) return -1;
    zend_long ts;
    double o, h, l, c;
    if (fastchart_zval_to_long(zts, &ts) != 0) return -1;
    if (fastchart_zval_to_double(zo, &o) != 0) return -1;
    if (fastchart_zval_to_double(zh, &h) != 0) return -1;
    if (fastchart_zval_to_double(zl, &l) != 0) return -1;
    if (fastchart_zval_to_double(zc, &c) != 0) return -1;
    if (!(isfinite(o) && isfinite(h) && isfinite(l) && isfinite(c))) return -1;
    out->ts = ts; out->open = o; out->high = h; out->low = l; out->close = c;
    out->volume = 0; out->has_volume = 0;
    zval *zv = zend_hash_index_find(r, 5);
    if (zv) {
        double v;
        if (fastchart_zval_to_double(zv, &v) == 0 && isfinite(v) && v >= 0) {
            out->volume = v; out->has_volume = 1;
        }
    }
    return 0;
}

ZEND_METHOD(FastChart_StockChart, setOhlcv)
{
    zval *rows;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(rows)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(rows);
    int n_input = (int)zend_hash_num_elements(ht);
    if (n_input == 0) {
        zend_value_error("FastChart\\StockChart::setOhlcv() requires one or more OHLC rows");
        RETURN_THROWS();
    }
    if (n_input > FASTCHART_MAX_CANDLES) n_input = FASTCHART_MAX_CANDLES;

    fastchart_candle *parsed = emalloc((size_t)n_input * sizeof(fastchart_candle));
    int n = 0;
    bool any_volume = false;
    {
        zval *row;
        ZEND_HASH_FOREACH_VAL(ht, row) {
            if (n >= n_input) break;
            if (fastchart_parse_candle(row, &parsed[n]) != 0) continue;
            if (parsed[n].has_volume) any_volume = true;
            n++;
        } ZEND_HASH_FOREACH_END();
    }
    if (n == 0) {
        efree(parsed);
        zend_value_error("FastChart\\StockChart::setOhlcv() found no valid OHLC rows; expected [ts, o, h, l, c] or [ts, o, h, l, c, v]");
        RETURN_THROWS();
    }

    /* Insertion sort by timestamp. Caller is expected to feed already-
     * sorted data so the loop runs O(n) on the happy path. */
    for (int i = 1; i < n; i++) {
        fastchart_candle k = parsed[i];
        int j = i - 1;
        while (j >= 0 && parsed[j].ts > k.ts) { parsed[j+1] = parsed[j]; j--; }
        parsed[j+1] = k;
    }

    /* Right-size to n (avoids over-allocation when input had non-numeric
     * rows that got skipped). */
    if (n < n_input) {
        fastchart_candle *trimmed = emalloc((size_t)n * sizeof(fastchart_candle));
        memcpy(trimmed, parsed, (size_t)n * sizeof(fastchart_candle));
        efree(parsed);
        parsed = trimmed;
    }

    if (self->candles) efree(self->candles);
    self->candles = parsed;
    self->candle_count = n;
    self->any_volume = any_volume;

    /* Price overlays (Bollinger Bands, Parabolic SAR) carry a/b/c
     * arrays sized parallel to the candle array at the time of the
     * addBollingerBands() / addParabolicSAR() call. Replacing the
     * candle buffer here invalidates those arrays — `ov->n` still
     * reflects the OLD candle count, so the overlay render loop
     * (fastchart_stock.c, Bollinger and PSAR branches) would index
     * candles[i] for i up to the OLD count, reading past the end of
     * the new (possibly shorter) candle buffer. Mirrors the
     * destructor at fastchart_stock_release_extras: free the parallel
     * arrays, zero the per-overlay state, reset overlay_count. The
     * caller must re-call addBollingerBands / addParabolicSAR to
     * recompute against the new candles. Indicator panes (RSI, MACD,
     * etc.) take caller-supplied values, not candle-derived data, so
     * they are intentionally NOT cleared. */
    for (int k = 0; k < self->overlay_count; k++) {
        if (self->overlays[k].a) efree(self->overlays[k].a);
        if (self->overlays[k].b) efree(self->overlays[k].b);
        if (self->overlays[k].c) efree(self->overlays[k].c);
        self->overlays[k].a = NULL;
        self->overlays[k].b = NULL;
        self->overlays[k].c = NULL;
        self->overlays[k].n = 0;
    }
    self->overlay_count = 0;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, setVolumeColors)
{
    zval *colors;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(colors)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(colors);
    uint32_t un = zend_hash_num_elements(ht);
    if (un == 0) {
        if (self->volume_colors) efree(self->volume_colors);
        self->volume_colors = NULL;
        self->volume_colors_count = 0;
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    if (un > FASTCHART_MAX_VOLUME_COLORS) un = FASTCHART_MAX_VOLUME_COLORS;
    int n = (int)un;
    int *parsed = emalloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) {
        zval *cv = zend_hash_index_find(ht, i);
        zend_long c = -1;
        if (cv && Z_TYPE_P(cv) == IS_LONG) c = Z_LVAL_P(cv);
        parsed[i] = (c >= 0 && c <= 0xFFFFFF) ? (int)c : -1;
    }
    if (self->volume_colors) efree(self->volume_colors);
    self->volume_colors = parsed;
    self->volume_colors_count = n;
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

    fastchart_pie_obj *self = Z_FASTCHART_PIE_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(offsets);
    if (self->explode) { efree(self->explode); self->explode = NULL; }
    self->explode_count = 0;
    if (zend_hash_num_elements(ht) == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    /* The stub documents [2 => 12] sparse syntax: the user names a
     * slice index, the rest stay at 0. Walk by integer key so that
     * sparse maps work; allocate through max(key) + 1 capped at
     * FASTCHART_MAX_SLICES. The previous positional 0..n-1 walk
     * silently dropped any entry whose key was beyond the element
     * count (so [2 => 12] became n=1 and ignored slice 2). */
    zend_ulong max_key = 0;
    bool any_int = false;
    {
        zend_ulong k_idx;
        zend_string *k_str;
        zval *v;
        ZEND_HASH_FOREACH_KEY_VAL(ht, k_idx, k_str, v) {
            (void)v;
            if (k_str) continue;  /* string keys ignored */
            if (k_idx >= FASTCHART_MAX_SLICES) continue;
            if (!any_int || k_idx > max_key) max_key = k_idx;
            any_int = true;
        } ZEND_HASH_FOREACH_END();
    }
    if (!any_int) RETURN_ZVAL(ZEND_THIS, 1, 0);
    int n = (int)max_key + 1;
    self->explode = ecalloc((size_t)n, sizeof(zend_long));
    for (int i = 0; i < n; i++) {
        zval *off_zv = zend_hash_index_find(ht, i);
        zend_long off = 0;
        if (off_zv && fastchart_zval_to_long(off_zv, &off) == 0 && off > 0) {
            self->explode[i] = off;
        }
    }
    self->explode_count = n;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PieChart, setSlices)
{
    zval *data_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(data_zv)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_pie_obj *self = Z_FASTCHART_PIE_OBJ_P(ZEND_THIS);

    /* Drop any previously-parsed slices. */
    if (self->slices) {
        for (int i = 0; i < self->slice_count; i++) {
            if (self->slices[i].label) efree(self->slices[i].label);
        }
        efree(self->slices);
        self->slices = NULL;
    }
    self->slice_count = 0;
    self->total = 0.0;

    HashTable *ht = Z_ARRVAL_P(data_zv);
    int n = (int)zend_hash_num_elements(ht);
    if (n == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);
    if (n > FASTCHART_MAX_SLICES) n = FASTCHART_MAX_SLICES;

    self->slices = ecalloc((size_t)n, sizeof(fastchart_pie_slice));
    int slot = 0;

    /* Decide shape by sampling first element: associative map vs
     * list of dicts. The associative shape requires every key to be
     * a string and every value to be numeric; otherwise treat as a
     * list of dicts and skip non-conformant entries. */
    int shape_assoc = 1;
    {
        zend_string *k;
        zend_ulong h;
        zval *v;
        ZEND_HASH_FOREACH_KEY_VAL(ht, h, k, v) {
            (void)h;
            if (!k || (Z_TYPE_P(v) != IS_LONG && Z_TYPE_P(v) != IS_DOUBLE)) {
                shape_assoc = 0;
            }
            break;
        } ZEND_HASH_FOREACH_END();
    }

    if (shape_assoc) {
        zend_string *k;
        zend_ulong h;
        zval *v;
        ZEND_HASH_FOREACH_KEY_VAL(ht, h, k, v) {
            (void)h;
            if (slot >= n) break;
            double d;
            if (fastchart_zval_to_double(v, &d) != 0) continue;
            if (d <= 0.0 || !isfinite(d)) continue;
            if (k && memchr(ZSTR_VAL(k), 0, ZSTR_LEN(k)) != NULL) continue;
            self->slices[slot].label = k ? fc_strdup_opt(ZSTR_VAL(k)) : NULL;
            if (!k) {
                snprintf(self->slices[slot].idx_label,
                         sizeof(self->slices[slot].idx_label),
                         "%d", slot);
            } else {
                self->slices[slot].idx_label[0] = '\0';
            }
            self->slices[slot].value = d;
            self->slices[slot].color_rgb = -1;
            self->total += d;
            slot++;
        } ZEND_HASH_FOREACH_END();
    } else {
        zval *entry;
        ZEND_HASH_FOREACH_VAL(ht, entry) {
            if (slot >= n) break;
            if (Z_TYPE_P(entry) != IS_ARRAY) continue;
            zval *value_zv = zend_hash_str_find(Z_ARRVAL_P(entry),
                                                "value", sizeof("value") - 1);
            if (!value_zv) continue;
            double d;
            if (fastchart_zval_to_double(value_zv, &d) != 0) continue;
            if (d <= 0.0 || !isfinite(d)) continue;

            zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(entry),
                                                "label", sizeof("label") - 1);
            const char *label = fastchart_label_or_null(label_zv);
            self->slices[slot].label = fc_strdup_opt(label);
            if (!label) {
                snprintf(self->slices[slot].idx_label,
                         sizeof(self->slices[slot].idx_label),
                         "%d", slot);
            } else {
                self->slices[slot].idx_label[0] = '\0';
            }
            self->slices[slot].value = d;
            self->slices[slot].color_rgb = -1;
            zval *color_zv = zend_hash_str_find(Z_ARRVAL_P(entry),
                                                "color", sizeof("color") - 1);
            if (color_zv && Z_TYPE_P(color_zv) == IS_LONG) {
                zend_long c = Z_LVAL_P(color_zv);
                if (c >= 0 && c <= 0xFFFFFF) {
                    self->slices[slot].color_rgb = (int)c;
                }
            }
            self->total += d;
            slot++;
        } ZEND_HASH_FOREACH_END();
    }

    if (slot < n) {
        /* Trim down. */
        if (slot == 0) {
            efree(self->slices);
            self->slices = NULL;
        } else {
            fastchart_pie_slice *trimmed = ecalloc((size_t)slot, sizeof(*trimmed));
            memcpy(trimmed, self->slices, (size_t)slot * sizeof(*trimmed));
            efree(self->slices);
            self->slices = trimmed;
        }
    }
    self->slice_count = slot;

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

ZEND_METHOD(FastChart_LineChart, setSeries)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_line_obj *self = Z_FASTCHART_LINE_OBJ_P(ZEND_THIS);
    fastchart_series_array_release(self->series, self->n_series);
    self->n_series = 0;
    self->max_len = 0;
    int flags = FC_SERIES_F_COLORS | FC_SERIES_F_RIGHTAXIS;
    if (self->strict) flags |= FC_SERIES_F_STRICT;
    if (fastchart_collect_series_into(arr, self->series, &self->n_series,
                                      &self->max_len, flags) != 0) {
        if (!EG(exception)) {
            zend_value_error("FastChart\\LineChart::setSeries() expects a numeric list or list of {data: [...], label?, colors?, axis?}");
        }
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_AreaChart, setSeries)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_area_obj *self = Z_FASTCHART_AREA_OBJ_P(ZEND_THIS);
    fastchart_series_array_release(self->series, self->n_series);
    self->n_series = 0;
    self->max_len = 0;
    int flags = FC_SERIES_F_RIGHTAXIS;
    if (self->strict) flags |= FC_SERIES_F_STRICT;
    if (fastchart_collect_series_into(arr, self->series, &self->n_series,
                                      &self->max_len, flags) != 0) {
        if (!EG(exception)) {
            zend_value_error("FastChart\\AreaChart::setSeries() expects a numeric list or list of {data: [...], label?, axis?}");
        }
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BarChart, setSeries)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_bar_obj *self = Z_FASTCHART_BAR_OBJ_P(ZEND_THIS);
    fastchart_series_array_release(self->series, self->n_series);
    self->n_series = 0;
    self->max_len = 0;
    int flags = FC_SERIES_F_COLORS;
    if (self->bar_floating) flags |= FC_SERIES_F_FLOATING;
    if (self->strict) flags |= FC_SERIES_F_STRICT;
    if (fastchart_collect_series_into(arr, self->series, &self->n_series,
                                      &self->max_len, flags) != 0) {
        if (!EG(exception)) {
            zend_value_error("FastChart\\BarChart::setSeries() expects a numeric list or list of {data: [...], label?, colors?}");
        }
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_AreaChart, setStacked)
{
    bool stacked;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(stacked)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_area_obj *self = Z_FASTCHART_AREA_OBJ_P(ZEND_THIS);
    self->stacked = stacked;
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
/* SVG-side dispatch. The pilot rotation covers Line/Bar/Pie/Stock;
 * other chart families throw a clear runtime error pointing callers
 * at renderPng/renderJpeg/etc. SVG support for the remaining 15
 * Cartesian families and the Symbol family fans out in follow-up
 * PRs. The non-pilot families keep working through the gd path
 * unchanged. */
static int dispatch_svg_render(fastchart_obj *self, zend_class_entry *ce, fastchart_target_t *t)
{
    if (ce == fastchart_line_chart_ce)
        return fastchart_line_render_to_target((fastchart_line_obj *)self, t);
    if (ce == fastchart_bar_chart_ce)
        return fastchart_bar_render_to_target((fastchart_bar_obj *)self, t);
    if (ce == fastchart_pie_chart_ce)
        return fastchart_pie_render_to_target((fastchart_pie_obj *)self, t);
    if (ce == fastchart_stock_chart_ce)
        return fastchart_stock_render_to_target((fastchart_stock_obj *)self, t);
    zend_throw_error(NULL,
        "FastChart: SVG output is not yet supported for this chart family. "
        "Pilot families (LineChart, BarChart, PieChart, StockChart) "
        "support renderSvg(); other families "
        "must use renderPng() / renderJpeg() / renderWebp() / renderGif() / "
        "renderAvif() / renderToFile() with a raster extension.");
    return -1;
}

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
    if (ce == fastchart_treemap_ce)       return fastchart_treemap_render_to_image((fastchart_treemap_obj *)self, im);
    if (ce == fastchart_funnel_ce)        return fastchart_funnel_render_to_image((fastchart_funnel_obj *)self, im);
    if (ce == fastchart_waterfall_ce)     return fastchart_waterfall_render_to_image((fastchart_waterfall_obj *)self, im);
    if (ce == fastchart_heatmap_ce)       return fastchart_heatmap_render_to_image((fastchart_heatmap_obj *)self, im);
    if (ce == fastchart_linear_meter_ce)  return fastchart_linear_meter_render_to_image((fastchart_linear_meter_obj *)self, im);
    zend_throw_error(NULL, "FastChart: render dispatch found unknown class entry");
    return -1;
}

/* Encode a rendered gdImagePtr in the requested format. Returns
 * malloc'd bytes via *out_bytes / *out_sz; caller gdFree's. NULL
 * out_bytes on failure (caller throws). `format`: 0 PNG, 1 JPEG,
 * 2 WebP, 3 GIF, 4 AVIF.
 *
 * Non-static so fastchart_symbol.c's render shortcuts share the
 * exact same encoder dispatch (single source of truth for AVIF
 * fallback + format range). Declared in fastchart_render_helpers.h. */
int fastchart_encode_image(gdImagePtr im, int format, int quality,
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

/* HiDPI canvas scale derived from setDpi(). 96 DPI = 1.0×; 200 DPI =
 * 200/96 ≈ 2.08×. The logical width/height is the user-supplied size;
 * the physical canvas allocated for the render*() helpers grows with
 * DPI so the chart keeps its apparent layout while every glyph and
 * shape gains pixel density. The PNG metadata then reports the same
 * DPI so retina viewers / print pipelines display the image at its
 * intended physical size. Static — only resolve_canvas_dims uses it. */
static double fastchart_dpi_scale_for(zend_long dpi)
{
    if (dpi <= 0 || dpi == 96) return 1.0;
    return (double)dpi / 96.0;
}

/* Resolve the physical (allocated) canvas dimensions from the logical
 * setSize() values and the chart's DPI scale, with a hard cap. setSize
 * accepts width/height up to 65535 ("fits in 16 bits"); setDpi(1200)
 * on those would otherwise allocate ~819188x819188 pixels — capping
 * here keeps gdImageCreateTrueColor inside libgd's safe range. Also
 * clamps below: setDpi(24) on a 1x1 canvas would otherwise round to
 * 0x0 and crash libgd. Returns 0 on success, -1 with a thrown
 * ValueError otherwise.
 *
 * Takes scalars instead of a base-struct pointer so the Chart and
 * Symbol families (with separate base layouts) share one
 * implementation. Non-static; declared in fastchart_render_helpers.h. */
int fastchart_resolve_canvas_dims(zend_long width, zend_long height,
                                  zend_long dpi,
                                  int *out_w, int *out_h)
{
    double scale = fastchart_dpi_scale_for(dpi);
    /* Round to int first so the cap comparison is on the actual
     * dimension we'll allocate (16384 exact must pass). */
    int pw = (int)((double)width  * scale + 0.5);
    int ph = (int)((double)height * scale + 0.5);
    /* Clamp below to avoid 0x0 allocations on tiny logical sizes
     * combined with sub-96 DPI (e.g. 1x1 at 24 DPI rounds to 0). */
    if (pw < 1) pw = 1;
    if (ph < 1) ph = 1;
    /* 16384 is libgd's de-facto safe upper bound on a single
     * dimension. The product cap is the load-bearing one: libgd
     * truecolor stores 4 bytes per pixel, so 16384 * 16384 alone is
     * 1 GiB before encoder buffers, and any caller with both axes
     * unconstrained can drive a single render-helper call into a
     * native allocation that pins the worker. */
    const int MAX_PHYS_DIM = 16384;
    if (pw > MAX_PHYS_DIM || ph > MAX_PHYS_DIM) {
        zend_value_error(
            "FastChart: physical canvas dimensions exceed the 16384px cap "
            "(setSize=%lldx%lld, setDpi=%ld -> %dx%d). "
            "Reduce setSize() or setDpi().",
            (long long)width, (long long)height,
            (long)dpi, pw, ph);
        return -1;
    }
    /* Product cap: 64M pixels = 256 MiB at 4 bytes/pixel for the
     * truecolor canvas, plus comparable encoder buffers. Square
     * worst case (16384 * 16384 = 268M) is rejected here even
     * though both dims pass MAX_PHYS_DIM. Multiply as int64 so the
     * arithmetic itself can't overflow before the comparison. */
    const long long MAX_PHYS_PIXELS = 64LL * 1024LL * 1024LL;
    long long pixels = (long long)pw * (long long)ph;
    if (pixels > MAX_PHYS_PIXELS) {
        zend_value_error(
            "FastChart: physical canvas pixel count exceeds the %lld "
            "budget (setSize=%lldx%lld, setDpi=%ld -> %dx%d = %lld pixels). "
            "Reduce setSize() or setDpi().",
            MAX_PHYS_PIXELS,
            (long long)width, (long long)height,
            (long)dpi, pw, ph, pixels);
        return -1;
    }
    *out_w = pw;
    *out_h = ph;
    return 0;
}

static void fastchart_render_to_string(INTERNAL_FUNCTION_PARAMETERS, int format, zend_long quality)
{
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);

    if (self->width <= 0 || self->height <= 0) {
        zend_throw_error(NULL, "FastChart: invalid canvas size; setSize() first");
        RETURN_THROWS();
    }

    int alloc_w, alloc_h;
    if (fastchart_resolve_canvas_dims(self->width, self->height, self->dpi, &alloc_w, &alloc_h) != 0) {
        RETURN_THROWS();
    }
    gdImagePtr im = gdImageCreateTrueColor(alloc_w, alloc_h);
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
    if (fastchart_encode_image(im, format, (int)quality, &bytes, &sz) != 0) {
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
    fastchart_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, 0);
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
    fastchart_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1, quality);
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
    fastchart_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2, quality);
}

ZEND_METHOD(FastChart_Chart, renderGif)
{
    ZEND_PARSE_PARAMETERS_NONE();
    fastchart_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 3, 0);
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
    fastchart_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 4, quality);
}

/* SVG render shared between Chart::renderSvg (fragment_only=0, emits a
 * full document) and Chart::drawSvgFragment (fragment_only=1, emits
 * just a <g> group for callers stitching multiple charts into one
 * outer <svg>). Output dimensions are the LOGICAL setSize() values —
 * SVG is vector-scalable, so the DPI knob doesn't multiply the
 * viewport. DPI still flows into layout (margins / label padding) and
 * into FreeType measurement so an SVG and PNG of the same chart pick
 * the same label widths. */
static void fastchart_render_to_svg(INTERNAL_FUNCTION_PARAMETERS, int fragment_only)
{
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->width <= 0 || self->height <= 0) {
        zend_throw_error(NULL, "FastChart: invalid canvas size; setSize() first");
        RETURN_THROWS();
    }

    smart_str buf = {0};
    if (!fragment_only) {
        fc_svg_emit_doc_open(&buf, (int)self->width, (int)self->height);
    }
    fc_svg_emit_g_open(&buf, "fastchart");

    fastchart_target_t t;
    fastchart_target_from_svg(&t, &buf,
                               (int)self->width, (int)self->height,
                               (int)self->dpi);

    if (dispatch_svg_render(self, Z_OBJCE_P(ZEND_THIS), &t) != 0 || EG(exception)) {
        smart_str_free(&buf);
        RETURN_THROWS();
    }

    fc_svg_emit_g_close(&buf);
    if (!fragment_only) {
        fc_svg_emit_doc_close(&buf);
    }
    smart_str_0(&buf);

    if (!buf.s) {
        zend_throw_error(NULL, "FastChart: SVG renderer produced no output");
        RETURN_THROWS();
    }
    /* Hand the smart_str's underlying zend_string to the return slot
     * directly — smart_str_0 has already NUL-terminated and finalised
     * the buffer. Transfers refcount=1 ownership. */
    RETURN_STR(buf.s);
}

ZEND_METHOD(FastChart_Chart, renderSvg)
{
    ZEND_PARSE_PARAMETERS_NONE();
    fastchart_render_to_svg(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

ZEND_METHOD(FastChart_Chart, drawSvgFragment)
{
    ZEND_PARSE_PARAMETERS_NONE();
    fastchart_render_to_svg(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

/* --------------------- renderToFile -------------------------------
 *
 * Path extension picks the format. Honors open_basedir. Writes via
 * the Zend stream layer so wrapper paths (file://, sftp://) work
 * within open_basedir constraints. */
/* Non-static so Symbol::renderToFile reuses the same extension table.
 * Declared in fastchart_render_helpers.h. */
int fastchart_format_from_path(const char *path, size_t len)
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

/* Case-insensitive ASCII check for a `.svg` tail. Same parsing
 * shape as fastchart_format_from_path so a path like
 * "weird.SVG/here.png" reports .png and "report.SVG" reports .svg. */
static int fc_path_ends_with_svg(const char *path, size_t len)
{
    if (len == 0 || len > 4096) return 0;
    const char *dot = NULL;
    for (size_t i = len; i > 0; i--) {
        if (path[i - 1] == '.') { dot = &path[i - 1]; break; }
        if (path[i - 1] == '/' || path[i - 1] == '\\') break;
    }
    if (!dot) return 0;
    const char *ext = dot + 1;
    size_t ext_len = strlen(ext);
    return zend_binary_strcasecmp(ext, ext_len, "svg", 3) == 0;
}

/* SVG file-write branch invoked by renderToFile when the path
 * extension is .svg. Bypasses fastchart_encode_image (libgd's raster
 * encoder dispatch); SVG is a text format produced directly via
 * smart_str. Honors open_basedir same as the raster path. */
static void fastchart_render_to_svg_file(INTERNAL_FUNCTION_PARAMETERS, zend_string *path)
{
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->width <= 0 || self->height <= 0) {
        zend_throw_error(NULL, "FastChart: invalid canvas size; setSize() first");
        RETURN_THROWS();
    }

    smart_str buf = {0};
    fc_svg_emit_doc_open(&buf, (int)self->width, (int)self->height);
    fc_svg_emit_g_open(&buf, "fastchart");

    fastchart_target_t t;
    fastchart_target_from_svg(&t, &buf,
                               (int)self->width, (int)self->height,
                               (int)self->dpi);

    if (dispatch_svg_render(self, Z_OBJCE_P(ZEND_THIS), &t) != 0 || EG(exception)) {
        smart_str_free(&buf);
        RETURN_THROWS();
    }
    fc_svg_emit_g_close(&buf);
    fc_svg_emit_doc_close(&buf);
    smart_str_0(&buf);

    if (!buf.s) {
        zend_throw_error(NULL, "FastChart: SVG renderer produced no output");
        RETURN_THROWS();
    }

    php_stream *stream = php_stream_open_wrapper(ZSTR_VAL(path), "wb",
        REPORT_ERRORS, NULL);
    if (!stream) {
        smart_str_free(&buf);
        if (!EG(exception)) {
            zend_throw_error(NULL,
                "FastChart\\Chart::renderToFile() could not open %s for writing",
                ZSTR_VAL(path));
        }
        RETURN_THROWS();
    }

    size_t sz = ZSTR_LEN(buf.s);
    ssize_t written = php_stream_write(stream, ZSTR_VAL(buf.s), sz);
    php_stream_close(stream);
    smart_str_free(&buf);

    if (written < 0) {
        zend_throw_error(NULL, "FastChart: write to %s failed", ZSTR_VAL(path));
        RETURN_THROWS();
    }
    if ((size_t)written != sz) {
        zend_throw_error(NULL,
            "FastChart: short write to %s (%zd of %zu bytes)",
            ZSTR_VAL(path), written, sz);
        RETURN_THROWS();
    }
    RETURN_LONG((zend_long)written);
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

    /* Vector branch. .svg ignores $quality (no lossy encoder) and
     * goes through a separate write path that emits text bytes. */
    if (fc_path_ends_with_svg(ZSTR_VAL(path), ZSTR_LEN(path))) {
        (void)quality;
        if (php_check_open_basedir(ZSTR_VAL(path))) {
            if (!EG(exception)) {
                zend_throw_error(NULL,
                    "FastChart\\Chart::renderToFile() open_basedir restriction "
                    "prevents access to %s", ZSTR_VAL(path));
            }
            RETURN_THROWS();
        }
        fastchart_render_to_svg_file(INTERNAL_FUNCTION_PARAM_PASSTHRU, path);
        return;
    }

    int format = fastchart_format_from_path(ZSTR_VAL(path), ZSTR_LEN(path));
    if (format < 0) {
        zend_value_error("FastChart\\Chart::renderToFile() could not infer format from extension; expected .png/.jpg/.jpeg/.webp/.gif/.avif/.svg");
        RETURN_THROWS();
    }
    /* Format-conditional quality validation. JPEG quality 0 is
     * degenerate (max compression, near-unrecognisable output) and
     * matches what renderJpeg() rejects, so reject it here too.
     * WebP and AVIF treat 0 as a valid "smallest size, lowest
     * fidelity" setting and renderWebp / renderAvif accept it.
     * PNG and GIF ignore the argument entirely (libgd doesn't expose
     * a per-call zlib level for PngPtr / GifPtr) but we still
     * sanity-check the range. */
    if (format == 1) {  /* JPEG */
        if (quality < 1 || quality > 100) {
            zend_value_error("FastChart\\Chart::renderToFile() JPEG quality must be in [1, 100]");
            RETURN_THROWS();
        }
    } else {  /* PNG / WebP / GIF / AVIF */
        if (quality < 0 || quality > 100) {
            zend_value_error("FastChart\\Chart::renderToFile() quality must be in [0, 100]");
            RETURN_THROWS();
        }
    }
    if (php_check_open_basedir(ZSTR_VAL(path))) {
        /* php_check_open_basedir emits E_WARNING but does not set
         * EG(exception); throw explicitly so RETURN_THROWS does not
         * assert under debug builds. */
        if (!EG(exception)) {
            zend_throw_error(NULL,
                "FastChart\\Chart::renderToFile() open_basedir restriction "
                "prevents access to %s", ZSTR_VAL(path));
        }
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (self->width <= 0 || self->height <= 0) {
        zend_throw_error(NULL, "FastChart: invalid canvas size; setSize() first");
        RETURN_THROWS();
    }

    int alloc_w, alloc_h;
    if (fastchart_resolve_canvas_dims(self->width, self->height, self->dpi, &alloc_w, &alloc_h) != 0) {
        RETURN_THROWS();
    }
    gdImagePtr im = gdImageCreateTrueColor(alloc_w, alloc_h);
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
    if (fastchart_encode_image(im, format, (int)quality, &bytes, &sz) != 0) {
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
        /* REPORT_ERRORS emits E_WARNING but does not set EG(exception).
         * Throw explicitly so RETURN_THROWS does not assert under
         * debug builds. */
        if (!EG(exception)) {
            zend_throw_error(NULL,
                "FastChart\\Chart::renderToFile() could not open %s for writing",
                ZSTR_VAL(path));
        }
        RETURN_THROWS();
    }

    ssize_t written = php_stream_write(stream, (const char *)bytes, (size_t)sz);
    php_stream_close(stream);
    gdFree(bytes);

    if (written < 0) {
        zend_throw_error(NULL, "FastChart: write to %s failed", ZSTR_VAL(path));
        RETURN_THROWS();
    }
    /* Short writes leave a truncated file behind. Treat as failure
     * rather than returning the partial byte count, which a caller
     * could mistake for success. */
    if ((size_t)written != (size_t)sz) {
        zend_throw_error(NULL,
            "FastChart: short write to %s (%zd of %d bytes)",
            ZSTR_VAL(path), written, sz);
        RETURN_THROWS();
    }
    RETURN_LONG((zend_long)written);
}

/* Parse a 2D PHP array into a typed fastchart_grid (row-major, NaN
 * for missing/non-finite cells). Out is overwritten; caller frees
 * grid->cells before calling. Returns 0 on success, -1 if the input
 * is empty or its dimensions overflow size_t. */
static int fastchart_parse_grid(zval *arr, fastchart_grid *out, const char *where)
{
    HashTable *ht = Z_ARRVAL_P(arr);
    int rows = (int)zend_hash_num_elements(ht);
    if (rows == 0) {
        out->cells = NULL; out->rows = 0; out->cols = 0;
        return 0;
    }
    int cols = 0;
    zval *row;
    ZEND_HASH_FOREACH_VAL(ht, row) {
        if (Z_TYPE_P(row) != IS_ARRAY) continue;
        int rlen = (int)zend_hash_num_elements(Z_ARRVAL_P(row));
        if (rlen > cols) cols = rlen;
    } ZEND_HASH_FOREACH_END();
    if (cols == 0) {
        out->cells = NULL; out->rows = 0; out->cols = 0;
        return 0;
    }
    if ((size_t)cols > SIZE_MAX / sizeof(double) ||
        (size_t)rows > (SIZE_MAX / sizeof(double)) / (size_t)cols) {
        zend_value_error("%s grid dimensions overflow allocation", where);
        return -1;
    }
    /* Per-request cell-count cap. Surface / contour grids are
     * inherently bounded by canvas pixel resolution; nothing realistic
     * needs more than ~10K cells. Without this cap, setGrid([10000][
     * 10000]) allocates 800MB of doubles per call. */
    if ((size_t)rows * (size_t)cols > FASTCHART_MAX_GRID_CELLS) {
        zend_value_error("%s grid exceeds %d cells (got rows=%d cols=%d)",
                         where, FASTCHART_MAX_GRID_CELLS, rows, cols);
        return -1;
    }
    out->cells = emalloc((size_t)rows * (size_t)cols * sizeof(double));
    out->rows = rows;
    out->cols = cols;
    int ri = 0;
    ZEND_HASH_FOREACH_VAL(ht, row) {
        if (Z_TYPE_P(row) != IS_ARRAY) {
            for (int j = 0; j < cols; j++) out->cells[ri * cols + j] = NAN;
            ri++;
            continue;
        }
        int j = 0;
        zval *cell;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(row), cell) {
            if (j >= cols) break;
            double d;
            if (fastchart_zval_to_double(cell, &d) == 0 && isfinite(d)) {
                out->cells[ri * cols + j] = d;
            } else {
                out->cells[ri * cols + j] = NAN;
            }
            j++;
        } ZEND_HASH_FOREACH_END();
        for (; j < cols; j++) out->cells[ri * cols + j] = NAN;
        ri++;
    } ZEND_HASH_FOREACH_END();
    return 0;
}

ZEND_METHOD(FastChart_SurfaceChart, setGrid)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_surface_obj *self = Z_FASTCHART_SURFACE_OBJ_P(ZEND_THIS);
    if (self->grid.cells) { efree(self->grid.cells); self->grid.cells = NULL; }
    self->grid.rows = 0;
    self->grid.cols = 0;
    if (fastchart_parse_grid(arr, &self->grid, "FastChart\\SurfaceChart::setGrid()") != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_ContourChart, setGrid)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_contour_obj *self = Z_FASTCHART_CONTOUR_OBJ_P(ZEND_THIS);
    if (self->grid.cells) { efree(self->grid.cells); self->grid.cells = NULL; }
    self->grid.rows = 0;
    self->grid.cols = 0;
    if (fastchart_parse_grid(arr, &self->grid, "FastChart\\ContourChart::setGrid()") != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_GanttChart, setTasks)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_gantt_obj *self = Z_FASTCHART_GANTT_OBJ_P(ZEND_THIS);
    if (self->tasks) {
        for (int i = 0; i < self->task_count; i++) {
            fc_efree_opt(self->tasks[i].name);
            fc_efree_opt(self->tasks[i].deps);
        }
        efree(self->tasks);
        self->tasks = NULL;
    }
    self->task_count = 0;

    HashTable *ht = Z_ARRVAL_P(arr);
    int n = (int)zend_hash_num_elements(ht);
    if (n == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);
    if (n > FASTCHART_MAX_GANTT_TASKS) n = FASTCHART_MAX_GANTT_TASKS;
    self->tasks = ecalloc((size_t)n, sizeof(fastchart_gantt_task));
    int slot = 0;

    zval *t;
    ZEND_HASH_FOREACH_VAL(ht, t) {
        if (slot >= n) break;
        if (Z_TYPE_P(t) != IS_ARRAY) continue;
        HashTable *th = Z_ARRVAL_P(t);
        zval *zs = zend_hash_str_find(th, "start", 5);
        zval *ze = zend_hash_str_find(th, "end",   3);
        if (!zs || !ze) continue;
        zend_long s, e;
        if (fastchart_zval_to_long(zs, &s) != 0) continue;
        if (fastchart_zval_to_long(ze, &e) != 0) continue;
        if (e < s) { zend_long tmp = s; s = e; e = tmp; }

        fastchart_gantt_task *out = &self->tasks[slot];
        out->start = s;
        out->end = e;
        out->name = NULL;
        out->color_rgb = -1;
        out->is_milestone = false;
        out->deps = NULL;
        out->n_deps = 0;

        zval *zn = zend_hash_str_find(th, "name", 4);
        out->name = fc_strdup_opt(fastchart_label_or_null(zn));

        zval *zc = zend_hash_str_find(th, "color", 5);
        if (zc && Z_TYPE_P(zc) == IS_LONG) {
            zend_long c = Z_LVAL_P(zc);
            if (c >= 0 && c <= 0xFFFFFF) out->color_rgb = (int)c;
        }
        zval *zm = zend_hash_str_find(th, "milestone", 9);
        out->is_milestone =
            (zm && (Z_TYPE_P(zm) == IS_TRUE ||
                    (Z_TYPE_P(zm) == IS_LONG && Z_LVAL_P(zm) != 0)));
        zval *zd = zend_hash_str_find(th, "depends", 7);
        if (zd && Z_TYPE_P(zd) == IS_ARRAY) {
            uint32_t udn = zend_hash_num_elements(Z_ARRVAL_P(zd));
            if (udn > FASTCHART_MAX_GANTT_DEPS) udn = FASTCHART_MAX_GANTT_DEPS;
            int dn = (int)udn;
            if (dn > 0) {
                out->deps = ecalloc((size_t)dn, sizeof(int));
                int k = 0;
                zval *dv;
                ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zd), dv) {
                    if (k >= dn) break;
                    if (Z_TYPE_P(dv) == IS_LONG) {
                        out->deps[k++] = (int)Z_LVAL_P(dv);
                    }
                } ZEND_HASH_FOREACH_END();
                out->n_deps = k;
                if (k == 0) { efree(out->deps); out->deps = NULL; }
            }
        }
        slot++;
    } ZEND_HASH_FOREACH_END();
    self->task_count = slot;
    if (slot == 0) {
        efree(self->tasks);
        self->tasks = NULL;
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_RadarChart, setSeries)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_radar_obj *self = Z_FASTCHART_RADAR_OBJ_P(ZEND_THIS);
    for (int i = 0; i < self->n_series; i++) {
        fc_efree_opt(self->series[i].values);
        fc_efree_opt(self->series[i].label);
        self->series[i].values = NULL;
        self->series[i].label = NULL;
        self->series[i].len = 0;
    }
    self->n_series = 0;

    HashTable *ht = Z_ARRVAL_P(arr);
    if (zend_hash_num_elements(ht) == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    zval *first = zend_hash_index_find(ht, 0);
    bool is_multi = false;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *d = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (d && Z_TYPE_P(d) == IS_ARRAY) is_multi = true;
    }

#define RADAR_PARSE_VALUES(slot_, ht_) do {                                  \
        HashTable *_dh = (ht_);                                              \
        uint32_t _unp = zend_hash_num_elements(_dh);                         \
        if (_unp > FASTCHART_MAX_RADAR_VALUES) _unp = FASTCHART_MAX_RADAR_VALUES; \
        int _np = (int)_unp;                                                 \
        if (_np > 0) {                                                       \
            (slot_)->values = emalloc((size_t)_np * sizeof(double));         \
            int _k = 0;                                                      \
            zval *_v;                                                        \
            ZEND_HASH_FOREACH_VAL(_dh, _v) {                                 \
                if (_k >= _np) break;                                        \
                double _d;                                                   \
                if (fastchart_zval_to_double(_v, &_d) == 0) {                \
                    if (_d < 0) _d = 0;                                      \
                    (slot_)->values[_k++] = _d;                              \
                } else {                                                     \
                    (slot_)->values[_k++] = 0;                               \
                }                                                            \
            } ZEND_HASH_FOREACH_END();                                       \
            (slot_)->len = _k;                                               \
        }                                                                    \
    } while (0)

    if (is_multi) {
        zval *s_zv;
        ZEND_HASH_FOREACH_VAL(ht, s_zv) {
            if (self->n_series >= FASTCHART_MAX_RADAR_SERIES) break;
            if (Z_TYPE_P(s_zv) != IS_ARRAY) continue;
            zval *d = zend_hash_str_find(Z_ARRVAL_P(s_zv), "data", sizeof("data") - 1);
            if (!d || Z_TYPE_P(d) != IS_ARRAY) continue;
            fastchart_radar_series *slot = &self->series[self->n_series];
            RADAR_PARSE_VALUES(slot, Z_ARRVAL_P(d));
            zval *l = zend_hash_str_find(Z_ARRVAL_P(s_zv), "label", sizeof("label") - 1);
            slot->label = fc_strdup_opt(fastchart_label_or_null(l));
            zval *c = zend_hash_str_find(Z_ARRVAL_P(s_zv), "color", sizeof("color") - 1);
            slot->color_rgb = -1;
            if (c && Z_TYPE_P(c) == IS_LONG) {
                zend_long cc = Z_LVAL_P(c);
                if (cc >= 0 && cc <= 0xFFFFFF) slot->color_rgb = (int)cc;
            }
            self->n_series++;
        } ZEND_HASH_FOREACH_END();
    } else {
        fastchart_radar_series *slot = &self->series[0];
        RADAR_PARSE_VALUES(slot, ht);
        slot->label = NULL;
        slot->color_rgb = -1;
        self->n_series = 1;
    }
#undef RADAR_PARSE_VALUES
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_PolarChart, setSeries)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_polar_obj *self = Z_FASTCHART_POLAR_OBJ_P(ZEND_THIS);
    /* Drop existing series. */
    for (int i = 0; i < self->n_series; i++) {
        fc_efree_opt(self->series[i].angles);
        fc_efree_opt(self->series[i].radii);
        fc_efree_opt(self->series[i].label);
        self->series[i].angles = NULL;
        self->series[i].radii = NULL;
        self->series[i].label = NULL;
        self->series[i].len = 0;
    }
    self->n_series = 0;

    HashTable *ht = Z_ARRVAL_P(arr);
    if (zend_hash_num_elements(ht) == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    zval *first = zend_hash_index_find(ht, 0);
    bool is_multi = false;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *d = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (d && Z_TYPE_P(d) == IS_ARRAY) is_multi = true;
    }

    /* Helper: parse a [angle, radius] list into the slot. */
#define POLAR_PARSE_DATA(slot_, ht_) do {                                \
        HashTable *_dh = (ht_);                                          \
        uint32_t _unp = zend_hash_num_elements(_dh);                     \
        if (_unp > FASTCHART_MAX_POLAR_POINTS) _unp = FASTCHART_MAX_POLAR_POINTS; \
        int _np = (int)_unp;                                             \
        if (_np > 0) {                                                   \
            (slot_)->angles = emalloc((size_t)_np * sizeof(double));     \
            (slot_)->radii  = emalloc((size_t)_np * sizeof(double));     \
            int _k = 0;                                                  \
            zval *_p;                                                    \
            ZEND_HASH_FOREACH_VAL(_dh, _p) {                             \
                if (Z_TYPE_P(_p) != IS_ARRAY) continue;                  \
                zval *_za = zend_hash_index_find(Z_ARRVAL_P(_p), 0);     \
                zval *_zr = zend_hash_index_find(Z_ARRVAL_P(_p), 1);     \
                if (!_za || !_zr) continue;                              \
                double _a, _r;                                           \
                if (fastchart_zval_to_double(_za, &_a) != 0) continue;   \
                if (fastchart_zval_to_double(_zr, &_r) != 0) continue;   \
                if (_r < 0) _r = 0;                                      \
                (slot_)->angles[_k] = _a;                                \
                (slot_)->radii[_k]  = _r;                                \
                _k++;                                                    \
            } ZEND_HASH_FOREACH_END();                                   \
            (slot_)->len = _k;                                           \
            if (_k == 0) {                                               \
                efree((slot_)->angles); efree((slot_)->radii);           \
                (slot_)->angles = NULL; (slot_)->radii = NULL;           \
            }                                                            \
        }                                                                \
    } while (0)

    if (is_multi) {
        zval *s;
        ZEND_HASH_FOREACH_VAL(ht, s) {
            if (self->n_series >= FASTCHART_MAX_POLAR_SERIES) break;
            if (Z_TYPE_P(s) != IS_ARRAY) continue;
            zval *d = zend_hash_str_find(Z_ARRVAL_P(s), "data", sizeof("data") - 1);
            if (!d || Z_TYPE_P(d) != IS_ARRAY) continue;
            fastchart_polar_series *slot = &self->series[self->n_series];
            POLAR_PARSE_DATA(slot, Z_ARRVAL_P(d));
            zval *l = zend_hash_str_find(Z_ARRVAL_P(s), "label", sizeof("label") - 1);
            slot->label = fc_strdup_opt(fastchart_label_or_null(l));
            zval *c = zend_hash_str_find(Z_ARRVAL_P(s), "color", sizeof("color") - 1);
            slot->color_rgb = -1;
            if (c && Z_TYPE_P(c) == IS_LONG) {
                zend_long cc = Z_LVAL_P(c);
                if (cc >= 0 && cc <= 0xFFFFFF) slot->color_rgb = (int)cc;
            }
            self->n_series++;
        } ZEND_HASH_FOREACH_END();
    } else {
        fastchart_polar_series *slot = &self->series[0];
        POLAR_PARSE_DATA(slot, ht);
        slot->label = NULL;
        slot->color_rgb = -1;
        self->n_series = 1;
    }
#undef POLAR_PARSE_DATA
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BoxPlot, setBoxes)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_boxplot_obj *self = Z_FASTCHART_BOXPLOT_OBJ_P(ZEND_THIS);
    if (self->entries) {
        for (int i = 0; i < self->entry_count; i++) {
            fc_efree_opt(self->entries[i].label);
            fc_efree_opt(self->entries[i].outliers);
        }
        efree(self->entries);
        self->entries = NULL;
    }
    self->entry_count = 0;

    HashTable *ht = Z_ARRVAL_P(arr);
    int n = (int)zend_hash_num_elements(ht);
    if (n == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);
    if (n > FASTCHART_MAX_BOXPLOT_ENTRIES) n = FASTCHART_MAX_BOXPLOT_ENTRIES;
    self->entries = ecalloc((size_t)n, sizeof(fastchart_boxplot_entry));
    int slot = 0;

    zval *entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (slot >= n) break;
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        HashTable *eh = Z_ARRVAL_P(entry);
        fastchart_boxplot_entry *out = &self->entries[slot];
        out->label = NULL;
        out->outliers = NULL;
        out->outlier_count = 0;

        /* Two accepted shapes:
         *   ['min'=>, 'q1'=>, 'median'=>, 'q3'=>, 'max'=>, 'label'?, 'outliers'?]
         *   [min, q1, median, q3, max]  positional
         * Detection: any 'min' string key triggers the dict shape. */
        bool dict_shape = (zend_hash_str_find(eh, "min", 3) != NULL);
        if (dict_shape) {
            zval *zmin = zend_hash_str_find(eh, "min", 3);
            zval *zq1  = zend_hash_str_find(eh, "q1",  2);
            zval *zmed = zend_hash_str_find(eh, "median", 6);
            zval *zq3  = zend_hash_str_find(eh, "q3",  2);
            zval *zmax = zend_hash_str_find(eh, "max", 3);
            if (!zmin || !zq1 || !zmed || !zq3 || !zmax) continue;
            if (fastchart_zval_to_double(zmin, &out->min) != 0) continue;
            if (fastchart_zval_to_double(zq1,  &out->q1) != 0) continue;
            if (fastchart_zval_to_double(zmed, &out->median) != 0) continue;
            if (fastchart_zval_to_double(zq3,  &out->q3) != 0) continue;
            if (fastchart_zval_to_double(zmax, &out->max) != 0) continue;
            zval *zlabel = zend_hash_str_find(eh, "label", 5);
            out->label = fc_strdup_opt(fastchart_label_or_null(zlabel));
            zval *zout = zend_hash_str_find(eh, "outliers", 8);
            if (zout && Z_TYPE_P(zout) == IS_ARRAY) {
                uint32_t uon = zend_hash_num_elements(Z_ARRVAL_P(zout));
                if (uon > FASTCHART_MAX_OUTLIERS) uon = FASTCHART_MAX_OUTLIERS;
                int on = (int)uon;
                if (on > 0) {
                    out->outliers = ecalloc((size_t)on, sizeof(double));
                    int k = 0;
                    zval *v;
                    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zout), v) {
                        if (k >= on) break;
                        double d;
                        if (fastchart_zval_to_double(v, &d) == 0) out->outliers[k++] = d;
                    } ZEND_HASH_FOREACH_END();
                    out->outlier_count = k;
                    if (k == 0) { efree(out->outliers); out->outliers = NULL; }
                }
            }
        } else {
            zval *z;
            z = zend_hash_index_find(eh, 0); if (!z || fastchart_zval_to_double(z, &out->min) != 0) continue;
            z = zend_hash_index_find(eh, 1); if (!z || fastchart_zval_to_double(z, &out->q1) != 0) continue;
            z = zend_hash_index_find(eh, 2); if (!z || fastchart_zval_to_double(z, &out->median) != 0) continue;
            z = zend_hash_index_find(eh, 3); if (!z || fastchart_zval_to_double(z, &out->q3) != 0) continue;
            z = zend_hash_index_find(eh, 4); if (!z || fastchart_zval_to_double(z, &out->max) != 0) continue;
        }
        slot++;
    } ZEND_HASH_FOREACH_END();
    self->entry_count = slot;
    if (slot == 0) {
        efree(self->entries);
        self->entries = NULL;
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BubbleChart, setPoints)
{
    zval *arr;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_bubble_obj *self = Z_FASTCHART_BUBBLE_OBJ_P(ZEND_THIS);
    if (self->points) { efree(self->points); self->points = NULL; }
    self->point_count = 0;

    HashTable *ht = Z_ARRVAL_P(arr);
    int n = (int)zend_hash_num_elements(ht);
    if (n == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);
    if (n > FASTCHART_MAX_BUBBLE_POINTS) n = FASTCHART_MAX_BUBBLE_POINTS;
    self->points = ecalloc((size_t)n, sizeof(fastchart_bubble_point));
    int slot = 0;
    zval *p;
    ZEND_HASH_FOREACH_VAL(ht, p) {
        if (slot >= n) break;
        if (Z_TYPE_P(p) != IS_ARRAY) continue;
        HashTable *t = Z_ARRVAL_P(p);
        zval *zx = zend_hash_index_find(t, 0);
        zval *zy = zend_hash_index_find(t, 1);
        zval *zs = zend_hash_index_find(t, 2);
        if (!zx || !zy || !zs) continue;
        double dx, dy, ds;
        if (fastchart_zval_to_double(zx, &dx) != 0) continue;
        if (fastchart_zval_to_double(zy, &dy) != 0) continue;
        if (fastchart_zval_to_double(zs, &ds) != 0) continue;
        if (ds < 0) ds = 0;
        self->points[slot].x = dx;
        self->points[slot].y = dy;
        self->points[slot].size = ds;
        self->points[slot].color_rgb = -1;
        zval *zc = zend_hash_index_find(t, 3);
        if (zc && Z_TYPE_P(zc) == IS_LONG) {
            zend_long c = Z_LVAL_P(zc);
            if (c >= 0 && c <= 0xFFFFFF) self->points[slot].color_rgb = (int)c;
        }
        slot++;
    } ZEND_HASH_FOREACH_END();
    self->point_count = slot;
    if (slot == 0) {
        efree(self->points);
        self->points = NULL;
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BarChart, setStacked)
{
    bool stacked;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(stacked)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_bar_obj *self = Z_FASTCHART_BAR_OBJ_P(ZEND_THIS);
    self->stacked = stacked;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_BarChart, setOrientation)
{
    zend_long orientation;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(orientation)
    ZEND_PARSE_PARAMETERS_END();
    if (orientation != FASTCHART_BAR_VERTICAL &&
        orientation != FASTCHART_BAR_HORIZONTAL) {
        zend_value_error("FastChart\\BarChart::setOrientation() expects BAR_VERTICAL or BAR_HORIZONTAL");
        RETURN_THROWS();
    }
    fastchart_bar_obj *self = Z_FASTCHART_BAR_OBJ_P(ZEND_THIS);
    self->bar_orientation = orientation;
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

    fastchart_pie_obj *self = Z_FASTCHART_PIE_OBJ_P(ZEND_THIS);
    self->donut_hole_ratio = ratio;

    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, addMovingAverage)
{
    zend_long period;
    zend_long type = FASTCHART_MA_SMA;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_LONG(period)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(type)
    ZEND_PARSE_PARAMETERS_END();

    if (period < 2 || period > INT_MAX) {
        zend_value_error("FastChart\\StockChart::addMovingAverage() period must be >= 2");
        RETURN_THROWS();
    }
    if (type != FASTCHART_MA_SMA && type != FASTCHART_MA_EMA && type != FASTCHART_MA_WMA) {
        zend_value_error("FastChart\\StockChart::addMovingAverage() type must be MA_SMA, MA_EMA or MA_WMA");
        RETURN_THROWS();
    }

    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    if (self->sma_count >= FASTCHART_MAX_SMA) {
        zend_value_error("FastChart\\StockChart::addMovingAverage() supports at most %d overlays",
                         FASTCHART_MAX_SMA);
        RETURN_THROWS();
    }
    self->sma_periods[self->sma_count] = (int)period;
    self->sma_types[self->sma_count] = (int)type;
    self->sma_count++;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, setMovingAverages)
{
    zval *periods;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(periods)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(periods);
    self->sma_count = 0;
    zval *p;
    ZEND_HASH_FOREACH_VAL(ht, p) {
        if (self->sma_count >= FASTCHART_MAX_SMA) break;
        zend_long pp;
        if (fastchart_zval_to_long(p, &pp) == 0 && pp >= 2 && pp <= INT_MAX) {
            self->sma_periods[self->sma_count] = (int)pp;
            self->sma_types[self->sma_count] = FASTCHART_MA_SMA;
            self->sma_count++;
        }
    } ZEND_HASH_FOREACH_END();
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

FASTCHART_BOOL_SETTER_AS(FastChart_StockChart, setVolumePane, Z_FASTCHART_STOCK_OBJ_P, volume_pane)

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

    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    if (self->indicator_pane_count >= FASTCHART_MAX_INDICATOR_PANES) {
        zend_value_error("FastChart\\StockChart::addIndicatorPane() supports at most 3 panes");
        RETURN_THROWS();
    }

    /* Parse values array into a typed double[]. Non-numeric / non-
     * finite cells become NaN so the renderer can break the line at
     * those gaps. */
    HashTable *vht = Z_ARRVAL_P(values);
    uint32_t uvn = zend_hash_num_elements(vht);
    if (uvn > FASTCHART_MAX_INDICATOR_VALUES) uvn = FASTCHART_MAX_INDICATOR_VALUES;
    int vn = (int)uvn;
    double *parsed_values = vn > 0 ? emalloc((size_t)vn * sizeof(double)) : NULL;
    int idx = 0;
    zval *vv;
    ZEND_HASH_FOREACH_VAL(vht, vv) {
        if (idx >= vn) break;
        double d;
        if (fastchart_zval_to_double(vv, &d) == 0 && isfinite(d)) {
            parsed_values[idx] = d;
        } else {
            parsed_values[idx] = NAN;
        }
        idx++;
    } ZEND_HASH_FOREACH_END();

    fastchart_indicator_pane *p = &self->indicator_panes[self->indicator_pane_count];
    size_t name_len = ZSTR_LEN(name);
    p->name = emalloc(name_len + 1);
    memcpy(p->name, ZSTR_VAL(name), name_len + 1);
    p->values = parsed_values;
    p->value_count = idx;
    p->has_color = false;     p->color_rgb = 0;
    p->has_reference = false; p->reference = 0.0;
    p->has_min = false;       p->min = 0.0;
    p->has_max = false;       p->max = 0.0;

    if (opts) {
        zval *opt;
        opt = zend_hash_str_find(opts, "color", sizeof("color") - 1);
        if (opt && Z_TYPE_P(opt) == IS_LONG) {
            zend_long c = Z_LVAL_P(opt);
            if (c >= 0 && c <= 0xFFFFFF) {
                p->has_color = true;
                p->color_rgb = (int)c;
            }
        }
        double d;
        opt = zend_hash_str_find(opts, "reference", sizeof("reference") - 1);
        if (opt && fastchart_zval_to_double(opt, &d) == 0 && isfinite(d)) {
            p->has_reference = true; p->reference = d;
        }
        opt = zend_hash_str_find(opts, "min", sizeof("min") - 1);
        if (opt && fastchart_zval_to_double(opt, &d) == 0 && isfinite(d)) {
            p->has_min = true; p->min = d;
        }
        opt = zend_hash_str_find(opts, "max", sizeof("max") - 1);
        if (opt && fastchart_zval_to_double(opt, &d) == 0 && isfinite(d)) {
            p->has_max = true; p->max = d;
        }
    }

    self->indicator_pane_count++;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Take ownership of `values` (efree'd by the dtor on failure or with
 * the chart on success) and clone the literal `name` into emalloc'd
 * storage. Returns 0 on success, -1 if the indicator-pane cap is
 * exhausted. The four native indicators below funnel through this
 * helper so all of them share the existing pane render path. */
static int push_indicator_pane(fastchart_stock_obj *self,
                               const char *name, double *values, int n,
                               bool has_reference, double reference,
                               bool has_min, double min,
                               bool has_max, double max)
{
    if (self->indicator_pane_count >= FASTCHART_MAX_INDICATOR_PANES) {
        if (values) efree(values);
        return -1;
    }
    fastchart_indicator_pane *p = &self->indicator_panes[self->indicator_pane_count];
    size_t len = strlen(name);
    p->name = emalloc(len + 1);
    memcpy(p->name, name, len + 1);
    p->values = values;
    p->value_count = n;
    p->has_color = false; p->color_rgb = 0;
    p->has_reference = has_reference; p->reference = reference;
    p->has_min = has_min; p->min = min;
    p->has_max = has_max; p->max = max;
    self->indicator_pane_count++;
    return 0;
}

/* Common preamble for native stock indicators: require setOhlcv
 * to have been called and return the candle pointer + count. The
 * indicators are computed eagerly at add() time, so the candle
 * data must already be present; calling setOhlcv() AFTER an
 * addX() leaves the panes pointing at values from the prior
 * candle set. Documented in the stub. */
static fastchart_candle *stock_require_candles(fastchart_stock_obj *self,
                                               const char *method_str,
                                               int *out_n)
{
    if (!self->candles || self->candle_count == 0) {
        zend_throw_error(NULL,
            "%s requires setOhlcv() to have been called first", method_str);
        return NULL;
    }
    *out_n = self->candle_count;
    return self->candles;
}

ZEND_METHOD(FastChart_StockChart, addRSI)
{
    zend_long period = 14;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(period)
    ZEND_PARSE_PARAMETERS_END();

    if (period < 2 || period > FASTCHART_MAX_INDICATOR_VALUES) {
        zend_value_error(
            "FastChart\\StockChart::addRSI() period must be in [2, %d]",
            FASTCHART_MAX_INDICATOR_VALUES);
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self, "FastChart\\StockChart::addRSI()", &n);
    if (!c) RETURN_THROWS();
    if ((int)period >= n) {
        zend_value_error(
            "FastChart\\StockChart::addRSI() period (%d) must be < candle count (%d)",
            (int)period, n);
        RETURN_THROWS();
    }

    /* Wilder's RSI: seed avg_gain / avg_loss with the SMA of the
     * first `period` close-to-close differences, then update with
     * the standard recurrence avg = (avg*(p-1) + cur) / p. The
     * warm-up window [0..period] stays NaN so the renderer breaks
     * the line at the gap. */
    double *out = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) out[i] = NAN;

    double avg_gain = 0, avg_loss = 0;
    int p = (int)period;
    for (int i = 1; i <= p; i++) {
        double diff = c[i].close - c[i - 1].close;
        if (diff > 0) avg_gain += diff;
        else          avg_loss -= diff;
    }
    avg_gain /= (double)p;
    avg_loss /= (double)p;
    out[p] = avg_loss == 0.0 ? 100.0
                             : 100.0 - 100.0 / (1.0 + avg_gain / avg_loss);

    double inv_p = 1.0 / (double)p;
    for (int i = p + 1; i < n; i++) {
        double diff = c[i].close - c[i - 1].close;
        double gain = diff > 0 ? diff : 0.0;
        double loss = diff < 0 ? -diff : 0.0;
        avg_gain = (avg_gain * (double)(p - 1) + gain) * inv_p;
        avg_loss = (avg_loss * (double)(p - 1) + loss) * inv_p;
        out[i] = avg_loss == 0.0 ? 100.0
                                 : 100.0 - 100.0 / (1.0 + avg_gain / avg_loss);
    }

    char name[32];
    snprintf(name, sizeof(name), "RSI(%d)", p);
    if (push_indicator_pane(self, name, out, n,
                            true, 50.0,
                            true, 0.0,
                            true, 100.0) != 0) {
        zend_value_error(
            "FastChart\\StockChart::addRSI() exceeds the indicator-pane cap of %d",
            FASTCHART_MAX_INDICATOR_PANES);
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, addMomentum)
{
    zend_long period = 10;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(period)
    ZEND_PARSE_PARAMETERS_END();

    if (period < 1 || period > FASTCHART_MAX_INDICATOR_VALUES) {
        zend_value_error(
            "FastChart\\StockChart::addMomentum() period must be in [1, %d]",
            FASTCHART_MAX_INDICATOR_VALUES);
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self,
        "FastChart\\StockChart::addMomentum()", &n);
    if (!c) RETURN_THROWS();
    if ((int)period >= n) {
        zend_value_error(
            "FastChart\\StockChart::addMomentum() period (%d) must be < candle count (%d)",
            (int)period, n);
        RETURN_THROWS();
    }

    /* Plain difference: close[i] - close[i-period]. NaN before warm-up. */
    double *out = emalloc((size_t)n * sizeof(double));
    int p = (int)period;
    for (int i = 0; i < p; i++) out[i] = NAN;
    for (int i = p; i < n; i++) {
        out[i] = c[i].close - c[i - p].close;
    }

    char name[32];
    snprintf(name, sizeof(name), "MOM(%d)", p);
    if (push_indicator_pane(self, name, out, n,
                            true, 0.0,
                            false, 0.0,
                            false, 0.0) != 0) {
        zend_value_error(
            "FastChart\\StockChart::addMomentum() exceeds the indicator-pane cap of %d",
            FASTCHART_MAX_INDICATOR_PANES);
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, addROC)
{
    zend_long period = 10;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(period)
    ZEND_PARSE_PARAMETERS_END();

    if (period < 1 || period > FASTCHART_MAX_INDICATOR_VALUES) {
        zend_value_error(
            "FastChart\\StockChart::addROC() period must be in [1, %d]",
            FASTCHART_MAX_INDICATOR_VALUES);
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self,
        "FastChart\\StockChart::addROC()", &n);
    if (!c) RETURN_THROWS();
    if ((int)period >= n) {
        zend_value_error(
            "FastChart\\StockChart::addROC() period (%d) must be < candle count (%d)",
            (int)period, n);
        RETURN_THROWS();
    }

    /* Rate of change: (close[i] / close[i-period] - 1) * 100. NaN
     * before warm-up; NaN if the prior close was zero (avoids /0). */
    double *out = emalloc((size_t)n * sizeof(double));
    int p = (int)period;
    for (int i = 0; i < p; i++) out[i] = NAN;
    for (int i = p; i < n; i++) {
        double prev = c[i - p].close;
        out[i] = prev == 0.0 ? NAN : (c[i].close / prev - 1.0) * 100.0;
    }

    char name[32];
    snprintf(name, sizeof(name), "ROC(%d)", p);
    if (push_indicator_pane(self, name, out, n,
                            true, 0.0,
                            false, 0.0,
                            false, 0.0) != 0) {
        zend_value_error(
            "FastChart\\StockChart::addROC() exceeds the indicator-pane cap of %d",
            FASTCHART_MAX_INDICATOR_PANES);
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, addOBV)
{
    ZEND_PARSE_PARAMETERS_NONE();

    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self,
        "FastChart\\StockChart::addOBV()", &n);
    if (!c) RETURN_THROWS();

    /* On-balance volume: cumulative sum of signed volume. Sign flips
     * with the close-to-close direction. Bars with no volume
     * (has_volume == 0) contribute 0, matching the convention used
     * by the volume-pane renderer for missing data. The first bar
     * has no prior close, so OBV[0] = 0 by definition. */
    double *out = emalloc((size_t)n * sizeof(double));
    out[0] = 0.0;
    double running = 0.0;
    for (int i = 1; i < n; i++) {
        if (c[i].has_volume) {
            if (c[i].close > c[i - 1].close)      running += c[i].volume;
            else if (c[i].close < c[i - 1].close) running -= c[i].volume;
        }
        out[i] = running;
    }

    if (push_indicator_pane(self, "OBV", out, n,
                            false, 0.0,
                            false, 0.0,
                            false, 0.0) != 0) {
        zend_value_error(
            "FastChart\\StockChart::addOBV() exceeds the indicator-pane cap of %d",
            FASTCHART_MAX_INDICATOR_PANES);
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Compute an EMA series of length n from `src` with the given period.
 * Seeds with the SMA of the first `period` values to keep the warm-up
 * region stable. Out indices [0..period-1] are NaN. */
static void compute_ema(const double *src, int n, int period, double *out)
{
    for (int i = 0; i < n && i < period; i++) out[i] = NAN;
    if (n <= period) return;
    double sum = 0;
    for (int i = 0; i < period; i++) sum += src[i];
    double ema = sum / (double)period;
    out[period - 1] = ema;
    double alpha = 2.0 / ((double)period + 1.0);
    for (int i = period; i < n; i++) {
        ema = alpha * src[i] + (1.0 - alpha) * ema;
        out[i] = ema;
    }
}

ZEND_METHOD(FastChart_StockChart, addMACD)
{
    zend_long fast = 12, slow = 26, signal_p = 9;
    ZEND_PARSE_PARAMETERS_START(0, 3)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(fast)
        Z_PARAM_LONG(slow)
        Z_PARAM_LONG(signal_p)
    ZEND_PARSE_PARAMETERS_END();
    /* Upper-bound every period before the cast to int. signal_p in
     * particular is later used as an array index — an unbounded
     * zend_long becomes a wraparound int and walks the buffer. */
    if (fast < 2 || slow < 2 || signal_p < 2 ||
        fast >= slow ||
        slow > FASTCHART_MAX_INDICATOR_VALUES ||
        signal_p > FASTCHART_MAX_INDICATOR_VALUES) {
        zend_value_error(
            "FastChart\\StockChart::addMACD() requires 2 <= fast < slow and 2 <= signal <= %d (default 12, 26, 9)",
            FASTCHART_MAX_INDICATOR_VALUES);
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self,
        "FastChart\\StockChart::addMACD()", &n);
    if (!c) RETURN_THROWS();
    if ((int)slow >= n) {
        zend_value_error(
            "FastChart\\StockChart::addMACD() slow period (%d) must be < candle count (%d)",
            (int)slow, n);
        RETURN_THROWS();
    }

    /* MACD line = EMA(fast) - EMA(slow); signal = EMA(MACD, signal);
     * histogram = MACD - signal. We collect everything into three
     * parallel arrays and stash them on the pane via the multi-series
     * fields. NaN fills the warm-up region. */
    double *closes = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) closes[i] = c[i].close;

    double *ema_fast = emalloc((size_t)n * sizeof(double));
    double *ema_slow = emalloc((size_t)n * sizeof(double));
    compute_ema(closes, n, (int)fast, ema_fast);
    compute_ema(closes, n, (int)slow, ema_slow);

    double *macd = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) {
        macd[i] = (isnan(ema_fast[i]) || isnan(ema_slow[i]))
            ? NAN : ema_fast[i] - ema_slow[i];
    }

    /* Signal: EMA over MACD. Skip the warm-up NaN region by feeding
     * the first `signal_p` non-NaN MACD values as the seed. */
    double *signal = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) signal[i] = NAN;
    int first_valid = (int)slow - 1;
    if (first_valid + (int)signal_p <= n) {
        double sum = 0;
        for (int i = first_valid; i < first_valid + (int)signal_p; i++) sum += macd[i];
        double sig = sum / (double)signal_p;
        signal[first_valid + (int)signal_p - 1] = sig;
        double a = 2.0 / ((double)signal_p + 1.0);
        for (int i = first_valid + (int)signal_p; i < n; i++) {
            sig = a * macd[i] + (1.0 - a) * sig;
            signal[i] = sig;
        }
    }

    double *hist = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) {
        hist[i] = (isnan(macd[i]) || isnan(signal[i])) ? NAN : macd[i] - signal[i];
    }

    efree(closes);
    efree(ema_fast);
    efree(ema_slow);

    char name[40];
    snprintf(name, sizeof(name), "MACD(%d,%d,%d)",
             (int)fast, (int)slow, (int)signal_p);
    if (push_indicator_pane(self, name, macd, n,
                            true, 0.0,
                            false, 0.0,
                            false, 0.0) != 0) {
        efree(signal);
        efree(hist);
        zend_value_error(
            "FastChart\\StockChart::addMACD() exceeds the indicator-pane cap of %d",
            FASTCHART_MAX_INDICATOR_PANES);
        RETURN_THROWS();
    }
    fastchart_indicator_pane *p = &self->indicator_panes[self->indicator_pane_count - 1];
    p->values2 = signal;
    p->values3 = hist;
    p->histogram_third = true;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, addStochastic)
{
    zend_long period = 14, smooth = 3;
    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(period)
        Z_PARAM_LONG(smooth)
    ZEND_PARSE_PARAMETERS_END();
    /* Upper-bound smooth before any cast — same reasoning as
     * addMACD: smooth drives index arithmetic at draw time and an
     * unbounded zend_long wraps to a destructive int. */
    if (period < 2 || smooth < 1 ||
        period > FASTCHART_MAX_INDICATOR_VALUES ||
        smooth > FASTCHART_MAX_INDICATOR_VALUES) {
        zend_value_error(
            "FastChart\\StockChart::addStochastic() requires 2 <= period <= %d and 1 <= smooth <= %d",
            FASTCHART_MAX_INDICATOR_VALUES, FASTCHART_MAX_INDICATOR_VALUES);
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self,
        "FastChart\\StockChart::addStochastic()", &n);
    if (!c) RETURN_THROWS();
    if ((int)period >= n) {
        zend_value_error(
            "FastChart\\StockChart::addStochastic() period (%d) must be < candle count (%d)",
            (int)period, n);
        RETURN_THROWS();
    }

    /* %K[i] = (close[i] - low_p[i]) / (high_p[i] - low_p[i]) * 100
     *   where high_p / low_p are the rolling max(high) / min(low)
     *   over the previous `period` bars (inclusive of i).
     * %D = SMA(%K, smooth). Values outside the warm-up window are
     * NaN. */
    double *k = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) k[i] = NAN;
    int p = (int)period;
    for (int i = p - 1; i < n; i++) {
        double hh = c[i].high, ll = c[i].low;
        for (int j = i - p + 1; j <= i; j++) {
            if (c[j].high > hh) hh = c[j].high;
            if (c[j].low  < ll) ll = c[j].low;
        }
        k[i] = (hh > ll) ? (c[i].close - ll) / (hh - ll) * 100.0 : 50.0;
    }
    double *d = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) d[i] = NAN;
    int s = (int)smooth;
    if (s > 1) {
        for (int i = p - 1 + s - 1; i < n; i++) {
            double sum = 0;
            for (int j = i - s + 1; j <= i; j++) sum += k[j];
            d[i] = sum / (double)s;
        }
    } else {
        for (int i = 0; i < n; i++) d[i] = k[i];
    }

    char name[40];
    snprintf(name, sizeof(name), "Stoch(%d,%d)", (int)period, (int)smooth);
    if (push_indicator_pane(self, name, k, n,
                            false, 0.0,
                            true, 0.0,
                            true, 100.0) != 0) {
        efree(d);
        zend_value_error(
            "FastChart\\StockChart::addStochastic() exceeds the indicator-pane cap of %d",
            FASTCHART_MAX_INDICATOR_PANES);
        RETURN_THROWS();
    }
    fastchart_indicator_pane *pane = &self->indicator_panes[self->indicator_pane_count - 1];
    pane->values2 = d;
    pane->histogram_third = false;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* Push a freshly-computed price overlay onto `self`. Takes
 * ownership of a/b/c (efree'd by the dtor on success or here on
 * failure). Returns 0 on success, -1 if the overlay cap is full. */
static int push_price_overlay(fastchart_stock_obj *self, int kind,
                              double *a, double *b, double *c, int n,
                              int color_rgb)
{
    if (self->overlay_count >= FASTCHART_MAX_PRICE_OVERLAYS) {
        if (a) efree(a);
        if (b) efree(b);
        if (c) efree(c);
        return -1;
    }
    fastchart_price_overlay *ov = &self->overlays[self->overlay_count];
    ov->kind = kind;
    ov->a = a; ov->b = b; ov->c = c; ov->n = n;
    ov->color_rgb = color_rgb;
    self->overlay_count++;
    return 0;
}

ZEND_METHOD(FastChart_StockChart, addBollingerBands)
{
    zend_long period = 20;
    double n_stddev = 2.0;
    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(period)
        Z_PARAM_DOUBLE(n_stddev)
    ZEND_PARSE_PARAMETERS_END();
    if (period < 2 || period > FASTCHART_MAX_INDICATOR_VALUES ||
        !isfinite(n_stddev) || n_stddev <= 0) {
        zend_value_error(
            "FastChart\\StockChart::addBollingerBands() requires period >= 2 and n_stddev > 0");
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self,
        "FastChart\\StockChart::addBollingerBands()", &n);
    if (!c) RETURN_THROWS();
    if ((int)period >= n) {
        zend_value_error(
            "FastChart\\StockChart::addBollingerBands() period (%d) must be < candle count (%d)",
            (int)period, n);
        RETURN_THROWS();
    }

    /* Middle = SMA(close, period). Standard deviation over the
     * same window. Upper = middle + n*sigma; lower = middle - n*sigma. */
    double *mid = emalloc((size_t)n * sizeof(double));
    double *up  = emalloc((size_t)n * sizeof(double));
    double *lo  = emalloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) mid[i] = up[i] = lo[i] = NAN;
    int p = (int)period;
    double sum = 0, sum_sq = 0;
    for (int i = 0; i < p; i++) {
        sum    += c[i].close;
        sum_sq += c[i].close * c[i].close;
    }
    /* Sliding-window mean+variance: O(n). */
    for (int i = p - 1; i < n; i++) {
        if (i >= p) {
            double drop = c[i - p].close;
            double add  = c[i].close;
            sum    += add - drop;
            sum_sq += add * add - drop * drop;
        }
        double mean = sum / (double)p;
        double var  = sum_sq / (double)p - mean * mean;
        if (var < 0) var = 0;  /* float guard */
        double sigma = sqrt(var);
        mid[i] = mean;
        up[i]  = mean + n_stddev * sigma;
        lo[i]  = mean - n_stddev * sigma;
    }

    if (push_price_overlay(self, FASTCHART_OVERLAY_BOLL, mid, up, lo, n, -1) != 0) {
        zend_value_error(
            "FastChart\\StockChart::addBollingerBands() exceeds the price-overlay cap of %d",
            FASTCHART_MAX_PRICE_OVERLAYS);
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_StockChart, addParabolicSAR)
{
    double af_init = 0.02;
    double af_max  = 0.2;
    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(af_init)
        Z_PARAM_DOUBLE(af_max)
    ZEND_PARSE_PARAMETERS_END();
    if (!isfinite(af_init) || !isfinite(af_max) ||
        af_init <= 0 || af_max <= 0 || af_init > af_max) {
        zend_value_error(
            "FastChart\\StockChart::addParabolicSAR() requires 0 < af_init <= af_max");
        RETURN_THROWS();
    }
    fastchart_stock_obj *self = Z_FASTCHART_STOCK_OBJ_P(ZEND_THIS);
    int n = 0;
    fastchart_candle *c = stock_require_candles(self,
        "FastChart\\StockChart::addParabolicSAR()", &n);
    if (!c) RETURN_THROWS();
    if (n < 3) {
        zend_value_error(
            "FastChart\\StockChart::addParabolicSAR() needs >= 3 candles");
        RETURN_THROWS();
    }

    /* Wilder's Parabolic SAR. State: trend (up/down), SAR price,
     * extreme point EP, acceleration factor AF.
     *
     *   - On each bar: SAR(i+1) = SAR(i) + AF * (EP - SAR(i)),
     *     bounded by the prior two bars' lows (uptrend) or highs
     *     (downtrend).
     *   - When price crosses SAR, flip the trend: new SAR = old EP,
     *     reset AF, set EP to the bar's price extreme.
     *   - When the bar's extreme exceeds the EP, advance EP and
     *     bump AF by af_init (capped at af_max).
     *
     * Seed: assume an uptrend, EP = candle 0 high, SAR = candle 0
     * low. Output at index 0 is the seed SAR (slightly below price);
     * subsequent indices are the projected SAR for that bar. */
    double *sar = emalloc((size_t)n * sizeof(double));
    int up = 1;
    double ep = c[0].high;
    double s  = c[0].low;
    double af = af_init;
    sar[0] = s;
    for (int i = 1; i < n; i++) {
        s = s + af * (ep - s);
        if (up) {
            /* Bound by min(low[i-1], low[i-2]) — SAR can't exceed
             * the prior two candles' lows in an uptrend. */
            if (i >= 2 && c[i - 2].low < s) s = c[i - 2].low;
            if (c[i - 1].low < s)           s = c[i - 1].low;
            if (c[i].low < s) {
                /* Flip to downtrend. */
                up = 0;
                s = ep;
                ep = c[i].low;
                af = af_init;
            } else {
                if (c[i].high > ep) {
                    ep = c[i].high;
                    af += af_init;
                    if (af > af_max) af = af_max;
                }
            }
        } else {
            if (i >= 2 && c[i - 2].high > s) s = c[i - 2].high;
            if (c[i - 1].high > s)           s = c[i - 1].high;
            if (c[i].high > s) {
                up = 1;
                s = ep;
                ep = c[i].high;
                af = af_init;
            } else {
                if (c[i].low < ep) {
                    ep = c[i].low;
                    af += af_init;
                    if (af > af_max) af = af_max;
                }
            }
        }
        sar[i] = s;
    }

    if (push_price_overlay(self, FASTCHART_OVERLAY_PSAR, sar, NULL, NULL, n, -1) != 0) {
        zend_value_error(
            "FastChart\\StockChart::addParabolicSAR() exceeds the price-overlay cap of %d",
            FASTCHART_MAX_PRICE_OVERLAYS);
        RETURN_THROWS();
    }
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Treemap, setItems)
{
    zval *items;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(items)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_treemap_obj *self = Z_FASTCHART_TREEMAP_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(items);
    uint32_t un = zend_hash_num_elements(ht);
    if (un > FASTCHART_MAX_TREEMAP_ITEMS) un = FASTCHART_MAX_TREEMAP_ITEMS;
    int n = (int)un;

    /* Free any prior items (setItems is idempotent — replace, don't
     * accumulate). */
    if (self->items) {
        for (int i = 0; i < self->item_count; i++) {
            if (self->items[i].label) efree(self->items[i].label);
        }
        efree(self->items);
        self->items = NULL;
        self->item_count = 0;
    }
    if (n == 0) {
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }

    fastchart_treemap_item *parsed = ecalloc((size_t)n, sizeof(*parsed));
    int idx = 0;

    zval *entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (idx >= n) break;
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        HashTable *eht = Z_ARRVAL_P(entry);

        /* Required: numeric `value`, must be > 0 to claim area.
         * Items with non-positive values are silently dropped here
         * to keep setItems() best-effort consistent with the other
         * shape parsers (Pie, Scatter). */
        zval *zv = zend_hash_str_find(eht, "value", sizeof("value") - 1);
        if (!zv) continue;
        double v;
        if (fastchart_zval_to_double(zv, &v) != 0 || !isfinite(v) || v <= 0) {
            continue;
        }
        parsed[idx].value = v;

        /* Optional `label`: clone into emalloc'd storage. NUL-bearing
         * labels are dropped (rendered text would terminate at the
         * NUL anyway). */
        zval *zl = zend_hash_str_find(eht, "label", sizeof("label") - 1);
        if (zl && Z_TYPE_P(zl) == IS_STRING) {
            size_t len = Z_STRLEN_P(zl);
            const char *s = Z_STRVAL_P(zl);
            if (memchr(s, 0, len) == NULL && len > 0) {
                parsed[idx].label = emalloc(len + 1);
                memcpy(parsed[idx].label, s, len);
                parsed[idx].label[len] = '\0';
            }
        }

        /* Optional `color`: 24-bit RGB; out-of-range silently
         * defaults to palette pick. */
        parsed[idx].color_rgb = -1;
        zval *zc = zend_hash_str_find(eht, "color", sizeof("color") - 1);
        if (zc && Z_TYPE_P(zc) == IS_LONG) {
            zend_long c = Z_LVAL_P(zc);
            if (c >= 0 && c <= 0xFFFFFF) parsed[idx].color_rgb = (int)c;
        }

        idx++;
    } ZEND_HASH_FOREACH_END();

    if (idx == 0) {
        efree(parsed);
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    self->items = parsed;
    self->item_count = idx;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Treemap, setShowLabels)
{
    bool flag;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(flag)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_treemap_obj *self = Z_FASTCHART_TREEMAP_OBJ_P(ZEND_THIS);
    self->show_labels = flag;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Funnel, setStages)
{
    zval *stages;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(stages)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_funnel_obj *self = Z_FASTCHART_FUNNEL_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(stages);
    uint32_t un = zend_hash_num_elements(ht);
    if (un > FASTCHART_MAX_FUNNEL_STAGES) un = FASTCHART_MAX_FUNNEL_STAGES;
    int n = (int)un;

    if (self->stages) {
        for (int i = 0; i < self->stage_count; i++) {
            if (self->stages[i].label) efree(self->stages[i].label);
        }
        efree(self->stages);
        self->stages = NULL;
        self->stage_count = 0;
    }
    if (n == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    fastchart_funnel_stage *parsed = ecalloc((size_t)n, sizeof(*parsed));
    int idx = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (idx >= n) break;
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        HashTable *eht = Z_ARRVAL_P(entry);

        zval *zv = zend_hash_str_find(eht, "value", sizeof("value") - 1);
        if (!zv) continue;
        double v;
        if (fastchart_zval_to_double(zv, &v) != 0 || !isfinite(v) || v <= 0) continue;
        parsed[idx].value = v;

        zval *zl = zend_hash_str_find(eht, "label", sizeof("label") - 1);
        if (zl && Z_TYPE_P(zl) == IS_STRING) {
            size_t len = Z_STRLEN_P(zl);
            const char *s = Z_STRVAL_P(zl);
            if (len > 0 && memchr(s, 0, len) == NULL) {
                parsed[idx].label = emalloc(len + 1);
                memcpy(parsed[idx].label, s, len);
                parsed[idx].label[len] = '\0';
            }
        }

        parsed[idx].color_rgb = -1;
        zval *zc = zend_hash_str_find(eht, "color", sizeof("color") - 1);
        if (zc && Z_TYPE_P(zc) == IS_LONG) {
            zend_long c = Z_LVAL_P(zc);
            if (c >= 0 && c <= 0xFFFFFF) parsed[idx].color_rgb = (int)c;
        }
        idx++;
    } ZEND_HASH_FOREACH_END();

    if (idx == 0) {
        efree(parsed);
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    self->stages = parsed;
    self->stage_count = idx;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* setShowValues(bool, string) is on the Chart base; Funnel uses it as-is. */

ZEND_METHOD(FastChart_Waterfall, setBars)
{
    zval *bars;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(bars)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_waterfall_obj *self = Z_FASTCHART_WATERFALL_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(bars);
    uint32_t un = zend_hash_num_elements(ht);
    if (un > FASTCHART_MAX_WATERFALL_BARS) un = FASTCHART_MAX_WATERFALL_BARS;
    int n = (int)un;

    if (self->bars) {
        for (int i = 0; i < self->bar_count; i++) {
            if (self->bars[i].label) efree(self->bars[i].label);
        }
        efree(self->bars);
        self->bars = NULL;
        self->bar_count = 0;
    }
    if (n == 0) RETURN_ZVAL(ZEND_THIS, 1, 0);

    fastchart_waterfall_bar *parsed = ecalloc((size_t)n, sizeof(*parsed));
    int idx = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (idx >= n) break;
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        HashTable *eht = Z_ARRVAL_P(entry);

        zval *zv = zend_hash_str_find(eht, "value", sizeof("value") - 1);
        if (!zv) continue;
        double v;
        if (fastchart_zval_to_double(zv, &v) != 0 || !isfinite(v)) continue;
        parsed[idx].value = v;

        parsed[idx].kind = FASTCHART_WF_DELTA;
        zval *zk = zend_hash_str_find(eht, "kind", sizeof("kind") - 1);
        if (zk && Z_TYPE_P(zk) == IS_STRING) {
            /* Length-aware compare: "totalXYZ" must NOT match
             * "total" via strncmp prefix, otherwise typos silently
             * change chart semantics. */
            if (Z_STRLEN_P(zk) == 5 &&
                memcmp(Z_STRVAL_P(zk), "total", 5) == 0) {
                parsed[idx].kind = FASTCHART_WF_TOTAL;
            }
        } else if (zk && Z_TYPE_P(zk) == IS_LONG) {
            zend_long k = Z_LVAL_P(zk);
            if (k == FASTCHART_WF_TOTAL) parsed[idx].kind = FASTCHART_WF_TOTAL;
        }

        zval *zl = zend_hash_str_find(eht, "label", sizeof("label") - 1);
        if (zl && Z_TYPE_P(zl) == IS_STRING) {
            size_t len = Z_STRLEN_P(zl);
            const char *s = Z_STRVAL_P(zl);
            if (len > 0 && memchr(s, 0, len) == NULL) {
                parsed[idx].label = emalloc(len + 1);
                memcpy(parsed[idx].label, s, len);
                parsed[idx].label[len] = '\0';
            }
        }
        idx++;
    } ZEND_HASH_FOREACH_END();

    if (idx == 0) {
        efree(parsed);
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    self->bars = parsed;
    self->bar_count = idx;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Waterfall, setRiseColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_LONG(rgb) ZEND_PARSE_PARAMETERS_END();
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(rgb, "FastChart\\Waterfall::setRiseColor");
    fastchart_waterfall_obj *self = Z_FASTCHART_WATERFALL_OBJ_P(ZEND_THIS);
    self->rise_color = (int)rgb;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}
ZEND_METHOD(FastChart_Waterfall, setFallColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_LONG(rgb) ZEND_PARSE_PARAMETERS_END();
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(rgb, "FastChart\\Waterfall::setFallColor");
    fastchart_waterfall_obj *self = Z_FASTCHART_WATERFALL_OBJ_P(ZEND_THIS);
    self->fall_color = (int)rgb;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}
ZEND_METHOD(FastChart_Waterfall, setTotalColor)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_LONG(rgb) ZEND_PARSE_PARAMETERS_END();
    FASTCHART_VALIDATE_RGB_OR_DEFAULT(rgb, "FastChart\\Waterfall::setTotalColor");
    fastchart_waterfall_obj *self = Z_FASTCHART_WATERFALL_OBJ_P(ZEND_THIS);
    self->total_color = (int)rgb;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Heatmap, setGrid)
{
    zval *grid;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(grid)
    ZEND_PARSE_PARAMETERS_END();

    /* Reuse the shared grid parser so Heatmap honours the same
     * overflow guard and FASTCHART_MAX_GRID_CELLS cap as
     * SurfaceChart and ContourChart. The previous inline copy
     * skipped both checks, allowing a sparse PHP input to amplify
     * into a multi-hundred-MB native allocation. */
    fastchart_heatmap_obj *self = Z_FASTCHART_HEATMAP_OBJ_P(ZEND_THIS);
    fastchart_grid parsed = { NULL, 0, 0 };
    if (fastchart_parse_grid(grid, &parsed, "FastChart\\Heatmap::setGrid()") != 0) {
        RETURN_THROWS();
    }
    if (self->grid.cells) efree(self->grid.cells);
    self->grid = parsed;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Heatmap, setColorRamp)
{
    zend_long lo, hi;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(lo)
        Z_PARAM_LONG(hi)
    ZEND_PARSE_PARAMETERS_END();
    FASTCHART_VALIDATE_RGB(lo, "FastChart\\Heatmap::setColorRamp");
    FASTCHART_VALIDATE_RGB(hi, "FastChart\\Heatmap::setColorRamp");
    fastchart_heatmap_obj *self = Z_FASTCHART_HEATMAP_OBJ_P(ZEND_THIS);
    self->color_low_rgb  = (int)lo;
    self->color_high_rgb = (int)hi;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* setShowValues(bool, string) on the Chart base sets show_values
 * AND value_format together — Heatmap consumes both. */

ZEND_METHOD(FastChart_LinearMeter, setRange)
{
    double mn, mx;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_DOUBLE(mn)
        Z_PARAM_DOUBLE(mx)
    ZEND_PARSE_PARAMETERS_END();
    if (!isfinite(mn) || !isfinite(mx) || mx <= mn) {
        zend_value_error("FastChart\\LinearMeter::setRange() requires finite min < max");
        RETURN_THROWS();
    }
    fastchart_linear_meter_obj *self = Z_FASTCHART_LINEAR_METER_OBJ_P(ZEND_THIS);
    self->meter_min = mn;
    self->meter_max = mx;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_LinearMeter, setValue)
{
    double v;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_DOUBLE(v) ZEND_PARSE_PARAMETERS_END();
    if (!isfinite(v)) {
        zend_value_error("FastChart\\LinearMeter::setValue() requires a finite number");
        RETURN_THROWS();
    }
    fastchart_linear_meter_obj *self = Z_FASTCHART_LINEAR_METER_OBJ_P(ZEND_THIS);
    self->meter_value = v;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_LinearMeter, setOrientation)
{
    zend_long o;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_LONG(o) ZEND_PARSE_PARAMETERS_END();
    if (o != FASTCHART_METER_HORIZONTAL && o != FASTCHART_METER_VERTICAL) {
        zend_value_error("FastChart\\LinearMeter::setOrientation() expects METER_HORIZONTAL or METER_VERTICAL");
        RETURN_THROWS();
    }
    fastchart_linear_meter_obj *self = Z_FASTCHART_LINEAR_METER_OBJ_P(ZEND_THIS);
    self->meter_orientation = (int)o;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_LinearMeter, setZones)
{
    zval *zones;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(zones)
    ZEND_PARSE_PARAMETERS_END();

    fastchart_linear_meter_obj *self = Z_FASTCHART_LINEAR_METER_OBJ_P(ZEND_THIS);
    HashTable *ht = Z_ARRVAL_P(zones);
    int idx = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (idx >= FASTCHART_MAX_METER_ZONES) break;
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        HashTable *eht = Z_ARRVAL_P(entry);
        double from, to;
        zval *zfrom = zend_hash_str_find(eht, "from", sizeof("from") - 1);
        zval *zto   = zend_hash_str_find(eht, "to",   sizeof("to") - 1);
        if (!zfrom || !zto) continue;
        if (fastchart_zval_to_double(zfrom, &from) != 0 || !isfinite(from)) continue;
        if (fastchart_zval_to_double(zto, &to) != 0 || !isfinite(to)) continue;
        if (to <= from) continue;
        self->zones[idx].from = from;
        self->zones[idx].to = to;
        self->zones[idx].color_rgb = -1;
        zval *zc = zend_hash_str_find(eht, "color", sizeof("color") - 1);
        if (zc && Z_TYPE_P(zc) == IS_LONG) {
            zend_long c = Z_LVAL_P(zc);
            if (c >= 0 && c <= 0xFFFFFF) self->zones[idx].color_rgb = (int)c;
        }
        idx++;
    } ZEND_HASH_FOREACH_END();
    self->n_zones = idx;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_LinearMeter, setValueFormat)
{
    zend_string *fmt;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_STR(fmt) ZEND_PARSE_PARAMETERS_END();
    if (fastchart_validate_double_format(fmt, "LinearMeter::setValueFormat") != 0) {
        RETURN_THROWS();
    }
    fastchart_linear_meter_obj *self = Z_FASTCHART_LINEAR_METER_OBJ_P(ZEND_THIS);
    if (self->meter_value_format) zend_string_release(self->meter_value_format);
    /* Empty string is the documented "clear / use default" sentinel —
     * mirror Gauge::setValueFormat. Storing the empty literal would
     * produce blank labels at draw time. */
    self->meter_value_format = ZSTR_LEN(fmt) == 0 ? NULL : zend_string_copy(fmt);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

PHP_MINIT_FUNCTION(fastchart)
{
    /* Per-class handlers init. Each handlers struct gets its own
     * std offset matching its per-type struct layout, plus the
     * class's free / clone hooks. */
/* Use offsetof, not XtOffsetOf — the latter was removed in PHP 8.6
 * (`Zend/zend_portability.h`). offsetof works on every supported PHP
 * version. The clone macro at fastchart_clone_object_for already uses
 * offsetof; this site lagged behind. */
#define FASTCHART_INIT_HANDLERS(name, T) do {                                            \
        memcpy(&fastchart_##name##_handlers, &std_object_handlers,                       \
               sizeof(zend_object_handlers));                                            \
        fastchart_##name##_handlers.offset    = offsetof(T, std);                        \
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
FASTCHART_INIT_HANDLERS(treemap, fastchart_treemap_obj);
FASTCHART_INIT_HANDLERS(funnel,  fastchart_funnel_obj);
FASTCHART_INIT_HANDLERS(waterfall, fastchart_waterfall_obj);
FASTCHART_INIT_HANDLERS(heatmap, fastchart_heatmap_obj);
FASTCHART_INIT_HANDLERS(linear_meter, fastchart_linear_meter_obj);
    /* Symbol family handlers. Same pattern as the chart classes; the
     * lifecycle macro in fastchart_symbol.c emits the create / free /
     * clone trio with external linkage so this MINIT can wire them in. */
    FASTCHART_INIT_HANDLERS(code128, fastchart_code128_obj);
    FASTCHART_INIT_HANDLERS(qrcode,  fastchart_qrcode_obj);
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

    fastchart_treemap_ce       = register_class_FastChart_Treemap(fastchart_chart_ce);
    fastchart_treemap_ce->create_object = fastchart_treemap_create_object;

    fastchart_funnel_ce        = register_class_FastChart_Funnel(fastchart_chart_ce);
    fastchart_funnel_ce->create_object = fastchart_funnel_create_object;

    fastchart_waterfall_ce     = register_class_FastChart_Waterfall(fastchart_chart_ce);
    fastchart_waterfall_ce->create_object = fastchart_waterfall_create_object;

    fastchart_heatmap_ce       = register_class_FastChart_Heatmap(fastchart_chart_ce);
    fastchart_heatmap_ce->create_object = fastchart_heatmap_create_object;

    fastchart_linear_meter_ce  = register_class_FastChart_LinearMeter(fastchart_chart_ce);
    fastchart_linear_meter_ce->create_object = fastchart_linear_meter_create_object;

    /* Symbol family. Parallel hierarchy to Chart: Symbol (abstract)
     * → Barcode (abstract, 1D) and Symbol → QrCode (final, 2D).
     * Code128 sits below Barcode. ZEND_ACC_ABSTRACT blocks
     * `new FastChart\Symbol()` directly, but a userland subclass
     * (`class MySym extends FastChart\Symbol {}`) would inherit the
     * abstract base's create_object and bypass that check; without a
     * sentinel handler it'd allocate a vanilla zend_object that
     * cannot back the typed C struct, and any inherited setter would
     * read out-of-bounds. The sentinel throws on any such path; the
     * concrete internal classes override it with their own handler
     * immediately after registration, so the sentinel only fires on
     * unsupported userland subclassing. */
    fastchart_symbol_ce  = register_class_FastChart_Symbol();
    fastchart_symbol_ce->create_object  = fastchart_symbol_abstract_create_object;
    fastchart_barcode_ce = register_class_FastChart_Barcode(fastchart_symbol_ce);
    fastchart_barcode_ce->create_object = fastchart_symbol_abstract_create_object;
    fastchart_code128_ce = register_class_FastChart_Code128(fastchart_barcode_ce);
    fastchart_code128_ce->create_object = fastchart_code128_create_object;
    fastchart_qrcode_ce  = register_class_FastChart_QrCode(fastchart_symbol_ce);
    fastchart_qrcode_ce->create_object  = fastchart_qrcode_create_object;

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

/* Runtime MINIT order: gd must run first so its class table entry for
   GdImage and the php_gd_libgdimageptr_from_zval_p export are visible
   before fastchart resolves them. PHP_ADD_EXTENSION_DEP(fastchart, gd)
   in config.m4 only affects the build system; the engine ignores it at
   load time. ZEND_MOD_REQUIRED("gd") is what reorders the MINIT chain
   regardless of php.ini / conf.d / -d extension= load order. */
static const zend_module_dep fastchart_deps[] = {
    ZEND_MOD_REQUIRED("gd")
    ZEND_MOD_END
};

zend_module_entry fastchart_module_entry = {
    STANDARD_MODULE_HEADER_EX, NULL,
    fastchart_deps,
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
