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

    /* X-axis: one row of labels at the standard size. */
    if (has_x_axis && font) {
        int lw, lh;
        if (fastchart_text_measure(font, size, "Mg9",
                                   &lw, &lh, NULL, 0) == 0) {
            bottom += lh + TICK_MARK_LEN + X_LABEL_PADDING;
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

int fastchart_y_to_pixel(double y,
                         const fastchart_value_range *range,
                         const fastchart_rect *plot)
{
    double span = range->max - range->min;
    if (span < 1e-12) return plot->y1;
    double frac = (y - range->min) / span;
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
    int label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.2);

    /* Cap rendered labels to ~10 to avoid overlap on dense categories. */
    int stride = 1;
    if (n_categories > 12) {
        stride = (n_categories + 9) / 10;
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
        fastchart_text_draw(im, font, size, pal->text,
                            x, label_y, FASTCHART_ALIGN_CENTER,
                            txt, NULL, 0);
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
    int label_y = plot->y1 + TICK_MARK_LEN + (int)(size * 1.2);

    /* 5 evenly-spaced ticks across the time range. */
    const int N = 5;
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

        fastchart_text_draw(im, font, size, pal->text,
                            x, label_y, FASTCHART_ALIGN_CENTER,
                            buf, NULL, 0);
    }
}
