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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "php_fastchart.h"
#include "fastchart_axis.h"
#include "fastchart_palette.h"
#include "fastchart_text.h"
#include "fastchart_effects.h"
#include "fastchart_target.h"

/* Portable thread-safe gmtime + UTC mktime. POSIX provides
 * gmtime_r and timegm; MSVC ships gmtime_s (arg order swapped)
 * and _mkgmtime. Wrap so the rest of the file stays uniform. */
static inline void fc_gmtime(time_t t, struct tm *out)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    gmtime_s(out, &t);
#else
    gmtime_r(&t, out);
#endif
}

static inline time_t fc_timegm(struct tm *tm)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    return _mkgmtime(tm);
#else
    return timegm(tm);
#endif
}

/* All layout pixel constants are 96-DPI reference values. At higher
 * DPI the rendered glyph pixel size grows by dpi/96 (FreeType handles
 * that), so the margins / paddings / tick marks need to scale the same
 * way or labels collide with the plot area. The DPI(...) macro pulls
 * the scale from chart->dpi at use sites; chart_dpi_scale() returns it
 * as a double for size-in-points multipliers like `size * 1.2`.
 *
 * SVG output is DPI-invariant — vector strokes scale infinitely and
 * the SVG viewport stays at the logical setSize() value regardless
 * of setDpi(). So chart_dpi_scale() returns 1.0 for SVG-backed
 * targets; layout reservations match the 96-DPI baseline and the
 * SVG output for setDpi(200) is identical to setDpi(96). The DPI
 * knob still flows into PNG / JPEG / WebP / GIF / AVIF output where
 * a denser canvas is actually allocated. */
#define MARGIN_RIGHT_PAD       12
#define MARGIN_TOP_PAD          8
#define MARGIN_BOTTOM_PAD      10
#define MARGIN_LEFT_PAD         8
#define TICK_MARK_LEN_PX        4
#define Y_LABEL_PADDING_PX      6
#define X_LABEL_PADDING_PX      6
#define TITLE_PADDING_BELOW_PX 10

static inline double chart_dpi_scale(const fastchart_obj *chart,
                                      const fastchart_target_t *t)
{
    if (t && t->kind == FASTCHART_TARGET_SVG) return 1.0;
    return chart->dpi > 96 ? (double)chart->dpi / 96.0 : 1.0;
}

#define DPI_PX(chart, t, value) ((int)((value) * chart_dpi_scale((chart), (t)) + 0.5))
#define TICK_MARK_LEN(chart, t)        DPI_PX((chart), (t), TICK_MARK_LEN_PX)
#define Y_LABEL_PADDING(chart, t)      DPI_PX((chart), (t), Y_LABEL_PADDING_PX)
#define X_LABEL_PADDING(chart, t)      DPI_PX((chart), (t), X_LABEL_PADDING_PX)
#define TITLE_PADDING_BELOW(chart, t)  DPI_PX((chart), (t), TITLE_PADDING_BELOW_PX)

int fastchart_zval_to_double(zval *zv, double *out)
{
    switch (Z_TYPE_P(zv)) {
        case IS_DOUBLE:
            if (!isfinite(Z_DVAL_P(zv))) return -1;
            *out = Z_DVAL_P(zv);
            return 0;
        case IS_LONG:
            *out = (double)Z_LVAL_P(zv);
            return 0;
        default:
            return -1;
    }
}

/* zend_long is int64_t on 64-bit and int32_t on 32-bit (regardless of
 * how the platform sizes `long` — Windows LLP64 has 32-bit `long` with
 * 64-bit zend_long). Take and return zend_long so timestamps past
 * 2038 round-trip correctly on every PHP-supported platform. */
int fastchart_zval_to_long(zval *zv, zend_long *out)
{
    switch (Z_TYPE_P(zv)) {
        case IS_LONG:
            *out = Z_LVAL_P(zv);
            return 0;
        case IS_DOUBLE: {
            double d = Z_DVAL_P(zv);
            if (!isfinite(d) ||
                d < (double)ZEND_LONG_MIN || d > (double)ZEND_LONG_MAX) {
                return -1;
            }
            *out = (zend_long)d;
            return 0;
        }
        default:
            return -1;
    }
}

/* Roles map to three buckets: title (title_font_*), axis tick + axis
 * title + xaxis/yaxis/xtitle/ytitle (axis_font_*), and label/annotation
 * (label_font_*). Anything unset falls through to the chart-wide
 * font_path / FASTCHART_DEFAULT_FONT_SIZE.
 *
 * Setters validate the path against open_basedir at the time the user
 * calls them, but draw() runs arbitrarily later. open_basedir can be
 * narrowed via ini_set() between the setter and the render, so the
 * resolver re-checks before handing the path to FreeType (which
 * stat()s and open()s it). On a runtime narrowing, fall through to
 * the default font_path or NULL. */
static const char *check_font_path(const zend_string *path)
{
    if (!path) return NULL;
    const char *s = ZSTR_VAL(path);
    if (php_check_open_basedir_ex(s, /*warn=*/0) != 0) return NULL;
    return s;
}

const char *fastchart_resolve_font(fastchart_obj *chart,
                                   fastchart_font_role role)
{
    /* Per-draw cache: invalidated by fastchart_compute_layout at the
     * top of each renderer. First call after invalidation runs the
     * full resolution + open_basedir checks; subsequent calls within
     * the same render hit the cached path for free. */
    if (chart->font_cache_valid) {
        return chart->font_cache_path[(int)role];
    }
    const char *p;
    const char *resolved[FC_FONT_ROLE_COUNT];
    const char *fallback = check_font_path(chart->font_path);
    if ((p = check_font_path(chart->title_font_path)) == NULL) p = fallback;
    resolved[FC_FONT_TITLE] = p;
    if ((p = check_font_path(chart->axis_font_path)) == NULL) p = fallback;
    resolved[FC_FONT_AXIS] = p;
    if ((p = check_font_path(chart->label_font_path)) == NULL) p = fallback;
    resolved[FC_FONT_LABEL] = p;
    resolved[FC_FONT_ANNOTATION] = resolved[FC_FONT_LABEL];
    for (int i = 0; i < FC_FONT_ROLE_COUNT; i++) {
        chart->font_cache_path[i] = resolved[i];
    }
    chart->font_cache_valid = true;
    return chart->font_cache_path[(int)role];
}

double fastchart_resolve_font_size(fastchart_obj *chart,
                                   fastchart_font_role role,
                                   double base_default)
{
    double sz = base_default;
    switch (role) {
        case FC_FONT_TITLE:
            if (chart->title_font_size > 0) sz = chart->title_font_size;
            break;
        case FC_FONT_AXIS:
            if (chart->axis_font_size > 0) sz = chart->axis_font_size;
            break;
        case FC_FONT_LABEL: case FC_FONT_ANNOTATION:
            if (chart->label_font_size > 0) sz = chart->label_font_size;
            break;
        case FC_FONT_ROLE_COUNT:
            break;  /* sentinel; not a real role */
    }
    if (chart->thumbnail_mode) sz *= 0.6;
    return sz;
}

/* Catmull-Rom interpolation of one segment (p1 -> p2) at parameter
 * t in [0, 1], with the surrounding control points p0 (or p1 if
 * none) and p3 (or p2 if none). */
static void catmull_point(int p0x, int p0y, int p1x, int p1y,
                          int p2x, int p2y, int p3x, int p3y,
                          double t, int *ox, int *oy)
{
    double t2 = t * t;
    double t3 = t2 * t;
    double x = 0.5 * ((2 * p1x) +
                      (-p0x + p2x) * t +
                      (2 * p0x - 5 * p1x + 4 * p2x - p3x) * t2 +
                      (-p0x + 3 * p1x - 3 * p2x + p3x) * t3);
    double y = 0.5 * ((2 * p1y) +
                      (-p0y + p2y) * t +
                      (2 * p0y - 5 * p1y + 4 * p2y - p3y) * t2 +
                      (-p0y + 3 * p1y - 3 * p2y + p3y) * t3);
    *ox = (int)(x + 0.5);
    *oy = (int)(y + 0.5);
}

/* Emit one polyline segment.
 *
 * The fast path is GD-native AA: when `aa_gd_color >= 0` and the
 * target is GD-backed, we keep calling gdImageLine with gdAntiAliased
 * directly so the libgd anti-aliased line algorithm runs unchanged
 * (it's a fundamentally different rasteriser than the thick / dashed
 * path and the target abstraction can't replicate it without losing
 * fidelity). For every other case — solid SVG, solid thick GD,
 * styled GD — we go through fastchart_target_line so the dispatch
 * works for both backends. */
static inline void poly_seg(fastchart_target_t *t,
                            int x0, int y0, int x1, int y1,
                            int color_handle, int thickness, int dash,
                            int aa_gd_color)
{
    if (aa_gd_color >= 0 && t->kind == FASTCHART_TARGET_GD) {
        gdImageLine(t->u.gd.im, x0, y0, x1, y1, gdAntiAliased);
        return;
    }
    fastchart_target_line(t, x0, y0, x1, y1, color_handle, thickness, dash);
}

/* Walk the polyline path once. Factored out so fastchart_draw_polyline
 * can run two passes (thick non-AA underbody + thin AA spine) when
 * both weight and edge smoothness are wanted.
 *
 * color_handle / thickness / dash drive fastchart_target_line for the
 * general case. aa_gd_color is non-negative only on the AA spine pass
 * — when set, GD-backed targets short-circuit through gdImageLine with
 * gdAntiAliased to preserve libgd's native AA. */
static void polyline_pass(fastchart_target_t *t, fastchart_obj *chart,
                          const fastchart_pt *pts, int n,
                          int color_handle, int thickness, int dash,
                          int aa_gd_color)
{
    if (chart->line_interpolation == FASTCHART_INTERP_STEP_AFTER ||
        chart->line_interpolation == FASTCHART_INTERP_STEP_BEFORE) {
        /* Staircase: each segment is one horizontal + one vertical
         * line. STEP_AFTER carries the previous y forward to the next
         * x (the "Excel step" shape: change happens at the new x).
         * STEP_BEFORE jumps to the next y at the previous x and then
         * holds (change happens at the start of each segment). */
        bool after = (chart->line_interpolation == FASTCHART_INTERP_STEP_AFTER);
        int prev_x = 0, prev_y = 0;
        bool prev_valid = false;
        for (int i = 0; i < n; i++) {
            if (!pts[i].valid) { prev_valid = false; continue; }
            if (prev_valid) {
                int corner_y = after ? prev_y : pts[i].y;
                poly_seg(t, prev_x, prev_y, pts[i].x, corner_y,
                         color_handle, thickness, dash, aa_gd_color);
                poly_seg(t, pts[i].x, corner_y, pts[i].x, pts[i].y,
                         color_handle, thickness, dash, aa_gd_color);
            }
            prev_x = pts[i].x; prev_y = pts[i].y; prev_valid = true;
        }
    } else if (chart->line_interpolation != FASTCHART_INTERP_SMOOTH) {
        /* Linear: connect consecutive valid points. */
        int prev_x = 0, prev_y = 0;
        bool prev_valid = false;
        for (int i = 0; i < n; i++) {
            if (!pts[i].valid) { prev_valid = false; continue; }
            if (prev_valid) {
                poly_seg(t, prev_x, prev_y, pts[i].x, pts[i].y,
                         color_handle, thickness, dash, aa_gd_color);
            }
            prev_x = pts[i].x; prev_y = pts[i].y; prev_valid = true;
        }
    } else {
        /* Catmull-Rom with adaptive sub-segment count. Two adjacent
         * points 3 px apart don't need 10 sub-segments (9 of them
         * collapse to the same pixel); 200 px apart need more than
         * 10 to keep the curve smooth. Step roughly one sub-segment
         * per 4 px of Manhattan distance, clamped to [2, 20]. */
        for (int i = 0; i < n - 1; i++) {
            if (!pts[i].valid || !pts[i + 1].valid) continue;
            int p0i = (i > 0 && pts[i - 1].valid) ? i - 1 : i;
            int p3i = (i + 2 < n && pts[i + 2].valid) ? i + 2 : i + 1;
            int dx = pts[i + 1].x - pts[i].x;
            int dy = pts[i + 1].y - pts[i].y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            int subdiv = (dx + dy) / 4;
            if (subdiv < 2)  subdiv = 2;
            if (subdiv > 20) subdiv = 20;
            int prev_x = pts[i].x, prev_y = pts[i].y;
            for (int k = 1; k <= subdiv; k++) {
                double tt = (double)k / (double)subdiv;
                int x, y;
                catmull_point(pts[p0i].x, pts[p0i].y,
                              pts[i].x,   pts[i].y,
                              pts[i + 1].x, pts[i + 1].y,
                              pts[p3i].x, pts[p3i].y,
                              tt, &x, &y);
                poly_seg(t, prev_x, prev_y, x, y,
                         color_handle, thickness, dash, aa_gd_color);
                prev_x = x; prev_y = y;
            }
        }
    }
}

