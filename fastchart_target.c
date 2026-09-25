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
#include "main/fopen_wrappers.h"
#include "main/php_streams.h"
#include "main/streams/php_stream_plain_wrapper.h"

#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#ifdef PHP_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#ifdef PHP_WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include <plutovg.h>

#include "fastchart_target.h"
#include "fastchart_svg.h"
#include "fastchart_rasterize.h"
#include "php_fastchart.h"
#include "fastchart_pdf.h"

/* Source-image bytes cap. FC_IMAGE_MAX_DIM / FC_IMAGE_MAX_PIXELS
 * are shared with the SVG rasterizer via fastchart_rasterize.h. */
#define FC_IMAGE_MAX_BYTES   (8 * 1024 * 1024)
#define FC_FONT_MAX_BYTES    (16 * 1024 * 1024)
#define FC_FONT_CACHE_BYTES  FC_FONT_MAX_BYTES

#define FC_PIN_CACHE_N 64
typedef struct {
    char *key;
    char *canonical;
    char *leaf;
    int dir_fd;
} fc_pinned_parent_entry;
static fc_pinned_parent_entry fc_pin_cache[FC_PIN_CACHE_N];
static unsigned fc_pin_next;
#ifdef PHP_WIN32
static SRWLOCK fc_pin_lock = SRWLOCK_INIT;
#define FC_PIN_LOCK() AcquireSRWLockExclusive(&fc_pin_lock)
#define FC_PIN_UNLOCK() ReleaseSRWLockExclusive(&fc_pin_lock)
#else
static pthread_mutex_t fc_pin_lock = PTHREAD_MUTEX_INITIALIZER;
#define FC_PIN_LOCK() pthread_mutex_lock(&fc_pin_lock)
#define FC_PIN_UNLOCK() pthread_mutex_unlock(&fc_pin_lock)
#endif

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
    /* Keep vector layout and text measurement at 96 DPI so setDpi cannot
     * inflate margins without scaling the logical viewport. */
    (void)dpi;
    t->u.svg.dpi = 96;
    t->u.svg.next_clip_id = 1;
    t->u.svg.next_grad_id = 1;
	t->u.svg.next_image_id = 1;
    t->u.svg.text_mode = (text_mode == FASTCHART_SVG_TEXT_NATIVE)
        ? FASTCHART_SVG_TEXT_NATIVE
        : FASTCHART_SVG_TEXT_PATHS;
}

void fastchart_target_from_pdf(fastchart_target_t *t, smart_str *out,
                                int width, int height, int dpi)
{
    memset(t, 0, sizeof(*t));
    t->kind = FASTCHART_TARGET_PDF;
#ifdef HAVE_FASTCHART_PDF
    t->u.pdf.state = fc_pdf_doc_open(out, width, height);
#else
    (void)out;
    t->u.pdf.state = NULL;
#endif
    t->u.pdf.width = width;
    t->u.pdf.height = height;
    /* Vector output is DPI-invariant, same rationale as the SVG target:
     * layout/text measurement stay on the 96 baseline. */
    (void)dpi;
    t->u.pdf.dpi = 96;
}

int fastchart_target_pdf_ok(const fastchart_target_t *t)
{
    return (t->kind == FASTCHART_TARGET_PDF && t->u.pdf.state != NULL) ? 1 : 0;
}

int fastchart_target_pdf_finish(fastchart_target_t *t)
{
    if (t->kind != FASTCHART_TARGET_PDF) return -1;
#ifdef HAVE_FASTCHART_PDF
    fc_pdf_state *st = (fc_pdf_state *)t->u.pdf.state;
    t->u.pdf.state = NULL;

    /* pdfioFileClose streams the xref/trailer (and the page content
     * stream) through our smart_str output callback, then frees all
     * pdfio objects in the same call. A memory_limit bailout during that
     * flush would longjmp past the frees, leaking pdfio's malloc'd
     * objects (untracked by the request allocator) for the life of a
     * worker, and pdfioFileClose is not re-entrant (it re-adds the PDF/A
     * OutputIntent and re-closes objects on each call), so the interrupted
     * close cannot be retried. Lift the limit across the bounded
     * flush so the close runs to completion atomically; the zend_try guard
     * still tears the document down before re-entering the bailout should
     * any other fatal (e.g. timeout) fire. */
    zend_long saved = PG(memory_limit);
    zend_set_memory_limit((size_t)-1);
    int rc = 0;
    zend_try {
        rc = fc_pdf_doc_close(st);
    } zend_catch {
        /* Reached only if some non-memory_limit fatal (e.g. timeout)
         * fired mid-close. pdfioFileClose is not re-entrant, so we can't
         * safely retry it; restore the limit and let the bailout unwind
         * the dying request. */
        zend_set_memory_limit((size_t)saved);
        zend_bailout();
    } zend_end_try();
    zend_set_memory_limit((size_t)saved);
    return rc;
#else
    return -1;
#endif
}

void fastchart_target_pdf_abort(fastchart_target_t *t)
{
    if (t->kind != FASTCHART_TARGET_PDF) return;
#ifdef HAVE_FASTCHART_PDF
    fc_pdf_doc_abort((fc_pdf_state *)t->u.pdf.state);
    t->u.pdf.state = NULL;
#endif
}

/* ============================================================ *
 * Color                                                         *
 * ============================================================ */

/* Fibonacci-hash slot for a packed rgba key. */
static inline int fc_color_slot(uint32_t key, int mask)
{
    return (int)((key * 2654435761u) & (uint32_t)mask);
}

static void fc_color_hash_grow(fastchart_target_t *t)
{
    int cap = t->color_hash_cap ? t->color_hash_cap * 2 : 128;
    if (t->color_hash) efree(t->color_hash);
    t->color_hash = safe_emalloc((size_t)cap, sizeof(int), 0);
    t->color_hash_cap = cap;
    memset(t->color_hash, 0xFF, (size_t)cap * sizeof(int)); /* -1 = empty */
    int mask = cap - 1;
    for (int i = 0; i < t->n_colors; i++) {
        int slot = fc_color_slot(t->color_rgba[i], mask);
        while (t->color_hash[slot] != -1) slot = (slot + 1) & mask;
        t->color_hash[slot] = i;
    }
}

