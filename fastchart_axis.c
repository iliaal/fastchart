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
#include <time.h>

#include "php_fastchart.h"
#include "fastchart_axis.h"
#include "fastchart_palette.h"
#include "fastchart_text.h"
#include "fastchart_effects.h"

#define MARGIN_RIGHT_PAD       12
#define MARGIN_TOP_PAD          8
#define MARGIN_BOTTOM_PAD      10
#define MARGIN_LEFT_PAD         8
#define TICK_MARK_LEN           4
#define Y_LABEL_PADDING         6
#define X_LABEL_PADDING         6
#define TITLE_PADDING_BELOW    10

/* ------------------------- zval coercion -------------------------- */

int fastchart_zval_to_double(zval *zv, double *out)
{
    switch (Z_TYPE_P(zv)) {
        case IS_DOUBLE: *out = Z_DVAL_P(zv); return 0;
        case IS_LONG:   *out = (double)Z_LVAL_P(zv); return 0;
        default:        return -1;
    }
}

int fastchart_zval_to_long(zval *zv, long *out)
{
    switch (Z_TYPE_P(zv)) {
        case IS_LONG:   *out = (long)Z_LVAL_P(zv); return 0;
        case IS_DOUBLE: *out = (long)Z_DVAL_P(zv); return 0;
        default:        return -1;
    }
}

/* ----------------------- font resolution ------------------------ */

const char *fastchart_resolve_font(fastchart_obj *chart, const char *role)
{
    if (strcmp(role, "title") == 0 && chart->title_font_path) {
        return ZSTR_VAL(chart->title_font_path);
    }
    if (strcmp(role, "xaxis") == 0) {
        if (chart->x_axis_font_path) return ZSTR_VAL(chart->x_axis_font_path);
        if (chart->axis_font_path)   return ZSTR_VAL(chart->axis_font_path);
    }
    if (strcmp(role, "yaxis") == 0) {
        if (chart->y_axis_font_path) return ZSTR_VAL(chart->y_axis_font_path);
        if (chart->axis_font_path)   return ZSTR_VAL(chart->axis_font_path);
    }
    if (strcmp(role, "xtitle") == 0) {
        if (chart->x_axis_title_font_path) return ZSTR_VAL(chart->x_axis_title_font_path);
        if (chart->axis_font_path)         return ZSTR_VAL(chart->axis_font_path);
    }
    if (strcmp(role, "ytitle") == 0) {
        if (chart->y_axis_title_font_path) return ZSTR_VAL(chart->y_axis_title_font_path);
        if (chart->axis_font_path)         return ZSTR_VAL(chart->axis_font_path);
    }
    if (strcmp(role, "annotation") == 0 && chart->annotation_font_path) {
        return ZSTR_VAL(chart->annotation_font_path);
    }
    if (strcmp(role, "axis") == 0 && chart->axis_font_path) {
        return ZSTR_VAL(chart->axis_font_path);
    }
    if (strcmp(role, "label") == 0 && chart->label_font_path) {
        return ZSTR_VAL(chart->label_font_path);
    }
    return chart->font_path ? ZSTR_VAL(chart->font_path) : NULL;
}

double fastchart_resolve_font_size(fastchart_obj *chart, const char *role,
                                    double base_default)
{
    double sz = base_default;
    if (strcmp(role, "title") == 0 && chart->title_font_size > 0) {
        sz = chart->title_font_size;
    } else if (strcmp(role, "xaxis") == 0) {
        if      (chart->x_axis_font_size > 0) sz = chart->x_axis_font_size;
        else if (chart->axis_font_size > 0)   sz = chart->axis_font_size;
    } else if (strcmp(role, "yaxis") == 0) {
        if      (chart->y_axis_font_size > 0) sz = chart->y_axis_font_size;
        else if (chart->axis_font_size > 0)   sz = chart->axis_font_size;
    } else if (strcmp(role, "xtitle") == 0) {
        if      (chart->x_axis_title_font_size > 0) sz = chart->x_axis_title_font_size;
        else if (chart->axis_font_size > 0)         sz = chart->axis_font_size;
    } else if (strcmp(role, "ytitle") == 0) {
        if      (chart->y_axis_title_font_size > 0) sz = chart->y_axis_title_font_size;
        else if (chart->axis_font_size > 0)         sz = chart->axis_font_size;
    } else if (strcmp(role, "annotation") == 0 && chart->annotation_font_size > 0) {
        sz = chart->annotation_font_size;
    } else if (strcmp(role, "axis") == 0 && chart->axis_font_size > 0) {
        sz = chart->axis_font_size;
    } else if (strcmp(role, "label") == 0 && chart->label_font_size > 0) {
        sz = chart->label_font_size;
    }
    if (chart->thumbnail_mode) sz *= 0.6;
    return sz;
}

/* ------------------------ polyline drawer ----------------------- */

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