void fastchart_draw_polyline(fastchart_target_t *t, fastchart_obj *chart,
                             const fastchart_pt *pts, int n,
                             int color, int thickness, bool antialiased)
{
    if (n < 2) return;
    /* Line style and antialiasing are mutually exclusive in libgd
     * (no gdStyled+gdAntiAliased compound). When the user picked
     * dashed/dotted, drop AA so the dash pattern is honored. */
    bool styled = (chart->line_style != FASTCHART_LINE_SOLID);
    int dash = (chart->line_style == FASTCHART_LINE_DOTTED)
                   ? FASTCHART_DASH_DOTTED
             : (chart->line_style == FASTCHART_LINE_DASHED)
                   ? FASTCHART_DASH_DASHED
                   : FASTCHART_DASH_SOLID;

    if (styled) {
        polyline_pass(t, chart, pts, n, color, thickness, dash, -1);
    } else if (antialiased && thickness > 1
               && t->kind == FASTCHART_TARGET_GD) {
        /* Two-pass on GD: libgd's thick-line path uses a square brush
         * stamp which leaves chunky aliased edges on diagonal segments;
         * libgd's gdAntiAliased line is always 1px. Combine them —
         * thick non-AA underbody for weight, then a 1px AA spine on
         * top so the centerline reads as smooth. The eye anchors on
         * the spine; perceived edge crispness comes up noticeably on
         * stock MA lines and dense scatter overlays. SVG strokes are
         * AA by default, so the spine pass would only thin the line
         * visually — skip it for SVG. */
        polyline_pass(t, chart, pts, n, color, thickness,
                      FASTCHART_DASH_SOLID, -1);
        int gd_c = fastchart_target_color_to_gd(t, color);
        if (gd_c >= 0) {
            gdImageSetAntiAliased(t->u.gd.im, gd_c);
            polyline_pass(t, chart, pts, n, color, 1,
                          FASTCHART_DASH_SOLID, gd_c);
        }
    } else if (antialiased && t->kind == FASTCHART_TARGET_GD) {
        /* Single AA pass on GD at thickness 1. */
        int gd_c = fastchart_target_color_to_gd(t, color);
        if (gd_c >= 0) {
            gdImageSetAntiAliased(t->u.gd.im, gd_c);
            polyline_pass(t, chart, pts, n, color, 1,
                          FASTCHART_DASH_SOLID, gd_c);
        } else {
            polyline_pass(t, chart, pts, n, color, thickness,
                          FASTCHART_DASH_SOLID, -1);
        }
    } else {
        /* Non-AA solid (or SVG, which is implicitly AA). */
        polyline_pass(t, chart, pts, n, color, thickness,
                      FASTCHART_DASH_SOLID, -1);
    }
}

void fastchart_draw_marker(fastchart_target_t *t, int x, int y,
                           int style, int size, int color)
{
    if (style == FASTCHART_MARKER_NONE || size <= 0) return;
    int half = size / 2;
    if (half < 1) half = 1;

    switch (style) {
        case FASTCHART_MARKER_CIRCLE:
            fastchart_target_ellipse(t, x, y, size / 2, size / 2,
                                     color, 1, 0);
            /* AA outline on top to soften the pixel edge of the fill.
             * GD-only: SVG strokes are already AA so an extra outline
             * just doubles the perimeter weight. */
            if (size >= 4 && t->kind == FASTCHART_TARGET_GD) {
                int gd_c = fastchart_target_color_to_gd(t, color);
                if (gd_c >= 0) {
                    gdImageSetAntiAliased(t->u.gd.im, gd_c);
                    gdImageEllipse(t->u.gd.im, x, y, size, size,
                                   gdAntiAliased);
                }
            }
            break;
        case FASTCHART_MARKER_SQUARE:
            fastchart_target_rect(t, x - half, y - half,
                                  (x + half) - (x - half) + 1,
                                  (y + half) - (y - half) + 1,
                                  color, 1, 0);
            break;
        case FASTCHART_MARKER_DIAMOND: {
            gdPoint pts[4] = {
                { x,        y - half },
                { x + half, y        },
                { x,        y + half },
                { x - half, y        },
            };
            fastchart_target_polygon(t, pts, 4, color, 1, 0);
            if (size >= 4 && t->kind == FASTCHART_TARGET_GD) {
                int gd_c = fastchart_target_color_to_gd(t, color);
                if (gd_c >= 0) {
                    gdImageSetAntiAliased(t->u.gd.im, gd_c);
                    gdImagePolygon(t->u.gd.im, pts, 4, gdAntiAliased);
                }
            }
            break;
        }
        case FASTCHART_MARKER_CROSS:
            fastchart_target_line(t, x - half, y - half, x + half, y + half,
                                  color, 2, FASTCHART_DASH_SOLID);
            fastchart_target_line(t, x - half, y + half, x + half, y - half,
                                  color, 2, FASTCHART_DASH_SOLID);
            break;
        case FASTCHART_MARKER_PLUS:
            fastchart_target_line(t, x - half, y, x + half, y,
                                  color, 2, FASTCHART_DASH_SOLID);
            fastchart_target_line(t, x, y - half, x, y + half,
                                  color, 2, FASTCHART_DASH_SOLID);
            break;
        default:
            break;
    }
}

void fastchart_begin_render(fastchart_obj *chart, fastchart_target_t *t)
{
    /* Single chokepoint for per-draw cache invalidation. Any ini_set
     * narrowing of open_basedir between two draws of the same chart
     * object must NOT let the prior draw's resolved font path leak
     * past — fastchart_resolve_font re-runs check_font_path() on the
     * first call after this point. The shadow color is re-allocated
     * against the new gdImage `im` on the first shadow draw.
     *
     * Renderers that call fastchart_compute_layout get this for free
     * (compute_layout calls us). Non-layout renderers (gauge, radar,
     * polar, surface, contour) must call this directly at draw entry,
     * before resolving any fonts or palette colors. */
    chart->font_cache_valid = false;
    chart->shadow_color_valid = false;

    /* Stamp the canvas resolution. Two consequences flow from this:
     *   - PNG pHYs / JPEG density metadata reports the right physical
     *     size when the image is embedded in print or HiDPI contexts.
     *   - fastchart_text_draw reads gdImageResolutionX(im) and feeds
     *     it to FreeType via gdImageStringFTEx + gdFTEX_RESOLUTION,
     *     which controls glyph hinting. Higher DPI -> finer hinting.
     * Without this call libgd defaults the resolution to 96 (image
     * meta) and FreeType defaults to 100 (hinting), leaving us with
     * inconsistent state. Setting both via gdImageSetResolution keeps
     * them aligned. SVG-backed targets carry DPI on the target itself
     * and have no canvas resolution to stamp. */
    if (chart->dpi > 0 && t->kind == FASTCHART_TARGET_GD) {
        gdImageSetResolution(t->u.gd.im, (unsigned int)chart->dpi,
                              (unsigned int)chart->dpi);
    }
}

void fastchart_compute_layout(fastchart_obj *chart, fastchart_target_t *t,
                              int has_y_axis, int has_x_axis,
                              const char *const *cat_y_labels,
                              int n_cat_y_labels,
                              fastchart_rect *out_plot)
{
    fastchart_begin_render(chart, t);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);

    /* Hard plot rectangle bypass: when setPlotRect() was called,
     * skip auto-layout entirely and clamp to canvas bounds. */
    if (chart->has_plot_rect) {
        out_plot->x0 = chart->plot_x0 < 0 ? 0 : chart->plot_x0;
        out_plot->y0 = chart->plot_y0 < 0 ? 0 : chart->plot_y0;
        out_plot->x1 = chart->plot_x1 > W - 1 ? W - 1 : chart->plot_x1;
        out_plot->y1 = chart->plot_y1 > H - 1 ? H - 1 : chart->plot_y1;
        if (out_plot->x1 < out_plot->x0 + 10) out_plot->x1 = out_plot->x0 + 10;
        if (out_plot->y1 < out_plot->y0 + 10) out_plot->y1 = out_plot->y0 + 10;
        return;
    }

    /* Scale hardcoded pixel constants with DPI so layout doesn't get
     * cramped at high-DPI canvases. At 96 DPI scale=1.0 (no change);
     * at 200 DPI scale~2.08 — fonts already grow ×scale via FreeType,
     * so margins / tick lengths / paddings need to grow proportionally
     * or labels overflow the canvas. */
    double dpi_scale = chart_dpi_scale(chart, t);
    int tick_mark_len = TICK_MARK_LEN(chart, t);
    int y_label_pad   = Y_LABEL_PADDING(chart, t);
    int x_label_pad   = X_LABEL_PADDING(chart, t);
    int title_pad     = TITLE_PADDING_BELOW(chart, t);

    int top    = DPI_PX(chart, t, MARGIN_TOP_PAD);
    int bottom = DPI_PX(chart, t, MARGIN_BOTTOM_PAD);
    int left   = DPI_PX(chart, t, MARGIN_LEFT_PAD);
    int right  = DPI_PX(chart, t, MARGIN_RIGHT_PAD);

    /* Each role is resolved through fastchart_resolve_font so the
     * draw-time open_basedir re-check fires before the path reaches
     * FreeType. Layout runs once per draw(), so the per-role
     * resolution cost is bounded. */
    const char *title_font = fastchart_resolve_font(chart, FC_FONT_TITLE);
    const char *axis_font  = fastchart_resolve_font(chart, FC_FONT_AXIS);
    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;

    /* Thumbnail mode suppresses every label / title / tick text, so
     * reserving margin for them just shrinks the plot for no reason.
     * Skip all label-driven reservations and let the plot fill the
     * canvas minus the small base padding constants. */
    bool labels_drawn = !chart->thumbnail_mode;

    /* Title: measure ascender height + a bit of padding. The "999999"
     * probe used for axis labels is cached once below. */
    int probe_w = 0, probe_h = 0;
    int probe_ok = (axis_font && fastchart_text_measure(t, axis_font, size, "999999",
                                                        &probe_w, &probe_h, NULL, 0) == 0);

    if (labels_drawn && chart->title && ZSTR_LEN(chart->title) > 0 && title_font) {
        int th;
        if (fastchart_text_measure(t, title_font, size * 1.4, ZSTR_VAL(chart->title),
                                   NULL, &th, NULL, 0) == 0) {
            top += th + title_pad;
        }
    }

    /* Y-axis: reserve enough room for the widest tick label.
     *
     * For numeric Y axes the actual label width is data-dependent;
     * we pick a conservative "999999" sample so layout is stable
     * across data ranges.
     *
     * For categorical Y axes (horizontal-bar) the labels can be
     * arbitrarily long ("/api/v2/exports", etc.) — measure the
     * widest one so it doesn't get clipped at the canvas edge.
     * Falls back to the numeric probe if none of the labels can be
     * measured. */
    if (labels_drawn && has_y_axis && probe_ok) {
        int y_label_w = probe_w;
        if (cat_y_labels && n_cat_y_labels > 0 && axis_font) {
            int widest = 0;
            for (int i = 0; i < n_cat_y_labels; i++) {
                if (!cat_y_labels[i]) continue;
                int w = 0;
                if (fastchart_text_measure(t, axis_font, size, cat_y_labels[i],
                                           &w, NULL, NULL, 0) == 0 && w > widest) {
                    widest = w;
                }
            }
            if (widest > y_label_w) y_label_w = widest;
        }
        left += y_label_w + tick_mark_len + y_label_pad;
    }

    /* Y-axis title: rotated 90deg on the left of the y-axis labels.
     * The title's height becomes its visible width after rotation. */
    if (labels_drawn && has_y_axis && chart->y_axis_title && axis_font) {
        int th;
        if (fastchart_text_measure(t, axis_font, size * 1.1, ZSTR_VAL(chart->y_axis_title),
                                   NULL, &th, NULL, 0) == 0) {
            left += th + (int)(8 * dpi_scale + 0.5);
        }
    }

    /* Secondary Y axis: mirror the left-side reservation on the
     * right edge using the cached probe. */
    if (labels_drawn && has_y_axis && chart->secondary_y && probe_ok) {
        right += probe_w + tick_mark_len + y_label_pad;
    } else if (labels_drawn && has_y_axis && has_x_axis && probe_ok
               && left > right * 2) {
        /* Anchor right margin at left / 2 (visual 2:1 ratio of left to
         * right). The bare MARGIN_RIGHT_PAD next to a full Y-axis
         * label reservation on the left read as visibly off-center;
         * this also gives the rightmost X-axis label half-extent
         * room to extend past plot.x1 without clipping the canvas. */
        right = left / 2;
    }

    /* X-axis labels. Horizontal labels reserve one line; rotated
     * labels reserve roughly the label width as height. The
     * "999999" numeric probe is fine for un-rotated numeric ticks
     * but understates category labels like "Jan 2025" when the
     * caller set setCategoryLabels(). For rotated layouts the
     * projected vertical extent is what bounds the bottom margin,
     * so when category labels are present we measure the widest
     * and use that instead of the probe. Matches the Y-axis logic
     * above for `cat_y_labels`. */
    if (labels_drawn && has_x_axis && probe_ok) {
        int x_label_w = probe_w;
        if (chart->category_labels && chart->n_category_labels > 0 && axis_font) {
            int widest = 0;
            for (int i = 0; i < chart->n_category_labels; i++) {
                const char *lbl = chart->category_labels[i];
                if (!lbl) continue;
                int w = 0;
                if (fastchart_text_measure(t, axis_font, size, lbl,
                                           &w, NULL, NULL, 0) == 0 && w > widest) {
                    widest = w;
                }
            }
            if (widest > x_label_w) x_label_w = widest;
        }
        int needed;
        if (chart->x_axis_label_angle == 90) {
            needed = x_label_w + tick_mark_len + x_label_pad;
        } else if (chart->x_axis_label_angle == 45) {
            /* The rotated label's drawing code (line ~1747) places the
             * baseline-anchor at plot.y1 + tick + 0.707w + 4 so the
             * RIGHT-END (top of rotated text) clears the plot rect.
             * From that anchor the text extends UP-LEFT, so the
             * LEFT-END (bottom of rotated text) sits another 0.707w
             * below — total descent below plot.y1 ≈ 1.414w + tick + 4.
             * Reserving only 0.707w (the half-projection) clipped
             * labels by half their projected height. Mirror the full
             * geometry so the canvas bottom fits the whole rotated
             * span. */
            needed = (int)((double)x_label_w * 1.414) + tick_mark_len + x_label_pad;
        } else {
            needed = probe_h + tick_mark_len + x_label_pad;
        }
        bottom += needed;
    }

    /* X-axis title: an extra line below the labels. */
    if (labels_drawn && has_x_axis && chart->x_axis_title && axis_font) {
        int th;
        if (fastchart_text_measure(t, axis_font, size * 1.1, ZSTR_VAL(chart->x_axis_title),
                                   NULL, &th, NULL, 0) == 0) {
            bottom += th + (int)(6 * dpi_scale + 0.5);
        }
    }

    out_plot->x0 = left;
    out_plot->y0 = top;
    out_plot->x1 = W - right - 1;
    out_plot->y1 = H - bottom - 1;

    if (out_plot->x1 < out_plot->x0 + 10) out_plot->x1 = out_plot->x0 + 10;
    if (out_plot->y1 < out_plot->y0 + 10) out_plot->y1 = out_plot->y0 + 10;
}

