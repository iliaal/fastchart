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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

#define MAX_POINTS 8192
#define FASTCHART_MAX_SCATTER_SERIES 8

int fastchart_scatter_render_to_image(fastchart_scatter_obj *self, gdImagePtr im)
{
    if (self->point_count == 0) {
        zend_throw_error(NULL,
            "FastChart\\ScatterChart::draw() requires setPoints() with one or more [x, y] pairs");
        return -1;
    }
    fastchart_scatter_point *points = self->points;
    int n = self->point_count;
    int n_series = self->n_series > 0 ? self->n_series : 1;

    /* Y range from data. */
    double y_min = points[0].y, y_max = points[0].y;
    double x_min = points[0].x, x_max = points[0].x;
    for (int i = 1; i < n; i++) {
        if (points[i].y < y_min) y_min = points[i].y;
        if (points[i].y > y_max) y_max = points[i].y;
        if (points[i].x < x_min) x_min = points[i].x;
        if (points[i].x > x_max) x_max = points[i].x;
    }

    fastchart_value_range yrange;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (fastchart_value_range_compute_log(y_min, y_max, &yrange) != 0) {
            zend_value_error("FastChart\\ScatterChart::draw(): log Y-axis requires strictly-positive Y values");
            return -1;
        }
    } else {
        fastchart_value_range_compute(y_min, y_max, 6, &yrange);
        fastchart_value_range_apply_override((fastchart_obj *)self, &yrange);
    }

    /* X range. */
    fastchart_value_range xrange;
    fastchart_value_range_compute(x_min, x_max, 6, &xrange);

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    fastchart_color_cache color_cache;
    fastchart_color_cache_init(&color_cache);

    fastchart_draw_frame(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(im, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_plot_bands(im, (fastchart_obj *)self, &plot, &yrange, &pal);
    fastchart_draw_v_plot_bands_xrange(im, (fastchart_obj *)self, &plot,
                                       &xrange, &pal);

    /* Custom X axis: continuous numeric ticks from xrange. Reuse the
     * categorical axis line + tick infrastructure by drawing the line
     * and emitting ticks at xrange values. */
    gdImageLine(im, plot.x0, plot.y1, plot.x1, plot.y1, pal.axis);
    const char *font = xrange.n_ticks > 0
        ? fastchart_resolve_font((fastchart_obj *)self, FC_FONT_AXIS) : NULL;
    if (font) {
        double size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        /* Match the shared numeric x-axis: scale tick length and
         * label offset with DPI, and use a measured ascender so the
         * label TOP sits below plot.y1 + tick rather than clipping
         * into the plot rect at higher DPIs. */
        double dpi_scale = ((fastchart_obj *)self)->dpi > 96
            ? (double)((fastchart_obj *)self)->dpi / 96.0 : 1.0;
        int tick_len = (int)(4 * dpi_scale + 0.5);
        int probe_h = 0;
        if (fastchart_text_measure(im, font, size, "Mg9", NULL, &probe_h, NULL, 0) != 0) {
            probe_h = (int)(size * 1.2 * dpi_scale);
        }
        int label_y = plot.y1 + tick_len + probe_h + (int)(4 * dpi_scale);
        for (int t = 0; t < xrange.n_ticks; t++) {
            double v = xrange.ticks[t];
            double frac = (v - xrange.min) / (xrange.max - xrange.min);
            int x = plot.x0 + (int)(frac * (plot.x1 - plot.x0) + 0.5);
            gdImageLine(im, x, plot.y1 + 1, x, plot.y1 + tick_len, pal.axis);

            char buf[32];
            if (self->x_axis_label_format) {
                fastchart_format_tick_label_user(v, self->x_axis_label_format,
                                                 buf, sizeof(buf));
            } else {
                int decimals = xrange.tick_step >= 1.0 ? 0
                    : (int)ceil(-log10(xrange.tick_step));
                if (decimals < 0) decimals = 0;
                if (decimals > 4) decimals = 4;
                snprintf(buf, sizeof(buf), "%.*f", decimals, v);
            }

            fastchart_text_draw(im, font, size, pal.text,
                                x, label_y, FASTCHART_ALIGN_CENTER,
                                buf, NULL, 0);
        }
    }

    fastchart_draw_axis_titles(im, (fastchart_obj *)self, &plot, &pal);

    /* Marker resolution: ScatterChart's default is a 7px circle. */
    int marker_style = self->marker_style >= 0
        ? (int)self->marker_style
        : FASTCHART_MARKER_CIRCLE;
    int marker_size = self->marker_size >= 1
        ? (int)self->marker_size
        : 7;

    /* Optional per-point error bars (parallel to setPoints index order). */
    double *err_lo = self->err_lo;
    double *err_hi = self->err_hi;
    int err_n = self->err_n;

    for (int i = 0; i < n; i++) {
        double frac_x = (xrange.max - xrange.min) > 0
            ? (points[i].x - xrange.min) / (xrange.max - xrange.min)
            : 0.5;
        int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
        int py = fastchart_y_to_pixel(points[i].y, &yrange, &plot);

        int color;
        if (points[i].color_rgb >= 0) {
            color = fastchart_color_cache_get(&color_cache, im, points[i].color_rgb);
        } else {
            color = pal.series[points[i].series_idx % FASTCHART_PALETTE_SERIES_N];
        }

        /* Error bar before marker so the marker overdraws the stem
         * cleanly. Typed err_lo/err_hi already parsed at setErrorBars
         * time: NaN slot means "no error bar at this point". */
        if (err_lo && err_n > 0 && i < err_n) {
            double lo = err_lo[i];
            double hi = err_hi[i];
            if (!(isnan(lo) && isnan(hi))) {
                if (isnan(lo)) lo = 0;
                if (isnan(hi)) hi = 0;
                if (lo > 0 || hi > 0) {
                    int py_lo = fastchart_y_to_pixel(points[i].y - lo, &yrange, &plot);
                    int py_hi = fastchart_y_to_pixel(points[i].y + hi, &yrange, &plot);
                    gdImageLine(im, px, py_hi, px, py_lo, pal.axis);
                    gdImageLine(im, px - 4, py_hi, px + 4, py_hi, pal.axis);
                    gdImageLine(im, px - 4, py_lo, px + 4, py_lo, pal.axis);
                }
            }
        }

        fastchart_draw_marker(im, px, py, marker_style, marker_size, color);
        fastchart_draw_value_label(im, (fastchart_obj *)self, &pal, px, py, points[i].y);
    }

    /* Trend line: least-squares fit. Linear (degree=1) uses the
     * closed-form 2x2 solution; polynomial (degree>=2) builds the
     * normal-equations matrix and solves via Gaussian elimination.
     * The fitted curve is rendered as 200 sub-segments across the
     * x range. */
    if (self->trend_line && n >= 2) {
        int deg = (int)self->trend_degree;
        if (deg < 1) deg = 1;
        /* Cap at 3. Quintic / quartic fits over raw scatter data
         * are essentially never the right answer — they overfit
         * noise and the high-order Vandermonde is numerically
         * fragile even with normalization. Three is high enough
         * for the vast majority of "is there a trend?" use cases. */
        if (deg > 3) deg = 3;
        if (deg + 1 > n) deg = n - 1;
        if (deg < 1) deg = 1;

        int color = self->trend_line_color >= 0
            ? gdImageColorAllocate(im,
                (int)((self->trend_line_color >> 16) & 0xFF),
                (int)((self->trend_line_color >>  8) & 0xFF),
                (int)( self->trend_line_color        & 0xFF))
            : pal.axis;

        /* Normalize x to xn = (x - x_mid) / x_half so xn ∈ [-1, 1]
         * across the input range. The Vandermonde of normalized x
         * is several orders of magnitude better-conditioned than
         * raw x for any non-trivial domain (e.g. timestamps near
         * 1.7e9 + degree 3 produces matrix entries near 1e30, well
         * past double-precision recovery). Evaluation re-applies
         * the same normalization. */
        double x_mid  = 0.5 * (x_min + x_max);
        double x_half = 0.5 * (x_max - x_min);
        if (x_half <= 0) x_half = 1.0;

        double coeffs[4] = {0};
        if (deg == 1) {
            double sx = 0, sy = 0, sxx = 0, sxy = 0;
            for (int i = 0; i < n; i++) {
                double xn = (points[i].x - x_mid) / x_half;
                sx  += xn;
                sy  += points[i].y;
                sxx += xn * xn;
                sxy += xn * points[i].y;
            }
            double denom = n * sxx - sx * sx;
            if (denom != 0.0) {
                coeffs[1] = (n * sxy - sx * sy) / denom;
                coeffs[0] = (sy - coeffs[1] * sx) / n;
            }
        } else {
            /* Normal equations for polynomial of degree `deg` in
             * normalized x:
             *   A[k][j] = sum xn^(j+k)        for j,k in 0..deg
             *   b[k]    = sum y * xn^k
             * Solve A * c = b (size deg+1) via partial-pivot Gauss. */
            int m = deg + 1;
            double A[4][5] = {{0}};   /* augmented [m | b] */
            for (int i = 0; i < n; i++) {
                double xn = (points[i].x - x_mid) / x_half;
                double yi = points[i].y;
                double xpow_row[8]; /* xn^0 .. xn^(2*deg) for deg<=3 */
                xpow_row[0] = 1.0;
                for (int p = 1; p <= 2 * deg; p++) xpow_row[p] = xpow_row[p-1] * xn;
                for (int k = 0; k < m; k++) {
                    for (int j = 0; j < m; j++) {
                        A[k][j] += xpow_row[j + k];
                    }
                    A[k][m] += yi * xpow_row[k];
                }
            }
            /* Gauss-Jordan with partial pivoting. */
            for (int k = 0; k < m; k++) {
                int piv = k;
                double best = fabs(A[k][k]);
                for (int r = k + 1; r < m; r++) {
                    if (fabs(A[r][k]) > best) { best = fabs(A[r][k]); piv = r; }
                }
                if (best < 1e-12) { /* singular -- skip */ goto no_fit; }
                if (piv != k) {
                    for (int c = 0; c <= m; c++) {
                        double tmp = A[k][c]; A[k][c] = A[piv][c]; A[piv][c] = tmp;
                    }
                }
                double pivval = A[k][k];
                for (int c = 0; c <= m; c++) A[k][c] /= pivval;
                for (int r = 0; r < m; r++) {
                    if (r == k) continue;
                    double f = A[r][k];
                    if (f == 0) continue;
                    for (int c = 0; c <= m; c++) A[r][c] -= f * A[k][c];
                }
            }
            for (int k = 0; k < m; k++) coeffs[k] = A[k][m];
        }

        /* Plot 200 sub-segments. Normalize x exactly as the fit did. */
        const int N = 200;
        gdImageSetThickness(im, 2);
        int prev_px = 0, prev_py = 0;
        for (int s = 0; s <= N; s++) {
            double t = (double)s / (double)N;
            double x = x_min + t * (x_max - x_min);
            double xn = (x - x_mid) / x_half;
            double y = 0;
            double xp = 1.0;
            for (int k = 0; k <= deg; k++) { y += coeffs[k] * xp; xp *= xn; }
            double frac = (xrange.max - xrange.min) > 0
                ? (x - xrange.min) / (xrange.max - xrange.min) : 0.5;
            int px = plot.x0 + (int)(frac * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(y, &yrange, &plot);
            if (s > 0) gdImageLine(im, prev_px, prev_py, px, py, color);
            prev_px = px; prev_py = py;
        }
        gdImageSetThickness(im, 1);
        no_fit: ;
    }

    fastchart_draw_h_annotations(im, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_v_annotations_continuous(im, (fastchart_obj *)self, &plot, &pal, &xrange);

    if (n_series >= 2) {
        int legend_colors[FASTCHART_MAX_SCATTER_SERIES];
        const char *legend_labels[FASTCHART_MAX_SCATTER_SERIES];
        int legend_count = 0;
        for (int s = 0; s < n_series; s++) {
            if (!self->series_labels[s]) continue;
            legend_colors[legend_count] = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            legend_labels[legend_count] = self->series_labels[s];
            legend_count++;
        }
        if (legend_count > 0) {
            fastchart_draw_legend(im, (fastchart_obj *)self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);

    /* IconPlot overlays at data coordinates. x is data-x in xrange,
     * y is data-y in yrange. Drawn on top of markers + annotations. */
    if (self->icons && self->n_icons > 0) {
        fastchart_obj *base = (fastchart_obj *)self;
        for (int i = 0; i < base->n_icons; i++) {
            const fastchart_icon *ic = &base->icons[i];
            double frac_x = (xrange.max - xrange.min) > 0
                ? (ic->x - xrange.min) / (xrange.max - xrange.min)
                : 0.5;
            int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(ic->y, &yrange, &plot);
            fastchart_blit_icon(im, ic, px, py);
        }
    }

    /* Build the image-map area list from typed points. The href and
     * tooltip pointers borrow from points[i] — same lifetime since
     * the area list is freed/repopulated on every render. */
    if (self->image_map_areas) {
        efree(self->image_map_areas);
        self->image_map_areas = NULL;
    }
    self->n_image_map_areas = 0;
    int href_count = 0;
    for (int i = 0; i < n; i++) {
        if (points[i].href) href_count++;
    }
    if (href_count > 0) {
        self->image_map_areas = ecalloc((size_t)href_count, sizeof(fastchart_image_map_area));
        int k = 0;
        for (int i = 0; i < n; i++) {
            if (!points[i].href) continue;
            double frac_x = (xrange.max - xrange.min) > 0
                ? (points[i].x - xrange.min) / (xrange.max - xrange.min)
                : 0.5;
            int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(points[i].y, &yrange, &plot);
            self->image_map_areas[k].x = px;
            self->image_map_areas[k].y = py;
            self->image_map_areas[k].r = marker_size;
            self->image_map_areas[k].href = points[i].href;
            self->image_map_areas[k].tooltip = points[i].tooltip;
            k++;
        }
        self->n_image_map_areas = k;
    }
    return 0;
}

ZEND_METHOD(FastChart_ScatterChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\ScatterChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();

    fastchart_scatter_obj *self = Z_FASTCHART_SCATTER_OBJ_P(ZEND_THIS);
    if (fastchart_scatter_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
