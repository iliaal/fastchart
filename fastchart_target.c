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

#include <gd.h>
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

void fastchart_target_from_gd(fastchart_target_t *t, gdImagePtr im, int dpi)
{
    memset(t, 0, sizeof(*t));
    t->kind = FASTCHART_TARGET_GD;
    t->u.gd.im = im;
    t->u.gd.last_thickness = -1;
    t->u.gd.last_dash = -1;
    t->u.gd.dash_color = -1;
    t->u.gd.clip_saved = 0;
    (void)dpi;  /* DPI lives on the gdImage via gdImageSetResolution */
}

void fastchart_target_from_svg(fastchart_target_t *t, smart_str *buf,
                                int width, int height, int dpi)
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
     * The `dpi` parameter is accepted for signature uniformity with
     * the GD constructor but is intentionally ignored. */
    (void)dpi;
    t->u.svg.dpi = 96;
    t->u.svg.next_clip_id = 1;
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
    if (t->kind == FASTCHART_TARGET_GD) {
        /* libgd alpha is 0..127 where 0 is opaque and 127 is
         * transparent — inverse of the 0..255 a-channel convention
         * we use. Translate at the boundary. */
        int gd_alpha = (255 - a) >> 1;          /* 0..127 */
        if (gd_alpha < 0) gd_alpha = 0;
        if (gd_alpha > 127) gd_alpha = 127;
        t->color_gd_int[idx] = gdImageColorAllocateAlpha(
            t->u.gd.im, r, g, b, gd_alpha);
    } else {
        t->color_gd_int[idx] = -1;
    }
    return idx;
}

int fastchart_target_color_rgb(fastchart_target_t *t, int rgb)
{
    int r = (rgb >> 16) & 0xFF;
    int g = (rgb >>  8) & 0xFF;
    int b =  rgb        & 0xFF;
    return fastchart_target_color(t, r, g, b, 0xFF);
}

int fastchart_target_color_to_gd(fastchart_target_t *t, int handle)
{
    if (handle < 0 || handle >= t->n_colors) return -1;
    return t->color_gd_int[handle];
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
    if (t->kind == FASTCHART_TARGET_GD) {
        *w = gdImageSX(t->u.gd.im);
        *h = gdImageSY(t->u.gd.im);
    } else {
        *w = t->u.svg.width;
        *h = t->u.svg.height;
    }
}

int fastchart_target_get_dpi(fastchart_target_t *t)
{
    if (t->kind == FASTCHART_TARGET_GD) {
        int dpi = (int)gdImageResolutionX(t->u.gd.im);
        return dpi > 0 ? dpi : 96;
    }
    return t->u.svg.dpi;
}

/* ============================================================ *
 * GD-side: thickness / dash modal state                         *
 * ============================================================ */

/* libgd line-style arrays. Indices match FASTCHART_DASH_*. The arrays
 * are reset on each install so the dash always starts from the same
 * phase; libgd's internal style cursor isn't externally visible. */
static void gd_install_dash(fastchart_target_t *t, int dash, int color)
{
    if (dash == FASTCHART_DASH_SOLID) {
        if (t->u.gd.last_dash != FASTCHART_DASH_SOLID) {
            /* No off-switch in libgd for SetStyle; we just stop
             * passing gdStyled as the color. Track for symmetry. */
            t->u.gd.last_dash = FASTCHART_DASH_SOLID;
            t->u.gd.dash_color = -1;
        }
        return;
    }
    if (dash == t->u.gd.last_dash && color == t->u.gd.dash_color) return;

    if (dash == FASTCHART_DASH_DOTTED) {
        int style[3] = { color, gdTransparent, gdTransparent };
        gdImageSetStyle(t->u.gd.im, style, 3);
    } else {
        /* DASHED. 4 on / 3 off. */
        int style[7] = { color, color, color, color,
                         gdTransparent, gdTransparent, gdTransparent };
        gdImageSetStyle(t->u.gd.im, style, 7);
    }
    t->u.gd.last_dash = dash;
    t->u.gd.dash_color = color;
}