void fastchart_value_range_compute(double dmin, double dmax,
                                   int target_ticks,
                                   fastchart_value_range *out)
{
    if (target_ticks < 2) target_ticks = 5;
    if (target_ticks > FASTCHART_MAX_TICKS) target_ticks = FASTCHART_MAX_TICKS;

    /* Degenerate / empty data: bracket around 0..1 so the axis still
     * draws something sensible instead of dividing by zero downstream. */
    if (!isfinite(dmin) || !isfinite(dmax) || dmin > dmax) {
        dmin = 0.0;
        dmax = 1.0;
    } else if (dmax - dmin < 1e-12) {
        if (fabs(dmin) < 1e-12) {
            dmax = 1.0;
        } else {
            double pad = fabs(dmin) * 0.1;
            dmin -= pad;
            dmax += pad;
        }
    }

    /* Pick a "nice" tick step using the 1/2/5 × 10^N progression.
     * Bail to a [0, 1] fallback if any intermediate goes non-finite —
     * extreme but finite input magnitudes (e.g. dmax - dmin near
     * DBL_MAX) can push log10 / pow / division through Inf and the
     * later (int)cast in the renderer would be UB. */
    double range = dmax - dmin;
    double rough_step = range / (double)(target_ticks - 1);
    double mag = isfinite(rough_step) && rough_step > 0
        ? pow(10.0, floor(log10(rough_step))) : 1.0;
    double norm = (isfinite(mag) && mag > 0) ? rough_step / mag : 1.0;
    double step;
    if      (norm < 1.5)  step = 1.0  * mag;
    else if (norm < 3.0)  step = 2.0  * mag;
    else if (norm < 4.0)  step = 2.5  * mag;
    else if (norm < 7.0)  step = 5.0  * mag;
    else                  step = 10.0 * mag;

    double nice_min = floor(dmin / step) * step;
    double nice_max = ceil(dmax / step) * step;
    if (!isfinite(nice_min) || !isfinite(nice_max) || !isfinite(step) || step <= 0) {
        nice_min = 0.0; nice_max = 1.0; step = 1.0;
    }

    out->min = nice_min;
    out->max = nice_max;
    out->tick_step = step;
    out->log_scale = 0;
    out->n_ticks = 0;

    for (double v = nice_min;
         v <= nice_max + step * 0.5 && out->n_ticks < FASTCHART_MAX_TICKS;
         v += step) {
        out->ticks[out->n_ticks++] = v;
    }
    if (out->n_ticks < 2) {
        out->ticks[0] = nice_min;
        out->ticks[1] = nice_max;
        out->n_ticks = 2;
    }
}

void fastchart_value_range_apply_override(const fastchart_obj *chart,
                                          fastchart_value_range *out)
{
    if (!chart->has_y_min && !chart->has_y_max && !chart->has_y_interval) return;
    if (out->log_scale) return;  /* log-scale ignores forced bounds */

    double mn = chart->has_y_min ? chart->y_min : out->min;
    double mx = chart->has_y_max ? chart->y_max : out->max;
    if (mx <= mn) return;  /* malformed override; leave the auto range */

    out->min = mn;
    out->max = mx;

    if (chart->has_y_interval) {
        double step = chart->y_interval;
        out->tick_step = step;
        out->n_ticks = 0;
        for (double v = mn;
             v <= mx + step * 0.5 && out->n_ticks < FASTCHART_MAX_TICKS;
             v += step) {
            out->ticks[out->n_ticks++] = v;
        }
        if (out->n_ticks < 2) {
            out->ticks[0] = mn;
            out->ticks[1] = mx;
            out->n_ticks = 2;
        }
    } else {
        /* Re-run the nice-tick generator on the forced bounds. */
        fastchart_value_range tmp;
        fastchart_value_range_compute(mn, mx, 6, &tmp);
        out->tick_step = tmp.tick_step;
        out->n_ticks = tmp.n_ticks;
        for (int i = 0; i < tmp.n_ticks; i++) out->ticks[i] = tmp.ticks[i];
        /* But preserve the user's literal min/max (not the rounded
         * "nice" version), since the user explicitly asked. */
        out->min = mn;
        out->max = mx;
    }
}

int fastchart_value_range_compute_log(double dmin, double dmax,
                                       fastchart_value_range *out)
{
    if (!isfinite(dmin) || !isfinite(dmax) || dmin <= 0.0 || dmax <= 0.0) {
        return -1;
    }
    if (dmax < dmin) {
        double t = dmin; dmin = dmax; dmax = t;
    }
    double lo = floor(log10(dmin));
    double hi = ceil(log10(dmax));
    if (hi <= lo) hi = lo + 1.0;

    out->min = pow(10.0, lo);
    out->max = pow(10.0, hi);
    out->tick_step = 1.0;        /* one decade per tick */
    out->log_scale = 1;
    out->n_ticks = 0;

    for (double e = lo;
         e <= hi + 0.5 && out->n_ticks < FASTCHART_MAX_TICKS;
         e += 1.0) {
        out->ticks[out->n_ticks++] = pow(10.0, e);
    }
    if (out->n_ticks < 2) {
        out->ticks[0] = out->min;
        out->ticks[1] = out->max;
        out->n_ticks = 2;
    }
    return 0;
}

int fastchart_y_to_pixel(double y,
                         const fastchart_value_range *range,
                         const fastchart_rect *plot)
{
    double frac;
    if (range->log_scale) {
        /* Clamp y to the positive range before taking the log so a
         * single bad point doesn't drop the whole series. The
         * strict-mode check happens upstream in each chart's
         * extraction; here we just keep the math defined. */
        if (y <= 0.0) return plot->y1;
        double l_min = log10(range->min);
        double l_max = log10(range->max);
        double l_span = l_max - l_min;
        if (l_span < 1e-12) return plot->y1;
        frac = (log10(y) - l_min) / l_span;
    } else {
        double span = range->max - range->min;
        if (span < 1e-12) return plot->y1;
        frac = (y - range->min) / span;
    }
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    int h = plot->y1 - plot->y0;
    return plot->y1 - (int)(frac * (double)h + 0.5);
}

int fastchart_x_to_pixel(double x,
                         const fastchart_value_range *range,
                         const fastchart_rect *plot)
{
    double frac;
    if (range->log_scale) {
        if (x <= 0.0) return plot->x0;
        double l_min = log10(range->min);
        double l_max = log10(range->max);
        double l_span = l_max - l_min;
        if (l_span < 1e-12) return plot->x0;
        frac = (log10(x) - l_min) / l_span;
    } else {
        double span = range->max - range->min;
        if (span < 1e-12) return plot->x0;
        frac = (x - range->min) / span;
    }
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    int w = plot->x1 - plot->x0;
    return plot->x0 + (int)(frac * (double)w + 0.5);
}

int fastchart_y_categorical_center(const fastchart_rect *plot, int idx, int n)
{
    if (n <= 0) return plot->y0;
    int h = plot->y1 - plot->y0;
    double step = (double)h / (double)n;
    return plot->y0 + (int)(step * (idx + 0.5));
}

/* Source-image budget: reject files larger than 8 MiB AND with
 * declared dimensions over 4096px on either axis, OR a pixel
 * product over 16M, before handing them to libgd. open_basedir
 * already gates which paths are reachable; these caps are
 * defense-in-depth so a caller who passes a user-supplied
 * background / icon path can't make libgd allocate hundreds of
 * MiB to decode a single image — a small JPEG with declared
 * dimensions of 100000x100000 still claims its full bitmap from
 * the decoder.
 *
 * The cap lives at the source-load helper rather than on the
 * setter — setter time is too early to know whether the file at
 * that path is still the same size (or exists at all) by the
 * time draw() runs. */
#define FASTCHART_SOURCE_IMAGE_MAX_BYTES   (8 * 1024 * 1024)
#define FASTCHART_SOURCE_IMAGE_MAX_DIM     4096
#define FASTCHART_SOURCE_IMAGE_MAX_PIXELS  (16 * 1024 * 1024)

/* Sniff PNG / JPEG / GIF / WebP headers to recover the declared
 * width and height before letting libgd decode the file. Returns
 * 0 on success and writes *w / *h, -1 if the format isn't
 * recognised (the caller treats that as "no preflight available"
 * — the byte-size cap still applies). */
static int fastchart_sniff_image_dims(const char *path, int *w, int *h)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    unsigned char buf[64];
    size_t got = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (got < 16) return -1;

    /* PNG: 8-byte signature, IHDR at offset 16..23 carries
     * width(BE)+height(BE). */
    if (got >= 24 &&
        buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G') {
        *w = (buf[16] << 24) | (buf[17] << 16) | (buf[18] << 8) | buf[19];
        *h = (buf[20] << 24) | (buf[21] << 16) | (buf[22] << 8) | buf[23];
        return 0;
    }
    /* GIF: "GIF87a"/"GIF89a", then width(LE), height(LE) at 6..9. */
    if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F' &&
        (buf[3] == '8') && (buf[4] == '7' || buf[4] == '9') && buf[5] == 'a') {
        *w = buf[6] | (buf[7] << 8);
        *h = buf[8] | (buf[9] << 8);
        return 0;
    }
    /* WebP: "RIFF"....."WEBP" + 4-byte sub-format. */
    if (got >= 30 &&
        buf[0] == 'R' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == 'F' &&
        buf[8] == 'W' && buf[9] == 'E' && buf[10] == 'B' && buf[11] == 'P') {
        if (buf[12] == 'V' && buf[13] == 'P' && buf[14] == '8' && buf[15] == ' ') {
            /* Lossy: skip 7-byte frame tag, find 9D 01 2A start code,
             * then 2 bytes width 14-bit LE + 2 bytes height. The
             * frame tag + start code start at byte 20; W/H at 26-29. */
            *w = (buf[26] | (buf[27] << 8)) & 0x3FFF;
            *h = (buf[28] | (buf[29] << 8)) & 0x3FFF;
            return 0;
        }
        if (got >= 25 &&
            buf[12] == 'V' && buf[13] == 'P' && buf[14] == '8' && buf[15] == 'L') {
            /* Lossless: 1-byte signature 0x2F at byte 20, then a
             * 28-bit packed (width-1, height-1) — 14 bits each, LE. */
            unsigned long bits = (unsigned long)buf[21]
                | ((unsigned long)buf[22] << 8)
                | ((unsigned long)buf[23] << 16)
                | ((unsigned long)buf[24] << 24);
            *w = (int)((bits & 0x3FFFu) + 1);
            *h = (int)(((bits >> 14) & 0x3FFFu) + 1);
            return 0;
        }
        if (got >= 30 &&
            buf[12] == 'V' && buf[13] == 'P' && buf[14] == '8' && buf[15] == 'X') {
            /* Extended: canvas width/height each 3-byte LE - 1
             * starting at byte 24. */
            *w = (buf[24] | (buf[25] << 8) | (buf[26] << 16)) + 1;
            *h = (buf[27] | (buf[28] << 8) | (buf[29] << 16)) + 1;
            return 0;
        }
    }
    /* JPEG: 0xFFD8 SOI, then walk markers. SOF0..SOF15 (excluding
     * C4 / C8 / CC) carry the dimensions. The first 64 bytes don't
     * always cover the SOF marker, but for the common JFIF/EXIF
     * shape we usually find SOF after the APP0/APP1 segment. Read
     * a bigger window for JPEG only. */
    if (buf[0] == 0xFF && buf[1] == 0xD8) {
        FILE *fp2 = fopen(path, "rb");
        if (!fp2) return -1;
        unsigned char jb[1024];
        size_t jgot = fread(jb, 1, sizeof(jb), fp2);
        fclose(fp2);
        size_t i = 2;
        while (i + 9 < jgot) {
            if (jb[i] != 0xFF) return -1;
            unsigned char marker = jb[i + 1];
            /* Skip marker padding 0xFF00 / 0xFFFF run-on. */
            if (marker == 0x00 || marker == 0xFF) { i++; continue; }
            /* SOI / EOI carry no length. */
            if (marker == 0xD8 || marker == 0xD9) { i += 2; continue; }
            /* SOF markers carry W/H. */
            if ((marker >= 0xC0 && marker <= 0xCF) &&
                marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
                /* segment length at i+2..i+3 (BE), precision at i+4,
                 * height at i+5..i+6 (BE), width at i+7..i+8 (BE). */
                *h = (jb[i + 5] << 8) | jb[i + 6];
                *w = (jb[i + 7] << 8) | jb[i + 8];
                return 0;
            }
            /* Other segments: skip via 2-byte BE length. */
            unsigned int seg_len = (jb[i + 2] << 8) | jb[i + 3];
            if (seg_len < 2) return -1;
            i += 2 + seg_len;
        }
        return -1;
    }
    return -1;
}

