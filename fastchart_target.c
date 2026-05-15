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
#include "Zend/zend_smart_str.h"
#include "Zend/zend_exceptions.h"
#include "ext/standard/base64.h"
#include "main/php_streams.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "fastchart_target.h"
#include "fastchart_svg.h"
#include "fastchart_rasterize.h"
#include "php_fastchart.h"

/* Source-image bytes cap. FC_IMAGE_MAX_DIM / FC_IMAGE_MAX_PIXELS
 * are shared with the SVG rasterizer via fastchart_rasterize.h. */
#define FC_IMAGE_MAX_BYTES   (8 * 1024 * 1024)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================ *
 * Init                                                          *
 * ============================================================ */

void fastchart_target_from_svg(fastchart_target_t *t, smart_str *buf,
                                int width, int height, int dpi,
                                int text_mode)
{
    memset(t, 0, sizeof(*t));
    t->kind = FASTCHART_TARGET_SVG;
    t->u.svg.buf = buf;
    t->u.svg.width = width;
    t->u.svg.height = height;
    /* SVG output is DPI-invariant: vectors scale infinitely, so DPI
     * has no effect on the output viewport, and we deliberately keep
     * layout / text measurement at the 96 baseline so an SVG render
     * with setDpi(200) is identical to one with setDpi(96). Honoring
     * the chart's DPI here would inflate label-reserved margins and
     * make text-measurement reserve room for 2x glyphs that the SVG
     * still emits at 1x — producing the "huge left margin" symptom.
     * The `dpi` parameter is accepted for signature stability but is
     * intentionally ignored. */
    (void)dpi;
    t->u.svg.dpi = 96;
    t->u.svg.next_clip_id = 1;
    t->u.svg.next_grad_id = 1;
    t->u.svg.text_mode = (text_mode == FASTCHART_SVG_TEXT_NATIVE)
        ? FASTCHART_SVG_TEXT_NATIVE
        : FASTCHART_SVG_TEXT_PATHS;
}

/* ============================================================ *
 * Color                                                         *
 * ============================================================ */

int fastchart_target_color(fastchart_target_t *t, int r, int g, int b, int a)
{
    /* Clamp 0..255. */
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    if (a < 0) a = 0; else if (a > 255) a = 255;

    uint32_t key = ((uint32_t)a << 24) | ((uint32_t)r << 16)
                 | ((uint32_t)g << 8)  |  (uint32_t)b;

    for (int i = 0; i < t->n_colors; i++) {
        if (t->color_rgba[i] == key) return i;
    }
    if (t->n_colors >= FASTCHART_TARGET_MAX_COLORS) {
        return t->n_colors - 1;  /* defensive: reuse last slot */
    }
    int idx = t->n_colors++;
    t->color_rgba[idx] = key;
    return idx;
}

int fastchart_target_color_rgb(fastchart_target_t *t, int rgb)
{
    int r = (rgb >> 16) & 0xFF;
    int g = (rgb >>  8) & 0xFF;
    int b =  rgb        & 0xFF;
    return fastchart_target_color(t, r, g, b, 0xFF);
}

uint32_t fastchart_target_color_to_rgba(fastchart_target_t *t, int handle)
{
    if (handle < 0 || handle >= t->n_colors) return 0xFF000000u;
    return t->color_rgba[handle];
}

/* ============================================================ *
 * Dimensions                                                    *
 * ============================================================ */

void fastchart_target_get_dims(fastchart_target_t *t, int *w, int *h)
{
    *w = t->u.svg.width;
    *h = t->u.svg.height;
}

int fastchart_target_get_dpi(fastchart_target_t *t)
{
    return t->u.svg.dpi;
}

/* ============================================================ *
 * Primitives                                                    *
 * ============================================================ */

void fastchart_target_line(fastchart_target_t *t,
                            int x0, int y0, int x1, int y1,
                            int color, int thickness, int dash)
{
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_line(t->u.svg.buf, x0, y0, x1, y1, rgba, thickness, dash);
}

void fastchart_target_rect(fastchart_target_t *t,
                            int x, int y, int w, int h,
                            int color, int fill, int thickness)
{
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_rect(t->u.svg.buf, x, y, w, h, rgba, fill, thickness);
}