static void gd_set_thickness(fastchart_target_t *t, int thickness)
{
    if (thickness < 1) thickness = 1;
    if (thickness == t->u.gd.last_thickness) return;
    gdImageSetThickness(t->u.gd.im, thickness);
    t->u.gd.last_thickness = thickness;
}

/* ============================================================ *
 * Primitives                                                    *
 * ============================================================ */

void fastchart_target_line(fastchart_target_t *t,
                            int x0, int y0, int x1, int y1,
                            int color, int thickness, int dash)
{
    if (t->kind == FASTCHART_TARGET_GD) {
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return;
        gd_set_thickness(t, thickness);
        if (dash != FASTCHART_DASH_SOLID) {
            gd_install_dash(t, dash, gd_color);
            gdImageLine(t->u.gd.im, x0, y0, x1, y1, gdStyled);
        } else {
            gdImageLine(t->u.gd.im, x0, y0, x1, y1, gd_color);
        }
        return;
    }
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_line(t->u.svg.buf, x0, y0, x1, y1, rgba, thickness, dash);
}

void fastchart_target_rect(fastchart_target_t *t,
                            int x, int y, int w, int h,
                            int color, int fill, int thickness)
{
    if (t->kind == FASTCHART_TARGET_GD) {
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return;
        if (fill) {
            gdImageFilledRectangle(t->u.gd.im, x, y, x + w - 1, y + h - 1, gd_color);
        } else {
            gd_set_thickness(t, thickness);
            gdImageRectangle(t->u.gd.im, x, y, x + w - 1, y + h - 1, gd_color);
        }
        return;
    }
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_rect(t->u.svg.buf, x, y, w, h, rgba, fill, thickness);
}

void fastchart_target_polygon(fastchart_target_t *t,
                               const gdPoint *pts, int n,
                               int color, int fill, int thickness)
{
    if (n < 2) return;
    if (t->kind == FASTCHART_TARGET_GD) {
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return;
        if (fill) {
            gdImageFilledPolygon(t->u.gd.im, (gdPointPtr)pts, n, gd_color);
        } else {
            gd_set_thickness(t, thickness);
            gdImagePolygon(t->u.gd.im, (gdPointPtr)pts, n, gd_color);
        }
        return;
    }
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
    if (t->kind == FASTCHART_TARGET_GD) {
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return;
        int diam_w = rx * 2;
        int diam_h = ry * 2;
        int sd = (int)(start_deg + 0.5);
        int ed = (int)(end_deg   + 0.5);
        if (fill) {
            gdImageFilledArc(t->u.gd.im, cx, cy, diam_w, diam_h,
                             sd, ed, gd_color, gdPie);
        } else {
            gd_set_thickness(t, thickness);
            gdImageArc(t->u.gd.im, cx, cy, diam_w, diam_h,
                       sd, ed, gd_color);
        }
        return;
    }
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    fc_svg_emit_path_arc(t->u.svg.buf, cx, cy, rx, ry,
                          start_deg, end_deg, rgba, fill, thickness);
}

void fastchart_target_ellipse(fastchart_target_t *t,
                               int cx, int cy, int rx, int ry,
                               int color, int fill, int thickness)
{
    if (t->kind == FASTCHART_TARGET_GD) {
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return;
        if (fill) {
            gdImageFilledEllipse(t->u.gd.im, cx, cy, rx * 2, ry * 2, gd_color);
        } else {
            gd_set_thickness(t, thickness);
            gdImageEllipse(t->u.gd.im, cx, cy, rx * 2, ry * 2, gd_color);
        }
        return;
    }
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

    if (t->kind == FASTCHART_TARGET_GD) {
        /* The GD backend's text path stays inside fastchart_text.c —
         * it already handles alignment, multi-line, FT error reporting.
         * This entry exists so SVG callers have a uniform call shape,
         * but in practice chart families call fastchart_text_draw*
         * directly for GD. Forward via gdImageStringFTEx as a safety
         * net; thin wrapper, no alignment handling here. */
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return;
        int brect[8];
        double rad = angle_deg * M_PI / 180.0;
        /* Apply alignment by measuring then offsetting x. */
        int dx = 0;
        if (align != FASTCHART_TARGET_ALIGN_LEFT) {
            gdImageStringFTEx(NULL, brect, -gd_color,
                              (char *)font_path, size_pt, 0.0, 0, 0,
                              (char *)text, NULL);
            int w = brect[2] - brect[0];
            dx = (align == FASTCHART_TARGET_ALIGN_CENTER) ? -w / 2 : -w;
        }
        gdImageStringFTEx(t->u.gd.im, brect, -gd_color,
                          (char *)font_path, size_pt, rad,
                          x + dx, y, (char *)text, NULL);
        return;
    }

    /* SVG. size_pt -> size_px at 96 DPI baseline: px = pt * 4/3. */
    double size_px = size_pt * (4.0 / 3.0);
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    char family[64];
    fastchart_target_resolve_font_family(t, font_path, family, sizeof(family));
    /* FUTURE: setSvgTextMode(SVG_TEXT_PATHS) for path-embedded
     * glyphs. Today we emit a CSS `<text>` element with the family
     * resolved above; viewers without the named font fall through
     * to `sans-serif`. */
    fc_svg_emit_text(t->u.svg.buf, x, y, family, size_px,
                      rgba, angle_deg, align, text, strlen(text));
}