void fastchart_draw_polyline(gdImagePtr im, fastchart_obj *chart,
                             const fastchart_pt *pts, int n,
                             int color, int thickness, bool antialiased)
{
    if (n < 2) return;
    if (thickness > 1) gdImageSetThickness(im, thickness);
    /* Line style and antialiasing are mutually exclusive in libgd
     * (no gdStyled+gdAntiAliased compound). When the user picked
     * dashed/dotted, drop AA so the dash pattern is honored. */
    bool styled = (chart->line_style != FASTCHART_LINE_SOLID);
    int draw_color;
    if (styled) {
        draw_color = fastchart_apply_line_style(im, chart, color);
    } else {
        if (antialiased) gdImageSetAntiAliased(im, color);
        draw_color = antialiased ? gdAntiAliased : color;
    }

    if (chart->line_interpolation != FASTCHART_INTERP_SMOOTH) {
        /* Linear: connect consecutive valid points. */
        int prev_x = 0, prev_y = 0;
        bool prev_valid = false;
        for (int i = 0; i < n; i++) {
            if (!pts[i].valid) { prev_valid = false; continue; }
            if (prev_valid) {
                gdImageLine(im, prev_x, prev_y, pts[i].x, pts[i].y, draw_color);
            }
            prev_x = pts[i].x; prev_y = pts[i].y; prev_valid = true;
        }
    } else {
        /* Catmull-Rom: 10 sub-segments per valid pair, with the
         * surrounding two valid neighbors as control points (or
         * the endpoints themselves at boundaries / gap edges). */
        const int subdiv = 10;
        for (int i = 0; i < n - 1; i++) {
            if (!pts[i].valid || !pts[i + 1].valid) continue;
            int p0i = (i > 0 && pts[i - 1].valid) ? i - 1 : i;
            int p3i = (i + 2 < n && pts[i + 2].valid) ? i + 2 : i + 1;
            int prev_x = pts[i].x, prev_y = pts[i].y;
            for (int k = 1; k <= subdiv; k++) {
                double t = (double)k / (double)subdiv;
                int x, y;
                catmull_point(pts[p0i].x, pts[p0i].y,
                              pts[i].x,   pts[i].y,
                              pts[i + 1].x, pts[i + 1].y,
                              pts[p3i].x, pts[p3i].y,
                              t, &x, &y);
                gdImageLine(im, prev_x, prev_y, x, y, draw_color);
                prev_x = x; prev_y = y;
            }
        }
    }

    if (thickness > 1) gdImageSetThickness(im, 1);
}

/* ----------------------------- markers --------------------------- */

void fastchart_draw_marker(gdImagePtr im, int x, int y,
                           int style, int size, int color)
{
    if (style == FASTCHART_MARKER_NONE || size <= 0) return;
    int half = size / 2;
    if (half < 1) half = 1;

    switch (style) {
        case FASTCHART_MARKER_CIRCLE:
            gdImageFilledEllipse(im, x, y, size, size, color);
            break;
        case FASTCHART_MARKER_SQUARE:
            gdImageFilledRectangle(im, x - half, y - half,
                                   x + half, y + half, color);
            break;
        case FASTCHART_MARKER_DIAMOND: {
            gdPoint pts[4] = {
                { x,        y - half },
                { x + half, y        },
                { x,        y + half },
                { x - half, y        },
            };
            gdImageFilledPolygon(im, pts, 4, color);
            break;
        }
        case FASTCHART_MARKER_CROSS:
            gdImageSetThickness(im, 2);
            gdImageLine(im, x - half, y - half, x + half, y + half, color);
            gdImageLine(im, x - half, y + half, x + half, y - half, color);
            gdImageSetThickness(im, 1);
            break;
        case FASTCHART_MARKER_PLUS:
            gdImageSetThickness(im, 2);
            gdImageLine(im, x - half, y, x + half, y, color);
            gdImageLine(im, x, y - half, x, y + half, color);
            gdImageSetThickness(im, 1);
            break;
        default:
            break;
    }
}

/* ----------------------------- layout ----------------------------- */

