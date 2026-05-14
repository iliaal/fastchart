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

  Render target abstraction. v1.0 has one backend — SVG into a
  smart_str. Raster outputs (PNG/JPG/WebP) are produced by handing the
  finished SVG to plutovg via fastchart_rasterize_svg() and then to
  libpng / libjpeg-turbo / libwebp via fastchart_encoder.c.

  The 28 high-level helpers in fastchart_axis.c, the 3 text helpers in
  fastchart_text.c, and the palette take a fastchart_target_t*. Color
  allocation goes through fastchart_target_color(t, r, g, b, a) which
  returns an opaque int handle (0..n_colors-1).
*/

#ifndef FASTCHART_TARGET_H
#define FASTCHART_TARGET_H

#include "php.h"
#include "Zend/zend_smart_str.h"
#include <stdint.h>

/* Retained for source-compat with chart bodies that reference the
 * enum even though only SVG is now valid. */
#define FASTCHART_TARGET_SVG  1

#define FASTCHART_TARGET_MAX_COLORS  512
#define FASTCHART_TARGET_CLIP_DEPTH  8
#define FASTCHART_TARGET_FONT_CACHE  4

/* Dash patterns. The SVG backend translates to stroke-dasharray;
 * the values are kept identical to v0.x for source compat. */
#define FASTCHART_DASH_SOLID   0
#define FASTCHART_DASH_DASHED  1
#define FASTCHART_DASH_DOTTED  2

#define FASTCHART_TARGET_ALIGN_LEFT    0
#define FASTCHART_TARGET_ALIGN_CENTER  1
#define FASTCHART_TARGET_ALIGN_RIGHT   2

/* SVG text emission mode. */
#define FASTCHART_SVG_TEXT_NATIVE  0
#define FASTCHART_SVG_TEXT_PATHS   1

/* Replacement for gd's gdPoint. Same layout (two ints) so existing
 * point-array consumers don't need adjustment. */
typedef struct fastchart_point {
    int x;
    int y;
} fastchart_point_t;

/* Source-compat alias so the existing chart-body code that uses
 * `gdPoint` continues to compile without sweeping renames. The real
 * libgd `gdPoint` is no longer available. */
typedef fastchart_point_t gdPoint;

typedef struct {
    const char *path;
    char family[64];
} fastchart_target_font_cache_entry;

typedef struct fastchart_target {
    int kind;
    /* SVG backend state. `u.svg.X` access kept for source-compat with
     * the dual-backend `t->u.svg` pattern in target.c / text.c — once
     * a third backend lands we can revisit the union. */
    union {
        struct {
            smart_str *buf;
            int width;
            int height;
            int dpi;
            int next_clip_id;
            int text_mode;
        } svg;
    } u;

    /* Shared color table. handle = index. */
    uint32_t color_rgba[FASTCHART_TARGET_MAX_COLORS];  /* 0xAARRGGBB */
    int n_colors;

    /* SVG clip stack (active clipPath ids, top = current). Sized for
     * the deepest nesting any chart family uses (currently 2). */
    int clip_stack[FASTCHART_TARGET_CLIP_DEPTH];
    int clip_depth;

    /* Per-target font-family cache. FT_New_Face is microseconds per
     * call but adds up over a render with many text emits. */
    fastchart_target_font_cache_entry font_cache[FASTCHART_TARGET_FONT_CACHE];
    int font_cache_n;
} fastchart_target_t;

/* Initialise as an SVG-backed target writing into `buf`. width/height
 * are the logical viewport dimensions. text_mode is one of
 * FASTCHART_SVG_TEXT_NATIVE / FASTCHART_SVG_TEXT_PATHS. */
void fastchart_target_from_svg(fastchart_target_t *t, smart_str *buf,
                                int width, int height, int dpi,
                                int text_mode);

/* Allocate a color handle for (r,g,b,a). 0..255 each; a=255 is opaque.
 * Returns handle index, or -1 if the per-target color table is full. */
int fastchart_target_color(fastchart_target_t *t, int r, int g, int b, int a);

/* Packed 0xRRGGBB convenience (alpha implied 255). */
int fastchart_target_color_rgb(fastchart_target_t *t, int rgb);

/* Resolve a color handle to packed 0xAARRGGBB. */
uint32_t fastchart_target_color_to_rgba(fastchart_target_t *t, int handle);

void fastchart_target_get_dims(fastchart_target_t *t, int *w, int *h);
int  fastchart_target_get_dpi(fastchart_target_t *t);

/* Primitives. All take a color HANDLE (not an rgba). thickness >= 1;
 * fill is 0/1. dash is FASTCHART_DASH_*. */

void fastchart_target_line(fastchart_target_t *t,
                            int x0, int y0, int x1, int y1,
                            int color, int thickness, int dash);

void fastchart_target_rect(fastchart_target_t *t,
                            int x, int y, int w, int h,
                            int color, int fill, int thickness);

void fastchart_target_polygon(fastchart_target_t *t,
                               const fastchart_point_t *pts, int n,
                               int color, int fill, int thickness);

void fastchart_target_arc(fastchart_target_t *t,
                           int cx, int cy, int rx, int ry,
                           double start_deg, double end_deg,
                           int color, int fill, int thickness);

void fastchart_target_ellipse(fastchart_target_t *t,
                               int cx, int cy, int rx, int ry,
                               int color, int fill, int thickness);

void fastchart_target_text(fastchart_target_t *t,
                            int x, int y,
                            const char *font_path, double size_pt,
                            int color, double angle_deg, int align,
                            const char *text);

void fastchart_target_clip_push(fastchart_target_t *t,
                                 int x, int y, int w, int h);
void fastchart_target_clip_pop(fastchart_target_t *t);

/* Image blit: v1.0 stub. Background-image / icon compositing through
 * SVG <image href="data:..." /> emission is deferred to v1.1. */
void fastchart_target_image(fastchart_target_t *t,
                             int x, int y, int w, int h);

/* Resolve a font file path to a CSS-safe family name via FreeType.
 * Result is written into out (null-terminated). out_n must be >= 64.
 * On any failure falls back to "sans-serif". Cached per-target. */
void fastchart_target_resolve_font_family(fastchart_target_t *t,
                                           const char *font_path,
                                           char *out, size_t out_n);

#endif /* FASTCHART_TARGET_H */
