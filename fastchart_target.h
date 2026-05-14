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

  Render target abstraction. Two backends behind one API:
    - GD   (kind == FASTCHART_TARGET_GD):   wraps a gdImagePtr; primitive
           calls translate to gdImage* directly. The hot path for
           renderPng/Jpeg/Webp/Gif/Avif and draw(\GdImage).
    - SVG  (kind == FASTCHART_TARGET_SVG):  wraps a smart_str buffer;
           primitive calls translate to SVG element emission via
           fastchart_svg.h.

  The 28 high-level helpers in fastchart_axis.c, the 3 text helpers in
  fastchart_text.c, and the palette take a fastchart_target_t* instead
  of a gdImagePtr so chart rendering logic is single-source.

  Color handling: both backends route color allocation through
  fastchart_target_color(t, r, g, b, a) which returns an opaque int
  handle (0..n_colors-1). The GD backend additionally caches the
  gd-allocated int per (r,g,b,a) so handle -> gd_int is O(1). The SVG
  backend stores the rgba and formats it inline at each primitive.

  Why not a vtable: only two backends, primitives are leaf-level, and
  GD is the hot path. A `switch (t->kind)` per primitive is one
  predicted branch; a vtable adds an indirect call. Easy to switch if
  a third backend ever appears.
*/

#ifndef FASTCHART_TARGET_H
#define FASTCHART_TARGET_H

#include "php.h"
#include "Zend/zend_smart_str.h"
#include <gd.h>
#include <stdint.h>

#define FASTCHART_TARGET_GD   0
#define FASTCHART_TARGET_SVG  1

#define FASTCHART_TARGET_MAX_COLORS  512
#define FASTCHART_TARGET_CLIP_DEPTH  8
#define FASTCHART_TARGET_FONT_CACHE  4

/* Dash patterns. The exact stroke-dasharray translation for SVG and
 * the libgd style array for GD are fixed per-pattern in
 * fastchart_target.c. */
#define FASTCHART_DASH_SOLID   0
#define FASTCHART_DASH_DASHED  1
#define FASTCHART_DASH_DOTTED  2

/* Text alignment. The (x, y) anchor sits at the baseline; alignment
 * controls horizontal placement of the text relative to x. */
#define FASTCHART_TARGET_ALIGN_LEFT    0
#define FASTCHART_TARGET_ALIGN_CENTER  1
#define FASTCHART_TARGET_ALIGN_RIGHT   2

typedef struct {
    /* Cache hit on font_path equality (pointer-equality first, then
     * strcmp for safety since callers may pass owned copies). Holds
     * the resolved CSS-safe font-family string used by SVG <text>. */
    const char *path;
    char family[64];
} fastchart_target_font_cache_entry;

typedef struct fastchart_target {
    int kind;
    union {
        struct {
            gdImagePtr im;
            int last_thickness;   /* last gdImageSetThickness arg, -1 if uninit */
            int last_dash;        /* last dash pattern installed, -1 if uninit */
            int dash_color;       /* color used by gdImageSetStyle, for replay */
            int saved_clip_x0;    /* gdImage clip rect saved on the first push */
            int saved_clip_y0;
            int saved_clip_x1;
            int saved_clip_y1;
            int clip_saved;
        } gd;
        struct {
            smart_str *buf;
            int width;            /* logical viewport width  */
            int height;           /* logical viewport height */
            int dpi;
            int next_clip_id;     /* monotonic for unique <clipPath id> */
        } svg;
    } u;

    /* Shared color table. handle = index. */
    uint32_t color_rgba[FASTCHART_TARGET_MAX_COLORS];  /* 0xAARRGGBB */
    int color_gd_int[FASTCHART_TARGET_MAX_COLORS];     /* GD-allocated int per handle */
    int n_colors;

    /* SVG-only clip stack (active clipPath ids, top of stack is
     * current). The GD backend uses its own gdImageSetClip; this
     * stack is sized for the deepest nesting any chart family uses
     * (currently 2). */
    int clip_stack[FASTCHART_TARGET_CLIP_DEPTH];
    int clip_depth;

    /* Per-target font-family cache. FT_New_Face is microseconds per
     * call but per-text-emit it adds up. 4 slots is enough for any
     * real chart (title + axis + label + value = 4 distinct fonts). */
    fastchart_target_font_cache_entry font_cache[FASTCHART_TARGET_FONT_CACHE];
    int font_cache_n;
} fastchart_target_t;