void fastchart_compute_layout(fastchart_obj *chart, gdImagePtr im,
                              int has_y_axis, int has_x_axis,
                              fastchart_rect *out_plot)
{
    int W = gdImageSX(im);
    int H = gdImageSY(im);

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

    int top = MARGIN_TOP_PAD;
    int bottom = MARGIN_BOTTOM_PAD;
    int left = MARGIN_LEFT_PAD;
    int right = MARGIN_RIGHT_PAD;

    const char *font = chart->font_path ? ZSTR_VAL(chart->font_path) : NULL;
    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;

    /* Title: measure ascender height + a bit of padding. */
    if (chart->title && ZSTR_LEN(chart->title) > 0 && font) {
        int tw, th;
        if (fastchart_text_measure(font, size * 1.4, ZSTR_VAL(chart->title),
                                   &tw, &th, NULL, 0) == 0) {
            top += th + TITLE_PADDING_BELOW;
        }
    }

    /* Y-axis: reserve enough room for a 6-char numeric label. The
     * actual label width is data-dependent; we pick a conservative
     * "999999" sample so layout is stable across data ranges. */
    if (has_y_axis && font) {
        int lw, lh;
        if (fastchart_text_measure(font, size, "999999",
                                   &lw, &lh, NULL, 0) == 0) {
            left += lw + TICK_MARK_LEN + Y_LABEL_PADDING;
        }
    }

    /* Y-axis title: rotated 90deg on the left of the y-axis labels.
     * The title's height becomes its visible width after rotation. */
    if (has_y_axis && chart->y_axis_title && font) {
        int tw, th;
        if (fastchart_text_measure(font, size * 1.1, ZSTR_VAL(chart->y_axis_title),
                                   &tw, &th, NULL, 0) == 0) {
            (void)tw;
            left += th + 8;
        }
    }

    /* Secondary Y axis: mirror the left-side reservation on the
     * right edge. */
    if (has_y_axis && chart->secondary_y && font) {
        int lw, lh;
        if (fastchart_text_measure(font, size, "999999",
                                   &lw, &lh, NULL, 0) == 0) {
            right += lw + TICK_MARK_LEN + Y_LABEL_PADDING;
        }
    }

    /* X-axis labels. Horizontal labels reserve one line; rotated
     * labels reserve roughly the label width as height. The
     * rotated reservation is conservative for 45deg (true height
     * is width / sqrt(2) but layout-wise we don't need a tight fit). */
    if (has_x_axis && font) {
        int lw, lh;
        const char *probe = "999999"; /* same probe as y-axis */
        if (fastchart_text_measure(font, size, probe,
                                   &lw, &lh, NULL, 0) == 0) {
            int needed;
            if (chart->x_axis_label_angle == 90) {
                needed = lw + TICK_MARK_LEN + X_LABEL_PADDING;
            } else if (chart->x_axis_label_angle == 45) {
                needed = (int)((double)lw * 0.75) + TICK_MARK_LEN + X_LABEL_PADDING;
            } else {
                needed = lh + TICK_MARK_LEN + X_LABEL_PADDING;
            }
            bottom += needed;
        }
    }

    /* X-axis title: an extra line below the labels. */
    if (has_x_axis && chart->x_axis_title && font) {
        int tw, th;
        if (fastchart_text_measure(font, size * 1.1, ZSTR_VAL(chart->x_axis_title),
                                   &tw, &th, NULL, 0) == 0) {
            (void)tw;
            bottom += th + 6;
        }
    }

    out_plot->x0 = left;
    out_plot->y0 = top;
    out_plot->x1 = W - right - 1;
    out_plot->y1 = H - bottom - 1;

    if (out_plot->x1 < out_plot->x0 + 10) out_plot->x1 = out_plot->x0 + 10;
    if (out_plot->y1 < out_plot->y0 + 10) out_plot->y1 = out_plot->y0 + 10;
}

/* --------------------------- value range -------------------------- */

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

    /* Pick a "nice" tick step using the 1/2/5 × 10^N progression. */
    double range = dmax - dmin;
    double rough_step = range / (double)(target_ticks - 1);
    double mag = pow(10.0, floor(log10(rough_step)));
    double norm = rough_step / mag;
    double step;
    if      (norm < 1.5)  step = 1.0  * mag;
    else if (norm < 3.0)  step = 2.0  * mag;
    else if (norm < 4.0)  step = 2.5  * mag;
    else if (norm < 7.0)  step = 5.0  * mag;
    else                  step = 10.0 * mag;

    double nice_min = floor(dmin / step) * step;
    double nice_max = ceil(dmax / step) * step;

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

/* --------------------------- frame + title ------------------------- */

/* Load and composite a background image onto the canvas. Format
 * detected from extension. Silently no-ops on load failure -- the
 * chart still renders, just without the bg image. */
static void composite_bg_image(gdImagePtr im, const char *path)
{
    int W = gdImageSX(im);
    int H = gdImageSY(im);

    /* gdImageCreateFromFile picks the right loader by signature
     * (libgd 2.2.0+). Failure returns NULL. */
    gdImagePtr src = gdImageCreateFromFile(path);
    if (!src) return;

    gdImageCopyResampled(im, src, 0, 0, 0, 0,
                         W, H,
                         gdImageSX(src), gdImageSY(src));
    gdImageDestroy(src);
}

void fastchart_draw_frame(gdImagePtr im, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal)
{
    int W = gdImageSX(im);
    int H = gdImageSY(im);

    if (chart->transparent_bg) {
        /* Reserve the canvas with a fully-transparent fill so PNG /
         * WebP / AVIF outputs preserve alpha. gdImageSaveAlpha must
         * be enabled or the encoder collapses alpha to opaque. */
        int trans = gdImageColorAllocateAlpha(im, 0xFF, 0xFF, 0xFF, 127);
        gdImageSaveAlpha(im, 1);
        gdImageAlphaBlending(im, 0);
        gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, trans);
        gdImageAlphaBlending(im, 1);
    } else if (chart->bg_image_path) {
        gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal->bg);
        composite_bg_image(im, ZSTR_VAL(chart->bg_image_path));
    } else {
        gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal->bg);
    }

    /* Plot area background stays opaque so chart elements remain
     * readable on top of a transparent canvas or a busy bg image. */
    gdImageFilledRectangle(im, plot->x0, plot->y0, plot->x1, plot->y1, pal->plot_bg);

    /* Border-side bitmask: draw selected sides individually. The
     * Y-axis line gets its own dedicated draw call elsewhere, so
     * suppressing BORDER_LEFT here is safe (the Y axis still shows). */
    zend_long sides = chart->border_sides;
    if (sides & FASTCHART_BORDER_TOP)
        gdImageLine(im, plot->x0, plot->y0, plot->x1, plot->y0, pal->border);
    if (sides & FASTCHART_BORDER_BOTTOM)
        gdImageLine(im, plot->x0, plot->y1, plot->x1, plot->y1, pal->border);
    if (sides & FASTCHART_BORDER_LEFT)
        gdImageLine(im, plot->x0, plot->y0, plot->x0, plot->y1, pal->border);
    if (sides & FASTCHART_BORDER_RIGHT)
        gdImageLine(im, plot->x1, plot->y0, plot->x1, plot->y1, pal->border);
}