static gdImagePtr fastchart_load_source_image(const char *path)
{
    struct stat sb;
    if (stat(path, &sb) != 0) return NULL;
    if (!S_ISREG(sb.st_mode)) return NULL;
    if (sb.st_size <= 0) return NULL;
    if (sb.st_size > FASTCHART_SOURCE_IMAGE_MAX_BYTES) return NULL;

    /* Header preflight: enforce the dim/pixel caps BEFORE handing
     * the file to libgd, so a small compressed input with huge
     * decoded dimensions can't force libgd to allocate the full
     * bitmap before fastchart rejects it. We only decode formats
     * we can sniff. Supported sniffers: PNG, JPEG, GIF, WebP —
     * the four formats fastchart itself emits. Unrecognised
     * formats (AVIF, BMP, TIFF, TGA, …) are refused at the gate
     * even when the byte-size cap allows them through; add a
     * sniffer in fastchart_sniff_image_dims() to re-enable. */
    int w = 0, h = 0;
    if (fastchart_sniff_image_dims(path, &w, &h) != 0) {
        return NULL;
    }
    if (w <= 0 || h <= 0 ||
        w > FASTCHART_SOURCE_IMAGE_MAX_DIM ||
        h > FASTCHART_SOURCE_IMAGE_MAX_DIM ||
        (long long)w * (long long)h > FASTCHART_SOURCE_IMAGE_MAX_PIXELS) {
        return NULL;
    }

    /* gdImageCreateFromFile picks the right loader by signature
     * (libgd 2.2.0+). Failure returns NULL. */
    gdImagePtr im = gdImageCreateFromFile(path);
    if (!im) return NULL;

    /* Defense-in-depth post-decode check: if libgd's decoder
     * disagreed with our sniffer about the dimensions (corrupt
     * header, encoder bug, format quirk we didn't anticipate),
     * still refuse to render with an oversized source. */
    int dw = gdImageSX(im), dh = gdImageSY(im);
    if (dw <= 0 || dh <= 0 ||
        dw > FASTCHART_SOURCE_IMAGE_MAX_DIM ||
        dh > FASTCHART_SOURCE_IMAGE_MAX_DIM ||
        (long long)dw * (long long)dh > FASTCHART_SOURCE_IMAGE_MAX_PIXELS) {
        gdImageDestroy(im);
        return NULL;
    }
    return im;
}

/* Load and composite a background image onto the canvas. Format
 * detected from extension. Silently no-ops on load failure -- the
 * chart still renders, just without the bg image.
 *
 * setBackgroundImage() already validated the path against
 * open_basedir, but render time is arbitrarily later — the
 * basedir set in INI may have changed, or open_basedir may have
 * been narrowed via ini_set() between the setter and draw. Re-
 * check here so a runtime narrowing isn't bypassed by a
 * pre-existing chart instance. */
static void composite_bg_image(gdImagePtr im, const char *path)
{
    if (php_check_open_basedir_ex(path, /*warn=*/0) != 0) return;

    int W = gdImageSX(im);
    int H = gdImageSY(im);

    gdImagePtr src = fastchart_load_source_image(path);
    if (!src) return;

    gdImageCopyResampled(im, src, 0, 0, 0, 0,
                         W, H,
                         gdImageSX(src), gdImageSY(src));
    gdImageDestroy(src);
}

void fastchart_blit_icon(fastchart_target_t *t, const fastchart_icon *icon,
                         int px, int py)
{
    if (!icon->path) return;
    if (php_check_open_basedir_ex(icon->path, /*warn=*/0) != 0) return;
    gdImagePtr src = fastchart_load_source_image(icon->path);
    if (!src) return;

    int sw = gdImageSX(src);
    int sh = gdImageSY(src);
    if (sw <= 0 || sh <= 0) {
        gdImageDestroy(src);
        return;
    }

    /* Determine the display size: respect both max_w and max_h while
     * preserving aspect. -1 in either bound means "no cap from this
     * side"; use a per-axis scale factor and pick the smaller (more
     * restrictive) one. */
    double sx = (icon->max_w > 0) ? (double)icon->max_w / (double)sw : 1.0;
    double sy = (icon->max_h > 0) ? (double)icon->max_h / (double)sh : 1.0;
    if (icon->max_w <= 0 && icon->max_h <= 0) {
        sx = sy = 1.0;
    } else if (icon->max_w <= 0) {
        sx = sy;
    } else if (icon->max_h <= 0) {
        sy = sx;
    } else {
        double s = sx < sy ? sx : sy;
        sx = sy = s;
    }

    int dw = (int)(sw * sx + 0.5);
    int dh = (int)(sh * sy + 0.5);
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;

    if (t->kind == FASTCHART_TARGET_GD) {
        /* Preserve the source image's alpha channel through the copy
         * so transparent PNGs blend cleanly onto the chart background.
         * The target's blit primitive doesn't toggle alpha-blending
         * mode; do it here at the GD callsite. */
        gdImageAlphaBlending(t->u.gd.im, 1);
        fastchart_target_image(t, px - dw / 2, py - dh / 2, dw, dh, src);
        gdImageAlphaBlending(t->u.gd.im, 0);
    } else {
        fastchart_target_image(t, px - dw / 2, py - dh / 2, dw, dh, src);
    }
    gdImageDestroy(src);
}

/* Translate libgd's 0..127 (0=opaque, 127=transparent) per-band alpha
 * to the 0..255 (255=opaque) convention used by fastchart_target_color.
 * The inverse of the gd_alpha = (255 - a) >> 1 mapping in
 * fastchart_target_color, with the +1 rounding so a band->alpha=0
 * round-trips to fully opaque. */
static inline int band_alpha_to_255(int gd_alpha)
{
    if (gd_alpha < 0) gd_alpha = 0;
    if (gd_alpha > 127) gd_alpha = 127;
    return 255 - gd_alpha * 2;
}

void fastchart_draw_plot_bands(fastchart_target_t *t, fastchart_obj *chart,
                               const fastchart_rect *plot,
                               const fastchart_value_range *yrange,
                               const fastchart_palette *pal)
{
    (void)pal;
    if (!chart->plot_bands || chart->n_plot_bands <= 0) return;
    for (int i = 0; i < chart->n_plot_bands; i++) {
        const fastchart_plot_band *b = &chart->plot_bands[i];
        if (b->is_vertical) continue;
        /* Map data Y to pixels. fastchart_y_to_pixel inverts the axis,
         * so the high data value becomes the small pixel y (top). */
        int y_top    = fastchart_y_to_pixel(b->high, yrange, plot);
        int y_bottom = fastchart_y_to_pixel(b->low,  yrange, plot);

        /* Clip to the plot rect. Bands fully outside the visible Y
         * range collapse to a zero-height rectangle and skip. */
        if (y_top < plot->y0) y_top = plot->y0;
        if (y_bottom > plot->y1) y_bottom = plot->y1;
        if (y_top >= y_bottom) continue;

        int r = (b->color_rgb >> 16) & 0xFF;
        int g = (b->color_rgb >> 8) & 0xFF;
        int bl = b->color_rgb & 0xFF;
        int color = fastchart_target_color(t, r, g, bl,
                                           band_alpha_to_255(b->alpha));
        if (color < 0) continue;
        fastchart_target_rect(t, plot->x0 + 1, y_top,
                              (plot->x1 - 1) - (plot->x0 + 1) + 1,
                              y_bottom - y_top + 1,
                              color, 1, 0);
    }
}

/* Shared inner: blit the per-band filled rectangle once x0/x1 pixel
 * bounds are computed by the caller. Bands fully outside the plot
 * rect or with x0 >= x1 are skipped. */
static void fastchart_draw_v_band_at(fastchart_target_t *t,
                                     const fastchart_plot_band *b,
                                     const fastchart_rect *plot,
                                     int x_left, int x_right)
{
    if (x_left < plot->x0) x_left = plot->x0 + 1;
    if (x_right > plot->x1) x_right = plot->x1 - 1;
    if (x_left >= x_right) return;
    int r = (b->color_rgb >> 16) & 0xFF;
    int g = (b->color_rgb >> 8) & 0xFF;
    int bl = b->color_rgb & 0xFF;
    int color = fastchart_target_color(t, r, g, bl,
                                       band_alpha_to_255(b->alpha));
    if (color < 0) return;
    fastchart_target_rect(t, x_left, plot->y0 + 1,
                          x_right - x_left + 1,
                          (plot->y1 - 1) - (plot->y0 + 1) + 1,
                          color, 1, 0);
}

/* Horizontal stripes spanning fractional Y category indices, for
 * horizontal-bar layouts where the category axis runs top-to-bottom.
 * Skips vertical bands (those are X-range stripes drawn separately
 * via the xrange helper). */
void fastchart_draw_h_plot_bands_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             int n_categories,
                                             const fastchart_palette *pal)
{
    (void)pal;
    if (!chart->plot_bands || chart->n_plot_bands <= 0) return;
    if (n_categories <= 0) return;
    int span = plot->y1 - plot->y0;
    for (int i = 0; i < chart->n_plot_bands; i++) {
        const fastchart_plot_band *b = &chart->plot_bands[i];
        if (b->is_vertical) continue;
        double frac_lo = b->low  / (double)n_categories;
        double frac_hi = b->high / (double)n_categories;
        int y_top    = plot->y0 + (int)(frac_lo * span + 0.5);
        int y_bottom = plot->y0 + (int)(frac_hi * span + 0.5);
        if (y_top < plot->y0) y_top = plot->y0 + 1;
        if (y_bottom > plot->y1) y_bottom = plot->y1 - 1;
        if (y_top >= y_bottom) continue;

        int r = (b->color_rgb >> 16) & 0xFF;
        int g = (b->color_rgb >> 8) & 0xFF;
        int bl = b->color_rgb & 0xFF;
        int color = fastchart_target_color(t, r, g, bl,
                                           band_alpha_to_255(b->alpha));
        if (color < 0) continue;
        fastchart_target_rect(t, plot->x0 + 1, y_top,
                              (plot->x1 - 1) - (plot->x0 + 1) + 1,
                              y_bottom - y_top + 1,
                              color, 1, 0);
    }
}

void fastchart_draw_v_plot_bands_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             int n_categories,
                                             const fastchart_palette *pal)
{
    (void)pal;
    if (!chart->plot_bands || chart->n_plot_bands <= 0) return;
    if (n_categories <= 0) return;
    int span = plot->x1 - plot->x0;
    for (int i = 0; i < chart->n_plot_bands; i++) {
        const fastchart_plot_band *b = &chart->plot_bands[i];
        if (!b->is_vertical) continue;
        /* Categorical X: low/high are fractional category indices.
         * Map index -> pixel by step = span / n_categories, with the
         * categorical-center half-step offset baked in by treating
         * 0..n as the band's natural domain (so band(0, 1) covers
         * the first slot end-to-end). */
        double frac_lo = b->low  / (double)n_categories;
        double frac_hi = b->high / (double)n_categories;
        int x_left  = plot->x0 + (int)(frac_lo * span + 0.5);
        int x_right = plot->x0 + (int)(frac_hi * span + 0.5);
        fastchart_draw_v_band_at(t, b, plot, x_left, x_right);
    }
}

void fastchart_draw_v_plot_bands_xrange(fastchart_target_t *t, fastchart_obj *chart,
                                        const fastchart_rect *plot,
                                        const fastchart_value_range *xrange,
                                        const fastchart_palette *pal)
{
    (void)pal;
    if (!chart->plot_bands || chart->n_plot_bands <= 0) return;
    for (int i = 0; i < chart->n_plot_bands; i++) {
        const fastchart_plot_band *b = &chart->plot_bands[i];
        if (!b->is_vertical) continue;
        int x_left  = fastchart_x_to_pixel(b->low,  xrange, plot);
        int x_right = fastchart_x_to_pixel(b->high, xrange, plot);
        fastchart_draw_v_band_at(t, b, plot, x_left, x_right);
    }
}

void fastchart_draw_v_plot_bands_time(fastchart_target_t *t, fastchart_obj *chart,
                                      const fastchart_rect *plot,
                                      zend_long t_min, zend_long t_max,
                                      const fastchart_palette *pal)
{
    (void)pal;
    if (!chart->plot_bands || chart->n_plot_bands <= 0) return;
    for (int i = 0; i < chart->n_plot_bands; i++) {
        const fastchart_plot_band *b = &chart->plot_bands[i];
        if (!b->is_vertical) continue;
        int x_left  = fastchart_x_time_to_pixel(plot, (zend_long)b->low,  t_min, t_max);
        int x_right = fastchart_x_time_to_pixel(plot, (zend_long)b->high, t_min, t_max);
        fastchart_draw_v_band_at(t, b, plot, x_left, x_right);
    }
}