int fastchart_target_color(fastchart_target_t *t, int r, int g, int b, int a)
{
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    if (a < 0) a = 0; else if (a > 255) a = 255;

    uint32_t key = ((uint32_t)a << 24) | ((uint32_t)r << 16)
                 | ((uint32_t)g << 8)  |  (uint32_t)b;

    if (t->n_colors * 2 >= t->color_hash_cap) {
        fc_color_hash_grow(t);
    }
    int mask = t->color_hash_cap - 1;
    int slot = fc_color_slot(key, mask);
    while (t->color_hash[slot] != -1) {
        if (t->color_rgba[t->color_hash[slot]] == key) {
            return t->color_hash[slot];
        }
        slot = (slot + 1) & mask;
    }
    if (t->n_colors >= t->color_cap) {
        int new_cap = t->color_cap ? t->color_cap * 2 : 64;
        t->color_rgba = erealloc(t->color_rgba,
                                 (size_t)new_cap * sizeof(*t->color_rgba));
        t->color_cap = new_cap;
    }
    int idx = t->n_colors++;
    t->color_rgba[idx] = key;
    t->color_hash[slot] = idx;
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
    if (t->kind == FASTCHART_TARGET_PDF) {
        *w = t->u.pdf.width;
        *h = t->u.pdf.height;
        return;
    }
    *w = t->u.svg.width;
    *h = t->u.svg.height;
}

int fastchart_target_get_dpi(fastchart_target_t *t)
{
    if (t->kind == FASTCHART_TARGET_PDF) return t->u.pdf.dpi;
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
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_line(t->u.pdf.state, x0, y0, x1, y1, rgba, thickness, dash);
        return;
    }
#endif
    fc_svg_emit_line(t->u.svg.buf, x0, y0, x1, y1, rgba, thickness, dash);
}

void fastchart_target_rect(fastchart_target_t *t,
                            int x, int y, int w, int h,
                            int color, int fill, int thickness)
{
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_rect(t->u.pdf.state, x, y, w, h, rgba, fill, thickness);
        return;
    }
#endif
    fc_svg_emit_rect(t->u.svg.buf, x, y, w, h, rgba, fill, thickness);
}

void fastchart_target_polygon(fastchart_target_t *t,
                               const fastchart_point_t *pts, int n,
                               int color, int fill, int thickness)
{
    if (n < 2) return;
    int xs_stack[256], ys_stack[256];
    int *xs = xs_stack, *ys = ys_stack;
    FASTCHART_SPLIT_POINTS(pts, n, xs, ys);
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_polygon(t->u.pdf.state, xs, ys, n, rgba, fill, thickness);
        if (n > 256) { efree(xs); efree(ys); }
        return;
    }
#endif
    fc_svg_emit_polygon(t->u.svg.buf, xs, ys, n, rgba, fill, thickness);
    if (n > 256) { efree(xs); efree(ys); }
}

void fastchart_target_polyline(fastchart_target_t *t,
                                const fastchart_point_t *pts, int n,
                                int color, int thickness, int dash)
{
    if (n < 2) return;
    int xs_stack[256], ys_stack[256];
    int *xs = xs_stack, *ys = ys_stack;
    FASTCHART_SPLIT_POINTS(pts, n, xs, ys);
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        /* One stroked path, matching SVG's single <polyline>: decomposing
         * into n-1 independent segments left butt-cap notches at interior
         * vertices for thickness > 1. */
        fc_pdf_emit_polyline(t->u.pdf.state, xs, ys, n, rgba, thickness, dash);
    } else
#endif
    fc_svg_emit_polyline(t->u.svg.buf, xs, ys, n, rgba, thickness, dash);
    if (n > 256) { efree(xs); efree(ys); }
}

void fastchart_target_arc(fastchart_target_t *t,
                           int cx, int cy, int rx, int ry,
                           double start_deg, double end_deg,
                           int color, int fill, int thickness)
{
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_arc(t->u.pdf.state, cx, cy, rx, ry,
                        start_deg, end_deg, rgba, fill, thickness);
        return;
    }
#endif
    fc_svg_emit_path_arc(t->u.svg.buf, cx, cy, rx, ry,
                          start_deg, end_deg, rgba, fill, thickness);
}

void fastchart_target_ellipse(fastchart_target_t *t,
                               int cx, int cy, int rx, int ry,
                               int color, int fill, int thickness)
{
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_ellipse(t->u.pdf.state, cx, cy, rx, ry, rgba, fill, thickness);
        return;
    }
#endif
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

/* Invalidate any glyph cache entries that point at `victim`. Called
 * just before FT_Done_Face on the evicted face so the cache never
 * carries dangling face pointers. */
static void fastchart_glyph_cache_drop_face(FT_Face victim)
{
    fc_glyph_cache_entry *g = FASTCHART_G(glyph_cache);
    for (int i = 0; i < FC_GLYPH_CACHE_N; i++) {
        if (g[i].face == victim) {
            free(g[i].ops);
            free(g[i].pts);
            memset(&g[i], 0, sizeof(g[i]));
        }
    }
    /* The measurement cache keys on the same face pointer; drop its
     * entries too so a reused pointer (a later FT_New_Face landing at
     * the freed address) can't produce a stale-advance false hit. */
    fc_measure_cache_entry *m = FASTCHART_G(measure_cache);
    for (int i = 0; i < FC_MEASURE_CACHE_N; i++) {
        if (m[i].face == victim) m[i].face = NULL;
    }
}

/* Advance-only glyph lookup for text measurement, backed by the
 * per-thread measure cache. The caller must have positioned the face
 * with FT_Set_Char_Size(size_64, dpi) already; the cache key records
 * that size so a hit returns an advance consistent with it. On a miss
 * the glyph is loaded for its advance only (no outline decompose) and
 * cached. Returns 0 with the advance in *out_adv, or -1 on FT_Load_Glyph
 * failure (caller skips the glyph). */
int fastchart_measured_advance(FT_Face face, int32_t size_64, int dpi,
                               uint32_t codepoint, int32_t *out_adv)
{
    fc_measure_cache_entry *m = FASTCHART_G(measure_cache);
    fc_measure_cache_entry *e = &m[codepoint & (FC_MEASURE_CACHE_N - 1)];
    if (e->face == face && e->codepoint == codepoint
        && e->size_64 == size_64 && e->dpi == dpi) {
        *out_adv = e->advance_x_64;
        return 0;
    }
    FT_UInt gi = FT_Get_Char_Index(face, codepoint);
    if (FT_Load_Glyph(face, gi, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING)) {
        return -1;
    }
    int32_t adv = (int32_t)face->glyph->advance.x;
    e->face = face;
    e->codepoint = codepoint;
    e->size_64 = size_64;
    e->dpi = dpi;
    e->advance_x_64 = adv;
    *out_adv = adv;
    return 0;
}