void fastchart_draw_title(gdImagePtr im, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal)
{
    if (!chart->title || ZSTR_LEN(chart->title) == 0) return;
    if (chart->thumbnail_mode) return;
    const char *font = fastchart_resolve_font(chart, "title");
    if (!font) return;

    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, "title", base * 1.4);
    int color = chart->title_color >= 0 ? (int)chart->title_color : pal->text;

    int W = gdImageSX(im);
    int cx = W / 2;
    int baseline = plot->y0 - TITLE_PADDING_BELOW;

    fastchart_text_draw(im, font, size, color, cx, baseline,
                        FASTCHART_ALIGN_CENTER, ZSTR_VAL(chart->title),
                        NULL, 0);
}

/* ------------------------------- y axis --------------------------- */

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

static void format_tick_label_user(double value, const zend_string *fmt,
                                   char *out, size_t out_n)
{
    /* Trust the user's sprintf string; we already rejected embedded
     * NULs at setter time. The callers always pass a small fixed
     * buffer so a runaway format truncates rather than overruns. */
    snprintf(out, out_n, ZSTR_VAL(fmt), value);
}

void fastchart_draw_y_axis(gdImagePtr im, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           const fastchart_value_range *range)
{
    if (!chart->y_axis_visible) return;

    /* Y axis line. */
    gdImageLine(im, plot->x0, plot->y0, plot->x0, plot->y1, pal->axis);

    const char *font = fastchart_resolve_font(chart, "yaxis");
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, "yaxis", base);
    int label_color = chart->axis_label_color >= 0 ? (int)chart->axis_label_color
                                                   : pal->text;

    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    char buf[32];
    for (int i = 0; i < range->n_ticks; i++) {
        double v = range->ticks[i];
        int y = fastchart_y_to_pixel(v, range, plot);

        /* Grid line across plot. */
        gdImageLine(im, plot->x0 + 1, y, plot->x1, y, pal->grid);

        /* Tick mark on the axis. */
        if (draw_points) {
            gdImageLine(im, plot->x0 - TICK_MARK_LEN, y,
                            plot->x0 - 1, y, pal->axis);
        }

        if (!draw_labels) continue;

        /* Label, right-aligned just to the left of the tick. */
        if (chart->y_axis_label_format) {
            format_tick_label_user(v, chart->y_axis_label_format, buf, sizeof(buf));
        } else {
            format_tick_label(v, range->tick_step, buf, sizeof(buf));
        }
        int label_x = plot->x0 - TICK_MARK_LEN - Y_LABEL_PADDING / 2;
        int label_y = y + (int)(size * 0.35);  /* baseline correction */
        fastchart_text_draw(im, font, size, label_color,
                            label_x, label_y, FASTCHART_ALIGN_RIGHT,
                            buf, NULL, 0);
    }

    /* Zero shelf: when the data range crosses zero, draw a heavier
     * horizontal axis-color line at y=0 to separate negative from
     * positive values visually. */
    if (chart->zero_shelf && range->min < 0.0 && range->max > 0.0) {
        int zy = fastchart_y_to_pixel(0.0, range, plot);
        gdImageLine(im, plot->x0 + 1, zy, plot->x1, zy, pal->axis);
    }
}

void fastchart_draw_y_axis_right(gdImagePtr im, fastchart_obj *chart,
                                 const fastchart_rect *plot,
                                 const fastchart_palette *pal,
                                 const fastchart_value_range *range)
{
    if (!chart->y_axis_visible) return;

    /* Right axis line. */
    gdImageLine(im, plot->x1, plot->y0, plot->x1, plot->y1, pal->axis);

    const char *font = fastchart_resolve_font(chart, "yaxis");
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, "yaxis", base);
    int label_color = chart->axis_label_color >= 0 ? (int)chart->axis_label_color
                                                   : pal->text;
    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    char buf[32];
    for (int i = 0; i < range->n_ticks; i++) {
        double v = range->ticks[i];
        int y = fastchart_y_to_pixel(v, range, plot);

        if (draw_points) {
            gdImageLine(im, plot->x1 + 1, y,
                            plot->x1 + TICK_MARK_LEN, y, pal->axis);
        }
        if (!draw_labels) continue;

        if (chart->y_axis_label_format) {
            format_tick_label_user(v, chart->y_axis_label_format, buf, sizeof(buf));
        } else {
            format_tick_label(v, range->tick_step, buf, sizeof(buf));
        }
        int label_x = plot->x1 + TICK_MARK_LEN + Y_LABEL_PADDING / 2;
        int label_y = y + (int)(size * 0.35);
        fastchart_text_draw(im, font, size, label_color,
                            label_x, label_y, FASTCHART_ALIGN_LEFT,
                            buf, NULL, 0);
    }
}