void fastchart_target_polygon(fastchart_target_t *t,
                               const fastchart_point_t *pts, int n,
                               int color, int fill, int thickness)
{
    if (n < 2) return;
    /* SVG path needs separate int arrays. Stack-buffer up to 256
     * points, else heap. Charts rarely exceed ~64 points in a polygon
     * (markers are 3-8; area fills can be larger but still bounded). */
    int xs_stack[256], ys_stack[256];
    int *xs = xs_stack, *ys = ys_stack;
    if (n > 256) {
        xs = emalloc(sizeof(int) * (size_t)n);
        ys = emalloc(sizeof(int) * (size_t)n);
    }
    for (int i = 0; i < n; i++) {
        xs[i] = pts[i].x;
        ys[i] = pts[i].y;
    }
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_polygon(t->u.svg.buf, xs, ys, n, rgba, fill, thickness);
    if (n > 256) { efree(xs); efree(ys); }
}

void fastchart_target_arc(fastchart_target_t *t,
                           int cx, int cy, int rx, int ry,
                           double start_deg, double end_deg,
                           int color, int fill, int thickness)
{
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_path_arc(t->u.svg.buf, cx, cy, rx, ry,
                          start_deg, end_deg, rgba, fill, thickness);
}

void fastchart_target_ellipse(fastchart_target_t *t,
                               int cx, int cy, int rx, int ry,
                               int color, int fill, int thickness)
{
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_ellipse(t->u.svg.buf, cx, cy, rx, ry, rgba, fill, thickness);
}

/* ============================================================ *
 * Font family resolution via FreeType                           *
 * ============================================================ */

/* FT state lives in TSRM module globals (see ZEND_BEGIN_MODULE_GLOBALS
 * in php_fastchart.h). Under NTS this is one struct shared process-
 * wide; under ZTS each thread gets its own copy via TSRM. No mutex
 * needed: FreeType operations on different FT_Library handles are
 * independent, and the face cache lives inside each thread's
 * globals so no two threads ever share an FT_Face.
 *
 * The "4-slot LRU" sizing matches the typical mixed-font dashboard:
 * one chart font, one symbol font, two for axis-label/title splits.
 * Linear scan + swap-to-front on hit keeps the hot path at slot 0. */

FT_Library fastchart_ft_library(void)
{
    if (FASTCHART_G(ft_lib) != NULL)     return FASTCHART_G(ft_lib);
    if (FASTCHART_G(ft_lib_init_failed)) return NULL;
    FT_Library lib = NULL;
    if (FT_Init_FreeType(&lib) != 0) {
        FASTCHART_G(ft_lib) = NULL;
        FASTCHART_G(ft_lib_init_failed) = 1;
        return NULL;
    }
    FASTCHART_G(ft_lib) = lib;
    return lib;
}

FT_Face fastchart_ft_face(const char *font_path)
{
    if (!font_path) return NULL;
    FT_Library lib = fastchart_ft_library();
    if (!lib) return NULL;

    fc_ft_face_slot *cache = FASTCHART_G(ft_face_cache);

    /* Hit? Swap-to-front to keep the hot path at slot 0. */
    for (int i = 0; i < FC_FT_FACE_CACHE_N; i++) {
        if (cache[i].path && strcmp(cache[i].path, font_path) == 0) {
            if (i != 0) {
                fc_ft_face_slot tmp = cache[0];
                cache[0] = cache[i];
                cache[i] = tmp;
            }
            return cache[0].face;
        }
    }

    /* Miss. Build the new entry (malloc + FT_New_Face) BEFORE
     * evicting anything so a failure mid-build leaves the cache
     * untouched. */
    size_t path_len = strlen(font_path);
    char  *path_copy = malloc(path_len + 1);
    if (!path_copy) return NULL;
    memcpy(path_copy, font_path, path_len + 1);

    FT_Face face = NULL;
    if (FT_New_Face(lib, font_path, 0, &face)) {
        free(path_copy);
        return NULL;
    }

    /* Evict the tail slot, shift right, install at slot 0. */
    int tail = FC_FT_FACE_CACHE_N - 1;
    if (cache[tail].face) FT_Done_Face(cache[tail].face);
    if (cache[tail].path) free(cache[tail].path);
    for (int i = tail; i > 0; i--) {
        cache[i] = cache[i - 1];
    }
    cache[0].path = path_copy;
    cache[0].face = face;
    return face;
}