static zend_string *fc_stream_copy_and_close(php_stream *stream, size_t max_len)
{
	php_stream * volatile live = stream;
	zend_string *raw = NULL;
	zend_try {
		raw = php_stream_copy_to_mem((php_stream *)live, max_len, 0);
		php_stream *closing = (php_stream *)live;
		live = NULL;
		php_stream_close(closing);
	} zend_catch {
        if (live) php_stream_close((php_stream *)live);
        zend_bailout();
    } zend_end_try();
    return raw;
}

static void fc_close_descriptor(int fd)
{
#ifdef PHP_WIN32
    _close(fd);
#else
    close(fd);
#endif
}

static php_stream *fc_stream_from_fd(int fd)
{
    php_stream * volatile stream = NULL;
    volatile int live_fd = fd;
    zend_try {
        stream = php_stream_fopen_from_fd(live_fd, "rb", NULL);
        if (stream) live_fd = -1;
    } zend_catch {
        if (live_fd >= 0) fc_close_descriptor(live_fd);
        zend_bailout();
    } zend_end_try();
    if (!stream && live_fd >= 0) fc_close_descriptor(live_fd);
    return stream;
}


/* Walk an already-resolved absolute path with descriptor-relative opens.
 * Each directory and the leaf are opened with O_NOFOLLOW, so replacing a
 * component with a symlink cannot redirect the final descriptor. */
static int fc_open_nofollow_absolute(const char *path, int flags)
{
#ifdef PHP_WIN32
    return VCWD_OPEN(path, flags);
#elif defined(O_NOFOLLOW) && defined(O_DIRECTORY)
    if (!path || path[0] != '/') return -1;

    const char *leaf = strrchr(path, '/');
    if (!leaf || !leaf[1]) return -1;
    size_t leaf_len = strlen(leaf + 1);
    if (leaf_len > NAME_MAX) return -1;

#ifdef O_PATH
    int dir_flags = O_PATH | O_DIRECTORY | O_NOFOLLOW;
#elif defined(O_SEARCH)
    int dir_flags = O_SEARCH | O_DIRECTORY | O_NOFOLLOW;
#else
    int dir_flags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW;
#endif
#ifdef O_CLOEXEC
    dir_flags |= O_CLOEXEC;
#endif
    int dirfd = open("/", dir_flags);
    if (dirfd < 0) return -1;

    const char *cursor = path;
    char component[NAME_MAX + 1];
    while (cursor < leaf) {
        while (*cursor == '/') cursor++;
        const char *end = strchr(cursor, '/');
        if (!end || end > leaf) end = leaf;
        size_t len = (size_t)(end - cursor);
        if (len == 0 || len > NAME_MAX) {
            close(dirfd);
            return -1;
        }
        memcpy(component, cursor, len);
        component[len] = '\0';
        int next = openat(dirfd, component, dir_flags);
        close(dirfd);
        if (next < 0) return -1;
        dirfd = next;
        cursor = end;
    }
    memcpy(component, leaf + 1, leaf_len + 1);

    int leaf_flags = flags | O_NOFOLLOW;
#ifdef O_CLOEXEC
    leaf_flags |= O_CLOEXEC;
#endif
    int fd = openat(dirfd, component, leaf_flags);
    close(dirfd);
    return fd;
#else
    /* Without no-follow descriptor-relative opens, fail closed rather than
     * reintroducing the check/open symlink race. */
    (void)path;
    (void)flags;
    return -1;
#endif
}

static int fc_pinned_parent_allowed(int dirfd, const char *leaf)
{
    if (!PG(open_basedir) || !*PG(open_basedir)) return 0;
#if defined(__linux__)
    char fd_path[64];
    int fd_path_len = snprintf(fd_path, sizeof(fd_path),
        "/proc/self/fd/%d", dirfd);
    if (fd_path_len <= 0 || (size_t)fd_path_len >= sizeof(fd_path)) return -1;
    char resolved[MAXPATHLEN];
    ssize_t resolved_len = readlink(fd_path, resolved, sizeof(resolved) - 1);
    if (resolved_len <= 0) return -1;
    size_t length = (size_t)resolved_len;
    resolved[length] = '\0';
    size_t leaf_len = strlen(leaf);
    if (length + leaf_len + 2 > sizeof(resolved)) return -1;
    resolved[length++] = '/';
    memcpy(resolved + length, leaf, leaf_len + 1);
    return php_check_open_basedir_ex(resolved, 0) == 0 ? 0 : -1;
#elif defined(__APPLE__)
    char resolved[MAXPATHLEN];
    if (fcntl(dirfd, F_GETPATH, resolved) == -1) return -1;
    size_t length = strlen(resolved);
    size_t leaf_len = strlen(leaf);
    if (length + leaf_len + 2 > sizeof(resolved)) return -1;
    resolved[length++] = '/';
    memcpy(resolved + length, leaf, leaf_len + 1);
    return php_check_open_basedir_ex(resolved, 0) == 0 ? 0 : -1;
#else
    (void)dirfd;
    (void)leaf;
    return -1;
#endif
}

static int fc_dup_descriptor(int fd)
{
#ifdef PHP_WIN32
    return _dup(fd);
#else
    return dup(fd);
#endif
}

static void fc_pin_cache_remove(const char *key)
{
    FC_PIN_LOCK();
    for (int i = 0; i < FC_PIN_CACHE_N; i++) {
        if (fc_pin_cache[i].key && strcmp(fc_pin_cache[i].key, key) == 0) {
            fc_close_descriptor(fc_pin_cache[i].dir_fd);
            free(fc_pin_cache[i].key);
            free(fc_pin_cache[i].canonical);
            free(fc_pin_cache[i].leaf);
            memset(&fc_pin_cache[i], 0, sizeof(fc_pin_cache[i]));
            break;
        }
    }
    FC_PIN_UNLOCK();
}