void fastchart_draw_axis_titles(gdImagePtr im, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal)
{
    if (chart->thumbnail_mode) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    int color = chart->axis_title_color >= 0 ? (int)chart->axis_title_color : pal->text;

    if (chart->x_axis_title && ZSTR_LEN(chart->x_axis_title) > 0) {
        const char *font = fastchart_resolve_font(chart, "xtitle");
        double size = fastchart_resolve_font_size(chart, "xtitle", base * 1.1);
        if (font) {
            int H = gdImageSY(im);
            int cx = (plot->x0 + plot->x1) / 2;
            int baseline = H - MARGIN_BOTTOM_PAD - 2;
            fastchart_text_draw(im, font, size, color,
                                cx, baseline, FASTCHART_ALIGN_CENTER,
                                ZSTR_VAL(chart->x_axis_title), NULL, 0);
        }
    }

    if (chart->y_axis_title && ZSTR_LEN(chart->y_axis_title) > 0) {
        const char *font = fastchart_resolve_font(chart, "ytitle");
        double size = fastchart_resolve_font_size(chart, "ytitle", base * 1.1);
        if (font) {
            int cy = (plot->y0 + plot->y1) / 2;
            int x = MARGIN_LEFT_PAD + (int)(size);
            int tw = 0, th = 0;
            if (fastchart_text_measure(font, size, ZSTR_VAL(chart->y_axis_title),
                                       &tw, &th, NULL, 0) == 0) {
                int y = cy + tw / 2;
                fastchart_text_draw_rotated(im, font, size, color,
                                            x, y, FASTCHART_ALIGN_LEFT, 90.0,
                                            ZSTR_VAL(chart->y_axis_title), NULL, 0);
            }
        }
    }

    if (chart->y_axis_title2 && ZSTR_LEN(chart->y_axis_title2) > 0 && chart->secondary_y) {
        const char *font = fastchart_resolve_font(chart, "ytitle");
        double size = fastchart_resolve_font_size(chart, "ytitle", base * 1.1);
        if (font) {
            int cy = (plot->y0 + plot->y1) / 2;
            int W = gdImageSX(im);
            int x = W - MARGIN_LEFT_PAD - (int)(size);
            int tw = 0, th = 0;
            if (fastchart_text_measure(font, size, ZSTR_VAL(chart->y_axis_title2),
                                       &tw, &th, NULL, 0) == 0) {
                int y = cy - tw / 2;
                fastchart_text_draw_rotated(im, font, size, color,
                                            x, y, FASTCHART_ALIGN_LEFT, 270.0,
                                            ZSTR_VAL(chart->y_axis_title2), NULL, 0);
            }
        }
    }
}

/* --------------------------- x axis (categorical) ------------------ */

int fastchart_x_categorical_center(const fastchart_rect *plot, int idx, int n)
{
    if (n <= 0) return plot->x0;
    int w = plot->x1 - plot->x0;
    /* Half-step at each end so first/last labels don't sit on the axis. */
    double step = (double)w / (double)n;
    return plot->x0 + (int)(step * (idx + 0.5));
}

void fastchart_draw_x_axis_categorical(gdImagePtr im, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       int n_categories,
                                       const char *const *labels)
{
    if (!chart->x_axis_visible) return;

    /* X axis line. */
    gdImageLine(im, plot->x0, plot->y1, plot->x1, plot->y1, pal->axis);

    if (n_categories <= 0) return;
    const char *font = fastchart_resolve_font(chart, "xaxis");
    if (!font) return;

    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, "xaxis", base);
    int angle = (int)chart->x_axis_label_angle;
    int label_color = chart->axis_label_color >= 0 ? (int)chart->axis_label_color
                                                   : pal->text;
    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    int label_y;
    fastchart_align align;
    if (angle == 0) {
        label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.2);
        align = FASTCHART_ALIGN_CENTER;
    } else if (angle == 45) {
        label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.0);
        align = FASTCHART_ALIGN_RIGHT;
    } else { /* 90 */
        label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 0.5);
        align = FASTCHART_ALIGN_RIGHT;
    }

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

    for (int i = 0; i < n_categories; i += stride) {
        int x = fastchart_x_categorical_center(plot, i, n_categories);
        if (draw_points) {
            gdImageLine(im, x, plot->y1 + 1, x,
                        plot->y1 + TICK_MARK_LEN, pal->axis);
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
            fastchart_text_draw(im, font, size, label_color,
                                x, label_y, align, txt, NULL, 0);
        } else {
            fastchart_text_draw_rotated(im, font, size, label_color,
                                        x, label_y, align, (double)angle,
                                        txt, NULL, 0);
        }
    }
}

/* --------------------------- x axis (time) ------------------------- */

int fastchart_x_time_to_pixel(const fastchart_rect *plot,
                              long ts, long t_min, long t_max)
{
    long span = t_max - t_min;
    if (span <= 0) return plot->x0;
    double frac = (double)(ts - t_min) / (double)span;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    int w = plot->x1 - plot->x0;
    return plot->x0 + (int)(frac * (double)w + 0.5);
}

/* --------------------------- legend ------------------------------- */