void fastchart_draw_frame(fastchart_target_t *t, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal)
{
    int W, H;
    fastchart_target_get_dims(t, &W, &H);

    /* setPlotRect implies the caller is compositing multiple charts on
     * one canvas — wiping the whole image to bg would erase neighbours.
     * Skip the canvas-wide fill in that case; the plot-area fill below
     * still happens, so each chart's own region is painted cleanly. The
     * caller is responsible for pre-filling the canvas they own. */
    if (chart->has_plot_rect) {
        /* no-op: caller manages canvas-wide background */
    } else if (chart->transparent_bg) {
        if (t->kind == FASTCHART_TARGET_GD) {
            /* Reserve the canvas with a fully-transparent fill so
             * PNG / WebP / AVIF outputs preserve alpha. gdImageSaveAlpha
             * must be enabled or the encoder collapses alpha to opaque.
             * SVG has implicit transparency; nothing to emit. */
            gdImagePtr im = t->u.gd.im;
            int trans = gdImageColorAllocateAlpha(im, 0xFF, 0xFF, 0xFF, 127);
            gdImageSaveAlpha(im, 1);
            gdImageAlphaBlending(im, 0);
            gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, trans);
            gdImageAlphaBlending(im, 1);
        }
    } else if (chart->bg_image_path) {
        fastchart_target_rect(t, 0, 0, W, H, pal->bg, 1, 0);
        if (t->kind == FASTCHART_TARGET_GD) {
            composite_bg_image(t->u.gd.im, ZSTR_VAL(chart->bg_image_path));
        }
        /* TODO(svg-refactor) bg_image_path under SVG: would need a
         * base64 <image> emit covering the canvas. Falls back to the
         * solid pal->bg fill above for now. */
    } else {
        fastchart_target_rect(t, 0, 0, W, H, pal->bg, 1, 0);
    }

    /* Plot area background stays opaque so chart elements remain
     * readable on top of a transparent canvas or a busy bg image. */
    fastchart_target_rect(t, plot->x0, plot->y0,
                          plot->x1 - plot->x0 + 1,
                          plot->y1 - plot->y0 + 1,
                          pal->plot_bg, 1, 0);

    /* Border-side bitmask: draw selected sides individually. The
     * Y-axis line gets its own dedicated draw call elsewhere, so
     * suppressing BORDER_LEFT here is safe (the Y axis still shows). */
    zend_long sides = chart->border_sides;
    if (sides & FASTCHART_BORDER_TOP)
        fastchart_target_line(t, plot->x0, plot->y0, plot->x1, plot->y0,
                              pal->border, 1, FASTCHART_DASH_SOLID);
    if (sides & FASTCHART_BORDER_BOTTOM)
        fastchart_target_line(t, plot->x0, plot->y1, plot->x1, plot->y1,
                              pal->border, 1, FASTCHART_DASH_SOLID);
    if (sides & FASTCHART_BORDER_LEFT)
        fastchart_target_line(t, plot->x0, plot->y0, plot->x0, plot->y1,
                              pal->border, 1, FASTCHART_DASH_SOLID);
    if (sides & FASTCHART_BORDER_RIGHT)
        fastchart_target_line(t, plot->x1, plot->y0, plot->x1, plot->y1,
                              pal->border, 1, FASTCHART_DASH_SOLID);
}

/* Centered title at canvas-coord baseline. Used by charts with
 * non-rectangular layouts (radar / gauge / surface / polar /
 * contour) that pass the baseline directly, and by the
 * plot-relative variant below which derives the baseline from
 * `plot->y0`. */
void fastchart_draw_floating_title(fastchart_target_t *t, fastchart_obj *chart,
                                   const fastchart_palette *pal,
                                   int cx, int baseline)
{
    if (!chart->title || ZSTR_LEN(chart->title) == 0) return;
    if (chart->thumbnail_mode) return;
    const char *font = fastchart_resolve_font(chart, FC_FONT_TITLE);
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_TITLE, base * 1.4);
    int color = chart->title_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->title_color)
        : pal->text;
    /* Drop shadow intentionally does NOT apply to the chart title.
     * On every theme it produces a stuttered "doubled" look against
     * a flat background; the effect is meant for filled shapes
     * (bars / pies / area polygons) where it reads as depth. */
    fastchart_text_draw(t, font, size, color, cx, baseline,
                        FASTCHART_ALIGN_CENTER, ZSTR_VAL(chart->title),
                        NULL, 0);
}

void fastchart_draw_title(fastchart_target_t *t, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal)
{
    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    /* Centre over the plot rect, not the canvas. With auto-layout the
     * two coincide; with setPlotRect (compositing several charts on
     * one canvas) we want each title above its own plot. */
    int cx = chart->has_plot_rect ? (plot->x0 + plot->x1) / 2
                                  : W / 2;
    fastchart_draw_floating_title(t, chart, pal,
                                  cx,
                                  plot->y0 - TITLE_PADDING_BELOW(chart, t));
}

static void format_tick_label(double value, double step, char *out, size_t out_n)
{
    /* Pick a decimal precision based on the step magnitude. Avoid
     * trailing-zero clutter for whole-number steps. */
    int decimals;
    if (step >= 1.0) {
        decimals = 0;
    } else {
        decimals = (int)ceil(-log10(step));
        if (decimals < 0) decimals = 0;
        if (decimals > 6) decimals = 6;
    }
    snprintf(out, out_n, "%.*f", decimals, value);
}

void fastchart_format_tick_label_user(double value, const zend_string *fmt,
                                      char *out, size_t out_n)
{
    /* Trust the user's sprintf string; we already rejected embedded
     * NULs at setter time. The callers always pass a small fixed
     * buffer so a runaway format truncates rather than overruns. */
    snprintf(out, out_n, ZSTR_VAL(fmt), value);
}

void fastchart_draw_y_axis(fastchart_target_t *t, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           const fastchart_value_range *range)
{
    if (!chart->y_axis_visible) return;

    /* Y axis line. */
    fastchart_target_line(t, plot->x0, plot->y0, plot->x0, plot->y1,
                          pal->axis, 1, FASTCHART_DASH_SOLID);

    const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base);
    int label_color = chart->axis_label_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->axis_label_color)
        : pal->text;

    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    char buf[32];
    for (int i = 0; i < range->n_ticks; i++) {
        double v = range->ticks[i];
        int y = fastchart_y_to_pixel(v, range, plot);

        /* Grid line across plot. */
        fastchart_target_line(t, plot->x0 + 1, y, plot->x1, y,
                              pal->grid, 1, FASTCHART_DASH_SOLID);

        /* Tick mark on the axis. */
        if (draw_points) {
            fastchart_target_line(t, plot->x0 - TICK_MARK_LEN(chart, t), y,
                                  plot->x0 - 1, y,
                                  pal->axis, 1, FASTCHART_DASH_SOLID);
        }

        if (!draw_labels) continue;

        /* Label, right-aligned just to the left of the tick. */
        if (chart->y_axis_label_format) {
            fastchart_format_tick_label_user(v, chart->y_axis_label_format, buf, sizeof(buf));
        } else {
            format_tick_label(v, range->tick_step, buf, sizeof(buf));
        }
        int label_x = plot->x0 - TICK_MARK_LEN(chart, t) - Y_LABEL_PADDING(chart, t) / 2;
        int label_y = y + (int)(size * 0.35 * chart_dpi_scale(chart, t));  /* baseline correction */
        fastchart_text_draw(t, font, size, label_color,
                            label_x, label_y, FASTCHART_ALIGN_RIGHT,
                            buf, NULL, 0);
    }

    /* Zero shelf: when the data range crosses zero, draw a heavier
     * horizontal axis-color line at y=0 to separate negative from
     * positive values visually. */
    if (chart->zero_shelf && range->min < 0.0 && range->max > 0.0) {
        int zy = fastchart_y_to_pixel(0.0, range, plot);
        fastchart_target_line(t, plot->x0 + 1, zy, plot->x1, zy,
                              pal->axis, 1, FASTCHART_DASH_SOLID);
    }
}

void fastchart_draw_y_axis_right(fastchart_target_t *t, fastchart_obj *chart,
                                 const fastchart_rect *plot,
                                 const fastchart_palette *pal,
                                 const fastchart_value_range *range)
{
    if (!chart->y_axis_visible) return;

    /* Right axis line. */
    fastchart_target_line(t, plot->x1, plot->y0, plot->x1, plot->y1,
                          pal->axis, 1, FASTCHART_DASH_SOLID);

    const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base);
    int label_color = chart->axis_label_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->axis_label_color)
        : pal->text;
    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    char buf[32];
    for (int i = 0; i < range->n_ticks; i++) {
        double v = range->ticks[i];
        int y = fastchart_y_to_pixel(v, range, plot);

        if (draw_points) {
            fastchart_target_line(t, plot->x1 + 1, y,
                                  plot->x1 + TICK_MARK_LEN(chart, t), y,
                                  pal->axis, 1, FASTCHART_DASH_SOLID);
        }
        if (!draw_labels) continue;

        if (chart->y_axis_label_format) {
            fastchart_format_tick_label_user(v, chart->y_axis_label_format, buf, sizeof(buf));
        } else {
            format_tick_label(v, range->tick_step, buf, sizeof(buf));
        }
        int label_x = plot->x1 + TICK_MARK_LEN(chart, t) + Y_LABEL_PADDING(chart, t) / 2;
        int label_y = y + (int)(size * 0.35 * chart_dpi_scale(chart, t));
        fastchart_text_draw(t, font, size, label_color,
                            label_x, label_y, FASTCHART_ALIGN_LEFT,
                            buf, NULL, 0);
    }
}

void fastchart_draw_axis_titles(fastchart_target_t *t, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal)
{
    if (chart->thumbnail_mode) return;
    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    int color = chart->axis_title_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->axis_title_color)
        : pal->text;

    if (chart->x_axis_title && ZSTR_LEN(chart->x_axis_title) > 0) {
        const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
        double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base * 1.1);
        if (font) {
            int cx = (plot->x0 + plot->x1) / 2;
            int baseline = H - MARGIN_BOTTOM_PAD - 2;
            fastchart_text_draw(t, font, size, color,
                                cx, baseline, FASTCHART_ALIGN_CENTER,
                                ZSTR_VAL(chart->x_axis_title), NULL, 0);
        }
    }

    if (chart->y_axis_title && ZSTR_LEN(chart->y_axis_title) > 0) {
        const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
        double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base * 1.1);
        if (font) {
            int cy = (plot->y0 + plot->y1) / 2;
            int x = MARGIN_LEFT_PAD + (int)(size);
            int tw = 0, th = 0;
            if (fastchart_text_measure(t, font, size, ZSTR_VAL(chart->y_axis_title),
                                       &tw, &th, NULL, 0) == 0) {
                int y = cy + tw / 2;
                fastchart_text_draw_rotated(t, font, size, color,
                                            x, y, FASTCHART_ALIGN_LEFT, 90.0,
                                            ZSTR_VAL(chart->y_axis_title), NULL, 0);
            }
        }
    }

    if (chart->y_axis_title2 && ZSTR_LEN(chart->y_axis_title2) > 0 && chart->secondary_y) {
        const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
        double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base * 1.1);
        if (font) {
            int cy = (plot->y0 + plot->y1) / 2;
            int x = W - MARGIN_LEFT_PAD - (int)(size);
            int tw = 0, th = 0;
            if (fastchart_text_measure(t, font, size, ZSTR_VAL(chart->y_axis_title2),
                                       &tw, &th, NULL, 0) == 0) {
                int y = cy - tw / 2;
                fastchart_text_draw_rotated(t, font, size, color,
                                            x, y, FASTCHART_ALIGN_LEFT, 270.0,
                                            ZSTR_VAL(chart->y_axis_title2), NULL, 0);
            }
        }
    }
}

int fastchart_x_categorical_center(const fastchart_rect *plot, int idx, int n)
{
    if (n <= 0) return plot->x0;
    int w = plot->x1 - plot->x0;
    /* Half-step at each end so first/last labels don't sit on the axis. */
    double step = (double)w / (double)n;
    return plot->x0 + (int)(step * (idx + 0.5));
}

void fastchart_draw_x_axis_numeric(fastchart_target_t *t, fastchart_obj *chart,
                                   const fastchart_rect *plot,
                                   const fastchart_palette *pal,
                                   const fastchart_value_range *range)
{
    if (!chart->x_axis_visible) return;

    /* X axis line at the bottom of the plot. */
    fastchart_target_line(t, plot->x0, plot->y1, plot->x1, plot->y1,
                          pal->axis, 1, FASTCHART_DASH_SOLID);

    const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base);
    int label_color = chart->axis_label_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->axis_label_color)
        : pal->text;

    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    /* Measure once: ascender pixel height at the chart's DPI so the
     * label's TOP sits below plot.y1 + tick. Falls back to the
     * point-size heuristic when measurement fails (no font installed,
     * etc.). The previous heuristic of `size * 1.2` undersized the
     * offset at all DPIs — the rendered ascender at 11pt + 96 DPI is
     * already ~12px tall, leaving ~1px clearance over the plot rect. */
    int probe_h = 0;
    if (fastchart_text_measure(t, font, size, "Mg9", NULL, &probe_h, NULL, 0) != 0) {
        probe_h = (int)(size * 1.2 * chart_dpi_scale(chart, t));
    }
    int label_y_base = plot->y1 + TICK_MARK_LEN(chart, t) + probe_h
                     + (int)(4 * chart_dpi_scale(chart, t));

    char buf[32];
    for (int i = 0; i < range->n_ticks; i++) {
        double v = range->ticks[i];
        int x = fastchart_x_to_pixel(v, range, plot);

        /* Vertical gridline across the plot. */
        fastchart_target_line(t, x, plot->y0, x, plot->y1 - 1,
                              pal->grid, 1, FASTCHART_DASH_SOLID);

        if (draw_points) {
            fastchart_target_line(t, x, plot->y1 + 1,
                                  x, plot->y1 + TICK_MARK_LEN(chart, t),
                                  pal->axis, 1, FASTCHART_DASH_SOLID);
        }

        if (!draw_labels) continue;
        if (chart->x_axis_label_format) {
            fastchart_format_tick_label_user(v, chart->x_axis_label_format, buf, sizeof(buf));
        } else {
            format_tick_label(v, range->tick_step, buf, sizeof(buf));
        }
        int label_y = label_y_base;
        fastchart_text_draw(t, font, size, label_color,
                            x, label_y, FASTCHART_ALIGN_CENTER,
                            buf, NULL, 0);
    }

    /* Zero shelf: when the value range crosses zero, draw a heavier
     * vertical axis-color line at x=0 to separate negative from
     * positive values visually. */
    if (chart->zero_shelf && range->min < 0.0 && range->max > 0.0) {
        int zx = fastchart_x_to_pixel(0.0, range, plot);
        fastchart_target_line(t, zx, plot->y0, zx, plot->y1 - 1,
                              pal->axis, 1, FASTCHART_DASH_SOLID);
    }
}

