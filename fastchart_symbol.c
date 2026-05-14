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

#include <stdio.h>
#include <string.h>

#include "php.h"
#include "php_streams.h"
#include "ext/standard/info.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_render_helpers.h"
#include "fastchart_target.h"
#include "fastchart_svg.h"
#include "qrcodegen.h"

/* fastchart_arginfo.h is intentionally NOT included here. It expands
 * `ZEND_BEGIN_ARG_INFO_EX` into `static const` argspec arrays and
 * a `static class_FastChart_*_methods` table; including it from a
 * second TU would fold multiple definitions at link time. The
 * argspec arrays and method tables are consumed only by MINIT in
 * fastchart.c; method bodies (this file) only need the
 * ZEND_METHOD macro from php.h. */

/* Initial defaults applied by every Symbol create_object handler before
 * the per-class init_extras runs. Mirrors fastchart_base_init_defaults
 * for the chart family but covers only the slim Symbol base. */
void fastchart_symbol_base_init_defaults(fastchart_symbol_obj *b)
{
    b->width = 0;            /* 0 = pick class default at render time */
    b->height = 0;
    b->dpi = 96;
    b->data = NULL;
    b->fg_rgb = 0x000000;
    b->bg_rgb = 0xFFFFFF;
    b->transparent_bg = false;
    b->quiet_zone = -1;      /* -1 = pick class default at render time */
}

void fastchart_symbol_base_release_owned(fastchart_symbol_obj *b)
{
    if (b->data) zend_string_release(b->data);
}

void fastchart_symbol_base_addref_owned(fastchart_symbol_obj *b)
{
    if (b->data) zend_string_addref(b->data);
}

/* Per-class init/release/addref helpers below mirror the chart-family
 * pattern. Each Symbol concrete class defines its own trio so the
 * lifecycle macro can call them by `name`. */
/* Per-class init hook. Sets class-specific defaults that aren't part
 * of FASTCHART_SYMBOL_BASE_FIELDS. Release / addref hooks were
 * removed when neither Code128 nor QrCode kept per-instance owned
 * state of their own — base data + the class-specific scalars are
 * lifecycle-safe under memcpy. Add a class-local release/addref
 * pair back into the macro below if a future symbology stores an
 * owned zend_string (font path, glyph cache, etc.). */
static void fastchart_code128_init_extras(fastchart_code128_obj *o)
{
    o->show_text = false;
}

static void fastchart_qrcode_init_extras(fastchart_qrcode_obj *o)
{
    o->ecc = FASTCHART_QR_ECC_M;
    o->min_version = qrcodegen_VERSION_MIN;
    o->max_version = qrcodegen_VERSION_MAX;
}

/* Generates the create / free / clone trio for one Symbol class.
 * Mirrors FASTCHART_DEFINE_LIFECYCLE in fastchart.c, swapping the
 * chart-base helpers for the symbol-base equivalents. The macro emits
 * external (non-static) symbols since MINIT lives in fastchart.c;
 * forward declarations land in php_fastchart.h. The handlers struct
 * must already be defined in this translation unit; MINIT memcpy's
 * std_object_handlers into it and sets offset / dtor / clone. */