static int fc_pin_cache_store(const char *canonical, const char *leaf,
                              int dir_fd)
{
    size_t canonical_len = strlen(canonical);
    size_t leaf_len = strlen(leaf);
    char *key_copy = malloc(canonical_len + 1);
    char *canonical_copy = malloc(canonical_len + 1);
    char *leaf_copy = malloc(leaf_len + 1);
    if (!key_copy || !canonical_copy || !leaf_copy) {
        free(key_copy);
        free(canonical_copy);
        free(leaf_copy);
        fc_close_descriptor(dir_fd);
        return -1;
    }
    memcpy(key_copy, canonical, canonical_len + 1);
    memcpy(canonical_copy, canonical, canonical_len + 1);
    memcpy(leaf_copy, leaf, leaf_len + 1);

    FC_PIN_LOCK();
    int slot = -1;
    for (int i = 0; i < FC_PIN_CACHE_N; i++) {
        if (fc_pin_cache[i].key && strcmp(fc_pin_cache[i].key, canonical) == 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        slot = (int)(fc_pin_next++ % FC_PIN_CACHE_N);
        if (fc_pin_cache[slot].key) {
            fc_close_descriptor(fc_pin_cache[slot].dir_fd);
            free(fc_pin_cache[slot].key);
            free(fc_pin_cache[slot].canonical);
            free(fc_pin_cache[slot].leaf);
        }
    } else if (fc_pin_cache[slot].key) {
        fc_close_descriptor(fc_pin_cache[slot].dir_fd);
        free(fc_pin_cache[slot].key);
        free(fc_pin_cache[slot].canonical);
        free(fc_pin_cache[slot].leaf);
    }
    fc_pin_cache[slot].key = key_copy;
    fc_pin_cache[slot].canonical = canonical_copy;
    fc_pin_cache[slot].leaf = leaf_copy;
    fc_pin_cache[slot].dir_fd = dir_fd;
    FC_PIN_UNLOCK();
    return 0;
}

static int fc_pin_canonical_path(const char *path, char *canonical)
{
    if (VCWD_REALPATH(path, canonical) != NULL) return 1;
    if (!php_is_stream_path(path)) return 0;
    const char *path_for_open = NULL;
    int er = EG(error_reporting);
    EG(error_reporting) = 0;
    php_stream_wrapper *wrapper = php_stream_locate_url_wrapper(
        path, &path_for_open, IGNORE_PATH);
    EG(error_reporting) = er;
    if (EG(exception)) {
        zend_clear_exception();
        return 0;
    }
    if (wrapper == &php_plain_files_wrapper && path_for_open) {
        return VCWD_REALPATH(path_for_open, canonical) != NULL;
    }
    return 0;
}

int fastchart_pin_path(const char *path)
{
#ifdef PHP_WIN32
    return 0;
#endif
    if (!path || !*path) return 0;
    char canonical[MAXPATHLEN];
    if (!fc_pin_canonical_path(path, canonical)) {
        return 0;
    }
    if (PG(open_basedir) && *PG(open_basedir)
        && php_check_open_basedir_ex(canonical, 0) != 0) {
        fc_pin_cache_remove(canonical);
        return -1;
    }
    const char *leaf = strrchr(canonical, '/');
    if (!leaf || !leaf[1]) return 0;
    char parent[MAXPATHLEN];
    size_t parent_len = (size_t)(leaf - canonical);
    if (parent_len == 0) {
        parent[0] = '/';
        parent[1] = '\0';
    } else {
        memcpy(parent, canonical, parent_len);
        parent[parent_len] = '\0';
    }
    int dir_flags = O_RDONLY;
#ifdef O_DIRECTORY
    dir_flags |= O_DIRECTORY;
#endif
#ifdef O_PATH
    dir_flags = O_PATH | O_DIRECTORY;
#endif
#ifdef O_NOFOLLOW
    dir_flags |= O_NOFOLLOW;
#endif
#ifdef O_CLOEXEC
    dir_flags |= O_CLOEXEC;
#endif
    int dir_fd;
    if (parent[0] == '/' && parent[1] == '\0') {
        dir_fd = open("/", dir_flags);
    } else {
        dir_fd = fc_open_nofollow_absolute(parent, dir_flags);
    }
    volatile int owned_fd = dir_fd;
    int pin_result = -1;
    zend_try {
        if (fc_pinned_parent_allowed(dir_fd, leaf + 1) == 0) {
            pin_result = fc_pin_cache_store(canonical, leaf + 1, owned_fd);
            owned_fd = -1;
        }
    } zend_catch {
        if (owned_fd >= 0) fc_close_descriptor(owned_fd);
        zend_bailout();
    } zend_end_try();
    if (pin_result != 0) {
        if (owned_fd >= 0) fc_close_descriptor(owned_fd);
        return -1;
    }
    return 0;
}

int fastchart_open_pinned_fd(const char *path, int flags, int *out_fd)
{
#ifndef PHP_WIN32
    *out_fd = -1;
    if (!path || !*path) return 0;
    char canonical[MAXPATHLEN];
    if (!fc_pin_canonical_path(path, canonical)) return 0;
    char leaf[NAME_MAX + 1];
    int dir_fd = -1;
    FC_PIN_LOCK();
    for (int i = 0; i < FC_PIN_CACHE_N; i++) {
        if (!fc_pin_cache[i].key || strcmp(fc_pin_cache[i].key, canonical) != 0) {
            continue;
        }
        size_t leaf_len = strlen(fc_pin_cache[i].leaf);
        if (leaf_len >= sizeof(leaf)) {
            FC_PIN_UNLOCK();
            return -1;
        }
        memcpy(leaf, fc_pin_cache[i].leaf, leaf_len + 1);
        dir_fd = fc_dup_descriptor(fc_pin_cache[i].dir_fd);
        break;
    }
    FC_PIN_UNLOCK();
    if (dir_fd < 0) return 0;
    volatile int fd = -1;
    int open_result = -1;
    zend_try {
        if (fc_pinned_parent_allowed(dir_fd, leaf) == 0) {
            int leaf_flags = flags | O_NOFOLLOW;
#ifdef O_CLOEXEC
            leaf_flags |= O_CLOEXEC;
#endif
            fd = openat(dir_fd, leaf, leaf_flags);
            if (fd >= 0 && fc_pinned_parent_allowed(dir_fd, leaf) == 0) {
                open_result = 1;
            } else if (fd >= 0) {
                fc_close_descriptor(fd);
                fd = -1;
            }
        }
    } zend_catch {
        if (fd >= 0) fc_close_descriptor(fd);
        fc_close_descriptor(dir_fd);
        zend_bailout();
    } zend_end_try();
    fc_close_descriptor(dir_fd);
    if (open_result != 1) return -1;
    *out_fd = fd;
    return 1;
#else
    *out_fd = -1;
    return 0;
#endif
}


void fastchart_pin_cache_shutdown(void)
{
    FC_PIN_LOCK();
    for (int i = 0; i < FC_PIN_CACHE_N; i++) {
        if (fc_pin_cache[i].key) {
            fc_close_descriptor(fc_pin_cache[i].dir_fd);
            free(fc_pin_cache[i].key);
            free(fc_pin_cache[i].canonical);
            free(fc_pin_cache[i].leaf);
            memset(&fc_pin_cache[i], 0, sizeof(fc_pin_cache[i]));
        }
    }
    FC_PIN_UNLOCK();
}

/* Enforce open_basedir, open without blocking on FIFOs, then bind the
 * access-policy check and object-type validation to the descriptor that
 * will actually be read. */
static int fc_open_regular_path(const char *path, zend_stat_t *st)
{
    int flags = O_RDONLY;
#ifdef O_NONBLOCK
    flags |= O_NONBLOCK;
#endif
#ifdef O_BINARY
    flags |= O_BINARY;
#endif
    int fd = -1;
    int pinned = fastchart_open_pinned_fd(path, flags, &fd);
    if (pinned < 0) return -1;
    if (pinned == 0) {
#ifdef PHP_WIN32
        if (php_check_open_basedir_ex(path, 0) != 0) return -1;
        fd = fc_open_nofollow_absolute(path, flags);
#else
        if (PG(open_basedir) && *PG(open_basedir)) {
            if (fastchart_pin_path(path) != 0) return -1;
            pinned = fastchart_open_pinned_fd(path, flags, &fd);
            if (pinned <= 0) return -1;
        } else {
            char resolved[MAXPATHLEN];
            char *resolved_path = VCWD_REALPATH(path, resolved);
            if (!resolved_path) return -1;
            fd = fc_open_nofollow_absolute(resolved_path, flags);
        }
#endif
    }
    if (zend_fstat(fd, st) != 0 || !S_ISREG(st->st_mode)) {
        fc_close_descriptor(fd);
        return -1;
    }
    return fd;
}


static bool fc_path_has_nested_url(const char *path)
{
    const char *first = strstr(path, "://");
    return first && strstr(first + 3, "://") != NULL;
}
static bool fc_path_is_php_filter(const char *path)
{
    static const char prefix[] = "php://filter";
    for (size_t i = 0; i < sizeof(prefix) - 1; i++) {
        unsigned char c = (unsigned char)path[i];
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c + ('a' - 'A'));
        if (c != (unsigned char)prefix[i]) return false;
    }
    return true;
}
static bool fc_path_starts_with_ci(const char *path, const char *prefix)
{
    for (size_t i = 0; prefix[i]; i++) {
        unsigned char c = (unsigned char)path[i];
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c + ('a' - 'A'));
        if (c != (unsigned char)prefix[i]) return false;
    }
    return true;
}
static bool fc_path_is_blocked_file_wrapper(const char *path)
{
    return fc_path_starts_with_ci(path, "compress.zlib://")
        || fc_path_starts_with_ci(path, "compress.bzip2://")
        || fc_path_starts_with_ci(path, "zlib://")
        || fc_path_starts_with_ci(path, "bzip2://")
        || fc_path_starts_with_ci(path, "php://stdin")
        || fc_path_starts_with_ci(path, "php://stdout")
        || fc_path_starts_with_ci(path, "php://stderr")
        || fc_path_starts_with_ci(path, "php://input")
        || fc_path_starts_with_ci(path, "php://output")
        || fc_path_starts_with_ci(path, "php://fd/");
}