void fastchart_draw_y_axis_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       int n_categories,
                                       const char *const *labels)
{
    if (!chart->y_axis_visible) return;

    /* Y axis line on the left edge. */
    fastchart_target_line(t, plot->x0, plot->y0, plot->x0, plot->y1,
                          pal->axis, 1, FASTCHART_DASH_SOLID);

    if (n_categories <= 0) return;
    const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
    if (!font) return;

    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base);
    int label_color = chart->axis_label_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->axis_label_color)
        : pal->text;
    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    /* Cap labels to ~20 visible categories — Y has more vertical room
     * than X for label stacking, but enough is enough. */
    int max_visible = 20;
    int stride = 1;
    if (n_categories > max_visible + 2) {
        stride = (n_categories + max_visible - 1) / max_visible;
    }

    for (int i = 0; i < n_categories; i += stride) {
        int y = fastchart_y_categorical_center(plot, i, n_categories);
        if (draw_points) {
            fastchart_target_line(t, plot->x0 - TICK_MARK_LEN(chart, t), y,
                                  plot->x0 - 1, y,
                                  pal->axis, 1, FASTCHART_DASH_SOLID);
        }
        if (!draw_labels) continue;

        char fallback[16];
        const char *txt;
        if (labels && labels[i]) {
            txt = labels[i];
        } else {
            snprintf(fallback, sizeof(fallback), "%d", i);
            txt = fallback;
        }
        int label_x = plot->x0 - TICK_MARK_LEN(chart, t) - Y_LABEL_PADDING(chart, t) / 2;
        int label_y = y + (int)(size * 0.35 * chart_dpi_scale(chart, t));
        fastchart_text_draw(t, font, size, label_color,
                            label_x, label_y, FASTCHART_ALIGN_RIGHT,
                            txt, NULL, 0);
    }
}

void fastchart_draw_x_axis_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       int n_categories,
                                       const char *const *labels)
{
    if (!chart->x_axis_visible) return;

    /* X axis line. */
    fastchart_target_line(t, plot->x0, plot->y1, plot->x1, plot->y1,
                          pal->axis, 1, FASTCHART_DASH_SOLID);

    if (n_categories <= 0) return;
    const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
    if (!font) return;

    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base);
    int angle = (int)chart->x_axis_label_angle;
    int label_color = chart->axis_label_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->axis_label_color)
        : pal->text;
    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    /* Stride caps horizontal labels to ~10; rotated labels are
     * narrow enough that we can show all of them up to ~30. The
     * user's setXLabelStride() multiplies on top of the auto value,
     * so passing 1 (default) doesn't fight the auto-density. */
    int max_visible = (angle == 0) ? 10 : 30;
    int stride = 1;
    if (n_categories > max_visible + 2) {
        stride = (n_categories + max_visible - 1) / max_visible;
    }
    if (chart->x_label_stride > 1) {
        stride *= (int)chart->x_label_stride;
    }

    /* Rotated-label anchor offset: when text is rotated CCW around
     * the anchor point, the unrotated text extends upward-left (45°)
     * or fully upward (90°). label_y must clear the rotated label's
     * top edge, which sits at anchor - label_width * sin(angle).
     * Measure the longest label that will actually be drawn so the
     * offset matches reality. Skip measurement entirely when labels
     * are suppressed (TICK_POINTS / TICK_NONE / thumbnail) — the
     * earlier unconditional walk burned >1s per render at 4096
     * categories even though no label would touch the canvas. */
    int max_label_w = 0;
    if (angle != 0 && draw_labels && labels) {
        for (int i = 0; i < n_categories; i += stride) {
            if (!labels[i]) continue;
            int w = 0, h = 0;
            if (fastchart_text_measure(t, font, size, labels[i],
                                       &w, &h, NULL, 0) == 0 && w > max_label_w) {
                max_label_w = w;
            }
        }
    }

    /* Measure ascender height once at the chart's DPI so the label
     * top sits below plot.y1 + tick. The point-size * 1.2 heuristic
     * undershoots in mixed FreeType configurations and at high DPI,
     * leaving the label clipping into the plot rect. */
    int probe_h_x = 0;
    if (fastchart_text_measure(t, font, size, "Mg9", NULL, &probe_h_x, NULL, 0) != 0) {
        probe_h_x = (int)(size * 1.2 * chart_dpi_scale(chart, t));
    }
    int label_y;
    fastchart_align align;
    if (angle == 0) {
        label_y = plot->y1 + TICK_MARK_LEN(chart, t) + probe_h_x
                + (int)(4 * chart_dpi_scale(chart, t));
        align = FASTCHART_ALIGN_CENTER;
    } else if (angle == 45) {
        /* Push baseline far enough below plot so the rotated label's
         * top corner clears the plot rect. sin(45°) ≈ 0.707; add a
         * small constant for visual breathing room. */
        int rotate_offset = (int)((double)max_label_w * 0.707) + 4;
        label_y = plot->y1 + TICK_MARK_LEN(chart, t) + rotate_offset;
        align = FASTCHART_ALIGN_RIGHT;
    } else { /* 90 */
        /* Vertical label extends fully upward from anchor by its
         * pre-rotation horizontal width. */
        label_y = plot->y1 + TICK_MARK_LEN(chart, t) + max_label_w + 4;
        align = FASTCHART_ALIGN_RIGHT;
    }

    for (int i = 0; i < n_categories; i += stride) {
        int x = fastchart_x_categorical_center(plot, i, n_categories);
        if (draw_points) {
            fastchart_target_line(t, x, plot->y1 + 1,
                                  x, plot->y1 + TICK_MARK_LEN(chart, t),
                                  pal->axis, 1, FASTCHART_DASH_SOLID);
        }
        if (!draw_labels) continue;

        char fallback[16];
        const char *txt;
        if (labels && labels[i]) {
            txt = labels[i];
        } else {
            snprintf(fallback, sizeof(fallback), "%d", i);
            txt = fallback;
        }
        if (angle == 0) {
            fastchart_text_draw(t, font, size, label_color,
                                x, label_y, align, txt, NULL, 0);
        } else {
            fastchart_text_draw_rotated(t, font, size, label_color,
                                        x, label_y, align, (double)angle,
                                        txt, NULL, 0);
        }
    }
}

int fastchart_x_time_to_pixel(const fastchart_rect *plot,
                              zend_long ts, zend_long t_min, zend_long t_max)
{
    zend_long span = t_max - t_min;
    if (span <= 0) return plot->x0;
    double frac = (double)(ts - t_min) / (double)span;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    int w = plot->x1 - plot->x0;
    return plot->x0 + (int)(frac * (double)w + 0.5);
}

void fastchart_draw_legend(fastchart_target_t *t, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           int n_entries,
                           const int *colors,
                           const char *const *labels)
{
    if (n_entries < 1) return;
    if (chart->legend_position == FASTCHART_LEGEND_NONE) return;

    /* Defer the "no usable font" decision to fastchart_resolve_font:
     * it knows about per-role overrides, so a chart with no global
     * font_path but an explicit setLabelFont() must still draw the
     * legend. The earlier `if (!chart->font_path) return;` short-
     * circuited that path. */
    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = fastchart_resolve_font(chart, FC_FONT_LABEL);
    if (!font) return;

    int swatch_w  = 14;
    int swatch_h  = 10;
    int row_pad   = 4;
    int outer_pad = 6;
    int gap       = 6;     /* swatch -> text gap */
    int margin    = 6;     /* gap to plot border */

    int row_h, dummy;
    if (fastchart_text_measure(t, font, size, "Mg9", &dummy, &row_h, NULL, 0) != 0) {
        row_h = (int)(size * 1.4);
    }
    if (row_h < swatch_h) row_h = swatch_h;

    /* Measure the longest label to size the legend box. Skip NULL
     * labels in both width measurement and rendering. */
    int max_label_w = 0;
    int rows = 0;
    for (int i = 0; i < n_entries; i++) {
        if (!labels[i]) continue;
        int w, h;
        if (fastchart_text_measure(t, font, size, labels[i], &w, &h, NULL, 0) == 0) {
            if (w > max_label_w) max_label_w = w;
        }
        rows++;
    }
    if (rows == 0) return;

    int box_w = outer_pad * 2 + swatch_w + gap + max_label_w;
    int box_h = outer_pad * 2 + rows * row_h + (rows - 1) * row_pad;

    int x0, y0;
    switch (chart->legend_position) {
        case FASTCHART_LEGEND_TOP_LEFT:
            x0 = plot->x0 + margin;
            y0 = plot->y0 + margin;
            break;
        case FASTCHART_LEGEND_BOTTOM_RIGHT:
            x0 = plot->x1 - box_w - margin;
            y0 = plot->y1 - box_h - margin;
            break;
        case FASTCHART_LEGEND_BOTTOM_LEFT:
            x0 = plot->x0 + margin;
            y0 = plot->y1 - box_h - margin;
            break;
        case FASTCHART_LEGEND_TOP_RIGHT:
        default:
            x0 = plot->x1 - box_w - margin;
            y0 = plot->y0 + margin;
            break;
    }
    int x1 = x0 + box_w;
    int y1 = y0 + box_h;
    if (x0 < plot->x0 + 6) x0 = plot->x0 + 6;
    if (y0 < plot->y0 + 6) y0 = plot->y0 + 6;

    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
                          pal->plot_bg, 1, 0);
    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
                          pal->border, 0, 1);

    int row_y = y0 + outer_pad;
    for (int i = 0; i < n_entries; i++) {
        if (!labels[i]) continue;
        int sx0 = x0 + outer_pad;
        int sy0 = row_y + (row_h - swatch_h) / 2;
        fastchart_target_rect(t, sx0, sy0, swatch_w, swatch_h,
                              colors[i], 1, 0);
        fastchart_target_rect(t, sx0, sy0, swatch_w, swatch_h,
                              pal->border, 0, 1);

        int tx = sx0 + swatch_w + gap;
        int ty = row_y + row_h - 2;
        fastchart_text_draw(t, font, size, pal->text,
                            tx, ty, FASTCHART_ALIGN_LEFT,
                            labels[i], NULL, 0);

        row_y += row_h + row_pad;
    }
}

void fastchart_draw_value_label(fastchart_target_t *t, fastchart_obj *chart,
                                const fastchart_palette *pal,
                                int x, int y, double value)
{
    if (!chart->show_values) return;
    if (!isfinite(value)) return;
    const char *font = fastchart_resolve_font(chart, FC_FONT_LABEL);
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_LABEL, base * 0.85);

    const char *fmt = chart->value_format ? ZSTR_VAL(chart->value_format) : "%g";
    char buf[32];
    snprintf(buf, sizeof(buf), fmt, value);

    /* Place baseline a few pixels above the data point. */
    int label_y = y - 6;
    fastchart_text_draw(t, font, size, pal->text,
                        x, label_y, FASTCHART_ALIGN_CENTER, buf, NULL, 0);
}

/* Allocate or reuse a color from an overlay's optional 'color' key.
 * Falls back to the rotating palette index `slot`. Returns a target
 * color handle. */
static int overlay_color(fastchart_target_t *t, const fastchart_palette *pal,
                         zval *entry, int slot)
{
    zval *c = zend_hash_str_find(Z_ARRVAL_P(entry), "color", sizeof("color") - 1);
    if (c && Z_TYPE_P(c) == IS_LONG) {
        zend_long v = Z_LVAL_P(c);
        if (v >= 0 && v <= 0xFFFFFF) {
            return fastchart_target_color_rgb(t, (int)v);
        }
    }
    return pal->series[(slot + 4) % FASTCHART_PALETTE_SERIES_N];
}

static int overlay_thickness(zval *entry)
{
    zval *t = zend_hash_str_find(Z_ARRVAL_P(entry), "thickness", sizeof("thickness") - 1);
    if (t && Z_TYPE_P(t) == IS_LONG) {
        zend_long v = Z_LVAL_P(t);
        if (v >= 1 && v <= 16) return (int)v;
    }
    return 2;
}

static bool overlay_right_axis(zval *entry)
{
    zval *a = zend_hash_str_find(Z_ARRVAL_P(entry), "axis", sizeof("axis") - 1);
    return (a && Z_TYPE_P(a) == IS_STRING &&
            zend_string_equals_literal(Z_STR_P(a), "right"));
}