void fastchart_ft_library_shutdown(void)
{
    /* Free cached faces explicitly. FT_Done_FreeType would walk and
     * free them anyway, but doing it ourselves keeps the cache state
     * machine deterministic and the path-string ownership tidy.
     * Called from PHP_GSHUTDOWN — once per thread under ZTS, once
     * at MSHUTDOWN under NTS. */
    fc_ft_face_slot *cache = FASTCHART_G(ft_face_cache);
    for (int i = 0; i < FC_FT_FACE_CACHE_N; i++) {
        if (cache[i].face) {
            FT_Done_Face(cache[i].face);
            cache[i].face = NULL;
        }
        if (cache[i].path) {
            free(cache[i].path);
            cache[i].path = NULL;
        }
    }
    if (FASTCHART_G(ft_lib) != NULL) {
        FT_Done_FreeType(FASTCHART_G(ft_lib));
        FASTCHART_G(ft_lib) = NULL;
    }
    FASTCHART_G(ft_lib_init_failed) = 0;
}

static void copy_family_name(char *out, size_t out_n, const char *src)
{
    if (!src || !*src) {
        snprintf(out, out_n, "sans-serif");
        return;
    }
    /* Allow ASCII letters/digits/space/hyphen/underscore through; any
     * other byte gets dropped. Keeps the result safe to inline in an
     * XML attribute even if the resolver is bypassed downstream. */
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < out_n; i++) {
        unsigned char c = (unsigned char)src[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9')
            || c == ' ' || c == '-' || c == '_' || c == '.') {
            out[j++] = (char)c;
        }
    }
    if (j == 0) {
        snprintf(out, out_n, "sans-serif");
        return;
    }
    out[j] = '\0';
}

void fastchart_target_resolve_font_family(fastchart_target_t *t,
                                           const char *font_path,
                                           char *out, size_t out_n)
{
    if (out_n == 0) return;
    if (!font_path || !*font_path) {
        copy_family_name(out, out_n, NULL);
        return;
    }

    /* Cache lookup. Pointer-equality first (palette helpers reuse
     * the same const char *); fall back to strcmp. */
    for (int i = 0; i < t->font_cache_n; i++) {
        fastchart_target_font_cache_entry *e = &t->font_cache[i];
        if (e->path == font_path
            || (e->path && strcmp(e->path, font_path) == 0)) {
            snprintf(out, out_n, "%s", e->family);
            return;
        }
    }

    /* FT lookup via the shared face cache. The face is owned by the
     * cache; family_name is a pointer into face-owned memory so we
     * copy the bytes out before the next caller might mutate face
     * state. */
    char raw[128] = "";
    FT_Face face = fastchart_ft_face(font_path);
    if (face && face->family_name) {
        snprintf(raw, sizeof(raw), "%s", face->family_name);
    }
    copy_family_name(out, out_n, raw[0] ? raw : NULL);

    /* Cache (path,family). Capacity is FASTCHART_TARGET_FONT_CACHE;
     * past that we silently skip the cache (next call hits FT
     * again — rare, since charts use 1-4 fonts max). */
    if (t->font_cache_n < FASTCHART_TARGET_FONT_CACHE) {
        fastchart_target_font_cache_entry *e =
            &t->font_cache[t->font_cache_n++];
        e->path = font_path;
        snprintf(e->family, sizeof(e->family), "%s", out);
    }
}

/* ============================================================ *
 * Text                                                          *
 * ============================================================ */

void fastchart_target_text(fastchart_target_t *t,
                            int x, int y,
                            const char *font_path, double size_pt,
                            int color, double angle_deg, int align,
                            const char *text)
{
    if (!text || !*text) return;

    /* SVG. size_pt -> size_px at 96 DPI baseline: px = pt * 4/3. */
    double size_px = size_pt * (4.0 / 3.0);
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    char family[64];
    fastchart_target_resolve_font_family(t, font_path, family, sizeof(family));

    if (t->u.svg.text_mode == FASTCHART_SVG_TEXT_PATHS) {
        /* Flatten each glyph to <path> via FreeType outline
         * decomposition. Self-contained — renders in plutovg / any
         * SVG rasterizer without text infrastructure. */
        fc_svg_emit_text_as_path(t->u.svg.buf, x, y, font_path, size_px,
                                  rgba, angle_deg, align, text, strlen(text));
    } else {
        /* Native <text>: smaller files; needs consumer text support. */
        fc_svg_emit_text(t->u.svg.buf, x, y, family, size_px,
                          rgba, angle_deg, align, text, strlen(text));
    }
}

/* ============================================================ *
 * Clip                                                          *
 * ============================================================ */

void fastchart_target_clip_push(fastchart_target_t *t,
                                 int x, int y, int w, int h)
{
    int id = t->u.svg.next_clip_id++;
    if (t->clip_depth < FASTCHART_TARGET_CLIP_DEPTH) {
        t->clip_stack[t->clip_depth++] = id;
    }
    fc_svg_emit_clip_open(t->u.svg.buf, id, x, y, w, h);
}