/* Keep URI/custom-wrapper compatibility on the legacy stream path. Plain
 * POSIX paths never enter this branch: they use the descriptor-first open
 * below, which is the only path for which PHP exposes O_NONBLOCK. */
static php_stream *fc_open_wrapped_source(const char *path, bool *handled)
{
    *handled = false;
    if (!php_is_stream_path(path)) return NULL;
    *handled = true;
    if (fc_path_has_nested_url(path)) return NULL;
    if (fc_path_is_php_filter(path)) return NULL;
    if (fc_path_is_blocked_file_wrapper(path)) return NULL;

    const char *path_for_open = NULL;
    int er = EG(error_reporting);
    EG(error_reporting) = 0;
    php_stream_wrapper *wrapper = php_stream_locate_url_wrapper(
        path, &path_for_open, IGNORE_PATH);
    EG(error_reporting) = er;
    if (EG(exception)) {
        zend_clear_exception();
        return NULL;
    }
    if (!wrapper || wrapper->is_url) return NULL;
    if (wrapper == &php_plain_files_wrapper) {
        if (!path_for_open || !*path_for_open) return NULL;
        zend_stat_t st;
        int fd = -1;
        int open_flags = O_RDONLY;
#ifdef O_NONBLOCK
        open_flags |= O_NONBLOCK;
#endif
        int pinned = fastchart_open_pinned_fd(path, open_flags, &fd);
        if (pinned < 0) return NULL;
        if (pinned == 0) {
            if (PG(open_basedir) && *PG(open_basedir)) {
                if (fastchart_pin_path(path) != 0) return NULL;
                pinned = fastchart_open_pinned_fd(path, open_flags, &fd);
                if (pinned < 0) return NULL;
            }
            if (pinned == 0) {
                fd = fc_open_regular_path(path_for_open, &st);
                if (fd < 0) {
                    if (EG(exception)) zend_clear_exception();
                    return NULL;
                }
            } else if (zend_fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
                fc_close_descriptor(fd);
                return NULL;
            }
        } else if (zend_fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
            fc_close_descriptor(fd);
            return NULL;
        }
        php_stream *stream = fc_stream_from_fd(fd);
        if (!stream && EG(exception)) zend_clear_exception();
        return stream;
    }

    if (PG(open_basedir) && *PG(open_basedir)) {
        int check_er = EG(error_reporting);
        EG(error_reporting) = 0;
        int denied = php_check_open_basedir_ex(path, 0) != 0;
        EG(error_reporting) = check_er;
        if (EG(exception)) zend_clear_exception();
        if (denied) return NULL;
    }
    er = EG(error_reporting);
    EG(error_reporting) = 0;
    php_stream *stream = php_stream_open_wrapper((char *)path, "rb",
        IGNORE_PATH, NULL);
    EG(error_reporting) = er;
    if (!stream) {
        if (EG(exception)) zend_clear_exception();
        return NULL;
    }

    /* User wrappers may omit stream_stat; PHP emits a warning for that
     * optional operation. Keep the historical MIME-sniff fallback and let
     * the read itself validate the payload instead. */
    return stream;
}