void fastchart_draw_legend(gdImagePtr im, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           int n_entries,
                           const int *colors,
                           const char *const *labels)
{
    if (n_entries < 1) return;
    if (!chart->font_path) return;
    if (chart->legend_position == FASTCHART_LEGEND_NONE) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = ZSTR_VAL(chart->font_path);

    int swatch_w  = 14;
    int swatch_h  = 10;
    int row_pad   = 4;
    int outer_pad = 6;
    int gap       = 6;     /* swatch -> text gap */
    int margin    = 6;     /* gap to plot border */

    int row_h, dummy;
    if (fastchart_text_measure(font, size, "Mg9", &dummy, &row_h, NULL, 0) != 0) {
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
        if (fastchart_text_measure(font, size, labels[i], &w, &h, NULL, 0) == 0) {
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

    gdImageFilledRectangle(im, x0, y0, x1, y1, pal->plot_bg);
    gdImageRectangle(im, x0, y0, x1, y1, pal->border);

    int row_y = y0 + outer_pad;
    for (int i = 0; i < n_entries; i++) {
        if (!labels[i]) continue;
        int sx0 = x0 + outer_pad;
        int sy0 = row_y + (row_h - swatch_h) / 2;
        gdImageFilledRectangle(im, sx0, sy0,
                               sx0 + swatch_w - 1, sy0 + swatch_h - 1,
                               colors[i]);
        gdImageRectangle(im, sx0, sy0,
                         sx0 + swatch_w - 1, sy0 + swatch_h - 1,
                         pal->border);

        int tx = sx0 + swatch_w + gap;
        int ty = row_y + row_h - 2;
        fastchart_text_draw(im, font, size, pal->text,
                            tx, ty, FASTCHART_ALIGN_LEFT,
                            labels[i], NULL, 0);

        row_y += row_h + row_pad;
    }
}

/* --------------------------- value labels ------------------------ */

void fastchart_draw_value_label(gdImagePtr im, fastchart_obj *chart,
                                const fastchart_palette *pal,
                                int x, int y, double value)
{
    if (!chart->show_values) return;
    if (!isfinite(value)) return;
    const char *font = fastchart_resolve_font(chart, "label");
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, "label", base * 0.85);

    const char *fmt = chart->value_format ? ZSTR_VAL(chart->value_format) : "%g";
    char buf[32];
    snprintf(buf, sizeof(buf), fmt, value);

    /* Place baseline a few pixels above the data point. */
    int label_y = y - 6;
    fastchart_text_draw(im, font, size, pal->text,
                        x, label_y, FASTCHART_ALIGN_CENTER, buf, NULL, 0);
}

/* --------------------------- combo overlays ---------------------- */

/* Allocate or reuse a color from an overlay's optional 'color' key.
 * Falls back to the rotating palette index `slot`. */
static int overlay_color(gdImagePtr im, const fastchart_palette *pal,
                         zval *entry, int slot)
{
    zval *c = zend_hash_str_find(Z_ARRVAL_P(entry), "color", sizeof("color") - 1);
    if (c && Z_TYPE_P(c) == IS_LONG) {
        long v = Z_LVAL_P(c);
        if (v >= 0 && v <= 0xFFFFFF) {
            return gdImageColorAllocate(im,
                (int)((v >> 16) & 0xFF),
                (int)((v >>  8) & 0xFF),
                (int)( v        & 0xFF));
        }
    }
    return pal->series[(slot + 4) % FASTCHART_PALETTE_SERIES_N];
}

static int overlay_thickness(zval *entry)
{
    zval *t = zend_hash_str_find(Z_ARRVAL_P(entry), "thickness", sizeof("thickness") - 1);
    if (t && Z_TYPE_P(t) == IS_LONG) {
        long v = Z_LVAL_P(t);
        if (v >= 1 && v <= 16) return (int)v;
    }
    return 2;
}

static bool overlay_right_axis(zval *entry)
{
    zval *a = zend_hash_str_find(Z_ARRVAL_P(entry), "axis", sizeof("axis") - 1);
    return (a && Z_TYPE_P(a) == IS_STRING && strcmp(Z_STRVAL_P(a), "right") == 0);
}

void fastchart_draw_overlays_categorical(gdImagePtr im, fastchart_obj *chart,
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
                        strcmp(Z_STRVAL_P(type_zv), "area") == 0);
        const fastchart_value_range *rng =
            (overlay_right_axis(entry) && yrange_right) ? yrange_right : yrange;
        int color = overlay_color(im, pal, entry, slot);
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
            int r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = color & 0xFF;
            (void)r; (void)g; (void)b;
            int alpha_color = gdImageColorAllocateAlpha(im,
                gdImageRed(im, color), gdImageGreen(im, color), gdImageBlue(im, color), 80);

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
            if (np >= 3) {
                gdImageAlphaBlending(im, 1);
                gdImageFilledPolygon(im, poly, np, alpha_color);
                gdImageAlphaBlending(im, 0);
            }
        }

        fastchart_draw_polyline(im, chart, pts, n_categories,
                                color, thick, !is_area);
        efree(pts);
        slot++;
    } ZEND_HASH_FOREACH_END();
}

void fastchart_draw_overlays_time(gdImagePtr im, fastchart_obj *chart,
                                  const fastchart_rect *plot,
                                  const fastchart_palette *pal,
                                  const fastchart_value_range *yrange,
                                  long t_min, long t_max,
                                  long *timestamps, int n_candles)
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

        int color = overlay_color(im, pal, entry, slot);
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
        fastchart_draw_polyline(im, chart, pts, n_candles, color, thick, true);
        efree(pts);
        slot++;
    } ZEND_HASH_FOREACH_END();
}

/* --------------------------- annotations ------------------------- */