void fastchart_target_clip_pop(fastchart_target_t *t)
{
    if (t->clip_depth > 0) t->clip_depth--;
    fc_svg_emit_clip_close(t->u.svg.buf);
}

/* ============================================================ *
 * Image blit                                                    *
 * ============================================================ */

/* Sniff first 16 bytes of `data` (len `n`) to recover a MIME type
 * the SVG <image> data URI loader can decode. Returns "image/png" /
 * "image/jpeg" or NULL for unsupported formats. WebP / GIF / AVIF
 * are unsupported by plutosvg's data-URI path, so they fall through
 * to NULL and the caller skips emission. */
static const char *fc_sniff_image_mime(const unsigned char *data, size_t n)
{
    if (n < 8) return NULL;
    if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G'
        && data[4] == '\r' && data[5] == '\n' && data[6] == 0x1A
        && data[7] == '\n') {
        return "image/png";
    }
    if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
        return "image/jpeg";
    }
    return NULL;
}

/* Sniff width/height for a PNG or JPEG buffer. Returns 0 on success,
 * -1 if the header layout doesn't yield dimensions. Mirrors the
 * fastchart_axis.c routine but operates on an in-memory buffer. */
static int fc_sniff_image_dims_mem(const unsigned char *b, size_t n,
                                    int *out_w, int *out_h)
{
    if (n < 24) return -1;
    if (b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G') {
        /* PNG IHDR width/height are big-endian uint32. b[i] integer-
         * promotes to (signed) int before the shift; for b[16] >= 0x80,
         * b[16] << 24 lands in the sign bit which is C11 UB. Parse via
         * uint32_t, reject 0 or > INT_MAX, then narrow. */
        uint32_t w = ((uint32_t)b[16] << 24) | ((uint32_t)b[17] << 16)
                   | ((uint32_t)b[18] <<  8) |  (uint32_t)b[19];
        uint32_t h = ((uint32_t)b[20] << 24) | ((uint32_t)b[21] << 16)
                   | ((uint32_t)b[22] <<  8) |  (uint32_t)b[23];
        if (w == 0 || h == 0 || w > INT_MAX || h > INT_MAX) return -1;
        *out_w = (int)w;
        *out_h = (int)h;
        return 0;
    }
    if (b[0] == 0xFF && b[1] == 0xD8) {
        size_t i = 2;
        while (i + 9 < n) {
            if (b[i] != 0xFF) return -1;
            unsigned char marker = b[i + 1];
            if (marker == 0x00 || marker == 0xFF) { i++; continue; }
            if (marker == 0xD8 || marker == 0xD9) { i += 2; continue; }
            if ((marker >= 0xC0 && marker <= 0xCF)
                && marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
                *out_h = (b[i + 5] << 8) | b[i + 6];
                *out_w = (b[i + 7] << 8) | b[i + 8];
                return 0;
            }
            unsigned int seg_len = (b[i + 2] << 8) | b[i + 3];
            if (seg_len < 2) return -1;
            i += 2 + seg_len;
        }
    }
    return -1;
}

/* Load `path` once through the PHP stream layer. The stream wrapper
 * enforces open_basedir natively, so there is no TOCTOU window
 * between an open_basedir check and the open. Reads up to
 * FC_IMAGE_MAX_BYTES + 1 so a file at exactly the cap passes while a
 * file one byte over gets rejected (php_stream_copy_to_mem with a
 * smaller cap would silently truncate). On success populates *out
 * with the bytes, sniffed MIME type, and declared width/height, then
 * applies the dimension caps. Returns 0 / -1; on -1 nothing is
 * allocated. */