static unsigned char *fastchart_load_font_bytes(const char *font_path,
                                                size_t *out_len)
{
    *out_len = 0;
    zend_stat_t st;
    int fd = fc_open_regular_path(font_path, &st);
    if (fd < 0 || st.st_size <= 0 ||
        (uintmax_t)st.st_size > FC_FONT_MAX_BYTES) {
        if (fd >= 0) fc_close_descriptor(fd);
        return NULL;
    }

    php_stream *stream = fc_stream_from_fd(fd);
    if (!stream) return NULL;

    zend_string *raw = fc_stream_copy_and_close(stream, FC_FONT_MAX_BYTES + 1);
    if (!raw || ZSTR_LEN(raw) == 0 || ZSTR_LEN(raw) > FC_FONT_MAX_BYTES) {
        if (raw) zend_string_release(raw);
        return NULL;
    }
    size_t len = ZSTR_LEN(raw);
    unsigned char *data = malloc(len);
    if (!data) {
        zend_string_release(raw);
        return NULL;
    }
    memcpy(data, ZSTR_VAL(raw), len);
    zend_string_release(raw);
    *out_len = len;
    return data;
}

static void fastchart_ft_face_slot_release(fc_ft_face_slot *slot)
{
	if (slot->face) {
        fastchart_glyph_cache_drop_face(slot->face);
        FT_Done_Face(slot->face);
	}
	free(slot->path);
	free(slot->data);
	memset(slot, 0, sizeof(*slot));
}

FT_Face fastchart_ft_face(const char *font_path)
{
    if (!font_path) return NULL;
    FT_Library lib = fastchart_ft_library();
    if (PG(open_basedir) && *PG(open_basedir)
        && fastchart_pin_path(font_path) != 0) return NULL;
    if (!lib) return NULL;

    fc_ft_face_slot *cache = FASTCHART_G(ft_face_cache);

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

    /* Raw libc allocators are deliberate throughout this cache (an
     * exception to the extension-wide pemalloc convention): the
     * face/glyph caches live for the process, and unlike pemalloc(n, 1)
     * a malloc failure degrades to an uncached face instead of fataling
     * the request.
     *
     * Miss. Build the new entry (malloc + FT_New_Memory_Face) BEFORE
     * evicting anything so a failure mid-build leaves the cache
     * untouched. */
    size_t data_len;
    unsigned char *data = fastchart_load_font_bytes(font_path, &data_len);
    if (!data) return NULL;

    size_t path_len = strlen(font_path);
    char  *path_copy = malloc(path_len + 1);
    if (!path_copy) {
        free(data);
        return NULL;
    }
    memcpy(path_copy, font_path, path_len + 1);

    FT_Face face = NULL;
    if (FT_New_Memory_Face(lib, data, (FT_Long)data_len, 0, &face)) {
        free(path_copy);
        free(data);
        return NULL;
    }

	/* Keep the retained backing bytes within one maximum-size font.
	 * Typical fonts still occupy all four LRU slots; unusually large
	 * faces evict as many cold entries as their bytes require. */
	size_t cached_bytes = 0;
	int used = 0;
	for (int i = 0; i < FC_FT_FACE_CACHE_N; i++) {
		if (!cache[i].path) break;
		cached_bytes += cache[i].data_len;
		used++;
	}

	while (used > 0 && (used >= FC_FT_FACE_CACHE_N ||
			cached_bytes > FC_FONT_CACHE_BYTES - data_len)) {
		used--;
		cached_bytes -= cache[used].data_len;
		fastchart_ft_face_slot_release(&cache[used]);
	}
	for (int i = used; i > 0; i--) {
        cache[i] = cache[i - 1];
    }
    cache[0].path = path_copy;
    cache[0].data = data;
    cache[0].data_len = data_len;
    cache[0].face = face;
    return face;
}

void fastchart_ft_library_shutdown(void)
{
    /* Free cached glyph paths first; they reference cached faces. */
    fc_glyph_cache_entry *gcache = FASTCHART_G(glyph_cache);
    for (int i = 0; i < FC_GLYPH_CACHE_N; i++) {
        free(gcache[i].ops);
        free(gcache[i].pts);
        memset(&gcache[i], 0, sizeof(gcache[i]));
    }
    memset(FASTCHART_G(measure_cache), 0,
           sizeof(fc_measure_cache_entry) * FC_MEASURE_CACHE_N);

    /* Free cached faces explicitly (FT_Done_FreeType would too) so the
     * cache state and path-string ownership stay deterministic. Called
     * from PHP_GSHUTDOWN: once per thread under ZTS, once at MSHUTDOWN
     * under NTS. */
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
        if (cache[i].data) {
            free(cache[i].data);
            cache[i].data = NULL;
            cache[i].data_len = 0;
        }
    }
    if (FASTCHART_G(ft_lib) != NULL) {
        FT_Done_FreeType(FASTCHART_G(ft_lib));
        FASTCHART_G(ft_lib) = NULL;
    }
    FASTCHART_G(ft_lib_init_failed) = 0;
}

/* Glyph cache lookup. Linear scan + swap-to-front. Returns NULL on
 * miss (caller must FT_Load_Glyph + FT_Outline_Decompose itself, then
 * insert via fastchart_glyph_cache_insert).
 *
 * Caller MUST have already called FT_Set_Pixel_Sizes(face, 0, pix_size)
 * to position the face at the requested size; the cache uses
 * (face, pix_size, codepoint) as the key but does not touch FT state.
 */
const fc_glyph_cache_entry *fastchart_glyph_cache_get(
    FT_Face face, uint16_t pix_size, uint32_t codepoint)
{
    fc_glyph_cache_entry *g = FASTCHART_G(glyph_cache);
    for (int i = 0; i < FC_GLYPH_CACHE_N; i++) {
        if (g[i].face == face
            && g[i].pix_size == pix_size
            && g[i].codepoint == codepoint) {
            if (i != 0) {
                fc_glyph_cache_entry tmp = g[0];
                g[0] = g[i];
                g[i] = tmp;
            }
            return &g[0];
        }
    }
    return NULL;
}

/* Glyph cache insert. Evicts the tail slot. ops_buf and pts_buf are
 * owned by the cache after this call (do not free in the caller).
 * Pass NULL for both with n_ops == 0 for whitespace glyphs (advance
 * is still cached). */
void fastchart_glyph_cache_insert(FT_Face face, uint16_t pix_size,
                                   uint32_t codepoint, int32_t advance_x_64,
                                   char *ops_buf, uint16_t n_ops,
                                   float *pts_buf, uint16_t n_pts)
{
    fc_glyph_cache_entry *g = FASTCHART_G(glyph_cache);
    int tail = FC_GLYPH_CACHE_N - 1;
    free(g[tail].ops);
    free(g[tail].pts);
    for (int i = tail; i > 0; i--) {
        g[i] = g[i - 1];
    }
    g[0].face = face;
    g[0].pix_size = pix_size;
    g[0].codepoint = codepoint;
    g[0].advance_x_64 = advance_x_64;
    g[0].ops = ops_buf;
    g[0].n_ops = n_ops;
    g[0].pts = pts_buf;
    g[0].n_pts = n_pts;
}

