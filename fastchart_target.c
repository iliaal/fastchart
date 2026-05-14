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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "fastchart_target.h"
#include "fastchart_svg.h"

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

static FT_Library fc_ft_lib = NULL;
static int fc_ft_lib_init_failed = 0;

/* Lazily init the per-process FT library. We never tear it down —
 * FT_Done_FreeType in MSHUTDOWN would race with any in-flight render
 * on ZTS, and the leak at process exit is negligible. */
static FT_Library fc_get_ft_library(void)
{
    if (fc_ft_lib != NULL) return fc_ft_lib;
    if (fc_ft_lib_init_failed) return NULL;
    if (FT_Init_FreeType(&fc_ft_lib) != 0) {
        fc_ft_lib = NULL;
        fc_ft_lib_init_failed = 1;
        return NULL;
    }
    return fc_ft_lib;
}

static void copy_family_name(char *out, size_t out_n, const char *src)
{
    if (!src || !*src) {
        strncpy(out, "sans-serif", out_n - 1);
        out[out_n - 1] = '\0';
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
        strncpy(out, "sans-serif", out_n - 1);
        out[out_n - 1] = '\0';
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
            strncpy(out, e->family, out_n - 1);
            out[out_n - 1] = '\0';
            return;
        }
    }

    /* FT lookup. */
    char raw[128] = "";
    FT_Library lib = fc_get_ft_library();
    if (lib) {
        FT_Face face = NULL;
        if (FT_New_Face(lib, font_path, 0, &face) == 0 && face) {
            if (face->family_name) {
                strncpy(raw, face->family_name, sizeof(raw) - 1);
                raw[sizeof(raw) - 1] = '\0';
            }
            FT_Done_Face(face);
        }
    }
    copy_family_name(out, out_n, raw[0] ? raw : NULL);

    /* Cache (path,family). Capacity is FASTCHART_TARGET_FONT_CACHE;
     * past that we silently skip the cache (next call hits FT
     * again — rare, since charts use 1-4 fonts max). */
    if (t->font_cache_n < FASTCHART_TARGET_FONT_CACHE) {
        fastchart_target_font_cache_entry *e =
            &t->font_cache[t->font_cache_n++];
        e->path = font_path;
        strncpy(e->family, out, sizeof(e->family) - 1);
        e->family[sizeof(e->family) - 1] = '\0';
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

void fastchart_target_image(fastchart_target_t *t,
                             int x, int y, int w, int h)
{
    /* v1.0: no-op. SVG <image href="data:..."/> emission for
     * IconPlot / background-image / watermark compositing is v1.1
     * work. Renderers that need it land an inline placeholder rect
     * at the call site if they want visible feedback. */
    (void)t; (void)x; (void)y; (void)w; (void)h;
}