void fastchart_draw_overlays_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                          const fastchart_rect *plot,
                                          const fastchart_palette *pal,
                                          const fastchart_value_range *yrange,
                                          const fastchart_value_range *yrange_right,
                                          int n_categories)
{
    zval *list = zend_hash_str_find(Z_ARRVAL(chart->config),
                                    "overlays", sizeof("overlays") - 1);
    if (!list || Z_TYPE_P(list) != IS_ARRAY) return;
    if (n_categories <= 0) return;

    int slot = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *type_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "type", sizeof("type") - 1);
        zval *vals_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "values", sizeof("values") - 1);
        if (!type_zv || !vals_zv || Z_TYPE_P(vals_zv) != IS_ARRAY) continue;

        bool is_area = (Z_TYPE_P(type_zv) == IS_STRING &&
                        zend_string_equals_literal(Z_STR_P(type_zv), "area"));
        const fastchart_value_range *rng =
            (overlay_right_axis(entry) && yrange_right) ? yrange_right : yrange;
        int color = overlay_color(t, pal, entry, slot);
        int thick = overlay_thickness(entry);

        /* Build a points array. Missing / non-numeric entries break
         * the polyline (matching the line-gap convention). */
        fastchart_pt *pts = ecalloc((size_t)n_categories, sizeof(fastchart_pt));
        for (int i = 0; i < n_categories; i++) {
            zval *vv = zend_hash_index_find(Z_ARRVAL_P(vals_zv), i);
            double d;
            if (vv && Z_TYPE_P(vv) != IS_NULL && fastchart_zval_to_double(vv, &d) == 0) {
                pts[i].x = fastchart_x_categorical_center(plot, i, n_categories);
                pts[i].y = fastchart_y_to_pixel(d, rng, plot);
                pts[i].valid = true;
            } else {
                pts[i].valid = false;
            }
        }

        if (is_area) {
            /* Build a closed polygon: top edge through valid points,
             * then bottom edge along zero baseline (or plot bottom
             * for log scale). Translucent fill so layered overlays
             * stay readable. */
            int zero_y = fastchart_y_to_pixel(rng->log_scale ? rng->min : 0.0, rng, plot);
            uint32_t rgba = fastchart_target_color_to_rgba(t, color);
            int r = (int)((rgba >> 16) & 0xFFu);
            int g = (int)((rgba >>  8) & 0xFFu);
            int b = (int)( rgba        & 0xFFu);
            /* Match the old gdImageColorAllocateAlpha(..., 80) blend:
             * 80 in libgd 0..127 -> 255 - 80*2 = 95 in 0..255. */
            int alpha_color = fastchart_target_color(t, r, g, b, 95);

            gdPoint poly[2 * 1024];
            int np = 0;
            for (int i = 0; i < n_categories && np < 1024; i++) {
                if (!pts[i].valid) continue;
                poly[np].x = pts[i].x; poly[np].y = pts[i].y; np++;
            }
            for (int i = n_categories - 1; i >= 0 && np < 2 * 1024; i--) {
                if (!pts[i].valid) continue;
                poly[np].x = pts[i].x; poly[np].y = zero_y; np++;
            }
            if (np >= 3 && alpha_color >= 0) {
                if (t->kind == FASTCHART_TARGET_GD) {
                    /* Enable alpha-blending mode for the translucent
                     * fill; restore opaque mode afterward so subsequent
                     * non-translucent draws aren't blended. */
                    gdImageAlphaBlending(t->u.gd.im, 1);
                    fastchart_target_polygon(t, poly, np, alpha_color, 1, 0);
                    gdImageAlphaBlending(t->u.gd.im, 0);
                } else {
                    fastchart_target_polygon(t, poly, np, alpha_color, 1, 0);
                }
            }
        }

        fastchart_draw_polyline(t, chart, pts, n_categories,
                                color, thick, !is_area);
        efree(pts);
        slot++;
    } ZEND_HASH_FOREACH_END();
}

/* Overlay rendering for horizontal-bar layouts: value axis is X,
 * category axis is Y. Same overlay-list source ('overlays' on config),
 * same per-entry shape (type / values / color / thickness). Unlike
 * the categorical helper, lines / area-fill axes flip: y comes from
 * the categorical center, x comes from the value range. Area fill
 * closes against x=0 (the value-axis zero shelf) instead of the
 * y baseline. */
void fastchart_draw_overlays_horizontal_bar(fastchart_target_t *t, fastchart_obj *chart,
                                            const fastchart_rect *plot,
                                            const fastchart_palette *pal,
                                            const fastchart_value_range *xrange,
                                            int n_categories)
{
    zval *list = zend_hash_str_find(Z_ARRVAL(chart->config),
                                    "overlays", sizeof("overlays") - 1);
    if (!list || Z_TYPE_P(list) != IS_ARRAY) return;
    if (n_categories <= 0) return;

    int slot = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *type_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "type", sizeof("type") - 1);
        zval *vals_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "values", sizeof("values") - 1);
        if (!type_zv || !vals_zv || Z_TYPE_P(vals_zv) != IS_ARRAY) continue;

        bool is_area = (Z_TYPE_P(type_zv) == IS_STRING &&
                        zend_string_equals_literal(Z_STR_P(type_zv), "area"));
        int color = overlay_color(t, pal, entry, slot);
        int thick = overlay_thickness(entry);

        fastchart_pt *pts = ecalloc((size_t)n_categories, sizeof(fastchart_pt));
        for (int i = 0; i < n_categories; i++) {
            zval *vv = zend_hash_index_find(Z_ARRVAL_P(vals_zv), i);
            double d;
            if (vv && Z_TYPE_P(vv) != IS_NULL && fastchart_zval_to_double(vv, &d) == 0) {
                pts[i].x = fastchart_x_to_pixel(d, xrange, plot);
                pts[i].y = fastchart_y_categorical_center(plot, i, n_categories);
                pts[i].valid = true;
            } else {
                pts[i].valid = false;
            }
        }

        if (is_area) {
            int zero_x = fastchart_x_to_pixel(xrange->log_scale ? xrange->min : 0.0,
                                              xrange, plot);
            uint32_t rgba = fastchart_target_color_to_rgba(t, color);
            int r = (int)((rgba >> 16) & 0xFFu);
            int g = (int)((rgba >>  8) & 0xFFu);
            int b = (int)( rgba        & 0xFFu);
            int alpha_color = fastchart_target_color(t, r, g, b, 95);

            gdPoint poly[2 * 1024];
            int np = 0;
            for (int i = 0; i < n_categories && np < 1024; i++) {
                if (!pts[i].valid) continue;
                poly[np].x = pts[i].x; poly[np].y = pts[i].y; np++;
            }
            for (int i = n_categories - 1; i >= 0 && np < 2 * 1024; i--) {
                if (!pts[i].valid) continue;
                poly[np].x = zero_x; poly[np].y = pts[i].y; np++;
            }
            if (np >= 3 && alpha_color >= 0) {
                if (t->kind == FASTCHART_TARGET_GD) {
                    gdImageAlphaBlending(t->u.gd.im, 1);
                    fastchart_target_polygon(t, poly, np, alpha_color, 1, 0);
                    gdImageAlphaBlending(t->u.gd.im, 0);
                } else {
                    fastchart_target_polygon(t, poly, np, alpha_color, 1, 0);
                }
            }
        }

        fastchart_draw_polyline(t, chart, pts, n_categories,
                                color, thick, !is_area);
        efree(pts);
        slot++;
    } ZEND_HASH_FOREACH_END();
}

void fastchart_draw_overlays_time(fastchart_target_t *t, fastchart_obj *chart,
                                  const fastchart_rect *plot,
                                  const fastchart_palette *pal,
                                  const fastchart_value_range *yrange,
                                  zend_long t_min, zend_long t_max,
                                  zend_long *timestamps, int n_candles)
{
    zval *list = zend_hash_str_find(Z_ARRVAL(chart->config),
                                    "overlays", sizeof("overlays") - 1);
    if (!list || Z_TYPE_P(list) != IS_ARRAY) return;
    if (n_candles <= 0) return;

    int slot = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *type_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "type", sizeof("type") - 1);
        zval *vals_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "values", sizeof("values") - 1);
        if (!type_zv || !vals_zv || Z_TYPE_P(vals_zv) != IS_ARRAY) continue;

        int color = overlay_color(t, pal, entry, slot);
        int thick = overlay_thickness(entry);

        fastchart_pt *pts = ecalloc((size_t)n_candles, sizeof(fastchart_pt));
        for (int i = 0; i < n_candles; i++) {
            zval *vv = zend_hash_index_find(Z_ARRVAL_P(vals_zv), i);
            double d;
            if (vv && Z_TYPE_P(vv) != IS_NULL && fastchart_zval_to_double(vv, &d) == 0) {
                pts[i].x = fastchart_x_time_to_pixel(plot, timestamps[i], t_min, t_max);
                pts[i].y = fastchart_y_to_pixel(d, yrange, plot);
                pts[i].valid = true;
            } else {
                pts[i].valid = false;
            }
        }
        fastchart_draw_polyline(t, chart, pts, n_candles, color, thick, true);
        efree(pts);
        slot++;
    } ZEND_HASH_FOREACH_END();
}

static int annotation_color(const fastchart_palette *pal, fastchart_target_t *t,
                            zval *color_zv)
{
    if (color_zv && Z_TYPE_P(color_zv) == IS_LONG) {
        zend_long c = Z_LVAL_P(color_zv);
        if (c >= 0 && c <= 0xFFFFFF) {
            return fastchart_target_color_rgb(t, (int)c);
        }
    }
    return pal->axis;
}

static zval *find_annotations(fastchart_obj *chart)
{
    zval *list = zend_hash_str_find(Z_ARRVAL(chart->config),
                                    "annotations", sizeof("annotations") - 1);
    if (!list || Z_TYPE_P(list) != IS_ARRAY) return NULL;
    return list;
}

void fastchart_draw_h_annotations(fastchart_target_t *t, fastchart_obj *chart,
                                  const fastchart_rect *plot,
                                  const fastchart_palette *pal,
                                  const fastchart_value_range *yrange)
{
    zval *list = find_annotations(chart);
    if (!list) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = fastchart_resolve_font(chart, FC_FONT_ANNOTATION);

    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *kind = zend_hash_str_find(Z_ARRVAL_P(entry), "kind", 4);
        if (!kind || Z_TYPE_P(kind) != IS_STRING) continue;
        if (strcmp(Z_STRVAL_P(kind), "h") != 0) continue;

        zval *value_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "value", 5);
        if (!value_zv || Z_TYPE_P(value_zv) != IS_DOUBLE) continue;
        double v = Z_DVAL_P(value_zv);

        int y = fastchart_y_to_pixel(v, yrange, plot);
        if (y < plot->y0 || y > plot->y1) continue;

        int color = annotation_color(pal, t,
            zend_hash_str_find(Z_ARRVAL_P(entry), "color", 5));
        fastchart_target_line(t, plot->x0 + 1, y, plot->x1 - 1, y,
                              color, 1, FASTCHART_DASH_DASHED);

        zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "label", 5);
        const char *label = fastchart_label_or_null(label_zv);
        if (label && font) {
            int tx = plot->x1 - 6;
            int ty = y - 4;  /* sit just above the line */
            fastchart_text_draw(t, font, size, color,
                                tx, ty, FASTCHART_ALIGN_RIGHT,
                                label, NULL, 0);
        }
    } ZEND_HASH_FOREACH_END();
}

/* Shared body for vertical annotations -- the only difference
 * across the three coord systems is how `position` becomes a pixel
 * x. We accept a callback. */
typedef int (*v_pos_to_x)(const fastchart_rect *plot, double position, void *ctx);

static int v_pos_categorical(const fastchart_rect *plot, double position, void *ctx)
{
    int n = *(int *)ctx;
    if (n <= 0) return -1;
    int idx = (int)floor(position + 0.5);
    if (idx < 0 || idx >= n) return -1;
    return fastchart_x_categorical_center(plot, idx, n);
}

typedef struct { double xmin; double xmax; } v_continuous_ctx;
static int v_pos_continuous(const fastchart_rect *plot, double position, void *ctx)
{
    v_continuous_ctx *c = (v_continuous_ctx *)ctx;
    double span = c->xmax - c->xmin;
    if (span <= 0) return -1;
    if (position < c->xmin || position > c->xmax) return -1;
    double frac = (position - c->xmin) / span;
    return plot->x0 + (int)(frac * (plot->x1 - plot->x0) + 0.5);
}

typedef struct { zend_long t_min; zend_long t_max; } v_time_ctx;
static int v_pos_time(const fastchart_rect *plot, double position, void *ctx)
{
    v_time_ctx *c = (v_time_ctx *)ctx;
    if (!isfinite(position) ||
        position < (double)ZEND_LONG_MIN || position > (double)ZEND_LONG_MAX) {
        return -1;
    }
    zend_long ts = (zend_long)position;
    if (ts < c->t_min || ts > c->t_max) return -1;
    return fastchart_x_time_to_pixel(plot, ts, c->t_min, c->t_max);
}

static void draw_v_annotations_with_mapper(fastchart_target_t *t, fastchart_obj *chart,
                                            const fastchart_rect *plot,
                                            const fastchart_palette *pal,
                                            v_pos_to_x mapper, void *ctx)
{
    zval *list = find_annotations(chart);
    if (!list) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = fastchart_resolve_font(chart, FC_FONT_ANNOTATION);

    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *kind = zend_hash_str_find(Z_ARRVAL_P(entry), "kind", 4);
        if (!kind || Z_TYPE_P(kind) != IS_STRING) continue;
        if (strcmp(Z_STRVAL_P(kind), "v") != 0) continue;

        zval *value_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "value", 5);
        if (!value_zv || Z_TYPE_P(value_zv) != IS_DOUBLE) continue;

        int x = mapper(plot, Z_DVAL_P(value_zv), ctx);
        if (x < plot->x0 || x > plot->x1) continue;

        int color = annotation_color(pal, t,
            zend_hash_str_find(Z_ARRVAL_P(entry), "color", 5));
        fastchart_target_line(t, x, plot->y0 + 1, x, plot->y1 - 1,
                              color, 1, FASTCHART_DASH_DASHED);

        zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "label", 5);
        const char *label = fastchart_label_or_null(label_zv);
        if (label && font) {
            /* Place the label just below the top edge of the plot
             * area, centered on the annotation line. */
            int ty = plot->y0 + (int)(size * 1.2 * chart_dpi_scale(chart, t)) + 2;
            fastchart_text_draw(t, font, size, color,
                                x + 4, ty, FASTCHART_ALIGN_LEFT,
                                label, NULL, 0);
        }
    } ZEND_HASH_FOREACH_END();
}

