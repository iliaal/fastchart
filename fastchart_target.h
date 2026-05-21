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

#include <ft2build.h>
#include FT_FREETYPE_H

/* Process-shared FreeType library handle. Lazy-init on first call;
 * fastchart_ft_library_shutdown() releases at MSHUTDOWN. All FT
 * consumers (font-family resolver in target.c, glyph-path emitter in
 * fastchart_svg.c, text-bbox measurer in fastchart_text.c) share one
 * library instead of paying FT_Init_FreeType per call. */
FT_Library fastchart_ft_library(void);
void fastchart_ft_library_shutdown(void);

/* Process-shared FT_Face cache, keyed by font_path. 4-slot LRU.
 * The same FT consumers above now skip FT_New_Face on a hit — opening
 * a face parses the entire font file (tables, charmaps, glyph index)
 * and dominates the per-label cost on dense labels.
 *
 * Callers MUST call FT_Set_Char_Size / FT_Set_Pixel_Sizes on the
 * returned face before glyph operations — face size is mutable
 * shared state. Callers MUST NOT call FT_Done_Face on the returned
 * face; the cache owns the lifetime and frees at MSHUTDOWN.
 *
 * Returns NULL on FT_Library init failure, FT_New_Face failure, or
 * an OOM on the path-key strdup. */
FT_Face fastchart_ft_face(const char *font_path);

/* Glyph outline cache. Process-shared LRU keyed by (face, pix_size,
 * codepoint). Each entry holds the glyph's advance + a decomposed
 * path command stream at pen_x=0, so subsequent renders of the same
 * codepoint at the same size skip both FT_Load_Glyph and
 * FT_Outline_Decompose. The cache automatically invalidates entries
 * whose owning face is evicted from the FT_Face cache, so dangling
 * face pointers cannot leak through.
 *
 * The forward struct decl is in php_fastchart.h (fc_glyph_cache_entry)
 * because the globals array is sized by FC_GLYPH_CACHE_N there. */
struct fc_glyph_cache_entry;
const struct fc_glyph_cache_entry *fastchart_glyph_cache_get(
    FT_Face face, uint16_t pix_size, uint32_t codepoint);

/* Inserts a new cache entry. Takes ownership of `ops_buf` and `pts_buf`
 * (they must be `malloc`'d or NULL). `n_ops == 0` is a valid entry —
 * whitespace glyphs have no contours but still cache an advance. */
void fastchart_glyph_cache_insert(FT_Face face, uint16_t pix_size,
                                   uint32_t codepoint, int32_t advance_x_64,
                                   char *ops_buf, uint16_t n_ops,
                                   float *pts_buf, uint16_t n_pts);

/* Resolve one codepoint via the glyph cache. On miss, runs the FT
 * load + outline-decompose + cache-insert pipeline and returns the
 * just-inserted entry. Returns NULL on FT failure. Caller MUST have
 * already pinned the face to pix_size via FT_Set_Pixel_Sizes. */
const struct fc_glyph_cache_entry *fastchart_resolve_glyph(
    FT_Face face, uint16_t pix_size, uint32_t codepoint);

/* Deferred text overlay (opt #7). When defer_text_paths is set on the
 * target, fastchart_target_text records PATHS-mode text here instead
 * of emitting glyph paths into the SVG smart_str. After plutosvg has
 * rasterised the (text-free) SVG, fastchart_apply_text_overlays draws
 * the recorded text directly onto the plutovg surface via cached
 * glyph paths — plutosvg sees a much smaller document with fewer
 * elements and no glyph d-strings to parse.
 *
 * font_path is borrowed (lives on the chart object); text is owned
 * (copied at record time because chart-side label buffers may not
 * survive to apply time). */
typedef struct fastchart_text_overlay {
    double      x_logical;
    double      y_logical;
    const char *font_path;
    double      size_px;
    uint32_t    rgba;          /* 0xAARRGGBB */
    double      angle_deg;
    int         align;
    char       *text;          /* malloc'd copy */
    size_t      text_len;
} fastchart_text_overlay_t;

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
            int next_grad_id;
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

    /* Deferred text overlays (opt #7). Populated by
     * fastchart_target_text when defer_text_paths is set; consumed by
     * fastchart_apply_text_overlays() at rasterize time. */
    fastchart_text_overlay_t *text_overlays;
    int n_text_overlays;
    int cap_text_overlays;
    int defer_text_paths;     /* 1 = record overlay; 0 = emit inline (default) */
} fastchart_target_t;