/* Set a dashed style on im for subsequent gdStyled draws. The
 * pattern draws 6 px of color, then 4 px of transparent. */
static void set_dash_style(gdImagePtr im, int color)
{
    static int style[10];
    for (int i = 0; i < 6; i++) style[i] = color;
    for (int i = 6; i < 10; i++) style[i] = gdTransparent;
    gdImageSetStyle(im, style, 10);
}

static int annotation_color(const fastchart_palette *pal, gdImagePtr im,
                            zval *color_zv)
{
    if (color_zv && Z_TYPE_P(color_zv) == IS_LONG) {
        long c = (long)Z_LVAL_P(color_zv);
        if (c >= 0 && c <= 0xFFFFFF) {
            return gdImageColorAllocate(im,
                (int)((c >> 16) & 0xFF),
                (int)((c >>  8) & 0xFF),
                (int)( c        & 0xFF));
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

void fastchart_draw_h_annotations(gdImagePtr im, fastchart_obj *chart,
                                  const fastchart_rect *plot,
                                  const fastchart_palette *pal,
                                  const fastchart_value_range *yrange)
{
    zval *list = find_annotations(chart);
    if (!list) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = chart->font_path ? ZSTR_VAL(chart->font_path) : NULL;

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

        int color = annotation_color(pal, im,
            zend_hash_str_find(Z_ARRVAL_P(entry), "color", 5));
        set_dash_style(im, color);
        gdImageLine(im, plot->x0 + 1, y, plot->x1 - 1, y, gdStyled);

        zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "label", 5);
        if (label_zv && Z_TYPE_P(label_zv) == IS_STRING && font) {
            int tx = plot->x1 - 6;
            int ty = y - 4;  /* sit just above the line */
            fastchart_text_draw(im, font, size, color,
                                tx, ty, FASTCHART_ALIGN_RIGHT,
                                Z_STRVAL_P(label_zv), NULL, 0);
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

typedef struct { long t_min; long t_max; } v_time_ctx;
static int v_pos_time(const fastchart_rect *plot, double position, void *ctx)
{
    v_time_ctx *c = (v_time_ctx *)ctx;
    long ts = (long)position;
    if (ts < c->t_min || ts > c->t_max) return -1;
    return fastchart_x_time_to_pixel(plot, ts, c->t_min, c->t_max);
}

static void draw_v_annotations_with_mapper(gdImagePtr im, fastchart_obj *chart,
                                            const fastchart_rect *plot,
                                            const fastchart_palette *pal,
                                            v_pos_to_x mapper, void *ctx)
{
    zval *list = find_annotations(chart);
    if (!list) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = chart->font_path ? ZSTR_VAL(chart->font_path) : NULL;

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

        int color = annotation_color(pal, im,
            zend_hash_str_find(Z_ARRVAL_P(entry), "color", 5));
        set_dash_style(im, color);
        gdImageLine(im, x, plot->y0 + 1, x, plot->y1 - 1, gdStyled);

        zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(entry), "label", 5);
        if (label_zv && Z_TYPE_P(label_zv) == IS_STRING && font) {
            /* Place the label just below the top edge of the plot
             * area, centered on the annotation line. */
            int ty = plot->y0 + (int)(size * 1.2) + 2;
            fastchart_text_draw(im, font, size, color,
                                x + 4, ty, FASTCHART_ALIGN_LEFT,
                                Z_STRVAL_P(label_zv), NULL, 0);
        }
    } ZEND_HASH_FOREACH_END();
}

void fastchart_draw_v_annotations_categorical(gdImagePtr im, fastchart_obj *chart,
                                              const fastchart_rect *plot,
                                              const fastchart_palette *pal,
                                              int n_categories)
{
    int ctx = n_categories;
    draw_v_annotations_with_mapper(im, chart, plot, pal, v_pos_categorical, &ctx);
}

void fastchart_draw_v_annotations_continuous(gdImagePtr im, fastchart_obj *chart,
                                             const fastchart_rect *plot,
                                             const fastchart_palette *pal,
                                             const fastchart_value_range *xrange)
{
    v_continuous_ctx ctx = { xrange->min, xrange->max };
    draw_v_annotations_with_mapper(im, chart, plot, pal, v_pos_continuous, &ctx);
}

void fastchart_draw_v_annotations_time(gdImagePtr im, fastchart_obj *chart,
                                       const fastchart_rect *plot,
                                       const fastchart_palette *pal,
                                       long t_min, long t_max)
{
    v_time_ctx ctx = { t_min, t_max };
    draw_v_annotations_with_mapper(im, chart, plot, pal, v_pos_time, &ctx);
}

void fastchart_draw_text_annotations(gdImagePtr im, fastchart_obj *chart,
                                     const fastchart_palette *pal)
{
    zval *list_zv = zend_hash_str_find(Z_ARRVAL(chart->config),
                                       "text_annotations",
                                       sizeof("text_annotations") - 1);
    if (!list_zv || Z_TYPE_P(list_zv) != IS_ARRAY) return;

    const char *font = fastchart_resolve_font(chart, "annotation");
    if (!font) return;
    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, "annotation", base);

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
        if (c_zv && Z_TYPE_P(c_zv) == IS_LONG) color = (int)Z_LVAL_P(c_zv);

        fastchart_text_draw(im, font, size, color, x, y,
                            FASTCHART_ALIGN_LEFT, Z_STRVAL_P(t_zv), NULL, 0);
    } ZEND_HASH_FOREACH_END();
}

