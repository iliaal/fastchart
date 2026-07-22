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
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

#define MAX_POINTS 8192
#define FASTCHART_MAX_SCATTER_SERIES 8

int fastchart_scatter_render_to_target(fastchart_scatter_obj *self, fastchart_target_t *t)
{
	fastchart_reset_image_map_areas((fastchart_obj *)self);
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

    if (self->err_lo && self->err_n > 0) {
        int lim = n < self->err_n ? n : self->err_n;
        for (int i = 0; i < lim; i++) {
            double lo = self->err_lo[i];
            double hi = self->err_hi[i];
            if (!isnan(lo)) {
                double edge = points[i].y - lo;
                if (isfinite(edge)
                    && (self->y_axis_scale != FASTCHART_SCALE_LOG || edge > 0.0)
                    && edge < y_min) {
                    y_min = edge;
                }
            }
            if (!isnan(hi)) {
                double edge = points[i].y + hi;
                if (isfinite(edge) && edge > y_max) y_max = edge;
            }
        }
    }

    fastchart_value_range yrange;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (fastchart_value_range_compute_log(y_min, y_max, &yrange) != 0) {
            zend_value_error("FastChart\\ScatterChart::draw(): log Y-axis requires strictly-positive Y values");
            return -1;
        }
    } else {
        fastchart_value_range_compute(y_min, y_max, 6, &yrange);
		if (fastchart_value_range_apply_override((fastchart_obj *)self,
				&yrange) != 0) {
			return -1;
		}
    }

    /* X range. */
    fastchart_value_range xrange;
    fastchart_value_range_compute(x_min, x_max, 6, &xrange);

    fastchart_rect plot;
    fastchart_palette pal;
    fastchart_render_cartesian_setup((fastchart_obj *)self, t, 1, 1, NULL, 0,
                                     &plot, &pal);
    fastchart_draw_y_axis(t, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_plot_bands(t, (fastchart_obj *)self, &plot, &yrange, &pal);
    fastchart_draw_v_plot_bands_xrange(t, (fastchart_obj *)self, &plot,
                                       &xrange, &pal);

    /* Continuous numeric X axis: the shared drawer honors
     * setXAxisVisible / thumbnail mode / the axis font+size and keeps
     * SVG output DPI-invariant (the hand-rolled block scaled ticks by
     * raw DPI even for SVG targets, breaking that contract). */
    fastchart_draw_x_axis_numeric(t, (fastchart_obj *)self, &plot, &pal,
                                  &xrange);

    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

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
	double x_span = xrange.max - xrange.min;
	bool x_span_fast = isfinite(x_span) && x_span > 0.0;

    for (int i = 0; i < n; i++) {
		double frac_x = EXPECTED(x_span_fast)
			? (points[i].x - xrange.min) / x_span
			: fastchart_normalize_finite(points[i].x,
				xrange.min, xrange.max);
        int px = fastchart_frac_to_px(frac_x, plot.x0, plot.x1);
        int py = fastchart_y_to_pixel(points[i].y, &yrange, &plot);

        /* color is a target handle so it flows through fastchart_draw_marker
         * and fastchart_draw_value_label unchanged. Per-point RGB overrides
         * route through the target's own dedupe table (color_rgba[]). */
        int color;
        if (points[i].color_rgb >= 0) {
            color = fastchart_target_color_rgb(t, points[i].color_rgb);
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
                    fastchart_target_line(t, px, py_hi, px, py_lo, pal.axis, 1, FASTCHART_DASH_SOLID);
                    fastchart_target_line(t, px - 4, py_hi, px + 4, py_hi, pal.axis, 1, FASTCHART_DASH_SOLID);
                    fastchart_target_line(t, px - 4, py_lo, px + 4, py_lo, pal.axis, 1, FASTCHART_DASH_SOLID);
                }
            }
        }

        fastchart_draw_marker(t, px, py, marker_style, marker_size, color);
        fastchart_draw_value_label(t, (fastchart_obj *)self, &pal, px, py, points[i].y);
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
            ? fastchart_target_color_rgb(t, (int)self->trend_line_color)
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
            /* All-equal x (e.g. a vertical scatter) makes denom 0; the
             * fit is undefined. Skip like the singular-matrix case below
             * rather than plotting the zero-initialized y=0 line. */
            if (denom == 0.0 || !isfinite(denom)) goto no_fit;
            coeffs[1] = (n * sxy - sx * sy) / denom;
            coeffs[0] = (sy - coeffs[1] * sx) / n;
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
        int prev_px = 0, prev_py = 0;
        for (int s = 0; s <= N; s++) {
            double frac_s = (double)s / (double)N;
            double x = x_min + frac_s * (x_max - x_min);
            double xn = (x - x_mid) / x_half;
            double y = 0;
            double xp = 1.0;
            for (int k = 0; k <= deg; k++) { y += coeffs[k] * xp; xp *= xn; }
			double frac = fastchart_normalize_finite(x, xrange.min,
				xrange.max);
            int px = fastchart_frac_to_px(frac, plot.x0, plot.x1);
            int py = fastchart_y_to_pixel(y, &yrange, &plot);
            if (s > 0) {
                fastchart_target_line(t, prev_px, prev_py, px, py,
                                      color, 2, FASTCHART_DASH_SOLID);
            }
            prev_px = px; prev_py = py;
        }
        no_fit: ;
    }

    fastchart_draw_h_annotations(t, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_v_annotations_continuous(t, (fastchart_obj *)self, &plot, &pal, &xrange);

    fastchart_draw_series_legend(t, (fastchart_obj *)self, &plot, &pal,
                                 n_series,
                                 (const char *const *)self->series_labels);

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);

    /* IconPlot overlays at data coordinates. x is data-x in xrange,
     * y is data-y in yrange. Drawn on top of markers + annotations. */
    if (self->icons && self->n_icons > 0) {
        fastchart_obj *base = (fastchart_obj *)self;
        for (int i = 0; i < base->n_icons; i++) {
            const fastchart_icon *ic = &base->icons[i];
			double frac_x = fastchart_normalize_finite(ic->x,
				xrange.min, xrange.max);
            int px = fastchart_frac_to_px(frac_x, plot.x0, plot.x1);
            int py = fastchart_y_to_pixel(ic->y, &yrange, &plot);
            fastchart_blit_icon(t, ic, px, py);
        }
    }

	/* Build a self-owning image-map artifact from the parsed points. */
    int href_count = 0;
    for (int i = 0; i < n; i++) {
        if (points[i].href) href_count++;
    }
	if (href_count > 0) {
		fastchart_reserve_image_map_areas((fastchart_obj *)self, href_count);
		int k = 0;
        for (int i = 0; i < n; i++) {
            if (!points[i].href) continue;
			double frac_x = fastchart_normalize_finite(points[i].x,
				xrange.min, xrange.max);
            int px = fastchart_frac_to_px(frac_x, plot.x0, plot.x1);
            int py = fastchart_y_to_pixel(points[i].y, &yrange, &plot);
            fastchart_image_map_area *area = &self->image_map_areas[k];
            area->shape = FASTCHART_IMAGE_MAP_CIRCLE;
            area->n_coords = 3;
            area->coords[0] = px;
            area->coords[1] = py;
            int radius = marker_size / 2;
            if (radius < 1) radius = 1;
            area->coords[2] = radius;
            area->href = zend_string_copy(points[i].href);
            area->tooltip = points[i].tooltip
                ? zend_string_copy(points[i].tooltip) : NULL;
            area->orig_index = i;
            k++;
            self->n_image_map_areas = k;
        }
    }
    return 0;
}