void fastchart_draw_v_annotations_categorical(fastchart_target_t *t, fastchart_obj *chart,
                                              const fastchart_rect *plot,
                                              const fastchart_palette *pal,
                                              int n_categories)
{
    int ctx = n_categories;
    draw_v_annotations_with_mapper(t, chart, plot, pal, v_pos_categorical, &ctx);
}

void fastchart_draw_v_annotations_continuous(fastchart_target_t *t, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             const fastchart_palette *pal,
                                             const fastchart_value_range *xrange)
{
    v_continuous_ctx ctx = { xrange->min, xrange->max };
    draw_v_annotations_with_mapper(t, chart, plot, pal, v_pos_continuous, &ctx);
}

void fastchart_draw_v_annotations_time(fastchart_target_t *t, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       zend_long t_min, zend_long t_max)
{
    v_time_ctx ctx = { t_min, t_max };
    draw_v_annotations_with_mapper(t, chart, plot, pal, v_pos_time, &ctx);
}

/* Annotation rendering for horizontal-bar layouts where the value
 * axis is X and the category axis is Y. Walks the shared annotation
 * list once and dispatches:
 *   - "h" entries (addHorizontalLine, value-axis annotation)
 *     -> vertical screen line at fastchart_x_to_pixel(value)
 *   - "v" entries (addVerticalLine, category-axis annotation,
 *     value = fractional category index)
 *     -> horizontal screen line at the corresponding category center
 * The user-facing API names are tied to the default vertical-bar
 * orientation; on horizontal-bar the visual roles swap but the
 * semantics remain "h = value, v = category". */
void fastchart_draw_horizontal_bar_annotations(fastchart_target_t *t, fastchart_obj *chart,
                                               const fastchart_rect *plot,
                                               const fastchart_palette *pal,
                                               const fastchart_value_range *xrange,
                                               int n_categories)
{
    zval *list = find_annotations(chart);
    if (!list) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = fastchart_resolve_font(chart, FC_FONT_ANNOTATION);

    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *kind_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "kind", 4);
        if (!kind_zv || Z_TYPE_P(kind_zv) != IS_STRING) continue;
        zval *value_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "value", 5);
        if (!value_zv || Z_TYPE_P(value_zv) != IS_DOUBLE) continue;
        double v = Z_DVAL_P(value_zv);

        int color = annotation_color(pal, t,
            zend_hash_str_find(Z_ARRVAL_P(entry), "color", 5));
        zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "label", 5);
        const char *label = fastchart_label_or_null(label_zv);

        if (strcmp(Z_STRVAL_P(kind_zv), "h") == 0) {
            /* Value-axis annotation: vertical screen line at x=value. */
            int x = fastchart_x_to_pixel(v, xrange, plot);
            if (x < plot->x0 || x > plot->x1) continue;
            fastchart_target_line(t, x, plot->y0 + 1, x, plot->y1 - 1,
                                  color, 1, FASTCHART_DASH_DASHED);
            if (label && font) {
                int ty = plot->y0 + (int)(size * 1.2 * chart_dpi_scale(chart, t)) + 2;
                fastchart_text_draw(t, font, size, color,
                                    x + 4, ty, FASTCHART_ALIGN_LEFT,
                                    label, NULL, 0);
            }
        } else if (strcmp(Z_STRVAL_P(kind_zv), "v") == 0) {
            /* Category-axis annotation: horizontal screen line at
             * y=category-center. v is a fractional category index. */
            if (n_categories <= 0) continue;
            int idx = (int)floor(v + 0.5);
            if (idx < 0 || idx >= n_categories) continue;
            int y = fastchart_y_categorical_center(plot, idx, n_categories);
            fastchart_target_line(t, plot->x0 + 1, y, plot->x1 - 1, y,
                                  color, 1, FASTCHART_DASH_DASHED);
            if (label && font) {
                int tx = plot->x1 - 6;
                int ty = y - 4;
                fastchart_text_draw(t, font, size, color,
                                    tx, ty, FASTCHART_ALIGN_RIGHT,
                                    label, NULL, 0);
            }
        }
    } ZEND_HASH_FOREACH_END();
}

void fastchart_draw_text_annotations(fastchart_target_t *t, fastchart_obj *chart,
                                     const fastchart_palette *pal)
{
    zval *list_zv = zend_hash_str_find(Z_ARRVAL(chart->config),
                                       "text_annotations",
                                       sizeof("text_annotations") - 1);
    if (!list_zv || Z_TYPE_P(list_zv) != IS_ARRAY) return;

    const char *font = fastchart_resolve_font(chart, FC_FONT_ANNOTATION);
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_ANNOTATION, base);

    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list_zv), entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *t_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "text", sizeof("text") - 1);
        zval *x_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "x", sizeof("x") - 1);
        zval *y_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "y", sizeof("y") - 1);
        if (!t_zv || !x_zv || !y_zv) continue;
        if (Z_TYPE_P(t_zv) != IS_STRING) continue;

        int x = (int)Z_LVAL_P(x_zv);
        int y = (int)Z_LVAL_P(y_zv);
        int color = pal->text;
        zval *c_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "color", sizeof("color") - 1);
        if (c_zv && Z_TYPE_P(c_zv) == IS_LONG) {
            color = fastchart_target_color_rgb(t, (int)Z_LVAL_P(c_zv));
        }

        fastchart_text_draw(t, font, size, color, x, y,
                            FASTCHART_ALIGN_LEFT, Z_STRVAL_P(t_zv), NULL, 0);
    } ZEND_HASH_FOREACH_END();
}

void fastchart_draw_x_axis_time(fastchart_target_t *t, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal,
                                zend_long t_min, zend_long t_max)
{
    if (!chart->x_axis_visible) return;
    fastchart_target_line(t, plot->x0, plot->y1, plot->x1, plot->y1,
                          pal->axis, 1, FASTCHART_DASH_SOLID);

    if (t_max <= t_min) return;
    const char *font = fastchart_resolve_font(chart, FC_FONT_AXIS);
    if (!font) return;

    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, FC_FONT_AXIS, base);
    int angle = (int)chart->x_axis_label_angle;
    int label_color = chart->axis_label_color >= 0
        ? fastchart_target_color_rgb(t, (int)chart->axis_label_color)
        : pal->text;
    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    /* Same measured-ascender rule as numeric/categorical: the
     * fixed-multiplier heuristic clips into the plot rect when
     * FreeType's ascender exceeds 1.2 * point-size at the chart's
     * DPI. */
    int probe_h_t = 0;
    if (fastchart_text_measure(t, font, size, "Mg9", NULL, &probe_h_t, NULL, 0) != 0) {
        probe_h_t = (int)(size * 1.2 * chart_dpi_scale(chart, t));
    }
    int label_y;
    fastchart_align align;
    if (angle == 0) {
        label_y = plot->y1 + TICK_MARK_LEN(chart, t) + probe_h_t
                + (int)(4 * chart_dpi_scale(chart, t));
        align = FASTCHART_ALIGN_CENTER;
    } else {
        label_y = plot->y1 + TICK_MARK_LEN(chart, t) + probe_h_t;
        align = FASTCHART_ALIGN_RIGHT;
    }

    /* Calendar-aware stride: emit ticks at unit boundaries (week
     * starts, month starts, etc.) instead of evenly-spaced dividers
     * across the range. Reverts to auto-density when every == 0. */
    if (chart->date_axis_every > 0) {
        zend_long every = chart->date_axis_every;
        struct tm tm_buf;
        time_t tt = (time_t)t_min;
        fc_gmtime(tt, &tm_buf);
        /* Snap start to the unit boundary at-or-after t_min. */
        switch (chart->date_axis_unit) {
            case FASTCHART_DATE_DAY:
                tm_buf.tm_hour = 0; tm_buf.tm_min = 0; tm_buf.tm_sec = 0;
                break;
            case FASTCHART_DATE_WEEK:
                /* Snap to Monday (tm_wday: Sun=0..Sat=6). */
                tm_buf.tm_hour = 0; tm_buf.tm_min = 0; tm_buf.tm_sec = 0;
                {
                    int dow = tm_buf.tm_wday == 0 ? 6 : tm_buf.tm_wday - 1;
                    tm_buf.tm_mday -= dow;
                }
                break;
            case FASTCHART_DATE_MONTH:
                tm_buf.tm_hour = 0; tm_buf.tm_min = 0; tm_buf.tm_sec = 0;
                tm_buf.tm_mday = 1;
                break;
            case FASTCHART_DATE_QUARTER:
                tm_buf.tm_hour = 0; tm_buf.tm_min = 0; tm_buf.tm_sec = 0;
                tm_buf.tm_mday = 1;
                tm_buf.tm_mon -= tm_buf.tm_mon % 3;
                break;
            case FASTCHART_DATE_YEAR:
                tm_buf.tm_hour = 0; tm_buf.tm_min = 0; tm_buf.tm_sec = 0;
                tm_buf.tm_mday = 1;
                tm_buf.tm_mon = 0;
                break;
        }
        time_t cur = fc_timegm(&tm_buf);
        if (cur < t_min) {
            /* Advance one unit so we always start inside the range. */
            switch (chart->date_axis_unit) {
                case FASTCHART_DATE_DAY:     tm_buf.tm_mday += 1; break;
                case FASTCHART_DATE_WEEK:    tm_buf.tm_mday += 7; break;
                case FASTCHART_DATE_MONTH:   tm_buf.tm_mon  += 1; break;
                case FASTCHART_DATE_QUARTER: tm_buf.tm_mon  += 3; break;
                case FASTCHART_DATE_YEAR:    tm_buf.tm_year += 1; break;
            }
            cur = fc_timegm(&tm_buf);
        }

        int n_emitted = 0;
        while (cur <= t_max && n_emitted < 64) {
            int x = fastchart_x_time_to_pixel(plot, (zend_long)cur, t_min, t_max);
            if (draw_points) {
                fastchart_target_line(t, x, plot->y1 + 1,
                                      x, plot->y1 + TICK_MARK_LEN(chart, t),
                                      pal->axis, 1, FASTCHART_DASH_SOLID);
            }
            if (draw_labels) {
                char buf[64];
                if (chart->x_axis_label_format) {
                    fastchart_format_tick_label_user((double)cur, chart->x_axis_label_format,
                                           buf, sizeof(buf));
                } else {
                    struct tm tm_lbl;
                    fc_gmtime(cur, &tm_lbl);
                    const char *fmt;
                    switch (chart->date_axis_unit) {
                        case FASTCHART_DATE_YEAR:    fmt = "%Y";       break;
                        case FASTCHART_DATE_QUARTER: fmt = "%Y-Q";     break;
                        case FASTCHART_DATE_MONTH:   fmt = "%Y-%m";    break;
                        default:                     fmt = "%Y-%m-%d"; break;
                    }
                    if (chart->date_axis_unit == FASTCHART_DATE_QUARTER) {
                        snprintf(buf, sizeof(buf), "%d-Q%d",
                                 tm_lbl.tm_year + 1900,
                                 (tm_lbl.tm_mon / 3) + 1);
                    } else {
                        strftime(buf, sizeof(buf), fmt, &tm_lbl);
                    }
                }
                if (angle == 0) {
                    fastchart_text_draw(t, font, size, label_color,
                                        x, label_y, align, buf, NULL, 0);
                } else {
                    fastchart_text_draw_rotated(t, font, size, label_color,
                                                x, label_y, align, (double)angle,
                                                buf, NULL, 0);
                }
            }
            /* Advance by `every` units. */
            for (zend_long e = 0; e < every; e++) {
                switch (chart->date_axis_unit) {
                    case FASTCHART_DATE_DAY:     tm_buf.tm_mday += 1; break;
                    case FASTCHART_DATE_WEEK:    tm_buf.tm_mday += 7; break;
                    case FASTCHART_DATE_MONTH:   tm_buf.tm_mon  += 1; break;
                    case FASTCHART_DATE_QUARTER: tm_buf.tm_mon  += 3; break;
                    case FASTCHART_DATE_YEAR:    tm_buf.tm_year += 1; break;
                }
            }
            cur = fc_timegm(&tm_buf);
            n_emitted++;
        }
        return;
    }

    /* Rotated labels are narrower so they can pack more densely. */
    const int N = (angle == 0) ? 5 : 8;
    for (int i = 0; i < N; i++) {
        zend_long ts = t_min + (zend_long)((double)(t_max - t_min) * i / (N - 1));
        int x = fastchart_x_time_to_pixel(plot, ts, t_min, t_max);
        if (draw_points) {
            fastchart_target_line(t, x, plot->y1 + 1,
                                  x, plot->y1 + TICK_MARK_LEN(chart, t),
                                  pal->axis, 1, FASTCHART_DASH_SOLID);
        }
        if (!draw_labels) continue;

        char buf[64];
        if (chart->x_axis_label_format) {
            /* Numeric format expects a double; pass the timestamp. */
            fastchart_format_tick_label_user((double)ts, chart->x_axis_label_format,
                                   buf, sizeof(buf));
        } else {
            struct tm tm_buf;
            time_t tt = (time_t)ts;
            fc_gmtime(tt, &tm_buf);
            strftime(buf, sizeof(buf), "%Y-%m-%d", &tm_buf);
        }

        if (angle == 0) {
            fastchart_text_draw(t, font, size, label_color,
                                x, label_y, align, buf, NULL, 0);
        } else {
            fastchart_text_draw_rotated(t, font, size, label_color,
                                        x, label_y, align, (double)angle,
                                        buf, NULL, 0);
        }
    }
}
