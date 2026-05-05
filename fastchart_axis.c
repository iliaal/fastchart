/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2026 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,     |
  | that is bundled with this package in the file LICENSE, and is       |
  | available through the world-wide-web at the following url:          |
  | http://www.php.net/license/3_01.txt                                 |
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

void fastchart_draw_frame(gdImagePtr im, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal)
{
    int W = gdImageSX(im);
    int H = gdImageSY(im);
    (void)chart;
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal->bg);
    gdImageFilledRectangle(im, plot->x0, plot->y0, plot->x1, plot->y1, pal->plot_bg);
    gdImageRectangle(im, plot->x0, plot->y0, plot->x1, plot->y1, pal->border);
}

void fastchart_draw_title(gdImagePtr im, fastchart_obj *chart,
                          const fastchart_rect *plot,
                          const fastchart_palette *pal)
{
    if (!chart->title || ZSTR_LEN(chart->title) == 0) return;
    if (!chart->font_path) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    int W = gdImageSX(im);
    int cx = W / 2;
    /* Place the title baseline so the text sits above the plot area. */
    int baseline = plot->y0 - TITLE_PADDING_BELOW;

    fastchart_text_draw(im, ZSTR_VAL(chart->font_path), size * 1.4,
                        pal->text, cx, baseline,
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

void fastchart_draw_y_axis(gdImagePtr im, fastchart_obj *chart,
                           const fastchart_rect *plot,
                           const fastchart_palette *pal,
                           const fastchart_value_range *range)
{
    /* Y axis line. */
    gdImageLine(im, plot->x0, plot->y0, plot->x0, plot->y1, pal->axis);

    if (!chart->font_path) return;
    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = ZSTR_VAL(chart->font_path);

    char buf[32];
    for (int i = 0; i < range->n_ticks; i++) {
        double v = range->ticks[i];
        int y = fastchart_y_to_pixel(v, range, plot);

        /* Grid line across plot. */
        gdImageLine(im, plot->x0 + 1, y, plot->x1, y, pal->grid);

        /* Tick mark on the axis. */
        gdImageLine(im, plot->x0 - TICK_MARK_LEN, y,
                        plot->x0 - 1, y, pal->axis);

        /* Label, right-aligned just to the left of the tick. */
        format_tick_label(v, range->tick_step, buf, sizeof(buf));
        int label_x = plot->x0 - TICK_MARK_LEN - Y_LABEL_PADDING / 2;
        int label_y = y + (int)(size * 0.35);  /* baseline correction */
        fastchart_text_draw(im, font, size, pal->text,
                            label_x, label_y, FASTCHART_ALIGN_RIGHT,
                            buf, NULL, 0);
    }
}

void fastchart_draw_y_axis_right(gdImagePtr im, fastchart_obj *chart,
                                 const fastchart_rect *plot,
                                 const fastchart_palette *pal,
                                 const fastchart_value_range *range)
{
    /* Right axis line. */
    gdImageLine(im, plot->x1, plot->y0, plot->x1, plot->y1, pal->axis);
    if (!chart->font_path) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = ZSTR_VAL(chart->font_path);

    char buf[32];
    for (int i = 0; i < range->n_ticks; i++) {
        double v = range->ticks[i];
        int y = fastchart_y_to_pixel(v, range, plot);

        gdImageLine(im, plot->x1 + 1, y,
                        plot->x1 + TICK_MARK_LEN, y, pal->axis);

        format_tick_label(v, range->tick_step, buf, sizeof(buf));
        int label_x = plot->x1 + TICK_MARK_LEN + Y_LABEL_PADDING / 2;
        int label_y = y + (int)(size * 0.35);
        fastchart_text_draw(im, font, size, pal->text,
                            label_x, label_y, FASTCHART_ALIGN_LEFT,
                            buf, NULL, 0);
    }
}

void fastchart_draw_axis_titles(gdImagePtr im, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal)
{
    if (!chart->font_path) return;
    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = ZSTR_VAL(chart->font_path);

    if (chart->x_axis_title && ZSTR_LEN(chart->x_axis_title) > 0) {
        int W = gdImageSX(im);
        int H = gdImageSY(im);
        (void)W;
        int cx = (plot->x0 + plot->x1) / 2;
        /* Place the X-title baseline near the bottom of the canvas
         * with a small bottom margin. */
        int baseline = H - MARGIN_BOTTOM_PAD - 2;
        fastchart_text_draw(im, font, size * 1.1, pal->text,
                            cx, baseline, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(chart->x_axis_title), NULL, 0);
    }

    if (chart->y_axis_title && ZSTR_LEN(chart->y_axis_title) > 0) {
        /* Y-title reads bottom-to-top centered on the plot's
         * vertical midpoint. gdImageStringFT with angle=π/2 starts
         * at (x, y) and progresses upward; the unrotated text width
         * becomes the rotated text's vertical extent. To visually
         * center on cy, set the baseline y = cy + width / 2 so the
         * top edge ends up at cy - width / 2. */
        int cy = (plot->y0 + plot->y1) / 2;
        int x = MARGIN_LEFT_PAD + (int)(size * 1.1);
        int tw = 0, th = 0;
        if (fastchart_text_measure(font, size * 1.1, ZSTR_VAL(chart->y_axis_title),
                                   &tw, &th, NULL, 0) == 0) {
            int y = cy + tw / 2;
            fastchart_text_draw_rotated(im, font, size * 1.1, pal->text,
                                        x, y, FASTCHART_ALIGN_LEFT, 90.0,
                                        ZSTR_VAL(chart->y_axis_title), NULL, 0);
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
    /* X axis line. */
    gdImageLine(im, plot->x0, plot->y1, plot->x1, plot->y1, pal->axis);

    if (n_categories <= 0) return;
    if (!chart->font_path) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = ZSTR_VAL(chart->font_path);
    int angle = (int)chart->x_axis_label_angle;

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
     * narrow enough that we can show all of them up to ~30. */
    int max_visible = (angle == 0) ? 10 : 30;
    int stride = 1;
    if (n_categories > max_visible + 2) {
        stride = (n_categories + max_visible - 1) / max_visible;
    }

    for (int i = 0; i < n_categories; i += stride) {
        int x = fastchart_x_categorical_center(plot, i, n_categories);
        gdImageLine(im, x, plot->y1 + 1, x,
                    plot->y1 + TICK_MARK_LEN, pal->axis);

        char fallback[16];
        const char *txt;
        if (labels && labels[i]) {
            txt = labels[i];
        } else {
            snprintf(fallback, sizeof(fallback), "%d", i);
            txt = fallback;
        }
        if (angle == 0) {
            fastchart_text_draw(im, font, size, pal->text,
                                x, label_y, align, txt, NULL, 0);
        } else {
            fastchart_text_draw_rotated(im, font, size, pal->text,
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

void fastchart_draw_x_axis_time(gdImagePtr im, fastchart_obj *chart,
                                const fastchart_rect *plot,
                                const fastchart_palette *pal,
                                long t_min, long t_max)
{
    gdImageLine(im, plot->x0, plot->y1, plot->x1, plot->y1, pal->axis);

    if (t_max <= t_min) return;
    if (!chart->font_path) return;

    double size = chart->font_size > 0 ? chart->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    const char *font = ZSTR_VAL(chart->font_path);
    int angle = (int)chart->x_axis_label_angle;

    int label_y;
    fastchart_align align;
    if (angle == 0) {
        label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.2);
        align = FASTCHART_ALIGN_CENTER;
    } else {
        label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.0);
        align = FASTCHART_ALIGN_RIGHT;
    }

    /* Rotated labels are narrower so they can pack more densely. */
    const int N = (angle == 0) ? 5 : 8;
    for (int i = 0; i < N; i++) {
        long ts = t_min + (long)((double)(t_max - t_min) * i / (N - 1));
        int x = fastchart_x_time_to_pixel(plot, ts, t_min, t_max);
        gdImageLine(im, x, plot->y1 + 1, x,
                    plot->y1 + TICK_MARK_LEN, pal->axis);

        struct tm tm_buf;
        time_t tt = (time_t)ts;
        gmtime_r(&tt, &tm_buf);
        char buf[16];
        strftime(buf, sizeof(buf), "%Y-%m-%d", &tm_buf);

        if (angle == 0) {
            fastchart_text_draw(im, font, size, pal->text,
                                x, label_y, align, buf, NULL, 0);
        } else {
            fastchart_text_draw_rotated(im, font, size, pal->text,
                                        x, label_y, align, (double)angle,
                                        buf, NULL, 0);
        }
    }
}