/* ============================================================ *
 * Clip                                                          *
 * ============================================================ */

void fastchart_target_clip_push(fastchart_target_t *t,
                                 int x, int y, int w, int h)
{
    if (t->kind == FASTCHART_TARGET_GD) {
        /* libgd has only one clip rect at a time. We save it on the
         * first push and restore on the matching last pop, supporting
         * a single level of nesting (which is all chart families use
         * today). Deeper nesting would need a real stack with
         * gdImageGetClip on every push; defer until needed. */
        if (t->clip_depth == 0) {
            gdImageGetClip(t->u.gd.im,
                           &t->u.gd.saved_clip_x0,
                           &t->u.gd.saved_clip_y0,
                           &t->u.gd.saved_clip_x1,
                           &t->u.gd.saved_clip_y1);
            t->u.gd.clip_saved = 1;
        }
        gdImageSetClip(t->u.gd.im, x, y, x + w - 1, y + h - 1);
        if (t->clip_depth < FASTCHART_TARGET_CLIP_DEPTH) {
            t->clip_stack[t->clip_depth++] = 1;
        }
        return;
    }
    int id = t->u.svg.next_clip_id++;
    if (t->clip_depth < FASTCHART_TARGET_CLIP_DEPTH) {
        t->clip_stack[t->clip_depth++] = id;
    }
    fc_svg_emit_clip_open(t->u.svg.buf, id, x, y, w, h);
}

void fastchart_target_clip_pop(fastchart_target_t *t)
{
    if (t->clip_depth > 0) t->clip_depth--;
    if (t->kind == FASTCHART_TARGET_GD) {
        if (t->clip_depth == 0 && t->u.gd.clip_saved) {
            gdImageSetClip(t->u.gd.im,
                           t->u.gd.saved_clip_x0,
                           t->u.gd.saved_clip_y0,
                           t->u.gd.saved_clip_x1,
                           t->u.gd.saved_clip_y1);
            t->u.gd.clip_saved = 0;
        }
        return;
    }
    fc_svg_emit_clip_close(t->u.svg.buf);
}

/* ============================================================ *
 * Image blit                                                    *
 * ============================================================ */

void fastchart_target_image(fastchart_target_t *t,
                             int x, int y, int w, int h,
                             gdImagePtr src)
{
    if (t->kind == FASTCHART_TARGET_GD) {
        if (!src) return;
        gdImageCopyResampled(t->u.gd.im, src,
                              x, y, 0, 0,
                              w, h,
                              gdImageSX(src), gdImageSY(src));
        return;
    }
    /* FUTURE: base64-encode src's PNG bytes via gdImagePngPtr and
     * emit <image href="data:image/png;base64,..."/>. For PR 1 we
     * emit a labeled placeholder so an IconPlot on an SVG render
     * is visually obvious during development without crashing. */
    smart_str *buf = t->u.svg.buf;
    smart_str_appends(buf,
        "<!-- IconPlot blit not yet supported in SVG -->\n");
    uint32_t placeholder = 0xFFCCCCCCu;
    fc_svg_emit_rect(buf, x, y, w, h, placeholder, 0, 1);
}