int fastchart_target_load_source_image(const char *path,
                                       fastchart_image_buf_t *out)
{
    if (!path || !*path || !out) return -1;
    memset(out, 0, sizeof(*out));

    /* Suppress the E_WARNING the stream wrapper would emit on
     * open_basedir refusal / missing file. Background-image and
     * icon callers fall back to their solid-color backup; the
     * refusal isn't an error condition from their POV. */
    int er = EG(error_reporting);
    EG(error_reporting) = 0;
    php_stream *stream = php_stream_open_wrapper((char *)path, "rb",
        IGNORE_PATH | IGNORE_URL, NULL);
    EG(error_reporting) = er;
    if (!stream) {
        if (EG(exception)) zend_clear_exception();
        return -1;
    }

    /* Reject non-regular files (directories, FIFOs, sockets, char
     * devices, /proc entries). The stream wrapper happily opens any
     * readable inode; without this check a render fed
     * /proc/self/maps would either trip the byte cap or return
     * unbounded data before the MIME sniff rejects it. The stat
     * call uses the stream's own backend (so wrappers without a real
     * stat — http, php://memory — silently fall through and the
     * MIME sniff is the only gate; that's intentional). */
    php_stream_statbuf ssb;
    if (php_stream_stat(stream, &ssb) == 0) {
        if (!S_ISREG(ssb.sb.st_mode)) {
            php_stream_close(stream);
            return -1;
        }
    }

    zend_string *raw =
        php_stream_copy_to_mem(stream, FC_IMAGE_MAX_BYTES + 1, 0);
    php_stream_close(stream);
    if (!raw) return -1;
    size_t n = ZSTR_LEN(raw);
    if (n == 0 || n > FC_IMAGE_MAX_BYTES) {
        zend_string_release(raw);
        return -1;
    }

    const unsigned char *bytes = (const unsigned char *)ZSTR_VAL(raw);
    const char *mime = fc_sniff_image_mime(bytes, n);
    if (!mime) {
        zend_string_release(raw);
        return -1;
    }

    int src_w = 0, src_h = 0;
    if (fc_sniff_image_dims_mem(bytes, n, &src_w, &src_h) != 0) {
        /* MIME passed but dimensions couldn't be recovered — refuse
         * the load. The dim cap is part of the contract; we don't
         * accept inputs the cap can't enforce. */
        zend_string_release(raw);
        return -1;
    }
    if (src_w <= 0 || src_h <= 0
        || src_w > FC_IMAGE_MAX_DIM
        || src_h > FC_IMAGE_MAX_DIM
        || (long long)src_w * (long long)src_h
            > (long long)FC_IMAGE_MAX_PIXELS) {
        zend_string_release(raw);
        return -1;
    }

    out->bytes  = raw;
    out->mime   = mime;
    out->width  = src_w;
    out->height = src_h;
    return 0;
}

void fastchart_target_image_release(fastchart_image_buf_t *buf)
{
    if (buf && buf->bytes) {
        zend_string_release(buf->bytes);
        buf->bytes = NULL;
    }
}

void fastchart_target_image_emit(fastchart_target_t *t,
                                 int x, int y, int w, int h,
                                 const fastchart_image_buf_t *buf)
{
    if (!t || !buf || !buf->bytes || !buf->mime) return;
    if (w <= 0 || h <= 0) return;

    zend_string *b64 = php_base64_encode(
        (const unsigned char *)ZSTR_VAL(buf->bytes),
        ZSTR_LEN(buf->bytes));
    if (!b64) return;
    fc_svg_emit_image_uri(t->u.svg.buf, x, y, w, h, buf->mime, ZSTR_VAL(b64));
    zend_string_release(b64);
}

void fastchart_target_image(fastchart_target_t *t,
                             int x, int y, int w, int h,
                             const char *path)
{
    if (w <= 0 || h <= 0) return;

    fastchart_image_buf_t buf;
    if (fastchart_target_load_source_image(path, &buf) != 0) return;
    fastchart_target_image_emit(t, x, y, w, h, &buf);
    fastchart_target_image_release(&buf);
}

void fastchart_target_gradient_rect(fastchart_target_t *t,
                                     int x, int y, int w, int h,
                                     uint32_t from_rgb, uint32_t to_rgb,
                                     int dir)
{
    if (w <= 0 || h <= 0) return;
    int id = t->u.svg.next_grad_id++;
    fc_svg_emit_gradient_rect(t->u.svg.buf, id, x, y, w, h,
                               from_rgb, to_rgb, dir);
}

void fastchart_target_gradient_polygon(fastchart_target_t *t,
                                        const fastchart_point_t *pts,
                                        int n,
                                        uint32_t from_rgb, uint32_t to_rgb,
                                        int dir)
{
    if (n < 3) return;
    int xs_stack[256], ys_stack[256];
    int *xs = xs_stack, *ys = ys_stack;
    if (n > 256) {
        xs = emalloc(sizeof(int) * (size_t)n);
        ys = emalloc(sizeof(int) * (size_t)n);
    }
    for (int i = 0; i < n; i++) {
        xs[i] = pts[i].x;
        ys[i] = pts[i].y;
    }
    int id = t->u.svg.next_grad_id++;
    fc_svg_emit_gradient_polygon(t->u.svg.buf, id, xs, ys, n,
                                  from_rgb, to_rgb, dir);
    if (n > 256) { efree(xs); efree(ys); }
}