/* Initialise as an SVG-backed target writing into `buf`. width/height
 * are the logical viewport dimensions. text_mode is one of
 * FASTCHART_SVG_TEXT_NATIVE / FASTCHART_SVG_TEXT_PATHS. */
void fastchart_target_from_svg(fastchart_target_t *t, smart_str *buf,
                                int width, int height, int dpi,
                                int text_mode);

/* Enable deferred text overlay recording (opt #7). When set, PATHS-mode
 * text emits go to t->text_overlays instead of the SVG buffer. Must
 * be called before any text primitive runs. Only meaningful when
 * text_mode == FASTCHART_SVG_TEXT_PATHS; NATIVE-mode text still emits
 * inline. */
void fastchart_target_enable_text_defer(fastchart_target_t *t);

/* Frees heap state attached to the target (currently: text_overlays).
 * Safe to call on a target with no heap state. Resets the array
 * counters so the target is ready for reuse. */
void fastchart_target_release(fastchart_target_t *t);

/* Apply deferred text overlays to a plutovg surface that was just
 * rendered from the SVG. Takes the surface as void* so this header
 * doesn't have to include plutovg.h. logical_w/logical_h are the
 * SVG document's viewport dims (chart coords); the function derives
 * the logical->physical scale from the surface dims. Idempotent
 * no-op when n_text_overlays == 0. */
void fastchart_apply_text_overlays(void *plutovg_surface,
                                    int logical_w, int logical_h,
                                    const fastchart_text_overlay_t *overlays,
                                    int n_overlays);

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

/* Image blit: emit <image href="data:image/<mime>;base64,..."/> for
 * the file at `path`. The implementation opens the file once through
 * the PHP stream layer (which enforces open_basedir natively — no
 * TOCTOU window) and enforces the source-image byte and dimension
 * caps on the loaded buffer. If anything fails (missing file, too
 * large, unsupported format, open_basedir refusal), the call emits
 * nothing — background-image / icon callers fall through to their
 * solid-fill backup. PNG and JPEG only; WebP/GIF/AVIF source files
 * are silently skipped because plutosvg's data-URI loader handles
 * just those two. */
void fastchart_target_image(fastchart_target_t *t,
                             int x, int y, int w, int h,
                             const char *path);

/* Two-step source-image load + emit, for callers that need the
 * declared width/height to compute layout (e.g. icon aspect-ratio
 * scaling). Same single-open / open_basedir-honoring semantics as
 * fastchart_target_image. After a successful _load, the caller may
 * inspect buf.width / buf.height and call _emit at any target
 * coordinates, then _release the bytes. */
typedef struct {
    zend_string *bytes;
    const char  *mime;   /* "image/png" or "image/jpeg" */
    int width;
    int height;
} fastchart_image_buf_t;

int  fastchart_target_load_source_image(const char *path,
                                        fastchart_image_buf_t *out);
void fastchart_target_image_emit(fastchart_target_t *t,
                                 int x, int y, int w, int h,
                                 const fastchart_image_buf_t *buf);
void fastchart_target_image_release(fastchart_image_buf_t *buf);

/* Gradient fills. `dir` is 0 (vertical) or 1 (horizontal); from/to
 * are 0xRRGGBB with alpha implied 0xFF. The target allocates a
 * unique gradient id per call. */
void fastchart_target_gradient_rect(fastchart_target_t *t,
                                     int x, int y, int w, int h,
                                     uint32_t from_rgb, uint32_t to_rgb,
                                     int dir);

void fastchart_target_gradient_polygon(fastchart_target_t *t,
                                        const fastchart_point_t *pts,
                                        int n,
                                        uint32_t from_rgb, uint32_t to_rgb,
                                        int dir);

/* Resolve a font file path to a CSS-safe family name via FreeType.
 * Result is written into out (null-terminated). out_n must be >= 64.
 * On any failure falls back to "sans-serif". Cached per-target. */
void fastchart_target_resolve_font_family(fastchart_target_t *t,
                                           const char *font_path,
                                           char *out, size_t out_n);

#endif /* FASTCHART_TARGET_H */