void fastchart_draw_x_axis_time(gdImagePtr im, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal,
                                long t_min, long t_max)
{
    if (!chart->x_axis_visible) return;
    gdImageLine(im, plot->x0, plot->y1, plot->x1, plot->y1, pal->axis);

    if (t_max <= t_min) return;
    const char *font = fastchart_resolve_font(chart, "xaxis");
    if (!font) return;

    double base = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(chart, "xaxis", base);
    int angle = (int)chart->x_axis_label_angle;
    int label_color = chart->axis_label_color >= 0 ? (int)chart->axis_label_color
                                                   : pal->text;
    bool draw_points = (chart->tick_mode & FASTCHART_TICK_POINTS) != 0;
    bool draw_labels = (chart->tick_mode & FASTCHART_TICK_LABELS) != 0;
    if (chart->thumbnail_mode) draw_labels = false;

    int label_y;
    fastchart_align align;
    if (angle == 0) {
        label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.2);
        align = FASTCHART_ALIGN_CENTER;
    } else {
        label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.0);
        align = FASTCHART_ALIGN_RIGHT;
    }

    /* Calendar-aware stride: emit ticks at unit boundaries (week
     * starts, month starts, etc.) instead of evenly-spaced dividers
     * across the range. Reverts to auto-density when every == 0. */
    if (chart->date_axis_every > 0) {
        long every = chart->date_axis_every;
        struct tm tm_buf;
        time_t tt = (time_t)t_min;
        gmtime_r(&tt, &tm_buf);
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
        time_t cur = timegm(&tm_buf);
        if (cur < t_min) {
            /* Advance one unit so we always start inside the range. */
            switch (chart->date_axis_unit) {
                case FASTCHART_DATE_DAY:     tm_buf.tm_mday += 1; break;
                case FASTCHART_DATE_WEEK:    tm_buf.tm_mday += 7; break;
                case FASTCHART_DATE_MONTH:   tm_buf.tm_mon  += 1; break;
                case FASTCHART_DATE_QUARTER: tm_buf.tm_mon  += 3; break;
                case FASTCHART_DATE_YEAR:    tm_buf.tm_year += 1; break;
            }
            cur = timegm(&tm_buf);
        }

        int n_emitted = 0;
        while (cur <= t_max && n_emitted < 64) {
            int x = fastchart_x_time_to_pixel(plot, (long)cur, t_min, t_max);
            if (draw_points) {
                gdImageLine(im, x, plot->y1 + 1, x,
                            plot->y1 + TICK_MARK_LEN, pal->axis);
            }
            if (draw_labels) {
                char buf[64];
                if (chart->x_axis_label_format) {
                    format_tick_label_user((double)cur, chart->x_axis_label_format,
                                           buf, sizeof(buf));
                } else {
                    struct tm tm_lbl;
                    gmtime_r(&cur, &tm_lbl);
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
                    fastchart_text_draw(im, font, size, label_color,
                                        x, label_y, align, buf, NULL, 0);
                } else {
                    fastchart_text_draw_rotated(im, font, size, label_color,
                                                x, label_y, align, (double)angle,
                                                buf, NULL, 0);
                }
            }
            /* Advance by `every` units. */
            for (long e = 0; e < every; e++) {
                switch (chart->date_axis_unit) {
                    case FASTCHART_DATE_DAY:     tm_buf.tm_mday += 1; break;
                    case FASTCHART_DATE_WEEK:    tm_buf.tm_mday += 7; break;
                    case FASTCHART_DATE_MONTH:   tm_buf.tm_mon  += 1; break;
                    case FASTCHART_DATE_QUARTER: tm_buf.tm_mon  += 3; break;
                    case FASTCHART_DATE_YEAR:    tm_buf.tm_year += 1; break;
                }
            }
            cur = timegm(&tm_buf);
            n_emitted++;
        }
        return;
    }

    /* Rotated labels are narrower so they can pack more densely. */
    const int N = (angle == 0) ? 5 : 8;
    for (int i = 0; i < N; i++) {
        long ts = t_min + (long)((double)(t_max - t_min) * i / (N - 1));
        int x = fastchart_x_time_to_pixel(plot, ts, t_min, t_max);
        if (draw_points) {
            gdImageLine(im, x, plot->y1 + 1, x,
                        plot->y1 + TICK_MARK_LEN, pal->axis);
        }
        if (!draw_labels) continue;

        char buf[64];
        if (chart->x_axis_label_format) {
            /* Numeric format expects a double; pass the timestamp. */
            format_tick_label_user((double)ts, chart->x_axis_label_format,
                                   buf, sizeof(buf));
        } else {
            struct tm tm_buf;
            time_t tt = (time_t)ts;
            gmtime_r(&tt, &tm_buf);
            strftime(buf, sizeof(buf), "%Y-%m-%d", &tm_buf);
        }

        if (angle == 0) {
            fastchart_text_draw(im, font, size, label_color,
                                x, label_y, align, buf, NULL, 0);
        } else {
            fastchart_text_draw_rotated(im, font, size, label_color,
                                        x, label_y, align, (double)angle,
                                        buf, NULL, 0);
        }
    }
}