static void copy_family_name(char *out, size_t out_n, const char *src)
{
    if (!src || !*src) {
        snprintf(out, out_n, "sans-serif");
        return;
    }
    /* Allow ASCII letters/digits/space/hyphen/underscore/dot through;
     * any other byte gets dropped. Keeps the result safe to inline in an
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
     * past that the cache is skipped and the next call hits FT again
     * (rare, since charts use 1-4 fonts). */
    if (t->font_cache_n < FASTCHART_TARGET_FONT_CACHE) {
        fastchart_target_font_cache_entry *e =
            &t->font_cache[t->font_cache_n++];
        e->path = font_path;
        snprintf(e->family, sizeof(e->family), "%s", out);
    }
}

/* ============================================================ *
 * Release                                                       *
 * ============================================================ */

void fastchart_target_release(fastchart_target_t *t)
{
	for (int i = 0; i < t->image_cache_n; i++) {
		fastchart_target_image_cache_entry *entry = &t->image_cache[i];
		if (entry->path) efree(entry->path);
		if (entry->bytes) zend_string_release(entry->bytes);
		memset(entry, 0, sizeof(*entry));
	}
	t->image_cache_n = 0;
    if (t->color_rgba) {
        efree(t->color_rgba);
        t->color_rgba = NULL;
    }
    t->n_colors = 0;
    t->color_cap = 0;
    if (t->color_hash) {
        efree(t->color_hash);
        t->color_hash = NULL;
    }
    t->color_hash_cap = 0;
}

/* ============================================================ *
 * Clip                                                          *
 * ============================================================ */

void fastchart_target_clip_push(fastchart_target_t *t,
                                 int x, int y, int w, int h)
{
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        if (t->clip_depth < FASTCHART_TARGET_CLIP_DEPTH) t->clip_depth++;
        fc_pdf_emit_clip_open(t->u.pdf.state, x, y, w, h);
        return;
    }
#endif
    /* Saturate rather than overflow: signed wrap is UB, and a wrapped
     * negative id would emit an invalid NCName. Unreachable in practice
     * (2^31 clips in one document exhausts memory first). */
    int id = t->u.svg.next_clip_id;
    if (t->u.svg.next_clip_id < INT_MAX) t->u.svg.next_clip_id++;
    if (t->clip_depth < FASTCHART_TARGET_CLIP_DEPTH) {
        t->clip_stack[t->clip_depth++] = id;
    }
    fc_svg_emit_clip_open(t->u.svg.buf, t->u.svg.id_ns, id, x, y, w, h);
}

void fastchart_target_clip_pop(fastchart_target_t *t)
{
    if (t->clip_depth > 0) t->clip_depth--;
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_clip_close(t->u.pdf.state);
        return;
    }
#endif
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

/* Walk a PNG's chunk list and verify every declared chunk length
 * fits within the buffer. The vendored stb_image accepts IDAT chunk
 * lengths up to ~1 GB before attempting the read, so a 30-byte PNG
 * declaring length=0x3FFFFFFF triggers a ~1 GB realloc, a single-
 * request OOM-kill on memory-constrained workers. stb's threat
 * model treats resource validation as the caller's responsibility,
 * so the check lives here. Returns 0 on PNGs with self-consistent
 * chunk framing (or any non-PNG input), -1 if any chunk declares a
 * length that overruns the remaining buffer. */