/* Initialise as a GD-backed target wrapping `im`. `dpi` should match
 * what gdImageSetResolution was called with on `im` (the FT layout
 * helpers consult it). Zeroes color/clip/font state. */
void fastchart_target_from_gd(fastchart_target_t *t, gdImagePtr im, int dpi);

/* Initialise as an SVG-backed target writing into `buf`. width/height
 * are the logical viewport dimensions used for both the SVG width/
 * height attributes and the viewBox. dpi influences text layout but
 * does not scale the SVG viewport. */
void fastchart_target_from_svg(fastchart_target_t *t, smart_str *buf,
                                int width, int height, int dpi);

/* Allocate a color handle for (r,g,b,a). 0..255 each; a=255 is opaque.
 * Returns handle index, or -1 if the per-target color table is full
 * (rare; FASTCHART_TARGET_MAX_COLORS is 512). For GD backend, also
 * runs gdImageColorAllocateAlpha so the gd-int is ready. */
int fastchart_target_color(fastchart_target_t *t, int r, int g, int b, int a);

/* Same as _color() but takes a packed 0xRRGGBB int (alpha implied 255).
 * Common convenience for fastchart's existing palette callsites. */
int fastchart_target_color_rgb(fastchart_target_t *t, int rgb);

/* Resolve a color handle back to the gd-int suitable for raw
 * gdImage* calls. Returns -1 on invalid handle. The few callsites in
 * chart families that still call gdImage* directly (during the Phase
 * 2 / 3 migration) use this to bridge. */
int fastchart_target_color_to_gd(fastchart_target_t *t, int handle);

/* Resolve a color handle to packed 0xAARRGGBB. Used by the SVG
 * backend; rarely needed externally. */
uint32_t fastchart_target_color_to_rgba(fastchart_target_t *t, int handle);

void fastchart_target_get_dims(fastchart_target_t *t, int *w, int *h);
int  fastchart_target_get_dpi(fastchart_target_t *t);

/* Primitives. All take a color HANDLE (not a gd-int, not an rgba).
 * thickness >= 1; fill is 0/1. dash is FASTCHART_DASH_*. */

void fastchart_target_line(fastchart_target_t *t,
                            int x0, int y0, int x1, int y1,
                            int color, int thickness, int dash);

void fastchart_target_rect(fastchart_target_t *t,
                            int x, int y, int w, int h,
                            int color, int fill, int thickness);

void fastchart_target_polygon(fastchart_target_t *t,
                               const gdPoint *pts, int n,
                               int color, int fill, int thickness);

/* Pie wedge or open arc. start_deg, end_deg are degrees with 0° at
 * 3 o'clock and angles increasing clockwise (matches gdImageArc /
 * gdImageFilledArc). If fill is non-zero, the wedge runs from
 * (cx,cy) out to the start radius, around the arc, and back. */
void fastchart_target_arc(fastchart_target_t *t,
                           int cx, int cy, int rx, int ry,
                           double start_deg, double end_deg,
                           int color, int fill, int thickness);

void fastchart_target_ellipse(fastchart_target_t *t,
                               int cx, int cy, int rx, int ry,
                               int color, int fill, int thickness);

/* (x, y) is the baseline anchor. `align` controls horizontal
 * placement relative to x. `angle_deg` rotates counter-clockwise
 * around the anchor (matches fastchart_text_draw_rotated). */
void fastchart_target_text(fastchart_target_t *t,
                            int x, int y,
                            const char *font_path, double size_pt,
                            int color, double angle_deg, int align,
                            const char *text);

void fastchart_target_clip_push(fastchart_target_t *t,
                                 int x, int y, int w, int h);
void fastchart_target_clip_pop(fastchart_target_t *t);

/* Blit a source gd image at (x,y) with width/height. For SVG output
 * in Phase 1 this is a placeholder (rect + comment); the
 * base64-encoded <image> implementation lands in a follow-up. */
void fastchart_target_image(fastchart_target_t *t,
                             int x, int y, int w, int h,
                             gdImagePtr src);

/* Resolve a font file path to a CSS-safe family name via FreeType.
 * Result is written into out (null-terminated). out_n must be >= 64.
 * On any failure falls back to "sans-serif". Cached per-target. */
void fastchart_target_resolve_font_family(fastchart_target_t *t,
                                           const char *font_path,
                                           char *out, size_t out_n);

#endif /* FASTCHART_TARGET_H */