#define FASTCHART_DEFINE_SYMBOL_LIFECYCLE(name, T)                               \
    zend_object *fastchart_##name##_create_object(zend_class_entry *ce)          \
    {                                                                            \
        T *intern = zend_object_alloc(sizeof(T), ce);                            \
        zend_object_std_init(&intern->std, ce);                                  \
        object_properties_init(&intern->std, ce);                                \
        fastchart_symbol_base_init_defaults((fastchart_symbol_obj *)intern);     \
        fastchart_##name##_init_extras(intern);                                  \
        intern->std.handlers = &fastchart_##name##_handlers;                     \
        return &intern->std;                                                     \
    }                                                                            \
    void fastchart_##name##_free_object(zend_object *object)                     \
    {                                                                            \
        T *intern = (T *)((char *)object - fastchart_##name##_handlers.offset);  \
        fastchart_symbol_base_release_owned((fastchart_symbol_obj *)intern);     \
        zend_object_std_dtor(&intern->std);                                      \
    }                                                                            \
    zend_object *fastchart_##name##_clone_object(zend_object *src_obj)           \
    {                                                                            \
        T *src = (T *)((char *)src_obj - fastchart_##name##_handlers.offset);    \
        zend_object *dst_obj = fastchart_##name##_create_object(src_obj->ce);    \
        T *dst = (T *)((char *)dst_obj - fastchart_##name##_handlers.offset);    \
        fastchart_symbol_base_release_owned((fastchart_symbol_obj *)dst);        \
        memcpy(dst, src, offsetof(T, std));                                      \
        fastchart_symbol_base_addref_owned((fastchart_symbol_obj *)dst);         \
        zend_objects_clone_members(&dst->std, &src->std);                        \
        return &dst->std;                                                        \
    }

/* Handler structs are populated by MINIT in fastchart.c; declared here
 * with external linkage so the lifecycle macro and MINIT can both
 * reference the same storage. */
zend_object_handlers fastchart_code128_handlers;
zend_object_handlers fastchart_qrcode_handlers;

FASTCHART_DEFINE_SYMBOL_LIFECYCLE(code128, fastchart_code128_obj)
FASTCHART_DEFINE_SYMBOL_LIFECYCLE(qrcode,  fastchart_qrcode_obj)

/* create_object handler attached at MINIT to the abstract Symbol and
 * Barcode class entries. ZEND_ACC_ABSTRACT blocks `new Symbol()` and
 * `new Barcode()` directly, but a userland subclass
 * (`class MySym extends FastChart\Symbol {}`) inherits the parent's
 * create_object and bypasses the abstract check. Without this guard,
 * `new MySym()` would allocate a vanilla zend_object whose memory
 * layout cannot back fastchart_symbol_obj — every inherited typed
 * method (setData / setSize / renderPng) would walk past the end of
 * the std header and crash under ASan. Throw cleanly instead.
 *
 * Concrete internal subclasses (Code128, QrCode) override create_object
 * with their own handler at MINIT, so this trampoline only fires on
 * unsupported userland subclassing paths. */
zend_object *fastchart_symbol_abstract_create_object(zend_class_entry *ce)
{
    zend_throw_error(NULL,
        "FastChart\\%s is internal and cannot be instantiated or subclassed; "
        "use a concrete class such as FastChart\\Code128 or FastChart\\QrCode.",
        ZSTR_VAL(ce->name));
    /* Returning NULL from create_object isn't a recognised contract, so
     * fall back to allocating a minimal zend_object the engine can
     * destroy normally. The thrown error short-circuits any caller
     * that would try to use the object before destruction. */
    zend_object *obj = zend_objects_new(ce);
    obj->handlers = &std_object_handlers;
    return obj;
}

/* Fill the canvas with the configured background, honouring
 * `transparent_bg`. Backend-aware: GD uses the alpha-blending dance
 * below; SVG emits a single bg rect (or nothing on transparent_bg —
 * SVG transparency is implicit, no element means see-through).
 *
 * GD path libgd quirk: `gdImageColorTransparent()` does NOT rewrite
 * existing pixel alpha — it only flags an index for legacy
 * palette-image transparency. On a truecolor canvas the encoder
 * writes per-pixel alpha when `gdImageSaveAlpha(im, 1)` is set, so
 * the bg pixels themselves must carry alpha=127 for the encoded
 * PNG/WebP/AVIF output to be transparent.
 *
 * Sequence (GD only):
 *   - alphaBlending(0) so the fill REPLACES the canvas instead of
 *     compositing over the engine-default opaque-black initial state
 *     (which would blend a partially-transparent fill into something
 *     darker than the user asked for).
 *   - saveAlpha(1) so the encoder writes the alpha channel for
 *     formats that carry it. Encoders for JPEG / GIF discard alpha
 *     by design; those callers see the bg colour blended against
 *     itself, which preserves the configured RGB.
 *   - fill with gdImageColorAllocateAlpha(..., 127) on transparent_bg,
 *     opaque allocate otherwise.
 *   - alphaBlending(1) restored so subsequent foreground draws
 *     composite normally (fg with alpha=0 = opaque overwrites bg). */
void fastchart_symbol_fill_background(fastchart_symbol_obj *self,
                                      fastchart_target_t *t)
{
    int r = (int)((self->bg_rgb >> 16) & 0xFF);
    int g = (int)((self->bg_rgb >> 8) & 0xFF);
    int b = (int)(self->bg_rgb & 0xFF);

    if (t->kind == FASTCHART_TARGET_GD) {
        gdImagePtr im = t->u.gd.im;
        int W = gdImageSX(im);
        int H = gdImageSY(im);

        gdImageAlphaBlending(im, 0);
        if (self->transparent_bg) {
            gdImageSaveAlpha(im, 1);
            int bg = gdImageColorAllocateAlpha(im, r, g, b, 127);
            gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, bg);
        } else {
            gdImageSaveAlpha(im, 0);
            int bg = gdImageColorAllocate(im, r, g, b);
            gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, bg);
        }
        gdImageAlphaBlending(im, 1);
        return;
    }

    /* SVG. transparent_bg = no bg element (SVG default is transparent
     * outside drawn elements). Opaque bg = single full-canvas rect. */
    if (self->transparent_bg) return;
    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    int bg = fastchart_target_color(t, r, g, b, 0xFF);
    fastchart_target_rect(t, 0, 0, W, H, bg, /*fill=*/1, /*thickness=*/0);
}

/* Code128 renderer lives in fastchart_code128.c; QrCode renderer
 * lives in fastchart_qrcode.c. */

/* ---------------- Symbol render dispatch + shortcuts -------------- */

/* Resolve the logical (post-default) canvas size for a Symbol
 * instance. Width / height of 0 means "user did not call setSize()" —
 * substitute the per-class default (300x80 for Code128, 300x300 for
 * QrCode). Anything > 0 is taken at face value; the cap policy in
 * fastchart_resolve_canvas_dims handles the upper bound after DPI
 * scaling. */
static void fastchart_symbol_logical_dims(fastchart_symbol_obj *self,
                                          zend_class_entry *ce,
                                          zend_long *out_w, zend_long *out_h)
{
    zend_long w = self->width;
    zend_long h = self->height;
    if (w <= 0 || h <= 0) {
        if (ce == fastchart_code128_ce) {
            if (w <= 0) w = FASTCHART_CODE128_DEFAULT_W;
            if (h <= 0) h = FASTCHART_CODE128_DEFAULT_H;
        } else if (ce == fastchart_qrcode_ce) {
            if (w <= 0) w = FASTCHART_QRCODE_DEFAULT_W;
            if (h <= 0) h = FASTCHART_QRCODE_DEFAULT_H;
        }
    }
    *out_w = w;
    *out_h = h;
}

static int dispatch_symbol_render(fastchart_symbol_obj *self,
                                  zend_class_entry *ce, gdImagePtr im)
{
    if (ce == fastchart_code128_ce)
        return fastchart_code128_render_to_image((fastchart_code128_obj *)self, im);
    if (ce == fastchart_qrcode_ce)
        return fastchart_qrcode_render_to_image((fastchart_qrcode_obj *)self, im);
    zend_throw_error(NULL, "FastChart\\Symbol: render dispatch found unknown class entry");
    return -1;
}

/* SVG-side dispatcher. Parallel to dispatch_symbol_render but routes
 * to the target-based renderer so the SVG backend receives vector
 * emissions instead of gdImage* primitive calls. */
static int dispatch_symbol_svg_render(fastchart_symbol_obj *self,
                                       zend_class_entry *ce,
                                       fastchart_target_t *t)
{
    if (ce == fastchart_code128_ce)
        return fastchart_code128_render_to_target((fastchart_code128_obj *)self, t);
    if (ce == fastchart_qrcode_ce)
        return fastchart_qrcode_render_to_target((fastchart_qrcode_obj *)self, t);
    zend_throw_error(NULL,
        "FastChart\\Symbol: SVG dispatch found unknown class entry");
    return -1;
}

static void fastchart_symbol_render_to_string(INTERNAL_FUNCTION_PARAMETERS,
                                              int format, zend_long quality)
{
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    zend_class_entry *ce = Z_OBJCE_P(ZEND_THIS);

    if (!self->data || ZSTR_LEN(self->data) == 0) {
        zend_throw_error(NULL,
            "FastChart\\Symbol: setData() is required before render");
        RETURN_THROWS();
    }

    zend_long lw, lh;
    fastchart_symbol_logical_dims(self, ce, &lw, &lh);

    int alloc_w, alloc_h;
    if (fastchart_resolve_canvas_dims(lw, lh, self->dpi, &alloc_w, &alloc_h) != 0) {
        RETURN_THROWS();
    }
    gdImagePtr im = gdImageCreateTrueColor(alloc_w, alloc_h);
    if (!im) {
        zend_throw_error(NULL, "FastChart: gdImageCreateTrueColor() returned NULL");
        RETURN_THROWS();
    }
    /* Stamp the canvas resolution so the encoded PNG/JPEG metadata
     * reports the configured DPI. Without this, libgd writes a default
     * 96 DPI regardless of `setDpi()`, and retina viewers / print
     * pipelines render at the wrong physical size. Mirrors the chart-
     * family wiring at fastchart_axis.c. */
    if (self->dpi > 0) {
        gdImageSetResolution(im, (unsigned int)self->dpi, (unsigned int)self->dpi);
    }

    if (dispatch_symbol_render(self, ce, im) != 0 || EG(exception)) {
        gdImageDestroy(im);
        RETURN_THROWS();
    }

    /* No post-render alpha bookkeeping here: transparent_bg is honoured
     * inside the renderer (fastchart_symbol_fill_background) by
     * allocating the bg with alpha=127 and setting gdImageSaveAlpha,
     * which is what actually makes PNG/WebP/AVIF output transparent.
     * gdImageColorTransparent() on a truecolor canvas does not rewrite
     * pixel alpha and would not transparentize the output. */

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

/* ---------------- Symbol SVG render ------------------------------- */

/* Case-insensitive ASCII check for a `.svg` tail. Same parsing shape
 * as fastchart_format_from_path so a path like "weird.SVG/here.png"
 * reports .png and "report.SVG" reports .svg. Duplicated from
 * fastchart.c (file-static there) — tiny helper, not worth exposing. */
static int fc_symbol_path_ends_with_svg(const char *path, size_t len)
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

/* Shared SVG entry. fragment_only=0 emits a full document; =1 emits
 * just the <g class="fastchart-symbol"> group. Output viewport = the
 * Symbol's logical canvas size (default-substituted per class) at 1:1
 * — DPI does not multiply the viewport (SVG is vector). Mirrors
 * fastchart.c:fastchart_render_to_svg for the Chart family. */
static void fastchart_symbol_render_to_svg(INTERNAL_FUNCTION_PARAMETERS,
                                            int fragment_only)
{
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    zend_class_entry *ce = Z_OBJCE_P(ZEND_THIS);

    if (!self->data || ZSTR_LEN(self->data) == 0) {
        zend_throw_error(NULL,
            "FastChart\\Symbol: setData() is required before render");
        RETURN_THROWS();
    }

    zend_long lw, lh;
    fastchart_symbol_logical_dims(self, ce, &lw, &lh);
    if (lw <= 0 || lh <= 0 || lw > 65535 || lh > 65535) {
        zend_throw_error(NULL,
            "FastChart\\Symbol: invalid canvas size for SVG render");
        RETURN_THROWS();
    }

    smart_str buf = {0};
    if (!fragment_only) {
        fc_svg_emit_doc_open(&buf, (int)lw, (int)lh);
    }
    fc_svg_emit_g_open(&buf, "fastchart-symbol");

    fastchart_target_t t;
    fastchart_target_from_svg(&t, &buf, (int)lw, (int)lh, (int)self->dpi);

    if (dispatch_symbol_svg_render(self, ce, &t) != 0 || EG(exception)) {
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
    RETURN_STR(buf.s);
}

/* SVG file-write branch invoked by Symbol::renderToFile when the path
 * extension is .svg. Mirrors fastchart_render_to_svg_file in
 * fastchart.c. Honors open_basedir, writes via php_stream_open_wrapper,
 * surfaces short writes as a thrown error. */
static void fastchart_symbol_render_to_svg_file(INTERNAL_FUNCTION_PARAMETERS,
                                                 zend_string *path)
{
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    zend_class_entry *ce = Z_OBJCE_P(ZEND_THIS);

    if (!self->data || ZSTR_LEN(self->data) == 0) {
        zend_throw_error(NULL,
            "FastChart\\Symbol: setData() is required before render");
        RETURN_THROWS();
    }

    zend_long lw, lh;
    fastchart_symbol_logical_dims(self, ce, &lw, &lh);
    if (lw <= 0 || lh <= 0 || lw > 65535 || lh > 65535) {
        zend_throw_error(NULL,
            "FastChart\\Symbol: invalid canvas size for SVG render");
        RETURN_THROWS();
    }

    smart_str buf = {0};
    fc_svg_emit_doc_open(&buf, (int)lw, (int)lh);
    fc_svg_emit_g_open(&buf, "fastchart-symbol");

    fastchart_target_t t;
    fastchart_target_from_svg(&t, &buf, (int)lw, (int)lh, (int)self->dpi);

    if (dispatch_symbol_svg_render(self, ce, &t) != 0 || EG(exception)) {
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
                "FastChart\\Symbol::renderToFile() could not open %s for writing",
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

/* ---------------- Symbol setters ---------------------------------- */

ZEND_METHOD(FastChart_Symbol, setSize)
{
    zend_long w, h;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(w)
        Z_PARAM_LONG(h)
    ZEND_PARSE_PARAMETERS_END();
    /* setSize(0, *) was once a sentinel for "use class default" but
     * the default is conveyed by NOT calling setSize at all (the
     * struct field stays 0). Reject 0 so a typo doesn't silently
     * fall back to the default. Upper bound matches Chart::setSize. */
    if (w <= 0 || h <= 0 || w > 65535 || h > 65535) {
        zend_value_error(
            "FastChart\\Symbol::setSize() width and height must be in [1, 65535]");
        RETURN_THROWS();
    }
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    self->width = w;
    self->height = h;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Symbol, setData)
{
    zend_string *data;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(data)
    ZEND_PARSE_PARAMETERS_END();
    if (ZSTR_LEN(data) == 0) {
        zend_value_error("FastChart\\Symbol::setData() data must not be empty");
        RETURN_THROWS();
    }
    if (memchr(ZSTR_VAL(data), 0, ZSTR_LEN(data)) != NULL) {
        zend_value_error(
            "FastChart\\Symbol::setData() data must not contain NUL bytes");
        RETURN_THROWS();
    }
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    if (self->data) zend_string_release(self->data);
    self->data = zend_string_copy(data);
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Symbol, setQuietZone)
{
    zend_long units;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(units)
    ZEND_PARSE_PARAMETERS_END();
    /* -1 sentinels "use class default"; anything below that is a
     * typo. 4096 is a generous ceiling that still leaves room inside
     * the canvas-dim cap on a 16384px render. */
    if (units < -1 || units > 4096) {
        zend_value_error(
            "FastChart\\Symbol::setQuietZone() units must be in [-1, 4096]");
        RETURN_THROWS();
    }
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    self->quiet_zone = units;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

static void fastchart_symbol_set_color(INTERNAL_FUNCTION_PARAMETERS,
                                       bool is_foreground)
{
    zend_long rgb;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(rgb)
    ZEND_PARSE_PARAMETERS_END();
    if (rgb < 0 || rgb > 0xFFFFFF) {
        zend_value_error(
            "FastChart\\Symbol: color must be a 24-bit RGB value in [0, 0xFFFFFF]");
        RETURN_THROWS();
    }
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    if (is_foreground) self->fg_rgb = rgb;
    else                self->bg_rgb = rgb;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Symbol, setForeground)
{
    fastchart_symbol_set_color(INTERNAL_FUNCTION_PARAM_PASSTHRU, true);
}

ZEND_METHOD(FastChart_Symbol, setBackground)
{
    fastchart_symbol_set_color(INTERNAL_FUNCTION_PARAM_PASSTHRU, false);
}

ZEND_METHOD(FastChart_Symbol, setTransparentBackground)
{
    bool enabled;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(enabled)
    ZEND_PARSE_PARAMETERS_END();
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    self->transparent_bg = enabled;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_Symbol, setDpi)
{
    zend_long dpi;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(dpi)
    ZEND_PARSE_PARAMETERS_END();
    /* Same range as Chart::setDpi (validated there); duplicated here
     * because Symbol's range is identical and the resolver in
     * fastchart_render_helpers.c relies on values within this band. */
    if (dpi < 24 || dpi > 1200) {
        zend_value_error(
            "FastChart\\Symbol::setDpi() dpi must be in [24, 1200]");
        RETURN_THROWS();
    }
    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    self->dpi = dpi;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* ---------------- Symbol render shortcuts ------------------------- */

ZEND_METHOD(FastChart_Symbol, renderPng)
{
    ZEND_PARSE_PARAMETERS_NONE();
    fastchart_symbol_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, 0);
}

ZEND_METHOD(FastChart_Symbol, renderJpeg)
{
    zend_long quality = 90;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();
    if (quality < 1 || quality > 100) {
        zend_value_error("FastChart\\Symbol::renderJpeg() quality must be in [1, 100]");
        RETURN_THROWS();
    }
    fastchart_symbol_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1, quality);
}

ZEND_METHOD(FastChart_Symbol, renderWebp)
{
    zend_long quality = 90;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();
    if (quality < 0 || quality > 100) {
        zend_value_error("FastChart\\Symbol::renderWebp() quality must be in [0, 100]");
        RETURN_THROWS();
    }
    fastchart_symbol_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2, quality);
}

ZEND_METHOD(FastChart_Symbol, renderGif)
{
    ZEND_PARSE_PARAMETERS_NONE();
    fastchart_symbol_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 3, 0);
}

ZEND_METHOD(FastChart_Symbol, renderAvif)
{
    zend_long quality = 60;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();
    if (quality < 0 || quality > 100) {
        zend_value_error("FastChart\\Symbol::renderAvif() quality must be in [0, 100]");
        RETURN_THROWS();
    }
    fastchart_symbol_render_to_string(INTERNAL_FUNCTION_PARAM_PASSTHRU, 4, quality);
}

ZEND_METHOD(FastChart_Symbol, renderSvg)
{
    ZEND_PARSE_PARAMETERS_NONE();
    fastchart_symbol_render_to_svg(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

ZEND_METHOD(FastChart_Symbol, drawSvgFragment)
{
    ZEND_PARSE_PARAMETERS_NONE();
    fastchart_symbol_render_to_svg(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

ZEND_METHOD(FastChart_Symbol, renderToFile)
{
    zend_string *path;
    zend_long quality = 90;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_PATH_STR(path)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(quality)
    ZEND_PARSE_PARAMETERS_END();

    /* Vector branch. .svg ignores $quality (no lossy encoder) and
     * goes through a separate write path that emits text bytes.
     * Mirrors the Chart-side .svg routing in fastchart.c. */
    if (fc_symbol_path_ends_with_svg(ZSTR_VAL(path), ZSTR_LEN(path))) {
        (void)quality;
        if (php_check_open_basedir(ZSTR_VAL(path))) {
            if (!EG(exception)) {
                zend_throw_error(NULL,
                    "FastChart\\Symbol::renderToFile() open_basedir restriction "
                    "prevents access to %s", ZSTR_VAL(path));
            }
            RETURN_THROWS();
        }
        fastchart_symbol_render_to_svg_file(INTERNAL_FUNCTION_PARAM_PASSTHRU, path);
        return;
    }

    int format = fastchart_format_from_path(ZSTR_VAL(path), ZSTR_LEN(path));
    if (format < 0) {
        zend_value_error(
            "FastChart\\Symbol::renderToFile() could not infer format from extension; "
            "expected .png/.jpg/.jpeg/.webp/.gif/.avif/.svg");
        RETURN_THROWS();
    }
    /* Format-conditional quality validation: same policy as
     * Chart::renderToFile. JPEG rejects 0 (degenerate); WebP / AVIF
     * accept it as "smallest, lowest fidelity"; PNG / GIF ignore it
     * but still range-check. */
    if (format == 1) {  /* JPEG */
        if (quality < 1 || quality > 100) {
            zend_value_error(
                "FastChart\\Symbol::renderToFile() JPEG quality must be in [1, 100]");
            RETURN_THROWS();
        }
    } else {
        if (quality < 0 || quality > 100) {
            zend_value_error(
                "FastChart\\Symbol::renderToFile() quality must be in [0, 100]");
            RETURN_THROWS();
        }
    }
    if (php_check_open_basedir(ZSTR_VAL(path))) {
        /* php_check_open_basedir emits E_WARNING but does not set
         * EG(exception); RETURN_THROWS's ZEND_ASSERT requires it
         * under debug builds. Throw an explicit Error so the caller
         * gets a structured exception, not a debug-build abort. */
        if (!EG(exception)) {
            zend_throw_error(NULL,
                "FastChart\\Symbol::renderToFile() open_basedir restriction "
                "prevents access to %s", ZSTR_VAL(path));
        }
        RETURN_THROWS();
    }

    fastchart_symbol_obj *self = Z_FASTCHART_SYMBOL_OBJ_P(ZEND_THIS);
    zend_class_entry *ce = Z_OBJCE_P(ZEND_THIS);
    if (!self->data || ZSTR_LEN(self->data) == 0) {
        zend_throw_error(NULL,
            "FastChart\\Symbol: setData() is required before render");
        RETURN_THROWS();
    }

    zend_long lw, lh;
    fastchart_symbol_logical_dims(self, ce, &lw, &lh);

    int alloc_w, alloc_h;
    if (fastchart_resolve_canvas_dims(lw, lh, self->dpi, &alloc_w, &alloc_h) != 0) {
        RETURN_THROWS();
    }
    gdImagePtr im = gdImageCreateTrueColor(alloc_w, alloc_h);
    if (!im) {
        zend_throw_error(NULL, "FastChart: gdImageCreateTrueColor() returned NULL");
        RETURN_THROWS();
    }
    /* Stamp the canvas resolution so the encoded PNG/JPEG metadata
     * reports the configured DPI. Without this, libgd writes a default
     * 96 DPI regardless of `setDpi()`, and retina viewers / print
     * pipelines render at the wrong physical size. Mirrors the chart-
     * family wiring at fastchart_axis.c. */
    if (self->dpi > 0) {
        gdImageSetResolution(im, (unsigned int)self->dpi, (unsigned int)self->dpi);
    }

    if (dispatch_symbol_render(self, ce, im) != 0 || EG(exception)) {
        gdImageDestroy(im);
        RETURN_THROWS();
    }

    /* transparent_bg honoured inside the renderer; see comment in
     * fastchart_symbol_render_to_string above. */

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
        /* php_stream_open_wrapper with REPORT_ERRORS emits E_WARNING
         * on failure but does not set EG(exception). Throw explicitly
         * so RETURN_THROWS does not assert under debug builds. */
        if (!EG(exception)) {
            zend_throw_error(NULL,
                "FastChart\\Symbol::renderToFile() could not open %s for writing",
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
    if ((size_t)written != (size_t)sz) {
        zend_throw_error(NULL,
            "FastChart: short write to %s (%zd of %d bytes)",
            ZSTR_VAL(path), written, sz);
        RETURN_THROWS();
    }
    RETURN_LONG((zend_long)written);
}

/* ---------------- Code128 setters --------------------------------- */

ZEND_METHOD(FastChart_Code128, setShowText)
{
    bool enabled;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(enabled)
    ZEND_PARSE_PARAMETERS_END();
    /* Lives on Code128 directly (not Barcode) so the cast is
     * type-safe at the leaf. Future Barcode subclasses (Code39,
     * EAN, ...) declare their own text-rendering setters with the
     * right semantics for that symbology. */
    fastchart_code128_obj *self = Z_FASTCHART_CODE128_OBJ_P(ZEND_THIS);
    self->show_text = enabled;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

/* ---------------- QrCode setters ---------------------------------- */

ZEND_METHOD(FastChart_QrCode, setEcc)
{
    zend_long level;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(level)
    ZEND_PARSE_PARAMETERS_END();
    if (level < FASTCHART_QR_ECC_L || level > FASTCHART_QR_ECC_H) {
        zend_value_error(
            "FastChart\\QrCode::setEcc() level must be one of "
            "QrCode::ECC_L / ECC_M / ECC_Q / ECC_H");
        RETURN_THROWS();
    }
    fastchart_qrcode_obj *self = Z_FASTCHART_QRCODE_OBJ_P(ZEND_THIS);
    self->ecc = level;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_QrCode, setMinVersion)
{
    zend_long v;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(v)
    ZEND_PARSE_PARAMETERS_END();
    if (v < qrcodegen_VERSION_MIN || v > qrcodegen_VERSION_MAX) {
        zend_value_error(
            "FastChart\\QrCode::setMinVersion() version must be in [%d, %d]",
            qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX);
        RETURN_THROWS();
    }
    fastchart_qrcode_obj *self = Z_FASTCHART_QRCODE_OBJ_P(ZEND_THIS);
    self->min_version = v;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}

ZEND_METHOD(FastChart_QrCode, setMaxVersion)
{
    zend_long v;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(v)
    ZEND_PARSE_PARAMETERS_END();
    if (v < qrcodegen_VERSION_MIN || v > qrcodegen_VERSION_MAX) {
        zend_value_error(
            "FastChart\\QrCode::setMaxVersion() version must be in [%d, %d]",
            qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX);
        RETURN_THROWS();
    }
    fastchart_qrcode_obj *self = Z_FASTCHART_QRCODE_OBJ_P(ZEND_THIS);
    self->max_version = v;
    RETURN_ZVAL(ZEND_THIS, 1, 0);
}