static int fc_validate_png_chunks(const unsigned char *b, size_t n)
{
    if (n < 16 || b[0] != 0x89 || b[1] != 'P'
        || b[2] != 'N' || b[3] != 'G') {
        return 0;
    }
    size_t off = 8; /* past PNG signature */
    /* Cap iterations only as a runaway guard: each pass advances off
     * by at least 12 bytes, so a buffer under FC_IMAGE_MAX_BYTES already
     * bounds the loop. libpng emits ~8KB IDATs, so a valid file near the
     * cap can hold well over 1000 chunks; keep the ceiling high enough
     * that legal PNGs are never rejected into the solid-fill fallback. */
    for (int iter = 0; iter < 65536; iter++) {
        if (off + 12 > n) return 0; /* truncated tail; stb handles it */
        uint32_t len = ((uint32_t)b[off]     << 24)
                     | ((uint32_t)b[off + 1] << 16)
                     | ((uint32_t)b[off + 2] <<  8)
                     |  (uint32_t)b[off + 3];
        if (len > (uint32_t)INT32_MAX) return -1;
        if ((size_t)len > n - off - 12) return -1;
        if (b[off + 4] == 'I' && b[off + 5] == 'E'
            && b[off + 6] == 'N' && b[off + 7] == 'D') {
            return 0;
        }
        off += 12 + (size_t)len;
    }
    return -1;
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

/* Load `path` through a descriptor opened with PHP's open_basedir policy
 * and nonblocking flags. The descriptor is fstat-checked before the PHP
 * stream layer reads it, so a pathname swapped to a FIFO cannot block the
 * render. Reads up to FC_IMAGE_MAX_BYTES + 1 so a file at exactly the cap
 * passes while a file one byte over gets rejected (php_stream_copy_to_mem
 * with a smaller cap would silently truncate). On success populates *out
 * with the bytes, sniffed MIME type, and declared width/height, then
 * applies the dimension caps. Returns 0 / -1; on -1 the cache entry
 * retains only its path. */
static int fastchart_load_source_image(const char *path,
                                       fastchart_target_image_cache_entry *out)
{
    if (!path || !*path || !out) return -1;

    /* Plain paths use O_NONBLOCK and fstat before any read. URI/custom
     * wrappers retain the prior stream-wrapper behavior because PHP does
     * not expose a descriptor-first nonblocking open for arbitrary
     * wrappers. */
    bool wrapped = false;
    php_stream *stream = fc_open_wrapped_source(path, &wrapped);
    if (!wrapped) {
        zend_stat_t st;
        int fd = fc_open_regular_path(path, &st);
        if (fd < 0) {
            if (EG(exception)) zend_clear_exception();
            return -1;
        }
        stream = fc_stream_from_fd(fd);
        if (!stream) {
            if (EG(exception)) zend_clear_exception();
            return -1;
        }
    } else if (!stream) {
        return -1;
    }

    zend_string *raw;
    if (wrapped) {
        int er = EG(error_reporting);
        EG(error_reporting) = 0;
        raw = fc_stream_copy_and_close(stream, FC_IMAGE_MAX_BYTES + 1);
        EG(error_reporting) = er;
        if (EG(exception)) zend_clear_exception();
    } else {
        raw = fc_stream_copy_and_close(stream, FC_IMAGE_MAX_BYTES + 1);
    }
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
        /* MIME passed but dimensions couldn't be recovered: refuse
         * inputs the dim cap can't enforce. */
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

    if (fc_validate_png_chunks(bytes, n) != 0) {
        zend_string_release(raw);
        return -1;
    }

    out->bytes  = raw;
    out->mime   = mime;
    out->width  = src_w;
    out->height = src_h;
    return 0;
}

static fastchart_target_image_cache_entry *fastchart_target_image_cache_get(
	fastchart_target_t *t, const char *path)
{
	if (!t || !path || !*path) return NULL;
	for (int i = 0; i < t->image_cache_n; i++) {
        fastchart_target_image_cache_entry *entry = &t->image_cache[i];
        if (entry->path && strcmp(entry->path, path) == 0) return entry;
	}

	if (t->image_cache_n >= FASTCHART_TARGET_IMAGE_CACHE) return NULL;
	fastchart_target_image_cache_entry *entry =
        &t->image_cache[t->image_cache_n++];
	memset(entry, 0, sizeof(*entry));
	entry->path = estrdup(path);
	entry->loaded = fastchart_load_source_image(path, entry) == 0 ? 1 : -1;
	return entry;
}

int fastchart_target_image_dims(fastchart_target_t *t, const char *path,
                                int *width, int *height)
{
	fastchart_target_image_cache_entry *entry =
        fastchart_target_image_cache_get(t, path);
	if (!entry || entry->loaded != 1) return -1;
	if (width) *width = entry->width;
	if (height) *height = entry->height;
	return 0;
}

void fastchart_target_image(fastchart_target_t *t,
                             int x, int y, int w, int h,
                             const char *path)
{
    if (w <= 0 || h <= 0) return;

	fastchart_target_image_cache_entry *entry =
        fastchart_target_image_cache_get(t, path);
	if (!entry || entry->loaded != 1 || !entry->mime) return;
#ifdef HAVE_FASTCHART_PDF
	/* v1 PDF does not embed raster images. Keep the path cache shared so
     * icon dimension lookup retains its existing behavior. */
	if (t->kind == FASTCHART_TARGET_PDF) {
		if (entry->bytes) {
			zend_string_release(entry->bytes);
			entry->bytes = NULL;
		}
		return;
	}
#endif

	if (entry->svg_id == 0) {
		if (!entry->bytes) return;
		zend_string *base64 = php_base64_encode(
			(const unsigned char *)ZSTR_VAL(entry->bytes),
			ZSTR_LEN(entry->bytes));
		if (!base64) return;
		entry->svg_id = t->u.svg.next_image_id;
		entry->svg_width = w;
		entry->svg_height = h;
		if (t->u.svg.next_image_id < INT_MAX) t->u.svg.next_image_id++;
		zend_try {
			fc_svg_emit_image_def(t->u.svg.buf, t->u.svg.id_ns,
				entry->svg_id, w, h, entry->mime, ZSTR_VAL(base64));
		} zend_catch {
			zend_string_release(base64);
			zend_string_release(entry->bytes);
			entry->bytes = NULL;
			zend_bailout();
		} zend_end_try();
		zend_string_release(base64);
		zend_string_release(entry->bytes);
		entry->bytes = NULL;
	}
	fc_svg_emit_image_use(t->u.svg.buf, t->u.svg.id_ns, entry->svg_id,
		x, y, w, h, entry->svg_width, entry->svg_height);
}

static int fastchart_target_svg_gradient_id(fastchart_target_t *t,
                                             uint32_t from_rgb,
                                             uint32_t to_rgb, int dir,
                                             int *emit_definition)
{
	for (int i = 0; i < t->u.svg.gradient_cache_n; i++) {
        fastchart_target_gradient_cache_entry *entry =
            &t->u.svg.gradient_cache[i];
        if (entry->from_rgb == from_rgb && entry->to_rgb == to_rgb
            && entry->dir == dir) {
            *emit_definition = 0;
            return entry->id;
        }
	}

	int id = t->u.svg.next_grad_id;
	if (t->u.svg.next_grad_id < INT_MAX) t->u.svg.next_grad_id++;
	*emit_definition = 1;

	if (t->u.svg.gradient_cache_n < FASTCHART_TARGET_GRADIENT_CACHE) {
        fastchart_target_gradient_cache_entry *entry =
            &t->u.svg.gradient_cache[t->u.svg.gradient_cache_n++];
        entry->from_rgb = from_rgb;
        entry->to_rgb = to_rgb;
        entry->dir = dir;
        entry->id = id;
	}
	return id;
}

void fastchart_target_gradient_rect(fastchart_target_t *t,
                                     int x, int y, int w, int h,
                                     uint32_t from_rgb, uint32_t to_rgb,
                                     int dir)
{
    if (w <= 0 || h <= 0) return;
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_gradient_rect(t->u.pdf.state, x, y, w, h,
                                   from_rgb, to_rgb, dir);
        return;
    }
#endif
	int emit_definition;
	int id = fastchart_target_svg_gradient_id(t, from_rgb, to_rgb, dir,
                                               &emit_definition);
	if (emit_definition) {
        fc_svg_emit_gradient_rect(t->u.svg.buf, t->u.svg.id_ns, id,
                                   x, y, w, h, from_rgb, to_rgb, dir);
	} else {
        fc_svg_emit_gradient_rect_ref(t->u.svg.buf, t->u.svg.id_ns, id,
                                       x, y, w, h);
	}
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
    FASTCHART_SPLIT_POINTS(pts, n, xs, ys);
#ifdef HAVE_FASTCHART_PDF
    if (t->kind == FASTCHART_TARGET_PDF) {
        fc_pdf_emit_gradient_polygon(t->u.pdf.state, xs, ys, n,
                                      from_rgb, to_rgb, dir);
        if (n > 256) { efree(xs); efree(ys); }
        return;
    }
#endif
	int emit_definition;
	int id = fastchart_target_svg_gradient_id(t, from_rgb, to_rgb, dir,
                                               &emit_definition);
	if (emit_definition) {
        fc_svg_emit_gradient_polygon(t->u.svg.buf, t->u.svg.id_ns, id,
                                      xs, ys, n, from_rgb, to_rgb, dir);
	} else {
        fc_svg_emit_gradient_polygon_ref(t->u.svg.buf, t->u.svg.id_ns, id,
                                          xs, ys, n);
	}
    if (n > 256) { efree(xs); efree(ys); }
}
